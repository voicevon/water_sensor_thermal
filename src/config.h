#pragma once

// ============================================================
//  全局配置文件 — 所有硬件参数、协议常量、网络配置的唯一来源
//  框架与通信协议（WiFi / WebServer / BLE / MQTT）完全兼容 water_sensor_hx711
// ============================================================

#include <Arduino.h>

#define SENSOR_COUNT    3

// -------- 热式传感器硬件引脚定义（与 sensor_design/main_board_schematic.md 一致） --------
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

// -------- 热式传感时序与算法参数（适配 OD10×ID6 壁厚 2mm 99刚玉盲管） --------
#define THERMAL_CONV_TIME_MS    800UL   // 18B20 12位转换等待时间 (规格书 750ms)
#define THERMAL_HEAT_PRE_MS     9200UL  // 加热至 9.2s 启动 T1 转换, 10.0s 读完关断 (~15J)
#define THERMAL_SLOT_TOTAL_MS   15000UL // 单通道时间片 15s (3路完整周期 = 45s < 60s)
#define THERMAL_CAL_SAMPLES     3       // 空管一键标定连续采样轮数
#define THERMAL_DEBOUNCE_COUNT  1       // 状态切换确认轮数
#define THERMAL_WDT_TIMEOUT_S   20      // 硬件看门狗 20 秒防干烧

// 默认每通道迟滞判定比例（支持按通道独立配置）
#define DEFAULT_RATIO_LIQUID    0.45f   // ΔT < 45% 空管温升 → 判有液
#define DEFAULT_RATIO_EMPTY     0.60f   // ΔT > 60% 空管温升 → 判空管

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
