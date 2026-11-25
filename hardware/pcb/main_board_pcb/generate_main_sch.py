"""
Generate a complete, self-contained KiCad 7/8 Schematic (.kicad_sch)
for the WST 3-Channel Pulsed Thermal Liquid Level Sensor Main Carrier Board
(ESP32 DevKit V1 30-Pin, 88.0 x 78.0 mm).

100% synchronized with main_board_pcb.kicad_pcb (generate_main_pcb.py):
  - All 53 symbols (49 electrical components + 4 mounting holes) with exact
    Reference Designators, Values, Footprints, and Pin Numbers.
  - Self-contained embedded (lib_symbols ...) so it opens cleanly in KiCad 7/8
    and imports cleanly into EasyEDA Pro with zero missing libraries.
  - Built-in geometric netlist verifier that checks 100% electrical equivalence
    against main_board_pcb.kicad_pcb (all 31 nets, all pin-to-net bindings).
"""

import math
import os
import re
import uuid
from collections import defaultdict


def uid() -> str:
    return str(uuid.uuid4())


# ==================== Embedded KiCad Library Symbols ====================
# All symbol pins are placed on 2.54mm grid multiples so placing any symbol on
# a 2.54mm grid guarantees exact grid-aligned wire connections.
LIB_SYMBOLS = r'''
    (symbol "Custom:R" (pin_numbers hide) (pin_names (offset 0)) (in_bom yes) (on_board yes)
      (property "Reference" "R" (at 2.032 0 90) (effects (font (size 1.27 1.27))))
      (property "Value" "R" (at -2.032 0 90) (effects (font (size 1.27 1.27))))
      (property "Footprint" "" (at 0 0 0) (effects (font (size 1.27 1.27)) hide))
      (property "Datasheet" "~" (at 0 0 0) (effects (font (size 1.27 1.27)) hide))
      (symbol "R_0_1"
        (rectangle (start -1.016 -2.54) (end 1.016 2.54) (stroke (width 0.254) (type default)) (fill (type none)))
      )
      (symbol "R_1_1"
        (pin passive line (at 0 5.08 270) (length 2.54) (name "~" (effects (font (size 1.27 1.27)))) (number "1" (effects (font (size 1.27 1.27)))))
        (pin passive line (at 0 -5.08 90) (length 2.54) (name "~" (effects (font (size 1.27 1.27)))) (number "2" (effects (font (size 1.27 1.27)))))
      )
    )
    (symbol "Custom:C" (pin_numbers hide) (pin_names (offset 0.254)) (in_bom yes) (on_board yes)
      (property "Reference" "C" (at 2.54 1.27 0) (effects (font (size 1.27 1.27)) (justify left)))
      (property "Value" "C" (at 2.54 -1.27 0) (effects (font (size 1.27 1.27)) (justify left)))
      (property "Footprint" "" (at 0 0 0) (effects (font (size 1.27 1.27)) hide))
      (property "Datasheet" "~" (at 0 0 0) (effects (font (size 1.27 1.27)) hide))
      (symbol "C_0_1"
        (polyline (pts (xy -2.032 0.762) (xy 2.032 0.762)) (stroke (width 0.508) (type default)) (fill (type none)))
        (polyline (pts (xy -2.032 -0.762) (xy 2.032 -0.762)) (stroke (width 0.508) (type default)) (fill (type none)))
      )
      (symbol "C_1_1"
        (pin passive line (at 0 5.08 270) (length 4.318) (name "~" (effects (font (size 1.27 1.27)))) (number "1" (effects (font (size 1.27 1.27)))))
        (pin passive line (at 0 -5.08 90) (length 4.318) (name "~" (effects (font (size 1.27 1.27)))) (number "2" (effects (font (size 1.27 1.27)))))
      )
    )
    (symbol "Custom:CP" (pin_numbers hide) (pin_names (offset 0.254)) (in_bom yes) (on_board yes)
      (property "Reference" "C" (at 2.54 1.27 0) (effects (font (size 1.27 1.27)) (justify left)))
      (property "Value" "CP" (at 2.54 -1.27 0) (effects (font (size 1.27 1.27)) (justify left)))
      (property "Footprint" "" (at 0 0 0) (effects (font (size 1.27 1.27)) hide))
      (property "Datasheet" "~" (at 0 0 0) (effects (font (size 1.27 1.27)) hide))
      (symbol "CP_0_1"
        (rectangle (start -2.032 0.762) (end 2.032 1.27) (stroke (width 0.254) (type default)) (fill (type none)))
        (rectangle (start -2.032 -1.27) (end 2.032 -0.762) (stroke (width 0.254) (type default)) (fill (type outline)))
        (polyline (pts (xy -1.524 2.54) (xy -0.508 2.54)) (stroke (width 0.254) (type default)) (fill (type none)))
        (polyline (pts (xy -1.016 2.032) (xy -1.016 3.048)) (stroke (width 0.254) (type default)) (fill (type none)))
      )
      (symbol "CP_1_1"
        (pin passive line (at 0 5.08 270) (length 3.81) (name "+" (effects (font (size 1.0 1.0)))) (number "1" (effects (font (size 1.27 1.27)))))
        (pin passive line (at 0 -5.08 90) (length 3.81) (name "-" (effects (font (size 1.0 1.0)))) (number "2" (effects (font (size 1.27 1.27)))))
      )
    )
    (symbol "Custom:L" (pin_numbers hide) (pin_names (offset 1.016)) (in_bom yes) (on_board yes)
      (property "Reference" "L" (at 0 2.54 0) (effects (font (size 1.27 1.27))))
      (property "Value" "L" (at 0 -2.54 0) (effects (font (size 1.27 1.27))))
      (property "Footprint" "" (at 0 0 0) (effects (font (size 1.27 1.27)) hide))
      (property "Datasheet" "~" (at 0 0 0) (effects (font (size 1.27 1.27)) hide))
      (symbol "L_0_1"
        (arc (start -2.54 0) (mid -1.905 0.635) (end -1.27 0) (stroke (width 0.254) (type default)) (fill (type none)))
        (arc (start -1.27 0) (mid -0.635 0.635) (end 0 0) (stroke (width 0.254) (type default)) (fill (type none)))
        (arc (start 0 0) (mid 0.635 0.635) (end 1.27 0) (stroke (width 0.254) (type default)) (fill (type none)))
        (arc (start 1.27 0) (mid 1.905 0.635) (end 2.54 0) (stroke (width 0.254) (type default)) (fill (type none)))
      )
      (symbol "L_1_1"
        (pin passive line (at -5.08 0 0) (length 2.54) (name "~" (effects (font (size 1.27 1.27)))) (number "1" (effects (font (size 1.27 1.27)))))
        (pin passive line (at 5.08 0 180) (length 2.54) (name "~" (effects (font (size 1.27 1.27)))) (number "2" (effects (font (size 1.27 1.27)))))
      )
    )
    (symbol "Custom:Fuse_PTC" (pin_numbers hide) (pin_names (offset 0)) (in_bom yes) (on_board yes)
      (property "Reference" "F" (at 0 2.54 0) (effects (font (size 1.27 1.27))))
      (property "Value" "PTC" (at 0 -2.54 0) (effects (font (size 1.27 1.27))))
      (property "Footprint" "" (at 0 0 0) (effects (font (size 1.27 1.27)) hide))
      (property "Datasheet" "~" (at 0 0 0) (effects (font (size 1.27 1.27)) hide))
      (symbol "Fuse_PTC_0_1"
        (rectangle (start -2.54 -1.016) (end 2.54 1.016) (stroke (width 0.254) (type default)) (fill (type none)))
        (polyline (pts (xy -2.032 -1.524) (xy -1.27 -1.524) (xy 1.27 1.524) (xy 2.032 1.524)) (stroke (width 0.254) (type default)) (fill (type none)))
      )
      (symbol "Fuse_PTC_1_1"
        (pin passive line (at -5.08 0 0) (length 2.54) (name "~" (effects (font (size 1.27 1.27)))) (number "1" (effects (font (size 1.27 1.27)))))
        (pin passive line (at 5.08 0 180) (length 2.54) (name "~" (effects (font (size 1.27 1.27)))) (number "2" (effects (font (size 1.27 1.27)))))
      )
    )
    (symbol "Custom:D_Schottky_AK" (pin_numbers hide) (pin_names (offset 1.016) hide) (in_bom yes) (on_board yes)
      (property "Reference" "D" (at 0 2.54 0) (effects (font (size 1.27 1.27))))
      (property "Value" "D_Schottky" (at 0 -2.54 0) (effects (font (size 1.27 1.27))))
      (property "Footprint" "" (at 0 0 0) (effects (font (size 1.27 1.27)) hide))
      (property "Datasheet" "~" (at 0 0 0) (effects (font (size 1.27 1.27)) hide))
      (symbol "D_Schottky_AK_0_1"
        (polyline (pts (xy -1.27 -1.27) (xy -1.27 1.27) (xy 1.27 0) (xy -1.27 -1.27)) (stroke (width 0.254) (type default)) (fill (type none)))
        (polyline (pts (xy 0.762 1.27) (xy 1.27 1.27) (xy 1.27 -1.27) (xy 1.778 -1.27)) (stroke (width 0.254) (type default)) (fill (type none)))
      )
      (symbol "D_Schottky_AK_1_1"
        (pin passive line (at -5.08 0 0) (length 3.81) (name "A" (effects (font (size 1.27 1.27)))) (number "1" (effects (font (size 1.27 1.27)))))
        (pin passive line (at 5.08 0 180) (length 3.81) (name "K" (effects (font (size 1.27 1.27)))) (number "2" (effects (font (size 1.27 1.27)))))
      )
    )
    (symbol "Custom:D_TVS_KA" (pin_numbers hide) (pin_names (offset 1.016) hide) (in_bom yes) (on_board yes)
      (property "Reference" "D" (at 2.54 1.27 0) (effects (font (size 1.27 1.27)) (justify left)))
      (property "Value" "TVS" (at 2.54 -1.27 0) (effects (font (size 1.27 1.27)) (justify left)))
      (property "Footprint" "" (at 0 0 0) (effects (font (size 1.27 1.27)) hide))
      (property "Datasheet" "~" (at 0 0 0) (effects (font (size 1.27 1.27)) hide))
      (symbol "D_TVS_KA_0_1"
        (polyline (pts (xy -1.27 -1.27) (xy 1.27 -1.27) (xy 0 1.27) (xy -1.27 -1.27)) (stroke (width 0.254) (type default)) (fill (type none)))
        (polyline (pts (xy -1.778 0.762) (xy -1.27 1.27) (xy 1.27 1.27) (xy 1.778 1.778)) (stroke (width 0.254) (type default)) (fill (type none)))
      )
      (symbol "D_TVS_KA_1_1"
        (pin passive line (at 0 5.08 270) (length 3.81) (name "K" (effects (font (size 1.27 1.27)))) (number "1" (effects (font (size 1.27 1.27)))))
        (pin passive line (at 0 -5.08 90) (length 3.81) (name "A" (effects (font (size 1.27 1.27)))) (number "2" (effects (font (size 1.27 1.27)))))
      )
    )
    (symbol "Custom:LED_AK" (pin_numbers hide) (pin_names (offset 1.016) hide) (in_bom yes) (on_board yes)
      (property "Reference" "LED" (at 2.54 1.27 0) (effects (font (size 1.27 1.27)) (justify left)))
      (property "Value" "LED" (at 2.54 -1.27 0) (effects (font (size 1.27 1.27)) (justify left)))
      (property "Footprint" "" (at 0 0 0) (effects (font (size 1.27 1.27)) hide))
      (property "Datasheet" "~" (at 0 0 0) (effects (font (size 1.27 1.27)) hide))
      (symbol "LED_AK_0_1"
        (polyline (pts (xy -1.27 1.27) (xy 1.27 1.27) (xy 0 -1.27) (xy -1.27 1.27)) (stroke (width 0.254) (type default)) (fill (type none)))
        (polyline (pts (xy -1.27 -1.27) (xy 1.27 -1.27)) (stroke (width 0.254) (type default)) (fill (type none)))
        (polyline (pts (xy 1.524 0.254) (xy 2.794 -1.016)) (stroke (width 0.2032) (type default)) (fill (type none)))
        (polyline (pts (xy 1.524 -0.762) (xy 2.794 -2.032)) (stroke (width 0.2032) (type default)) (fill (type none)))
      )
      (symbol "LED_AK_1_1"
        (pin passive line (at 0 5.08 270) (length 3.81) (name "A" (effects (font (size 1.27 1.27)))) (number "1" (effects (font (size 1.27 1.27)))))
        (pin passive line (at 0 -5.08 90) (length 3.81) (name "K" (effects (font (size 1.27 1.27)))) (number "2" (effects (font (size 1.27 1.27)))))
      )
    )
    (symbol "Custom:Q_PMOS_GSD" (pin_names (offset 0.508) hide) (in_bom yes) (on_board yes)
      (property "Reference" "QP" (at 3.81 2.54 0) (effects (font (size 1.27 1.27)) (justify left)))
      (property "Value" "AO3401A" (at 3.81 0 0) (effects (font (size 1.27 1.27)) (justify left)))
      (property "Footprint" "" (at 0 0 0) (effects (font (size 1.27 1.27)) hide))
      (property "Datasheet" "~" (at 0 0 0) (effects (font (size 1.27 1.27)) hide))
      (symbol "Q_PMOS_GSD_0_1"
        (circle (center 0.508 0) (radius 2.794) (stroke (width 0.254) (type default)) (fill (type none)))
        (polyline (pts (xy -1.016 -1.524) (xy -1.016 1.524)) (stroke (width 0.254) (type default)) (fill (type none)))
        (polyline (pts (xy -0.254 -1.778) (xy -0.254 1.778)) (stroke (width 0.254) (type default)) (fill (type none)))
        (polyline (pts (xy -0.254 1.27) (xy 2.54 1.27) (xy 2.54 2.54)) (stroke (width 0.254) (type default)) (fill (type none)))
        (polyline (pts (xy -0.254 -1.27) (xy 2.54 -1.27) (xy 2.54 -2.54)) (stroke (width 0.254) (type default)) (fill (type none)))
        (polyline (pts (xy -0.254 0) (xy 2.54 0) (xy 2.54 -1.27)) (stroke (width 0.254) (type default)) (fill (type none)))
        (polyline (pts (xy 0.508 0.508) (xy 1.524 0) (xy 0.508 -0.508)) (stroke (width 0.254) (type default)) (fill (type outline)))
        (polyline (pts (xy -2.54 -1.27) (xy -1.016 -1.27)) (stroke (width 0.254) (type default)) (fill (type none)))
      )
      (symbol "Q_PMOS_GSD_1_1"
        (pin input line (at -5.08 0 0) (length 4.064) (name "G" (effects (font (size 1.0 1.0)))) (number "1" (effects (font (size 1.0 1.0)))))
        (pin passive line (at 2.54 5.08 270) (length 2.54) (name "S" (effects (font (size 1.0 1.0)))) (number "2" (effects (font (size 1.0 1.0)))))
        (pin passive line (at 2.54 -5.08 90) (length 2.54) (name "D" (effects (font (size 1.0 1.0)))) (number "3" (effects (font (size 1.0 1.0)))))
      )
    )
    (symbol "Custom:Q_NPN_BEC" (pin_names (offset 0.508) hide) (in_bom yes) (on_board yes)
      (property "Reference" "QN" (at 3.81 2.54 0) (effects (font (size 1.27 1.27)) (justify left)))
      (property "Value" "S8050" (at 3.81 0 0) (effects (font (size 1.27 1.27)) (justify left)))
      (property "Footprint" "" (at 0 0 0) (effects (font (size 1.27 1.27)) hide))
      (property "Datasheet" "~" (at 0 0 0) (effects (font (size 1.27 1.27)) hide))
      (symbol "Q_NPN_BEC_0_1"
        (circle (center 0.508 0) (radius 2.794) (stroke (width 0.254) (type default)) (fill (type none)))
        (polyline (pts (xy -0.762 -1.778) (xy -0.762 1.778)) (stroke (width 0.508) (type default)) (fill (type none)))
        (polyline (pts (xy -0.762 0.762) (xy 2.54 2.54)) (stroke (width 0.254) (type default)) (fill (type none)))
        (polyline (pts (xy -0.762 -0.762) (xy 2.54 -2.54)) (stroke (width 0.254) (type default)) (fill (type none)))
        (polyline (pts (xy 0.762 -1.905) (xy 1.778 -2.159) (xy 1.27 -1.143)) (stroke (width 0.254) (type default)) (fill (type outline)))
      )
      (symbol "Q_NPN_BEC_1_1"
        (pin input line (at -5.08 0 0) (length 4.318) (name "B" (effects (font (size 1.0 1.0)))) (number "1" (effects (font (size 1.0 1.0)))))
        (pin passive line (at 2.54 -5.08 90) (length 2.54) (name "E" (effects (font (size 1.0 1.0)))) (number "2" (effects (font (size 1.0 1.0)))))
        (pin passive line (at 2.54 5.08 270) (length 2.54) (name "C" (effects (font (size 1.0 1.0)))) (number "3" (effects (font (size 1.0 1.0)))))
      )
    )
    (symbol "Custom:XL1509-5.0E1" (pin_names (offset 1.016)) (in_bom yes) (on_board yes)
      (property "Reference" "U" (at 0 8.89 0) (effects (font (size 1.27 1.27))))
      (property "Value" "XL1509-5.0E1" (at 0 -8.89 0) (effects (font (size 1.27 1.27))))
      (property "Footprint" "" (at 0 0 0) (effects (font (size 1.27 1.27)) hide))
      (property "Datasheet" "~" (at 0 0 0) (effects (font (size 1.27 1.27)) hide))
      (symbol "XL1509-5.0E1_0_1"
        (rectangle (start -7.62 -7.62) (end 7.62 7.62) (stroke (width 0.254) (type default)) (fill (type background)))
      )
      (symbol "XL1509-5.0E1_1_1"
        (pin power_in line (at -12.7 5.08 0) (length 5.08) (name "VIN" (effects (font (size 1.27 1.27)))) (number "1" (effects (font (size 1.27 1.27)))))
        (pin output line (at 12.7 5.08 180) (length 5.08) (name "SW" (effects (font (size 1.27 1.27)))) (number "2" (effects (font (size 1.27 1.27)))))
        (pin input line (at 12.7 0 180) (length 5.08) (name "FB" (effects (font (size 1.27 1.27)))) (number "3" (effects (font (size 1.27 1.27)))))
        (pin input line (at -12.7 -2.54 0) (length 5.08) (name "ON/OFF" (effects (font (size 1.27 1.27)))) (number "4" (effects (font (size 1.27 1.27)))))
        (pin power_in line (at -5.08 -12.7 90) (length 5.08) (name "GND" (effects (font (size 1.0 1.0)))) (number "5" (effects (font (size 1.0 1.0)))))
        (pin power_in line (at -2.54 -12.7 90) (length 5.08) (name "GND" (effects (font (size 1.0 1.0)))) (number "6" (effects (font (size 1.0 1.0)))))
        (pin power_in line (at 2.54 -12.7 90) (length 5.08) (name "GND" (effects (font (size 1.0 1.0)))) (number "7" (effects (font (size 1.0 1.0)))))
        (pin power_in line (at 5.08 -12.7 90) (length 5.08) (name "GND" (effects (font (size 1.0 1.0)))) (number "8" (effects (font (size 1.0 1.0)))))
      )
    )
    (symbol "Custom:Conn_01x02" (pin_names (offset 1.016)) (in_bom yes) (on_board yes)
      (property "Reference" "J" (at 0 5.08 0) (effects (font (size 1.27 1.27))))
      (property "Value" "Conn_01x02" (at 0 -5.08 0) (effects (font (size 1.27 1.27))))
      (property "Footprint" "" (at 0 0 0) (effects (font (size 1.27 1.27)) hide))
      (property "Datasheet" "~" (at 0 0 0) (effects (font (size 1.27 1.27)) hide))
      (symbol "Conn_01x02_0_1"
        (rectangle (start -3.81 -3.81) (end 2.54 3.81) (stroke (width 0.254) (type default)) (fill (type background)))
      )
      (symbol "Conn_01x02_1_1"
        (pin passive line (at 7.62 2.54 180) (length 5.08) (name "VIN+" (effects (font (size 1.27 1.27)))) (number "1" (effects (font (size 1.27 1.27)))))
        (pin passive line (at 7.62 -2.54 180) (length 5.08) (name "GND" (effects (font (size 1.27 1.27)))) (number "2" (effects (font (size 1.27 1.27)))))
      )
    )
    (symbol "Custom:Conn_01x04" (pin_names (offset 1.016)) (in_bom yes) (on_board yes)
      (property "Reference" "J" (at 0 7.62 0) (effects (font (size 1.27 1.27))))
      (property "Value" "Conn_01x04" (at 0 -7.62 0) (effects (font (size 1.27 1.27))))
      (property "Footprint" "" (at 0 0 0) (effects (font (size 1.27 1.27)) hide))
      (property "Datasheet" "~" (at 0 0 0) (effects (font (size 1.27 1.27)) hide))
      (symbol "Conn_01x04_0_1"
        (rectangle (start -3.81 -6.35) (end 5.08 6.35) (stroke (width 0.254) (type default)) (fill (type background)))
      )
      (symbol "Conn_01x04_1_1"
        (pin passive line (at -7.62 5.08 0) (length 3.81) (name "5V_H" (effects (font (size 1.1 1.1)))) (number "1" (effects (font (size 1.1 1.1)))))
        (pin passive line (at -7.62 2.54 0) (length 3.81) (name "3V3" (effects (font (size 1.1 1.1)))) (number "2" (effects (font (size 1.1 1.1)))))
        (pin passive line (at -7.62 -2.54 0) (length 3.81) (name "DQ" (effects (font (size 1.1 1.1)))) (number "3" (effects (font (size 1.1 1.1)))))
        (pin passive line (at -7.62 -5.08 0) (length 3.81) (name "GND" (effects (font (size 1.1 1.1)))) (number "4" (effects (font (size 1.1 1.1)))))
      )
    )
    (symbol "Custom:MountingHole" (pin_numbers hide) (pin_names (offset 1.016)) (in_bom no) (on_board yes)
      (property "Reference" "H" (at 0 3.81 0) (effects (font (size 1.27 1.27))))
      (property "Value" "M3" (at 0 -3.81 0) (effects (font (size 1.27 1.27))))
      (property "Footprint" "" (at 0 0 0) (effects (font (size 1.27 1.27)) hide))
      (property "Datasheet" "~" (at 0 0 0) (effects (font (size 1.27 1.27)) hide))
      (symbol "MountingHole_0_1"
        (circle (center 0 0) (radius 1.778) (stroke (width 0.254) (type default)) (fill (type none)))
        (circle (center 0 0) (radius 1.016) (stroke (width 0.254) (type default)) (fill (type none)))
      )
    )
    (symbol "Custom:ESP32_DevKit_30P" (pin_names (offset 1.016)) (in_bom yes) (on_board yes)
      (property "Reference" "M" (at 0 22.86 0) (effects (font (size 1.524 1.524))))
      (property "Value" "ESP32_DEVKIT_30P" (at 0 -22.86 0) (effects (font (size 1.524 1.524))))
      (property "Footprint" "" (at 0 0 0) (effects (font (size 1.27 1.27)) hide))
      (property "Datasheet" "~" (at 0 0 0) (effects (font (size 1.27 1.27)) hide))
      (symbol "ESP32_DevKit_30P_0_1"
        (rectangle (start -12.7 -20.32) (end 12.7 20.32) (stroke (width 0.254) (type default)) (fill (type background)))
      )
      (symbol "ESP32_DevKit_30P_1_1"
        (pin input line (at -17.78 17.78 0) (length 5.08) (name "EN" (effects (font (size 1.1 1.1)))) (number "L1" (effects (font (size 1.1 1.1)))))
        (pin input line (at -17.78 15.24 0) (length 5.08) (name "VP/IO36" (effects (font (size 1.1 1.1)))) (number "L2" (effects (font (size 1.1 1.1)))))
        (pin input line (at -17.78 12.70 0) (length 5.08) (name "VN/IO39" (effects (font (size 1.1 1.1)))) (number "L3" (effects (font (size 1.1 1.1)))))
        (pin input line (at -17.78 10.16 0) (length 5.08) (name "IO34/ADC" (effects (font (size 1.1 1.1)))) (number "L4" (effects (font (size 1.1 1.1)))))
        (pin input line (at -17.78 7.62 0) (length 5.08) (name "IO35" (effects (font (size 1.1 1.1)))) (number "L5" (effects (font (size 1.1 1.1)))))
        (pin bidirectional line (at -17.78 5.08 0) (length 5.08) (name "IO32" (effects (font (size 1.1 1.1)))) (number "L6" (effects (font (size 1.1 1.1)))))
        (pin bidirectional line (at -17.78 2.54 0) (length 5.08) (name "IO33" (effects (font (size 1.1 1.1)))) (number "L7" (effects (font (size 1.1 1.1)))))
        (pin bidirectional line (at -17.78 0.00 0) (length 5.08) (name "IO25" (effects (font (size 1.1 1.1)))) (number "L8" (effects (font (size 1.1 1.1)))))
        (pin bidirectional line (at -17.78 -2.54 0) (length 5.08) (name "IO26" (effects (font (size 1.1 1.1)))) (number "L9" (effects (font (size 1.1 1.1)))))
        (pin bidirectional line (at -17.78 -5.08 0) (length 5.08) (name "IO27" (effects (font (size 1.1 1.1)))) (number "L10" (effects (font (size 1.1 1.1)))))
        (pin bidirectional line (at -17.78 -7.62 0) (length 5.08) (name "IO14" (effects (font (size 1.1 1.1)))) (number "L11" (effects (font (size 1.1 1.1)))))
        (pin bidirectional line (at -17.78 -10.16 0) (length 5.08) (name "IO12" (effects (font (size 1.1 1.1)))) (number "L12" (effects (font (size 1.1 1.1)))))
        (pin bidirectional line (at -17.78 -12.70 0) (length 5.08) (name "IO13" (effects (font (size 1.1 1.1)))) (number "L13" (effects (font (size 1.1 1.1)))))
        (pin power_in line (at -17.78 -15.24 0) (length 5.08) (name "GND" (effects (font (size 1.1 1.1)))) (number "L14" (effects (font (size 1.1 1.1)))))
        (pin power_in line (at -17.78 -17.78 0) (length 5.08) (name "VIN(5V)" (effects (font (size 1.1 1.1)))) (number "L15" (effects (font (size 1.1 1.1)))))

        (pin bidirectional line (at 17.78 17.78 180) (length 5.08) (name "IO23" (effects (font (size 1.1 1.1)))) (number "R1" (effects (font (size 1.1 1.1)))))
        (pin bidirectional line (at 17.78 15.24 180) (length 5.08) (name "IO22" (effects (font (size 1.1 1.1)))) (number "R2" (effects (font (size 1.1 1.1)))))
        (pin bidirectional line (at 17.78 12.70 180) (length 5.08) (name "TX0" (effects (font (size 1.1 1.1)))) (number "R3" (effects (font (size 1.1 1.1)))))
        (pin bidirectional line (at 17.78 10.16 180) (length 5.08) (name "RX0" (effects (font (size 1.1 1.1)))) (number "R4" (effects (font (size 1.1 1.1)))))
        (pin bidirectional line (at 17.78 7.62 180) (length 5.08) (name "IO21" (effects (font (size 1.1 1.1)))) (number "R5" (effects (font (size 1.1 1.1)))))
        (pin bidirectional line (at 17.78 5.08 180) (length 5.08) (name "IO19" (effects (font (size 1.1 1.1)))) (number "R6" (effects (font (size 1.1 1.1)))))
        (pin bidirectional line (at 17.78 2.54 180) (length 5.08) (name "IO18" (effects (font (size 1.1 1.1)))) (number "R7" (effects (font (size 1.1 1.1)))))
        (pin bidirectional line (at 17.78 0.00 180) (length 5.08) (name "IO5" (effects (font (size 1.1 1.1)))) (number "R8" (effects (font (size 1.1 1.1)))))
        (pin bidirectional line (at 17.78 -2.54 180) (length 5.08) (name "IO17" (effects (font (size 1.1 1.1)))) (number "R9" (effects (font (size 1.1 1.1)))))
        (pin bidirectional line (at 17.78 -5.08 180) (length 5.08) (name "IO16" (effects (font (size 1.1 1.1)))) (number "R10" (effects (font (size 1.1 1.1)))))
        (pin bidirectional line (at 17.78 -7.62 180) (length 5.08) (name "IO4" (effects (font (size 1.1 1.1)))) (number "R11" (effects (font (size 1.1 1.1)))))
        (pin bidirectional line (at 17.78 -10.16 180) (length 5.08) (name "IO2" (effects (font (size 1.1 1.1)))) (number "R12" (effects (font (size 1.1 1.1)))))
        (pin bidirectional line (at 17.78 -12.70 180) (length 5.08) (name "IO15" (effects (font (size 1.1 1.1)))) (number "R13" (effects (font (size 1.1 1.1)))))
        (pin power_in line (at 17.78 -15.24 180) (length 5.08) (name "GND" (effects (font (size 1.1 1.1)))) (number "R14" (effects (font (size 1.1 1.1)))))
        (pin power_out line (at 17.78 -17.78 180) (length 5.08) (name "3V3" (effects (font (size 1.1 1.1)))) (number "R15" (effects (font (size 1.1 1.1)))))
      )
    )
    (symbol "power:GND" (power) (pin_names (offset 0)) (in_bom yes) (on_board yes)
      (property "Reference" "#PWR" (at 0 -3.81 0) (effects (font (size 1.27 1.27)) hide))
      (property "Value" "GND" (at 0 -3.81 0) (effects (font (size 1.27 1.27))))
      (property "Footprint" "" (at 0 0 0) (effects (font (size 1.27 1.27)) hide))
      (property "Datasheet" "" (at 0 0 0) (effects (font (size 1.27 1.27)) hide))
      (symbol "GND_0_1"
        (polyline (pts (xy 0 0) (xy 0 -1.27) (xy 1.27 -1.27) (xy 0 -2.54) (xy -1.27 -1.27) (xy 0 -1.27)) (stroke (width 0) (type default)) (fill (type none)))
      )
      (symbol "GND_1_1"
        (pin power_in line (at 0 0 270) (length 0) hide (name "GND" (effects (font (size 1.27 1.27)))) (number "1" (effects (font (size 1.27 1.27)))))
      )
    )
    (symbol "power:+5V_HEAT" (power) (pin_names (offset 0)) (in_bom yes) (on_board yes)
      (property "Reference" "#PWR" (at 0 -3.81 0) (effects (font (size 1.27 1.27)) hide))
      (property "Value" "+5V_HEAT" (at 0 3.556 0) (effects (font (size 1.27 1.27))))
      (property "Footprint" "" (at 0 0 0) (effects (font (size 1.27 1.27)) hide))
      (property "Datasheet" "" (at 0 0 0) (effects (font (size 1.27 1.27)) hide))
      (symbol "+5V_HEAT_0_1"
        (polyline (pts (xy -0.762 1.27) (xy 0 2.54)) (stroke (width 0) (type default)) (fill (type none)))
        (polyline (pts (xy 0 0) (xy 0 2.54)) (stroke (width 0) (type default)) (fill (type none)))
        (polyline (pts (xy 0 2.54) (xy 0.762 1.27)) (stroke (width 0) (type default)) (fill (type none)))
      )
      (symbol "+5V_HEAT_1_1"
        (pin power_in line (at 0 0 90) (length 0) hide (name "+5V_HEAT" (effects (font (size 1.27 1.27)))) (number "1" (effects (font (size 1.27 1.27)))))
      )
    )
    (symbol "power:+3V3" (power) (pin_names (offset 0)) (in_bom yes) (on_board yes)
      (property "Reference" "#PWR" (at 0 -3.81 0) (effects (font (size 1.27 1.27)) hide))
      (property "Value" "+3V3" (at 0 3.556 0) (effects (font (size 1.27 1.27))))
      (property "Footprint" "" (at 0 0 0) (effects (font (size 1.27 1.27)) hide))
      (property "Datasheet" "" (at 0 0 0) (effects (font (size 1.27 1.27)) hide))
      (symbol "+3V3_0_1"
        (polyline (pts (xy -0.762 1.27) (xy 0 2.54)) (stroke (width 0) (type default)) (fill (type none)))
        (polyline (pts (xy 0 0) (xy 0 2.54)) (stroke (width 0) (type default)) (fill (type none)))
        (polyline (pts (xy 0 2.54) (xy 0.762 1.27)) (stroke (width 0) (type default)) (fill (type none)))
      )
      (symbol "+3V3_1_1"
        (pin power_in line (at 0 0 90) (length 0) hide (name "+3V3" (effects (font (size 1.27 1.27)))) (number "1" (effects (font (size 1.27 1.27)))))
      )
    )
'''

