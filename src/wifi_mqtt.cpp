#include "wifi_mqtt.h"
#include "config.h"
#include "nvs_config.h"
#include "ThermalChannel.h"

#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <atomic>

class MqttLock {
public:
    MqttLock(SemaphoreHandle_t mutex) : _mutex(mutex), _locked(false) {
        if (_mutex) {
            _locked = (xSemaphoreTake(_mutex, pdMS_TO_TICKS(3000)) == pdTRUE);
            if (!_locked) {
                Serial.println("[MQTT] WARN: MqttLock timeout (3s), skipping operation.");
            }
        }
    }
    ~MqttLock() {
        if (_mutex && _locked) {
            xSemaphoreGive(_mutex);
        }
    }
    bool locked() const { return _locked; }
private:
    SemaphoreHandle_t _mutex;
    bool _locked;
};

static WiFiClient        s_espClient;
static PubSubClient      s_mqttClient(s_espClient);
static SemaphoreHandle_t s_mqttMutex = NULL;

static IPAddress         s_resolved_broker_ip = IPAddress(0, 0, 0, 0);
static std::atomic<bool> s_mqtt_connecting(false);
static unsigned long     s_last_dns_resolve_ms = 0;
static unsigned long     s_last_mqtt_reconnect_attempt = 0;
static unsigned long     s_last_wifi_reconnect_attempt = 0;
static uint8_t           s_wifi_fail_count = 0;
static bool              s_mqtt_send_enabled = false;

static IPAddress resolve_broker_ip() {
    IPAddress resolvedIP;
    String broker = get_mqtt_broker();
    if (resolvedIP.fromString(broker.c_str())) {
        return resolvedIP;
    }
    if (WiFi.hostByName(broker.c_str(), resolvedIP)) {
        Serial.printf("[DNS] Successfully resolved %s to %s\n",
                      broker.c_str(), resolvedIP.toString().c_str());
        return resolvedIP;
    }
    Serial.printf("[DNS] Standard DNS failed for %s\n", broker.c_str());
    return IPAddress(0, 0, 0, 0);
}

static void mqtt_callback(char* topic, byte* payload, unsigned int length) {
    Serial.print("[MQTT RX] Topic: ");
    Serial.println(topic);

    // 1. 兼容旧控制主题：water/sensor/start
    if (strcmp(topic, MQTT_CONTROL_TOPIC) == 0) {
        char payload_str[128];
        unsigned int len = length < 127 ? length : 127;
        memcpy(payload_str, payload, len);
        payload_str[len] = '\0';

        Serial.print("[MQTT RX] Payload: ");
        Serial.println(payload_str);

        StaticJsonDocument<256> doc;
        DeserializationError error = deserializeJson(doc, payload_str);
        if (error) {
            Serial.print("[MQTT] JSON parse error: ");
            Serial.println(error.c_str());
            return;
        }

        const char* command = doc["command"];
        if (command && strcmp(command, "stop") == 0) {
            s_mqtt_send_enabled = false;
            Serial.println("[MQTT] Global DEBUG DISABLED");
            return;
        }

        const char* name = doc["name"];
        if (name && strcmp(name, get_device_name().c_str()) == 0) {
            const char* action = doc["action"];
            if (action && strcmp(action, "restart") == 0) {
                if (millis() < 15000UL) {
                    Serial.println("[MQTT] WARN: Ignoring restart command within 15s boot cooldown!");
                    return;
                }
                const char* cmd_id = doc["cmd_id"] | "";
                app_all_heaters_off();
                String resp_topic = String("water/") + get_device_name() + "/control/response";
                char ack_buf[256];
                snprintf(ack_buf, sizeof(ack_buf),
                         "{\"cmd_id\":\"%s\",\"from\":\"sensor\",\"action\":\"restart\",\"status\":\"accept\",\"event\":\"restarting\",\"name\":\"%s\"}",
                         cmd_id, get_device_name().c_str());
                s_mqttClient.publish(resp_topic.c_str(), ack_buf);
                delay(300);
                ESP.restart();
                return;
            }

            s_mqtt_send_enabled = true;
            int interval_sec = doc["interval"] | 1;
            if (interval_sec < 1) interval_sec = 1;

            extern uint32_t g_mqtt_publish_interval_ms;
            g_mqtt_publish_interval_ms = (uint32_t)interval_sec * 1000UL;

            extern unsigned long s_last_mqtt_publish_time;
            s_last_mqtt_publish_time = 0;

            Serial.printf("[MQTT] DEBUG ENABLED, interval: %d s\n", interval_sec);
        }
    }

    // 2. 匹配统一控制主题：water/{device_name}/control
    String station_ctrl_topic = String("water/") + get_device_name() + "/control";
    if (strcmp(topic, station_ctrl_topic.c_str()) == 0) {
        char payload_str[256];
        unsigned int len = length < 255 ? length : 255;
        memcpy(payload_str, payload, len);
        payload_str[len] = '\0';

        Serial.printf("[MQTT Control RX] Payload: %s\n", payload_str);

        StaticJsonDocument<256> doc;
        DeserializationError error = deserializeJson(doc, payload_str);
        if (error) {
            Serial.printf("[MQTT Control] JSON parse error: %s\n", error.c_str());
            return;
        }

        const char* target = doc["target"];
        if (!target || (strcmp(target, "sensor") != 0 && strcmp(target, "all") != 0)) {
            return;
        }

        const char* action = doc["action"];
        if (action && strcmp(action, "restart") == 0) {
            if (millis() < 15000UL) {
                Serial.println("[MQTT Control] WARN: Ignoring restart command within 15s boot cooldown!");
                return;
            }
            const char* cmd_id = doc["cmd_id"] | "";
            app_all_heaters_off();
            String resp_topic = String("water/") + get_device_name() + "/control/response";
            char ack_buf[256];
            snprintf(ack_buf, sizeof(ack_buf),
                     "{\"cmd_id\":\"%s\",\"from\":\"sensor\",\"action\":\"restart\",\"status\":\"accept\",\"event\":\"restarting\",\"name\":\"%s\"}",
                     cmd_id, get_device_name().c_str());
            s_mqttClient.publish(resp_topic.c_str(), ack_buf);
            delay(300);
            ESP.restart();
            return;
        }

        if (action && strcmp(action, "calibrate") == 0) {
            const char* cmd_id = doc["cmd_id"] | "";
            int ch = doc["ch"] | -1;
            app_start_calibration(ch);
            String resp_topic = String("water/") + get_device_name() + "/control/response";
            char ack_buf[256];
            snprintf(ack_buf, sizeof(ack_buf),
                     "{\"cmd_id\":\"%s\",\"from\":\"sensor\",\"action\":\"calibrate\",\"status\":\"accept\",\"event\":\"calibrating\",\"name\":\"%s\"}",
                     cmd_id, get_device_name().c_str());
            s_mqttClient.publish(resp_topic.c_str(), ack_buf);
            return;
        }
    }
}

