#include "ble_adv.h"
#include "config.h"

#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>

static BLEAdvertising *s_pAdvertising = nullptr;
static uint8_t         s_seq_num      = 0;

void ble_init() {
    BLEDevice::init(BLE_DEVICE_NAME);
    s_pAdvertising = BLEDevice::getAdvertising();
    s_pAdvertising->start();
    Serial.printf("[BLE] Advertising started (Name: %s)\n", BLE_DEVICE_NAME);
}

void ble_update(const int32_t *sensors, const bool *states) {
    if (!s_pAdvertising) return;

    BLEAdvertisementData advData;
    advData.setFlags(0x06);
    advData.setName(BLE_DEVICE_NAME);

    // Company ID(2B) + Sensor1~3(各3B大端) + StateByte(1B) + SeqNum(1B) = 13B
    uint8_t payload[2 + SENSOR_COUNT * 3 + 1 + 1];
    size_t idx = 0;
    payload[idx++] = BLE_COMPANY_ID_LSB;
    payload[idx++] = BLE_COMPANY_ID_MSB;

    for (int i = 0; i < SENSOR_COUNT; i++) {
        payload[idx++] = (uint8_t)((sensors[i] >> 16) & 0xFF);
        payload[idx++] = (uint8_t)((sensors[i] >> 8)  & 0xFF);
        payload[idx++] = (uint8_t)(sensors[i]         & 0xFF);
    }

    uint8_t state_byte = 0;
    for (int i = 0; i < SENSOR_COUNT; i++) {
        if (states[i]) state_byte |= (1 << i);
    }
    payload[idx++] = state_byte;
    payload[idx++] = s_seq_num++;

#if defined(ESP_ARDUINO_VERSION) && (ESP_ARDUINO_VERSION >= ESP_ARDUINO_VERSION_VAL(3, 0, 0))
    advData.setManufacturerData(String((const char*)payload, idx));
#else
    advData.setManufacturerData(std::string((const char*)payload, idx));
#endif

    s_pAdvertising->setAdvertisementData(advData);
}
