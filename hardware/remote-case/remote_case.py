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

Assembly: 4x M3x16 screws from the back into the front shell (self-tapping
into the printed pilot holes), 4x M3x6 to hold the joystick.

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
    wall=2.0,          # side walls
    face=2.0,          # front face thickness
    floor=2.0,         # back cover thickness
    corner_r=6.0,      # vertical corner radius
    edge_r=1.2,        # rounding of the outer front/back edges

    # --- overall size ---
    width=46.0,        # Y
    height=23.0,       # Z, total thickness
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
    td_press_points=((-8.0, 26.0), (8.0, 33.0), (-8.0, 50.0), (4.0, 56.0)),  # bare PCB spots

    # visible screen opening (1.9" 170x320 active area ~22.7 x 42.6 + margin)
    win_l=46.0, win_w=23.5, win_chamfer=1.2,

    # USB-C plug overmold opening
    usb_open_w=13.0, usb_open_h=7.5,

    # --- button caps ---
    cap_d=3.2, cap_hole_d=3.7, cap_proud=1.0, cap_gap=0.2,
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

    # --- screws ---
    screw_pilot_d=2.6, screw_clear_d=3.4, screw_head_d=6.2, screw_head_h=3.0,
    boss_d=7.0, boss_inset=6.0,
    js_standoff_d=6.0, js_pilot_d=2.6,

    # --- shell joint ---
    lip_t=1.0, lip_h=2.0, lip_gap=0.2,
)
globals().update(P)

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
split_z = min(td_tz - td_back_parts - 1.0, js_pcb_bot - 1.0)

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
    outer = rounded_box(0, 0, 0, length, width, split_z, corner_r)
    outer = round_edges_at(outer, 0, edge_r)
    pocket = rounded_box(wall, wall, floor, length - wall, width - wall, split_z + 1,
                         corner_r - wall)
    shell = outer.cut(pocket)
    # lip that slides into the front shell
    # (slides along the inside of the front shell walls)
    g = wall + lip_gap
    lip_o = rounded_box(g, g, split_z - 0.01, length - g, width - g, split_z + lip_h,
                        corner_r - g)
    lip_i = rounded_box(g + lip_t, g + lip_t, split_z - 1,
                        length - g - lip_t, width - g - lip_t, split_z + lip_h + 1,
                        max(corner_r - g - lip_t, 0.5))
    adds = [lip_o.cut(lip_i)]
    for (x, y) in bosses:
        adds.append(cyl(boss_d, x, y, 0, split_z))
    # posts pressing the T-Display PCB against the front face
    for (px, py) in td_press_points:
        c = b2c(px, py)
        adds.append(cyl(4.0, c.x, c.y, floor - 0.01, td_tz))
    shell = fuse([shell] + adds)
    cuts = []
    for (x, y) in bosses:
        cuts.append(cyl(screw_clear_d, x, y, -1, split_z + lip_h + 1))
        cuts.append(cyl(screw_head_d, x, y, -1, screw_head_h))
    # cut lip away where it would hit the bosses
    for (x, y) in bosses:
        cuts.append(cyl(boss_d + 2 * lip_gap, x, y, split_z, split_z + lip_h + 1))
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
    hx2 = hx + js_header_side * (js_header_clear - lip_t - lip_gap - 0.8)
    header = box(min(hx, hx2), js_pcb_cy - 6.5, js_pcb_bot - 1.0,
                 max(hx, hx2), js_pcb_cy + 6.5, js_pcb_top + 2.6)
    return pcb, fuse([body, shaft, cap]), header


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

    print(f"case: {length:.1f} x {width:.1f} x {height:.1f} mm, split at Z={split_z:.2f}")

    # --- interference checks ---
    td = tdisplay_model()
    js_pcb, js_top, js_hdr = joystick_model()
    checks = [("front shell vs back cover", front, back)]
    if td is not None:
        checks += [("T-Display vs front shell", td, front),
                   ("T-Display vs back cover", td, back),
                   ("T-Display vs button caps", td, fuse(caps))]
    checks += [("joystick PCB vs front shell", js_pcb, front),
               ("joystick PCB vs back cover", js_pcb, back),
               ("joystick body vs front shell", js_top, front),
               ("joystick header vs back cover", js_hdr, back),
               ("button caps vs front shell", fuse(caps), front)]
    for name, a, b in checks:
        v = overlap(a, b)
        flag = "OK" if v < 0.5 else "COLLISION"
        print(f"  {name:32s} overlap {v:8.2f} mm3  {flag}")

    # --- exports ---
    for name, shape in (("remote_case_front", front), ("remote_case_back", back),
                        ("remote_button_cap", cap_print)):
        Mesh.export([_feature(name, shape)], os.path.join(OUT, name + ".stl"))
        shape.exportStep(os.path.join(OUT, name + ".step"))

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
    items += [(js_pcb, (0.2, 0.3, 0.7)), (js_top, (0.15, 0.15, 0.15))]
    render(items, os.path.join(OUT, "preview_assembly.png"),
           views=((35, -60), (-35, -120)), titles=["front", "back"], size=6)
    render([(front, grey)], os.path.join(OUT, "preview_front_inside.png"),
           views=((-60, -75),), titles=["front shell, inside"], size=7)
    render([(back, dark)], os.path.join(OUT, "preview_back_inside.png"),
           views=((55, -70),), titles=["back cover, inside"], size=7)
    print("ALL SOLIDS OK" if ok else "SOME SOLIDS INVALID")


def _feature(name, shape):
    doc = FreeCAD.ActiveDocument or FreeCAD.newDocument("case")
    f = doc.addObject("Part::Feature", name)
    f.Shape = shape
    return f


main()