static void mqtt_connect_task(void* pvParameters) {
    (void)pvParameters;
    unsigned long now = millis();
    if (s_resolved_broker_ip[0] == 0 || (now - s_last_dns_resolve_ms > 300000UL)) {
        s_last_dns_resolve_ms = now;
        IPAddress tempIP = resolve_broker_ip();
        if (tempIP[0] != 0) {
            s_resolved_broker_ip = tempIP;
        }
    }

    {
        MqttLock lock(s_mqttMutex);
        if (lock.locked()) {
            if (s_resolved_broker_ip[0] != 0) {
                s_mqttClient.setServer(s_resolved_broker_ip, get_mqtt_port());
            } else {
                s_mqttClient.setServer(get_mqtt_broker().c_str(), get_mqtt_port());
            }

            String clientId = "ESP32Client-" + String(random(0xffff), HEX);
            Serial.println("[MQTT Task] Attempting connection to Broker...");

            if (s_mqttClient.connect(clientId.c_str())) {
                Serial.println("[MQTT Task] Connected successfully!");
                s_mqttClient.subscribe(MQTT_CONTROL_TOPIC);

                String station_ctrl_topic = String("water/") + get_device_name() + "/control";
                s_mqttClient.subscribe(station_ctrl_topic.c_str());

                String station_resp_topic = String("water/") + get_device_name() + "/control/response";
                char boot_msg[128];
                snprintf(boot_msg, sizeof(boot_msg),
                         "{\"from\":\"sensor\",\"event\":\"online\",\"status\":\"ready\",\"name\":\"%s\"}",
                         get_device_name().c_str());
                s_mqttClient.publish(station_resp_topic.c_str(), boot_msg);
            } else {
                Serial.printf("[MQTT Task] Connection failed, state = %d\n", s_mqttClient.state());
                s_resolved_broker_ip = IPAddress(0, 0, 0, 0);
                s_espClient.stop();
            }
        }
    }

    s_mqtt_connecting = false;
    vTaskDelete(NULL);
}

void wifi_mqtt_init() {
    if (s_mqttMutex == NULL) {
        s_mqttMutex = xSemaphoreCreateMutex();
    }

    s_mqttClient.setBufferSize(MQTT_BUFFER_SIZE);
    s_mqttClient.setCallback(mqtt_callback);

    String target_ssid = get_sta_ssid();
    String target_pass = get_sta_password();
    Serial.printf("\n[WiFi] Target SSID to connect: \"%s\"\n", target_ssid.c_str());

    WiFi.mode(WIFI_AP_STA);
    WiFi.setAutoReconnect(false);
    WiFi.begin(target_ssid.c_str(), target_pass.c_str());

    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
        delay(500);
        Serial.print(".");
        attempts++;
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.printf("\n[WiFi] Connected. IP: %s\n", WiFi.localIP().toString().c_str());
    } else {
        Serial.println("\n[WiFi] Connect failed. Will retry in background.");
    }

    IPAddress brokerIP = resolve_broker_ip();
    int port = get_mqtt_port();
    String broker = get_mqtt_broker();
    if (brokerIP[0] != 0) {
        s_resolved_broker_ip = brokerIP;
        s_last_dns_resolve_ms = millis();
        s_mqttClient.setServer(brokerIP, port);
    } else {
        s_mqttClient.setServer(broker.c_str(), port);
    }
}

