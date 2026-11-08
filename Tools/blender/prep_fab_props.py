"""
Fab에서 받은 무료 소품(산탄총, 탄약 상자, 스프레이 캔, 깡통)을 인벤토리 아이템용 FBX로 다듬는다.
- 필요한 메시만 골라 합치고, 가장 긴 축을 +X, 가장 짧은 축을 +Z로 돌린다 (위에서 옆모습이 보이게)
- 피벗을 바닥 가운데로, 텍스처를 Principled BSDF에 연결 (UE 임포트 시 머티리얼 자동 생성)
  blender -b --factory-startup --python Tools/blender/prep_fab_props.py -- <C:/RE4Inventory_LocalAssets/fab> <출력 폴더>
원본 zip/압축 해제본은 C:/RE4Inventory_LocalAssets/fab/<에셋>/src 에만 둔다. 출처·라이선스는 ASSETS.md.
"""
import bpy, bmesh, os, sys
from mathutils import Matrix, Vector

ROOT, OUT = sys.argv[sys.argv.index("--") + 1:][:2]
os.makedirs(OUT, exist_ok=True)

# name: (fbx, 쓸 메시 이름(없으면 전부), 제외할 이름 접두사, 축 자동 정렬, 슬롯별 텍스처 {base, normal, rough, metal})
ASSETS = {
    "SM_Fab_Shotgun": dict(
        fbx="Model12/src/source/inner/Pump action shotgun/Model12_shotgun.fbx",
        # 탄창 속 탄피(shell1~6)와 낱개 탄(shellshot*)은 뺀다. 총 부품 shelling은 남긴다
        exclude=("shell1", "shell2", "shell3", "shell4", "shell5", "shell6", "shellshot"),
        orient=True,
        tex={"8k_Shotgun_Mat": dict(base="Model12/src/textures/4k_Shotgun_colour_muted.png",
                                    normal="Model12/src/textures/2kNormal_shotgun.png",
                                    rough="Model12/src/textures/4kRoughness_shotgun.png",
                                    metal="Model12/src/textures/4k_Shotgun_Metallic.png")}),
    "SM_Fab_Shells": dict(
        fbx="Model12/src/source/inner/Pump action shotgun/Model12_shotgun.fbx",
        only=("shellshot_2",),
        trim=0.86,  # 탄 끝(+X)의 갈라진 몸통이 톱니처럼 비쳐 길이 86%에서 잘라 막는다
        trim_uv=(0.807, 0.156),  # 막은 면에 줄 빨간 몸통 텍셀
        grid=(2, 6),  # 낱개 탄 하나를 2열 x 6줄로 깔아 산탄총 탄 묶음을 만든다 (오른쪽 열을 뒤집어 놋쇠 머리가 양 바깥으로)
        orient=True,
        # 탄의 빨간 몸통까지 갈색이 되지 않게 나무 보정 전 원본 색에, 크림프 끝 검은 틈만 메운 것(Tools/pad_shell_texture.py)을 쓴다
        tex={"8k_Shotgun_Mat": dict(base="Model12/src/textures/4k_Shotgun_colour_shells.png",
                                    normal="Model12/src/textures/2kNormal_shotgun.png",
                                    rough="Model12/src/textures/4kRoughness_shotgun.png",
                                    metal="Model12/src/textures/4k_Shotgun_Metallic.png")}),
    "SM_Fab_AmmoCrate": dict(
        fbx="Cartridges/src/source/inner/box.fbx",
        only=("box", "bullets"),  # 뚜껑을 연 상자 + 안의 탄 (둘이 같은 Z 회전을 공유)
        unrotate_z=True,
        orient=True,
        tex={"box": dict(base="Cartridges/src/textures/box4k_BaseColor.png",
                         normal="Cartridges/src/textures/box4k_Normal.png",
                         rough="Cartridges/src/textures/box4k_Roughness.png",
                         metal="Cartridges/src/textures/box4k_Metallic.png"),
             "bullet.002": dict(base="Cartridges/src/textures/BaseColor_bullet.jpg",
                                normal="Cartridges/src/textures/Normal_bullet.png")}),
    "SM_Fab_SprayCan": dict(
        fbx="SprayBottles/src/spray_paint_bottles.fbx",
        only=("spray_paint_bottles",),
        orient=True,
        tex={"spray_paint_bottles": dict(base="SprayBottles/src/tex/textures/spray_paint_bottles_diff_4k_medical.png",
                                         normal="SprayBottles/src/tex/textures/spray_paint_bottles_nor_gl_4k.exr",
                                         rough="SprayBottles/src/tex/textures/spray_paint_bottles_rough_4k.exr",
                                         metal="SprayBottles/src/tex/textures/spray_paint_bottles_metal_4k.exr")}),
    "SM_Fab_Tin": dict(
        fbx="TinBox/src/source/TinCanBox.fbx",
        close_lid=True,  # 열린 뚜껑을 몸통 위에 닫아 얹는다. 거의 정육면체라 축 정렬은 하지 않음
        orient=False,
        squash_z=0.4,  # 케이스 칸에 비해 너무 높고 원근으로 옆 칸에 걸쳐 보여 납작한 화약 통으로 누른다
        # TinCanBoxBase/Top.png는 이름과 달리 노멀 맵(평균 128,128,255). 색은 Albedo 하나를 같이 쓴다
        tex={"TinCanBoxBase": dict(base="TinBox/src/textures/TinCanBoxAlbedo.png",
                                   normal="TinBox/src/textures/TinCanBoxBase.png",
                                   rough="TinBox/src/textures/TinCanBoxRough.png",
                                   metal="TinBox/src/textures/TinCanBoxMetal.png"),
             "TinCanBoxTop": dict(base="TinBox/src/textures/TinCanBoxAlbedo.png",
                                  normal="TinBox/src/textures/TinCanBoxTop.png",
                                  rough="TinBox/src/textures/TinCanBoxRough.png",
                                  metal="TinBox/src/textures/TinCanBoxMetal.png")}),
}


