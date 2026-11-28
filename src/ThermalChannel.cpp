#include "ThermalChannel.h"
#include "nvs_config.h"
#include <math.h>

ThermalChannel::ThermalChannel(int id, uint8_t dqPin, uint8_t heatPin)
    : _id(id),
      _dqPin(dqPin),
      _heatPin(heatPin),
      _oneWire(dqPin),
      _dallas(&_oneWire) {}

void ThermalChannel::begin() {
    pinMode(_heatPin, OUTPUT);
    digitalWrite(_heatPin, LOW);
    _heating = false;

    _dallas.begin();
    _dallas.setResolution(12);
    _dallas.setWaitForConversion(false);
    _online = (_dallas.getDeviceCount() > 0);
    if (_online) {
        _dallas.requestTemperatures();
    }
}

void ThermalChannel::setProbeType(ProbeType type) {
    _probeType = (type == PROBE_CERAMIC) ? PROBE_CERAMIC : PROBE_STEEL;
    if (_probeType == PROBE_CERAMIC) {
        _heatMs     = CERAMIC_HEAT_TIME_MS;
        _coolSkipMs = CERAMIC_COOL_SKIP_MS;
        _coolMidMs  = CERAMIC_COOL_MID_MS;
        _coolEndMs  = CERAMIC_COOL_END_MS;
    } else {
        _heatMs     = STEEL_HEAT_TIME_MS;
        _coolSkipMs = STEEL_COOL_SKIP_MS;
        _coolMidMs  = STEEL_COOL_MID_MS;
        _coolEndMs  = STEEL_COOL_END_MS;
    }
    _coolPhase = COOL_IDLE;
}

void ThermalChannel::heaterOn() {
    digitalWrite(_heatPin, HIGH);
    _heating = true;
}

void ThermalChannel::heaterOff() {
    digitalWrite(_heatPin, LOW);
    _heating = false;
}

void ThermalChannel::requestTemp() {
    _dallas.requestTemperatures();
}

float ThermalChannel::readTempC() {
    return _dallas.getTempCByIndex(0);
}

void ThermalChannel::onTempSample(float tempC, unsigned long now) {
    if (!isValidTemp(tempC)) {
        _tNow = NAN;
        if (_heating) heaterOff();
        _online    = false;
        _coolPhase = COOL_IDLE;
        if (_state != ThermalState::FAULT) {
            Serial.printf("[Thermal ERR] CH%d 18B20 读温异常 (%.2f), 禁止加热\n", _id + 1, tempC);
        }
        _transitionState(ThermalState::FAULT);
        return;
    }

    _tNow   = tempC;
    _online = true;

    if (_state == ThermalState::FAULT) {
        _transitionState(_hasRatio ? ((_coolRatio > _ratioLiquid) ? ThermalState::LIQUID : ThermalState::EMPTY)
                                   : ThermalState::UNCALIBRATED);
    }

    switch (_coolPhase) {
        case COOL_HEATING:
            if (isnan(_tPeak) || tempC >= _tPeak) {
                _tPeak  = tempC;
                _peakMs = now;
            }
            break;

        case COOL_FIND_PEAK:
            if (isnan(_tPeak) || tempC >= _tPeak) {
                _tPeak  = tempC;
                _peakMs = now;
            }
            if (now - _heatEndMs >= THERMAL_PEAK_WINDOW_MS) {
                if (!isnan(_t0) && !isnan(_tPeak)) {
                    _deltaT = _tPeak - _t0;
                }
                if (_coolSkipMs == 0) {
                    _tCoolStart = _tPeak;
                    _coolPhase  = COOL_WAIT_MID;
                } else {
                    _coolPhase  = COOL_WAIT_START;
                }
            }
            break;

        case COOL_WAIT_START:
            if (now - _peakMs >= _coolSkipMs) {
                _tCoolStart = tempC;
                _coolPhase  = COOL_WAIT_MID;
            }
            break;

        case COOL_WAIT_MID:
            if (now - _peakMs >= _coolMidMs) {
                _tCoolMid  = tempC;
                _coolPhase = COOL_WAIT_END;
            }
            break;

        case COOL_WAIT_END:
            if (now - _peakMs >= _coolEndMs) {
                _finalizeCooling(tempC);
            }
            break;

        case COOL_IDLE:
        default:
            break;
    }
}

