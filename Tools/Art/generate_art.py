"""YJJDefense 아트 생성기.

방향: 냥코 대전쟁처럼 귀엽지만 엉뚱한 그림 — 굵은 먹선, 흰 몸통, 점 눈, 멍한 표정, 막대 팔다리.
모든 그림을 코드로 그린다 (4배 크게 그린 뒤 줄여서 매끈한 선). 외부 이미지 없음.

실행 (Tools/Art/BuildArt.ps1이 대신 해 준다):
    Tools/Art/.venv/Scripts/python.exe Tools/Art/generate_art.py <출력 폴더> <맵 inl 경로>

스프라이트는 화면 표시 크기의 2배로 만든다 (1080p보다 큰 화면에서도 선명하게).
"""

import math
import os
import random
import sys

from PIL import Image, ImageDraw, ImageFilter

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import map_data  # noqa: E402

SS = 4
INK = (36, 30, 40, 255)
WHITE = (255, 255, 255, 255)
SHADE = (226, 224, 234, 255)


def rgba(value, alpha=255):
    value = value.lstrip("#")
    return (int(value[0:2], 16), int(value[2:4], 16), int(value[4:6], 16), alpha)


RED = rgba("ff5a4f")
ORANGE = rgba("ff8a3c")
YELLOW = rgba("ffd84a")
GREEN = rgba("7fd35a")
MINT = rgba("5fdcb4")
SKY = rgba("7fd6ff")
BLUE = rgba("4f8dff")
PURPLE = rgba("9a78ff")
PINK = rgba("ff9ec4")
TAN = rgba("ecca8c")
BROWN = rgba("a8703c")
STONE = rgba("e6e3ee")
GRAY = rgba("b8bfca")
DARK = rgba("5d5a6b")


class Art:
    """굵은 먹선 도형을 그리는 작은 캔버스. 좌표는 완성 크기 기준 픽셀."""

    def __init__(self, w, h, ss=SS, lw=5.0):
        self.w, self.h, self.ss, self.lw = w, h, ss, lw
        self.img = Image.new("RGBA", (int(w * ss), int(h * ss)), (0, 0, 0, 0))
        self.d = ImageDraw.Draw(self.img)

    def _p(self, pts):
        return [(x * self.ss, y * self.ss) for x, y in pts]

    def _w(self, lw):
        return max(int(round((self.lw if lw is None else lw) * self.ss)), 1)

    def ellipse(self, cx, cy, rx, ry, fill=WHITE, outline=INK, lw=None):
        box = [(cx - rx) * self.ss, (cy - ry) * self.ss, (cx + rx) * self.ss, (cy + ry) * self.ss]
        if outline is None or lw == 0:
            self.d.ellipse(box, fill=fill)
        else:
            self.d.ellipse(box, fill=fill, outline=outline, width=self._w(lw))

    def dot(self, cx, cy, r, color=INK):
        self.ellipse(cx, cy, r, r, fill=color, outline=None)

    def poly(self, pts, fill=WHITE, outline=INK, lw=None):
        p = self._p(pts)
        if fill is not None:
            self.d.polygon(p, fill=fill)
        if outline is not None and lw != 0:
            width = self._w(lw)
            self.d.line(p + [p[0], p[1]], fill=outline, width=width, joint="curve")

    def line(self, pts, color=INK, lw=None, cap=True):
        p = self._p(pts)
        width = self._w(lw)
        self.d.line(p, fill=color, width=width, joint="curve")
        if cap:
            r = width / 2.0
            for x, y in (p[0], p[-1]):
                self.d.ellipse([x - r, y - r, x + r, y + r], fill=color)

    def blob(self, cx, cy, rx, ry, angle=0.0, fill=WHITE, outline=INK, lw=None, power=2.0, n=56):
        """기울인 타원 (power가 크면 네모에 가까워진다)."""
        ca, sa = math.cos(math.radians(angle)), math.sin(math.radians(angle))
        pts = []
        for i in range(n):
            t = 2 * math.pi * i / n
            c, s = math.cos(t), math.sin(t)
            x = rx * math.copysign(abs(c) ** (2.0 / power), c)
            y = ry * math.copysign(abs(s) ** (2.0 / power), s)
            pts.append((cx + x * ca - y * sa, cy + x * sa + y * ca))
        self.poly(pts, fill=fill, outline=outline, lw=lw)

    def rrect(self, x0, y0, x1, y1, r, fill=WHITE, outline=INK, lw=None):
        box = [x0 * self.ss, y0 * self.ss, x1 * self.ss, y1 * self.ss]
        if outline is None or lw == 0:
            self.d.rounded_rectangle(box, radius=r * self.ss, fill=fill)
        else:
            self.d.rounded_rectangle(box, radius=r * self.ss, fill=fill, outline=outline, width=self._w(lw))

    def chord(self, cx, cy, rx, ry, a0, a1, fill=WHITE, outline=INK, lw=None):
        box = [(cx - rx) * self.ss, (cy - ry) * self.ss, (cx + rx) * self.ss, (cy + ry) * self.ss]
        self.d.chord(box, a0, a1, fill=fill, outline=outline, width=self._w(lw))

    def arc(self, cx, cy, rx, ry, a0, a1, color=INK, lw=None):
        box = [(cx - rx) * self.ss, (cy - ry) * self.ss, (cx + rx) * self.ss, (cy + ry) * self.ss]
        self.d.arc(box, a0, a1, fill=color, width=self._w(lw))

    def cloud(self, circles, fill=WHITE, outline=INK, lw=None):
        """원 여러 개를 한 덩어리로 (먹선은 바깥 테두리에만)."""
        grow = self.lw if lw is None else lw
        for cx, cy, r in circles:
            self.dot(cx, cy, r + grow, outline)
        for cx, cy, r in circles:
            self.dot(cx, cy, r, fill)

    def star(self, cx, cy, r_out, r_in, points, fill=YELLOW, outline=INK, lw=None, rot=-90.0):
        pts = []
        for i in range(points * 2):
            r = r_out if i % 2 == 0 else r_in
            t = math.radians(rot) + math.pi * i / points
            pts.append((cx + r * math.cos(t), cy + r * math.sin(t)))
        self.poly(pts, fill=fill, outline=outline, lw=lw)

    def eyes(self, cx, cy, gap=13.0, r=4.5, look=0.0):
        self.dot(cx - gap + look, cy, r)
        self.dot(cx + gap + look, cy, r)

    def result(self):
        # 알파를 미리 곱한 상태로 줄여야 투명한 가장자리가 검게 번지지 않는다
        return self.img.convert("RGBa").resize((int(self.w), int(self.h)), Image.LANCZOS).convert("RGBA")


def legs(a, x0, x1, y_top, y_foot, frame, stride=7.0, lw=6.0, foot=8.0):
    """막대 다리 두 개. frame마다 앞뒤가 바뀐다."""
    s = stride if frame % 2 == 0 else -stride
    for x, d in ((x0, s), (x1, -s)):
        a.line([(x, y_top), (x + d, y_foot)], lw=lw)
        a.ellipse(x + d + 3, y_foot, foot, foot * 0.5, fill=INK, outline=None)


# ---------------------------------------------------------------------------------------------
# 몬스터 (오른쪽을 본다. 2프레임 걷기)

