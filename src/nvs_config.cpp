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
static uint8_t s_probe_type[SENSOR_COUNT]   = { DEFAULT_PROBE_TYPE_0, DEFAULT_PROBE_TYPE_1, DEFAULT_PROBE_TYPE_2 };
static float   s_empty_dt[SENSOR_COUNT]     = { 0.0f, 0.0f, 0.0f };
static float   s_ratio_liquid[SENSOR_COUNT] = { STEEL_RATIO_LIQUID, STEEL_RATIO_LIQUID, CERAMIC_RATIO_LIQUID };
static float   s_ratio_empty[SENSOR_COUNT]  = { STEEL_RATIO_EMPTY,  STEEL_RATIO_EMPTY,  CERAMIC_RATIO_EMPTY  };

static float default_ratio_liquid_for(uint8_t pt) {
    return (pt == PROBE_CERAMIC) ? CERAMIC_RATIO_LIQUID : STEEL_RATIO_LIQUID;
}
static float default_ratio_empty_for(uint8_t pt) {
    return (pt == PROBE_CERAMIC) ? CERAMIC_RATIO_EMPTY : STEEL_RATIO_EMPTY;
}

void nvs_config_init() {
    s_prefs.begin(NVS_NAMESPACE, false);

    s_sta_ssid     = s_prefs.getString(NVS_KEY_SSID,   FACTORY_WIFI_SSID);
    s_sta_password = s_prefs.getString(NVS_KEY_PASS,   FACTORY_WIFI_PASSWORD);
    s_device_name  = s_prefs.getString(NVS_KEY_NAME,   FACTORY_DEVICE_NAME);
    s_mqtt_broker  = s_prefs.getString(NVS_KEY_BROKER, FACTORY_MQTT_BROKER);
    s_mqtt_port    = s_prefs.getInt(NVS_KEY_PORT,      FACTORY_MQTT_PORT);

    const uint8_t factory_pt[SENSOR_COUNT] = { DEFAULT_PROBE_TYPE_0, DEFAULT_PROBE_TYPE_1, DEFAULT_PROBE_TYPE_2 };
    for (int i = 0; i < SENSOR_COUNT; i++) {
        char kPt[12], kEmp[12], kLiq[12], kRe[12];
        snprintf(kPt,  sizeof(kPt),  "p_type_%d",  i);
        snprintf(kEmp, sizeof(kEmp), "c_emp_r_%d", i);
        snprintf(kLiq, sizeof(kLiq), "c_liq_%d",   i);
        snprintf(kRe,  sizeof(kRe),  "c_emp_%d",   i);

        uint8_t pt = s_prefs.getUChar(kPt, factory_pt[i]);
        s_probe_type[i] = (pt == PROBE_CERAMIC) ? PROBE_CERAMIC : PROBE_STEEL;

        float v = s_prefs.getFloat(kEmp, 0.0f);
        s_empty_dt[i] = (v > 0.15f && v < 0.80f) ? v : 0.0f;

        float defLiq = default_ratio_liquid_for(s_probe_type[i]);
        float defEmp = default_ratio_empty_for(s_probe_type[i]);
        float rl = s_prefs.getFloat(kLiq, defLiq);
        float re = s_prefs.getFloat(kRe,  defEmp);
        if (re > 0.10f && rl < 0.95f && re < rl) {
            s_ratio_liquid[i] = rl;
            s_ratio_empty[i]  = re;
        } else {
            s_ratio_liquid[i] = defLiq;
            s_ratio_empty[i]  = defEmp;
        }
    }

    Serial.printf("[NvsConfig] WiFi STA: %s, Device: %s, MQTT: %s:%d\n",
                  s_sta_ssid.c_str(), s_device_name.c_str(),
                  s_mqtt_broker.c_str(), s_mqtt_port);
    for (int i = 0; i < SENSOR_COUNT; i++) {
        Serial.printf("[NvsConfig] CH%d type=%s empty_ref=%.3f, coolRatio=[LIQUID>%.2f / EMPTY<%.2f]\n",
                      i + 1, s_probe_type[i] == PROBE_CERAMIC ? "CERAMIC" : "STEEL",
                      s_empty_dt[i], s_ratio_liquid[i], s_ratio_empty[i]);
    }
}

String get_sta_ssid()     { return s_sta_ssid; }
String get_sta_password() { return s_sta_password; }
String get_device_name()  { return s_device_name; }
String get_mqtt_broker()  { return s_mqtt_broker; }
int    get_mqtt_port()    { return s_mqtt_port; }

uint8_t get_probe_type(int ch) {
    return (ch >= 0 && ch < SENSOR_COUNT) ? s_probe_type[ch] : PROBE_STEEL;
}

bool nvs_set_probe_type(int ch, uint8_t probe_type) {
    if (ch < 0 || ch >= SENSOR_COUNT) return false;
    uint8_t pt = (probe_type == PROBE_CERAMIC) ? PROBE_CERAMIC : PROBE_STEEL;
    s_probe_type[ch] = pt;
    char kPt[12];
    snprintf(kPt, sizeof(kPt), "p_type_%d", ch);
    s_prefs.putUChar(kPt, pt);
    // 切换封装类型时同步重置为该封装的推荐默认阈值
    nvs_set_empty_delta_t(ch, 0.0f);
    nvs_set_channel_ratios(ch, default_ratio_liquid_for(pt), default_ratio_empty_for(pt));
    return true;
}

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
    if (val != 0.0f && (val <= 0.15f || val >= 0.80f)) return false;
    s_empty_dt[ch] = val;
    char key[12];
    snprintf(key, sizeof(key), "c_emp_r_%d", ch);
    s_prefs.putFloat(key, val);
    return true;
}

bool nvs_set_channel_ratios(int ch, float ratio_liquid, float ratio_empty) {
    if (ch < 0 || ch >= SENSOR_COUNT) return false;
    if (!(ratio_empty > 0.10f && ratio_liquid < 0.95f && ratio_empty < ratio_liquid)) {
        return false;
    }
    s_ratio_liquid[ch] = ratio_liquid;
    s_ratio_empty[ch]  = ratio_empty;
    char kLiq[12], kRe[12];
    snprintf(kLiq, sizeof(kLiq), "c_liq_%d", ch);
    snprintf(kRe,  sizeof(kRe),  "c_emp_%d", ch);
    s_prefs.putFloat(kLiq, ratio_liquid);
    s_prefs.putFloat(kRe,  ratio_empty);
    return true;
}
