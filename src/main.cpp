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
//  分时轮巡加热调度状态机（1Hz 实时采样由 run_1hz_sampler 统一驱动）
// ============================================================
enum MeasurePhase : uint8_t {
    PH_START_PULSE = 0,
    PH_HEATING,
    PH_COOLDOWN
};

static uint8_t       s_active_ch            = 0;
static MeasurePhase  s_phase                = PH_START_PULSE;
static unsigned long s_slot_start           = 0;
static unsigned long s_phase_start          = 0;
static float         s_bus_v5               = 5.0f;
static bool          s_has_first_sample     = false;

// MQTT 定时发布全局变量（与 wifi_mqtt.cpp 共享）
uint32_t      g_mqtt_publish_interval_ms = 1000UL;
unsigned long s_last_mqtt_publish_time   = 0;

static unsigned long s_last_send_time       = 0;
static unsigned long s_last_sample_time     = 0;
static bool          s_last_led_on          = false;
static bool          s_last_onboard_led_on  = false;

// 1Hz 串口 CSV 连续采数 & 单通道阶跃测试状态（默认关闭，串口发指令触发）
static bool          s_csv_log_enabled      = false;
static float         s_live_temp[SENSOR_COUNT] = { NAN, NAN, NAN };

static bool          s_step_active          = false;
static uint8_t       s_step_ch              = 0;
static bool          s_step_heating         = false;
static unsigned long s_step_start_ms        = 0;
static unsigned long s_step_heat_ms         = 60000UL;
static unsigned long s_step_cool_ms         = 180000UL;
static constexpr float STEP_MAX_SAFE_TEMP_C = 65.0f;

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

