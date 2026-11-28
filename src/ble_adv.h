#pragma once

// ============================================================
//  BLE 广播管理层 — Thermal Pulse v2 紧凑 16B 主广播单包协议
//  Layout (Manufacturer Data, 16 bytes):
//    [0..1]  Company ID 0xFFFF (LE: 0xFF, 0xFF)
//    [2]     Version = 0x02
//    [3..8]  temp_ch0..2: int16_t BE in 0.01°C (0x8000 = NaN/offline)
//    [9..11] ratio_ch0..2: uint8_t in 0.5% (0..200, 0xFF = unmeasured)
//    [12]    states_byte: bit[1:0]=CH0, bit[3:2]=CH1, bit[5:4]=CH2 (0..3), bit[7:6]=active_ch
//    [13]    flags_byte:  bit[2:0]=heating, bit[5:3]=probe_type, bit[6]=bus_v5_ok, bit[7]=calibrating
//    [14]    bus_v5_x20:  uint8_t (round(v5 * 20), 0.05V precision)
//    [15]    seq_num:     uint8_t
// ============================================================

#include "config.h"

struct ThermalBleFrame {
    float   temp_c[SENSOR_COUNT];
    float   cool_ratio[SENSOR_COUNT];
    bool    has_ratio[SENSOR_COUNT];
    uint8_t state[SENSOR_COUNT];       // 0=EMPTY, 1=LIQUID, 2=WAIT, 3=FAULT
    bool    heating[SENSOR_COUNT];
    uint8_t probe_type[SENSOR_COUNT];  // 0=STEEL, 1=CERAMIC
    uint8_t active_ch;                 // 0..2
    float   bus_v5;
    bool    bus_v5_ok;
    bool    calibrating;
};

void ble_init();
void ble_update(const ThermalBleFrame& frame);