bool ThermalChannel::startPulse(unsigned long now) {
    // 若上一轮因峰值略晚尚未满 _coolEndMs（但已过中点后至少 85% 窗口），在开启新脉冲前先收尾结算
    unsigned long minEarlyEnd = (_coolEndMs > 5000UL) ? (_coolEndMs - 5000UL) : _coolMidMs;
    if (_coolPhase == COOL_WAIT_END && isValidTemp(_tNow) && (now - _peakMs >= minEarlyEnd)) {
        _finalizeCooling(_tNow);
    }

    if (!isValidTemp(_tNow)) {
        heaterOff();
        _online    = false;
        _coolPhase = COOL_IDLE;
        _transitionState(ThermalState::FAULT);
        return false;
    }

    _t0        = _tNow;
    _tPeak     = _tNow;
    _peakMs    = now;
    _coolPhase = COOL_HEATING;
    heaterOn();
    return true;
}

void ThermalChannel::endHeating(unsigned long now) {
    heaterOff();
    if (!isValidTemp(_tNow)) {
        _coolPhase = COOL_IDLE;
        _transitionState(ThermalState::FAULT);
        return;
    }
    _t1        = _tNow;
    _heatEndMs = now;
    if (isnan(_tPeak) || _t1 >= _tPeak) {
        _tPeak  = _t1;
        _peakMs = now;
    }
    _deltaT    = _tPeak - _t0;
    _coolPhase = COOL_FIND_PEAK;
}

void ThermalChannel::_finalizeCooling(float tEnd) {
    _tCoolEnd  = tEnd;
    _coolPhase = COOL_IDLE;

    if (isnan(_tCoolStart) || isnan(_tCoolMid)) return;

    _drop10 = _tCoolStart - _tCoolMid;
    _drop30 = _tCoolStart - _tCoolEnd;

    if (_drop30 >= THERMAL_MIN_DROP_C) {
        float r = _drop10 / _drop30;
        if (r < 0.0f) r = 0.0f;
        if (r > 1.0f) r = 1.0f;
        _coolRatio = r;
        _hasRatio  = true;
        _evaluate();
    }
}

void ThermalChannel::startCalibration() {
    _calibrating = true;
    _calSum      = 0.0f;
    _calCount    = 0;
    Serial.printf("[CAL] CH%d (%s) 开始 %u 轮空管冷却衰减比自动调参...\n",
                  _id + 1, _probeType == PROBE_CERAMIC ? "陶瓷" : "不锈钢", THERMAL_CAL_SAMPLES);
}

void ThermalChannel::resetCalibration() {
    _calibrating = false;
    _emptyRef    = 0.0f;
    _calSum      = 0.0f;
    _calCount    = 0;
    _debounceCnt = 0;
    _ratioLiquid = (_probeType == PROBE_CERAMIC) ? CERAMIC_RATIO_LIQUID : STEEL_RATIO_LIQUID;
    _ratioEmpty  = (_probeType == PROBE_CERAMIC) ? CERAMIC_RATIO_EMPTY  : STEEL_RATIO_EMPTY;
    nvs_set_empty_delta_t(_id, 0.0f);
    nvs_set_channel_ratios(_id, _ratioLiquid, _ratioEmpty);
    Serial.printf("[CAL] CH%d 已恢复默认免标定阈值 (LIQUID>%.0f%% / EMPTY<%.0f%%)\n",
                  _id + 1, _ratioLiquid * 100.0f, _ratioEmpty * 100.0f);
}

void ThermalChannel::setEmptyDeltaT(float val) {
    _emptyRef = (val > 0.15f && val < 0.80f) ? val : 0.0f;
}

void ThermalChannel::setRatios(float ratioLiquid, float ratioEmpty) {
    if (ratioEmpty > 0.10f && ratioLiquid < 0.95f && ratioEmpty < ratioLiquid) {
        _ratioLiquid = ratioLiquid;
        _ratioEmpty  = ratioEmpty;
    }
}

int32_t ThermalChannel::getRawInt24() const {
    return (int32_t)lroundf(_coolRatio * 1000.0f);
}