void wifi_mqtt_loop(unsigned long current_time) {
    if (WiFi.status() != WL_CONNECTED) {
        uint8_t shift = s_wifi_fail_count < WIFI_RECONNECT_BACKOFF_MAX_SHIFT
                        ? s_wifi_fail_count : WIFI_RECONNECT_BACKOFF_MAX_SHIFT;
        unsigned long interval = WIFI_RECONNECT_BASE_MS << shift;
        if (current_time - s_last_wifi_reconnect_attempt >= interval) {
            s_last_wifi_reconnect_attempt = current_time;
            s_wifi_fail_count++;
            Serial.printf("[WiFi] Disconnected. Reconnecting (fail=%u, next in %lus)...\n",
                          s_wifi_fail_count, interval / 1000UL);
            WiFi.begin(get_sta_ssid().c_str(), get_sta_password().c_str());
        }
        return;
    } else {
        s_last_wifi_reconnect_attempt = current_time;
        s_wifi_fail_count = 0;
    }

    if (s_mqtt_connecting) return;

    bool connected = false;
    {
        MqttLock lock(s_mqttMutex);
        if (lock.locked()) connected = s_mqttClient.connected();
    }

    if (!connected) {
        s_resolved_broker_ip = IPAddress(0, 0, 0, 0);
        if (current_time - s_last_mqtt_reconnect_attempt >= MQTT_RECONNECT_INTERVAL_MS) {
            s_last_mqtt_reconnect_attempt = current_time;
            s_mqtt_connecting = true;
            if (xTaskCreate(mqtt_connect_task, "mqtt_async_conn", 8192, NULL, 1, NULL) != pdPASS) {
                s_mqtt_connecting = false;
            }
        }
    } else {
        MqttLock lock(s_mqttMutex);
        if (lock.locked()) s_mqttClient.loop();
    }
}

static inline float round2(float v) {
    return roundf(v * 100.0f) / 100.0f;
}

static inline float round3(float v) {
    return roundf(v * 1000.0f) / 1000.0f;
}

bool mqtt_publish_thermal(const SensorDataCache* channels,
                          float bus_v5,
                          bool bus_v5_ok,
                          uint8_t active_ch,
                          bool calibrating) {
    if (!s_mqtt_send_enabled || s_mqtt_connecting || !channels) return false;

    MqttLock lock(s_mqttMutex);
    if (!lock.locked() || !s_mqttClient.connected()) return false;

    StaticJsonDocument<1024> doc;
    doc["ver"]         = MQTT_PROTOCOL_VERSION;
    doc["name"]        = get_device_name();
    doc["bus_v5"]      = round2(bus_v5);
    doc["bus_v5_ok"]   = bus_v5_ok;
    doc["active_ch"]   = active_ch;
    doc["calibrating"] = calibrating;

    JsonArray arr = doc.createNestedArray("channels");
    for (int i = 0; i < SENSOR_COUNT; i++) {
        const SensorDataCache& c = channels[i];
        JsonObject ch = arr.createNestedObject();
        ch["ch"] = i + 1;
        if (isnan(c.t_now) || isinf(c.t_now)) ch["t_now"] = nullptr;
        else                                   ch["t_now"] = round2(c.t_now);
        ch["heating"]      = c.heating;
        if (c.calibrated)  ch["cool_ratio"] = round3(c.cool_ratio);
        else               ch["cool_ratio"] = nullptr;
        ch["state"]        = c.state;
        ch["probe_type"]   = c.probe_type;
        ch["online"]       = c.online;
        if (isnan(c.t0) || isinf(c.t0))         ch["t0"]     = nullptr;
        else                                    ch["t0"]     = round2(c.t0);
        if (isnan(c.t_peak) || isinf(c.t_peak)) ch["t_peak"] = nullptr;
        else                                    ch["t_peak"] = round2(c.t_peak);
        ch["delta_t"]      = round2(c.delta_t);
        ch["drop_mid"]     = round2(c.drop10);
        ch["drop_end"]     = round2(c.drop30);
        ch["ratio_liquid"] = round2(c.ratio_liquid);
        ch["ratio_empty"]  = round2(c.ratio_empty);
    }

    char json_buf[MQTT_BUFFER_SIZE];
    size_t n = serializeJson(doc, json_buf, sizeof(json_buf));
    if (n == 0 || n >= sizeof(json_buf)) {
        Serial.println("[MQTT ERR] Thermal v2 JSON serialization overflow");
        return false;
    }

    Serial.printf("[MQTT Publish] Topic: %s (%uB)\n", MQTT_STATUS_TOPIC, (unsigned)n);
    return s_mqttClient.publish(MQTT_STATUS_TOPIC, json_buf);
}

bool mqtt_is_connected() {
    if (s_mqtt_connecting) return false;
    MqttLock lock(s_mqttMutex);
    return lock.locked() && s_mqttClient.connected();
}

bool wifi_is_connected() {
    return WiFi.status() == WL_CONNECTED;
}
