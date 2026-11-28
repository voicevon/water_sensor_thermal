#include "web_config.h"
#include "nvs_config.h"
#include "data_cache.h"
#include "index_html.h"
#include "config.h"
#include "wifi_mqtt.h"
#include "ThermalChannel.h"

#include <WiFi.h>
#include <WebServer.h>
#include <esp_task_wdt.h>
#include <math.h>

static WebServer s_server(80);

static void print_wifi_status(const char* label) {
    wifi_mode_t mode = WiFi.getMode();
    const char* mode_str = "UNKNOWN";
    if (mode == WIFI_OFF)         mode_str = "OFF";
    else if (mode == WIFI_STA)    mode_str = "STA";
    else if (mode == WIFI_AP)     mode_str = "AP";
    else if (mode == WIFI_AP_STA) mode_str = "AP_STA";

    Serial.printf("[%s] Mode: %s, AP IP: %s, STA IP: %s, Status: %d\n",
                  label, mode_str,
                  WiFi.softAPIP().toString().c_str(),
                  WiFi.localIP().toString().c_str(),
                  WiFi.status());
}

static String json_escape(const String& s) {
    String out;
    out.reserve(s.length() + 8);
    for (unsigned int i = 0; i < s.length(); i++) {
        char c = s[i];
        if (c == '"' || c == '\\') { out += '\\'; out += c; }
        else if ((unsigned char)c >= 0x20) out += c;
    }
    return out;
}

static String format_float_or_null(float v, int decimals = 2) {
    if (isnan(v) || isinf(v)) return "null";
    return String(v, decimals);
}

// GET /api/data — 兼容 water_sensor_hx711 并扩展每通道热式字段
static void handle_get_data() {
    String json = "{\"sensors\":[";
    for (int i = 0; i < SENSOR_COUNT; i++) {
        const SensorDataCache& s = data_cache_get_sensor(i);
        json += "{";
        json += "\"raw_val\":"      + String(s.raw_val) + ",";
        json += "\"filtered\":"     + String(s.filtered) + ",";
        json += "\"baseline\":"     + String(s.baseline) + ",";
        json += "\"threshold\":"    + String(s.threshold) + ",";
        json += "\"detected\":"     + String(s.detected ? "true" : "false") + ",";
        json += "\"t_now\":"        + format_float_or_null(s.t_now, 2) + ",";
        json += "\"t0\":"           + format_float_or_null(s.t0, 2) + ",";
        json += "\"t1\":"           + format_float_or_null(s.t1, 2) + ",";
        json += "\"t_peak\":"       + format_float_or_null(s.t_peak, 2) + ",";
        json += "\"delta_t\":"      + String(s.delta_t, 2) + ",";
        json += "\"drop10\":"       + String(s.drop10, 2) + ",";
        json += "\"drop30\":"       + String(s.drop30, 2) + ",";
        json += "\"cool_ratio\":"   + String(s.cool_ratio, 4) + ",";
        json += "\"empty_dt\":"     + String(s.empty_dt, 3) + ",";
        json += "\"ratio_liquid\":" + String(s.ratio_liquid, 2) + ",";
        json += "\"ratio_empty\":"  + String(s.ratio_empty, 2) + ",";
        json += "\"calibrated\":"   + String(s.calibrated ? "true" : "false") + ",";
        json += "\"calibrating\":"  + String(s.calibrating ? "true" : "false") + ",";
        json += "\"state\":"        + String(s.state) + ",";
        json += "\"state_str\":\""  + String(ThermalChannel::stateToString((ThermalState)s.state)) + "\",";
        json += "\"probe_type\":"   + String(s.probe_type) + ",";
        json += "\"online\":"       + String(s.online ? "true" : "false") + ",";
        json += "\"heating\":"      + String(s.heating ? "true" : "false");
        json += "}";
        if (i < SENSOR_COUNT - 1) json += ",";
    }
    json += "],";
    json += "\"bus_v5\":"         + String(app_get_bus_voltage_5v(), 2) + ",";
    json += "\"calibrating\":"    + String(app_is_any_calibrating() ? "true" : "false") + ",";
    json += "\"active_ch\":"      + String(app_get_active_channel()) + ",";
    json += "\"wifi_connected\":" + String(wifi_is_connected() ? "true" : "false") + ",";
    json += "\"mqtt_connected\":" + String(mqtt_is_connected() ? "true" : "false");
    json += "}";
    s_server.send(200, "application/json", json);
}

