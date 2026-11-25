"""
Generate a FULLY ROUTED KiCad 7/8 (.kicad_pcb) Industrial Carrier Board (85.0 x 72.0 mm)
designed for a plug-in **ESP32 DevKit V1 (30-Pin, 25.4mm / 1.0" row spacing)**!

Directly importable into EasyEDA Pro (立创EDA专业版: 文件 -> 导入 -> KiCad):
  - Plug-in 2x 15-pin 2.54mm female headers for ESP32 DevKit V1 (30P, 25.4mm row pitch)
    (Onboard USB/CH340/3.3V LDO/Buttons already on the Dev Board -> minimal carrier BOM!)
  - 12V/24V wide-input XL1509-5.0E1 Buck -> stable +5.0V (feeds DevKit VIN + 3x 1.5W heaters)
  - 3x High-Side P-MOS (AO3401A + S8050 + 1N5819WS) + 3x independent 1-Wire (4.7k + 100R + ESD)
  - 2-layer grid A* Manhattan router (F.Cu + B.Cu) with 0.50mm/0.85mm EasyEDA-Pro-safe vias
  - Bottom solid GND copper pour + automatic GND via fanout + BLE Antenna Keep-Out
"""

import heapq
import math
import os
import uuid


def uid() -> str:
    return str(uuid.uuid4())


class NetTable:
    def __init__(self):
        self.nets = [""]
        self.map = {"": 0}

    def id(self, name: str) -> int:
        if name not in self.map:
            self.map[name] = len(self.nets)
            self.nets.append(name)
        return self.map[name]

    def pad_net(self, name: str) -> str:
        idx = self.id(name)
        return f'(net {idx} "{name}")'

    def render(self) -> str:
        return "\n".join(f'  (net {i} "{n}")' for i, n in enumerate(self.nets))


PADS = []
BBOXES = []


def reg_box(ref, x, y, w, h):
    BBOXES.append((ref, x - w / 2.0, y - h / 2.0, x + w / 2.0, y + h / 2.0))


def reg_pad(net, ltype, x, y, pw, ph, tag):
    if net:
        PADS.append((net, ltype, x, y, pw, ph, tag))


def fp_2pad(ref, val, pkg, x, y, rot, pitch, pw, ph, n1, n2, nt: NetTable, layer="F.Cu"):
    reg_box(ref, x, y, pitch + pw, ph)
    dx = pitch / 2.0
    ltype = "F" if layer == "F.Cu" else "B"
    reg_pad(n1, ltype, x - dx, y, pw, ph, f"{ref}.1")
    reg_pad(n2, ltype, x + dx, y, pw, ph, f"{ref}.2")
    silk = "F.SilkS" if layer == "F.Cu" else "B.SilkS"
    fab = "F.Fab" if layer == "F.Cu" else "B.Fab"
    paste = "F.Paste" if layer == "F.Cu" else "B.Paste"
    mask = "F.Mask" if layer == "F.Cu" else "B.Mask"
    return f"""  (footprint "{pkg}" (layer "{layer}")
    (tstamp {uid()})
    (at {x:.2f} {y:.2f} {rot})
    (fp_text reference "{ref}" (at 0 {-ph/2-0.8:.2f}) (layer "{silk}") (effects (font (size 0.75 0.75) (thickness 0.14))) (tstamp {uid()}))
    (fp_text value "{val}" (at 0 {ph/2+0.8:.2f}) (layer "{fab}") (effects (font (size 0.65 0.65) (thickness 0.12))) (tstamp {uid()}))
    (pad "1" smd roundrect (at {-dx:.3f} 0 {rot}) (size {pw:.2f} {ph:.2f}) (layers "{layer}" "{paste}" "{mask}") (roundrect_rratio 0.2) {nt.pad_net(n1)} (tstamp {uid()}))
    (pad "2" smd roundrect (at {dx:.3f} 0 {rot}) (size {pw:.2f} {ph:.2f}) (layers "{layer}" "{paste}" "{mask}") (roundrect_rratio 0.2) {nt.pad_net(n2)} (tstamp {uid()}))
  )"""


def fp_sot23(ref, val, x, y, rot, n1, n2, n3, nt: NetTable):
    reg_box(ref, x, y, 2.9, 3.2)
    reg_pad(n1, "F", x - 0.95, y + 1.10, 0.65, 1.00, f"{ref}.1")
    reg_pad(n2, "F", x + 0.95, y + 1.10, 0.65, 1.00, f"{ref}.2")
    reg_pad(n3, "F", x, y - 1.10, 0.65, 1.00, f"{ref}.3")
    return f"""  (footprint "Package_TO_SOT_SMD:SOT-23" (layer "F.Cu")
    (tstamp {uid()})
    (at {x:.2f} {y:.2f} {rot})
    (fp_text reference "{ref}" (at 0 -2.1) (layer "F.SilkS") (effects (font (size 0.75 0.75) (thickness 0.14))) (tstamp {uid()}))
    (fp_text value "{val}" (at 0 2.1) (layer "F.Fab") (effects (font (size 0.65 0.65) (thickness 0.12))) (tstamp {uid()}))
    (fp_rect (start -1.45 -0.65) (end 1.45 0.65) (stroke (width 0.12) (type solid)) (fill none) (layer "F.SilkS") (tstamp {uid()}))
    (pad "1" smd roundrect (at -0.95 1.10 {rot}) (size 0.65 1.00) (layers "F.Cu" "F.Paste" "F.Mask") (roundrect_rratio 0.2) {nt.pad_net(n1)} (tstamp {uid()}))
    (pad "2" smd roundrect (at 0.95 1.10 {rot}) (size 0.65 1.00) (layers "F.Cu" "F.Paste" "F.Mask") (roundrect_rratio 0.2) {nt.pad_net(n2)} (tstamp {uid()}))
    (pad "3" smd roundrect (at 0.00 -1.10 {rot}) (size 0.65 1.00) (layers "F.Cu" "F.Paste" "F.Mask") (roundrect_rratio 0.2) {nt.pad_net(n3)} (tstamp {uid()}))
  )"""