# Local pin coordinates in symbol space (x_right, y_up) for exact sheet transform & verification
SYMBOL_PINS = {
    "Custom:R": {"1": (0, 5.08), "2": (0, -5.08)},
    "Custom:C": {"1": (0, 5.08), "2": (0, -5.08)},
    "Custom:CP": {"1": (0, 5.08), "2": (0, -5.08)},
    "Custom:L": {"1": (-5.08, 0), "2": (5.08, 0)},
    "Custom:Fuse_PTC": {"1": (-5.08, 0), "2": (5.08, 0)},
    "Custom:D_Schottky_AK": {"1": (-5.08, 0), "2": (5.08, 0)},
    "Custom:D_TVS_KA": {"1": (0, 5.08), "2": (0, -5.08)},
    "Custom:LED_AK": {"1": (0, 5.08), "2": (0, -5.08)},
    "Custom:Q_PMOS_GSD": {"1": (-5.08, 0), "2": (2.54, 5.08), "3": (2.54, -5.08)},
    "Custom:Q_NPN_BEC": {"1": (-5.08, 0), "2": (2.54, -5.08), "3": (2.54, 5.08)},
    "Custom:XL1509-5.0E1": {
        "1": (-12.7, 5.08), "2": (12.7, 5.08), "3": (12.7, 0), "4": (-12.7, -2.54),
        "5": (-5.08, -12.7), "6": (-2.54, -12.7), "7": (2.54, -12.7), "8": (5.08, -12.7),
    },
    "Custom:Conn_01x02": {"1": (7.62, 2.54), "2": (7.62, -2.54)},
    "Custom:Conn_01x04": {"1": (-7.62, 5.08), "2": (-7.62, 2.54), "3": (-7.62, -2.54), "4": (-7.62, -5.08)},
    "Custom:MountingHole": {},
    "Custom:ESP32_DevKit_30P": {
        **{f"L{i+1}": (-17.78, 17.78 - i * 2.54) for i in range(15)},
        **{f"R{i+1}": (17.78, 17.78 - i * 2.54) for i in range(15)},
    },
    "power:GND": {"1": (0, 0)},
    "power:+5V_HEAT": {"1": (0, 0)},
    "power:+3V3": {"1": (0, 0)},
}


