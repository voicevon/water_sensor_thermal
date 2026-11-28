# water_sensor_thermal

污水处理厂三通道脉冲热式管道有液检测传感器（全栈项目：系统设计文档 + KiCad 原理图/PCB + 机械探头工艺 + ESP32 PlatformIO 固件 + 1Hz 串口采数分析工具）。

面向 **碳源、次氯酸钠、聚合氯化铁** 三路强腐蚀/易挂壁/易结垢化工药剂管道（DN75 PPR 水平管），采用 **1 拖 3 单主控盒** 与 **脉冲加热 + 冷却衰减比算法** 实现免标定开箱即用、宽温（-10℃ ~ +60℃）可靠有液/空管检测，支持 **不锈钢/陶瓷探头混合封装**，并通过 **BLE 广播、WiFi Web 后台及 MQTT** 与上位系统（`water_relay` / 监控平台）无缝对接。

---

## 目录结构

```text
water_sensor_thermal/
├── README.md                          # 项目总览与文档索引
├── platformio.ini                     # PlatformIO 构建配置（ESP32 Arduino）
├── src/                               # ESP32 固件源码
│   ├── config.h                       # 硬件引脚、不锈钢/陶瓷算法参数、WiFi/MQTT/BLE 配置
│   ├── main.cpp                       # 1Hz 统一采样、动态分时轮巡调度、串口采数与阶跃测试
│   ├── ThermalChannel.{h,cpp}         # 单通道峰值追踪、冷却衰减比计算、混合封装与迟滞判定
│   ├── ble_adv.{h,cpp}                # 13B BLE 广播封装（兼容 water_sensor_hx711 / water_relay）
│   ├── wifi_mqtt.{h,cpp}              # WiFi 指数退避重连与 MQTT 异步连接/上报/远程控制
│   ├── web_config.{h,cpp}             # AP_STA 配网、Web REST API 与每通道在线配置
│   ├── index_html.h                   # 内嵌紧凑型暗色 SPA 监控与配置页面
│   ├── nvs_config.{h,cpp}             # NVS 掉电持久化（网络、封装类型、阈值、调参基准）
│   └── data_cache.{h,cpp}             # 实时采样数据缓存
├── tools/
│   └── thermal_logger.py              # 1Hz 串口 CSV 采数、阶跃测试与自动绘图分析脚本
├── docs/                              # 完整设计与演进文档
│   ├── design_spec.md                 # 系统设计规格书 (v2.0)：工况、冷却衰减比算法、通信协议
│   ├── main_board_schematic.md        # 主控板原理图与 BOM（最新 DevKit V1 载板版 v4）
│   ├── probe_mechanical_spec.md       # 探头机械结构、标准件清单、混合封装与灌封工艺 (v2.1)
│   ├── hardware_design.md             # 早期裸模组硬件方案草稿与 BOM (v2.0 历史存档)
│   └── pcb_layout.md                  # 早期 PCB 布局规划与布线规范 (v1.0 历史存档)
└── hardware/
    └── pcb/
        ├── main_board_pcb/            # 主控载板 KiCad 工程 (88×78 mm) + Python 生成脚本
        │   ├── main_board_pcb.kicad_pro
        │   ├── main_board_pcb.kicad_sch
        │   ├── main_board_pcb.kicad_pcb
        │   ├── generate_main_sch.py
        │   └── generate_main_pcb.py
        └── probe_mini_pcb/            # 探头微型传感板 KiCad 工程 (4.8×12 mm) + Python 生成脚本
            ├── probe_mini_pcb.kicad_pcb
            └── generate_probe_pcb.py
```

---

## 设计演进与核心特性

1. **算法升级：从温升 $\Delta T$ 演进为免标定冷却衰减比（[design_spec.md](docs/design_spec.md)）**：
   - 统一 1Hz 非阻塞采集 3 路 **SOT-23-3 18B20**（独立 GPIO：`25/26/27`），加热断电后追踪 5 s 真实温度峰值，以归一化冷却衰减比判定有液/空管，天然免疫环境/水温变化、5V 电源压降、阻值公差与连续轮巡热累积漂移。
   - **混合探头自适应**：每通道可在 Web 页面独立切换封装类型并掉电保存（出厂默认 CH1/CH2 不锈钢、CH3 陶瓷）：
     - **0.5 mm 不锈钢管（`PROBE_STEEL`）**：10 s 加热，`frac_10_30 = (T_peak - T10) / (T_peak - T30)`（空气 36~42% vs 水中 58~63%，默认阈值 `>52%` 有液 / `<46%` 空管；3 路全不锈钢总周期 **45 s**）。
     - **2.0 mm 99 刚玉陶瓷管（`PROBE_CERAMIC`）**：30 s 加热，跳过前 10 s 厚壁自身吸热取 `tail(10→30/60s) = (T10 - T30) / (T10 - T60)`（空气 ~66% vs 水中 ~82%，默认阈值 `>75%` 有液 / `<70%` 空管；混合配置自动动态扩展轮巡周期至 **~98 s**）。