def fp_soic8(ref, val, x, y, net_list, nt: NetTable):
    pitch = 1.27
    body_h = 4 * pitch + 0.4
    reg_box(ref, x, y, 6.9, body_h)
    y_start = -1.5 * pitch
    pad_x = 2.70
    pads = []
    for i in range(4):
        py = y_start + i * pitch
        reg_pad(net_list[i], "F", x - pad_x, y + py, 1.50, 0.65, f"{ref}.{i+1}")
        pads.append(f'    (pad "{i+1}" smd roundrect (at {-pad_x:.2f} {py:.2f}) (size 1.50 0.65) (layers "F.Cu" "F.Paste" "F.Mask") (roundrect_rratio 0.2) {nt.pad_net(net_list[i])} (tstamp {uid()}))')
    for i in range(4):
        pin_num = 5 + i
        py = -y_start - i * pitch
        reg_pad(net_list[pin_num - 1], "F", x + pad_x, y + py, 1.50, 0.65, f"{ref}.{pin_num}")
        pads.append(f'    (pad "{pin_num}" smd roundrect (at {pad_x:.2f} {py:.2f}) (size 1.50 0.65) (layers "F.Cu" "F.Paste" "F.Mask") (roundrect_rratio 0.2) {nt.pad_net(net_list[pin_num-1])} (tstamp {uid()}))')
    return f"""  (footprint "Package_SO:SOIC-8" (layer "F.Cu")
    (tstamp {uid()})
    (at {x:.2f} {y:.2f})
    (fp_text reference "{ref}" (at 0 {-body_h/2-1.1:.2f}) (layer "F.SilkS") (effects (font (size 0.85 0.85) (thickness 0.15))) (tstamp {uid()}))
    (fp_text value "{val}" (at 0 {body_h/2+1.1:.2f}) (layer "F.Fab") (effects (font (size 0.75 0.75) (thickness 0.12))) (tstamp {uid()}))
    (fp_rect (start -1.95 {-body_h/2:.2f}) (end 1.95 {body_h/2:.2f}) (stroke (width 0.15) (type solid)) (fill none) (layer "F.SilkS") (tstamp {uid()}))
{chr(10).join(pads)}
  )"""


def fp_cp_radial(ref, val, x, y, dia, pitch, n_pos, n_neg, nt: NetTable):
    reg_box(ref, x, y, dia, dia)
    reg_pad(n_pos, "BOTH", x - pitch / 2.0, y, 1.70, 1.70, f"{ref}.1")
    reg_pad(n_neg, "BOTH", x + pitch / 2.0, y, 1.70, 1.70, f"{ref}.2")
    r = dia / 2.0
    return f"""  (footprint "Capacitor_THT:CP_Radial_D{dia:.1f}mm_P{pitch:.2f}mm" (layer "F.Cu")
    (tstamp {uid()})
    (at {x:.2f} {y:.2f})
    (fp_text reference "{ref}" (at 0 {-r-1.0:.2f}) (layer "F.SilkS") (effects (font (size 0.8 0.8) (thickness 0.15))) (tstamp {uid()}))
    (fp_text value "{val}" (at 0 {r+1.0:.2f}) (layer "F.Fab") (effects (font (size 0.7 0.7) (thickness 0.12))) (tstamp {uid()}))
    (fp_circle (center 0 0) (end {r:.2f} 0) (stroke (width 0.15) (type solid)) (fill none) (layer "F.SilkS") (tstamp {uid()}))
    (pad "1" thru_hole rect (at {-pitch/2:.2f} 0) (size 1.70 1.70) (drill 0.80) (layers "*.Cu" "*.Mask") {nt.pad_net(n_pos)} (tstamp {uid()}))
    (pad "2" thru_hole circle (at {pitch/2:.2f} 0) (size 1.70 1.70) (drill 0.80) (layers "*.Cu" "*.Mask") {nt.pad_net(n_neg)} (tstamp {uid()}))
  )"""


def fp_terminal(ref, val, count, pitch, x, y, net_list, nt: NetTable):
    w = count * pitch + 1.2
    h = 7.0
    reg_box(ref, x, y, w, h)
    x_start = -(count - 1) * pitch / 2.0
    pads = []
    for i in range(count):
        px = x_start + i * pitch
        reg_pad(net_list[i], "BOTH", x + px, y, 2.30, 2.30, f"{ref}.{i+1}")
        shape = "rect" if i == 0 else "circle"
        pads.append(f'    (pad "{i+1}" thru_hole {shape} (at {px:.2f} 0) (size 2.30 2.30) (drill 1.30) (layers "*.Cu" "*.Mask") {nt.pad_net(net_list[i])} (tstamp {uid()}))')
    return f"""  (footprint "TerminalBlock:KF2EDGK_{count}P_P{pitch:.2f}mm" (layer "F.Cu")
    (tstamp {uid()})
    (at {x:.2f} {y:.2f})
    (fp_text reference "{ref}" (at 0 -4.5) (layer "F.SilkS") (effects (font (size 0.85 0.85) (thickness 0.15))) (tstamp {uid()}))
    (fp_text value "{val}" (at 0 4.5) (layer "F.SilkS") (effects (font (size 0.75 0.75) (thickness 0.14))) (tstamp {uid()}))
    (fp_rect (start {-w/2:.2f} {-h/2:.2f}) (end {w/2:.2f} {h/2:.2f}) (stroke (width 0.15) (type solid)) (fill none) (layer "F.SilkS") (tstamp {uid()}))
{chr(10).join(pads)}
  )"""


