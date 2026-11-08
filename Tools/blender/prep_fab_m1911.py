"""
Fab 'M1911 (FREE)' (Xeradev, Fab Standard License) FBX를 인벤토리 아이템용으로 다듬는다.
- 옆모습이 위(+Z)에서 보이도록 눕히고, 피벗을 바닥 가운데로 옮긴다
- Unreal용 ORM 텍스처를 거칠기·금속성으로 나눠 Principled BSDF에 연결한다 (UE 임포트 시 머티리얼 자동 생성)
  blender -b --factory-startup --python Tools/blender/prep_fab_m1911.py -- <압축 푼 폴더> <출력 FBX>
원본 파일은 C:/RE4Inventory_LocalAssets/fab/M1911 에만 둔다.
"""
import bpy, os, sys, math
import numpy as np

SRC, OUT = sys.argv[sys.argv.index("--") + 1:][:2]
TEX = os.path.join(SRC, "tex", "Unreal_Textures", "M1911")
SPLIT = os.path.join(os.path.dirname(OUT), "split")
os.makedirs(SPLIT, exist_ok=True)

bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.fbx(filepath=os.path.join(SRC, "fbx", "FBX", "NonRigged", "NonRigged_M1911.fbx"))
obj = [o for o in bpy.context.scene.objects if o.type == "MESH"][0]
bpy.context.view_layer.objects.active = obj
obj.select_set(True)
obj.rotation_euler = (math.radians(-90), 0, 0)
bpy.ops.object.transform_apply(location=False, rotation=True, scale=True)
xs = [v.co.x for v in obj.data.vertices]; ys = [v.co.y for v in obj.data.vertices]; zs = [v.co.z for v in obj.data.vertices]
off = ((min(xs) + max(xs)) / 2, (min(ys) + max(ys)) / 2, min(zs))
for v in obj.data.vertices:
    v.co.x -= off[0]; v.co.y -= off[1]; v.co.z -= off[2]
obj.location = (0, 0, 0)


def load(path, non_color):
    img = bpy.data.images.load(path)
    if non_color:
        img.colorspace_settings.name = "Non-Color"
    return img


def split_orm(part):
    src = bpy.data.images.load(os.path.join(TEX, f"M1911_low_{part}_OcclusionRoughnessMetallic.png"))
    w, h = src.size
    px = np.array(src.pixels[:], dtype=np.float32).reshape(h, w, 4)
    out = {}
    for name, ch in (("Roughness", 1), ("Metallic", 2)):
        img = bpy.data.images.new(f"M1911_{part}_{name}", w, h, alpha=False, float_buffer=False)
        g = np.repeat(px[:, :, ch:ch + 1], 3, axis=2)
        img.pixels[:] = np.concatenate([g, np.ones((h, w, 1), np.float32)], axis=2).ravel()
        img.filepath_raw = os.path.join(SPLIT, f"M1911_{part}_{name}.png")
        img.file_format = "PNG"
        img.save()
        img.colorspace_settings.name = "Non-Color"
        out[name] = img
    return out


for slot in obj.material_slots:
    part = slot.material.name.split(".")[0]
    mat = bpy.data.materials.new(part)
    mat.use_nodes = True
    nt = mat.node_tree
    bsdf = nt.nodes["Principled BSDF"]
    base = nt.nodes.new("ShaderNodeTexImage"); base.image = load(os.path.join(TEX, f"M1911_low_{part}_BaseColor.png"), False)
    nt.links.new(base.outputs["Color"], bsdf.inputs["Base Color"])
    orm = split_orm(part)
    for name in ("Roughness", "Metallic"):
        n = nt.nodes.new("ShaderNodeTexImage"); n.image = orm[name]
        nt.links.new(n.outputs["Color"], bsdf.inputs[name])
    nrm = nt.nodes.new("ShaderNodeTexImage"); nrm.image = load(os.path.join(TEX, f"M1911_low_{part}_Normal.png"), True)
    nm = nt.nodes.new("ShaderNodeNormalMap")
    nt.links.new(nrm.outputs["Color"], nm.inputs["Color"])
    nt.links.new(nm.outputs["Normal"], bsdf.inputs["Normal"])
    slot.material = mat

obj.name = "SM_Fab_M1911"
obj.data.name = "SM_Fab_M1911"
bpy.ops.export_scene.fbx(filepath=OUT, use_selection=True, path_mode="COPY", embed_textures=True,
                         apply_scale_options="FBX_SCALE_UNITS", mesh_smooth_type="FACE")
print("[prep] dims", tuple(round(d, 3) for d in obj.dimensions), "->", OUT)
