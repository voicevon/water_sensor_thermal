#!/usr/bin/env python3
"""
ESP32 热式液位传感器 1Hz 串口数据采集与 CH1(空气) vs CH3(水) 曲线对比工具

用法示例:
  1) 自动检测串口，采集 20 分钟并自动出图:
     python tools/thermal_logger.py --duration 20

  2) 指定串口采集 10 分钟:
     python tools/thermal_logger.py --port COM4 --duration 10

  3) 不限时长采集（按 Ctrl+C 停止并自动出图）:
     python tools/thermal_logger.py --port COM4 --duration 0

  4) 对已保存的 CSV 重新生成对比图与统计摘要:
     python tools/thermal_logger.py --plot-only logs/thermal_20261128_121000.csv
"""

import argparse
import csv
import datetime as dt
import os
import sys
import time
from pathlib import Path

import serial
import serial.tools.list_ports

import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt


def pick_serial_port(requested: str | None) -> str:
    ports = list(serial.tools.list_ports.comports())
    if requested:
        return requested

    if not ports:
        print("未检测到任何串口设备，请检查 USB 连接或用 --port COMx 指定。", file=sys.stderr)
        sys.exit(1)

    # 优先匹配常见 USB 转串口芯片 (CH340 / CP210x / FTDI / USB Serial)
    keywords = ("CH340", "CP210", "USB", "UART", "FTDI", "Silicon Labs", "JTAG")
    usb_ports = [
        p for p in ports
        if any(k.lower() in (p.description or "").lower() for k in keywords)
        and "bluetooth" not in (p.description or "").lower()
    ]

    if len(usb_ports) == 1:
        p = usb_ports[0]
        print(f"自动选择串口: {p.device} ({p.description})")
        return p.device

    print("检测到以下串口:")
    for idx, p in enumerate(ports, 1):
        print(f"  [{idx}] {p.device} - {p.description}")
    choice = input("请输入序号或串口名 (直接回车选 1): ").strip()
    if not choice:
        return ports[0].device
    if choice.isdigit() and 1 <= int(choice) <= len(ports):
        return ports[int(choice) - 1].device
    return choice


def parse_float(val: str) -> float | None:
    val = val.strip()
    if not val:
        return None
    try:
        return float(val)
    except ValueError:
        return None


def analyze_cycles(times: list[float], temps: list[float | None], heats: list[int], name: str):
    """提取每个加热脉冲的起止温升 ΔT 与断电后 10s 回落量。"""
    valid_t = [t for t in temps if t is not None]
    if not valid_t:
        print(f"  {name}: 无有效温度数据")
        return

    print(
        f"  {name}: 样本={len(valid_t)} | 最低={min(valid_t):.2f}℃ | "
        f"最高={max(valid_t):.2f}℃ | 均值={sum(valid_t)/len(valid_t):.2f}℃"
    )

    cycles = []
    n = len(heats)
    i = 0
    while i < n:
        if heats[i] == 1:
            start = i
            while i < n and heats[i] == 1:
                i += 1
            end = i - 1
            t_start = temps[start]
            t_end = temps[end]
            if t_start is not None and t_end is not None and (times[end] - times[start]) >= 3.0:
                dt_heat = t_end - t_start
                # 找断电后约 10 秒的冷却温度
                t_cool10 = None
                for j in range(end + 1, n):
                    if times[j] - times[end] >= 10.0 and temps[j] is not None:
                        t_cool10 = temps[j]
                        break
                drop10 = (t_end - t_cool10) if t_cool10 is not None else None
                cycles.append((times[start], t_start, t_end, dt_heat, drop10))
        else:
            i += 1

    if cycles:
        avg_dt = sum(c[3] for c in cycles) / len(cycles)
        drops = [c[4] for c in cycles if c[4] is not None]
        avg_drop = (sum(drops) / len(drops)) if drops else None
        extra = f", 断电10s平均回落={avg_drop:.2f}℃" if avg_drop is not None else ""
        print(f"    检测到 {len(cycles)} 个完整加热周期: 平均温升 ΔT={avg_dt:.2f}℃{extra}")
        for idx, (ts, t0, t1, d_t, dr) in enumerate(cycles[-5:], max(1, len(cycles) - 4)):
            dr_str = f", 10s回落={dr:.2f}℃" if dr is not None else ""
            print(f"      周期#{idx} (@{ts:.0f}s): T0={t0:.2f}℃ -> T1={t1:.2f}℃ (ΔT={d_t:+.2f}℃{dr_str})")


