"""
make_items.py로 만든 FBX를 다시 불러와 위(+Z)에서 내려다본 모습을 한 장씩 렌더한다 (검수용).
  blender -b --factory-startup --python Tools/blender/preview_items.py -- <FBX 폴더> <출력 폴더>
"""
import bpy
import math
import os
import sys

args = sys.argv[sys.argv.index("--") + 1:]
SRC, OUT = args[0], args[1]
os.makedirs(OUT, exist_ok=True)

COLORS = {
    "Gunmetal": (0.05, 0.05, 0.055), "GunmetalDark": (0.02, 0.02, 0.022), "Polymer": (0.03, 0.03, 0.03),
    "PolymerDark": (0.015, 0.015, 0.015), "Wood": (0.25, 0.12, 0.05), "WoodDark": (0.12, 0.05, 0.02),
    "Rubber": (0.02, 0.02, 0.02), "RubberDark": (0.01, 0.01, 0.01), "Steel": (0.55, 0.55, 0.57),
    "Cardboard": (0.45, 0.32, 0.18), "CardboardDark": (0.3, 0.2, 0.1), "LabelYellow": (0.8, 0.6, 0.1),
    "LabelRed": (0.6, 0.05, 0.03), "LabelDark": (0.05, 0.05, 0.05), "Brass": (0.7, 0.5, 0.2),
    "Stem": (0.2, 0.3, 0.1), "LeafGreen": (0.1, 0.45, 0.08), "LeafRed": (0.55, 0.06, 0.05),
    "Twine": (0.5, 0.4, 0.25), "Olive": (0.18, 0.2, 0.1), "OliveDark": (0.1, 0.11, 0.05),
    "Paint": (0.8, 0.8, 0.78), "PaintDark": (0.4, 0.4, 0.4), "LabelGreen": (0.1, 0.5, 0.2),
    "PlasticWhite": (0.9, 0.9, 0.9), "Glass": (0.6, 0.7, 0.7), "Powder": (0.15, 0.15, 0.15),
    "Leather": (0.15, 0.08, 0.04), "LeatherDark": (0.06, 0.03, 0.015), "BrassDark": (0.4, 0.28, 0.1),
}

for f in sorted(os.listdir(SRC)):
    if not f.lower().endswith(".fbx"):
        continue
    bpy.ops.wm.read_factory_settings(use_empty=True)
    bpy.ops.import_scene.fbx(filepath=os.path.join(SRC, f))
    objs = [o for o in bpy.context.scene.objects if o.type == "MESH"]
    for o in objs:
        for m in o.data.materials:
            base = m.name.split(".")[0]
            m.diffuse_color = (*COLORS.get(base, (1, 0, 1)), 1)
    xs, ys, zs = [], [], []
    for o in objs:
        for c in o.bound_box:
            w = o.matrix_world @ __import__("mathutils").Vector(c)
            xs.append(w.x); ys.append(w.y); zs.append(w.z)
    cx, cy = (min(xs) + max(xs)) / 2, (min(ys) + max(ys)) / 2
    span = max(max(xs) - min(xs), max(ys) - min(ys)) * 1.15
    cam_data = bpy.data.cameras.new("cam")
    cam_data.type = "ORTHO"
    cam_data.ortho_scale = span
    cam = bpy.data.objects.new("cam", cam_data)
    cam.location = (cx, cy, max(zs) + 1.0)
    bpy.context.scene.collection.objects.link(cam)
    bpy.context.scene.camera = cam
    sc = bpy.context.scene
    sc.render.engine = "BLENDER_WORKBENCH"
    sc.display.shading.light = "STUDIO"
    sc.display.shading.color_type = "MATERIAL"
    sc.display.shading.show_cavity = True
    sc.render.resolution_x = 360
    sc.render.resolution_y = 360
    sc.render.filepath = os.path.join(OUT, f.replace(".fbx", ".png"))
    bpy.ops.render.render(write_still=True)
    print(f"[preview] {f} size=({max(xs)-min(xs):.3f}, {max(ys)-min(ys):.3f}, {max(zs)-min(zs):.3f})")
