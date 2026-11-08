"""
Fab 스프레이 캔(Houdini "Spray Paint Bottles", CC-BY) diff 텍스처의 페인트 라벨을 독자 디자인 구급 스프레이 라벨로 덮어 그린다.
원본은 건드리지 않고 같은 폴더에 *_medical.png로 저장한다. prep_fab_props.py가 이 파일을 쓴다.
  python -X utf8 -I Tools/make_spray_label.py <spray_paint_bottles_diff_4k.jpg> <출력 png>
"""
import sys
from PIL import Image, ImageChops, ImageDraw, ImageFont

src, out = sys.argv[1:3]
im = Image.open(src).convert("RGB")
W, H = im.size
s = W / 4096
# 라벨이 차지하는 UV 영역 (4k 기준). 가로 = 원통 둘레, 세로 = 캔 길이. 왼쪽 끝 이음매 띠까지 덮는다
L, T, R, B = (int(v * s) for v in (0, 1820, 3080, 4096))

PAPER = (226, 224, 214)
GREEN = (38, 112, 70)
DARK = (28, 34, 30)


def font(size):
    for name in ("arialbd.ttf", "segoeuib.ttf", "DejaVuSans-Bold.ttf"):
        try:
            return ImageFont.truetype(name, int(size * s))
        except OSError:
            pass
    return ImageFont.load_default()


def text_at(d, x, cy, text, f, fill):
    box = d.textbbox((0, 0), text, font=f)
    d.text((x - box[0], cy - (box[3] - box[1]) / 2 - box[1]), text, font=f, fill=fill)


# 캔은 케이스 안에 눕혀 놓이므로 글자가 캔 길이 방향으로 읽히게, 가로로 긴 패널을 그려 90도 돌려 붙인다.
# 위에서 둘레의 절반쯤이 보이므로 둘레를 두 패널로 나눠 같은 그림을 반복한다
pw, ph = (R - L) // 2, B - T  # 패널 하나: 둘레 절반 x 캔 길이
strip = Image.new("RGB", (R - L, ph), PAPER)
for i in range(2):
    panel = Image.new("RGB", (ph, pw), PAPER)  # 돌리기 전: 가로 = 캔 길이, 세로 = 둘레 절반
    d = ImageDraw.Draw(panel)
    band = int(110 * s)
    d.rectangle((0, 0, band, pw), fill=GREEN)  # 캔 양 끝 띠
    d.rectangle((ph - band, 0, ph, pw), fill=GREEN)
    d.rectangle((0, 0, ph, int(40 * s)), fill=GREEN)  # 둘레 방향 가는 테두리
    d.rectangle((0, pw - int(40 * s), ph, pw), fill=GREEN)
    # 왼쪽: 녹색 십자 (적십자 표장과 구분되는 녹색)
    cx, cy, arm, thick = band + int(320 * s), pw // 2, int(230 * s), int(80 * s)
    d.rectangle((cx - arm, cy - thick, cx + arm, cy + thick), fill=GREEN)
    d.rectangle((cx - thick, cy - arm, cx + thick, cy + arm), fill=GREEN)
    # 오른쪽: 글자. 위에서는 둘레 가운데 좁은 띠만 잘 보여 "MEDICAL" 한 줄을 남은 폭에 꽉 차게 키운다
    x = cx + arm + int(90 * s)
    room = ph - band - int(90 * s) - x
    size = 400
    while size > 100 and d.textlength("MEDICAL", font=font(size)) > room:
        size -= 10
    text_at(d, x, cy - int(size * 0.85), "FIRST AID", font(size * 0.42), GREEN)
    text_at(d, x, cy, "MEDICAL", font(size), DARK)
    text_at(d, x, cy + int(size * 0.85), "SPRAY  •  150 ml", font(size * 0.42), DARK)
    strip.paste(panel.rotate(90, expand=True), (i * pw, 0))
# 캔을 눕혔을 때 위로 오는 쪽이 패널 경계 근처라, 둘레 방향으로 반 패널 밀어 패널 가운데가 위로 오게 한다
im.paste(ImageChops.offset(strip, pw // 2, 0), (L, T))
im.save(out)
print("[label]", out, im.size)