def sym_to_sheet(sx, sy, rot, lx, ly):
    rad = math.radians(rot)
    rx = lx * math.cos(rad) - ly * math.sin(rad)
    ry = lx * math.sin(rad) + ly * math.cos(rad)
    return round(sx + rx, 2), round(sy - ry, 2)


class SchematicBuilder:
    def __init__(self, project_name="main_board_pcb"):
        self.root_uuid = uid()
        self.project_name = project_name
        self.elements = []
        self.pwr_idx = 1
        # Geometry tracking for automatic netlist verification
        self.pin_locs = {}       # (ref, pin) -> (x, y)
        self.power_locs = []     # (net_name, (x, y))
        self.wires = []          # ((x1, y1), (x2, y2))
        self.junctions = set()   # (x, y)
        self.labels = []         # (name, (x, y))
        self.no_connects = []    # (x, y)

    def _pt(self, x, y):
        return (round(x, 2), round(y, 2))

    def add_symbol(self, lib_id, ref, val, fp, x, y, rot=0,
                   ref_off=(2.54, -2.54), val_off=(2.54, 2.54), is_power=False):
        x, y = round(x, 2), round(y, 2)
        in_bom = "no" if lib_id == "Custom:MountingHole" else "yes"
        pins = SYMBOL_PINS[lib_id]
        pin_lines = []
        for pnum, (lx, ly) in pins.items():
            px, py = sym_to_sheet(x, y, rot, lx, ly)
            if is_power:
                self.power_locs.append((val, (px, py)))
            else:
                self.pin_locs[(ref, pnum)] = (px, py)
            pin_lines.append(f'    (pin "{pnum}" (uuid "{uid()}"))')

        rx, ry = round(x + ref_off[0], 2), round(y + ref_off[1], 2)
        vx, vy = round(x + val_off[0], 2), round(y + val_off[1], 2)
        ref_hide = " hide" if is_power else ""
        block = f'''  (symbol (lib_id "{lib_id}") (at {x:.2f} {y:.2f} {rot}) (unit 1)
    (in_bom {in_bom}) (on_board yes) (dnp no)
    (uuid "{uid()}")
    (property "Reference" "{ref}" (at {rx:.2f} {ry:.2f} 0) (effects (font (size 1.27 1.27)){ref_hide}))
    (property "Value" "{val}" (at {vx:.2f} {vy:.2f} 0) (effects (font (size 1.27 1.27))))
    (property "Footprint" "{fp}" (at {x:.2f} {y:.2f} 0) (effects (font (size 1.27 1.27)) hide))
    (property "Datasheet" "~" (at {x:.2f} {y:.2f} 0) (effects (font (size 1.27 1.27)) hide))
{chr(10).join(pin_lines)}
    (instances
      (project "{self.project_name}"
        (path "/{self.root_uuid}"
          (reference "{ref}") (unit 1)
        )
      )
    )
  )'''
        self.elements.append(block)
        return {pnum: sym_to_sheet(x, y, rot, lx, ly) for pnum, (lx, ly) in pins.items()}

    def add_power(self, net_name, x, y, rot=0):
        ref = f"#PWR{self.pwr_idx:02d}"
        self.pwr_idx += 1
        val_off = (0, 3.81) if net_name == "GND" else (0, -3.56)
        self.add_symbol(f"power:{net_name}", ref, net_name, "", x, y, rot=rot,
                        ref_off=(0, 3.81), val_off=val_off, is_power=True)

    def add_wire(self, p1, p2):
        p1 = self._pt(*p1)
        p2 = self._pt(*p2)
        if p1 == p2:
            return
        self.wires.append((p1, p2))
        self.elements.append(
            f'  (wire (pts (xy {p1[0]:.2f} {p1[1]:.2f}) (xy {p2[0]:.2f} {p2[1]:.2f}))'
            f' (stroke (width 0) (type default)) (uuid "{uid()}"))'
        )

    def add_poly_wire(self, pts):
        for a, b in zip(pts, pts[1:]):
            self.add_wire(a, b)

    def add_junction(self, pt):
        pt = self._pt(*pt)
        if pt in self.junctions:
            return
        self.junctions.add(pt)
        self.elements.append(
            f'  (junction (at {pt[0]:.2f} {pt[1]:.2f}) (diameter 0) (color 0 0 0 0) (uuid "{uid()}"))'
        )

    def add_label(self, name, pt, rot=0, justify="left"):
        pt = self._pt(*pt)
        self.labels.append((name, pt))
        self.elements.append(
            f'  (label "{name}" (at {pt[0]:.2f} {pt[1]:.2f} {rot}) (fields_autoplaced)'
            f' (effects (font (size 1.27 1.27)) (justify {justify} bottom)) (uuid "{uid()}"))'
        )

    def add_no_connect(self, pt):
        pt = self._pt(*pt)
        self.no_connects.append(pt)
        self.elements.append(
            f'  (no_connect (at {pt[0]:.2f} {pt[1]:.2f}) (uuid "{uid()}"))'
        )

    def add_box(self, x1, y1, x2, y2, title):
        self.elements.append(
            f'  (rectangle (start {x1:.2f} {y1:.2f}) (end {x2:.2f} {y2:.2f})'
            f' (stroke (width 0.254) (type dash) (color 0 90 160 1)) (fill (type none)) (uuid "{uid()}"))'
        )
        self.elements.append(
            f'  (text "{title}" (at {x1 + 2.54:.2f} {y1 + 3.81:.2f} 0)'
            f' (effects (font (size 2.032 2.032) bold) (justify left)) (uuid "{uid()}"))'
        )

    def add_note(self, x, y, text):
        self.elements.append(
            f'  (text "{text}" (at {x:.2f} {y:.2f} 0)'
            f' (effects (font (size 1.27 1.27)) (justify left)) (uuid "{uid()}"))'
        )

    def render(self) -> str:
        return f'''(kicad_sch (version 20230121) (generator wst_main_sch_gen)

  (uuid "{self.root_uuid}")

  (paper "A3")

  (title_block
    (title "WST 3-CH Thermal Liquid Level Sensor - Main Carrier Board")
    (date "2026-11-25")
    (rev "v4.0")
    (company "WST Water")
    (comment 1 "Plug-in ESP32 DevKit V1 (30P, 25.4mm) | 88.0 x 78.0 mm 2-Layer PCB")
    (comment 2 "12-24V Wide Input -> XL1509-5.0E1 (+5V_HEAT) | 3x High-Side P-MOS + 3x Independent 1-Wire")
  )

  (lib_symbols
{LIB_SYMBOLS}
  )

{chr(10).join(self.elements)}

  (sheet_instances
    (path "/" (page "1"))
  )
)
'''

    # ==================== Geometric Netlist Extractor ====================
    def extract_netlist(self):
        """
        Reconstruct electrical nets purely from wires, junctions, pins, power symbols,
        and net labels (exact KiCad connectivity semantics).
        """
        parent = {}

        def find(a):
            parent.setdefault(a, a)
            while parent[a] != a:
                parent[a] = parent[parent[a]]
                a = parent[a]
            return a

        def union(a, b):
            ra, rb = find(a), find(b)
            if ra != rb:
                parent[ra] = rb

        def pt_on_seg(p, a, b):
            px, py = p
            ax, ay = a
            bx, by = b
            if abs((bx - ax) * (py - ay) - (by - ay) * (px - ax)) > 1e-4:
                return False
            return (min(ax, bx) - 1e-4 <= px <= max(ax, bx) + 1e-4 and
                    min(ay, by) - 1e-4 <= py <= max(ay, by) + 1e-4)

        all_pts = set()
        for a, b in self.wires:
            union(a, b)
            all_pts.add(a)
            all_pts.add(b)
        for p in self.pin_locs.values():
            all_pts.add(p)
        for _, p in self.power_locs:
            all_pts.add(p)
        for _, p in self.labels:
            all_pts.add(p)
        for j in self.junctions:
            all_pts.add(j)

        # In KiCad, a pin, junction, or label sitting on a wire connects to it
        connectable = set(self.pin_locs.values()) | {p for _, p in self.power_locs} | {p for _, p in self.labels} | self.junctions
        for p in connectable:
            for a, b in self.wires:
                if pt_on_seg(p, a, b):
                    union(p, a)

        # Named nets connect globally across labels & power symbols of the same name
        name_anchor = {}
        for name, p in self.power_locs + self.labels:
            if name in name_anchor:
                union(p, name_anchor[name])
            else:
                name_anchor[name] = p

        # Build net -> set of (ref, pin)
        root_to_name = {}
        for name, p in self.power_locs + self.labels:
            r = find(p)
            if r in root_to_name and root_to_name[r] != name:
                raise RuntimeError(f"SHORT CIRCUIT between {root_to_name[r]} and {name} at {p}!")
            root_to_name[r] = name

        nets = defaultdict(set)
        for (ref, pnum), p in self.pin_locs.items():
            r = find(p)
            nname = root_to_name.get(r, f"__UNLABELED_{p}__")
            nets[nname].add(f"{ref}.{pnum}")
        return nets


