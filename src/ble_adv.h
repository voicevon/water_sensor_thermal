#pragma once

// ============================================================
//  BLE 广播管理层（与 water_sensor_hx711 / water_relay 100% 协议兼容）
// ============================================================

#include "config.h"

void ble_init();
void ble_update(const int32_t *sensors, const bool *states);
