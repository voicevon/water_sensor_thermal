#include <Arduino.h>
#include <esp_log.h>
#include <esp_task_wdt.h>
#include <math.h>

#include "config.h"
#include "ThermalChannel.h"
#include "ble_adv.h"
#include "wifi_mqtt.h"
#include "web_config.h"
#include "nvs_config.h"

// ============================================================
//  实例化 3 路 ThermalChannel 对象（对齐 water_sensor_hx711 的 s_sensors[3]）
// ============================================================
static ThermalChannel s_channels[SENSOR_COUNT] = {
    ThermalChannel(0, THERMAL_DQ_1_PIN, THERMAL_HEAT_1_PIN),
    ThermalChannel(1, THERMAL_DQ_2_PIN, THERMAL_HEAT_2_PIN),
    ThermalChannel(2, THERMAL_DQ_3_PIN, THERMAL_HEAT_3_PIN)
};

// ============================================================
//  分时轮巡调度状态机（由 main.cpp 统一控制 activeCh 切换与 5V 监测）
// ============================================================
enum MeasurePhase : uint8_t {
    PH_START_T0 = 0,
    PH_WAIT_T0,
    PH_HEATING,
    PH_WAIT_T1,
    PH_COOLDOWN
};

static uint8_t       s_active_ch    = 0;
static MeasurePhase  s_phase        = PH_START_T0;
static unsigned long s_slot_start   = 0;
static unsigned long s_phase_start  = 0;
static float         s_bus_v5       = 5.0f;

// MQTT 定时发布全局变量（与 wifi_mqtt.cpp 共享）
uint32_t      g_mqtt_publish_interval_ms = 1000UL;
unsigned long s_last_mqtt_publish_time   = 0;

static unsigned long s_last_send_time = 0;
static bool          s_last_led_on    = false;

// ============================================================
//  板级辅助与协作钩子实现（声明于 ThermalChannel.h）
// ============================================================
static float read_5v_bus() {
    uint32_t mvSum = 0;
    for (int i = 0; i < 16; i++) {
        mvSum += analogReadMilliVolts(PIN_5V_MON);
    }
    return (mvSum / 16.0f) / 1000.0f * 3.0f; // 20k/10k 分压比 = 3.0
}

void app_all_heaters_off() {
    for (int i = 0; i < SENSOR_COUNT; i++) {
        s_channels[i].heaterOff();
    }
}

void app_start_calibration(int ch) {
    if (ch < 0) {
        for (int i = 0; i < SENSOR_COUNT; i++) s_channels[i].startCalibration();
    } else if (ch < SENSOR_COUNT) {
        s_channels[ch].startCalibration();
    }
}

void app_reset_calibration(int ch) {
    if (ch < 0) {
        for (int i = 0; i < SENSOR_COUNT; i++) s_channels[i].resetCalibration();
    } else if (ch < SENSOR_COUNT) {
        s_channels[ch].resetCalibration();
    }
}

void app_sync_channel_ratios(int ch) {
    if (ch < 0 || ch >= SENSOR_COUNT) return;
    s_channels[ch].setRatios(get_channel_ratio_liquid(ch), get_channel_ratio_empty(ch));
}

bool app_is_channel_online(int ch) {
    return (ch >= 0 && ch < SENSOR_COUNT) ? s_channels[ch].isOnline() : false;
}

bool app_is_any_calibrating() {
    for (int i = 0; i < SENSOR_COUNT; i++) {
        if (s_channels[i].isCalibrating()) return true;
    }
    return false;
}

uint8_t app_get_active_channel() { return s_active_ch; }
float   app_get_bus_voltage_5v() { return s_bus_v5; }

static bool is_any_heating() {
    for (int i = 0; i < SENSOR_COUNT; i++) {
        if (s_channels[i].isHeating()) return true;
    }
    return false;
}

