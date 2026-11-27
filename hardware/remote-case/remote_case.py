"""Parametric case for the mecanum robocar remote (FreeCAD script).

Layout (seen from the front, held horizontally):

    +---------------------------------------------------------------+
    |  [B]  +---------------------------+                           |
    | USB   |      T-Display-S3 LCD     |          (  stick  )      |
    |  [K]  +---------------------------+                           |
    +---------------------------------------------------------------+
     ^ USB-C in the left end wall          KY-023 joystick on the right

Parts (all exported to out/):
  remote_case_front.stl  front shell: screen window, stick hole, button holes,
                         holds the T-Display-S3 and the joystick (print face down)
  remote_case_back.stl   back cover, presses the board against the front
                         (print outside face down)
  remote_button_cap.stl  plunger extension for BOOT / KEY (print 2)

Assembly: 4x M3 screws from the back into the front shell (self-tapping
into the printed pilot holes; the script prints the length), 4x M3x6 to hold
the joystick.

Run:  /Applications/FreeCAD.app/Contents/Resources/bin/freecadcmd remote_case.py

Coordinates: X along the case (left = USB end), Y across, Z up = towards the
user. Z = 0 is the outside of the back cover.
"""

import os
import sys

import FreeCAD
import Part
from FreeCAD import Vector as V

HERE = os.path.dirname(os.path.abspath(__file__)) if "__file__" in dir() else os.getcwd()
for a in sys.argv:
    if a.endswith("remote_case.py"):
        HERE = os.path.dirname(os.path.abspath(a))
OUT = os.environ.get("OUT_DIR", os.path.join(HERE, "out"))
REF_STEP = os.path.join(HERE, "ref", "t-display-s3-full.stp")