def make_imp(frame):
    a = Art(112, 124)
    bob = 0 if frame == 0 else -3
    legs(a, 46, 66, 94 + bob, 116, frame)
    a.poly([(49, 36 + bob), (59, 8 + bob), (70, 36 + bob)], fill=YELLOW)          # 외뿔
    arm = 6 if frame == 0 else -4
    a.line([(22, 68 + bob), (9, 80 + arm)], lw=6)
    a.line([(90, 68 + bob), (103, 80 - arm)], lw=6)
    a.ellipse(56, 64 + bob, 38, 35)                                                   # 찹쌀떡 몸
    a.eyes(62, 58 + bob, gap=13, r=4.5)
    a.ellipse(66, 73 + bob, 4.5, 5.5, fill=INK, outline=None)                         # 멍하게 벌린 입
    a.poly([(56, 78 + bob), (59, 85 + bob), (62, 78 + bob)], fill=WHITE, lw=2.5)      # 덧니
    return a.result()


def make_chicken(frame):
    a = Art(116, 116)
    bob = 0 if frame == 0 else -2
    if frame == 0:
        a.line([(46, 76), (30, 104)], lw=5)
        a.line([(62, 76), (84, 98)], lw=5)
        a.line([(22, 104), (38, 104)], lw=5)
        a.line([(80, 100), (94, 104)], lw=5)
    else:
        a.line([(46, 76), (62, 104)], lw=5)
        a.line([(62, 76), (42, 96)], lw=5)
        a.line([(56, 104), (72, 104)], lw=5)
        a.line([(30, 96), (44, 100)], lw=5)
    a.poly([(20, 44 + bob), (4, 34 + bob), (10, 52 + bob), (2, 60 + bob), (22, 62 + bob)], fill=WHITE)   # 꼬리
    a.cloud([(60, 20 + bob, 8), (72, 16 + bob, 9), (84, 22 + bob, 8)], fill=RED)                          # 볏
    a.poly([(92, 44 + bob), (112, 52 + bob), (92, 60 + bob)], fill=YELLOW)                                # 부리
    a.blob(56, 52 + bob, 38, 29, angle=-12)
    a.dot(78, 44 + bob, 4.5)
    a.line([(68, 34 + bob), (86, 39 + bob)], lw=4)                                                       # 비장한 눈썹
    a.poly([(40, 50 + bob), (26, 60 + bob), (46, 66 + bob)], fill=SHADE, lw=4)                            # 날개
    return a.result()


def make_potato(frame):
    a = Art(148, 152)
    bob = 0 if frame == 0 else -3
    legs(a, 60, 88, 124 + bob, 144, frame, stride=5, lw=7, foot=9)
    flex = 0 if frame == 0 else 5
    for side in (-1, 1):
        cx = 74 + side * 52
        a.ellipse(cx + side * 4, 40 + bob + flex, 10, 15, fill=TAN)       # 팔뚝
        a.ellipse(cx + side * 4, 23 + bob + flex, 10, 10, fill=TAN)       # 주먹
        a.ellipse(cx, 66 + bob, 17, 16, fill=TAN)                          # 알통
    a.blob(74, 82 + bob, 44, 52, fill=TAN, power=2.4)
    for x, y, r in ((52, 110, 5), (96, 60, 4), (92, 118, 6), (50, 56, 3)):
        a.ellipse(x, y + bob, r, r * 0.7, fill=rgba("cfa765"), outline=None)
    a.eyes(76, 76 + bob, gap=12, r=4)
    a.line([(58, 64 + bob), (72, 69 + bob)], lw=4)
    a.line([(94, 64 + bob), (80, 69 + bob)], lw=4)
    a.line([(68, 92 + bob), (86, 92 + bob)], lw=4)
    return a.result()


def make_toast(frame):
    a = Art(136, 116)
    wing = -34 if frame == 0 else 16
    a.blob(24, 54 + wing * 0.3, 22, 11, angle=wing, fill=WHITE)
    a.blob(112, 54 + wing * 0.3, 22, 11, angle=-wing, fill=WHITE)

    def bread(inset, color):
        a.ellipse(52, 34, 22 - inset, 20 - inset, fill=color, outline=None)
        a.ellipse(84, 34, 22 - inset, 20 - inset, fill=color, outline=None)
        a.rrect(32 + inset, 30 + inset, 104 - inset, 100 - inset, 12, fill=color, outline=None)

    bread(0, INK)
    bread(5, rgba("c98a4a"))
    bread(12, rgba("fbe9bd"))
    a.rrect(74, 36, 96, 52, 4, fill=YELLOW, lw=3)                 # 버터
    a.eyes(64, 66, gap=13, r=4.2)
    a.arc(59, 76, 6, 6, 0, 180, lw=3.5)
    a.arc(71, 76, 6, 6, 0, 180, lw=3.5)
    return a.result()


def make_mushroom(frame):
    a = Art(128, 140)
    bob = 0 if frame == 0 else -3
    legs(a, 54, 74, 116 + bob, 132, frame, stride=5)
    up = (-1, 1) if frame == 0 else (1, -1)
    for side, lift in zip((-1, 1), up):
        hx, hy = 64 + side * 48, 84 + bob + lift * 20
        a.line([(64 + side * 24, 92 + bob), (hx, hy)], lw=6)
        a.cloud([(hx - 7, hy, 9), (hx + 7, hy, 9), (hx, hy - 8, 9), (hx, hy + 7, 8)], fill=PINK, lw=4)   # 응원 수술
    a.ellipse(64, 94 + bob, 27, 30)
    a.chord(64, 62 + bob, 52, 46, 180, 360, fill=RED)
    for x, y, r in ((40, 42, 8), (70, 30, 10), (94, 46, 7)):
        a.ellipse(x, y + bob, r, r * 0.8, fill=WHITE, outline=None)
    a.eyes(66, 88 + bob, gap=11, r=4)
    a.arc(66, 96 + bob, 8, 7, 10, 170, lw=3.5)
    return a.result()


def make_pudding_king(frame):
    a = Art(264, 264)
    sq = 0 if frame == 0 else 8
    legs(a, 108, 156, 220, 250, frame, stride=6, lw=8, foot=12)
    a.line([(62 - sq, 160), (30, 190 - sq)], lw=8)
    a.line([(202 + sq, 160), (234, 190 - sq)], lw=8)
    top = 92 + sq
    a.poly([(56 - sq, 224), (82, top), (182, top), (208 + sq, 224), (132, 236)], fill=rgba("ffe38f"), lw=6)
    # 카라멜
    a.poly([(80, top + 6), (84, top - 12), (180, top - 12), (184, top + 6), (176, top + 30), (164, top + 10), (150, top + 38),
            (136, top + 12), (118, top + 30), (104, top + 10), (92, top + 26)], fill=rgba("a3622f"), lw=6)
    # 왕관
    cy = top - 14
    a.poly([(98, cy), (94, cy - 40), (112, cy - 22), (132, cy - 50), (152, cy - 22), (170, cy - 40), (166, cy)], fill=YELLOW, lw=6)
    for x in (112, 132, 152):
        a.dot(x, cy - 10, 5, RED)
    a.eyes(132, 158, gap=24, r=6.5)
    a.arc(132, 170, 16, 12, 20, 160, lw=5)
    a.ellipse(96, 174, 9, 5, fill=PINK, outline=None)
    a.ellipse(168, 174, 9, 5, fill=PINK, outline=None)
    return a.result()