static void init_hardware_wdt() {
#if defined(ESP_ARDUINO_VERSION) && (ESP_ARDUINO_VERSION >= ESP_ARDUINO_VERSION_VAL(3, 0, 0))
    esp_task_wdt_config_t wdtCfg = {
        .timeout_ms     = THERMAL_WDT_TIMEOUT_S * 1000,
        .idle_core_mask = 0,
        .trigger_panic  = true
    };
    esp_task_wdt_reconfigure(&wdtCfg);
#else
    esp_task_wdt_init(THERMAL_WDT_TIMEOUT_S, true);
#endif
    esp_task_wdt_add(NULL);
}

static void run_thermal_scheduler(unsigned long now) {
    ThermalChannel& ch = s_channels[s_active_ch];

    switch (s_phase) {
        case PH_START_T0: {
            app_all_heaters_off();
            s_slot_start  = now;
            s_phase_start = now;
            s_bus_v5      = read_5v_bus();
            ch.requestTemp();
            s_phase = PH_WAIT_T0;
            break;
        }
        case PH_WAIT_T0: {
            if (now - s_phase_start >= THERMAL_CONV_TIME_MS) {
                float t0 = ch.readTempC();
                if (ch.onT0Measured(t0)) {
                    ch.heaterOn();
                    s_phase_start = now;
                    s_phase = PH_HEATING;
                } else {
                    s_phase = PH_COOLDOWN;
                }
            }
            break;
        }
        case PH_HEATING: {
            if (now - s_phase_start >= THERMAL_HEAT_PRE_MS) {
                ch.requestTemp();
                s_phase_start = now;
                s_phase = PH_WAIT_T1;
            }
            break;
        }
        case PH_WAIT_T1: {
            if (now - s_phase_start >= THERMAL_CONV_TIME_MS) {
                float t1 = ch.readTempC();
                ch.onT1Measured(t1);
                app_all_heaters_off();
                s_phase = PH_COOLDOWN;
            }
            break;
        }
        case PH_COOLDOWN: {
            if (now - s_slot_start >= THERMAL_SLOT_TOTAL_MS) {
                s_active_ch = (s_active_ch + 1) % SENSOR_COUNT;
                s_phase     = PH_START_T0;
            }
            break;
        }
    }
}

// ============================================================
//  Setup & Loop
// ============================================================
void setup() {
    // 0. 第一时间强制关闭所有加热器（Fail-Safe）
    pinMode(THERMAL_HEAT_1_PIN, OUTPUT); digitalWrite(THERMAL_HEAT_1_PIN, LOW);
    pinMode(THERMAL_HEAT_2_PIN, OUTPUT); digitalWrite(THERMAL_HEAT_2_PIN, LOW);
    pinMode(THERMAL_HEAT_3_PIN, OUTPUT); digitalWrite(THERMAL_HEAT_3_PIN, LOW);

    pinMode(PIN_LED_STAT, OUTPUT);
    digitalWrite(PIN_LED_STAT, HIGH); // 低电平点亮，初始灭

    esp_log_level_set("*", ESP_LOG_NONE);

    Serial.begin(115200);
    delay(500);
    Serial.println("\n====================================");
    Serial.println("ESP32 3-CH Thermal Liquid Sensor (OOP)");
    Serial.println("Protocol Compatible: water_sensor_hx711");
    Serial.printf("DQ Pins:   CH1=%d, CH2=%d, CH3=%d\n",
                  THERMAL_DQ_1_PIN, THERMAL_DQ_2_PIN, THERMAL_DQ_3_PIN);
    Serial.printf("HEAT Pins: CH1=%d, CH2=%d, CH3=%d\n",
                  THERMAL_HEAT_1_PIN, THERMAL_HEAT_2_PIN, THERMAL_HEAT_3_PIN);
    Serial.println("====================================");

    // 1. 启动 Web 配置服务器（内部会先调 nvs_config_init 加载 NVS）
    web_config_init();

    // 2. 配置 5V 母线 ADC 并初始化 3 路 ThermalChannel 对象
    analogReadResolution(12);
    analogSetPinAttenuation(PIN_5V_MON, ADC_11db);
    s_bus_v5 = read_5v_bus();

    for (int i = 0; i < SENSOR_COUNT; i++) {
        s_channels[i].begin();
        s_channels[i].setEmptyDeltaT(get_empty_delta_t(i));
        s_channels[i].setRatios(get_channel_ratio_liquid(i), get_channel_ratio_empty(i));
    }

    s_active_ch   = 0;
    s_phase       = PH_START_T0;
    s_slot_start  = millis();
    s_phase_start = s_slot_start;

    // 3. 启动 WiFi / MQTT 与 BLE 广播
    wifi_mqtt_init();
    ble_init();

    // 4. 启动 20s 硬件看门狗防干烧
    init_hardware_wdt();
}