# =============================================================================
# Parameters (mm)
# =============================================================================
P = dict(
    # --- printing / fit (Bambu Lab, 0.4 mm nozzle, PLA/PETG) ---
    fit=0.3,           # clearance around boards
    wall=2.0,          # side walls (2.6 with joint="skirt")
    face=2.0,          # front face thickness
    floor=2.5,         # back cover thickness
    corner_r=6.0,      # vertical corner radius
    edge_r=1.2,        # rounding of the outer front/back edges

    # --- overall size ---
    width=46.0,        # Y
                       # Z (thickness) follows from the board stack + wiring room, see below
    js_x=93.0,         # X of the joystick centre (from the left outer edge)
                       # (case length follows from the joystick + header clearance)

    # --- T-Display-S3 (from LilyGo's STEP model, board coordinates) ---
    td_pcb_l=60.78, td_pcb_w=25.5, td_pcb_t=1.2,
    td_glass_l=49.87, td_glass_w=25.95, td_glass_y0=6.66, td_glass_t=1.53,
    td_front_z=-6.38,                     # glass surface (board Z), back of PCB = 0
    td_back_parts=3.5,                    # tallest part on the PCB back
    td_usb_y0=-1.2, td_usb_x=-0.14, td_usb_z=-2.78,   # receptacle front/centre
    td_buttons=((-9.29, 2.32), (8.81, 2.20)),        # BOOT / KEY plunger centres (x, y)
    td_button_top_z=-3.6,                 # plunger top (board Z)
    td_reset=(13.34, 10.3, 1.25),         # side reset button (x, y, z)
    td_press_points=((-7.0, 26.0), (7.0, 33.0), (-7.0, 48.0), (4.0, 56.0)),  # bare PCB spots, clear of the plugs

    # pre-soldered pin headers: two rows of 12 at board x = +-11.43, from y = 24.0, 2.54 pitch,
    # pins pointing out of the back of the board
    td_pin_rows=(-11.43, 11.43), td_pin_y0=24.0, td_pin_n=12, td_pin_pitch=2.54,
    td_pin_spacer=2.5,                    # black plastic spacer on the PCB back
    td_pin_len=8.5,                       # PCB back -> pin tips
    td_plug_l=14.0,                       # Dupont female housing length (on top of the spacer)
    td_wire_clear=20.0,                   # PCB back -> inside of the back cover:
                                          # spacer + Dupont plug + wire bend. About 12 is
                                          # enough if the wires are soldered to the pins.

    # visible screen opening (1.9" 170x320 active area ~22.7 x 42.6 + margin)
    win_l=46.0, win_w=23.5, win_chamfer=1.2,

    # USB-C plug overmold opening
    usb_open_w=13.0, usb_open_h=7.5,

    # --- button caps ---
    cap_d=3.2, cap_hole_d=3.7, cap_proud=4.0, cap_gap=0.2,
    cap_flange=(6.0, 3.4, 0.0),           # foot size: along board x, along board y

    # --- KY-023 joystick ---
    # measured: PCB 34.0 x 26.0, holes 26.67 x 20.32 centre-to-centre
    js_pcb_l=34.0, js_pcb_w=26.0, js_pcb_t=1.6,
    js_hole_d=3.2, js_hole_dx=26.67, js_hole_dy=20.32,
    # NOT YET MEASURED (typical values) - everything below until "screws":
    js_stick_off=(0.0, 0.0),              # stick centre relative to PCB centre
    js_header_side=+1,                    # +1: header on the +X short edge
    js_header_clear=18.0,                 # free space past that edge for plugs
    js_body=(20.0, 20.0),                 # black housing footprint
    js_body_h=12.0,                       # PCB top -> top of black housing
    js_face_gap=0.5,                      # housing top -> inside of front face
    js_hole_d_face=24.0,                  # stick opening in the front face
    js_cap_d=26.0, js_cap_h=9.0, js_cap_z=17.0,  # cap diameter, height, bottom above PCB top

    # --- shell joint ---
    # "flat":  deep front shell, flat back plate with a short locating rim
    # "skirt": shallow tray as back cover; its outer wall rises past the split and
    #          wraps round the bottom of the front shell, which is stepped in for it
    joint=os.environ.get("JOINT", "flat"),
    rim_t=1.2, rim_h=2.0, rim_gap=0.2,           # flat: locating rim on the plate
    skirt_t=1.2, skirt_h=4.0, skirt_gap=0.2,     # skirt

    # --- screws ---
    # back cover holes print without supports: flat -> 90 deg countersink in the
    # plate; skirt -> counterbore with a 45 deg cone roof. Countersunk (flat head)
    # screws sit flush; pan / socket heads also work.
    screw_len=None,    # case screws, M3; None = shortest that bites >= 8 mm
    screw_cb=3.0,      # skirt: counterbore depth
    plate_cb=1.5,      # flat, "counterbore" variant: head recess depth in the back plate
    screw_pilot_d=2.6, screw_clear_d=3.4, screw_head_d=6.4,
    boss_d=7.0, back_boss_d=10.0, boss_inset=6.5,
    js_standoff_d=6.0, js_pilot_d=2.6,
)
globals().update(P)
if joint == "skirt":
    wall = max(wall, skirt_t + skirt_gap + 1.2)
    screw_head_d = max(screw_head_d, 6.6)

# Thickness: back cover + wiring room under the T-Display + board stack + front face.
import math
height = math.ceil((floor + td_wire_clear - td_front_z + face) * 2) / 2
face_in = height - face               # inside of front face

# T-Display placement: board (x, y, z) -> case (Y, X, Z) = (x + ty, y + tx, -z + tz)
td_tx = wall + fit - td_usb_y0
td_ty = width / 2.0
td_tz = face_in + td_front_z          # glass touches the front face


def b2c(x, y, z=0.0):
    """Board coordinates -> case coordinates."""
    return V(y + td_tx, x + td_ty, -z + td_tz)


# =============================================================================
# Helpers
# =============================================================================
def box(x0, y0, z0, x1, y1, z1):
    return Part.makeBox(x1 - x0, y1 - y0, z1 - z0, V(x0, y0, z0))


def cyl(d, x, y, z0, z1):
    return Part.makeCylinder(d / 2.0, z1 - z0, V(x, y, z0), V(0, 0, 1))


def rounded_box(x0, y0, z0, x1, y1, z1, r):
    b = box(x0, y0, z0, x1, y1, z1)
    if r <= 0:
        return b
    edges = [e for e in b.Edges if abs(e.Vertexes[0].Z - e.Vertexes[1].Z) > 1e-6]
    return b.makeFillet(min(r, (x1 - x0) / 2 - 0.01, (y1 - y0) / 2 - 0.01), edges)