# ---------------------------------------------------------------------------------------------
# 병졸 (콩처럼 생긴 흰 녀석들)

def bean(a, cx, cy, rx=24, ry=28, look=4.0, mouth="line"):
    a.ellipse(cx, cy, rx, ry)
    a.eyes(cx + look, cy - 2, gap=rx * 0.36, r=3.6)
    if mouth == "line":
        a.line([(cx + look - 3, cy + 9), (cx + look + 5, cy + 9)], lw=3)
    elif mouth == "o":
        a.ellipse(cx + look + 1, cy + 10, 3.5, 4.5, fill=INK, outline=None)


def make_soldier(frame):
    a = Art(96, 108)
    lean = 4 if frame == 2 else 0
    legs(a, 38, 54, 80, 100, frame if frame < 2 else 0, stride=6 if frame < 2 else 9)
    if frame == 2:
        a.line([(62, 58), (94, 54)], lw=8)                        # 찌르기
        a.line([(64, 58), (92, 54.5)], color=rgba("e4ebf2"), lw=3)
    else:
        a.line([(64, 66), (82, 36)], lw=8)
        a.line([(65, 64), (81, 38)], color=rgba("e4ebf2"), lw=3)
    bean(a, 46 + lean, 56, 24, 27, look=5, mouth="line" if frame < 2 else "o")
    a.chord(46 + lean, 42, 27, 24, 180, 360, fill=GRAY)          # 냄비 투구
    a.line([(18 + lean, 42), (76 + lean, 42)], lw=5)
    a.dot(46 + lean, 18, 5, RED)
    a.ellipse(24 + lean, 68, 15, 15, fill=SKY)                    # 방패
    a.dot(24 + lean, 68, 4, WHITE)
    return a.result()


def make_wizard(frame):
    a = Art(200, 320)
    # 돌기둥
    a.rrect(62, 182, 138, 314, 10, fill=STONE)
    for y in (220, 256, 290):
        a.line([(66, y), (134, y)], color=rgba("c9c5d6"), lw=3, cap=False)
    a.rrect(48, 168, 152, 194, 9, fill=WHITE)
    # 지팡이
    if frame == 0:
        staff = [(56, 170), (60, 74)]
    else:
        staff = [(60, 166), (34, 78)]
    a.line(staff, color=INK, lw=9)
    a.line(staff, color=BROWN, lw=4)
    ox, oy = staff[1]
    a.ellipse(ox, oy - 8, 14, 14, fill=SKY)
    a.dot(ox - 4, oy - 12, 4, WHITE)
    # 몸 + 수염 + 모자
    a.ellipse(104, 140, 32, 34)
    a.cloud([(86, 158, 9), (98, 164, 10), (112, 164, 10), (124, 157, 9)], fill=WHITE, lw=4)
    a.eyes(110, 132, gap=11, r=3.8)
    a.poly([(70, 116), (106, 30), (124, 56), (140, 116)], fill=PURPLE)
    a.ellipse(104, 116, 46, 10, fill=PURPLE)
    a.star(108, 84, 11, 5, 5, lw=3)
    return a.result()


# ---------------------------------------------------------------------------------------------
# 타워 (레벨 1~3). 240x300, 바닥 중심이 건설 자리

def flag(a, x, y_base, height, color=RED):
    a.line([(x, y_base), (x, y_base - height)], lw=5)
    a.poly([(x, y_base - height), (x + 30, y_base - height + 11), (x, y_base - height + 22)], fill=color, lw=4)


def make_tower_archer(level):
    a = Art(240, 300)
    hh = 104 + level * 18
    top = 276 - hh
    a.rrect(74, top, 166, 282, 12, fill=WHITE)
    a.rrect(108, 232, 132, 282, 10, fill=BROWN)
    a.rrect(110, top + 34, 130, top + 60, 8, fill=DARK)
    if level >= 1:
        a.rrect(74, top + 70, 166, top + 84, 0, fill=SKY, lw=4)
    # 망루 위 궁수 (아래 절반은 난간에 가린다)
    bean(a, 120, top - 34, 22, 24, look=5)
    a.poly([(98, top - 50), (118, top - 78), (142, top - 50)], fill=GREEN)
    a.arc(150, top - 34, 12, 26, -80, 80, lw=5)
    a.line([(152, top - 60), (152, top - 8)], lw=2.5, cap=False)
    if level >= 2:
        bean(a, 84, top - 28, 18, 20, look=4)
        a.poly([(66, top - 42), (82, top - 64), (102, top - 42)], fill=GREEN)
    trim = YELLOW if level >= 2 else WHITE
    a.rrect(58, top - 16, 182, top + 16, 8, fill=trim)
    for x in (58, 92, 126, 160):
        a.rrect(x, top - 34, x + 22, top - 10, 5, fill=trim)
    flag(a, 176, top - 30, 54)
    if level >= 1:
        flag(a, 64, top - 30, 40, color=SKY)
    return a.result()


def make_tower_barracks(level):
    a = Art(240, 300)
    wall_top = 186 - level * 10
    a.rrect(52, wall_top, 188, 284, 10, fill=WHITE)
    a.rrect(104, 226, 136, 284, 12, fill=BROWN)
    a.dot(128, 258, 3.5, YELLOW)
    a.ellipse(120, wall_top + 22, 15, 15, fill=SKY)
    a.dot(120, wall_top + 22, 4.5, WHITE)
    if level >= 1:
        a.rrect(64, 232, 90, 258, 6, fill=SKY, lw=4)
        a.rrect(150, 232, 176, 258, 6, fill=SKY, lw=4)
    peak = 104 - level * 14
    a.poly([(34, wall_top + 6), (120, peak), (206, wall_top + 6)], fill=ORANGE if level < 2 else RED)
    if level >= 2:
        a.line([(52, wall_top - 2), (120, peak + 26), (188, wall_top - 2)], color=YELLOW, lw=6, cap=False)
    flag(a, 120, peak + 4, 46, color=SKY if level == 0 else YELLOW)
    if level >= 2:
        flag(a, 60, wall_top - 10, 36)
    return a.result()


def make_tower_bombard(level):
    a = Art(240, 300)
    a.rrect(54, 214, 186, 284, 26, fill=STONE)
    for x in (80, 120, 160):
        a.dot(x, 262, 5, GRAY)
    bean(a, 78, 196, 20, 22, look=4, mouth="o")
    a.chord(78, 184, 22, 20, 180, 360, fill=DARK)
    length = 96 + level * 16
    width = 44 + level * 6
    bx, by = 118, 214
    ex, ey = bx + length * 0.52, by - length * 0.86
    a.line([(bx, by), (ex, ey)], color=INK, lw=width + 10)
    a.line([(bx, by), (ex, ey)], color=DARK, lw=width)
    a.line([(bx - 6, by - 4), (ex - 6, ey - 4)], color=rgba("8a8799"), lw=width * 0.25)
    ring = YELLOW if level >= 2 else GRAY
    a.blob(ex, ey, width * 0.62, width * 0.3, angle=31, fill=ring)
    a.blob(ex + 2, ey - 3, width * 0.36, width * 0.15, angle=31, fill=INK, outline=None)
    if level >= 1:
        a.blob(bx + length * 0.26, by - length * 0.43, width * 0.62, width * 0.2, angle=31, fill=ring, lw=4)
    a.ellipse(120, 214, 40, 20, fill=GRAY)
    return a.result()


