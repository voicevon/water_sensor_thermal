#pragma once

#include <Arduino.h>
#include "config.h"

// ============================================================
//  3路脉冲热式液位传感器实时数据缓存结构
//  兼顾 water_sensor_hx711 /api/data 字段与热式每通道扩展字段
// ============================================================
struct SensorDataCache {
    // 兼容 water_sensor_hx711 通用整型字段（冷却衰减比 × 1000，单位 0.1%）
    int32_t raw_val;      // cool_ratio × 1000 (如 597 = 59.7%)
    int32_t filtered;     // cool_ratio × 1000
    int32_t baseline;     // 空管阈值 ratio_empty × 1000
    int32_t threshold;    // 有液阈值 ratio_liquid × 1000
    bool    detected;     // true = 有液 (ThermalState::LIQUID)

    // 热式通道详细状态字段
    float   t_now;        // 1Hz 实时温度 (℃)
    float   t0;           // 加热前基准温度 (℃)
    float   t1;           // 加热结束温度 (℃)
    float   t_peak;       // 断电后追踪到的峰值温度 (℃)
    float   delta_t;      // 峰值温升 ΔT = T_peak - T0 (℃)
    float   drop10;       // 峰值后 10s 降温幅度 (℃)
    float   drop30;       // 峰值后 30s 降温幅度 (℃)
    float   cool_ratio;   // 冷却衰减比 frac_10_30 = drop10 / drop30 (0~1)
    float   empty_dt;     // 可选空管基准衰减比 (0 表示使用默认免标定阈值)
    float   ratio_liquid; // 有液判定阈值 (cool_ratio > ratio_liquid)
    float   ratio_empty;  // 空管判定阈值 (cool_ratio < ratio_empty)
    bool    calibrated;   // 是否已有有效判定结果
    bool    calibrating;  // 该通道是否正在空管自动调参
    uint8_t state;        // 0=EMPTY, 1=LIQUID, 2=WAITING(首轮采集中), 3=FAULT
    uint8_t probe_type;   // 0=PROBE_STEEL(0.5mm不锈钢), 1=PROBE_CERAMIC(2mm陶瓷)
    bool    online;       // 18B20 是否在线
    bool    heating;      // 当前是否正在加热
};

void data_cache_update_sensor(int idx, const SensorDataCache& data);
const SensorDataCache& data_cache_get_sensor(int idx);
