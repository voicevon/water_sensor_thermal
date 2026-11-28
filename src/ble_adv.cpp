#include "ble_adv.h"
#include "config.h"

#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <math.h>

static BLEAdvertising *s_pAdvertising = nullptr;
static uint8_t         s_seq_num      = 0;

static inline int16_t encode_temp_x100(float tempC) {
    if (isnan(tempC) || isinf(tempC) || tempC < -300.0f || tempC > 300.0f) {
        return (int16_t)0x8000; // -32768 = NaN / offline
    }
    long v = lroundf(tempC * 100.0f);
    if (v <= -32767L) v = -32767L;
    if (v >   32767L) v =  32767L;
    return (int16_t)v;
}

static inline uint8_t encode_ratio_half_pct(float ratio, bool hasRatio) {
    if (!hasRatio || isnan(ratio) || isinf(ratio)) {
        return 0xFF;
    }
    long v = lroundf(ratio * 200.0f); // 0.5% steps: 0..200
    if (v < 0L)   v = 0L;
    if (v > 200L) v = 200L;
    return (uint8_t)v;
}

void ble_init() {
    BLEDevice::init(BLE_DEVICE_NAME);
    s_pAdvertising = BLEDevice::getAdvertising();
    s_pAdvertising->start();
    Serial.printf("[BLE] Thermal v2 Advertising started (Name: %s, 16B payload)\n", BLE_DEVICE_NAME);
}

void ble_update(const ThermalBleFrame& frame) {
    if (!s_pAdvertising) return;

    BLEAdvertisementData advData;
    advData.setFlags(0x06);
    advData.setName(BLE_DEVICE_NAME);

    // Company ID(2B) + Ver(1B) + Temp(6B) + Ratio(3B) + States(1B) + Flags(1B) + BusV5(1B) + Seq(1B) = 16B
    uint8_t payload[16];
    size_t idx = 0;

    payload[idx++] = BLE_COMPANY_ID_LSB;
    payload[idx++] = BLE_COMPANY_ID_MSB;
    payload[idx++] = BLE_PROTOCOL_VERSION;

    for (int i = 0; i < SENSOR_COUNT; i++) {
        int16_t t = encode_temp_x100(frame.temp_c[i]);
        payload[idx++] = (uint8_t)((t >> 8) & 0xFF);
        payload[idx++] = (uint8_t)(t & 0xFF);
    }

    for (int i = 0; i < SENSOR_COUNT; i++) {
        payload[idx++] = encode_ratio_half_pct(frame.cool_ratio[i], frame.has_ratio[i]);
    }

    uint8_t states_byte = (uint8_t)((frame.active_ch & 0x03) << 6);
    for (int i = 0; i < SENSOR_COUNT; i++) {
        states_byte |= (uint8_t)((frame.state[i] & 0x03) << (i * 2));
    }
    payload[idx++] = states_byte;

    uint8_t flags_byte = 0;
    for (int i = 0; i < SENSOR_COUNT; i++) {
        if (frame.heating[i])    flags_byte |= (uint8_t)(1U << i);
        if (frame.probe_type[i]) flags_byte |= (uint8_t)(1U << (i + 3));
    }
    if (frame.bus_v5_ok)   flags_byte |= (1U << 6);
    if (frame.calibrating) flags_byte |= (1U << 7);
    payload[idx++] = flags_byte;

    long v5_x20 = lroundf(frame.bus_v5 * 20.0f);
    if (v5_x20 < 0L)   v5_x20 = 0L;
    if (v5_x20 > 255L) v5_x20 = 255L;
    payload[idx++] = (uint8_t)v5_x20;

    payload[idx++] = s_seq_num++;

#if defined(ESP_ARDUINO_VERSION) && (ESP_ARDUINO_VERSION >= ESP_ARDUINO_VERSION_VAL(3, 0, 0))
    advData.setManufacturerData(String((const char*)payload, idx));
#else
    advData.setManufacturerData(std::string((const char*)payload, idx));
#endif

    s_pAdvertising->setAdvertisementData(advData);
}
