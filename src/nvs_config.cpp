#include "nvs_config.h"
#include "config.h"
#include <Preferences.h>

static Preferences s_prefs;

static const char NVS_NAMESPACE[]  = "thermal_cfg";
static const char NVS_KEY_SSID[]   = "sta_ssid";
static const char NVS_KEY_PASS[]   = "sta_pass";
static const char NVS_KEY_NAME[]   = "sta_name";
static const char NVS_KEY_BROKER[] = "mqtt_broker";
static const char NVS_KEY_PORT[]   = "mqtt_port";

static String s_sta_ssid     = "";
static String s_sta_password = "";
static String s_device_name  = "";
static String s_mqtt_broker  = "";
static int    s_mqtt_port    = FACTORY_MQTT_PORT;

// 每通道独立热式参数缓存
static float s_empty_dt[SENSOR_COUNT]     = { 0.0f, 0.0f, 0.0f };
static float s_ratio_liquid[SENSOR_COUNT] = { DEFAULT_RATIO_LIQUID, DEFAULT_RATIO_LIQUID, DEFAULT_RATIO_LIQUID };
static float s_ratio_empty[SENSOR_COUNT]  = { DEFAULT_RATIO_EMPTY,  DEFAULT_RATIO_EMPTY,  DEFAULT_RATIO_EMPTY  };

void nvs_config_init() {
    s_prefs.begin(NVS_NAMESPACE, false);

    s_sta_ssid     = s_prefs.getString(NVS_KEY_SSID,   FACTORY_WIFI_SSID);
    s_sta_password = s_prefs.getString(NVS_KEY_PASS,   FACTORY_WIFI_PASSWORD);
    s_device_name  = s_prefs.getString(NVS_KEY_NAME,   FACTORY_DEVICE_NAME);
    s_mqtt_broker  = s_prefs.getString(NVS_KEY_BROKER, FACTORY_MQTT_BROKER);
    s_mqtt_port    = s_prefs.getInt(NVS_KEY_PORT,      FACTORY_MQTT_PORT);

    for (int i = 0; i < SENSOR_COUNT; i++) {
        char kEmp[12], kLiq[12], kRe[12];
        snprintf(kEmp, sizeof(kEmp), "emp_dt_%d", i);
        snprintf(kLiq, sizeof(kLiq), "r_liq_%d",  i);
        snprintf(kRe,  sizeof(kRe),  "r_emp_%d",  i);

        float v = s_prefs.getFloat(kEmp, 0.0f);
        s_empty_dt[i] = (v > 2.0f && v < 40.0f) ? v : 0.0f;

        float rl = s_prefs.getFloat(kLiq, DEFAULT_RATIO_LIQUID);
        float re = s_prefs.getFloat(kRe,  DEFAULT_RATIO_EMPTY);
        if (rl > 0.1f && re < 0.95f && rl < re) {
            s_ratio_liquid[i] = rl;
            s_ratio_empty[i]  = re;
        } else {
            s_ratio_liquid[i] = DEFAULT_RATIO_LIQUID;
            s_ratio_empty[i]  = DEFAULT_RATIO_EMPTY;
        }
    }

    Serial.printf("[NvsConfig] WiFi STA: %s, Device: %s, MQTT: %s:%d\n",
                  s_sta_ssid.c_str(), s_device_name.c_str(),
                  s_mqtt_broker.c_str(), s_mqtt_port);
    for (int i = 0; i < SENSOR_COUNT; i++) {
        Serial.printf("[NvsConfig] CH%d emp_dT=%.2f, ratio=[<%.2f / >%.2f]\n",
                      i + 1, s_empty_dt[i], s_ratio_liquid[i], s_ratio_empty[i]);
    }
}

String get_sta_ssid()     { return s_sta_ssid; }
String get_sta_password() { return s_sta_password; }
String get_device_name()  { return s_device_name; }
String get_mqtt_broker()  { return s_mqtt_broker; }
int    get_mqtt_port()    { return s_mqtt_port; }

float get_empty_delta_t(int ch) {
    return (ch >= 0 && ch < SENSOR_COUNT) ? s_empty_dt[ch] : 0.0f;
}

float get_channel_ratio_liquid(int ch) {
    return (ch >= 0 && ch < SENSOR_COUNT) ? s_ratio_liquid[ch] : DEFAULT_RATIO_LIQUID;
}

float get_channel_ratio_empty(int ch) {
    return (ch >= 0 && ch < SENSOR_COUNT) ? s_ratio_empty[ch] : DEFAULT_RATIO_EMPTY;
}

bool nvs_set_sta_ssid(const String& val) {
    if (val.length() == 0 || val == s_sta_ssid) return false;
    s_sta_ssid = val;
    s_prefs.putString(NVS_KEY_SSID, val);
    return true;
}

bool nvs_set_sta_password(const String& val) {
    if (val == s_sta_password) return false;
    s_sta_password = val;
    s_prefs.putString(NVS_KEY_PASS, val);
    return true;
}

bool nvs_set_device_name(const String& val) {
    if (val.length() == 0 || val == s_device_name) return false;
    s_device_name = val;
    s_prefs.putString(NVS_KEY_NAME, val);
    return true;
}

bool nvs_set_mqtt_broker(const String& val) {
    if (val.length() == 0 || val == s_mqtt_broker) return false;
    s_mqtt_broker = val;
    s_prefs.putString(NVS_KEY_BROKER, val);
    return true;
}

bool nvs_set_mqtt_port(int val) {
    if (val <= 0 || val > 65535 || val == s_mqtt_port) return false;
    s_mqtt_port = val;
    s_prefs.putInt(NVS_KEY_PORT, val);
    return true;
}

bool nvs_set_empty_delta_t(int ch, float val) {
    if (ch < 0 || ch >= SENSOR_COUNT) return false;
    if (val != 0.0f && (val <= 2.0f || val >= 40.0f)) return false;
    s_empty_dt[ch] = val;
    char key[12];
    snprintf(key, sizeof(key), "emp_dt_%d", ch);
    s_prefs.putFloat(key, val);
    return true;
}

bool nvs_set_channel_ratios(int ch, float ratio_liquid, float ratio_empty) {
    if (ch < 0 || ch >= SENSOR_COUNT) return false;
    if (!(ratio_liquid > 0.1f && ratio_empty < 0.95f && ratio_liquid < ratio_empty)) {
        return false;
    }
    s_ratio_liquid[ch] = ratio_liquid;
    s_ratio_empty[ch]  = ratio_empty;
    char kLiq[12], kRe[12];
    snprintf(kLiq, sizeof(kLiq), "r_liq_%d", ch);
    snprintf(kRe,  sizeof(kRe),  "r_emp_%d", ch);
    s_prefs.putFloat(kLiq, ratio_liquid);
    s_prefs.putFloat(kRe,  ratio_empty);
    return true;
}
