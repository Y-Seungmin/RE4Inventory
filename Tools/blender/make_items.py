"""
RE4Inventory 독자 디자인 소품 생성 스크립트 (Blender 5.x, 백그라운드 실행).

원작 게임 모델을 쓰지 않고, 일반적인 형태(권총, 펌프 샷건, 탄약 상자, 허브 다발,
수류탄, 스프레이 캔, 칼, 화약 병)와 케이스 부품을 기본 도형 조합으로 만든다.

규칙
- 길이가 긴 쪽은 +X, 케이스 안에서 위에서 보이는 면은 +Z (옆모습이 위를 보게 눕힌 상태)
- 머티리얼은 이름만 붙인다. 실제 PBR 값은 UE의 ACaseViewActor::SurfaceLooks가 이름으로 정한다
- 단위는 cm 감각으로 잡고 0.01을 곱해 m로 만든다. 크기는 UE에서 칸에 맞춰 자동 조정된다

실행
  blender -b --factory-startup --python Tools/blender/make_items.py -- <출력 폴더>
"""
import bpy
import bmesh
import math
import os
import sys

OUT = sys.argv[sys.argv.index("--") + 1] if "--" in sys.argv else os.path.join(os.path.dirname(__file__), "..", "..", "ArtSource", "Meshes")
os.makedirs(OUT, exist_ok=True)
CM = 0.01


def reset():
    bpy.ops.wm.read_factory_settings(use_empty=True)


def material(name):
    mat = bpy.data.materials.get(name) or bpy.data.materials.new(name)
    return mat


def assign(obj, mat_name):
    obj.data.materials.clear()
    obj.data.materials.append(material(mat_name))


def bevel(obj, width, segments=2):
    if width <= 0:
        return obj
    mod = obj.modifiers.new("Bevel", "BEVEL")
    mod.width = width * CM
    mod.segments = segments
    mod.limit_method = "ANGLE"
    bpy.context.view_layer.objects.active = obj
    bpy.ops.object.modifier_apply(modifier=mod.name)
    return obj


def box(size, loc, mat, bev=0.0, rot=(0, 0, 0), seg=2):
    bpy.ops.mesh.primitive_cube_add(size=1.0, location=[v * CM for v in loc], rotation=[math.radians(r) for r in rot])
    obj = bpy.context.active_object
    obj.scale = [v * CM for v in size]
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
    assign(obj, mat)
    return bevel(obj, bev, seg)


def cyl(radius, depth, loc, mat, rot=(0, 0, 0), verts=24, bev=0.0):
    bpy.ops.mesh.primitive_cylinder_add(vertices=verts, radius=radius * CM, depth=depth * CM,
                                        location=[v * CM for v in loc], rotation=[math.radians(r) for r in rot])
    obj = bpy.context.active_object
    assign(obj, mat)
    return bevel(obj, bev)


def sphere(radius, loc, mat, scale=(1, 1, 1), rot=(0, 0, 0), seg=20, rings=12):
    bpy.ops.mesh.primitive_uv_sphere_add(segments=seg, ring_count=rings, radius=radius * CM,
                                         location=[v * CM for v in loc], rotation=[math.radians(r) for r in rot])
    obj = bpy.context.active_object
    obj.scale = scale
    bpy.ops.object.transform_apply(location=False, rotation=True, scale=True)
    assign(obj, mat)
    return obj


def torus(major, minor, loc, mat, rot=(0, 0, 0)):
    bpy.ops.mesh.primitive_torus_add(major_radius=major * CM, minor_radius=minor * CM, major_segments=24, minor_segments=8,
                                     location=[v * CM for v in loc], rotation=[math.radians(r) for r in rot])
    obj = bpy.context.active_object
    assign(obj, mat)
    return obj


