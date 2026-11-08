"""
Fab Model12 산탄총 colour 텍스처의 낱개 탄 영역은 UV 섬 가장자리까지 색이 칠해져 있지 않아, 크림프 끝이 검은 바탕을 집어와 톱니처럼 보인다.
그 영역만 주변 색을 검은 바탕 쪽으로 번지게(UV 패딩) 해서 *_shells.png로 저장한다. prep_fab_props.py의 SM_Fab_Shells가 이 파일을 쓴다.
  python -X utf8 -I Tools/pad_shell_texture.py <4k_Shotgun_colour.png> <출력 png>
"""
import sys
import numpy as np
from PIL import Image
from scipy.ndimage import distance_transform_edt

BOX = (3040, 3060, 4096, 4096)  # 4k 기준 탄 UV 영역 (오른쪽 아래)
STEPS = 48  # 번지게 할 최대 거리(픽셀)

src, out = sys.argv[1:3]
im = Image.open(src)
s = im.size[0] / 4096
box = tuple(int(v * s) for v in BOX)
rgb = im.convert("RGB")
part = rgb.crop(box)
a = np.asarray(part).astype(np.int16)
empty = a.max(axis=2) < 12
before = empty.mean()
# 빈 픽셀마다 가장 가까운 칠해진 픽셀의 색을 가져온다 (최댓값 필터는 노랑이 빨강을 이겨 크림프에 노랑이 섞임)
dist, (iy, ix) = distance_transform_edt(empty, return_indices=True)
fill = empty & (dist <= STEPS)
a[fill] = a[iy[fill], ix[fill]]
empty = empty & ~fill
rgb.paste(Image.fromarray(a.astype(np.uint8)), box[:2])
if im.mode == "RGBA":
    rgb.putalpha(im.getchannel("A"))
rgb.save(out)
assert empty.mean() < before, "빈 픽셀이 줄지 않음"
print("[shells]", out, f"빈 픽셀 {before:.1%} -> {empty.mean():.1%}")