def make_tower_mage(level):
    a = Art(240, 300)
    top = 150 - level * 14
    a.poly([(84, 284), (92, top), (148, top), (156, 284)], fill=WHITE)
    a.rrect(108, 240, 132, 284, 12, fill=PURPLE)
    a.ellipse(120, top + 40, 12, 14, fill=SKY)
    if level >= 1:
        a.rrect(76, top + 74, 164, top + 90, 6, fill=PURPLE, lw=4)
    tip = top - 100
    a.poly([(66, top + 8), (120, tip), (174, top + 8)], fill=PURPLE)
    a.star(120, top - 26, 12, 5.5, 5, lw=3)
    # 떠 있는 수정
    def crystal(cx, cy, s):
        a.poly([(cx, cy - 16 * s), (cx + 10 * s, cy), (cx, cy + 16 * s), (cx - 10 * s, cy)], fill=SKY, lw=4)
        a.line([(cx - 3 * s, cy - 6 * s), (cx + 2 * s, cy - 10 * s)], color=WHITE, lw=2.5)
    crystal(120, tip - 20, 1.0)
    if level >= 2:
        crystal(62, top - 30, 0.8)
        crystal(178, top - 30, 0.8)
    return a.result()


def make_tower_shrine(level):
    a = Art(240, 300)
    faces = (("line", RED), ("o", SKY), ("w", YELLOW))
    y = 284
    for index in range(level + 1):
        mouth, color = faces[index]
        a.rrect(60, y - 78, 180, y, 18, fill=WHITE if index % 2 == 0 else STONE)
        a.rrect(60, y - 78, 180, y - 62, 8, fill=color, lw=4)
        a.eyes(120, y - 38, gap=24, r=6.5)
        if mouth == "line":
            a.line([(106, y - 18), (134, y - 18)], lw=5)
        elif mouth == "o":
            a.ellipse(120, y - 17, 8, 9, fill=INK, outline=None)
        else:
            a.arc(111, y - 22, 9, 9, 0, 180, lw=5)
            a.arc(129, y - 22, 9, 9, 0, 180, lw=5)
        y -= 76
    a.chord(120, y - 4, 40, 26, 0, 180, fill=STONE)          # 원소 구슬을 올리는 그릇
    a.line([(78, y - 4), (162, y - 4)], lw=5)
    return a.result()


def make_build_spot():
    """빈 건설 자리: 두툼한 돌 받침 + 망치 표지판. 풀밭·흙길과 확실히 구분되게 회색 돌로."""
    a = Art(220, 156)
    cx, cy = 110, 100
    rx, ry = 92, 38
    # 바닥 그림자, 받침 옆면(두께), 윗면
    a.ellipse(cx, cy + 20, rx + 6, ry + 6, fill=rgba("6a9a4a", 110), outline=None)
    a.ellipse(cx, cy + 12, rx, ry, fill=rgba("a8a3b8"), lw=6)
    a.rrect(cx - rx, cy, cx + rx, cy + 12, 0, fill=rgba("a8a3b8"), outline=None)
    a.line([(cx - rx, cy), (cx - rx, cy + 12)], lw=6, cap=False)
    a.line([(cx + rx, cy), (cx + rx, cy + 12)], lw=6, cap=False)
    a.ellipse(cx, cy, rx, ry, fill=STONE, lw=6)
    # 포석 무늬: 안쪽 원 + 바깥 고리를 나누는 줄
    irx, iry = 50, 20
    a.ellipse(cx, cy, irx, iry, fill=rgba("f2f0f7"), outline=rgba("c4bfd3"), lw=3.5)
    for index in range(8):
        t = 2 * math.pi * (index + 0.5) / 8
        a.line([(cx + irx * math.cos(t), cy + iry * math.sin(t)), (cx + (rx - 6) * math.cos(t), cy + (ry - 4) * math.sin(t))],
               color=rgba("c4bfd3"), lw=3.5)
    a.arc(cx - 20, cy - 8, 22, 8, 190, 300, color=WHITE, lw=4)
    # 표지판: 말뚝 + 노란 판 + 망치
    a.line([(cx, cy + 2), (cx, cy - 42)], color=INK, lw=11)
    a.line([(cx, cy + 2), (cx, cy - 42)], color=BROWN, lw=5)
    a.rrect(cx - 36, 6, cx + 36, 58, 10, fill=YELLOW, lw=6)
    a.line([(cx - 14, 46), (cx + 8, 22)], color=INK, lw=9)
    a.line([(cx - 14, 46), (cx + 8, 22)], color=BROWN, lw=4)
    a.blob(cx + 9, 21, 15, 7, angle=-48, fill=GRAY, lw=4, power=4.0)
    return a.result()


# ---------------------------------------------------------------------------------------------
# 투사체

def make_arrow():
    a = Art(64, 20, lw=3)
    a.line([(4, 10), (54, 10)], color=INK, lw=6)
    a.line([(6, 10), (50, 10)], color=BROWN, lw=2.5)
    a.poly([(48, 3), (62, 10), (48, 17)], fill=GRAY, lw=3)
    a.poly([(2, 4), (14, 10), (2, 16)], fill=RED, lw=3)
    return a.result()


def make_bomb():
    a = Art(56, 60)
    a.line([(36, 18), (44, 6)], lw=5)
    a.star(46, 6, 8, 3.5, 4, fill=YELLOW, lw=2.5)
    a.ellipse(28, 34, 22, 22, fill=DARK)
    a.eyes(30, 32, gap=7, r=3)
    a.dot(18, 24, 4, rgba("8a8799"))
    return a.result()


def make_magic_bolt():
    a = Art(56, 56)
    a.star(28, 28, 24, 11, 4, fill=PURPLE, lw=4)
    a.dot(28, 28, 6, WHITE)
    return a.result()


def make_ball(kind):
    a = Art(80, 80)
    cx, cy = 40, 42
    if kind == "Fire":
        a.poly([(40, 4), (58, 26), (66, 46), (58, 66), (40, 74), (22, 66), (14, 46), (24, 30), (30, 36)], fill=ORANGE)
        a.ellipse(40, 54, 15, 15, fill=YELLOW, outline=None)
        cy = 52
    elif kind == "Ice":
        a.blob(cx, cy, 28, 28, fill=SKY, n=6, angle=30)
        a.line([(26, 30), (36, 24)], color=WHITE, lw=4)
    elif kind == "Water":
        a.poly([(40, 6), (58, 34), (62, 50), (54, 66), (40, 72), (26, 66), (18, 50), (22, 34)], fill=BLUE)
        a.dot(30, 44, 5, rgba("bcd6ff"))
        cy = 50
    elif kind == "Lightning":
        a.star(cx, cy, 34, 20, 9, fill=YELLOW)
    elif kind == "Poison":
        a.ellipse(cx, cy, 27, 27, fill=GREEN)
        a.ellipse(66, 14, 7, 7, fill=GREEN, lw=3)
        a.ellipse(14, 20, 5, 5, fill=GREEN, lw=3)
    elif kind == "Wind":
        a.ellipse(cx, cy, 27, 27, fill=MINT)
        a.arc(cx, cy, 18, 18, 200, 420, color=WHITE, lw=4)
    else:
        a.ellipse(cx, cy, 27, 27, fill=PURPLE)
        a.star(54, 26, 9, 4, 4, fill=WHITE, lw=2.5)

    if kind == "Poison":
        for ex in (cx - 9, cx + 9):
            a.line([(ex - 4, cy - 6), (ex + 4, cy + 2)], lw=3)
            a.line([(ex + 4, cy - 6), (ex - 4, cy + 2)], lw=3)
    else:
        a.eyes(cx, cy - 2, gap=9, r=3.6)
    a.ellipse(cx, cy + 9, 3, 4, fill=INK, outline=None)
    return a.result()


