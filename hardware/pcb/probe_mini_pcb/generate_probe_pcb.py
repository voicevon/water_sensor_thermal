"""
Generate a KiCad 7/8 (.kicad_pcb) file for the 5.0 x 15.5 mm Thermal Probe Mini-PCB
using a standard Narrow SOP-8 (SOIC-8) DS18B20Z rotated 90 degrees!

Directly importable into EasyEDA Pro (立创EDA专业版: 文件 -> 导入 -> KiCad) with 0 DRC errors
under EasyEDA Pro's default rules (via drill >= 0.50mm, edge-to-pad >= 0.30mm, pad-to-via >= 0.16mm).

Board: 5.00 mm (W) x 15.50 mm (H), 2-layer FR4, **0.8 mm thickness** (recommended for ID 6.0mm ceramic tube!).
Staggered placement: 1206 heaters on Top (y=104.8..109.2), SOP-8 18B20 on Bottom (y=108.5..115.0).
Max thickness at SOP-8 = 0.8 + 1.65 = 2.45 mm -> Diagonal = sqrt(5.0^2 + 2.45^2) = 5.57 mm < 6.00 mm ID!

Top (F.Cu):
  - J_H  : HEAT_5V wire pad (1.6 x 1.6 mm)
  - J_3V : VDD_3V3 wire pad (1.6 x 1.6 mm)
  - R_H1 : 33 Ohm 1206 heater resistor #1
  - R_H2 : 33 Ohm 1206 heater resistor #2 (parallel = 16.5 Ohm, ~1.5W @ 5.0V)

Bottom (B.Cu):
  - J_GND: GND wire pad (1.6 x 1.6 mm)
  - J_DQ : DQ 1-Wire pad (1.6 x 1.6 mm)
  - C1   : 100 nF 0603 X7R decoupling cap
  - U1   : Narrow SOP-8 DS18B20Z (universal pinout: Pin3=VDD, Pin4=DQ, Pin5=GND; Pins 1,2,6,7,8=NC)
"""

import math
import os
import uuid


def uid() -> str:
    return str(uuid.uuid4())