def frustum(cx, cy, z0, l0, w0, z1, l1, w1):
    def rect(z, l, w):
        return Part.makePolygon([V(cx - l / 2, cy - w / 2, z), V(cx + l / 2, cy - w / 2, z),
                                 V(cx + l / 2, cy + w / 2, z), V(cx - l / 2, cy + w / 2, z),
                                 V(cx - l / 2, cy - w / 2, z)])
    return Part.makeLoft([rect(z0, l0, w0), rect(z1, l1, w1)], True)


def round_edges_at(shape, z, r):
    edges = [e for e in shape.Edges
             if all(abs(v.Z - z) < 1e-6 for v in e.Vertexes)]
    try:
        return shape.makeFillet(r, edges)
    except Exception as ex:  # keep the sharp edge if OCC cannot fillet
        print("edge rounding skipped:", ex)
        return shape


def fuse(shapes):
    s = shapes[0]
    for o in shapes[1:]:
        s = s.fuse(o)
    return s.removeSplitter()


# =============================================================================
# Derived positions
# =============================================================================
js_cx, js_cy = js_x, width / 2.0
js_pcb_cx = js_cx - js_stick_off[0]
js_pcb_cy = js_cy - js_stick_off[1]
js_pcb_top = face_in - js_face_gap - js_body_h
js_pcb_bot = js_pcb_top - js_pcb_t
js_holes = [(js_pcb_cx + sx * js_hole_dx / 2, js_pcb_cy + sy * js_hole_dy / 2)
            for sx in (-1, 1) for sy in (-1, 1)]

# Case length: joystick PCB + room for the header plugs + wall.
length = js_pcb_cx + js_pcb_l / 2 + (js_header_clear if js_header_side > 0 else 4.0) + wall
# Back cover top / front shell bottom: below everything mounted in the front shell.
if joint == "skirt":
    split_z = min(td_tz - td_back_parts - 1.0, js_pcb_bot - 1.0)
else:
    split_z = floor

# How far the case screws go into the front shell bosses (screw tip - split).
_head_z = screw_cb if joint == "skirt" else 0.0
if screw_len is None:
    screw_len = next(L for L in (8, 10, 12, 16, 20, 25, 30) if _head_z + L - split_z >= 8)
screw_bite = _head_z + screw_len - split_z
assert 7.0 <= screw_bite <= face_in - 1.0 - split_z, \
    f"M3x{screw_len} bites {screw_bite:.1f} mm: pick another screw_len"

bosses = [(boss_inset, boss_inset), (boss_inset, width - boss_inset),
          (length - boss_inset, boss_inset), (length - boss_inset, width - boss_inset)]