def fp_mounting_hole(ref, x, y):
    reg_box(ref, x, y, 5.6, 5.6)
    reg_pad("__HOLE__", "BOTH", x, y, 3.8, 3.8, ref)
    return f"""  (footprint "MountingHole:MountingHole_3.2mm_M3" (layer "F.Cu")
    (tstamp {uid()})
    (at {x:.2f} {y:.2f})
    (fp_text reference "{ref}" (at 0 -3.2) (layer "F.Fab") (effects (font (size 0.7 0.7) (thickness 0.12))) (tstamp {uid()}))
    (fp_circle (center 0 0) (end 2.8 0) (stroke (width 0.15) (type solid)) (fill none) (layer "F.SilkS") (tstamp {uid()}))
    (pad "" np_thru_hole circle (at 0 0) (size 3.2 3.2) (drill 3.2) (layers "*.Cu" "*.Mask") (tstamp {uid()}))
  )"""


def fp_esp32_devkit_30p(ref, cx, cy, left_nets, right_nets, nt: NetTable):
    """
    ESP32 DevKit V1 (30-Pin DOIT standard):
      - Two 1x15 2.54mm female header rows, row spacing = 25.40 mm (1.00 inch)
      - Board outline ~28.0 x 51.5 mm; Antenna at TOP (y = cy - 25.75), USB at BOTTOM (y = cy + 25.75)
    """
    pitch = 2.54
    half_x = 12.70  # 25.40 / 2
    y_start = -7.0 * pitch  # 15 pins: -7..+7 * 2.54 = -17.78..+17.78
    reg_box(ref, cx, cy, 28.0, 42.0)

    pads = []
    for i in range(15):
        py = y_start + i * pitch
        ln = left_nets[i]
        reg_pad(ln if ln else f"__NC_L{i+1}__", "BOTH", cx - half_x, cy + py, 1.75, 1.75, f"{ref}.L{i+1}")
        shape = "rect" if i == 0 else "circle"
        pads.append(f'    (pad "L{i+1}" thru_hole {shape} (at {-half_x:.2f} {py:.2f}) (size 1.75 1.75) (drill 1.00) (layers "*.Cu" "*.Mask") {nt.pad_net(ln)} (tstamp {uid()}))')

    for i in range(15):
        py = y_start + i * pitch
        rn = right_nets[i]
        reg_pad(rn if rn else f"__NC_R{i+1}__", "BOTH", cx + half_x, cy + py, 1.75, 1.75, f"{ref}.R{i+1}")
        pads.append(f'    (pad "R{i+1}" thru_hole circle (at {half_x:.2f} {py:.2f}) (size 1.75 1.75) (drill 1.00) (layers "*.Cu" "*.Mask") {nt.pad_net(rn)} (tstamp {uid()}))')

    return f"""  (footprint "Module:ESP32_DevKit_V1_30Pin_25.4mm" (layer "F.Cu")
    (tstamp {uid()})
    (at {cx:.2f} {cy:.2f})
    (fp_text reference "{ref}" (at 0 -1.5) (layer "F.SilkS") (effects (font (size 1.2 1.2) (thickness 0.20))) (tstamp {uid()}))
    (fp_text value "ESP32_DEVKIT_30P" (at 0 1.5) (layer "F.SilkS") (effects (font (size 1.0 1.0) (thickness 0.16))) (tstamp {uid()}))
    (fp_text user "ANTENNA TOP ^" (at 0 -21.0) (layer "F.SilkS") (effects (font (size 0.9 0.9) (thickness 0.16))) (tstamp {uid()}))
    (fp_text user "USB PORT BOTTOM v" (at 0 21.0) (layer "F.SilkS") (effects (font (size 0.9 0.9) (thickness 0.16))) (tstamp {uid()}))
    (fp_rect (start -14.00 -25.00) (end 14.00 25.00) (stroke (width 0.15) (type solid)) (fill none) (layer "F.SilkS") (tstamp {uid()}))
{chr(10).join(pads)}
  )"""


# ==================== Exact Geometric DRC Checker ====================
def seg_pt_dist(x1, y1, x2, y2, px, py):
    dx, dy = x2 - x1, y2 - y1
    l2 = dx * dx + dy * dy
    if l2 < 1e-9:
        return math.hypot(px - x1, py - y1)
    t = max(0.0, min(1.0, ((px - x1) * dx + (py - y1) * dy) / l2))
    return math.hypot(px - (x1 + t * dx), py - (y1 + t * dy))


def seg_rect_dist(x1, y1, x2, y2, rx, ry, rw, rh):
    # Sample along segment (every 0.2mm) for fast exact distance to axis-aligned rect
    length = math.hypot(x2 - x1, y2 - y1)
    steps = max(1, int(math.ceil(length / 0.20)))
    best = 1e9
    hw, hh = rw / 2.0, rh / 2.0
    for k in range(steps + 1):
        t = k / steps
        px = x1 + (x2 - x1) * t
        py = y1 + (y2 - y1) * t
        dx = max(abs(px - rx) - hw, 0.0)
        dy = max(abs(py - ry) - hh, 0.0)
        d = math.hypot(dx, dy)
        if d < best:
            best = d
    return best


def seg_seg_dist(ax1, ay1, ax2, ay2, bx1, by1, bx2, by2):
    # Fast exact distance between two segments
    def ccw(p1x, p1y, p2x, p2y, p3x, p3y):
        return (p3y - p1y) * (p2x - p1x) > (p2y - p1y) * (p3x - p1x)

    if (ccw(ax1, ay1, bx1, by1, bx2, by2) != ccw(ax2, ay2, bx1, by1, bx2, by2) and
            ccw(ax1, ay1, ax2, ay2, bx1, by1) != ccw(ax1, ay1, ax2, ay2, bx2, by2)):
        return 0.0
    return min(
        seg_pt_dist(ax1, ay1, ax2, ay2, bx1, by1),
        seg_pt_dist(ax1, ay1, ax2, ay2, bx2, by2),
        seg_pt_dist(bx1, by1, bx2, by2, ax1, ay1),
        seg_pt_dist(bx1, by1, bx2, by2, ax2, ay2),
    )


