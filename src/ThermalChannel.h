#pragma once

// ============================================================
//  ThermalChannel — 单通道脉冲热式液位传感器 OOP 类
//  职责：
//    - 封装单路 1-Wire 18B20 测温硬件 + 高边 P-MOS 加热控制
//    - 独立维护该通道的空管标定、迟滞比例阈值、防抖与状态变化回调
//  调度：
//    - main.cpp 实例化 ThermalChannel s_channels[3] 并统一管理分时轮巡
// ============================================================

#include <Arduino.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include "config.h"

enum class ThermalState : uint8_t {
    EMPTY        = 0, // 空管
    LIQUID       = 1, // 有液流动
    UNCALIBRATED = 2, // 尚未进行空管标定
    FAULT        = 3  // 18B20 断线/短路/CRC故障
};

class ThermalChannel {
public:
    typedef void (*StateChangeCallback)(int channelId, ThermalState newState);

    ThermalChannel(int id, uint8_t dqPin, uint8_t heatPin);

    // ---- 硬件初始化与 Fail-Safe 控制 ----
    void begin();
    void heaterOn();
    void heaterOff();
    void requestTemp();
    float readTempC();

    // ---- 测量阶段回调（由 main.cpp 分时轮巡调用） ----
    // 记录 T0：有效返回 true（允许加热），无效标记 FAULT 并返回 false（禁止加热）
    bool onT0Measured(float t0);
    // 记录 T1 并执行标定/迟滞状态判定（内部会先关断加热）
    void onT1Measured(float t1);

    // ---- 标定与配置接口 ----
    void startCalibration();
    void resetCalibration();
    void setEmptyDeltaT(float val);
    void setRatios(float ratioLiquid, float ratioEmpty);
    void onStateChange(StateChangeCallback cb) { _stateChangeCb = cb; }

    // ---- 状态查询 Getter ----
    int          getId()           const { return _id; }
    float        getT0()           const { return _t0; }
    float        getT1()           const { return _t1; }
    float        getDeltaT()       const { return _deltaT; }
    float        getEmptyDeltaT()  const { return _emptyDeltaT; }
    float        getRatioLiquid()  const { return _ratioLiquid; }
    float        getRatioEmpty()   const { return _ratioEmpty; }
    bool         isCalibrated()    const { return _calibrated; }
    bool         isCalibrating()   const { return _calibrating; }
    uint8_t      getCalCount()     const { return _calCount; }
    bool         isOnline()        const { return _online; }
    bool         isHeating()       const { return _heating; }
    ThermalState getState()        const { return _state; }
    bool         isDetected()      const { return _state == ThermalState::LIQUID; }

    // 兼容 HX711 BLE/MQTT 协议的 24 位有符号整型（ΔT × 100，单位 0.01 ℃）
    int32_t      getRawInt24()     const;
    int32_t      getBaselineInt()  const;
    int32_t      getThresholdInt() const;

    static bool        isValidTemp(float t);
    static const char* stateToString(ThermalState st);

private:
    int               _id;
    uint8_t           _dqPin;
    uint8_t           _heatPin;
    OneWire           _oneWire;
    DallasTemperature _dallas;

    // 测量与标定状态
    float        _t0           = NAN;
    float        _t1           = NAN;
    float        _deltaT       = 0.0f;
    float        _emptyDeltaT  = 0.0f;
    float        _ratioLiquid  = DEFAULT_RATIO_LIQUID;
    float        _ratioEmpty   = DEFAULT_RATIO_EMPTY;
    bool         _calibrated   = false;
    bool         _calibrating  = false;
    float        _calSum       = 0.0f;
    uint8_t      _calCount     = 0;

    // 硬件与判定状态
    bool         _online       = false;
    bool         _heating      = false;
    ThermalState _state        = ThermalState::UNCALIBRATED;
    ThermalState _pendingState = ThermalState::UNCALIBRATED;
    uint8_t      _debounceCnt  = 0;

    StateChangeCallback _stateChangeCb = nullptr;

    void _evaluate();
    void _transitionState(ThermalState next);
};

// ============================================================
//  板级协作钩子（由 main.cpp 实现，供 web_config / wifi_mqtt 调用）
// ============================================================
void    app_all_heaters_off();
void    app_start_calibration(int ch); // ch < 0 表示全部通道，0~2 表示单通道
void    app_reset_calibration(int ch); // ch < 0 表示全部通道，0~2 表示单通道
void    app_sync_channel_ratios(int ch);
bool    app_is_channel_online(int ch);
bool    app_is_any_calibrating();
uint8_t app_get_active_channel();
float   app_get_bus_voltage_5v();
