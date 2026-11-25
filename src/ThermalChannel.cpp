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

bool ThermalChannel::onT0Measured(float t0) {
    _t0 = t0;
    if (!isValidTemp(t0)) {
        heaterOff();
        _online = false;
        _deltaT = 0.0f;
        _transitionState(ThermalState::FAULT);
        Serial.printf("[Thermal ERR] CH%d 18B20 读温异常 (%.2f), 禁止加热\n", _id + 1, t0);
        return false;
    }
    _online = true;
    return true;
}

void ThermalChannel::onT1Measured(float t1) {
    // 读完立即关断加热（Fail-Safe）
    heaterOff();
    _t1 = t1;
    if (!isValidTemp(t1)) {
        _online = false;
        _deltaT = 0.0f;
        _transitionState(ThermalState::FAULT);
        return;
    }
    _online = true;
    _deltaT = _t1 - _t0;
    _evaluate();
}

void ThermalChannel::startCalibration() {
    _calibrating = true;
    _calSum      = 0.0f;
    _calCount    = 0;
    Serial.printf("[CAL] CH%d 开始 %u 轮空管标定...\n", _id + 1, THERMAL_CAL_SAMPLES);
}

void ThermalChannel::resetCalibration() {
    _calibrating = false;
    _calibrated  = false;
    _emptyDeltaT = 0.0f;
    _calSum      = 0.0f;
    _calCount    = 0;
    _debounceCnt = 0;
    nvs_set_empty_delta_t(_id, 0.0f);
    _transitionState(ThermalState::UNCALIBRATED);
    Serial.printf("[CAL] CH%d 已清除标定数据\n", _id + 1);
}

void ThermalChannel::setEmptyDeltaT(float val) {
    if (val > 2.0f && val < 40.0f) {
        _emptyDeltaT = val;
        _calibrated  = true;
        if (_state == ThermalState::UNCALIBRATED) {
            _state = ThermalState::EMPTY;
        }
    } else {
        _emptyDeltaT = 0.0f;
        _calibrated  = false;
        _state       = ThermalState::UNCALIBRATED;
    }
}

void ThermalChannel::setRatios(float ratioLiquid, float ratioEmpty) {
    if (ratioLiquid > 0.1f && ratioEmpty < 0.95f && ratioLiquid < ratioEmpty) {
        _ratioLiquid = ratioLiquid;
        _ratioEmpty  = ratioEmpty;
    }
}

int32_t ThermalChannel::getRawInt24() const {
    return (int32_t)lroundf(_deltaT * 100.0f);
}

int32_t ThermalChannel::getBaselineInt() const {
    return (int32_t)lroundf(_emptyDeltaT * 100.0f);
}

int32_t ThermalChannel::getThresholdInt() const {
    return (int32_t)lroundf(_emptyDeltaT * _ratioLiquid * 100.0f);
}

bool ThermalChannel::isValidTemp(float t) {
    return (!isnan(t) && t > -50.0f && t < 120.0f && t != DEVICE_DISCONNECTED_C);
}

const char* ThermalChannel::stateToString(ThermalState st) {
    switch (st) {
        case ThermalState::EMPTY:        return "EMPTY";
        case ThermalState::LIQUID:       return "LIQUID";
        case ThermalState::UNCALIBRATED: return "UNCALIBRATED";
        case ThermalState::FAULT:        return "FAULT";
        default:                         return "UNKNOWN";
    }
}

void ThermalChannel::_evaluate() {
    if (_calibrating) {
        if (_deltaT > 2.0f && _deltaT < 40.0f) {
            _calSum += _deltaT;
            _calCount++;
            Serial.printf("[CAL] CH%d 样本 %u/%u: dT=%.2f C\n",
                          _id + 1, _calCount, THERMAL_CAL_SAMPLES, _deltaT);
        }
        if (_calCount >= THERMAL_CAL_SAMPLES) {
            _emptyDeltaT = _calSum / _calCount;
            _calibrated  = true;
            _calibrating = false;
            _debounceCnt = 0;
            nvs_set_empty_delta_t(_id, _emptyDeltaT);
            _transitionState(ThermalState::EMPTY);
            Serial.printf("[CAL] CH%d 空管标定完成 (emp=%.2f C) 并已写入 NVS!\n",
                          _id + 1, _emptyDeltaT);
        }
        return;
    }

    if (!_calibrated || _emptyDeltaT <= 2.0f) {
        _transitionState(ThermalState::UNCALIBRATED);
        return;
    }

    float ratio = _deltaT / _emptyDeltaT;
    ThermalState candidate = _state;
    if (_state == ThermalState::FAULT || _state == ThermalState::UNCALIBRATED) {
        candidate = (ratio < _ratioLiquid) ? ThermalState::LIQUID : ThermalState::EMPTY;
    } else if (ratio < _ratioLiquid) {
        candidate = ThermalState::LIQUID;
    } else if (ratio > _ratioEmpty) {
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
    Serial.printf("[STATE] CH%d -> %s (dT=%.2f / emp=%.2f, rL=%.2f, rE=%.2f)\n",
                  _id + 1, stateToString(_state),
                  _deltaT, _emptyDeltaT, _ratioLiquid, _ratioEmpty);
    if (_stateChangeCb) {
        _stateChangeCb(_id, _state);
    }
}
