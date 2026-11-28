#pragma once

#include <Arduino.h>
#include "config.h"

// ============================================================
//  3路脉冲热式液位传感器实时数据缓存结构 (Thermal Pulse v2)
// ============================================================
struct SensorDataCache {
    bool    detected;     // true = 有液 (ThermalState::LIQUID)
    float   t_now;        // 1Hz 实时温度 (℃)
    float   t0;           // 加热前基准温度 (℃)
    float   t1;           // 加热结束温度 (℃)
    float   t_peak;       // 断电后追踪到的峰值温度 (℃)
    float   delta_t;      // 峰值温升 ΔT = T_peak - T0 (℃)
    float   drop10;       // 前段降温幅度 (℃)
    float   drop30;       // 总段降温幅度 (℃)
    float   cool_ratio;   // 冷却衰减比 = drop_mid / drop_end (0~1)
    float   empty_dt;     // 可选空管基准衰减比 (0 表示使用默认免标定阈值)
    float   ratio_liquid; // 有液判定阈值 (cool_ratio > ratio_liquid)
    float   ratio_empty;  // 空管判定阈值 (cool_ratio < ratio_empty)
    bool    calibrated;   // 是否已有有效判定结果
    bool    calibrating;  // 该通道是否正在空管自动调参
    uint8_t state;        // 0=EMPTY, 1=LIQUID, 2=WAIT(首轮采集中), 3=FAULT
    uint8_t probe_type;   // 0=PROBE_STEEL(0.5mm不锈钢), 1=PROBE_CERAMIC(2mm陶瓷)
    bool    online;       // 18B20 是否在线
    bool    heating;      // 当前是否正在加热
};

void data_cache_update_sensor(int idx, const SensorDataCache& data);
const SensorDataCache& data_cache_get_sensor(int idx);