def taper(obj, axis, start_scale, end_scale):
    """axis 방향으로 갈수록 다른 두 축을 start->end 비율로 줄이거나 늘린다 (개머리판, 손잡이용)"""
    me = obj.data
    coords = [v.co[axis] for v in me.vertices]
    lo, hi = min(coords), max(coords)
    for v in me.vertices:
        t = (v.co[axis] - lo) / max(hi - lo, 1e-6)
        s = start_scale + (end_scale - start_scale) * t
        for a in range(3):
            if a != axis and a != 2:
                v.co[a] *= s
    me.update()


def finish(name, lay_flat=True):
    """조각을 합치고, 옆모습이 위(+Z)를 보게 눕히고, 피벗을 바닥 가운데로 옮긴 뒤 FBX로 내보낸다"""
    objs = [o for o in bpy.context.scene.objects if o.type == "MESH"]
    bpy.ops.object.select_all(action="DESELECT")
    for o in objs:
        o.select_set(True)
    bpy.context.view_layer.objects.active = objs[0]
    bpy.ops.object.join()
    obj = bpy.context.active_object
    # 합치면 기준 오브젝트의 회전이 오브젝트 값으로 남는다. 눕히기 전에 메시에 적용해 둔다
    bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)
    obj.name = name
    obj.data.name = name
    if lay_flat:
        # 세워서 만든 옆모습(X 길이, Z 높이)을 X축으로 -90도 눕혀 옆면이 +Z를 보게 한다
        obj.rotation_euler = (math.radians(-90), 0, 0)
        bpy.ops.object.transform_apply(location=False, rotation=True, scale=False)
    # 피벗: 바운드 바닥 가운데
    xs = [v.co.x for v in obj.data.vertices]
    ys = [v.co.y for v in obj.data.vertices]
    zs = [v.co.z for v in obj.data.vertices]
    center = ((min(xs) + max(xs)) / 2, (min(ys) + max(ys)) / 2, min(zs))
    for v in obj.data.vertices:
        v.co.x -= center[0]
        v.co.y -= center[1]
        v.co.z -= center[2]
    obj.location = (0, 0, 0)
    bpy.ops.object.shade_auto_smooth(angle=math.radians(40))
    path = os.path.join(OUT, f"{name}.fbx")
    bpy.ops.export_scene.fbx(filepath=path, use_selection=False, object_types={"MESH"}, apply_unit_scale=True,
                             apply_scale_options="FBX_SCALE_ALL", mesh_smooth_type="FACE", add_leaf_bones=False,
                             bake_anim=False, axis_forward="-Z", axis_up="Y")
    dims = [round((max(c) - min(c)) / CM, 2) for c in (xs, ys, zs)]
    print(f"[make_items] {name}: {len(obj.data.vertices)} verts, slots={[m.name for m in obj.data.materials]}, size_cm(before lay)={dims}")


# ---------------- 아이템 ----------------

def handgun():
    reset()
    # 슬라이드, 프레임, 총열 끝, 손잡이(뒤로 기움), 방아쇠울, 방아쇠, 가늠자
    box((19, 3.0, 3.6), (0, 0, 5.4), "Gunmetal", bev=0.35)
    for i in range(7):  # 슬라이드 뒤쪽 홈
        box((0.35, 3.1, 2.2), (-7.5 + i * 0.9, 0, 5.6), "GunmetalDark")
    box((17, 2.8, 1.8), (0.5, 0, 2.9), "Polymer", bev=0.3)
    cyl(0.65, 1.2, (9.8, 0, 5.6), "GunmetalDark", rot=(0, 90, 0), verts=16)
    grip = box((3.6, 3.0, 9.5), (-6.0, 0, -1.6), "Polymer", bev=0.6, rot=(0, -14, 0))
    box((2.8, 3.12, 7.6), (-6.0, 0, -1.9), "PolymerDark", bev=0.4, rot=(0, -14, 0))  # 손잡이 패널 (같은 기울기, 안쪽)
    torus(2.3, 0.35, (-1.2, 0, 1.2), "Polymer", rot=(90, 0, 0))
    box((0.5, 0.9, 1.8), (-1.0, 0, 1.0), "GunmetalDark", rot=(0, 15, 0))
    box((0.8, 1.2, 0.6), (8.4, 0, 7.4), "GunmetalDark")
    box((0.8, 2.2, 0.7), (-8.6, 0, 7.4), "GunmetalDark")
    finish("SM_Item_Handgun")