# ---------------------------------------------------------------------------------------------
# 이펙트 (흰색으로 만들고 게임에서 색을 입힌다)

def make_puff(frame):
    a = Art(160, 160)
    random.seed(40 + frame)
    if frame < 2:
        radius = 30 + frame * 16
        circles = [(80 + radius * 0.7 * math.cos(t), 80 + radius * 0.7 * math.sin(t), radius * 0.55)
                   for t in [2 * math.pi * i / 7 for i in range(7)]]
        circles.append((80, 80, radius * 0.8))
        a.cloud(circles)
    else:
        spread = 46 + (frame - 2) * 14
        size = 11 - (frame - 2) * 5
        for i in range(7):
            t = 2 * math.pi * i / 7 + 0.3
            a.cloud([(80 + spread * math.cos(t), 80 + spread * math.sin(t), size)], lw=4)
    return a.result()


def make_burst():
    a = Art(160, 160)
    a.star(80, 80, 74, 40, 11, fill=WHITE, lw=6)
    return a.result()


def make_ring():
    a = Art(256, 256, ss=2)
    a.ellipse(128, 128, 124, 124, fill=None, outline=WHITE, lw=12)
    return a.result()


def make_dash_ring():
    a = Art(512, 512, ss=2)
    count = 40
    for index in range(count):
        a0 = 360.0 * index / count
        a.arc(256, 256, 250, 250, a0, a0 + 360.0 / count * 0.55, color=WHITE, lw=6)
    return a.result()


def make_disc():
    size = 256
    img = Image.new("L", (size, size), 0)
    ImageDraw.Draw(img).ellipse([24, 24, size - 24, size - 24], fill=255)
    img = img.filter(ImageFilter.GaussianBlur(10))
    out = Image.new("RGBA", (size, size), (255, 255, 255, 0))
    out.putalpha(img)
    return out


def make_shadow():
    size = (128, 48)
    img = Image.new("L", size, 0)
    ImageDraw.Draw(img).ellipse([12, 8, size[0] - 12, size[1] - 8], fill=150)
    img = img.filter(ImageFilter.GaussianBlur(4))
    out = Image.new("RGBA", size, (20, 30, 20, 0))
    out.putalpha(img)
    return out


def make_star_fx():
    a = Art(48, 48)
    a.star(24, 24, 21, 8, 4, fill=WHITE, lw=4)
    return a.result()


def make_ice_block():
    a = Art(120, 132)
    a.poly([(18, 34), (60, 10), (102, 30), (108, 104), (62, 124), (14, 106)], fill=rgba("bfeaff", 190), lw=5)
    a.line([(60, 12), (62, 122)], color=rgba("ffffff", 150), lw=3, cap=False)
    a.line([(30, 44), (44, 36)], color=WHITE, lw=5)
    a.line([(28, 58), (34, 54)], color=WHITE, lw=5)
    return a.result()


def make_flame_fx():
    a = Art(48, 56)
    a.poly([(24, 2), (36, 20), (44, 36), (36, 50), (24, 54), (12, 50), (4, 36), (12, 24), (17, 30)], fill=WHITE, lw=4)
    return a.result()


# ---------------------------------------------------------------------------------------------
# 룬 아이콘

def make_rune(kind):
    a = Art(96, 96)
    a.rrect(5, 5, 91, 91, 22, fill=WHITE, lw=6)
    a.rrect(11, 60, 85, 85, 16, fill=SHADE, outline=None)
    a.rrect(11, 11, 85, 78, 16, fill=WHITE, outline=None)
    if kind == "Fire":
        a.poly([(48, 14), (62, 34), (72, 52), (64, 72), (48, 80), (32, 72), (24, 52), (33, 38), (39, 46)], fill=ORANGE, lw=4)
        a.ellipse(48, 62, 11, 13, fill=YELLOW, outline=None)
    elif kind == "Ice":
        for angle in (0, 60, 120):
            dx, dy = 30 * math.cos(math.radians(angle)), 30 * math.sin(math.radians(angle))
            a.line([(48 - dx, 48 - dy), (48 + dx, 48 + dy)], color=INK, lw=11)
        for angle in (0, 60, 120):
            dx, dy = 30 * math.cos(math.radians(angle)), 30 * math.sin(math.radians(angle))
            a.line([(48 - dx, 48 - dy), (48 + dx, 48 + dy)], color=SKY, lw=5)
        a.dot(48, 48, 6, WHITE)
    elif kind == "Water":
        a.poly([(48, 12), (66, 42), (70, 58), (62, 74), (48, 80), (34, 74), (26, 58), (30, 42)], fill=BLUE, lw=4)
        a.dot(39, 54, 5, rgba("bcd6ff"))
    elif kind == "Lightning":
        a.poly([(56, 10), (26, 52), (44, 52), (36, 86), (70, 40), (52, 40)], fill=YELLOW, lw=4)
    elif kind == "Poison":
        a.ellipse(48, 46, 24, 24, fill=GREEN, lw=4)
        a.eyes(48, 42, gap=9, r=4.5)
        a.rrect(38, 62, 58, 78, 4, fill=GREEN, lw=4)
        a.line([(48, 64), (48, 76)], lw=2.5, cap=False)
    elif kind == "Wind":
        a.arc(42, 40, 20, 14, 150, 400, color=INK, lw=10)
        a.arc(42, 40, 20, 14, 150, 400, color=MINT, lw=5)
        a.line([(20, 62), (70, 62)], color=INK, lw=10)
        a.line([(20, 62), (70, 62)], color=MINT, lw=5)
        a.line([(34, 76), (60, 76)], color=INK, lw=10)
        a.line([(34, 76), (60, 76)], color=MINT, lw=5)
    elif kind == "Explosion":
        a.star(48, 48, 36, 19, 9, fill=RED, lw=4)
        a.star(48, 48, 19, 10, 9, fill=YELLOW, outline=None)
    elif kind == "Chain":
        a.ellipse(36, 40, 18, 13, fill=None, outline=INK, lw=11)
        a.ellipse(36, 40, 15.5, 10.5, fill=None, outline=PURPLE, lw=6)
        a.ellipse(60, 58, 18, 13, fill=None, outline=INK, lw=11)
        a.ellipse(60, 58, 15.5, 10.5, fill=None, outline=PURPLE, lw=6)
    elif kind == "Split":
        for dx, dy in ((-24, -22), (0, -30), (24, -22)):
            a.line([(48, 76), (48 + dx, 76 + dy * 1.6)], color=INK, lw=10)
        for dx, dy in ((-24, -22), (0, -30), (24, -22)):
            a.line([(48, 76), (48 + dx, 76 + dy * 1.6)], color=ORANGE, lw=5)
            a.dot(48 + dx, 76 + dy * 1.6, 7, ORANGE)
    return a.result()