def build_schematic(output_path: str):
    sb = SchematicBuilder("main_board_pcb")

    # =========================================================================
    # BLOCK 1: Industrial Wide-Input Power Supply (12-24V -> +5V_HEAT)
    # =========================================================================
    sb.add_box(15.24, 15.24, 198.12, 127.00, "1. Industrial Wide-Input Power (12-24V DC -> +5.0V_HEAT, 2A Buck)")

    # J1: 12-24V Input Terminal
    j1 = sb.add_symbol("Custom:Conn_01x02", "J1", "12-24V_IN", "TerminalBlock:KF2EDGK_2P_P5.08mm",
                       30.48, 53.34, 0, ref_off=(-2.54, -6.35), val_off=(-2.54, 6.35))
    sb.add_wire(j1["2"], (43.18, 55.88))
    sb.add_wire((43.18, 55.88), (43.18, 96.52))
    sb.add_power("GND", 43.18, 96.52)

    # F1: PTC 1A 36V
    f1 = sb.add_symbol("Custom:Fuse_PTC", "F1", "PTC_1A_36V", "Fuse:Fuse_1812",
                       53.34, 50.80, 0, ref_off=(0, -3.81), val_off=(0, 3.81))
    sb.add_wire(j1["1"], f1["1"])
    sb.add_label("VIN_RAW", (43.18, 50.80))

    # D1: SS34 Reverse Polarity Protection
    d1 = sb.add_symbol("Custom:D_Schottky_AK", "D1", "SS34", "Diode_SMD:D_SMA",
                       73.66, 50.80, 0, ref_off=(0, -3.81), val_off=(0, 3.81))
    sb.add_wire(f1["2"], d1["1"])
    sb.add_label("VIN_FUSE", (63.50, 50.80))

    # VIN_PROT rail to D2, C1, U1.1
    d2 = sb.add_symbol("Custom:D_TVS_KA", "D2", "SMBJ28A", "Diode_SMD:D_SMB",
                       86.36, 73.66, 0, ref_off=(2.54, -1.27), val_off=(2.54, 1.27))
    c1 = sb.add_symbol("Custom:CP", "C1", "100uF_35V", "Capacitor_THT:CP_Radial_D8.0mm_P3.50mm",
                       104.14, 73.66, 0, ref_off=(2.54, -1.27), val_off=(2.54, 1.27))
    u1 = sb.add_symbol("Custom:XL1509-5.0E1", "U1", "XL1509-5.0E1", "Package_SO:SOIC-8",
                       132.08, 55.88, 0, ref_off=(0, -10.16), val_off=(0, -12.70))

    sb.add_wire(d1["2"], u1["1"])
    sb.add_label("VIN_PROT", (91.44, 50.80))
    sb.add_wire((86.36, 50.80), d2["1"])
    sb.add_junction((86.36, 50.80))
    sb.add_wire(d2["2"], (86.36, 96.52))
    sb.add_power("GND", 86.36, 96.52)

    sb.add_wire((104.14, 50.80), c1["1"])
    sb.add_junction((104.14, 50.80))
    sb.add_wire(c1["2"], (104.14, 96.52))
    sb.add_power("GND", 104.14, 96.52)

    # U1 ON/OFF (Pin 4) and GND (Pins 5..8)
    sb.add_poly_wire([u1["4"], (116.84, 58.42), (116.84, 96.52), (137.16, 96.52)])
    for p in ("5", "6", "7", "8"):
        sb.add_wire(u1[p], (u1[p][0], 96.52))
        if p != "8":
            sb.add_junction((u1[p][0], 96.52))
    sb.add_power("GND", 132.08, 96.52)
    sb.add_junction((132.08, 96.52))

    # D3: SS34 Freewheeling Diode (Anode=1=GND, Cathode=2=XL_SW -> rot=90 puts Pin1 bottom, Pin2 top? Let's check!)
    # Custom:D_Schottky_AK has Pin1=(-5.08, 0), Pin2=(+5.08, 0).
    # With rot=90: Pin1 -> (sx, sy + 5.08) [BOTTOM], Pin2 -> (sx, sy - 5.08) [TOP]!
    d3 = sb.add_symbol("Custom:D_Schottky_AK", "D3", "SS34", "Diode_SMD:D_SMA",
                       152.40, 73.66, 90, ref_off=(2.54, -1.27), val_off=(2.54, 1.27))
    l1 = sb.add_symbol("Custom:L", "L1", "47uH_3A", "Inductor_SMD:L_10x10",
                       165.10, 50.80, 0, ref_off=(0, -3.81), val_off=(0, 3.81))

    sb.add_wire(u1["2"], l1["1"])
    sb.add_label("XL_SW", (147.32, 50.80))
    sb.add_wire((152.40, 50.80), d3["2"])
    sb.add_junction((152.40, 50.80))
    sb.add_wire(d3["1"], (152.40, 96.52))
    sb.add_power("GND", 152.40, 96.52)

    # +5V_HEAT Output rail: L1.2 -> C3, C4, U1.3 (FB), +5V_HEAT
    c3 = sb.add_symbol("Custom:CP", "C3", "220uF_10V", "Capacitor_THT:CP_Radial_D6.3mm_P2.50mm",
                       177.80, 73.66, 0, ref_off=(2.54, -1.27), val_off=(2.54, 1.27))
    c4 = sb.add_symbol("Custom:C", "C4", "10uF_X7R", "Capacitor_SMD:C_0805",
                       190.50, 73.66, 0, ref_off=(2.54, -1.27), val_off=(2.54, 1.27))

    sb.add_wire(l1["2"], (190.50, 50.80))
    sb.add_wire((177.80, 50.80), (177.80, 40.64))
    sb.add_junction((177.80, 50.80))
    sb.add_power("+5V_HEAT", 177.80, 40.64)

    sb.add_wire((177.80, 50.80), c3["1"])
    sb.add_wire(c3["2"], (177.80, 96.52))
    sb.add_power("GND", 177.80, 96.52)

    sb.add_wire((190.50, 50.80), c4["1"])
    sb.add_wire(c4["2"], (190.50, 96.52))
    sb.add_power("GND", 190.50, 96.52)

    # U1.3 (FB) feedback to +5V_HEAT
    sb.add_poly_wire([u1["3"], (172.72, 55.88), (172.72, 50.80)])
    sb.add_junction((172.72, 50.80))

    sb.add_note(20.32, 114.30, "Note: C1/C3 must be -40~105C Low-ESR industrial electrolytic; C4 X7R MLCC.")
    sb.add_note(20.32, 119.38, "Single-channel heating load: 5.0V / 16.5R = 303mA (1.52W); Total peak < 0.6A.")

    # =========================================================================
    # BLOCK 2: Main Controller - ESP32 DevKit V1 (30-Pin) + Mounting Holes
    # =========================================================================
    sb.add_box(205.74, 15.24, 322.58, 127.00, "2. Main Controller (ESP32 DevKit V1 30-Pin, 25.4mm)")

    m1 = sb.add_symbol("Custom:ESP32_DevKit_30P", "M1", "ESP32_DEVKIT_30P",
                       "Module:ESP32_DevKit_V1_30Pin_25.4mm",
                       264.16, 68.58, 0, ref_off=(0, -22.86), val_off=(0, 22.86))

    # Left side connections
    left_used = {
        "L4": ("ADC_5V_MON", "label"),
        "L8": ("DQ_CH1", "label"),
        "L9": ("DQ_CH2", "label"),
        "L10": ("DQ_CH3", "label"),
        "L14": ("GND", "power"),
        "L15": ("+5V_HEAT", "power"),
    }
    for i in range(1, 16):
        pin = f"L{i}"
        pt = m1[pin]
        if pin in left_used:
            name, kind = left_used[pin]
            if kind == "label":
                end = (pt[0] - 12.70, pt[1])
                sb.add_wire(pt, end)
                sb.add_label(name, end, rot=0, justify="left")
            elif name == "GND":
                sb.add_poly_wire([pt, (pt[0] - 5.08, pt[1]), (pt[0] - 5.08, pt[1] + 5.08)])
                sb.add_power("GND", pt[0] - 5.08, pt[1] + 5.08)
            else:  # +5V_HEAT
                sb.add_poly_wire([pt, (pt[0] - 12.70, pt[1]), (pt[0] - 12.70, pt[1] - 5.08)])
                sb.add_power("+5V_HEAT", pt[0] - 12.70, pt[1] - 5.08)
        else:
            sb.add_no_connect(pt)

    # Right side connections
    right_used = {
        "R6": ("LED_STAT", "label"),
        "R7": ("HEAT_EN3", "label"),
        "R9": ("HEAT_EN2", "label"),
        "R10": ("HEAT_EN1", "label"),
        "R14": ("GND", "power"),
        "R15": ("+3V3", "power"),
    }
    for i in range(1, 16):
        pin = f"R{i}"
        pt = m1[pin]
        if pin in right_used:
            name, kind = right_used[pin]
            if kind == "label":
                end = (pt[0] + 12.70, pt[1])
                sb.add_wire(pt, end)
                sb.add_label(name, (pt[0] + 2.54, pt[1]), rot=0, justify="left")
            elif name == "GND":
                sb.add_poly_wire([pt, (pt[0] + 5.08, pt[1]), (pt[0] + 5.08, pt[1] + 5.08)])
                sb.add_power("GND", pt[0] + 5.08, pt[1] + 5.08)
            else:  # +3V3
                sb.add_poly_wire([pt, (pt[0] + 12.70, pt[1]), (pt[0] + 12.70, pt[1] - 5.08)])
                sb.add_power("+3V3", pt[0] + 12.70, pt[1] - 5.08)
        else:
            sb.add_no_connect(pt)

    # 4x M3 Mounting Holes
    for idx in range(4):
        sb.add_symbol("Custom:MountingHole", f"H{idx+1}", "M3",
                      "MountingHole:MountingHole_3.2mm_M3",
                      223.52 + idx * 25.40, 114.30, 0, ref_off=(0, -3.81), val_off=(0, 3.81))

    # =========================================================================
    # BLOCK 3: 5V Bus Voltage Monitor & Status LED
    # =========================================================================
    sb.add_box(330.20, 15.24, 403.86, 127.00, "3. 5V Monitor & LED")

    # 5V Voltage Divider (20k / 10k + 100nF)
    rm1 = sb.add_symbol("Custom:R", "R_MON1", "20k_1%", "Resistor_SMD:R_0603",
                        345.44, 48.26, 0, ref_off=(2.54, -1.27), val_off=(2.54, 1.27))
    rm2 = sb.add_symbol("Custom:R", "R_MON2", "10k_1%", "Resistor_SMD:R_0603",
                        345.44, 68.58, 0, ref_off=(2.54, -1.27), val_off=(2.54, 1.27))
    cm = sb.add_symbol("Custom:C", "C_MON", "100nF", "Capacitor_SMD:C_0603",
                       365.76, 68.58, 0, ref_off=(2.54, -1.27), val_off=(2.54, 1.27))

    sb.add_wire(rm1["1"], (345.44, 38.10))
    sb.add_power("+5V_HEAT", 345.44, 38.10)
    sb.add_wire(rm1["2"], rm2["1"])
    sb.add_poly_wire([(345.44, 58.42), (365.76, 58.42), cm["1"]])
    sb.add_junction((345.44, 58.42))
    sb.add_label("ADC_5V_MON", (347.98, 58.42))
    sb.add_wire(rm2["2"], (345.44, 81.28))
    sb.add_power("GND", 345.44, 81.28)
    sb.add_wire(cm["2"], (365.76, 81.28))
    sb.add_power("GND", 365.76, 81.28)

    # Status LED (Active LOW)
    rled = sb.add_symbol("Custom:R", "R_LED", "1k", "Resistor_SMD:R_0603",
                         386.08, 48.26, 0, ref_off=(2.54, -1.27), val_off=(2.54, 1.27))
    led1 = sb.add_symbol("Custom:LED_AK", "LED1", "GREEN_0603", "LED_SMD:LED_0603",
                         386.08, 68.58, 0, ref_off=(2.54, -1.27), val_off=(2.54, 1.27))
    sb.add_wire(rled["1"], (386.08, 38.10))
    sb.add_power("+3V3", 386.08, 38.10)
    sb.add_wire(rled["2"], led1["1"])
    sb.add_label("LED_A", (386.08, 58.42))
    sb.add_wire(led1["2"], (386.08, 81.28))
    sb.add_label("LED_STAT", (386.08, 81.28))

    sb.add_note(335.28, 96.52, "V_ADC = V_5V * 10k / (20k+10k)")
    sb.add_note(335.28, 101.60, "5.0V -> 1.667V (ADC1_CH6)")
    sb.add_note(335.28, 111.76, "LED1: GPIO19 LOW = ON (~1.3mA)")

    # =========================================================================
    # BLOCK 4: 3x Identical Probe Channels (High-Side P-MOS + 1-Wire + Terminal)
    # =========================================================================
    ch_names = ["CH1: Carbon Source (J2)", "CH2: NaClO (J3)", "CH3: PFC (J4)"]
    ch_vals = ["CH1_CARBON", "CH2_NACLO", "CH3_PFC"]

    for idx in range(3):
        ch = idx + 1
        ox = 15.24 + idx * 132.08  # Column origin X: 15.24, 147.32, 279.40
        oy = 134.62

        sb.add_box(ox, oy, ox + 124.46, 274.32, f"4.{ch} {ch_names[idx]}")

        # --- High-Side P-MOS Driver (Upper half of channel box) ---
        # HEAT_EN -> R_PD (to GND) & R_B -> QN (S8050) -> PMOS_G -> R_PU (to +5V) & QP (AO3401A) -> HEAT_OUT -> D4..6 & Jx.1
        rpd = sb.add_symbol("Custom:R", f"R_PD{ch}", "10k", "Resistor_SMD:R_0603",
                            ox + 15.24, oy + 63.50, 0, ref_off=(2.54, -1.27), val_off=(2.54, 1.27))
        # R_B horizontal (rot=90: Pin1 left, Pin2 right)
        rb = sb.add_symbol("Custom:R", f"R_B{ch}", "2.2k", "Resistor_SMD:R_0603",
                           ox + 25.40, oy + 53.34, 90, ref_off=(0, -3.81), val_off=(0, 3.81))
        qn = sb.add_symbol("Custom:Q_NPN_BEC", f"QN{ch}", "S8050", "Package_TO_SOT_SMD:SOT-23",
                           ox + 40.64, oy + 53.34, 0, ref_off=(3.81, -1.27), val_off=(3.81, 1.27))
        rpu = sb.add_symbol("Custom:R", f"R_PU{ch}", "10k", "Resistor_SMD:R_0603",
                            ox + 43.18, oy + 25.40, 0, ref_off=(-8.89, -1.27), val_off=(-8.89, 1.27))
        qp = sb.add_symbol("Custom:Q_PMOS_GSD", f"QP{ch}", "AO3401A", "Package_TO_SOT_SMD:SOT-23",
                           ox + 68.58, oy + 38.10, 0, ref_off=(3.81, -1.27), val_off=(3.81, 1.27))
        # D4..6 (1N5819WS): rot=90 -> Pin1(A)=bottom, Pin2(K)=top
        dfw = sb.add_symbol("Custom:D_Schottky_AK", f"D{ch+3}", "1N5819WS", "Diode_SMD:D_SOD-123",
                            ox + 83.82, oy + 58.42, 90, ref_off=(2.54, -1.27), val_off=(2.54, 1.27))

        # Wire HEAT_EN -> R_PD & R_B
        sb.add_wire((ox + 5.08, oy + 53.34), rb["1"])
        sb.add_label(f"HEAT_EN{ch}", (ox + 5.08, oy + 53.34))
        sb.add_wire((ox + 15.24, oy + 53.34), rpd["1"])
        sb.add_junction((ox + 15.24, oy + 53.34))
        sb.add_wire(rpd["2"], (ox + 15.24, oy + 73.66))
        sb.add_power("GND", ox + 15.24, oy + 73.66)

        # Wire R_B -> QN.1 (B)
        sb.add_wire(rb["2"], qn["1"])
        sb.add_label(f"NPN_B{ch}", (ox + 33.02, oy + 53.34))

        # QN.2 (E) -> GND
        sb.add_wire(qn["2"], (ox + 43.18, oy + 73.66))
        sb.add_power("GND", ox + 43.18, oy + 73.66)

        # QN.3 (C) -> PMOS_G -> R_PU.2 & QP.1 (G)
        sb.add_wire(qn["3"], rpu["2"])
        sb.add_wire((ox + 43.18, oy + 38.10), qp["1"])
        sb.add_junction((ox + 43.18, oy + 38.10))
        sb.add_label(f"PMOS_G{ch}", (ox + 48.26, oy + 38.10))

        # +5V_HEAT -> R_PU.1 & QP.2 (S)
        sb.add_wire(rpu["1"], (ox + 43.18, oy + 15.24))
        sb.add_wire((ox + 43.18, oy + 15.24), (ox + 71.12, oy + 15.24))
        sb.add_wire((ox + 71.12, oy + 15.24), qp["2"])
        sb.add_power("+5V_HEAT", ox + 58.42, oy + 15.24)
        sb.add_junction((ox + 58.42, oy + 15.24))

        # --- Terminal Block J2/J3/J4 (Right side of channel box) ---
        jx = sb.add_symbol("Custom:Conn_01x04", f"J{ch+1}", ch_vals[idx],
                           "TerminalBlock:KF2EDGK_4P_P3.81mm",
                           ox + 111.76, oy + 96.52, 0, ref_off=(0, -8.89), val_off=(0, 8.89))

        # QP.3 (D) -> HEAT_OUT -> Dfw.2 (K) & Jx.1
        sb.add_poly_wire([qp["3"], (ox + 71.12, oy + 48.26), (ox + 99.06, oy + 48.26),
                          (ox + 99.06, jx["1"][1]), jx["1"]])
        sb.add_label(f"HEAT_OUT{ch}", (ox + 73.66, oy + 48.26))
        sb.add_wire((ox + 83.82, oy + 48.26), dfw["2"])
        sb.add_junction((ox + 83.82, oy + 48.26))
        sb.add_wire(dfw["1"], (ox + 83.82, oy + 73.66))
        sb.add_power("GND", ox + 83.82, oy + 73.66)

        # --- 1-Wire Interface (Lower half of channel box) ---
        # +3V3 -> R_V (10R) -> VDD_OUT -> Jx.2
        rv = sb.add_symbol("Custom:R", f"R_V{ch}", "10R", "Resistor_SMD:R_0603",
                           ox + 71.12, jx["2"][1], 90, ref_off=(0, -3.81), val_off=(0, 3.81))
        sb.add_poly_wire([(ox + 58.42, oy + 86.36), (ox + 58.42, jx["2"][1]), rv["1"]])
        sb.add_power("+3V3", ox + 58.42, oy + 86.36)
        sb.add_wire(rv["2"], jx["2"])
        sb.add_label(f"VDD_OUT{ch}", (ox + 81.28, jx["2"][1]))

        # DQ_CH -> R_SR (100R) -> DQ_EXT -> R_OW (4.7k to +3V3), ESD (to GND), Jx.3
        rsr = sb.add_symbol("Custom:R", f"R_SR{ch}", "100R", "Resistor_SMD:R_0603",
                            ox + 27.94, jx["3"][1], 90, ref_off=(0, -3.81), val_off=(0, 3.81))
        row = sb.add_symbol("Custom:R", f"R_OW{ch}", "4.7k", "Resistor_SMD:R_0603",
                            ox + 45.72, oy + 93.98, 0, ref_off=(-6.35, -1.27), val_off=(-6.35, 1.27))
        esd = sb.add_symbol("Custom:D_TVS_KA", f"ESD{ch}", "PESD3V3", "Diode_SMD:D_SOD-323",
                            ox + 71.12, oy + 114.30, 0, ref_off=(2.54, -1.27), val_off=(2.54, 1.27))

        sb.add_wire((ox + 5.08, jx["3"][1]), rsr["1"])
        sb.add_label(f"DQ_CH{ch}", (ox + 5.08, jx["3"][1]))

        sb.add_wire(rsr["2"], jx["3"])
        sb.add_label(f"DQ_EXT{ch}", (ox + 48.26, jx["3"][1]))

        # R_OW pull-up to +3V3
        sb.add_wire(row["1"], (ox + 45.72, oy + 86.36))
        sb.add_wire((ox + 45.72, oy + 86.36), (ox + 58.42, oy + 86.36))
        sb.add_junction((ox + 58.42, oy + 86.36))
        sb.add_wire(row["2"], (ox + 45.72, jx["3"][1]))
        sb.add_junction((ox + 45.72, jx["3"][1]))

        # ESD to GND
        sb.add_wire((ox + 71.12, jx["3"][1]), esd["1"])
        sb.add_junction((ox + 71.12, jx["3"][1]))
        sb.add_wire(esd["2"], (ox + 71.12, oy + 127.00))
        sb.add_power("GND", ox + 71.12, oy + 127.00)

        # Jx.4 -> GND
        sb.add_poly_wire([jx["4"], (ox + 96.52, jx["4"][1]), (ox + 96.52, oy + 127.00)])
        sb.add_power("GND", ox + 96.52, oy + 127.00)

    os.makedirs(os.path.dirname(output_path), exist_ok=True)
    with open(output_path, "w", encoding="utf-8", newline="\n") as f:
        f.write(sb.render())

    return sb


