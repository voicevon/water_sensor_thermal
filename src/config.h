#pragma once

// ============================================================
//  全局配置文件 — 所有硬件参数、协议常量、网络配置的唯一来源
//  框架与通信协议（WiFi / WebServer / BLE / MQTT）完全兼容 water_sensor_hx711
// ============================================================

#include <Arduino.h>

#define SENSOR_COUNT    3

// -------- 热式传感器硬件引脚定义（与 docs/main_board_schematic.md 一致） --------
// 3 路独立 1-Wire 18B20 数据线（CH1 碳源, CH2 次氯酸钠, CH3 聚合氯化铁）
#define THERMAL_DQ_1_PIN    25
#define THERMAL_DQ_2_PIN    26
#define THERMAL_DQ_3_PIN    27

// 3 路高边 P-MOS 加热使能（HIGH = 加热, LOW = 断开）
#define THERMAL_HEAT_1_PIN  16
#define THERMAL_HEAT_2_PIN  17
#define THERMAL_HEAT_3_PIN  18

// 5V 加热母线电压监测（ADC1_CH6，20k / 10k 分压比 = 1/3）
#define PIN_5V_MON          34

// 板内状态 LED（载板 GPIO19，低电平点亮）
#define PIN_LED_STAT        19

// ESP32 开发板板载 LED（GPIO2，高电平点亮：未连网灭，WiFi 慢闪 2s，MQTT 快闪 1s）
#define PIN_LED_ONBOARD     2

// -------- 探头封装类型（支持三路混合使用：2路不锈钢 + 1路陶瓷） --------
enum ProbeType : uint8_t {
    PROBE_STEEL   = 0, // 0.5mm 薄壁不锈钢管：10s 加热，frac(0→10/30s)
    PROBE_CERAMIC = 1  // 2.0mm 厚壁99刚玉陶瓷管：30s 加热，tail(10→30/60s)
};

// 出厂默认各通道封装类型（CH1/CH2 不锈钢，CH3 陶瓷，可在 Web 页面按通道随时切换）
#define DEFAULT_PROBE_TYPE_0    PROBE_STEEL
#define DEFAULT_PROBE_TYPE_1    PROBE_STEEL
#define DEFAULT_PROBE_TYPE_2    PROBE_CERAMIC

// -------- 热式传感时序与算法参数（按封装类型自适应） --------
#define THERMAL_SAMPLE_MS       1000UL  // 18B20 1Hz 非阻塞实时采样周期 (>750ms 12位转换)
#define THERMAL_PEAK_WINDOW_MS  5000UL  // 断电后峰值追踪窗口 5s (覆盖 2~3s 热传导滞后)
#define THERMAL_MIN_DROP_C      0.30f   // 冷却窗口最小有效降温幅度，防除零/异常扰动
#define THERMAL_CAL_SAMPLES     3       // 可选空管自动调参连续采样轮数
#define THERMAL_DEBOUNCE_COUNT  1       // 状态切换确认轮数
#define THERMAL_WDT_TIMEOUT_S   20      // 硬件看门狗 20 秒防干烧

// 0.5mm 不锈钢管参数（实测：空气 36~42%，静水 58~63%）
#define STEEL_HEAT_TIME_MS      10000UL // 10s 加热 (~15J)
#define STEEL_COOL_SKIP_MS      0UL     // 从峰值起算
#define STEEL_COOL_MID_MS       10000UL // 峰值后 10s
#define STEEL_COOL_END_MS       30000UL // 峰值后 30s
#define STEEL_RATIO_LIQUID      0.52f   // frac > 52% → 判有液
#define STEEL_RATIO_EMPTY       0.46f   // frac < 46% → 判空管

// 2.0mm 陶瓷管参数（实测：空气 ~66%，静水 ~82%，跳过前 10s 厚壁自身吸热）
#define CERAMIC_HEAT_TIME_MS    30000UL // 30s 加热 (~45J)
#define CERAMIC_COOL_SKIP_MS    10000UL // 跳过峰值后前 10s
#define CERAMIC_COOL_MID_MS     30000UL // 峰值后 30s
#define CERAMIC_COOL_END_MS     60000UL // 峰值后 60s
#define CERAMIC_RATIO_LIQUID    0.75f   // tail(10→30/60s) > 75% → 判有液
#define CERAMIC_RATIO_EMPTY     0.70f   // tail(10→30/60s) < 70% → 判空管

// 兼容旧宏别名（默认指向不锈钢参数）
#define DEFAULT_RATIO_LIQUID    STEEL_RATIO_LIQUID
#define DEFAULT_RATIO_EMPTY     STEEL_RATIO_EMPTY

// -------- 采样与广播发送定时周期 (1 Hz，与 water_sensor_hx711 一致) --------
#define SEND_INTERVAL_MS        1000UL

// -------- BLE 广播参数（与 water_sensor_hx711 / water_relay 完全兼容） --------
#define BLE_DEVICE_NAME         "FengBLE"
// Company ID: 0xFFFF（厂商测试标识），BLE 小端序：LSB 在前
#define BLE_COMPANY_ID_LSB      0xFF
#define BLE_COMPANY_ID_MSB      0xFF

// -------- WiFi 网络配置 --------
#define FACTORY_WIFI_SSID        "Perfect"
#define FACTORY_WIFI_PASSWORD    "12344321"

// 热式传感器专用配网热点名称（便于现场与 AP_HX711 区分）
#define FACTORY_WIFI_AP_SSID     "AP_THERMAL"
#define FACTORY_WIFI_AP_PASSWORD "12344321"

// STA 断线重连：基础间隔起指数退避（20s→40s→80s→160s→320s 封顶）
#define WIFI_RECONNECT_BASE_MS           20000UL
#define WIFI_RECONNECT_BACKOFF_MAX_SHIFT 4

// -------- MQTT Broker & 设备命名配置（与 water_sensor_hx711 完全兼容） --------
#define FACTORY_DEVICE_NAME         "home"
#define FACTORY_MQTT_BROKER         "voicevon.vicp.io"
#define FACTORY_MQTT_PORT           1883
#define MQTT_CONTROL_TOPIC          "water/sensor/start"
#define MQTT_STATUS_TOPIC           "water/sensor/status"
#define MQTT_RECONNECT_INTERVAL_MS  5000UL
