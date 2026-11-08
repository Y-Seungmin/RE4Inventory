"""
에디터 PIE 시연 영상용 입력 드라이버. 커서를 목표까지 부드럽게 옮긴 뒤 클릭한다
(게임은 마우스 이동 이벤트로 커서 위치를 갱신하므로 절대 좌표 클릭만으로는 안 먹는다).
  python -X utf8 Tools/demo_drive.py <timeline.json>
timeline: [{"t":초, "move":[x,y], "dur":초} | {"t":초, "click":[x,y]} | {"t":초, "key":"r"}]  좌표는 화면 픽셀
"""
import ctypes, json, sys, time

u = ctypes.windll.user32
u.SetProcessDPIAware()
MOUSEEVENTF_MOVE, LEFTDOWN, LEFTUP = 0x1, 0x2, 0x4
KEYUP, SCANCODE = 0x2, 0x8


def pos():
    p = ctypes.wintypes.POINT()
    u.GetCursorPos(ctypes.byref(p))
    return p.x, p.y


def glide(x, y, dur):
    x0, y0 = pos()
    n = max(1, int(dur * 60))
    for i in range(1, n + 1):
        k = i / n
        k = k * k * (3 - 2 * k)  # smoothstep
        u.SetCursorPos(int(x0 + (x - x0) * k), int(y0 + (y - y0) * k))
        u.mouse_event(MOUSEEVENTF_MOVE, 0, 0, 0, 0)
        time.sleep(dur / n)


def key(name):
    vk = ord(name.upper())
    sc = u.MapVirtualKeyW(vk, 0)
    u.keybd_event(vk, sc, 0, 0)
    time.sleep(0.05)
    u.keybd_event(vk, sc, KEYUP, 0)


import ctypes.wintypes  # noqa: E402  (POINT)

steps = json.load(open(sys.argv[1], encoding="utf-8"))
start = time.perf_counter()
for s in steps:
    while time.perf_counter() - start < s["t"]:
        time.sleep(0.005)
    if "move" in s:
        glide(*s["move"], s.get("dur", 0.6))
    elif "click" in s:
        glide(*s["click"], s.get("dur", 0.6))
        time.sleep(0.12)
        u.mouse_event(LEFTDOWN, 0, 0, 0, 0)
        time.sleep(0.06)
        u.mouse_event(LEFTUP, 0, 0, 0, 0)
    elif "key" in s:
        key(s["key"])
    print(f"[{time.perf_counter() - start:5.2f}] {s}", flush=True)