def plot_csv(csv_path: Path, png_path: Path | None = None):
    times: list[float] = []
    v5_list: list[float | None] = []
    ch1_t: list[float | None] = []
    ch1_h: list[int] = []
    ch3_t: list[float | None] = []
    ch3_h: list[int] = []

    with csv_path.open("r", encoding="utf-8", newline="") as f:
        reader = csv.DictReader(f)
        for row in reader:
            t_s = parse_float(row.get("elapsed_s", ""))
            if t_s is None:
                continue
            times.append(t_s)
            v5_list.append(parse_float(row.get("v5", "")))
            ch1_t.append(parse_float(row.get("ch1_t", "")))
            ch1_h.append(int(row.get("ch1_heat", "0") or 0))
            ch3_t.append(parse_float(row.get("ch3_t", "")))
            ch3_h.append(int(row.get("ch3_heat", "0") or 0))

    if not times:
        print("CSV 中没有有效数据，跳过绘图。")
        return

    print("\n================ 数据统计摘要 ================")
    print(f"文件: {csv_path}  (总时长: {times[-1]:.1f} s, 共 {len(times)} 条)")
    analyze_cycles(times, ch1_t, ch1_h, "CH1 (空气)")
    analyze_cycles(times, ch3_t, ch3_h, "CH3 (水中)")
    print("==============================================")

    # 配置中文字体（Windows 常见字体兜底）
    plt.rcParams["font.sans-serif"] = ["Microsoft YaHei", "SimHei", "DejaVu Sans"]
    plt.rcParams["axes.unicode_minus"] = False

    t_min = [t / 60.0 for t in times]
    fig, (ax1, ax2, ax3) = plt.subplots(
        3, 1, figsize=(13, 9), sharex=True,
        gridspec_kw={"height_ratios": [3, 2.5, 1.2]}
    )
    fig.suptitle(f"CH1(空气) vs CH3(水) 热式探头 1Hz 实时曲线对比 ({csv_path.name})", fontsize=13)

    # 1) 绝对温度曲线
    ax1.plot(t_min, ch1_t, color="#f97316", linewidth=1.6, label="CH1 温度 (空气)")
    ax1.plot(t_min, ch3_t, color="#06b6d4", linewidth=1.6, label="CH3 温度 (水)")
    ax1.set_ylabel("绝对温度 (℃)")
    ax1.grid(True, alpha=0.3)
    ax1.legend(loc="upper right")

    # 标注加热区间底色
    def shade_heat(ax, heats, color, label):
        in_seg = False
        seg_start = 0.0
        labeled = False
        for idx, h in enumerate(heats):
            if h == 1 and not in_seg:
                in_seg = True
                seg_start = t_min[idx]
            elif h == 0 and in_seg:
                in_seg = False
                ax.axvspan(seg_start, t_min[idx], color=color, alpha=0.14,
                           label=None if labeled else label)
                labeled = True
        if in_seg:
            ax.axvspan(seg_start, t_min[-1], color=color, alpha=0.14,
                       label=None if labeled else label)

    shade_heat(ax1, ch1_h, "#f97316", "CH1 加热中")
    shade_heat(ax1, ch3_h, "#06b6d4", "CH3 加热中")
    ax1.legend(loc="upper right", fontsize=9)

    # 2) 相对起始温度的变化曲线（便于直接重叠比较波动幅度与响应速度）
    ch1_base = next((x for x in ch1_t if x is not None), None)
    ch3_base = next((x for x in ch3_t if x is not None), None)
    ch1_rel = [(x - ch1_base) if (x is not None and ch1_base is not None) else None for x in ch1_t]
    ch3_rel = [(x - ch3_base) if (x is not None and ch3_base is not None) else None for x in ch3_t]

    ax2.plot(t_min, ch1_rel, color="#f97316", linewidth=1.6,
             label=f"CH1 相对温变 (起点 {ch1_base:.2f}℃)" if ch1_base is not None else "CH1 相对温变")
    ax2.plot(t_min, ch3_rel, color="#06b6d4", linewidth=1.6,
             label=f"CH3 相对温变 (起点 {ch3_base:.2f}℃)" if ch3_base is not None else "CH3 相对温变")
    shade_heat(ax2, ch1_h, "#f97316", "")
    shade_heat(ax2, ch3_h, "#06b6d4", "")
    ax2.axhline(0, color="#64748b", linestyle="--", linewidth=0.8)
    ax2.set_ylabel("相对起始温度 ΔT (℃)")
    ax2.grid(True, alpha=0.3)
    ax2.legend(loc="upper right", fontsize=9)

    # 3) 5V 母线电压曲线（排查加热压降）
    ax3.plot(t_min, v5_list, color="#10b981", linewidth=1.2, label="5V 母线电压")
    ax3.set_ylabel("5V (V)")
    ax3.set_xlabel("采集时间 (分钟)")
    ax3.grid(True, alpha=0.3)
    ax3.legend(loc="upper right", fontsize=9)

    plt.tight_layout()
    out_png = png_path or csv_path.with_suffix(".png")
    fig.savefig(out_png, dpi=150)
    plt.close(fig)
    print(f"对比曲线图已保存: {out_png}")