// GET /api/sysconfig
static void handle_get_sysconfig() {
    String json = "{";
    json += "\"ssid\":\""   + json_escape(get_sta_ssid()) + "\",";
    json += "\"pass\":\""   + json_escape(get_sta_password()) + "\",";
    json += "\"name\":\""   + json_escape(get_device_name()) + "\",";
    json += "\"broker\":\"" + json_escape(get_mqtt_broker()) + "\",";
    json += "\"port\":"     + String(get_mqtt_port());
    json += "}";
    s_server.send(200, "application/json", json);
}

// POST /api/sysconfig
static void handle_post_sysconfig() {
    bool changed = false;
    if (s_server.hasArg("ssid"))   changed |= nvs_set_sta_ssid(s_server.arg("ssid"));
    if (s_server.hasArg("password") && s_server.arg("password").length() > 0)
                                   changed |= nvs_set_sta_password(s_server.arg("password"));
    if (s_server.hasArg("name"))   changed |= nvs_set_device_name(s_server.arg("name"));
    if (s_server.hasArg("broker")) changed |= nvs_set_mqtt_broker(s_server.arg("broker"));
    if (s_server.hasArg("port"))   changed |= nvs_set_mqtt_port(s_server.arg("port").toInt());

    if (changed) {
        Serial.println("[WebConfig] System configurations updated in NVS.");
        WiFi.begin(get_sta_ssid().c_str(), get_sta_password().c_str());
    }
    s_server.send(200, "text/plain", "OK");
}

// GET /api/scan
static void handle_wifi_scan() {
    esp_task_wdt_reset();
    int n = WiFi.scanNetworks(false, false);
    esp_task_wdt_reset();

    String json = "{\"networks\":[";
    if (n > 0) {
        int indices[n];
        for (int i = 0; i < n; i++) indices[i] = i;
        for (int i = 0; i < n - 1; i++) {
            for (int j = i + 1; j < n; j++) {
                if (WiFi.RSSI(indices[j]) > WiFi.RSSI(indices[i])) {
                    int temp = indices[i];
                    indices[i] = indices[j];
                    indices[j] = temp;
                }
            }
        }
        for (int i = 0; i < n; i++) {
            int idx = indices[i];
            json += "{";
            json += "\"ssid\":\"" + json_escape(WiFi.SSID(idx)) + "\",";
            json += "\"rssi\":"   + String(WiFi.RSSI(idx));
            json += "}";
            if (i < n - 1) json += ",";
        }
    }
    json += "]}";
    WiFi.scanDelete();
    s_server.send(200, "application/json", json);
}

// GET /api/hx711 — 兼容旧接口，返回各通道 18B20 在线状态
static void handle_get_hx711() {
    String json = "{\"shift_n\":0,\"channels\":[";
    for (int i = 0; i < SENSOR_COUNT; i++) {
        json += "{";
        json += "\"ch\":" + String(i) + ",";
        json += "\"online\":" + String(app_is_channel_online(i) ? "true" : "false");
        json += "}";
        if (i < SENSOR_COUNT - 1) json += ",";
    }
    json += "]}";
    s_server.send(200, "application/json", json);
}

// POST /api/calibrate — 支持全通道 (ch=-1 或省略) 或单通道 (ch=0~2) 标定/重置
static void handle_post_calibrate() {
    String action = s_server.hasArg("action") ? s_server.arg("action") : "";
    int ch = s_server.hasArg("ch") ? s_server.arg("ch").toInt() : -1;
    if (ch >= SENSOR_COUNT) {
        s_server.send(400, "text/plain", "Invalid ch");
        return;
    }

    if (action == "start") {
        app_start_calibration(ch);
        s_server.send(200, "text/plain", "Calibration started");
    } else if (action == "reset") {
        app_reset_calibration(ch);
        s_server.send(200, "text/plain", "Calibration cleared");
    } else {
        s_server.send(400, "text/plain", "Invalid action (use start or reset)");
    }
}