# =============================================================================
# Front shell
# =============================================================================
def front_shell():
    outer = rounded_box(0, 0, split_z, length, width, height, corner_r)
    outer = round_edges_at(outer, height, edge_r)
    inner = rounded_box(wall, wall, split_z - 1, length - wall, width - wall, face_in,
                        corner_r - wall)
    shell = outer.cut(inner)
    if joint == "skirt":
        # step the outside in at the bottom so the back cover's skirt wraps round it
        r_in = skirt_t + skirt_gap
        ring = rounded_box(-1, -1, split_z - 1, length + 1, width + 1, split_z + skirt_h, 0).cut(
            rounded_box(r_in, r_in, split_z - 2, length - r_in, width - r_in,
                        split_z + skirt_h + 1, corner_r - r_in))
        shell = shell.cut(ring)

    adds = []
    # screw bosses
    for (x, y) in bosses:
        adds.append(cyl(boss_d, x, y, split_z, face_in + 0.01))
    # T-Display locating frame: short ribs around PCB/glass outline
    gx0 = td_tx + td_glass_y0 - fit
    gx1 = td_tx + td_glass_y0 + td_glass_l + fit
    hw = max(td_pcb_w, td_glass_w) / 2 + fit
    rib_z0 = td_tz + 0.5  # stop above the PCB so it is free to seat
    for yy in (td_ty - hw - 1.2, td_ty + hw):
        adds.append(box(gx0 + 4, yy, rib_z0, gx1 - 4, yy + 1.2, face_in + 0.01))
    adds.append(box(td_tx + td_pcb_l + fit, td_ty - 8, rib_z0,
                    td_tx + td_pcb_l + fit + 1.2, td_ty + 8, face_in + 0.01))
    # joystick standoffs
    for (x, y) in js_holes:
        adds.append(cyl(js_standoff_d, x, y, js_pcb_top, face_in + 0.01))
    shell = fuse([shell] + adds)

    cuts = []
    # screen window with chamfer
    wcx = td_tx + td_glass_y0 + td_glass_l / 2
    cuts.append(box(wcx - win_l / 2, td_ty - win_w / 2, face_in - 0.1,
                    wcx + win_l / 2, td_ty + win_w / 2, height + 0.1))
    cuts.append(frustum(wcx, td_ty, height - win_chamfer, win_l, win_w,
                        height + 0.01, win_l + 2 * win_chamfer, win_w + 2 * win_chamfer))
    # glass clearance pocket (glass is slightly wider than the PCB)
    cuts.append(box(gx0, td_ty - td_glass_w / 2 - fit, td_tz, gx1,
                    td_ty + td_glass_w / 2 + fit, face_in))
    # button holes
    for (bx, by) in td_buttons:
        c = b2c(bx, by)
        cuts.append(cyl(cap_hole_d, c.x, c.y, face_in - 1, height + 1))
    # USB-C opening in the left end wall
    u = b2c(td_usb_x, td_usb_y0, td_usb_z)
    cuts.append(box(-1, u.y - usb_open_w / 2, u.z - usb_open_h / 2,
                    wall + 0.5, u.y + usb_open_w / 2, u.z + usb_open_h / 2))
    # reset pinhole in the side wall
    r = b2c(*td_reset)
    cuts.append(Part.makeCylinder(0.9, wall + 2, V(r.x, width - wall - 1, r.z), V(0, 1, 0)))
    # joystick opening
    cuts.append(cyl(js_hole_d_face, js_cx, js_cy, face_in - 1, height + 1))
    cuts.append(Part.makeCone(js_hole_d_face / 2, js_hole_d_face / 2 + 1.0, 1.01,
                              V(js_cx, js_cy, height - 1.0)))
    # screw pilots and joystick pilots
    for (x, y) in bosses:
        cuts.append(cyl(screw_pilot_d, x, y, split_z - 1, face_in - 1.0))
    for (x, y) in js_holes:
        cuts.append(cyl(js_pilot_d, x, y, js_pcb_top - 1, face_in - 1.0))
    return fuse_cut(shell, cuts)


def fuse_cut(shape, cuts):
    for c in cuts:
        shape = shape.cut(c)
    return shape.removeSplitter()


# =============================================================================
# Back cover
# =============================================================================
def back_cover():
    return back_plate() if joint == "flat" else back_tray()


def press_posts():
    """Posts pressing the T-Display PCB against the front face (wide foot for strength)."""
    out = []
    for (px, py) in td_press_points:
        c = b2c(px, py)
        out.append(cyl(4.0, c.x, c.y, floor - 0.01, td_tz))
        out.append(Part.makeCone(3.5, 2.0, 3.0, V(c.x, c.y, floor - 0.01)))
    return out