2. **主控板演进（[hardware_design.md](docs/hardware_design.md) → [main_board_schematic.md](docs/main_board_schematic.md)）**：
   - 最终量产载板（`v4`，`88×78 mm`）采用插接式 **30-Pin ESP32 DevKit V1 载板**，复用开发板自带 USB 烧录口、3.3V LDO 与 PCB 天线。
   - 宽压 `XL1509-5.0E1`（12V~24V → 恒定 5.0V）+ `GPIO34` 5V 母线电压监测 + 3 路高边 P-MOS 开关（`S8050 + AO3401A`），实现探头 **4 芯共地线缆**（`5V_HEAT / 3V3 / DQ / GND`）及上电/复位/看门狗默认关断防干烧保护。
3. **探头机械结构演进（[probe_mechanical_spec.md](docs/probe_mechanical_spec.md)）**：
   - 强腐蚀工位采用 **100 mm 长 99 刚玉陶瓷盲管（OD10×ID6×壁厚2mm）一管通到底 + 外购 PP/PTFE 10mm 卡套密封接头**，实现零机加工、浸液区零接缝；非强腐/台架工位兼容 0.5 mm 不锈钢盲管。
   - 配套 `4.8 × 12.0 mm` 探头微型 PCB（双 33Ω 1206 并联 = `16.5 Ω`，~1.52 W @ 5V）。
4. **全栈通信与调试工具链**：
   - **BLE 广播**：1Hz 被动广播 `FengBLE`（13 字节载荷），与 `water_sensor_hx711` / `water_relay` 100% 协议兼容。
   - **Web 本地后台**：常驻 `AP_THERMAL`（密码 `12344321`）+ STA 双模，手机/电脑直连 `192.168.4.1` 查看三通道实时对比表、切换探头封装、微调阈值或一键空管自动调参。
   - **MQTT 远程对接**：支持状态上报（`water/sensor/status`）与远程重启/标定（`water/{name}/control`）。
   - **串口采数与阶跃分析**：通过 `tools/thermal_logger.py` 一键记录 1Hz 温度与 5V 压降 CSV 并自动出图。

---

## 文档与工程快速索引

| 文档 / 工程 | 说明 | 状态 |
| :--- | :--- | :--- |
| [docs/design_spec.md](docs/design_spec.md) | 应用工况、冷却衰减比算法、动态时序、BLE/Web/MQTT 通信协议 | **最新系统规格 (v2.0)** |
| [docs/main_board_schematic.md](docs/main_board_schematic.md) | 主控载板完整原理图、引脚表、31 条网络表、BOM 与上电测试指南 | **最新硬件基准 (v4)** |
| [docs/probe_mechanical_spec.md](docs/probe_mechanical_spec.md) | 99 刚玉盲管/不锈钢混合探头结构、标准件清单、灌封 SOP 与验收指标 | **最新结构基准 (v2.1)** |
| [tools/thermal_logger.py](tools/thermal_logger.py) | 1Hz 串口 CSV 采集、单通道阶跃测试与温度曲线自动对比工具 | **最新测试工具** |
| [hardware/pcb/main_board_pcb/](hardware/pcb/main_board_pcb/) | 主控载板 KiCad 原理图/PCB 与 Python 生成脚本 | **最新 PCB** |
| [hardware/pcb/probe_mini_pcb/](hardware/pcb/probe_mini_pcb/) | 探头微型板 KiCad PCB 与 Python 生成脚本 | **最新 PCB** |
| [docs/hardware_design.md](docs/hardware_design.md) | 早期裸模组方案详细电路与 BOM | 历史设计存档 |
| [docs/pcb_layout.md](docs/pcb_layout.md) | 早期 PCB 正反面布局规范与反热短路过孔设计要点 | 历史设计存档 |

---

## 固件编译、烧录与台架采数

本项目使用 [PlatformIO](https://platformio.org/) 构建（目标板：`esp32dev`）：

```bash
# 编译固件
pio run

# 烧录到 ESP32 DevKit V1
pio run -t upload

# 打开串口监视器（115200 bps）
pio device monitor
```

使用 Python 脚本进行 1Hz 串口采数与自动绘图（依赖 `pyserial`、`matplotlib`）：

```bash
# 自动检测串口，采集 15 分钟并自动生成对比图（保存至 logs/）
python tools/thermal_logger.py --duration 15

# 对指定通道执行阶跃测试（如 CH3 加热 60s + 冷却 180s）
python tools/thermal_logger.py --duration 5 --step "step 3 60 180"

# 对已有 CSV 重新统计并出图
python tools/thermal_logger.py --plot-only logs/thermal_20261128_150855.csv
```

> ⚠️ **上电安全提醒**：切勿同时接入载板 `J1` 的 12~24V 工业电源与电脑 USB 调试线，烧录时请拔下 DevKit 单独连 USB 或断开 `J1` 电源。详见 [main_board_schematic.md 第 7 节](docs/main_board_schematic.md#7-焊接装配与首次上电测试指南)。