// POST /api/thermal — 每通道独立配置封装类型（probe_type）和/或迟滞阈值（ratio_liquid, ratio_empty）
static void handle_post_thermal() {
    if (!s_server.hasArg("ch")) {
        s_server.send(400, "text/plain", "Missing ch");
        return;
    }
    int ch = s_server.arg("ch").toInt();
    if (ch < 0 || ch >= SENSOR_COUNT) {
        s_server.send(400, "text/plain", "Invalid ch");
        return;
    }

    bool handled = false;
    if (s_server.hasArg("probe_type")) {
        uint8_t pt = (uint8_t)s_server.arg("probe_type").toInt();
        nvs_set_probe_type(ch, pt);
        app_sync_channel_probe_type(ch);
        Serial.printf("[WebConfig] CH%d probe_type updated -> %s\n",
                      ch + 1, pt == PROBE_CERAMIC ? "CERAMIC" : "STEEL");
        handled = true;
    }

    if (s_server.hasArg("ratio_liquid") && s_server.hasArg("ratio_empty")) {
        float rl = s_server.arg("ratio_liquid").toFloat();
        float re = s_server.arg("ratio_empty").toFloat();
        if (!nvs_set_channel_ratios(ch, rl, re)) {
            s_server.send(400, "text/plain", "Invalid ratios (require 0.1 < empty < liquid < 0.95)");
            return;
        }
        app_sync_channel_ratios(ch);
        Serial.printf("[WebConfig] CH%d coolRatio thresholds updated: LIQUID>%.2f, EMPTY<%.2f\n", ch + 1, rl, re);
        handled = true;
    }

    if (!handled) {
        s_server.send(400, "text/plain", "Missing probe_type or ratio_liquid/ratio_empty");
        return;
    }
    s_server.send(200, "text/plain", "OK");
}

void web_config_init() {
    nvs_config_init();

    print_wifi_status("WebConfig BEFORE softAP");
    WiFi.mode(WIFI_AP_STA);
    WiFi.softAP(FACTORY_WIFI_AP_SSID, FACTORY_WIFI_AP_PASSWORD);
    print_wifi_status("WebConfig AFTER softAP");

    s_server.on("/", HTTP_GET, []() {
        s_server.send_P(200, "text/html", INDEX_HTML);
    });
    s_server.on("/api/data",      HTTP_GET,  handle_get_data);
    s_server.on("/api/sysconfig", HTTP_GET,  handle_get_sysconfig);
    s_server.on("/api/sysconfig", HTTP_POST, handle_post_sysconfig);
    s_server.on("/api/wifi",      HTTP_GET,  handle_get_sysconfig);
    s_server.on("/api/wifi",      HTTP_POST, handle_post_sysconfig);
    s_server.on("/api/scan",      HTTP_GET,  handle_wifi_scan);
    s_server.on("/api/hx711",     HTTP_GET,  handle_get_hx711);
    s_server.on("/api/calibrate", HTTP_POST, handle_post_calibrate);
    s_server.on("/api/thermal",   HTTP_POST, handle_post_thermal);

    s_server.begin();
    Serial.println("[WebConfig] Embedded Web Server started on port 80");
}

void web_config_loop() {
    if (WiFi.getMode() == WIFI_STA) {
        Serial.println("[WebConfig] WiFi mode reverted to STA. Restoring AP_STA...");
        WiFi.mode(WIFI_AP_STA);
        WiFi.softAP(FACTORY_WIFI_AP_SSID, FACTORY_WIFI_AP_PASSWORD);
        print_wifi_status("WebConfig RESTORED AP_STA");
    }
    s_server.handleClient();
}

void web_config_update_sensor(int idx, const SensorDataCache& data) {
    data_cache_update_sensor(idx, data);
}
