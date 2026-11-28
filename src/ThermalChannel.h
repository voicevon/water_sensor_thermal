#pragma once

// ============================================================
//  ThermalChannel — 单通道脉冲热式液位传感器 OOP 类
//  核心算法：峰值追踪 + 冷却衰减比 frac_10_30 = (Tpk - T10) / (Tpk - T30)
//    - 免标定开箱即用：空气 ~36~42%，水中 ~58~63%（流动水更高）
//    - 免疫水温高低、5V 电源压降与连续轮巡热累积漂移
//  调度：
//    - main.cpp 统一 1Hz 非阻塞采集全部 3 路 18B20 并驱动分时加热脉冲
// ============================================================

#include <Arduino.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include "config.h"

enum class ThermalState : uint8_t {
    EMPTY        = 0, // 空管
    LIQUID       = 1, // 有液
    UNCALIBRATED = 2, // 首轮采集中（等待首个 30s 冷却周期完成）
    FAULT        = 3  // 18B20 断线/短路/CRC故障
};

class ThermalChannel {
public:
    typedef void (*StateChangeCallback)(int channelId, ThermalState newState);

    ThermalChannel(int id, uint8_t dqPin, uint8_t heatPin);

    // ---- 硬件初始化与 Fail-Safe 控制 ----
    void  begin();
    void  heaterOn();
    void  heaterOff();
    void  requestTemp();
    float readTempC();

    // ---- 1Hz 采样驱动与加热脉冲边界回调（由 main.cpp 调用） ----
    // 每秒喂入一次最新的 18B20 温度读数，自动驱动峰值追踪与 10s/30s 冷却采样
    void onTempSample(float tempC, unsigned long now);
    // 启动 10s 加热脉冲：有效返回 true（已开启加热），无效标 FAULT 返回 false
    bool startPulse(unsigned long now);
    // 结束 10s 加热脉冲：立即关断加热并进入断电后峰值追踪窗口
    void endHeating(unsigned long now);

    // ---- 封装类型、标定（可选空管自动微调阈值）与配置接口 ----
    void setProbeType(ProbeType type);
    void startCalibration();
    void resetCalibration();
    void setEmptyDeltaT(float val); // 存储可选空管参考衰减比 (0=默认免标定)
    void setRatios(float ratioLiquid, float ratioEmpty);
    void onStateChange(StateChangeCallback cb) { _stateChangeCb = cb; }

    // ---- 状态查询 Getter ----
    int          getId()           const { return _id; }
    ProbeType    getProbeType()    const { return _probeType; }
    unsigned long getHeatMs()      const { return _heatMs; }
    unsigned long getCoolEndMs()   const { return _coolEndMs; }
    float        getTempNow()      const { return _tNow; }
    float        getT0()           const { return _t0; }
    float        getT1()           const { return _t1; }
    float        getTPeak()        const { return _tPeak; }
    float        getDeltaT()       const { return _deltaT; }
    float        getDrop10()       const { return _drop10; }
    float        getDrop30()       const { return _drop30; }
    float        getCoolRatio()    const { return _coolRatio; }
    float        getEmptyDeltaT()  const { return _emptyRef; }
    float        getRatioLiquid()  const { return _ratioLiquid; }
    float        getRatioEmpty()   const { return _ratioEmpty; }
    bool         isCalibrated()    const { return _hasRatio; }
    bool         isCalibrating()   const { return _calibrating; }
    uint8_t      getCalCount()     const { return _calCount; }
    bool         isOnline()        const { return _online; }
    bool         isHeating()       const { return _heating; }
    ThermalState getState()        const { return _state; }
    bool         isDetected()      const { return _state == ThermalState::LIQUID; }

    // 兼容 HX711 BLE/MQTT 协议的 24 位有符号整型（冷却衰减比 × 1000，单位 0.1%）
    int32_t      getRawInt24()     const;
    int32_t      getBaselineInt()  const;
    int32_t      getThresholdInt() const;

    static bool        isValidTemp(float t);
    static const char* stateToString(ThermalState st);

private:
    enum CoolPhase : uint8_t {
        COOL_IDLE = 0,
        COOL_HEATING,
        COOL_FIND_PEAK,
        COOL_WAIT_START,
        COOL_WAIT_MID,
        COOL_WAIT_END
    };

    int               _id;
    uint8_t           _dqPin;
    uint8_t           _heatPin;
    OneWire           _oneWire;
    DallasTemperature _dallas;

    // 探头封装类型与对应时序参数
    ProbeType     _probeType   = PROBE_STEEL;
    unsigned long _heatMs      = STEEL_HEAT_TIME_MS;
    unsigned long _coolSkipMs  = STEEL_COOL_SKIP_MS;
    unsigned long _coolMidMs   = STEEL_COOL_MID_MS;
    unsigned long _coolEndMs   = STEEL_COOL_END_MS;

    // 1Hz 实时温度与脉冲/冷却曲线采样
    float         _tNow        = NAN;
    float         _t0          = NAN;
    float         _t1          = NAN;
    float         _tPeak       = NAN;
    float         _tCoolStart  = NAN;  // 峰值后 _coolSkipMs (不锈钢=Tpk, 陶瓷=T10)
    float         _tCoolMid    = NAN;  // 峰值后 _coolMidMs  (不锈钢=T10, 陶瓷=T30)
    float         _tCoolEnd    = NAN;  // 峰值后 _coolEndMs  (不锈钢=T30, 陶瓷=T60)
    float         _deltaT      = 0.0f; // T_peak - T0
    float         _drop10      = 0.0f; // 前段降温 (T_start - T_mid)
    float         _drop30      = 0.0f; // 总段降温 (T_start - T_end)
    float         _coolRatio   = 0.0f; // 冷却衰减比 = drop_mid / drop_end
    bool          _hasRatio    = false;

    CoolPhase     _coolPhase   = COOL_IDLE;
    unsigned long _heatEndMs   = 0;
    unsigned long _peakMs      = 0;

    // 迟滞阈值与可选空管微调
    float         _emptyRef    = 0.0f;
    float         _ratioLiquid = DEFAULT_RATIO_LIQUID;
    float         _ratioEmpty  = DEFAULT_RATIO_EMPTY;
    bool          _calibrating = false;
    float         _calSum      = 0.0f;
    uint8_t       _calCount    = 0;

    // 硬件与判定状态
    bool          _online       = false;
    bool          _heating      = false;
    ThermalState  _state        = ThermalState::UNCALIBRATED;
    ThermalState  _pendingState = ThermalState::UNCALIBRATED;
    uint8_t       _debounceCnt  = 0;

    StateChangeCallback _stateChangeCb = nullptr;

    void _finalizeCooling(float t30);
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
void    app_sync_channel_probe_type(int ch);
bool    app_is_channel_online(int ch);
bool    app_is_any_calibrating();
uint8_t app_get_active_channel();
float   app_get_bus_voltage_5v();