def shotgun():
    reset()
    cyl(1.15, 46, (8, 0, 4.2), "Gunmetal", rot=(0, 90, 0), verts=24)          # 총열
    cyl(0.95, 34, (2, 0, 2.0), "GunmetalDark", rot=(0, 90, 0), verts=20)      # 탄창관
    pump = cyl(1.65, 13, (10, 0, 2.0), "Wood", rot=(0, 90, 0), verts=24, bev=0.2)
    for i in range(9):                                                       # 펌프 홈
        cyl(1.7, 0.35, (5.2 + i * 1.2, 0, 2.0), "WoodDark", rot=(0, 90, 0), verts=24)
    box((16, 3.0, 5.6), (-20, 0, 3.0), "Gunmetal", bev=0.4)                 # 기관부
    box((2.2, 0.9, 2.8), (-21, 0, 0.0), "GunmetalDark", rot=(0, 20, 0))    # 방아쇠
    torus(2.6, 0.4, (-21, 0, 0.4), "Gunmetal", rot=(90, 0, 0))
    stock = box((26, 3.2, 6.5), (-40, 0, 0.5), "Wood", bev=0.8, rot=(0, -6, 0))  # 개머리판
    taper(stock, 0, 0.75, 1.0)
    box((1.4, 3.4, 8.0), (-53.3, 0, -0.9), "Rubber", bev=0.4, rot=(0, -6, 0))   # 개머리 패드
    box((1.0, 0.8, 0.8), (30.5, 0, 5.6), "GunmetalDark")                     # 가늠쇠
    finish("SM_Item_Shotgun")


def knife():
    reset()
    blade = box((16, 0.5, 3.2), (6, 0, 0), "Steel", bev=0.15)
    me = blade.data  # 칼끝: 위쪽 앞 모서리를 아래로 모아 뾰족하게
    for v in me.vertices:
        if v.co.x > 0.12:
            v.co.z = min(v.co.z, -0.01 + (0.14 - v.co.x) * 1.4)
    me.update()
    box((0.8, 3.6, 4.6), (-2.5, 0, 0), "GunmetalDark", bev=0.15)            # 가드
    h = box((11, 2.4, 3.0), (-9, 0, -0.2), "Rubber", bev=0.7)              # 손잡이
    for i in range(5):
        box((0.5, 2.5, 3.1), (-5 - i * 2, 0, -0.2), "RubberDark", bev=0.1)
    cyl(1.0, 1.2, (-15.2, 0, -0.2), "Steel", rot=(0, 90, 0), verts=16)     # 손잡이 끝
    finish("SM_Item_Knife")


def ammo_box(name, size, label_mat, rounds_visible):
    reset()
    w, d, h = size
    box((w, d, h), (0, 0, h / 2), "Cardboard", bev=0.25)
    box((w * 1.01, d * 1.01, h * 0.12), (0, 0, h * 0.94), "CardboardDark", bev=0.1)   # 뚜껑 턱
    box((w * 0.72, d * 1.02, h * 0.42), (0, 0, h * 0.48), label_mat, bev=0.05)         # 띠 라벨
    box((w * 1.02, d * 0.35, h * 0.08), (0, 0, h * 0.5), "LabelDark")                 # 라벨 줄
    # 위에서 보이는 뚜껑 라벨: 색 판 + 검은 줄 (탄 종류를 색으로 구분)
    box((w * 0.82, d * 0.8, 0.12), (0, 0, h + 0.05), label_mat, bev=0.04)
    for i in range(rounds_visible):
        box((w * 0.62, 0.22, 0.14), (0, -d * 0.28 + i * (d * 0.56 / max(rounds_visible - 1, 1)), h + 0.08), "LabelDark")
    finish(name, lay_flat=False)