def verify_against_pcb(sb: SchematicBuilder, pcb_path: str):
    """Parse main_board_pcb.kicad_pcb and verify 100% netlist equivalence."""
    txt = open(pcb_path, encoding="utf-8").read()

    # Extract every footprint's pad -> net from PCB
    pcb_nets = defaultdict(set)
    fp_blocks = txt.split('  (footprint "')[1:]
    for blk in fp_blocks:
        ref_m = re.search(r'\(fp_text reference "([^"]+)"', blk)
        if not ref_m:
            continue
        ref = ref_m.group(1)
        for pnum, nname in re.findall(r'\(pad "([^"]+)"[^)]*\(at[^)]+\)[^)]*\(size[^)]+\).*?\(net \d+ "([^"]*)"\)', blk):
            if nname:
                pcb_nets[nname].add(f"{ref}.{pnum}")

    sch_nets = sb.extract_netlist()
    # Filter out unconnected DevKit pins (which have __UNLABELED_... in sch and empty net in pcb)
    sch_named = {k: v for k, v in sch_nets.items() if not k.startswith("__UNLABELED_")}

    print(f"Schematic named nets: {len(sch_named)} | PCB named nets: {len(pcb_nets)}")
    mismatches = 0
    for net in sorted(set(sch_named.keys()) | set(pcb_nets.keys())):
        s_pins = sch_named.get(net, set())
        p_pins = pcb_nets.get(net, set())
        if s_pins != p_pins:
            mismatches += 1
            print(f"  [MISMATCH] Net '{net}':")
            if s_pins - p_pins:
                print(f"    Only in SCH: {sorted(s_pins - p_pins)}")
            if p_pins - s_pins:
                print(f"    Only in PCB: {sorted(p_pins - s_pins)}")

    if mismatches == 0:
        print("SUCCESS: 100% Netlist & Pin-to-Net equivalence verified against main_board_pcb.kicad_pcb!")
    else:
        raise SystemExit(f"FAILED: {mismatches} netlist mismatches found!")


if __name__ == "__main__":
    here = os.path.dirname(os.path.abspath(__file__))
    sch_out = os.path.join(here, "main_board_pcb.kicad_sch")
    pcb_in = os.path.join(here, "main_board_pcb.kicad_pcb")
    sb = build_schematic(sch_out)
    print(f"Generated KiCad 7/8 Schematic: {sch_out}")
    if os.path.exists(pcb_in):
        verify_against_pcb(sb, pcb_in)