def bake_world(obj):
    """부모(아머처 등)와 회전·크기를 메시 정점에 굽고 부모를 끊는다"""
    mw = obj.matrix_world.copy()
    obj.parent = None
    obj.data.transform(mw)
    obj.matrix_world = Matrix.Identity(4)


def bounds(obj):
    vs = [v.co for v in obj.data.vertices]
    lo = Vector((min(v.x for v in vs), min(v.y for v in vs), min(v.z for v in vs)))
    hi = Vector((max(v.x for v in vs), max(v.y for v in vs), max(v.z for v in vs)))
    return lo, hi


def material(name, spec):
    mat = bpy.data.materials.new(name)
    mat.use_nodes = True
    nt = mat.node_tree
    bsdf = nt.nodes["Principled BSDF"]

    def img(rel, non_color):
        n = nt.nodes.new("ShaderNodeTexImage")
        n.image = bpy.data.images.load(os.path.join(ROOT, rel))
        if non_color:
            n.image.colorspace_settings.name = "Non-Color"
        return n
    nt.links.new(img(spec["base"], False).outputs["Color"], bsdf.inputs["Base Color"])
    if "rough" in spec:
        nt.links.new(img(spec["rough"], True).outputs["Color"], bsdf.inputs["Roughness"])
    if "metal" in spec:
        nt.links.new(img(spec["metal"], True).outputs["Color"], bsdf.inputs["Metallic"])
    if "normal" in spec:
        nm = nt.nodes.new("ShaderNodeNormalMap")
        nt.links.new(img(spec["normal"], True).outputs["Color"], nm.inputs["Color"])
        nt.links.new(nm.outputs["Normal"], bsdf.inputs["Normal"])
    return mat