def herb(name, leaf_mat):
    reset()
    # 끈으로 묶은 짧은 잎 다발: 줄기 3개가 앞으로 퍼지고, 잎이 줄기를 따라 어긋나게 붙는다
    rng = [0.0, 0.37, 0.71, 0.13, 0.59, 0.88, 0.25, 0.5, 0.95, 0.66]
    k = 0
    for j, spread in enumerate((-16, 0, 15)):
        a = math.radians(spread)
        cyl(0.2, 7.5, (3.2 * math.cos(a) - 0.5, 3.2 * math.sin(a), 0), "Stem", rot=(0, 90, spread), verts=8)
        for i in range(3):
            t = 2.0 + i * 2.3
            side = 1 if (i + j) % 2 == 0 else -1
            la = a + math.radians(side * (28 + 10 * rng[k % 10]))
            px, py = t * math.cos(a), t * math.sin(a)
            length = 2.6 + 0.8 * rng[(k + 3) % 10]
            leaf = sphere(1.0, (px + 0.6 * length * math.cos(la), py + 0.6 * length * math.sin(la), 0.3 + 0.15 * i),
                          leaf_mat, scale=(length, 1.1, 0.28), seg=16, rings=8)
            leaf.rotation_euler = (0, 0, la)
            bpy.context.view_layer.objects.active = leaf
            bpy.ops.object.transform_apply(location=False, rotation=True, scale=False)
            me = leaf.data  # 잎 가운데 접기
            for v in me.vertices:
                v.co.z -= abs(v.co.y) * 0.3
            me.update()
            k += 1
        # 줄기 끝 잎
        tip = sphere(1.0, (8.0 * math.cos(a), 8.0 * math.sin(a), 0.5), leaf_mat, scale=(2.4, 1.2, 0.3), seg=16, rings=8)
        tip.rotation_euler = (0, 0, a)
        bpy.context.view_layer.objects.active = tip
        bpy.ops.object.transform_apply(location=False, rotation=True, scale=False)
    torus(0.75, 0.22, (-0.1, 0, 0), "Twine", rot=(0, 90, 0))   # 줄기 밑동을 묶은 끈
    torus(0.75, 0.22, (-0.7, 0, 0), "Twine", rot=(0, 90, 0))
    finish(name, lay_flat=False)


def grenade():
    reset()
    body = sphere(3.0, (0, 0, 0), "Olive", scale=(1.25, 1.0, 1.0), seg=24, rings=16)
    for i in range(5):  # 가로 홈
        torus(3.05 * math.sin(math.radians(30 + i * 30)) * 1.0, 0.18, (-3.0 + i * 1.5, 0, 0), "OliveDark", rot=(0, 90, 0))
    for i in range(4):  # 세로 홈
        torus(3.02, 0.16, (0, 0, 0), "OliveDark", rot=(0, 90, i * 45))
    cyl(1.0, 1.6, (4.2, 0, 0), "Steel", rot=(0, 90, 0), verts=16)           # 신관 머리
    box((6.0, 1.1, 0.4), (2.4, 0, 2.0), "Steel", bev=0.1, rot=(0, -12, 0))   # 안전 손잡이
    torus(1.2, 0.18, (4.4, 0, -1.6), "Steel", rot=(90, 0, 0))               # 핀 고리
    finish("SM_Item_Grenade")


def spray():
    reset()
    cyl(2.4, 13, (0, 0, 0), "Paint", rot=(0, 90, 0), verts=32, bev=0.3)     # 캔 몸통
    cyl(2.45, 4.5, (-1.5, 0, 0), "LabelGreen", rot=(0, 90, 0), verts=32)     # 초록 띠 (적십자 표장은 쓰지 않음)
    cyl(2.46, 0.5, (1.2, 0, 0), "PaintDark", rot=(0, 90, 0), verts=32)
    sphere(2.3, (6.5, 0, 0), "Steel", scale=(0.45, 1.0, 1.0), seg=24, rings=10)   # 어깨
    cyl(1.0, 2.0, (8.0, 0, 0), "PlasticWhite", rot=(0, 90, 0), verts=16)     # 노즐 캡
    box((0.8, 0.8, 0.8), (8.6, 0, 1.0), "PlasticWhite")
    cyl(2.35, 0.6, (-6.6, 0, 0), "Steel", rot=(0, 90, 0), verts=32)          # 바닥 테
    finish("SM_Item_Spray")


