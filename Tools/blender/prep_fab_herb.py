"""
Fab 'Peppermint' (Quixel Megascans, Fab Standard License) 두 포기를 묶어 인벤토리 약초 아이템으로 다듬는다.
- 선 포기를 높이만 눌러 케이스 안에 들어가게 하고(위에서 잎 면이 보임), 피벗을 바닥 가운데로
- BaseColor/Opacity/Normal/Roughness를 Principled BSDF에 연결 (UE 임포트 시 머티리얼 자동 생성)
- 케이스 조명에서 너무 어두워 BaseColor를 1.7배 밝힘
- 붉은 변형: 잎 색만 색조를 돌린 BaseColor를 따로 만든다 (원본 텍스처 수정, 라이선스상 허용)
  blender -b --factory-startup --python Tools/blender/prep_fab_herb.py -- <src 폴더> <출력 폴더> <변형 글자들, 예: BC>
원본 zip/압축 해제본은 C:/RE4Inventory_LocalAssets/fab/Peppermint 에만 둔다.
"""
import bpy, os, sys, math, glob, colorsys
import numpy as np

SRC, OUT, VAR = sys.argv[sys.argv.index("--") + 1:][:3]
os.makedirs(OUT, exist_ok=True)
tex = lambda kind: glob.glob(os.path.join(SRC, f"*_Mid_2K_{kind}.jpg"))[0]


def make_red(src_path, dst_path):
    img = bpy.data.images.load(src_path)
    w, h = img.size
    px = np.array(img.pixels[:], dtype=np.float32).reshape(h, w, 4)
    rgb = px[:, :, :3]
    mx, mn = rgb.max(2), rgb.min(2)
    # 초록 성분이 큰 픽셀(잎)만 빨강 쪽으로: R과 G를 맞바꾸고 채도를 조금 올린다
    leaf = (rgb[:, :, 1] >= rgb[:, :, 0]) & (mx - mn > 0.02)
    out = rgb.copy()
    out[leaf, 0] = np.clip(rgb[leaf, 1] * 1.15, 0, 1)
    out[leaf, 1] = rgb[leaf, 0] * 0.55
    out[leaf, 2] = rgb[leaf, 2] * 0.6
    res = bpy.data.images.new("red", w, h, alpha=False)
    res.pixels[:] = np.concatenate([out, np.ones((h, w, 1), np.float32)], axis=2).ravel()
    res.filepath_raw = dst_path
    res.file_format = "PNG"
    res.save()
    return dst_path


def build(name, base_path):
    bpy.ops.wm.read_factory_settings(use_empty=True)
    parts = []
    for i, var in enumerate(VAR):
        fbx = glob.glob(os.path.join(SRC, f"*_Var{var}_LOD1.fbx"))[0]
        bpy.ops.import_scene.fbx(filepath=fbx)
        part = [o for o in bpy.context.scene.objects if o.type == "MESH" and o not in parts][0]
        bpy.ops.object.select_all(action="DESELECT")
        bpy.context.view_layer.objects.active = part
        part.select_set(True)
        bpy.ops.object.transform_apply(location=False, rotation=True, scale=True)
        # 2x1 칸에 맞게 포기를 가로(X)로 나란히, 조금씩 겹치게
        part.location.x += i * 0.32
        parts.append(part)
    bpy.ops.object.select_all(action="DESELECT")
    for part in parts:
        part.select_set(True)
    bpy.context.view_layer.objects.active = parts[0]
    bpy.ops.object.join()
    obj = bpy.context.view_layer.objects.active
    bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)
    # 눕히면 잎이 옆을 봐서 위에서 줄기만 보인다. 선 채로 두고 높이만 눌러(35%) 위에서 잎 면이 보이게 한다
    obj.scale = (1.0, 1.0, 0.35)
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
    xs = [v.co.x for v in obj.data.vertices]; ys = [v.co.y for v in obj.data.vertices]; zs = [v.co.z for v in obj.data.vertices]
    off = ((min(xs) + max(xs)) / 2, (min(ys) + max(ys)) / 2, min(zs))
    for v in obj.data.vertices:
        v.co.x -= off[0]; v.co.y -= off[1]; v.co.z -= off[2]
    obj.location = (0, 0, 0)

    mat = bpy.data.materials.new(name + "_Leaf")
    mat.use_nodes = True
    nt = mat.node_tree
    bsdf = nt.nodes["Principled BSDF"]
    def img(path, non_color):
        n = nt.nodes.new("ShaderNodeTexImage"); n.image = bpy.data.images.load(path)
        if non_color: n.image.colorspace_settings.name = "Non-Color"
        return n
    nt.links.new(img(base_path, False).outputs["Color"], bsdf.inputs["Base Color"])
    nt.links.new(img(tex("Opacity"), True).outputs["Color"], bsdf.inputs["Alpha"])
    nt.links.new(img(tex("Roughness"), True).outputs["Color"], bsdf.inputs["Roughness"])
    nm = nt.nodes.new("ShaderNodeNormalMap")
    nt.links.new(img(tex("Normal"), True).outputs["Color"], nm.inputs["Color"])
    nt.links.new(nm.outputs["Normal"], bsdf.inputs["Normal"])
    obj.data.materials.clear()
    obj.data.materials.append(mat)
    obj.name = obj.data.name = name
    path = os.path.join(OUT, name + ".fbx")
    bpy.ops.export_scene.fbx(filepath=path, use_selection=True, path_mode="COPY", embed_textures=True,
                             apply_scale_options="FBX_SCALE_UNITS", mesh_smooth_type="FACE")
    print("[herb]", name, tuple(round(d, 3) for d in obj.dimensions), "->", path)


def brighten(src_path, dst_path, gain=1.7):
    img = bpy.data.images.load(src_path)
    w, h = img.size
    px = np.array(img.pixels[:], dtype=np.float32).reshape(h, w, 4)
    px[:, :, :3] = np.clip(px[:, :, :3] * gain, 0, 1)
    res = bpy.data.images.new("bright", w, h, alpha=False)
    res.pixels[:] = px.ravel()
    res.filepath_raw = dst_path
    res.file_format = "PNG"
    res.save()
    return dst_path


green = brighten(tex("BaseColor"), os.path.join(OUT, "Peppermint_Green_BaseColor.png"))
build("SM_Fab_HerbGreen", green)
build("SM_Fab_HerbRed", make_red(green, os.path.join(OUT, "Peppermint_Red_BaseColor.png")))