def back_plate(holes="countersink"):
    """Flat plate with a short rim that locates it inside the front shell walls.

    holes: how the screw holes meet the outside of the plate
      "countersink": 90 deg countersinks, flush flat-head screws, no supports
      "flat":        plain holes, the outside stays flat (pan / socket heads)
      "counterbore": cylindrical recesses for pan / socket heads; the recess roof
                     is a flat overhang, so print it with supports

    Printed outside face down: the rim and posts grow straight up from the plate,
    the screw holes are 90 degree countersinks, so nothing needs supports.
    """
    plate = rounded_box(0, 0, 0, length, width, floor, corner_r)
    plate = round_edges_at(plate, 0, edge_r)
    g = wall + rim_gap
    rim = rounded_box(g, g, floor - 0.01, length - g, width - g, floor + rim_h, corner_r - g).cut(
        rounded_box(g + rim_t, g + rim_t, floor - 1, length - g - rim_t, width - g - rim_t,
                    floor + rim_h + 1, max(corner_r - g - rim_t, 0.5)))
    for (x, y) in bosses:   # the front shell bosses sit in the corners
        rim = rim.cut(cyl(boss_d + 2 * rim_gap, x, y, floor - 1, floor + rim_h + 1))
    shell = fuse([plate, rim] + press_posts())
    cuts = []
    csk_h = (screw_head_d - screw_clear_d) / 2           # 90 degree countersink
    for (x, y) in bosses:
        cuts.append(cyl(screw_clear_d, x, y, -1, floor + 1))
        if holes == "counterbore":
            cuts.append(cyl(screw_head_d, x, y, -1, plate_cb))
        if holes != "countersink":
            continue
        cuts.append(Part.makeCone(screw_head_d / 2 + 0.1, screw_clear_d / 2, csk_h + 0.1,
                                  V(x, y, -0.1)))
    return fuse_cut(shell, cuts)


def back_tray():
    """Tray whose outer wall continues past the split as a skirt round the front shell.

    Printed outside face down: every wall is vertical and the screw holes have
    45 degree roofs, so nothing needs supports.
    """
    outer = rounded_box(0, 0, 0, length, width, split_z + skirt_h - skirt_gap, corner_r)
    outer = round_edges_at(outer, 0, edge_r)
    pocket = rounded_box(wall, wall, floor, length - wall, width - wall, split_z + skirt_h + 1,
                         corner_r - wall)
    skirt_in = rounded_box(skirt_t, skirt_t, split_z, length - skirt_t, width - skirt_t,
                           split_z + skirt_h + 1, corner_r - skirt_t)
    shell = outer.cut(pocket).cut(skirt_in)
    adds = []
    for (x, y) in bosses:
        # thick boss, clipped to the inside of the outer wall
        b = cyl(back_boss_d, x, y, floor - 0.01, split_z)
        adds.append(b.common(box(skirt_t, skirt_t, 0, length - skirt_t, width - skirt_t, split_z)))
    shell = fuse([shell] + adds + press_posts())
    cuts = []
    cone_h = (screw_head_d - screw_clear_d) / 2           # 45 degree roof
    for (x, y) in bosses:
        cuts.append(cyl(screw_clear_d, x, y, -1, split_z + 1))
        cuts.append(cyl(screw_head_d, x, y, -1, screw_cb))
        cuts.append(Part.makeCone(screw_head_d / 2, screw_clear_d / 2, cone_h,
                                  V(x, y, screw_cb - 0.001)))
    return fuse_cut(shell, cuts)


# =============================================================================
# Button cap: a rectangular foot (sits on the plunger, can't fall out through
# the hole) and a round stem through the front face. Print foot down, no supports.
# =============================================================================
def button_cap(x=0.0, y=0.0):
    plunger_z = -td_button_top_z + td_tz          # case Z of the plunger top
    z0 = plunger_z + cap_gap
    z_top = height + cap_proud
    fl, fw, _ = cap_flange
    z_fl1 = face_in - 0.3                         # 0.3 mm free travel under the face
    stem = cyl(cap_d, x, y, z0, z_top)
    flange = box(x - fw / 2, y - fl / 2, z0, x + fw / 2, y + fl / 2, z_fl1)
    s = stem.fuse(flange).removeSplitter()
    return round_edges_at(s, z_top, 0.6)


def button_caps_in_place():
    out = []
    for (bx, by) in td_buttons:
        c = b2c(bx, by)
        out.append(button_cap(c.x, c.y))
    return out


# =============================================================================
# Reference models (for checks and previews)
# =============================================================================
def tdisplay_model():
    if not os.path.exists(REF_STEP):
        return None
    import Import
    doc = FreeCAD.newDocument("ref")
    Import.insert(REF_STEP, "ref")
    shapes = [o.Shape for o in doc.Objects
              if hasattr(o, "Shape") and o.Shape.Solids and "ASM" not in o.Label]
    m = FreeCAD.Matrix(0, 1, 0, td_tx, 1, 0, 0, td_ty, 0, 0, -1, td_tz, 0, 0, 0, 1)
    comp = Part.makeCompound([s.copy() for s in shapes])
    comp = comp.transformGeometry(m)
    FreeCAD.closeDocument("ref")
    return comp


