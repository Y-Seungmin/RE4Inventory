"""
Fab Model12 산탄총 colour 텍스처의 나무(주황) 부분만 채도·밝기를 낮춰 짙은 호두나무색으로 만든다. 금속(무채색)은 그대로.
원본은 두고 *_muted.png로 저장한다. prep_fab_props.py가 이 파일을 쓴다.
  python -X utf8 -I Tools/mute_shotgun_wood.py <4k_Shotgun_colour.png> <출력 png>
"""
import sys
import numpy as np
from PIL import Image

SAT, VAL = 0.55, 0.62  # 나무 부분 채도·밝기 배율

src, out = sys.argv[1:3]
im = Image.open(src)
alpha = im.getchannel("A") if im.mode == "RGBA" else None
hsv = np.asarray(im.convert("RGB").convert("HSV")).astype(np.float32)
h, s, v = hsv[..., 0], hsv[..., 1], hsv[..., 2]
# PIL HSV의 색상은 0~255. 빨강~주황(0~40도)이면서 채도가 있는 픽셀만
wood = (h < 40 / 360 * 255) & (s > 80)
h[wood] += 5  # 채도를 낮추면 붉은 기가 분홍으로 보여 갈색 쪽으로 살짝 민다
s[wood] *= SAT
v[wood] *= VAL
res = Image.fromarray(np.clip(hsv, 0, 255).astype(np.uint8), "HSV").convert("RGB")
if alpha is not None:
    res.putalpha(alpha)
res.save(out)
assert wood.mean() > 0.05, "나무 픽셀이 너무 적음: 색상 범위가 틀렸을 수 있음"
print("[wood]", out, f"{wood.mean():.1%} 픽셀 조정")