# ---------------------------------------------------------------------------------------------
# UI

def make_panel():
    a = Art(64, 64)
    a.rrect(3, 3, 61, 61, 18, fill=INK, outline=None)
    a.rrect(8, 8, 56, 58, 13, fill=SHADE, outline=None)
    a.rrect(8, 8, 56, 52, 13, fill=WHITE, outline=None)
    return a.result()


def make_panel_flat():
    a = Art(64, 64)
    a.rrect(3, 3, 61, 61, 18, fill=WHITE, outline=None)
    return a.result()


def make_disk():
    """스펠 디스크: 룬 두 개를 꽂는 원반."""
    a = Art(240, 240)
    a.ellipse(120, 120, 114, 114, fill=WHITE, lw=7)
    a.ellipse(120, 120, 98, 98, fill=None, outline=SHADE, lw=5)
    a.ellipse(120, 120, 80, 80, fill=None, outline=SHADE, lw=3)
    for index in range(12):
        t = 2 * math.pi * index / 12
        a.dot(120 + 106 * math.cos(t), 120 + 106 * math.sin(t), 3.5, GRAY)
    a.ellipse(120, 120, 16, 16, fill=SHADE, lw=5)
    return a.result()


def make_socket():
    a = Art(96, 96)
    a.rrect(5, 5, 91, 91, 22, fill=rgba("d6d2e0"), lw=6)
    a.rrect(16, 16, 80, 80, 14, fill=rgba("c2bdd0"), outline=None)
    a.line([(48, 36), (48, 60)], color=WHITE, lw=7)
    a.line([(36, 48), (60, 48)], color=WHITE, lw=7)
    return a.result()


def make_icon(kind):
    a = Art(64, 64)
    if kind == "Heart":
        a.cloud([(22, 24, 13), (42, 24, 13)], fill=RED)
        a.poly([(8, 30), (32, 58), (56, 30)], fill=RED, lw=5)
        a.poly([(12, 28), (32, 50), (52, 28), (32, 22)], fill=RED, outline=None)
        a.dot(20, 20, 4, WHITE)
    elif kind == "Coin":
        a.ellipse(32, 32, 26, 26, fill=YELLOW)
        a.ellipse(32, 32, 16, 16, fill=None, outline=rgba("d9a520"), lw=4)
        a.dot(24, 22, 4, WHITE)
    elif kind == "Flag":
        a.line([(18, 58), (18, 8)], lw=6)
        a.poly([(18, 8), (54, 20), (18, 34)], fill=RED)
    elif kind == "Lock":
        a.ellipse(32, 24, 14, 16, fill=None, outline=INK, lw=7)
        a.rrect(12, 26, 52, 58, 8, fill=GRAY)
        a.dot(32, 40, 5)
        a.line([(32, 40), (32, 50)], lw=4)
    elif kind in ("Star", "StarEmpty"):
        a.star(32, 34, 28, 13, 5, fill=YELLOW if kind == "Star" else rgba("d6d2e0"))
    elif kind == "Up":
        a.poly([(32, 6), (58, 34), (42, 34), (42, 58), (22, 58), (22, 34), (6, 34)], fill=GREEN)
    elif kind == "Sell":
        a.poly([(20, 18), (44, 18), (56, 44), (48, 58), (16, 58), (8, 44)], fill=TAN)
        a.line([(22, 18), (42, 18)], lw=7)
        a.dot(32, 40, 9, YELLOW)
    elif kind == "Pause":
        a.rrect(14, 10, 28, 54, 5, fill=WHITE)
        a.rrect(36, 10, 50, 54, 5, fill=WHITE)
    elif kind == "Play":
        a.poly([(16, 8), (54, 32), (16, 56)], fill=WHITE)
    elif kind == "Fast":
        a.poly([(6, 10), (32, 32), (6, 54)], fill=WHITE)
        a.poly([(32, 10), (58, 32), (32, 54)], fill=WHITE)
    elif kind == "Cycle":
        a.arc(32, 32, 20, 20, 30, 300, lw=7)
        a.poly([(44, 4), (58, 20), (38, 24)], fill=INK, lw=3)
    elif kind == "Wand":
        a.line([(14, 54), (40, 24)], color=INK, lw=10)
        a.line([(14, 54), (40, 24)], color=BROWN, lw=5)
        a.star(44, 18, 16, 7, 5, fill=YELLOW, lw=4)
    elif kind == "Check":
        a.line([(12, 34), (26, 48), (52, 16)], color=INK, lw=13)
        a.line([(12, 34), (26, 48), (52, 16)], color=GREEN, lw=7)
    elif kind == "Skull":
        a.ellipse(32, 28, 22, 20, fill=WHITE)
        a.rrect(22, 40, 42, 56, 5, fill=WHITE)
        a.eyes(32, 28, gap=9, r=5)
    return a.result()


# ---------------------------------------------------------------------------------------------
# 배경