def gunpowder():
    reset()
    # 옆으로 눕힌 병 (길이 X). 위에서 보면 병 옆모습과 내용물이 보인다
    cyl(2.2, 4.5, (0, 0, 0), "Glass", rot=(0, 90, 0), verts=24, bev=0.2)       # 병
    cyl(1.9, 3.6, (-0.2, 0, -0.15), "Powder", rot=(0, 90, 0), verts=24)       # 내용물
    cyl(2.3, 1.2, (2.8, 0, 0), "Brass", rot=(0, 90, 0), verts=24, bev=0.15)   # 뚜껑
    finish("SM_Item_Gunpowder", lay_flat=False)


# ---------------- 케이스 부품 (크기 독립, C++가 배치·늘림) ----------------

def bevel_panel():
    """모서리를 둥글린 1x1x1 판. 늘려 쓰면 모서리가 같이 늘어나므로 얇은 축 방향으로만 늘려 쓴다"""
    reset()
    box((100, 100, 100), (0, 0, 50), "Leather", bev=6, seg=3)
    finish("SM_Case_BevelBox", lay_flat=False)


def corner_guard():
    reset()
    b = box((4, 4, 4), (0, 0, 2), "Brass", bev=1.4, seg=3)
    finish("SM_Case_Corner", lay_flat=False)


def latch():
    reset()
    box((5.0, 1.0, 3.4), (0, 0, 1.7), "Brass", bev=0.3)
    box((3.4, 1.4, 1.6), (0, 0.3, 2.0), "BrassDark", bev=0.25)
    cyl(0.35, 4.6, (0, 0.2, 3.5), "Steel", rot=(0, 90, 0), verts=12)
    finish("SM_Case_Latch", lay_flat=False)


def handle():
    reset()
    bpy.ops.curve.primitive_bezier_curve_add()
    curve = bpy.context.active_object
    pts = curve.data.splines[0].bezier_points
    pts[0].co = (-7 * CM, 0, 0)
    pts[0].handle_left = (-8 * CM, 0, 0)
    pts[0].handle_right = (-6 * CM, 0, 3.5 * CM)
    pts[1].co = (7 * CM, 0, 0)
    pts[1].handle_left = (6 * CM, 0, 3.5 * CM)
    pts[1].handle_right = (8 * CM, 0, 0)
    curve.data.bevel_depth = 0.9 * CM
    curve.data.bevel_resolution = 4
    bpy.ops.object.convert(target="MESH")
    assign(bpy.context.active_object, "LeatherDark")
    box((2.2, 1.6, 1.6), (-7.2, 0, 0.4), "Brass", bev=0.3)
    box((2.2, 1.6, 1.6), (7.2, 0, 0.4), "Brass", bev=0.3)
    finish("SM_Case_Handle", lay_flat=False)


def hinge():
    reset()
    cyl(0.6, 6, (0, 0, 0.6), "Steel", rot=(0, 90, 0), verts=16)
    box((5.6, 2.0, 0.3), (0, 1.0, 0.5), "Brass", bev=0.1)
    finish("SM_Case_Hinge", lay_flat=False)


if __name__ == "__main__":
    handgun()
    shotgun()
    knife()
    ammo_box("SM_Item_AmmoSmall", (4.4, 9.2, 3.0), "LabelYellow", 5)
    ammo_box("SM_Item_AmmoShells", (9.6, 9.6, 4.2), "LabelRed", 4)
    herb("SM_Item_HerbGreen", "LeafGreen")
    herb("SM_Item_HerbRed", "LeafRed")
    grenade()
    spray()
    gunpowder()
    bevel_panel()
    corner_guard()
    latch()
    handle()
    hinge()
    print("[make_items] done ->", os.path.abspath(OUT))