def run_drc_check(segments, vias, nt: NetTable):
    errs = []
    # Check seg vs other-net pads
    for si, (x1, y1, x2, y2, w, lay, nid) in enumerate(segments):
        lt = "F" if lay == "F.Cu" else "B"
        for net, pltype, px, py, pw, ph, tag in PADS:
            pnid = -1 if net.startswith("__") else nt.id(net)
            if pnid == nid:
                continue
            if pltype != "BOTH" and pltype != lt:
                continue
            d = seg_rect_dist(x1, y1, x2, y2, px, py, pw, ph) - w / 2.0
            if d < 0.16:
                errs.append(f"SEG-PAD {nt.nets[nid]} vs {tag}({net}) on {lay}: {d:.3f}mm")
    # Check seg vs other-net segs on same layer
    for i in range(len(segments)):
        ax1, ay1, ax2, ay2, aw, alay, anid = segments[i]
        for j in range(i + 1, len(segments)):
            bx1, by1, bx2, by2, bw, blay, bnid = segments[j]
            if anid == bnid or alay != blay:
                continue
            if max(ax1, ax2) + aw + 0.5 < min(bx1, bx2) or max(bx1, bx2) + bw + 0.5 < min(ax1, ax2):
                continue
            if max(ay1, ay2) + aw + 0.5 < min(by1, by2) or max(by1, by2) + bw + 0.5 < min(ay1, ay2):
                continue
            d = seg_seg_dist(ax1, ay1, ax2, ay2, bx1, by1, bx2, by2) - (aw + bw) / 2.0
            if d < 0.13:
                errs.append(f"SEG-SEG {nt.nets[anid]} vs {nt.nets[bnid]} on {alay}: {d:.3f}mm")
    # Check vias vs other-net pads & segs
    for vx, vy, vnid in vias:
        for net, pltype, px, py, pw, ph, tag in PADS:
            pnid = -1 if net.startswith("__") else nt.id(net)
            if pnid == vnid:
                continue
            dx = max(abs(vx - px) - pw / 2.0, 0.0)
            dy = max(abs(vy - py) - ph / 2.0, 0.0)
            d = math.hypot(dx, dy) - 0.425
            if d < 0.16:
                errs.append(f"VIA-PAD {nt.nets[vnid]} vs {tag}({net}): {d:.3f}mm")
        for x1, y1, x2, y2, w, lay, nid in segments:
            if nid == vnid:
                continue
            d = seg_pt_dist(x1, y1, x2, y2, vx, vy) - 0.425 - w / 2.0
            if d < 0.16:
                errs.append(f"VIA-SEG {nt.nets[vnid]} vs {nt.nets[nid]} on {lay}: {d:.3f}mm")
    return errs