def make_map(path):
    """1920x1080 좌표로 그려 2560x1440으로 저장."""
    out_w, out_h = 2560, 1440
    ss = 2
    k = out_w / map_data.VIEW_W
    a = Art(out_w, out_h, ss=ss, lw=6)
    rng = random.Random(7)

    def P(pts):
        return [(x * k, y * k) for x, y in pts]

    # 풀밭: 밝은 연두 바탕 + 살짝 진한 얼룩
    a.d.rectangle([0, 0, out_w * ss, out_h * ss], fill=rgba("c9ec9c"))
    for _ in range(46):
        x, y = rng.uniform(0, 1920), rng.uniform(0, 1080)
        a.blob(x * k, y * k, rng.uniform(70, 220) * k, rng.uniform(40, 110) * k, angle=rng.uniform(-20, 20),
               fill=rgba("bde58c"), outline=None, n=24)

    # 연못 (왼쪽 아래) — 길과 떨어진 곳
    a.blob(1040 * k, 990 * k, 150 * k, 46 * k, fill=rgba("9fdcff"), lw=6, n=40)
    a.arc(1010 * k, 985 * k, 30 * k, 8 * k, 180, 360, color=WHITE, lw=4)
    a.arc(1090 * k, 1000 * k, 22 * k, 6 * k, 180, 360, color=WHITE, lw=4)

    # 길: 먹선 → 모래색 → 밝은 가운데
    pk = P(path)
    width = map_data.PATH_WIDTH * k
    # (굵은 선은 이음매에 틈이 생겨서 원을 촘촘히 찍는다)
    for color, w in ((INK, width + 12), (rgba("f6e3b4"), width), (rgba("fbeecb"), width * 0.5)):
        for x, y in pk:
            a.dot(x, y, w * 0.5, color)
    # 길 위 자갈·발자국
    total = len(path)
    for index in range(6, total - 4, 9):
        x, y = path[index]
        nx, ny = path[index + 1][0] - x, path[index + 1][1] - y
        length = math.hypot(nx, ny) or 1.0
        off = rng.uniform(-26, 26)
        px, py = x - ny / length * off, y + nx / length * off
        if rng.random() < 0.5:
            a.ellipse(px * k, py * k, rng.uniform(4, 8) * k, rng.uniform(3, 5) * k, fill=rgba("e3cc96"), outline=None)
        else:
            a.ellipse(px * k, py * k, 5 * k, 3.5 * k, fill=None, outline=rgba("d9bf85"), lw=2.5)

    def free(x, y, margin):
        if map_data.dist_to_path((x, y), path) < map_data.PATH_WIDTH * 0.5 + margin:
            return False
        for sx, sy in map_data.SPOTS:
            if abs(x - sx) < 95 + margin * 0.5 and -150 - margin * 0.3 < y - sy < 55 + margin * 1.4:
                return False
        wx, wy = map_data.WIZARD
        if abs(x - wx) < 80 and -190 < y - wy < 40:
            return False
        gx, gy = map_data.GATE
        if x > gx - 150 and abs(y - gy + 60) < 190:
            return False
        return True

    # 풀 낙서
    for _ in range(420):
        x, y = rng.uniform(10, 1910), rng.uniform(10, 1070)
        if not free(x, y, 8):
            continue
        s = rng.uniform(0.8, 1.3)
        color = rgba("8cc765")
        a.line([((x - 7 * s) * k, (y - 6 * s) * k), ((x - 3 * s) * k, y * k)], color=color, lw=3)
        a.line([(x * k, (y - 9 * s) * k), (x * k, y * k)], color=color, lw=3)
        a.line([((x + 7 * s) * k, (y - 6 * s) * k), ((x + 3 * s) * k, y * k)], color=color, lw=3)

    # 꽃
    for _ in range(70):
        x, y = rng.uniform(20, 1900), rng.uniform(20, 1060)
        if not free(x, y, 14):
            continue
        petal = rng.choice([WHITE, PINK, YELLOW])
        for t in range(5):
            ang = 2 * math.pi * t / 5
            a.ellipse((x + 6 * math.cos(ang)) * k, (y + 6 * math.sin(ang)) * k, 5 * k, 5 * k, fill=petal, lw=2.5)
        a.dot(x * k, y * k, 3.5 * k, ORANGE)

    # 바위 (가끔 얼굴이 있다)
    for _ in range(26):
        x, y = rng.uniform(30, 1890), rng.uniform(40, 1050)
        if not free(x, y, 30):
            continue
        r = rng.uniform(16, 30)
        a.ellipse(x * k, (y + r * 0.5) * k, r * 1.2 * k, r * 0.35 * k, fill=rgba("a9d67c"), outline=None)
        a.blob(x * k, y * k, r * k, r * 0.78 * k, fill=STONE, lw=5, power=2.6, n=24)
        a.arc((x - r * 0.3) * k, (y - r * 0.25) * k, r * 0.35 * k, r * 0.3 * k, 190, 280, color=WHITE, lw=4)
        if rng.random() < 0.45:
            a.dot((x - r * 0.25) * k, y * k, 2.6 * k)
            a.dot((x + r * 0.25) * k, y * k, 2.6 * k)
            a.line([((x - r * 0.12) * k, (y + r * 0.28) * k), ((x + r * 0.12) * k, (y + r * 0.28) * k)], lw=2.5)

    # 나무: 아래쪽에 있는 것이 위를 덮도록 y순으로
    trees = []
    attempts = 0
    while len(trees) < 64 and attempts < 4000:
        attempts += 1
        x, y = rng.uniform(-10, 1930), rng.uniform(60, 1075)
        if not free(x, y, 52):
            continue
        # 나무는 위로 길다. 잎이 길을 덮으면 몬스터가 나무 위를 걷는 것처럼 보인다
        half = map_data.PATH_WIDTH * 0.5
        if map_data.dist_to_path((x, y - 60), path) < half + 40 or map_data.dist_to_path((x, y - 110), path) < half + 30:
            continue
        if any(abs(x - tx) < 46 and abs(y - ty) < 30 for tx, ty, _, _ in trees):
            continue
        # 화면 위쪽 HUD, 아래 스펠 바 자리는 비워 둔다
        if (y < 215 and x < 470) or (y > 900 and 560 < x < 1360) or (y < 150 and x > 1640):
            continue
        trees.append((x, y, rng.uniform(0.85, 1.25), rng.random()))
    trees.sort(key=lambda t: t[1])
    for x, y, s, kind in trees:
        a.ellipse(x * k, (y + 2) * k, 30 * s * k, 9 * s * k, fill=rgba("a9d67c"), outline=None)
        a.line([(x * k, y * k), (x * k, (y - 34 * s) * k)], color=INK, lw=15 * s)
        a.line([(x * k, y * k), (x * k, (y - 34 * s) * k)], color=BROWN, lw=8 * s)
        leaf = rgba("7fcf6a") if kind < 0.6 else rgba("63bf7c")
        if kind < 0.75:
            a.cloud([((x - 20 * s) * k, (y - 56 * s) * k, 22 * s * k), ((x + 20 * s) * k, (y - 56 * s) * k, 22 * s * k),
                     (x * k, (y - 78 * s) * k, 26 * s * k), (x * k, (y - 52 * s) * k, 22 * s * k)], fill=leaf, lw=5)
            a.arc((x - 6 * s) * k, (y - 78 * s) * k, 14 * s * k, 12 * s * k, 190, 290, color=rgba("b6ec9c"), lw=5)
        else:
            a.poly([((x - 30 * s) * k, (y - 28 * s) * k), (x * k, (y - 112 * s) * k), ((x + 30 * s) * k, (y - 28 * s) * k)], fill=leaf, lw=5)
        if kind > 0.9:
            a.dot((x - 8 * s) * k, (y - 60 * s) * k, 3 * k)
            a.dot((x + 8 * s) * k, (y - 60 * s) * k, 3 * k)

    # 입구 표지판
    sx, sy = 70, 170
    a.line([(sx * k, (sy + 40) * k), (sx * k, sy * k)], lw=7)
    a.rrect((sx - 44) * k, (sy - 34) * k, (sx + 44) * k, (sy + 6) * k, 8 * k, fill=WHITE, lw=5)
    a.poly([((sx - 20) * k, (sy - 14) * k), ((sx + 6) * k, (sy - 14) * k), ((sx + 6) * k, (sy - 24) * k), ((sx + 26) * k, (sy - 14) * k),
            ((sx + 6) * k, (sy - 4) * k), ((sx + 6) * k, (sy - 14) * k)], fill=RED, lw=4)

    # 성문 (오른쪽 끝)
    gx, gy = map_data.GATE
    def K(x, y):
        return ((gx + x) * k, (gy + y) * k)
    a.ellipse(K(-10, 34)[0], K(-10, 34)[1], 150 * k, 30 * k, fill=rgba("a9d67c"), outline=None)
    for tx in (-96, 56):
        a.rrect(K(tx, -250)[0], K(tx, -250)[1], K(tx + 62, 30)[0], K(tx + 62, 30)[1], 10 * k, fill=WHITE, lw=6)
        for bx in (tx - 6, tx + 20, tx + 46):
            a.rrect(K(bx, -276)[0], K(bx, -276)[1], K(bx + 22, -244)[0], K(bx + 22, -244)[1], 4 * k, fill=WHITE, lw=5)
        a.rrect(K(tx + 22, -200)[0], K(tx + 22, -200)[1], K(tx + 40, -168)[0], K(tx + 40, -168)[1], 7 * k, fill=DARK, lw=4)
    a.rrect(K(-40, -170)[0], K(-40, -170)[1], K(62, 30)[0], K(62, 30)[1], 6 * k, fill=STONE, lw=6)
    a.chord(K(11, -40)[0], K(11, -40)[1], 40 * k, 64 * k, 180, 360, fill=DARK, lw=6)
    a.rrect(K(-29, -42)[0], K(-29, -42)[1], K(51, 30)[0], K(51, 30)[1], 0, fill=DARK, lw=0)
    a.line([K(-29, -40), K(-29, 30)], lw=6, cap=False)
    a.line([K(51, -40), K(51, 30)], lw=6, cap=False)
    for bx in (-34, -8, 18, 44):
        a.rrect(K(bx, -192)[0], K(bx, -192)[1], K(bx + 20, -164)[0], K(bx + 20, -164)[1], 4 * k, fill=STONE, lw=5)
    # 깃발
    a.line([K(-65, -276), K(-65, -340)], lw=6)
    a.poly([K(-65, -340), K(-22, -326), K(-65, -310)], fill=RED, lw=5)
    a.line([K(87, -276), K(87, -340)], lw=6)
    a.poly([K(87, -340), K(130, -326), K(87, -310)], fill=SKY, lw=5)
    # 성문 위의 멍한 얼굴
    a.dot(K(0, -128)[0], K(0, -128)[1], 5 * k)
    a.dot(K(24, -128)[0], K(24, -128)[1], 5 * k)
    a.line([K(6, -112), K(18, -112)], lw=4)

    return a.result()