def generate_kicad_pcb(output_path: str) -> None:
    # Board origin (100, 100), W = 5.00 mm, H = 15.50 mm
    x0, y0 = 100.0, 100.0
    x1, y1 = 105.0, 115.5

    content = f"""(kicad_pcb (version 20221018) (generator probe_mini_pcb_sop8_v3)

  (general
    (thickness 0.8)
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

  (net 0 "")
  (net 1 "HEAT_5V")
  (net 2 "VDD_3V3")
  (net 3 "DQ")
  (net 4 "GND")

  ;; ==================== Board Outline (Edge.Cuts: 5.00 x 15.50 mm, 0.8mm thick) ====================
  (gr_rect (start {x0:.2f} {y0:.2f}) (end {x1:.2f} {y1:.2f})
    (stroke (width 0.10) (type solid)) (fill none) (layer "Edge.Cuts") (tstamp {uid()}))

  ;; ==================== Top Wire Solder Pads (F.Cu) ====================
  (footprint "Probe:WirePad_1.6x1.6" (layer "F.Cu")
    (tstamp {uid()})
    (at 101.25 101.25)
    (fp_text reference "J_H" (at 0 -1.25) (layer "F.SilkS") (effects (font (size 0.55 0.55) (thickness 0.12))) (tstamp {uid()}))
    (fp_text value "H+" (at 0 1.25) (layer "F.Fab") (effects (font (size 0.55 0.55) (thickness 0.12))) (tstamp {uid()}))
    (pad "1" smd rect (at 0 0) (size 1.60 1.60) (layers "F.Cu" "F.Paste" "F.Mask") (net 1 "HEAT_5V") (tstamp {uid()}))
  )

  (footprint "Probe:WirePad_1.6x1.6" (layer "F.Cu")
    (tstamp {uid()})
    (at 103.75 101.25)
    (fp_text reference "J_3V" (at 0 -1.25) (layer "F.SilkS") (effects (font (size 0.55 0.55) (thickness 0.12))) (tstamp {uid()}))
    (fp_text value "3V3" (at 0 1.25) (layer "F.Fab") (effects (font (size 0.55 0.55) (thickness 0.12))) (tstamp {uid()}))
    (pad "1" smd rect (at 0 0) (size 1.60 1.60) (layers "F.Cu" "F.Paste" "F.Mask") (net 2 "VDD_3V3") (tstamp {uid()}))
  )

  ;; ==================== Bottom Wire Solder Pads (B.Cu: GND left, DQ right) ====================
  (footprint "Probe:WirePad_1.6x1.6" (layer "B.Cu")
    (tstamp {uid()})
    (at 101.25 101.25)
    (fp_text reference "J_GND" (at 0 -1.25) (layer "B.SilkS") (effects (font (size 0.55 0.55) (thickness 0.12)) (justify mirror)) (tstamp {uid()}))
    (fp_text value "GND" (at 0 1.25) (layer "B.Fab") (effects (font (size 0.55 0.55) (thickness 0.12)) (justify mirror)) (tstamp {uid()}))
    (pad "1" smd rect (at 0 0) (size 1.60 1.60) (layers "B.Cu" "B.Paste" "B.Mask") (net 4 "GND") (tstamp {uid()}))
  )

  (footprint "Probe:WirePad_1.6x1.6" (layer "B.Cu")
    (tstamp {uid()})
    (at 103.75 101.25)
    (fp_text reference "J_DQ" (at 0 -1.25) (layer "B.SilkS") (effects (font (size 0.55 0.55) (thickness 0.12)) (justify mirror)) (tstamp {uid()}))
    (fp_text value "DQ" (at 0 1.25) (layer "B.Fab") (effects (font (size 0.55 0.55) (thickness 0.12)) (justify mirror)) (tstamp {uid()}))
    (pad "1" smd rect (at 0 0) (size 1.60 1.60) (layers "B.Cu" "B.Paste" "B.Mask") (net 3 "DQ") (tstamp {uid()}))
  )

  ;; ==================== Top Heater Resistors (2x 33R 1206 = 16.5R, F.Cu) ====================
  (footprint "Resistor_SMD:R_1206_3216Metric" (layer "F.Cu")
    (tstamp {uid()})
    (at 102.50 105.60)
    (fp_text reference "R_H1" (at 0 -1.20) (layer "F.SilkS") (effects (font (size 0.55 0.55) (thickness 0.12))) (tstamp {uid()}))
    (fp_text value "33R_0.5W" (at 0 1.20) (layer "F.Fab") (effects (font (size 0.50 0.50) (thickness 0.10))) (tstamp {uid()}))
    (fp_line (start -0.75 -0.85) (end 0.75 -0.85) (stroke (width 0.12) (type solid)) (layer "F.SilkS") (tstamp {uid()}))
    (fp_line (start -0.75 0.85) (end 0.75 0.85) (stroke (width 0.12) (type solid)) (layer "F.SilkS") (tstamp {uid()}))
    (pad "1" smd roundrect (at -1.45 0) (size 1.10 1.60) (layers "F.Cu" "F.Paste" "F.Mask") (roundrect_rratio 0.2) (net 1 "HEAT_5V") (tstamp {uid()}))
    (pad "2" smd roundrect (at 1.45 0) (size 1.10 1.60) (layers "F.Cu" "F.Paste" "F.Mask") (roundrect_rratio 0.2) (net 4 "GND") (tstamp {uid()}))
  )

  (footprint "Resistor_SMD:R_1206_3216Metric" (layer "F.Cu")
    (tstamp {uid()})
    (at 102.50 108.40)
    (fp_text reference "R_H2" (at 0 -1.20) (layer "F.SilkS") (effects (font (size 0.55 0.55) (thickness 0.12))) (tstamp {uid()}))
    (fp_text value "33R_0.5W" (at 0 1.20) (layer "F.Fab") (effects (font (size 0.50 0.50) (thickness 0.10))) (tstamp {uid()}))
    (fp_line (start -0.75 -0.85) (end 0.75 -0.85) (stroke (width 0.12) (type solid)) (layer "F.SilkS") (tstamp {uid()}))
    (fp_line (start -0.75 0.85) (end 0.75 0.85) (stroke (width 0.12) (type solid)) (layer "F.SilkS") (tstamp {uid()}))
    (pad "1" smd roundrect (at -1.45 0) (size 1.10 1.60) (layers "F.Cu" "F.Paste" "F.Mask") (roundrect_rratio 0.2) (net 1 "HEAT_5V") (tstamp {uid()}))
    (pad "2" smd roundrect (at 1.45 0) (size 1.10 1.60) (layers "F.Cu" "F.Paste" "F.Mask") (roundrect_rratio 0.2) (net 4 "GND") (tstamp {uid()}))
  )

  ;; ==================== Bottom Decoupling Cap (100nF 0603, B.Cu) ====================
  (footprint "Capacitor_SMD:C_0603_1608Metric" (layer "B.Cu")
    (tstamp {uid()})
    (at 102.30 105.00)
    (fp_text reference "C1" (at 0 -0.90) (layer "B.SilkS") (effects (font (size 0.55 0.55) (thickness 0.12)) (justify mirror)) (tstamp {uid()}))
    (fp_text value "100nF" (at 0 0.90) (layer "B.Fab") (effects (font (size 0.50 0.50) (thickness 0.10)) (justify mirror)) (tstamp {uid()}))
    (pad "1" smd roundrect (at -0.80 0) (size 0.85 0.90) (layers "B.Cu" "B.Paste" "B.Mask") (roundrect_rratio 0.2) (net 4 "GND") (tstamp {uid()}))
    (pad "2" smd roundrect (at 0.80 0) (size 0.85 0.90) (layers "B.Cu" "B.Paste" "B.Mask") (roundrect_rratio 0.2) (net 2 "VDD_3V3") (tstamp {uid()}))
  )

  ;; ==================== Bottom Narrow SOP-8 DS18B20Z (Rotated 90 deg, B.Cu) ====================
  ;; Universal DS18B20Z SOP-8 Pinout (CCW looking at Bottom):
  ;;   Top row (y=109.10): Pin1(NC), Pin2(NC), Pin3(VDD), Pin4(DQ)
  ;;   Bot row (y=114.40): Pin5(GND), Pin6(NC), Pin7(NC), Pin8(NC)
  (footprint "Package_SO:SOIC-8_3.9x4.9mm_P1.27mm" (layer "B.Cu")
    (tstamp {uid()})
    (at 102.50 111.75)
    (fp_text reference "U1" (at 0 0) (layer "B.SilkS") (effects (font (size 0.65 0.65) (thickness 0.12)) (justify mirror)) (tstamp {uid()}))
    (fp_text value "DS18B20Z_SOP8" (at 0 1.20) (layer "B.Fab") (effects (font (size 0.48 0.48) (thickness 0.10)) (justify mirror)) (tstamp {uid()}))
    (fp_rect (start -2.35 -1.90) (end 2.35 1.90) (stroke (width 0.12) (type solid)) (fill none) (layer "B.SilkS") (tstamp {uid()}))
    (fp_circle (center -1.85 -1.35) (end -1.65 -1.35) (stroke (width 0.18) (type solid)) (fill solid) (layer "B.SilkS") (tstamp {uid()}))
    (pad "1" smd roundrect (at -1.905 -2.65) (size 0.55 1.25) (layers "B.Cu" "B.Paste" "B.Mask") (roundrect_rratio 0.2) (net 0 "") (tstamp {uid()}))
    (pad "2" smd roundrect (at -0.635 -2.65) (size 0.55 1.25) (layers "B.Cu" "B.Paste" "B.Mask") (roundrect_rratio 0.2) (net 0 "") (tstamp {uid()}))
    (pad "3" smd roundrect (at  0.635 -2.65) (size 0.55 1.25) (layers "B.Cu" "B.Paste" "B.Mask") (roundrect_rratio 0.2) (net 2 "VDD_3V3") (tstamp {uid()}))
    (pad "4" smd roundrect (at  1.905 -2.65) (size 0.55 1.25) (layers "B.Cu" "B.Paste" "B.Mask") (roundrect_rratio 0.2) (net 3 "DQ") (tstamp {uid()}))
    (pad "5" smd roundrect (at  1.905  2.65) (size 0.55 1.25) (layers "B.Cu" "B.Paste" "B.Mask") (roundrect_rratio 0.2) (net 4 "GND") (tstamp {uid()}))
    (pad "6" smd roundrect (at  0.635  2.65) (size 0.55 1.25) (layers "B.Cu" "B.Paste" "B.Mask") (roundrect_rratio 0.2) (net 0 "") (tstamp {uid()}))
    (pad "7" smd roundrect (at -0.635  2.65) (size 0.55 1.25) (layers "B.Cu" "B.Paste" "B.Mask") (roundrect_rratio 0.2) (net 0 "") (tstamp {uid()}))
    (pad "8" smd roundrect (at -1.905  2.65) (size 0.55 1.25) (layers "B.Cu" "B.Paste" "B.Mask") (roundrect_rratio 0.2) (net 0 "") (tstamp {uid()}))
  )

  ;; ==================== Copper Routing (All Vias: 0.50mm drill / 0.85mm pad) ====================
  ;; Net 1: HEAT_5V (F.Cu, 0.45 mm)
  (segment (start 101.25 101.25) (end 100.60 102.50) (width 0.45) (layer "F.Cu") (net 1) (tstamp {uid()}))
  (segment (start 100.60 102.50) (end 100.60 104.70) (width 0.45) (layer "F.Cu") (net 1) (tstamp {uid()}))
  (segment (start 100.60 104.70) (end 101.05 105.60) (width 0.45) (layer "F.Cu") (net 1) (tstamp {uid()}))
  (segment (start 101.05 105.60) (end 101.05 108.40) (width 0.45) (layer "F.Cu") (net 1) (tstamp {uid()}))

  ;; Net 2: VDD_3V3 (F.Cu -> Via -> B.Cu -> C1.2 -> U1.3, 0.30 mm)
  (segment (start 103.75 101.25) (end 103.05 103.10) (width 0.30) (layer "F.Cu") (net 2) (tstamp {uid()}))
  (via (at 103.05 103.10) (size 0.85) (drill 0.50) (layers "F.Cu" "B.Cu") (net 2) (tstamp {uid()}))
  (segment (start 103.05 103.10) (end 103.10 105.00) (width 0.30) (layer "B.Cu") (net 2) (tstamp {uid()}))
  (segment (start 103.10 105.00) (end 103.135 109.10) (width 0.30) (layer "B.Cu") (net 2) (tstamp {uid()}))

  ;; Net 3: DQ (B.Cu: J_DQ -> right edge -> U1.4, 0.25 mm)
  (segment (start 103.75 101.25) (end 104.405 102.60) (width 0.25) (layer "B.Cu") (net 3) (tstamp {uid()}))
  (segment (start 104.405 102.60) (end 104.405 109.10) (width 0.25) (layer "B.Cu") (net 3) (tstamp {uid()}))

  ;; Net 4: GND (J_GND -> Via_GND_T -> F.Cu R_H1/R_H2 -> Via_GND_B -> B.Cu U1.5)
  (segment (start 101.25 101.25) (end 101.60 103.10) (width 0.45) (layer "B.Cu") (net 4) (tstamp {uid()}))
  (via (at 101.60 103.10) (size 0.85) (drill 0.50) (layers "F.Cu" "B.Cu") (net 4) (tstamp {uid()}))
  (segment (start 101.60 103.10) (end 101.50 105.00) (width 0.30) (layer "B.Cu") (net 4) (tstamp {uid()}))
  (segment (start 101.60 103.10) (end 101.60 104.25) (width 0.40) (layer "F.Cu") (net 4) (tstamp {uid()}))
  (segment (start 101.60 104.25) (end 103.95 104.25) (width 0.40) (layer "F.Cu") (net 4) (tstamp {uid()}))
  (segment (start 103.95 104.25) (end 103.95 105.60) (width 0.45) (layer "F.Cu") (net 4) (tstamp {uid()}))
  (segment (start 103.95 105.60) (end 103.95 108.40) (width 0.45) (layer "F.Cu") (net 4) (tstamp {uid()}))
  (segment (start 103.95 108.40) (end 104.05 111.75) (width 0.45) (layer "F.Cu") (net 4) (tstamp {uid()}))
  (via (at 104.05 111.75) (size 0.85) (drill 0.50) (layers "F.Cu" "B.Cu") (net 4) (tstamp {uid()}))
  (segment (start 104.05 111.75) (end 104.405 114.40) (width 0.40) (layer "B.Cu") (net 4) (tstamp {uid()}))

  ;; ==================== Isothermal GND Copper Zones ====================
  (zone (net 4) (net_name "GND") (layer "F.Cu") (tstamp {uid()})
    (hatch edge 0.5)
    (connect_pads (clearance 0.25))
    (min_thickness 0.2)
    (fill (thermal_gap 0.25) (thermal_bridge_width 0.3))
    (polygon
      (pts
        (xy 100.35 104.20) (xy 104.65 104.20) (xy 104.65 115.15) (xy 100.35 115.15)
      )
    )
  )

  (zone (net 4) (net_name "GND") (layer "B.Cu") (tstamp {uid()})
    (hatch edge 0.5)
    (connect_pads (clearance 0.25))
    (min_thickness 0.2)
    (fill (thermal_gap 0.25) (thermal_bridge_width 0.3))
    (polygon
      (pts
        (xy 100.35 104.20) (xy 104.65 104.20) (xy 104.65 115.15) (xy 100.35 115.15)
      )
    )
  )
)
"""
    os.makedirs(os.path.dirname(output_path), exist_ok=True)
    with open(output_path, "w", encoding="utf-8", newline="\n") as f:
        f.write(content)
    print(f"Generated SOP-8 v3: {output_path} ({len(content)} bytes)")


if __name__ == "__main__":
    here = os.path.dirname(os.path.abspath(__file__))
    generate_kicad_pcb(os.path.join(here, "probe_mini_pcb.kicad_pcb"))
