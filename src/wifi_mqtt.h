#pragma once

// ============================================================
//  网络通信层（WiFi + MQTT）— Thermal Pulse v2 JSON 协议
// ============================================================

#include "config.h"
#include "data_cache.h"

void wifi_mqtt_init();
void wifi_mqtt_loop(unsigned long current_time);
bool mqtt_publish_thermal(const SensorDataCache* channels,
                          float bus_v5,
                          bool bus_v5_ok,
                          uint8_t active_ch,
                          bool calibrating);
bool mqtt_is_connected();
bool wifi_is_connected();