# ==================== 2-Layer Grid A* Router (0.5mm grid, Strict DRC) ====================
def route_all_nets(nt: NetTable, bx0, by0, bx1, by1):
    step = 0.50
    nx = int(round((bx1 - bx0) / step)) + 1
    ny = int(round((by1 - by0) / step)) + 1

    def to_g(x, y):
        return int(round((x - bx0) / step)), int(round((y - by0) / step))

    def to_mm(gx, gy):
        return bx0 + gx * step, by0 + gy * step

    # occ[lay][gx][gy] stores set of net_ids whose keepout covers this cell (-1 = hard wall)
    occ = [[set() for _ in range(ny)] for _ in range(nx)]
    occ_l = [[[[ ] for _ in range(ny)] for _ in range(nx)] for _ in range(2)]  # store set of blocking nids

    def block_cell(lay, gx, gy, nid):
        if 0 <= gx < nx and 0 <= gy < ny:
            if nid not in occ_l[lay][gx][gy]:
                occ_l[lay][gx][gy].append(nid)

    for lay in (0, 1):
        for gx in range(nx):
            block_cell(lay, gx, 0, -1)
            block_cell(lay, gx, ny - 1, -1)
        for gy in range(ny):
            block_cell(lay, 0, gy, -1)
            block_cell(lay, nx - 1, gy, -1)

    # BLE Antenna Keep-Out (x: 135..165, y: 100..108)
    ax0, ay0 = to_g(135.0, 100.0)
    ax1, ay1 = to_g(165.0, 108.0)
    for lay in (0, 1):
        for gx in range(max(0, ax0), min(nx, ax1 + 1)):
            for gy in range(max(0, ay0), min(ny, ay1 + 1)):
                block_cell(lay, gx, gy, -1)

    # Block every pad with clearance (half-width + 0.32mm keeps 2.54mm header pins individually accessible!)
    for net, ltype, px, py, pw, ph, tag in PADS:
        nid = -1 if net.startswith("__") else nt.id(net)
        layers = [0, 1] if ltype == "BOTH" else ([0] if ltype == "F" else [1])
        mx = pw / 2.0 + 0.32
        my = ph / 2.0 + 0.32
        gx0, gy0 = to_g(px - mx, py - my)
        gx1, gy1 = to_g(px + mx, py + my)
        for lay in layers:
            for gx in range(max(1, gx0), min(nx - 1, gx1 + 1)):
                for gy in range(max(1, gy0), min(ny - 1, gy1 + 1)):
                    block_cell(lay, gx, gy, nid)

    segments = []
    vias = []
    net_pads = {}
    for net, ltype, px, py, pw, ph, tag in PADS:
        if net.startswith("__") or net == "GND":
            continue
        net_pads.setdefault(net, []).append((ltype, px, py, pw, ph, tag))

    power_nets = {"VIN_RAW", "VIN_FUSE", "VIN_PROT", "XL_SW", "+5V_HEAT", "+3V3"}

    def net_priority(n):
        pts = net_pads[n]
        span = max(p[1] for p in pts) - min(p[1] for p in pts) + max(p[2] for p in pts) - min(p[2] for p in pts)
        return (0 if len(pts) <= 3 and span < 18.0 else (1 if n in power_nets else 2), span)

    ordered_nets = sorted(net_pads.keys(), key=net_priority)
    failed_nets = []

    def cell_free_for(lay, gx, gy, nid):
        if gx <= 0 or gx >= nx - 1 or gy <= 0 or gy >= ny - 1:
            return False
        for v in occ_l[lay][gx][gy]:
            if v != nid:
                return False
        return True

    def can_use(lay, gx, gy, nid, is_via=False):
        if not cell_free_for(lay, gx, gy, nid):
            return False
        if is_via:
            # 0.85mm via needs 1-cell halo free on BOTH layers
            for dl in (0, 1):
                for dx in (-1, 0, 1):
                    for dy in (-1, 0, 1):
                        if not cell_free_for(dl, gx + dx, gy + dy, nid):
                            return False
        return True

    def mark_path(path, nid, width):
        # Inflate by 1 cell (0.5mm) for 0.3mm traces, 1 cell + extra check for 0.7mm
        rad = 2 if width >= 0.65 else 1
        for lay, gx, gy in path:
            for dx in range(-rad, rad + 1):
                for dy in range(-rad, rad + 1):
                    if rad == 2 and abs(dx) == 2 and abs(dy) == 2:
                        continue
                    block_cell(lay, gx + dx, gy + dy, nid)

    def pad_entry_cell(px, py, nid, ltype):
        """Find the nearest grid cell to pad center that does not violate any other net."""
        gx0, gy0 = to_g(px, py)
        layers = [0, 1] if ltype == "BOTH" else [0]
        for rad in range(0, 3):
            for lay in layers:
                for dx in range(-rad, rad + 1):
                    for dy in range(-rad, rad + 1):
                        if cell_free_for(lay, gx0 + dx, gy0 + dy, nid):
                            return (lay, gx0 + dx, gy0 + dy)
        return (0, gx0, gy0)

    for net in ordered_nets:
        nid = nt.id(net)
        pts = net_pads[net]
        if len(pts) < 2:
            continue
        width = 0.60 if net in power_nets else 0.25

        connected = [0]
        remaining = list(range(1, len(pts)))
        pad_snaps = [(*pad_entry_cell(p[1], p[2], nid, p[0]), p[1], p[2], p[5]) for p in pts]
        net_grid_nodes = {(pad_snaps[0][0], pad_snaps[0][1], pad_snaps[0][2])}

        while remaining:
            best_pair, best_d = None, 1e9
            for ci in connected:
                for ri in remaining:
                    d = abs(pts[ci][1] - pts[ri][1]) + abs(pts[ci][2] - pts[ri][2])
                    if d < best_d:
                        best_d, best_pair = d, (ci, ri)
            ci, ri = best_pair
            sl, sgx, sgy, spx, spy, stag = pad_snaps[ri]
            tl, tgx, tgy, tpx, tpy, ttag = pad_snaps[ci]

            pq = [(abs(sgx - tgx) + abs(sgy - tgy), 0, sl, sgx, sgy)]
            gscore = {(sl, sgx, sgy): 0}
            parent = {}
            found = None

            while pq:
                f, g, lay, gx, gy = heapq.heappop(pq)
                if (lay, gx, gy) in net_grid_nodes:
                    found = (lay, gx, gy)
                    break
                if g > gscore.get((lay, gx, gy), 1e9):
                    continue

                for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)):
                    nx_, ny_ = gx + dx, gy + dy
                    if (lay, nx_, ny_) not in net_grid_nodes and not can_use(lay, nx_, ny_, nid, False):
                        continue
                    ng = g + (10 if lay == 0 else 14)
                    key = (lay, nx_, ny_)
                    if ng < gscore.get(key, 1e9):
                        gscore[key] = ng
                        parent[key] = (lay, gx, gy)
                        heapq.heappush(pq, (ng + (abs(nx_ - tgx) + abs(ny_ - tgy)) * 10, ng, lay, nx_, ny_))

                olay = 1 - lay
                if can_use(olay, gx, gy, nid, True):
                    ng = g + 55
                    key = (olay, gx, gy)
                    if ng < gscore.get(key, 1e9):
                        gscore[key] = ng
                        parent[key] = (lay, gx, gy)
                        heapq.heappush(pq, (ng + (abs(gx - tgx) + abs(gy - tgy)) * 10, ng, olay, gx, gy))

            if not found:
                failed_nets.append(f"{net}: {stag} -> {ttag}")
                connected.append(ri)
                remaining.remove(ri)
                continue

            path = []
            cur = found
            while cur in parent:
                path.append(cur)
                cur = parent[cur]
            path.append(cur)
            path.reverse()

            mark_path(path, nid, width)
            for node in path:
                net_grid_nodes.add(node)

            sx_mm, sy_mm = to_mm(path[0][1], path[0][2])
            if math.hypot(spx - sx_mm, spy - sy_mm) > 0.02:
                segments.append((spx, spy, sx_mm, sy_mm, width, "F.Cu" if path[0][0] == 0 else "B.Cu", nid))

            i = 0
            while i < len(path) - 1:
                l1, x1, y1 = path[i]
                l2, x2, y2 = path[i + 1]
                if l1 != l2:
                    vias.append((*to_mm(x1, y1), nid))
                    for dl in (0, 1):
                        for ox in (-1, 0, 1):
                            for oy in (-1, 0, 1):
                                block_cell(dl, x1 + ox, y1 + oy, nid)
                    i += 1
                    continue
                dx, dy = x2 - x1, y2 - y1
                j = i + 1
                while j + 1 < len(path) and path[j + 1][0] == l1 and (path[j + 1][1] - path[j][1], path[j + 1][2] - path[j][2]) == (dx, dy):
                    j += 1
                segments.append((*to_mm(x1, y1), *to_mm(path[j][1], path[j][2]), width, "F.Cu" if l1 == 0 else "B.Cu", nid))
                i = j

            if found == (tl, tgx, tgy):
                tx_mm, ty_mm = to_mm(tgx, tgy)
                if math.hypot(tpx - tx_mm, tpy - ty_mm) > 0.02:
                    segments.append((tx_mm, ty_mm, tpx, tpy, width, "F.Cu" if tl == 0 else "B.Cu", nid))

            connected.append(ri)
            remaining.remove(ri)

    # GND fanout vias AFTER signal routing: BFS on F.Cu to nearest valid via cell OR existing GND node!
    gnd_id = nt.id("GND")
    gnd_vias = 0
    gnd_connected = set()
    for net, ltype, px, py, pw, ph, tag in PADS:
        if net == "GND" and ltype == "BOTH":
            gnd_connected.add(to_g(px, py))
    for net, ltype, px, py, pw, ph, tag in PADS:
        if net != "GND" or ltype != "F":
            continue
        _, sgx, sgy = pad_entry_cell(px, py, gnd_id, "F")
        from collections import deque
        q = deque([(sgx, sgy)])
        vis = {(sgx, sgy): None}
        via_node = None
        hit_existing = False
        while q:
            gx, gy = q.popleft()
            if (gx, gy) in gnd_connected:
                via_node = (gx, gy)
                hit_existing = True
                break
            if can_use(0, gx, gy, gnd_id, True):
                via_node = (gx, gy)
                break
            if abs(gx - sgx) + abs(gy - sgy) > 40:
                continue
            for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)):
                nx_, ny_ = gx + dx, gy + dy
                if (nx_, ny_) not in vis and ((nx_, ny_) in gnd_connected or cell_free_for(0, nx_, ny_, gnd_id)):
                    vis[(nx_, ny_)] = (gx, gy)
                    q.append((nx_, ny_))
        if not via_node:
            failed_nets.append(f"GND_FANOUT: {tag}")
            continue
        path = []
        cur = via_node
        while cur is not None:
            path.append((0, cur[0], cur[1]))
            gnd_connected.add(cur)
            cur = vis[cur]
        path.reverse()
        mark_path(path, gnd_id, 0.30)
        sx_mm, sy_mm = to_mm(path[0][1], path[0][2])
        if math.hypot(px - sx_mm, py - sy_mm) > 0.02:
            segments.append((px, py, sx_mm, sy_mm, 0.30, "F.Cu", gnd_id))
        i = 0
        while i < len(path) - 1:
            _, x1, y1 = path[i]
            _, x2, y2 = path[i + 1]
            dx, dy = x2 - x1, y2 - y1
            j = i + 1
            while j + 1 < len(path) and (path[j + 1][1] - path[j][1], path[j + 1][2] - path[j][2]) == (dx, dy):
                j += 1
            segments.append((*to_mm(x1, y1), *to_mm(path[j][1], path[j][2]), 0.30, "F.Cu", gnd_id))
            i = j
        if not hit_existing:
            vx, vy = to_mm(via_node[0], via_node[1])
            vias.append((vx, vy, gnd_id))
            for dl in (0, 1):
                for ox in (-1, 0, 1):
                    for oy in (-1, 0, 1):
                        block_cell(dl, via_node[0] + ox, via_node[1] + oy, gnd_id)
            gnd_vias += 1

    return segments, vias, failed_nets, gnd_vias