for name, cfg in ASSETS.items():
    bpy.ops.wm.read_factory_settings(use_empty=True)
    bpy.ops.import_scene.fbx(filepath=os.path.join(ROOT, cfg["fbx"]))
    meshes = [o for o in bpy.context.scene.objects if o.type == "MESH"]
    if "only" in cfg:
        meshes = [o for o in meshes if o.name in cfg["only"]]
    if "exclude" in cfg:
        meshes = [o for o in meshes if not o.name.lower().startswith(cfg["exclude"])]
    for o in [o for o in bpy.context.scene.objects if o not in meshes]:
        bpy.data.objects.remove(o, do_unlink=True)

    if cfg.get("unrotate_z"):
        # 상자와 탄이 같은 Z 회전으로 비스듬히 놓여 있어 그 회전을 먼저 되돌린다
        angle = meshes[0].rotation_euler.z
        for o in meshes:
            o.rotation_euler.z -= angle
            o.location = Matrix.Rotation(-angle, 4, "Z") @ o.location
        bpy.context.view_layer.update()  # matrix_world를 새 회전으로 갱신해야 bake_world가 맞는 값을 굽는다
    if cfg.get("close_lid"):
        base = next(o for o in meshes if "Base" in o.name)
        top = next(o for o in meshes if "Top" in o.name)
        top.rotation_euler = base.rotation_euler.copy()
        bpy.context.view_layer.update()
    for o in meshes:
        bake_world(o)
    if cfg.get("close_lid"):
        blo, bhi = bounds(base)
        tlo, thi = bounds(top)
        shift = Vector(((blo.x + bhi.x) / 2 - (tlo.x + thi.x) / 2,
                        (blo.y + bhi.y) / 2 - (tlo.y + thi.y) / 2,
                        bhi.z - (thi.z - tlo.z) * 0.6 - tlo.z))
        top.data.transform(Matrix.Translation(shift))

    if cfg.get("trim"):
        o = meshes[0]
        lo, hi = bounds(o)
        bm = bmesh.new()
        bm.from_mesh(o.data)
        bmesh.ops.bisect_plane(bm, geom=bm.verts[:] + bm.edges[:] + bm.faces[:], clear_outer=True,
                               plane_co=(lo.x + (hi.x - lo.x) * cfg["trim"], 0.0, 0.0), plane_no=(1.0, 0.0, 0.0))
        caps = bmesh.ops.holes_fill(bm, edges=[e for e in bm.edges if e.is_boundary], sides=0)["faces"]
        uv = bm.loops.layers.uv.active
        for f in caps:
            for loop in f.loops:
                loop[uv].uv = cfg["trim_uv"]
        bm.to_mesh(o.data)
        bm.free()
        print("[trim]", name, len(caps), "faces capped")
    if cfg.get("grid"):
        cols, rows = cfg["grid"]
        base = meshes[0]
        lo, hi = bounds(base)
        base.data.transform(Matrix.Translation(-(lo + hi) / 2))  # 탄 가운데를 원점으로
        size = hi - lo
        copies = []
        for c in range(cols):
            for r in range(rows):
                o = base.copy()
                o.data = base.data.copy()
                bpy.context.scene.collection.objects.link(o)
                turn = Matrix.Rotation(3.14159265 if c == 1 else 0.0, 4, "Z")
                o.data.transform(Matrix.Translation(Vector(((c - (cols - 1) / 2) * (size.x + 0.003),
                                                            (r - (rows - 1) / 2) * size.y, 0.0))) @ turn)
                copies.append(o)
        bpy.data.objects.remove(base, do_unlink=True)
        meshes = copies

    # 슬롯마다 텍스처 머티리얼로 바꿔 끼운다
    made = {}
    for o in meshes:
        for slot in o.material_slots:
            key = slot.material.name.split(".")[0] if slot.material.name not in cfg["tex"] else slot.material.name
            if key not in cfg["tex"]:
                key = slot.material.name
            if key in cfg["tex"]:
                if key not in made:
                    made[key] = material(name + "_" + key.replace(".", "_"), cfg["tex"][key])
                slot.material = made[key]

    bpy.ops.object.select_all(action="DESELECT")
    for o in meshes:
        o.select_set(True)
    bpy.context.view_layer.objects.active = meshes[0]
    if len(meshes) > 1:
        bpy.ops.object.join()
    obj = bpy.context.view_layer.objects.active

    if cfg["orient"]:
        lo, hi = bounds(obj)
        size = hi - lo
        order = sorted(range(3), key=lambda i: -size[i])  # 긴 축 → X, 중간 → Y, 짧은 축 → Z
        m = Matrix(((0.0,) * 3,) * 3)
        for new_axis, old_axis in enumerate(order):
            m[new_axis][old_axis] = 1.0
        if m.determinant() < 0:
            m[1] = -m[1]  # 거울상이 되지 않게 Y를 뒤집어 회전으로 만든다
        obj.data.transform(m.to_4x4())

    if cfg.get("squash_z"):
        obj.data.transform(Matrix.Diagonal((1.0, 1.0, cfg["squash_z"], 1.0)))
    lo, hi = bounds(obj)
    obj.data.transform(Matrix.Translation(Vector((-(lo.x + hi.x) / 2, -(lo.y + hi.y) / 2, -lo.z))))
    obj.name = obj.data.name = name
    path = os.path.join(OUT, name + ".fbx")
    bpy.ops.export_scene.fbx(filepath=path, use_selection=True, path_mode="COPY", embed_textures=True,
                             apply_scale_options="FBX_SCALE_UNITS", mesh_smooth_type="FACE")
    lo, hi = bounds(obj)
    print("[prop]", name, tuple(round(v, 3) for v in (hi - lo)), [s.material.name for s in obj.material_slots])
