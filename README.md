# water_sensor_thermal

污水处理厂三通道脉冲热式管道有液检测传感器（全栈项目：系统设计文档 + KiCad 原理图/PCB + 机械探头工艺 + ESP32 PlatformIO 固件）。

面向 **碳源、次氯酸钠、聚合氯化铁** 三路强腐蚀/易挂壁/易结垢化工药剂管道（DN75 PPR 水平管），采用 **1 拖 3 单主控盒** 与 **脉冲自加热温差法（$\Delta T = T_1 - T_0$）** 实现无金属裸露、宽温（-10℃ ~ +60℃）可靠有液/空管检测，并通过 BLE 广播、WiFi Web 配网及 MQTT 与上位系统通信。

---

## 目录结构

```text
water_sensor_thermal/
├── README.md                          # 项目总览与文档索引
├── platformio.ini                     # PlatformIO 构建配置（ESP32 Arduino）
├── src/                               # ESP32 固件源码
│   ├── config.h                       # 硬件引脚、热式时序、WiFi/MQTT/BLE 参数
│   ├── main.cpp                       # 主状态机与分时轮巡调度
│   ├── ThermalChannel.{h,cpp}         # 单通道脉冲加热、18B20 测温、标定与迟滞判定
│   ├── ble_adv.{h,cpp}                # BLE 广播封装
│   ├── wifi_mqtt.{h,cpp}              # WiFi 重连与 MQTT 上报/控制
│   ├── web_config.{h,cpp}             # Web 配网与现场标定后台
│   ├── index_html.h                   # 内嵌 Web 调试页面
│   ├── nvs_config.{h,cpp}             # NVS 掉电配置持久化
│   └── data_cache.{h,cpp}             # 实时采样数据缓存
├── docs/                              # 完整设计与演进文档
│   ├── design_spec.md                 # 系统设计规格书 (v1.0)：工况、原理、协议
│   ├── main_board_schematic.md        # 主控板原理图与 BOM（最新 DevKit V1 载板版 v4）
│   ├── probe_mechanical_spec.md       # 探头机械结构、标准件清单与灌封工艺 (v2.0)
│   ├── hardware_design.md             # 早期裸模组硬件方案草稿与 BOM (v2.0 存档)
│   └── pcb_layout.md                  # 早期 PCB 布局规划与布线规范 (v1.0 存档)
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

## 设计演进与核心思路

本项目保留了从早期方案探索到最终工程落地的完整设计文档，核心演进如下：

1. **传感原理（[design_spec.md](docs/design_spec.md)）**：
   - 采用**脉冲热式温差法**（定功率短时加热，仅比较加热前后相对温升 $\Delta T$），天然屏蔽济南室外铁皮屋 `-10℃ ~ +60℃` 季节温差与药剂自身温差，抗挂壁、抗结垢。
   - 测温器件选用数字 1-Wire **SOT-23-3 18B20**，三路独立 GPIO（`GPIO25/26/27`），单路探头更换免 ROM 配对、单路短路不牵连其他通道。
2. **主控板演进（[hardware_design.md](docs/hardware_design.md) → [main_board_schematic.md](docs/main_board_schematic.md)）**：
   - 早期草稿采用 `ESP32-WROOM-32E` 裸模组 + `CH340C` + `AMS1117`；最终量产载板（`v4`）优化为插接式 **30-Pin ESP32 DevKit V1 载板**，复用开发板自带 USB 烧录口、3.3V LDO 与天线，精简 BOM 并方便现场热插拔维护。
   - 电源采用宽压 `XL1509-5.0E1`（12V~24V → 恒定 5.0V）+ 3 路高边 P-MOS 开关（`S8050 + AO3401A`），实现探头 **4 芯共地线缆**（`5V_HEAT / 3V3 / DQ / GND`）及上电/复位默认关断防干烧保护。
3. **探头机械结构演进（[design_spec.md](docs/design_spec.md) → [probe_mechanical_spec.md](docs/probe_mechanical_spec.md)）**：
   - 从早期的“PTFE 车削探杆 + 粘接陶瓷帽”升级为 **100 mm 长 99 刚玉陶瓷盲管（OD10×ID6×壁厚2mm）一管通到底 + 外购 PP/PTFE 10mm 卡套密封接头**，实现**零机加工、浸液区零接缝**。
   - 配套 `4.8 × 12.0 mm` 探头微型 PCB（双 33Ω 1206 并联 = 16.5Ω，~1.52W @ 5V），适配 2 mm 厚陶瓷壁热容，固件采用 15s 单通道时间片（9.2s 加热 + 0.8s 转换，3 路总周期 45s < 60s）。

---

## 文档与工程快速索引

| 文档 / 工程 | 说明 | 状态 |
| :--- | :--- | :--- |
| [docs/design_spec.md](docs/design_spec.md) | 应用工况、脉冲热式原理、标定与判定算法、通信架构 | 系统规格基准 |
| [docs/main_board_schematic.md](docs/main_board_schematic.md) | 主控载板完整原理图、引脚表、31 条网络表、BOM 与上电测试指南 | **最新硬件基准 (v4)** |
| [docs/probe_mechanical_spec.md](docs/probe_mechanical_spec.md) | 99 刚玉盲管 + 卡套接头总装图、外购件清单、灌封 SOP 与开孔安装 | **最新结构基准 (v2.0)** |
| [hardware/pcb/main_board_pcb/](hardware/pcb/main_board_pcb/) | 主控载板 KiCad 原理图/PCB 与 Python 生成脚本 | **最新 PCB** |
| [hardware/pcb/probe_mini_pcb/](hardware/pcb/probe_mini_pcb/) | 探头微型板 KiCad PCB 与 Python 生成脚本 | **最新 PCB** |
| [docs/hardware_design.md](docs/hardware_design.md) | 早期裸模组方案详细电路与 BOM | 历史设计存档 |
| [docs/pcb_layout.md](docs/pcb_layout.md) | 早期 PCB 正反面布局规范与反热短路过孔设计要点 | 历史设计参考 |

---

## 固件编译与烧录

本项目使用 [PlatformIO](https://platformio.org/) 构建（目标板：`esp32dev`）：

```bash
# 编译固件
pio run

# 烧录到 ESP32 DevKit V1
pio run -t upload

# 打开串口监视器（115200 bps）
pio device monitor
```

> ⚠️ **上电安全提醒**：切勿同时接入载板 `J1` 的 12~24V 工业电源与电脑 USB 调试线，烧录时请拔下 DevKit 单独连 USB 或断开 `J1` 电源。详见 [main_board_schematic.md 第 7 节](docs/main_board_schematic.md#7-焊接装配与首次上电测试指南)。