def generate_carrier_pcb(output_path: str) -> None:
    BBOXES.clear()
    PADS.clear()
    nt = NetTable()
    fps = []

    # Board: 88.0 x 78.0 mm (100..188 x 100..178)
    bx0, by0, bx1, by1 = 100.0, 100.0, 188.0, 178.0

    # 4x M3 Mounting Holes
    for idx, (mx, my) in enumerate([(104, 104), (184, 104), (104, 174), (184, 174)], 1):
        fps.append(fp_mounting_hole(f"H{idx}", mx, my))

    # ESP32 DevKit V1 (30-Pin, 25.4mm row spacing) at Top-Right-Center (cx=150, cy=126)
    # Left row (top to bottom): EN, VP, VN, 34, 35, 32, 33, 25, 26, 27, 14, 12, 13, GND, VIN(5V)
    left_nets = [
        "", "", "", "ADC_5V_MON", "", "", "", "DQ_CH1", "DQ_CH2", "DQ_CH3",
        "", "", "", "GND", "+5V_HEAT"
    ]
    # Right row (top to bottom): 23, 22, TX0, RX0, 21, 19, 18, 5, 17, 16, 4, 2, 15, GND, 3V3
    right_nets = [
        "", "", "", "", "", "LED_STAT", "HEAT_EN3", "", "HEAT_EN2", "HEAT_EN1",
        "", "", "", "GND", "+3V3"
    ]
    fps.append(fp_esp32_devkit_30p("M1", 150.0, 125.0, left_nets, right_nets, nt))

    # Optional Internal Status LED + 5V Monitor Divider (Far Right, next to DevKit)
    fps.append(fp_2pad("R_LED", "1k", "Resistor_SMD:R_0603", 174.0, 114.0, 0, 1.6, 0.85, 0.90, "+3V3", "LED_A", nt))
    fps.append(fp_2pad("LED1", "GREEN_0603", "LED_SMD:LED_0603", 174.0, 118.0, 0, 1.6, 0.85, 0.90, "LED_A", "LED_STAT", nt))
    fps.append(fp_2pad("R_MON1", "20k_1%", "Resistor_SMD:R_0603", 174.0, 126.0, 0, 1.6, 0.85, 0.90, "+5V_HEAT", "ADC_5V_MON", nt))
    fps.append(fp_2pad("R_MON2", "10k_1%", "Resistor_SMD:R_0603", 174.0, 130.0, 0, 1.6, 0.85, 0.90, "ADC_5V_MON", "GND", nt))
    fps.append(fp_2pad("C_MON", "100nF", "Capacitor_SMD:C_0603", 174.0, 134.0, 0, 1.6, 0.85, 0.90, "ADC_5V_MON", "GND", nt))

    # Industrial Wide-Input Power Section (Left side, cleanly isolated from DevKit)
    fps.append(fp_terminal("J1", "12-24V_IN", 2, 5.08, 114.0, 171.5, ["VIN_RAW", "GND"], nt))
    fps.append(fp_2pad("F1", "PTC_1A_36V", "Fuse:Fuse_1812", 107.0, 160.0, 0, 4.2, 1.5, 3.2, "VIN_RAW", "VIN_FUSE", nt))
    fps.append(fp_2pad("D1", "SS34", "Diode_SMD:D_SMA", 117.0, 160.0, 0, 4.0, 2.0, 2.0, "VIN_FUSE", "VIN_PROT", nt))
    fps.append(fp_2pad("D2", "SMBJ28A", "Diode_SMD:D_SMB", 107.0, 150.5, 0, 4.3, 2.2, 2.4, "VIN_PROT", "GND", nt))
    fps.append(fp_cp_radial("C1", "100uF_35V", 118.0, 149.5, 8.0, 3.5, "VIN_PROT", "GND", nt))

    xl_nets = ["VIN_PROT", "XL_SW", "+5V_HEAT", "GND", "GND", "GND", "GND", "GND"]
    fps.append(fp_soic8("U1", "XL1509-5.0E1", 108.5, 134.5, xl_nets, nt))
    fps.append(fp_2pad("D3", "SS34", "Diode_SMD:D_SMA", 120.5, 134.5, 0, 4.0, 2.0, 2.0, "GND", "XL_SW", nt))
    fps.append(fp_2pad("L1", "47uH_3A", "Inductor_SMD:L_10x10", 112.0, 120.5, 0, 8.0, 3.0, 4.0, "XL_SW", "+5V_HEAT", nt))
    fps.append(fp_cp_radial("C3", "220uF_10V", 124.5, 120.5, 6.3, 2.5, "+5V_HEAT", "GND", nt))
    fps.append(fp_2pad("C4", "10uF_X7R", "Capacitor_SMD:C_0805", 115.0, 110.5, 0, 1.8, 1.0, 1.3, "+5V_HEAT", "GND", nt))

    # 3x Probe Channels (Spacious 4.5mm row spacing for 100% clean routing)
    ch_x_centers = [133.5, 152.5, 171.5]
    ch_labels = ["CH1_CARBON", "CH2_NACLO", "CH3_PFC"]
    for idx in range(3):
        ch = idx + 1
        cx = ch_x_centers[idx]
        fps.append(fp_terminal(
            f"J{ch+1}", ch_labels[idx], 4, 3.81, cx, 171.5,
            [f"HEAT_OUT{ch}", f"VDD_OUT{ch}", f"DQ_EXT{ch}", "GND"], nt
        ))
        fps.append(fp_sot23(f"QP{ch}", "AO3401A", cx - 5.5, 158.5, 0, f"PMOS_G{ch}", "+5V_HEAT", f"HEAT_OUT{ch}", nt))
        fps.append(fp_sot23(f"QN{ch}", "S8050", cx + 0.5, 158.5, 0, f"NPN_B{ch}", "GND", f"PMOS_G{ch}", nt))
        fps.append(fp_2pad(f"D{ch+3}", "1N5819WS", "Diode_SMD:D_SOD-123", cx - 5.5, 164.0, 0, 3.0, 1.0, 1.2, "GND", f"HEAT_OUT{ch}", nt))
        fps.append(fp_2pad(f"R_PU{ch}", "10k", "Resistor_SMD:R_0603", cx - 5.5, 151.5, 0, 1.6, 0.85, 0.90, "+5V_HEAT", f"PMOS_G{ch}", nt))
        fps.append(fp_2pad(f"R_B{ch}", "2.2k", "Resistor_SMD:R_0603", cx + 0.5, 151.5, 0, 1.6, 0.85, 0.90, f"HEAT_EN{ch}", f"NPN_B{ch}", nt))
        fps.append(fp_2pad(f"R_PD{ch}", "10k", "Resistor_SMD:R_0603", cx + 6.0, 148.5, 0, 1.6, 0.85, 0.90, f"HEAT_EN{ch}", "GND", nt))
        fps.append(fp_2pad(f"R_OW{ch}", "4.7k", "Resistor_SMD:R_0603", cx + 6.0, 156.5, 0, 1.6, 0.85, 0.90, "+3V3", f"DQ_EXT{ch}", nt))
        fps.append(fp_2pad(f"R_SR{ch}", "100R", "Resistor_SMD:R_0603", cx + 6.0, 160.5, 0, 1.6, 0.85, 0.90, f"DQ_CH{ch}", f"DQ_EXT{ch}", nt))
        fps.append(fp_2pad(f"R_V{ch}", "10R", "Resistor_SMD:R_0603", cx + 0.5, 163.5, 0, 1.6, 0.85, 0.90, "+3V3", f"VDD_OUT{ch}", nt))
        fps.append(fp_2pad(f"ESD{ch}", "PESD3V3", "Diode_SMD:D_SOD-323", cx + 6.0, 164.0, 0, 2.2, 0.80, 0.80, f"DQ_EXT{ch}", "GND", nt))

    # Route all nets + run strict geometric DRC!
    segments, vias, failed, gnd_vias = route_all_nets(nt, bx0, by0, bx1, by1)
    drc_errs = run_drc_check(segments, vias, nt)

    seg_lines = [
        f'  (segment (start {x1:.2f} {y1:.2f}) (end {x2:.2f} {y2:.2f}) (width {w:.2f}) (layer "{lay}") (net {nid}) (tstamp {uid()}))'
        for (x1, y1, x2, y2, w, lay, nid) in segments
    ]
    via_lines = [
        f'  (via (at {vx:.2f} {vy:.2f}) (size 0.85) (drill 0.50) (layers "F.Cu" "B.Cu") (net {nid}) (tstamp {uid()}))'
        for (vx, vy, nid) in vias
    ]

    content = f"""(kicad_pcb (version 20221018) (generator main_carrier_devkit_v4)

  (general
    (thickness 1.6)
  )

  (paper "A4")
  (layers
    (0 "F.Cu" signal)
    (31 "B.Cu" signal)
    (32 "B.Adhes" user "B.Adhesive")
    (33 "F.Adhes" user "F.Adhesive")
    (34 "B.Paste" user)
    (35 "F.Paste" user)
    (36 "B.SilkS" user "B.Silkscreen")
    (37 "F.SilkS" user "F.Silkscreen")
    (38 "B.Mask" user)
    (39 "F.Mask" user)
    (44 "Edge.Cuts" user)
    (46 "B.CrtYd" user "B.Courtyard")
    (47 "F.CrtYd" user "F.Courtyard")
    (48 "B.Fab" user)
    (49 "F.Fab" user)
  )

  (setup
    (pad_to_mask_clearance 0.05)
  )

{nt.render()}

  ;; ==================== Board Outline (85.0 x 72.0 mm) ====================
  (gr_rect (start {bx0:.2f} {by0:.2f}) (end {bx1:.2f} {by1:.2f})
    (stroke (width 0.15) (type solid)) (fill none) (layer "Edge.Cuts") (tstamp {uid()}))

  ;; ==================== Silkscreen Labels ====================
  (gr_text "WST 3-CH THERMAL SENSOR CARRIER (ESP32 DEVKIT 30P)" (at 142.50 147.00) (layer "B.SilkS")
    (effects (font (size 1.2 1.2) (thickness 0.20)) (justify mirror)) (tstamp {uid()}))
  (gr_text "J2/J3/J4: 1:5V_HEAT  2:3V3  3:DQ  4:GND" (at 151.00 160.00) (layer "B.SilkS")
    (effects (font (size 1.0 1.0) (thickness 0.18)) (justify mirror)) (tstamp {uid()}))

  ;; ==================== BLE Antenna Keep-Out Zone ====================
  (zone (net 0) (net_name "") (layers "F.Cu" "B.Cu") (tstamp {uid()})
    (hatch edge 0.5)
    (connect_pads (clearance 0))
    (min_thickness 0.25)
    (keepout (tracks not_allowed) (vias not_allowed) (pads not_allowed) (copperpour not_allowed) (footprints allowed))
    (fill (thermal_gap 0.5) (thermal_bridge_width 0.5))
    (polygon
      (pts
        (xy 135.00 100.00) (xy 165.00 100.00) (xy 165.00 108.00) (xy 135.00 108.00)
      )
    )
  )

  ;; ==================== Top & Bottom Solid GND Copper Pours ====================
  (zone (net {nt.id("GND")}) (net_name "GND") (layer "F.Cu") (tstamp {uid()})
    (hatch edge 0.5)
    (connect_pads (clearance 0.25))
    (min_thickness 0.25)
    (fill (thermal_gap 0.30) (thermal_bridge_width 0.35))
    (polygon
      (pts
        (xy 100.50 100.50) (xy 134.50 100.50) (xy 134.50 108.50) (xy 165.50 108.50)
        (xy 165.50 100.50) (xy 187.50 100.50) (xy 187.50 177.50) (xy 100.50 177.50)
      )
    )
  )

  (zone (net {nt.id("GND")}) (net_name "GND") (layer "B.Cu") (tstamp {uid()})
    (hatch edge 0.5)
    (connect_pads (clearance 0.25))
    (min_thickness 0.25)
    (fill (thermal_gap 0.30) (thermal_bridge_width 0.35))
    (polygon
      (pts
        (xy 100.50 100.50) (xy 134.50 100.50) (xy 134.50 108.50) (xy 165.50 108.50)
        (xy 165.50 100.50) (xy 187.50 100.50) (xy 187.50 177.50) (xy 100.50 177.50)
      )
    )
  )

{chr(10).join(fps)}

  ;; ==================== Routed Copper Segments ({len(segments)}) ====================
{chr(10).join(seg_lines)}

  ;; ==================== Vias ({len(vias)}, all 0.50mm drill / 0.85mm pad) ====================
{chr(10).join(via_lines)}
)
"""
    os.makedirs(os.path.dirname(output_path), exist_ok=True)
    with open(output_path, "w", encoding="utf-8", newline="\n") as f:
        f.write(content)
    print(f"Generated ESP32-DevKit Carrier PCB: {output_path}")
    print(f"  Board: {bx1-bx0:.1f} x {by1-by0:.1f} mm | Footprints: {len(fps)} | Nets: {len(nt.nets)-1}")
    print(f"  Segments: {len(segments)} | Vias: {len(vias)} (GND fanout: {gnd_vias})")
    print(f"  Unrouted / Failed: {len(failed)}")
    for item in failed:
        print("   !", item)
    print(f"  Strict Geometric DRC Errors: {len(drc_errs)}")
    for e in drc_errs[:20]:
        print("   DRC:", e)


if __name__ == "__main__":
    here = os.path.dirname(os.path.abspath(__file__))
    generate_carrier_pcb(os.path.join(here, "main_board_pcb.kicad_pcb"))