def joystick_model():
    pcb = box(js_pcb_cx - js_pcb_l / 2, js_pcb_cy - js_pcb_w / 2, js_pcb_bot,
              js_pcb_cx + js_pcb_l / 2, js_pcb_cy + js_pcb_w / 2, js_pcb_top)
    for (x, y) in js_holes:
        pcb = pcb.cut(cyl(js_hole_d, x, y, js_pcb_bot - 1, js_pcb_top + 1))
    bw, bl = js_body
    body = box(js_cx - bw / 2, js_cy - bl / 2, js_pcb_top, js_cx + bw / 2, js_cy + bl / 2,
               js_pcb_top + js_body_h)
    shaft = cyl(5.0, js_cx, js_cy, js_pcb_top + js_body_h, js_pcb_top + js_cap_z)
    cap = cyl(js_cap_d, js_cx, js_cy, js_pcb_top + js_cap_z, js_pcb_top + js_cap_z + js_cap_h)
    # right-angle header + Dupont plugs, lying in the PCB plane
    hx = js_pcb_cx + js_header_side * (js_pcb_l / 2)
    hx2 = hx + js_header_side * (js_header_clear - 0.8)
    header = box(min(hx, hx2), js_pcb_cy - 6.5, js_pcb_bot - 1.0,
                 max(hx, hx2), js_pcb_cy + 6.5, js_pcb_top + 2.6)
    return pcb, fuse([body, shaft, cap]), header


def tdisplay_pins_model():
    """Pre-soldered headers: spacer + pins, and the Dupont plugs + wire room on them."""
    y0 = td_pin_y0 - td_pin_pitch / 2
    y1 = td_pin_y0 + (td_pin_n - 0.5) * td_pin_pitch
    hw = 1.3                                  # half width of a spacer / plug row
    pins, plugs = [], []
    for x in td_pin_rows:
        a, b = b2c(x - hw, y0, 0), b2c(x + hw, y1, td_pin_spacer)
        pins.append(box(min(a.x, b.x), min(a.y, b.y), b.z, max(a.x, b.x), max(a.y, b.y), a.z))
        a, b = b2c(x - hw, y0, td_pin_spacer), b2c(x + hw, y1, td_pin_spacer + td_plug_l)
        plugs.append(box(min(a.x, b.x), min(a.y, b.y), b.z, max(a.x, b.x), max(a.y, b.y), a.z))
    return fuse(pins), fuse(plugs)


# =============================================================================
# Build, check, export
# =============================================================================
def check_solid(name, s):
    ok = s.isValid() and len(s.Solids) == 1 and s.Shells[0].isClosed()
    b = s.optimalBoundingBox()
    print(f"[{name}] valid={s.isValid()} solids={len(s.Solids)} closed={s.Shells[0].isClosed()} "
          f"size={b.XLength:.1f} x {b.YLength:.1f} x {b.ZLength:.1f} mm  volume={s.Volume / 1000:.1f} cm3")
    return ok


def overlap(a, b):
    try:
        return a.common(b).Volume
    except Exception:
        return float("nan")


