#pragma once

#include <Arduino.h>
#include "config.h"

// ============================================================
//  3路脉冲热式液位传感器实时数据缓存结构
//  兼顾 water_sensor_hx711 /api/data 字段与热式每通道扩展字段
// ============================================================
struct SensorDataCache {
    // 兼容 water_sensor_hx711 通用整型字段（单位：0.01 ℃）
    int32_t raw_val;      // ΔT × 100
    int32_t filtered;     // ΔT × 100
    int32_t baseline;     // 空管标定温升 emptyDeltaT × 100
    int32_t threshold;    // 有液判定阈值 (emptyDeltaT * ratioLiquid) × 100
    bool    detected;     // true = 有液 (ThermalState::LIQUID)

    // 热式通道详细状态字段
    float   t0;           // 加热前基准温度 (℃)
    float   t1;           // 加热后温度 (℃)
    float   delta_t;      // 温升 ΔT = T1 - T0 (℃)
    float   empty_dt;     // NVS 空管标定温升 (℃)
    float   ratio_liquid; // 该通道有液比例阈值
    float   ratio_empty;  // 该通道空管比例阈值
    bool    calibrated;   // 是否已完成空管标定
    bool    calibrating;  // 该通道是否正在标定
    uint8_t state;        // 0=EMPTY, 1=LIQUID, 2=UNCALIBRATED, 3=FAULT
    bool    online;       // 18B20 是否在线
    bool    heating;      // 当前是否正在加热
};

void data_cache_update_sensor(int idx, const SensorDataCache& data);
const SensorDataCache& data_cache_get_sensor(int idx);