def run_logger(port: str, baud: int, duration_min: float, out_dir: Path, step_cmd: str | None):
    out_dir.mkdir(parents=True, exist_ok=True)
    ts_str = dt.datetime.now().strftime("%Y%m%d_%H%M%S")
    csv_path = out_dir / f"thermal_{ts_str}.csv"

    print(f"正在打开串口 {port} @ {baud} ...")
    ser = serial.Serial(port, baud, timeout=1.0)
    time.sleep(0.5)
    ser.reset_input_buffer()

    if step_cmd:
        print(f"发送阶跃测试指令: {step_cmd}")
        ser.write((step_cmd.strip() + "\n").encode("ascii"))
    else:
        print("发送指令: log on (开启 ESP32 1Hz 连续采数)")
        ser.write(b"log on\n")

    start_wall = time.time()
    duration_s = duration_min * 60.0 if duration_min > 0 else None
    count = 0

    print(f"开始记录到: {csv_path}")
    if duration_s:
        print(f"计划采集时长: {duration_min:.1f} 分钟（随时可按 Ctrl+C 提前结束并出图）")
    else:
        print("持续采集中（按 Ctrl+C 结束并自动出图）")

    header = [
        "timestamp", "elapsed_s", "esp_ms", "v5", "active_ch",
        "ch1_t", "ch1_heat", "ch2_t", "ch2_heat", "ch3_t", "ch3_heat"
    ]

    try:
        with csv_path.open("w", encoding="utf-8", newline="") as f:
            writer = csv.writer(f)
            writer.writerow(header)
            f.flush()

            while True:
                elapsed = time.time() - start_wall
                if duration_s is not None and elapsed >= duration_s:
                    print(f"\n已达到预定采集时长 ({duration_min:.1f} 分钟)，停止采集。")
                    break

                raw = ser.readline()
                if not raw:
                    continue
                line = raw.decode("utf-8", errors="replace").strip()
                if not line:
                    continue

                if line.startswith("CSV,"):
                    parts = line.split(",")
                    if len(parts) < 10:
                        continue
                    now_str = dt.datetime.now().strftime("%Y-%m-%d %H:%M:%S")
                    row = [now_str, f"{elapsed:.2f}"] + parts[1:10]
                    writer.writerow(row)
                    f.flush()
                    count += 1

                    v5 = parts[2]
                    act = parts[3]
                    c1_t, c1_h = parts[4] or "--", parts[5]
                    c3_t, c3_h = parts[8] or "--", parts[9]
                    mm, ss = divmod(int(elapsed), 60)
                    print(
                        f"\r[{mm:02d}:{ss:02d}] #{count:04d} | 5V={v5}V Act=CH{act} | "
                        f"CH1(空)={c1_t}℃(H={c1_h}) | CH3(水)={c3_t}℃(H={c3_h})   ",
                        end="", flush=True
                    )
                elif line.startswith("#"):
                    print(f"\n[ESP32] {line}")
    except KeyboardInterrupt:
        print("\n收到 Ctrl+C，正在停止采集...")
    finally:
        try:
            ser.write(b"log off\n")
            time.sleep(0.1)
            ser.close()
        except Exception:
            pass

    print(f"采集完成，共记录 {count} 条 1Hz 数据 -> {csv_path}")
    if count > 0:
        plot_csv(csv_path)


def main():
    parser = argparse.ArgumentParser(description="ESP32 热式探头 1Hz 串口采集与 CH1/CH3 曲线对比工具")
    parser.add_argument("--port", "-p", help="串口号，例如 COM4（省略则自动检测）")
    parser.add_argument("--baud", "-b", type=int, default=115200, help="波特率 (默认 115200)")
    parser.add_argument("--duration", "-d", type=float, default=15.0,
                        help="采集时长(分钟)，默认 15；设为 0 表示不限时直至 Ctrl+C")
    parser.add_argument("--out-dir", default="logs", help="CSV 与图片输出目录 (默认 logs)")
    parser.add_argument("--step", help="可选：发送单通道阶跃命令，例如 'step 1 60 180'")
    parser.add_argument("--plot-only", help="仅对已有 CSV 文件重新统计并绘图")
    args = parser.parse_args()

    if args.plot_only:
        plot_csv(Path(args.plot_only))
        return

    port = pick_serial_port(args.port)
    run_logger(port, args.baud, args.duration, Path(args.out_dir), args.step)


if __name__ == "__main__":
    main()