def main():
    import Mesh
    os.makedirs(OUT, exist_ok=True)
    front = front_shell()
    back = back_cover()
    caps = button_caps_in_place()
    cap_print = button_cap()
    cap_print.translate(V(0, 0, -cap_print.BoundBox.ZMin))

    ok = True
    ok &= check_solid("front", front)
    ok &= check_solid("back", back)
    ok &= check_solid("cap", cap_print)
    # back plate variants (flat joint only): only the screw holes differ
    back_variants = {}
    if joint == "flat":
        back_variants = {"flat": back_plate("flat"), "counterbore": back_plate("counterbore")}
        for k, b in back_variants.items():
            ok &= check_solid(f"back_{k}", b)

    print(f"joint: {joint}")
    print(f"case: {length:.1f} x {width:.1f} x {height:.1f} mm, split at Z={split_z:.2f}")
    print(f"case screws: 4x M3x{screw_len}, {'countersunk' if joint == 'flat' else 'head recess %.1f mm' % screw_cb}, bite {screw_bite:.1f} mm")
    print(f"wiring room under the T-Display: {td_tz - floor:.1f} mm "
          f"(plugs end {td_pin_spacer + td_plug_l:.1f} mm from the PCB)")

    # --- interference checks ---
    td = tdisplay_model()
    js_pcb, js_top, js_hdr = joystick_model()
    td_pins, td_plugs = tdisplay_pins_model()
    checks = [("front shell vs back cover", front, back)]
    if td is not None:
        checks += [("T-Display vs front shell", td, front),
                   ("T-Display vs back cover", td, back),
                   ("T-Display vs button caps", td, fuse(caps))]
    checks += [("joystick PCB vs front shell", js_pcb, front),
               ("joystick PCB vs back cover", js_pcb, back),
               ("joystick body vs front shell", js_top, front),
               ("joystick header vs back cover", js_hdr, back),
               ("button caps vs front shell", fuse(caps), front),
               ("T-Display pins vs front shell", td_pins, front),
               ("T-Display plugs vs front shell", td_plugs, front),
               ("T-Display plugs vs back cover", td_plugs, back)]
    for name, a, b in checks:
        v = overlap(a, b)
        flag = "OK" if v < 0.5 else "COLLISION"
        print(f"  {name:32s} overlap {v:8.2f} mm3  {flag}")
    gap = td_plugs.distToShape(back)[0]
    print(f"  {'T-Display plugs to back cover':32s} gap     {gap:8.2f} mm   "
          f"{'OK' if gap >= 0.5 else 'TIGHT'}")

    # --- exports ---
    for name, shape in (("remote_case_front", front), ("remote_case_back", back),
                        ("remote_button_cap", cap_print)):
        Mesh.export([_feature(name, shape)], os.path.join(OUT, name + ".stl"))
        shape.exportStep(os.path.join(OUT, name + ".step"))
    for k, b in back_variants.items():
        name = f"remote_case_back_{k}"
        Mesh.export([_feature(name, b)], os.path.join(OUT, name + ".stl"))
        b.exportStep(os.path.join(OUT, name + ".step"))

    # --- previews (slow: set PREVIEW=1 in the environment) ---
    if os.environ.get("PREVIEW") != "1":
        print("previews skipped (PREVIEW=1 to render)")
        print("ALL SOLIDS OK" if ok else "SOME SOLIDS INVALID")
        return
    sys.path.insert(0, HERE)
    from render import render
    grey, dark, blue = (0.78, 0.8, 0.84), (0.3, 0.32, 0.36), (0.25, 0.35, 0.85)
    items = [(front, grey), (back, dark)] + [(c, (0.9, 0.5, 0.2)) for c in caps]
    if td is not None:
        items.append((td, (0.2, 0.55, 0.3)))
    items += [(js_pcb, (0.2, 0.3, 0.7)), (js_top, (0.15, 0.15, 0.15)),
              (td_pins, (0.1, 0.1, 0.1)), (td_plugs, (0.55, 0.2, 0.6))]
    render(items, os.path.join(OUT, "preview_assembly.png"),
           views=((35, -60), (-35, -120)), titles=["front", "back"], size=6)
    render([(front, grey)], os.path.join(OUT, "preview_front_inside.png"),
           views=((-60, -75),), titles=["front shell, inside"], size=7)
    if back_variants:
        row = [back] + list(back_variants.values())
        render([(b.translated(V(0, 55 * i, 0)), dark) for i, b in enumerate(row)],
               os.path.join(OUT, "preview_back_variants.png"), views=((-50, -100),),
               titles=["back plate outside: countersink / flat / counterbore"], size=7)
    render([(back, dark)], os.path.join(OUT, "preview_back_inside.png"),
           views=((55, -70),), titles=["back cover, inside"], size=7)
    print("ALL SOLIDS OK" if ok else "SOME SOLIDS INVALID")


def _feature(name, shape):
    doc = FreeCAD.ActiveDocument or FreeCAD.newDocument("case")
    f = doc.addObject("Part::Feature", name)
    f.Shape = shape
    return f


main()