void app_sync_channel_probe_type(int ch) {
    if (ch < 0 || ch >= SENSOR_COUNT) return;
    s_channels[ch].setProbeType((ProbeType)get_probe_type(ch));
    s_channels[ch].setEmptyDeltaT(get_empty_delta_t(ch));
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

// 统一 1Hz 非阻塞采样：读全部 3 路 18B20 并立即启动下一轮 12 位转换（750ms < 1000ms）
static void run_1hz_sampler(unsigned long now) {
    if (now - s_last_sample_time < THERMAL_SAMPLE_MS) return;
    s_last_sample_time = now;

    s_bus_v5 = read_5v_bus();

    for (int i = 0; i < SENSOR_COUNT; i++) {
        float t = s_channels[i].readTempC();
        s_channels[i].requestTemp();
        s_live_temp[i] = ThermalChannel::isValidTemp(t) ? t : NAN;
        if (!s_step_active) {
            s_channels[i].onTempSample(t, now);
        }
    }
    s_has_first_sample = true;

    if (s_csv_log_enabled || s_step_active) {
        char tbuf[SENSOR_COUNT][16];
        for (int i = 0; i < SENSOR_COUNT; i++) {
            if (isnan(s_live_temp[i])) {
                tbuf[i][0] = '\0';
            } else {
                snprintf(tbuf[i], sizeof(tbuf[i]), "%.2f", s_live_temp[i]);
            }
        }
        uint8_t cur_ch = s_step_active ? (s_step_ch + 1) : (s_active_ch + 1);
        Serial.printf("CSV,%lu,%.2f,%u,%s,%u,%s,%u,%s,%u\n",
                      now, s_bus_v5, cur_ch,
                      tbuf[0], s_channels[0].isHeating() ? 1 : 0,
                      tbuf[1], s_channels[1].isHeating() ? 1 : 0,
                      tbuf[2], s_channels[2].isHeating() ? 1 : 0);
    }
}

// 按 3 路封装组合动态计算每通道时间片（保证陶瓷管有足够 60s 冷却窗口，全不锈钢则保持 ~45s）
static unsigned long get_channel_slot_ms(uint8_t ch_idx) {
    unsigned long sum_base  = 0;
    unsigned long req_cycle = 0;
    for (int i = 0; i < SENSOR_COUNT; i++) {
        unsigned long base = s_channels[i].getHeatMs() + 5000UL;
        sum_base += base;
        unsigned long margin = (s_channels[i].getProbeType() == PROBE_CERAMIC) ? 8000UL : 5000UL;
        unsigned long need   = s_channels[i].getHeatMs() + s_channels[i].getCoolEndMs() + margin;
        if (need > req_cycle) req_cycle = need;
    }
    unsigned long my_base = s_channels[ch_idx].getHeatMs() + 5000UL;
    if (req_cycle > sum_base) {
        unsigned long extra = (req_cycle - sum_base + SENSOR_COUNT - 1) / SENSOR_COUNT;
        return my_base + extra;
    }
    return my_base;
}

static void run_thermal_scheduler(unsigned long now) {
    if (!s_has_first_sample) return;

    ThermalChannel& ch = s_channels[s_active_ch];

    switch (s_phase) {
        case PH_START_PULSE: {
            app_all_heaters_off();
            s_slot_start  = now;
            s_phase_start = now;
            if (ch.startPulse(now)) {
                s_phase = PH_HEATING;
            } else {
                s_phase = PH_COOLDOWN;
            }
            break;
        }
        case PH_HEATING: {
            if (now - s_phase_start >= ch.getHeatMs()) {
                ch.endHeating(now);
                app_all_heaters_off();
                s_phase = PH_COOLDOWN;
            }
            break;
        }
        case PH_COOLDOWN: {
            if (now - s_slot_start >= get_channel_slot_ms(s_active_ch)) {
                s_active_ch = (s_active_ch + 1) % SENSOR_COUNT;
                s_phase     = PH_START_PULSE;
            }
            break;
        }
    }
}

static void handle_serial_commands(unsigned long now) {
    static char   cmd_buf[64];
    static size_t cmd_len = 0;

    while (Serial.available() > 0) {
        char c = (char)Serial.read();
        if (c == '\r') continue;
        if (c == '\n') {
            cmd_buf[cmd_len] = '\0';
            cmd_len = 0;
            if (cmd_buf[0] == '\0') continue;

            if (strcmp(cmd_buf, "log on") == 0) {
                s_csv_log_enabled = true;
                Serial.println("# CSV,ms,v5,active_ch,ch1_t,ch1_heat,ch2_t,ch2_heat,ch3_t,ch3_heat");
            } else if (strcmp(cmd_buf, "log off") == 0) {
                s_csv_log_enabled = false;
                Serial.println("# CSV log stopped");
            } else if (strcmp(cmd_buf, "step stop") == 0) {
                if (s_step_active) {
                    app_all_heaters_off();
                    s_step_active  = false;
                    s_step_heating = false;
                    s_phase        = PH_START_PULSE;
                    Serial.println("# STEP stopped, resumed normal scheduler");
                }
            } else if (strncmp(cmd_buf, "step ", 5) == 0) {
                int ch_num = 0, heat_s = 60, cool_s = 180;
                int parsed = sscanf(cmd_buf + 5, "%d %d %d", &ch_num, &heat_s, &cool_s);
                if (parsed >= 1 && ch_num >= 1 && ch_num <= SENSOR_COUNT) {
                    if (heat_s < 5)   heat_s = 5;
                    if (heat_s > 120) heat_s = 120;
                    if (cool_s < 10)  cool_s = 10;
                    if (cool_s > 600) cool_s = 600;
                    app_all_heaters_off();
                    s_step_active     = true;
                    s_step_ch         = (uint8_t)(ch_num - 1);
                    s_step_heating    = true;
                    s_step_heat_ms    = (unsigned long)heat_s * 1000UL;
                    s_step_cool_ms    = (unsigned long)cool_s * 1000UL;
                    s_step_start_ms   = now;
                    s_csv_log_enabled = true;
                    s_channels[s_step_ch].heaterOn();
                    Serial.printf("# STEP started CH%d heat=%ds cool=%ds (max %.0fC)\n",
                                  ch_num, heat_s, cool_s, STEP_MAX_SAFE_TEMP_C);
                    Serial.println("# CSV,ms,v5,active_ch,ch1_t,ch1_heat,ch2_t,ch2_heat,ch3_t,ch3_heat");
                } else {
                    Serial.println("# Usage: step <1..3> [heat_s=60] [cool_s=180] | step stop | log on | log off");
                }
            }
        } else if (cmd_len + 1 < sizeof(cmd_buf)) {
            cmd_buf[cmd_len++] = c;
        }
    }
}

static void run_step_test(unsigned long now) {
    unsigned long elapsed = now - s_step_start_ms;
    if (s_step_heating) {
        bool over_temp = (!isnan(s_live_temp[s_step_ch]) && s_live_temp[s_step_ch] >= STEP_MAX_SAFE_TEMP_C);
        if (elapsed >= s_step_heat_ms || over_temp) {
            app_all_heaters_off();
            s_step_heating = false;
            if (over_temp) {
                Serial.printf("# STEP CH%u over-temp (%.2fC), heater off early\n",
                              s_step_ch + 1, s_live_temp[s_step_ch]);
            }
        }
    } else if (elapsed >= s_step_heat_ms + s_step_cool_ms) {
        app_all_heaters_off();
        s_step_active = false;
        s_phase       = PH_START_PULSE;
        Serial.printf("# STEP CH%u completed, resumed normal scheduler\n", s_step_ch + 1);
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

    pinMode(PIN_LED_ONBOARD, OUTPUT);
    digitalWrite(PIN_LED_ONBOARD, LOW); // 高电平点亮，上电初始灭

    esp_log_level_set("*", ESP_LOG_NONE);

    Serial.begin(115200);
    delay(500);
    Serial.println("\n====================================");
    Serial.println("ESP32 3-CH Thermal Liquid Sensor (frac_10_30)");
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
        s_channels[i].setProbeType((ProbeType)get_probe_type(i));
        s_channels[i].setEmptyDeltaT(get_empty_delta_t(i));
        s_channels[i].setRatios(get_channel_ratio_liquid(i), get_channel_ratio_empty(i));
    }

    s_active_ch        = 0;
    s_phase            = PH_START_PULSE;
    s_slot_start       = millis();
    s_phase_start      = s_slot_start;
    s_last_sample_time = s_slot_start;
    s_has_first_sample = false;

    // 3. 启动 WiFi / MQTT 与 BLE 广播
    wifi_mqtt_init();
    ble_init();

    // 4. 启动 20s 硬件看门狗防干烧
    init_hardware_wdt();
}

void loop() {
    esp_task_wdt_reset();
    unsigned long now = millis();

    // 0. 处理串口调试/采数指令 (log on / log off / step ...)
    handle_serial_commands(now);

    // 1. 统一 1Hz 非阻塞测温采样 + 驱动分时加热脉冲（或单通道阶跃测试）
    run_1hz_sampler(now);
    if (s_step_active) {
        run_step_test(now);
    } else {
        run_thermal_scheduler(now);
    }

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

    // 3b. ESP32 开发板板载 LED (GPIO2, 高电平点亮：未连网灭，WiFi 慢闪 2s，MQTT 快闪 1s)
    bool onboard_led_on = false;
    if (wifi_is_connected()) {
        onboard_led_on = mqtt_is_connected() ? ((now % 1000) < 500) : ((now % 2000) < 1000);
    }
    if (onboard_led_on != s_last_onboard_led_on) {
        s_last_onboard_led_on = onboard_led_on;
        digitalWrite(PIN_LED_ONBOARD, onboard_led_on ? HIGH : LOW);
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
            cache.t_now        = ch.getTempNow();
            cache.t0           = ch.getT0();
            cache.t1           = ch.getT1();
            cache.t_peak       = ch.getTPeak();
            cache.delta_t      = ch.getDeltaT();
            cache.drop10       = ch.getDrop10();
            cache.drop30       = ch.getDrop30();
            cache.cool_ratio   = ch.getCoolRatio();
            cache.empty_dt     = ch.getEmptyDeltaT();
            cache.ratio_liquid = ch.getRatioLiquid();
            cache.ratio_empty  = ch.getRatioEmpty();
            cache.calibrated   = ch.isCalibrated();
            cache.calibrating  = ch.isCalibrating();
            cache.state        = (uint8_t)ch.getState();
            cache.probe_type   = (uint8_t)ch.getProbeType();
            cache.online       = ch.isOnline();
            cache.heating      = ch.isHeating();
            web_config_update_sensor(i, cache);
        }

        ble_update(out_sensors, out_states);

        if (now - s_last_mqtt_publish_time >= g_mqtt_publish_interval_ms) {
            s_last_mqtt_publish_time = now;
            mqtt_publish(out_sensors, state_byte);
        }

        // 仅在周期结果或状态变化时打印串口汇总（避免数据未变时刷屏）
        static char s_last_serial_buf[512] = "";
        char serial_buf[512];
        int  pos = snprintf(serial_buf, sizeof(serial_buf),
                            "[Thermal] 5V=%.2fV Active=CH%u Cal=%u WiFi=%u MQTT=%u StateByte=0x%02X\n",
                            s_bus_v5, s_active_ch + 1,
                            app_is_any_calibrating() ? 1 : 0,
                            wifi_is_connected() ? 1 : 0,
                            mqtt_is_connected() ? 1 : 0,
                            state_byte);
        for (int i = 0; i < SENSOR_COUNT && pos > 0 && (size_t)pos < sizeof(serial_buf); i++) {
            const ThermalChannel& ch = s_channels[i];
            int n = snprintf(serial_buf + pos, sizeof(serial_buf) - (size_t)pos,
                             "  CH%d(%s): frac=%.1f%% (dM=%.2f/dE=%.2f) T0=%.2f Tpk=%.2f dT=%.2f [%s]%s\n",
                             i + 1,
                             ch.getProbeType() == PROBE_CERAMIC ? "C" : "S",
                             ch.getCoolRatio() * 100.0f,
                             ch.getDrop10(),
                             ch.getDrop30(),
                             isnan(ch.getT0()) ? -99.0f : ch.getT0(),
                             isnan(ch.getTPeak()) ? -99.0f : ch.getTPeak(),
                             ch.getDeltaT(),
                             ThermalChannel::stateToString(ch.getState()),
                             ch.isHeating() ? " *HEAT*" : "");
            if (n < 0) break;
            pos += n;
        }
        if (strcmp(serial_buf, s_last_serial_buf) != 0) {
            strncpy(s_last_serial_buf, serial_buf, sizeof(s_last_serial_buf) - 1);
            s_last_serial_buf[sizeof(s_last_serial_buf) - 1] = '\0';
            Serial.print(serial_buf);
        }
    }
}
