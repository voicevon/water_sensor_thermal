#pragma once

#include <Arduino.h>

/**
 * @brief 初始化 NVS 存储并加载所有配置项到内存缓存
 *        由 web_config_init() 调用
 */
void nvs_config_init();

// ---- 基础网络与 MQTT 配置（与 water_sensor_hx711 完全兼容） ----
bool   nvs_set_sta_ssid(const String& val);
bool   nvs_set_sta_password(const String& val);
bool   nvs_set_device_name(const String& val);
bool   nvs_set_mqtt_broker(const String& val);
bool   nvs_set_mqtt_port(int val);

String get_sta_ssid();
String get_sta_password();
String get_device_name();
String get_mqtt_broker();
int    get_mqtt_port();

// ---- 每通道独立热式封装类型、标定与迟滞比例阈值配置（ch: 0~2） ----
bool    nvs_set_probe_type(int ch, uint8_t probe_type);
uint8_t get_probe_type(int ch);

bool    nvs_set_empty_delta_t(int ch, float val);
float   get_empty_delta_t(int ch);

bool    nvs_set_channel_ratios(int ch, float ratio_liquid, float ratio_empty);
float   get_channel_ratio_liquid(int ch);
float   get_channel_ratio_empty(int ch);