# ---------------------------------------------------------------------------------------------

def write_map_inl(path_points, inl_path):
    # 게임 규칙용으로는 24px 간격이면 충분하다
    coarse = path_points[::3]
    if coarse[-1] != path_points[-1]:
        coarse.append(path_points[-1])

    lines = [
        "// Tools/Art/generate_art.py가 Tools/Art/map_data.py에서 생성한다. 직접 고치지 말 것.",
        "",
        "const FVector2f GMap01Path[] = {",
    ]
    for index in range(0, len(coarse), 6):
        row = ", ".join("{ %.1ff, %.1ff }" % p for p in coarse[index:index + 6])
        lines.append("\t" + row + ",")
    lines.append("};")
    lines.append("")
    lines.append("const FVector2f GMap01Spots[] = {")
    lines.append("\t" + ", ".join("{ %.1ff, %.1ff }" % p for p in map_data.SPOTS) + ",")
    lines.append("};")
    lines.append("")
    lines.append("const FVector2f GMap01Wizard = { %.1ff, %.1ff };" % map_data.WIZARD)
    lines.append("const FVector2f GMap01Gate = { %.1ff, %.1ff };" % map_data.GATE)
    lines.append("")
    with open(inl_path, "w", encoding="utf-8", newline="\n") as f:
        f.write("\n".join(lines))


def build_all():
    sprites = {}

    for name, func in (("Imp", make_imp), ("Chicken", make_chicken), ("Potato", make_potato), ("Toast", make_toast),
                       ("Mushroom", make_mushroom), ("PuddingKing", make_pudding_king)):
        for frame in range(2):
            sprites["T_Enemy_%s_%d" % (name, frame)] = func(frame)

    for frame in range(3):
        sprites["T_Soldier_%d" % frame] = make_soldier(frame)
    for frame in range(2):
        sprites["T_Wizard_%d" % frame] = make_wizard(frame)

    for name, func in (("Archer", make_tower_archer), ("Barracks", make_tower_barracks), ("Bombard", make_tower_bombard),
                       ("Mage", make_tower_mage), ("Shrine", make_tower_shrine)):
        for level in range(3):
            sprites["T_Tower_%s_%d" % (name, level)] = func(level)
    sprites["T_BuildSpot"] = make_build_spot()

    sprites["T_Arrow"] = make_arrow()
    sprites["T_Bomb"] = make_bomb()
    sprites["T_MagicBolt"] = make_magic_bolt()
    for kind in ("Fire", "Ice", "Water", "Lightning", "Poison", "Wind", "Arcane"):
        sprites["T_Ball_" + kind] = make_ball(kind)

    for frame in range(4):
        sprites["T_FX_Puff%d" % frame] = make_puff(frame)
    sprites["T_FX_Burst"] = make_burst()
    sprites["T_FX_Ring"] = make_ring()
    sprites["T_FX_DashRing"] = make_dash_ring()
    sprites["T_FX_Disc"] = make_disc()
    sprites["T_FX_Shadow"] = make_shadow()
    sprites["T_FX_Star"] = make_star_fx()
    sprites["T_FX_Ice"] = make_ice_block()
    sprites["T_FX_Flame"] = make_flame_fx()

    for kind in ("Fire", "Ice", "Water", "Lightning", "Poison", "Wind", "Explosion", "Chain", "Split"):
        sprites["T_Rune_" + kind] = make_rune(kind)

    sprites["T_UI_Panel"] = make_panel()
    sprites["T_UI_PanelFlat"] = make_panel_flat()
    sprites["T_UI_Disk"] = make_disk()
    sprites["T_UI_Socket"] = make_socket()
    for kind in ("Heart", "Coin", "Flag", "Lock", "Star", "StarEmpty", "Up", "Sell", "Pause", "Play", "Fast", "Cycle", "Wand", "Check", "Skull"):
        sprites["T_UI_" + kind] = make_icon(kind)
    return sprites


def make_sheet(sprites, path):
    """검토용 모음 시트."""
    names = [n for n in sprites if n != "T_Map01"]
    cell = 200
    cols = 10
    rows = (len(names) + cols - 1) // cols
    sheet = Image.new("RGBA", (cols * cell, rows * cell), rgba("c9ec9c"))
    for index, name in enumerate(names):
        img = sprites[name]
        scale = min((cell - 16) / img.width, (cell - 16) / img.height, 1.0)
        if scale < 1.0:
            img = img.resize((max(int(img.width * scale), 1), max(int(img.height * scale), 1)), Image.LANCZOS)
        x = (index % cols) * cell + (cell - img.width) // 2
        y = (index // cols) * cell + (cell - img.height) // 2
        sheet.alpha_composite(img, (x, y))
    sheet.convert("RGB").save(path)


def main():
    out_dir = sys.argv[1] if len(sys.argv) > 1 else os.path.join(os.path.dirname(os.path.abspath(__file__)), "Generated")
    inl_path = sys.argv[2] if len(sys.argv) > 2 else None
    os.makedirs(out_dir, exist_ok=True)

    path_points = map_data.smooth_path(8.0)
    if inl_path:
        write_map_inl(path_points, inl_path)
        print("map inl ->", inl_path)

    sprites = build_all()
    sprites["T_Map01"] = make_map(path_points)
    for name, img in sprites.items():
        img.save(os.path.join(out_dir, name + ".png"))
    make_sheet(sprites, os.path.join(out_dir, "_sheet.png"))
    print("%d sprites -> %s" % (len(sprites), out_dir))


if __name__ == "__main__":
    main()
