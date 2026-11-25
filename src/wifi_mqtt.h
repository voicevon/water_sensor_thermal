#pragma once

// ============================================================
//  网络通信层（WiFi + MQTT）— 与 water_sensor_hx711 100% 协议兼容
// ============================================================

#include "config.h"

void wifi_mqtt_init();
void wifi_mqtt_loop(unsigned long current_time);
bool mqtt_publish(const int32_t *sensors, uint8_t stateByte);
bool mqtt_is_connected();
bool wifi_is_connected();