int32_t ThermalChannel::getBaselineInt() const {
    return (int32_t)lroundf(_ratioEmpty * 1000.0f);
}

int32_t ThermalChannel::getThresholdInt() const {
    return (int32_t)lroundf(_ratioLiquid * 1000.0f);
}

bool ThermalChannel::isValidTemp(float t) {
    return (!isnan(t) && t > -50.0f && t < 120.0f && t != DEVICE_DISCONNECTED_C);
}

const char* ThermalChannel::stateToString(ThermalState st) {
    switch (st) {
        case ThermalState::EMPTY:        return "EMPTY";
        case ThermalState::LIQUID:       return "LIQUID";
        case ThermalState::UNCALIBRATED: return "WAIT";
        case ThermalState::FAULT:        return "FAULT";
        default:                         return "UNKNOWN";
    }
}

void ThermalChannel::_evaluate() {
    if (_calibrating) {
        if (_coolRatio > 0.15f && _coolRatio < 0.80f) {
            _calSum += _coolRatio;
            _calCount++;
            Serial.printf("[CAL] CH%d 空管样本 %u/%u: frac=%.1f%%\n",
                          _id + 1, _calCount, THERMAL_CAL_SAMPLES, _coolRatio * 100.0f);
        }
        if (_calCount >= THERMAL_CAL_SAMPLES) {
            _emptyRef    = _calSum / _calCount;
            _calibrating = false;
            _debounceCnt = 0;
            // 陶瓷管空管基准较高，偏移略收紧 (+5%/+10%)；不锈钢管 (+8%/+14%)
            float empOff = (_probeType == PROBE_CERAMIC) ? 0.05f : 0.08f;
            float liqOff = (_probeType == PROBE_CERAMIC) ? 0.10f : 0.14f;
            float newEmpty  = _emptyRef + empOff;
            float newLiquid = _emptyRef + liqOff;
            if (newEmpty  < 0.25f) newEmpty  = 0.25f;
            if (newLiquid > 0.90f) newLiquid = 0.90f;
            if (newEmpty >= newLiquid) newEmpty = newLiquid - 0.05f;
            _ratioEmpty  = newEmpty;
            _ratioLiquid = newLiquid;
            nvs_set_empty_delta_t(_id, _emptyRef);
            nvs_set_channel_ratios(_id, _ratioLiquid, _ratioEmpty);
            _transitionState(ThermalState::EMPTY);
            Serial.printf("[CAL] CH%d 空管调参完成 (ref=%.1f%% -> EMPTY<%.1f%% / LIQUID>%.1f%%)\n",
                          _id + 1, _emptyRef * 100.0f, _ratioEmpty * 100.0f, _ratioLiquid * 100.0f);
        }
        return;
    }

    ThermalState candidate = _state;
    if (_state == ThermalState::FAULT || _state == ThermalState::UNCALIBRATED) {
        candidate = (_coolRatio > _ratioLiquid) ? ThermalState::LIQUID : ThermalState::EMPTY;
    } else if (_coolRatio > _ratioLiquid) {
        candidate = ThermalState::LIQUID;
    } else if (_coolRatio < _ratioEmpty) {
        candidate = ThermalState::EMPTY;
    }

    if (candidate != _state) {
        if (candidate == _pendingState) {
            _debounceCnt++;
        } else {
            _pendingState = candidate;
            _debounceCnt  = 1;
        }
        if (_debounceCnt >= THERMAL_DEBOUNCE_COUNT) {
            _debounceCnt = 0;
            _transitionState(candidate);
        }
    } else {
        _debounceCnt = 0;
    }
}

void ThermalChannel::_transitionState(ThermalState next) {
    if (next == _state) return;
    _state = next;
    Serial.printf("[STATE] CH%d(%s) -> %s (frac=%.1f%%, dMid=%.2f/dEnd=%.2f, thr=[<%.0f%% / >%.0f%%])\n",
                  _id + 1, _probeType == PROBE_CERAMIC ? "C" : "S",
                  stateToString(_state),
                  _coolRatio * 100.0f, _drop10, _drop30,
                  _ratioEmpty * 100.0f, _ratioLiquid * 100.0f);
    if (_stateChangeCb) {
        _stateChangeCb(_id, _state);
    }
}