void loop() {
    esp_task_wdt_reset();
    unsigned long now = millis();

    // 1. 驱动 3 路分时轮巡测量状态机
    run_thermal_scheduler(now);

    // 2. 驱动 Web Server 与 WiFi/MQTT 心跳
    web_config_loop();
    wifi_mqtt_loop(now);

    // 3. 板内状态 LED (GPIO19, 低电平点亮)
    bool led_on = false;
    if (is_any_heating()) {
        led_on = true;
    } else if (wifi_is_connected()) {
        led_on = mqtt_is_connected() ? ((now % 1000) < 500) : ((now % 2000) < 1000);
    }
    if (led_on != s_last_led_on) {
        s_last_led_on = led_on;
        digitalWrite(PIN_LED_STAT, led_on ? LOW : HIGH);
    }

    // 4. 1Hz 非阻塞定时同步：Web 缓存 + BLE 广播 + MQTT 上报
    if (now - s_last_send_time >= SEND_INTERVAL_MS) {
        s_last_send_time = now;

        int32_t out_sensors[SENSOR_COUNT] = { 0, 0, 0 };
        bool    out_states[SENSOR_COUNT]  = { false, false, false };
        uint8_t state_byte = 0;

        for (int i = 0; i < SENSOR_COUNT; i++) {
            const ThermalChannel& ch = s_channels[i];
            out_sensors[i] = ch.getRawInt24();
            out_states[i]  = ch.isDetected();
            if (out_states[i]) state_byte |= (1 << i);

            SensorDataCache cache;
            cache.raw_val      = out_sensors[i];
            cache.filtered     = out_sensors[i];
            cache.baseline     = ch.getBaselineInt();
            cache.threshold    = ch.getThresholdInt();
            cache.detected     = out_states[i];
            cache.t0           = ch.getT0();
            cache.t1           = ch.getT1();
            cache.delta_t      = ch.getDeltaT();
            cache.empty_dt     = ch.getEmptyDeltaT();
            cache.ratio_liquid = ch.getRatioLiquid();
            cache.ratio_empty  = ch.getRatioEmpty();
            cache.calibrated   = ch.isCalibrated();
            cache.calibrating  = ch.isCalibrating();
            cache.state        = (uint8_t)ch.getState();
            cache.online       = ch.isOnline();
            cache.heating      = ch.isHeating();
            web_config_update_sensor(i, cache);
        }

        ble_update(out_sensors, out_states);

        if (now - s_last_mqtt_publish_time >= g_mqtt_publish_interval_ms) {
            s_last_mqtt_publish_time = now;
            mqtt_publish(out_sensors, state_byte);
        }

        Serial.printf("[Thermal] 5V=%.2fV Active=CH%u Cal=%u WiFi=%u MQTT=%u StateByte=0x%02X\n",
                      s_bus_v5, s_active_ch + 1,
                      app_is_any_calibrating() ? 1 : 0,
                      wifi_is_connected() ? 1 : 0,
                      mqtt_is_connected() ? 1 : 0,
                      state_byte);
        for (int i = 0; i < SENSOR_COUNT; i++) {
            const ThermalChannel& ch = s_channels[i];
            Serial.printf("  CH%d: T0=%.2f T1=%.2f dT=%.2f (raw=%ld) emp=%.2f [%s]%s\n",
                          i + 1,
                          isnan(ch.getT0()) ? -99.0f : ch.getT0(),
                          isnan(ch.getT1()) ? -99.0f : ch.getT1(),
                          ch.getDeltaT(),
                          (long)out_sensors[i],
                          ch.getEmptyDeltaT(),
                          ThermalChannel::stateToString(ch.getState()),
                          ch.isHeating() ? " *HEAT*" : "");
        }
    }
}
