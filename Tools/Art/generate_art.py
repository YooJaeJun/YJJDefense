"""YJJDefense 아트 생성기.

방향: 냥코 대전쟁처럼 귀엽지만 엉뚱한 얼굴 + 킹덤러쉬급 색·명암·재질 (paint.py, units_hd.py로 옮겨 가는 중).
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
# 명암·재질을 넣어 다시 그린 것 (점점 늘려 간다)
from units_hd import make_imp, make_soldier, make_tower_archer, make_tower_barracks  # noqa: E402
from towers_hd import make_tower_bombard, make_tower_mage, make_tower_shrine, make_wizard  # noqa: E402
from monsters_hd import (make_chicken, make_coco_kid, make_coconut, make_crab, make_jellyfish, make_king_crab,  # noqa: E402
                         make_mushroom, make_penguin, make_potato, make_pudding_king, make_snowball, make_snowman,
                         make_toast, make_yeti)

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
SNOW = rgba("f7fbff")


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


# ---------------------------------------------------------------------------------------------
# 캐릭터·타워는 units_hd.py, towers_hd.py, monsters_hd.py (명암 톤). 여기는 건설 자리·투사체·이펙트·UI·배경

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

THEMES = {
    "meadow": dict(
        base="c9ec9c", patch="bde58c", shade="a9d67c", scribble="8cc765",
        road=("f6e3b4", "fbeecb"), pebble=("e3cc96", "d9bf85"),
        pond=("9fdcff", (1040, 990, 150, 46)),
    ),
    "beach": dict(
        base="f8e4b2", patch="f2d79c", shade="e2c283", scribble="e6c98b",
        road=("b9814a", "d39d63"), pebble=("9c6838", "8a5a30"),
        pond=("8fdcff", (1060, 990, 140, 40)),
    ),
    "snow": dict(
        base="eef5fd", patch="e0ecfa", shade="cbdcef", scribble="c7dbf2",
        road=("c4d6ec", "d8e5f4"), pebble=("aec3dc", "9fb6d1"),
        pond=("bfe6ff", (1060, 990, 150, 42)),
    ),
}


def draw_gate(a, k, gate):
    gx, gy = gate

    def K(x, y):
        return ((gx + x) * k, (gy + y) * k)
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
    a.line([K(-65, -276), K(-65, -340)], lw=6)
    a.poly([K(-65, -340), K(-22, -326), K(-65, -310)], fill=RED, lw=5)
    a.line([K(87, -276), K(87, -340)], lw=6)
    a.poly([K(87, -340), K(130, -326), K(87, -310)], fill=SKY, lw=5)
    a.dot(K(0, -128)[0], K(0, -128)[1], 5 * k)
    a.dot(K(24, -128)[0], K(24, -128)[1], 5 * k)
    a.line([K(6, -112), K(18, -112)], lw=4)


def draw_tree(a, k, theme, x, y, s, kind):
    if theme == "beach":
        # 야자수: 휘어진 줄기 + 잎 다섯
        lean = 18 * s if kind < 0.5 else -18 * s
        top = (x + lean, y - 92 * s)
        trunk = [(x, y), (x + lean * 0.3, y - 40 * s), top]
        a.line([(px * k, py * k) for px, py in trunk], color=INK, lw=15 * s)
        a.line([(px * k, py * k) for px, py in trunk], color=rgba("c99a5e"), lw=8 * s)
        for angle in (-160, -120, -60, -20, 200):
            t = math.radians(angle)
            tip = (top[0] + 46 * s * math.cos(t), top[1] + 30 * s * math.sin(t) + 14 * s)
            mid = ((top[0] + tip[0]) / 2, (top[1] + tip[1]) / 2 - 10 * s)
            a.blob(mid[0] * k, mid[1] * k, 28 * s * k, 9 * s * k, angle=math.degrees(math.atan2(tip[1] - top[1], tip[0] - top[0])),
                   fill=rgba("62c46e"), lw=5)
        a.dot((top[0] - 6 * s) * k, (top[1] + 6 * s) * k, 6 * s * k, rgba("8f5c36"))
        a.dot((top[0] + 6 * s) * k, (top[1] + 8 * s) * k, 6 * s * k, rgba("8f5c36"))
        return
    if theme == "snow":
        # 눈 덮인 전나무 3단
        a.line([(x * k, y * k), (x * k, (y - 20 * s) * k)], color=INK, lw=13 * s)
        a.line([(x * k, y * k), (x * k, (y - 20 * s) * k)], color=BROWN, lw=7 * s)
        leaf = rgba("4f9f7a") if kind < 0.6 else rgba("3f8a6e")
        for index, (w, top, bottom) in enumerate(((34, 70, 18), (27, 94, 48), (19, 116, 74))):
            a.poly([((x - w * s) * k, (y - bottom * s) * k), (x * k, (y - top * s) * k), ((x + w * s) * k, (y - bottom * s) * k)], fill=leaf, lw=5)
            a.poly([((x - w * 0.55 * s) * k, (y - (top - (top - bottom) * 0.45) * s) * k), (x * k, (y - top * s) * k),
                    ((x + w * 0.55 * s) * k, (y - (top - (top - bottom) * 0.45) * s) * k)], fill=SNOW, outline=None)
        if kind > 0.88:
            a.dot((x - 7 * s) * k, (y - 46 * s) * k, 3 * k)
            a.dot((x + 7 * s) * k, (y - 46 * s) * k, 3 * k)
        return
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


def draw_flower(a, k, theme, rng, x, y):
    if theme == "beach":
        if rng.random() < 0.5:
            a.star(x * k, y * k, 10 * k, 4.5 * k, 5, fill=rgba("ff9a6a"), lw=3, rot=rng.uniform(0, 360))
        else:
            a.chord(x * k, y * k, 9 * k, 9 * k, 180, 360, fill=rgba("ffe1ea"), lw=3)
            for dx in (-4, 0, 4):
                a.line([((x + dx * 0.6) * k, (y - 1) * k), ((x + dx) * k, (y - 7) * k)], color=rgba("e8a6b8"), lw=2)
        return
    if theme == "snow":
        a.poly([(x * k, (y - 9) * k), ((x + 5) * k, y * k), (x * k, (y + 9) * k), ((x - 5) * k, y * k)], fill=rgba("aee0ff"), lw=3)
        return
    petal = rng.choice([WHITE, PINK, YELLOW])
    for t in range(5):
        ang = 2 * math.pi * t / 5
        a.ellipse((x + 6 * math.cos(ang)) * k, (y + 6 * math.sin(ang)) * k, 5 * k, 5 * k, fill=petal, lw=2.5)
    a.dot(x * k, y * k, 3.5 * k, ORANGE)


def draw_road(a, k, theme, path, rng):
    t = THEMES[theme]
    pk = [(x * k, y * k) for x, y in path]
    width = map_data.PATH_WIDTH * k
    # 굵은 선은 이음매에 틈이 생겨서 원을 촘촘히 찍는다
    for color, w in ((INK, width + 12), (rgba(t["road"][0]), width), (rgba(t["road"][1]), width * 0.5)):
        for x, y in pk:
            a.dot(x, y, w * 0.5, color)

    if theme == "beach":
        # 나무 판자 길: 길을 가로지르는 판자 틈
        for index in range(3, len(path) - 2, 3):
            x, y = path[index]
            nx, ny = path[index + 1][0] - path[index - 1][0], path[index + 1][1] - path[index - 1][1]
            length = math.hypot(nx, ny) or 1.0
            px, py = -ny / length, nx / length
            h = map_data.PATH_WIDTH * 0.46
            a.line([((x - px * h) * k, (y - py * h) * k), ((x + px * h) * k, (y + py * h) * k)], color=rgba("8a5a30"), lw=2.5, cap=False)
        return

    for index in range(6, len(path) - 4, 9):
        x, y = path[index]
        nx, ny = path[index + 1][0] - x, path[index + 1][1] - y
        length = math.hypot(nx, ny) or 1.0
        off = rng.uniform(-26, 26)
        px, py = x - ny / length * off, y + nx / length * off
        if rng.random() < 0.5:
            a.ellipse(px * k, py * k, rng.uniform(4, 8) * k, rng.uniform(3, 5) * k, fill=rgba(t["pebble"][0]), outline=None)
        else:
            a.ellipse(px * k, py * k, 5 * k, 3.5 * k, fill=None, outline=rgba(t["pebble"][1]), lw=2.5)


def make_map(m, path):
    """1920x1080 좌표로 그려 2560x1440으로 저장."""
    theme = m["theme"]
    t = THEMES[theme]
    out_w, out_h = 2560, 1440
    ss = 2
    k = out_w / map_data.VIEW_W
    a = Art(out_w, out_h, ss=ss, lw=6)
    rng = random.Random(7 + len(path))

    # 바탕 + 얼룩
    a.d.rectangle([0, 0, out_w * ss, out_h * ss], fill=rgba(t["base"]))
    for _ in range(46):
        x, y = rng.uniform(0, 1920), rng.uniform(0, 1080)
        a.blob(x * k, y * k, rng.uniform(70, 220) * k, rng.uniform(40, 110) * k, angle=rng.uniform(-20, 20), fill=rgba(t["patch"]), outline=None, n=24)

    sea_y = 0
    if theme == "beach":
        # 위쪽 바다: 젖은 모래 띠 → 물결 → 바다
        sea_y = 118
        a.d.rectangle([0, 0, out_w * ss, (sea_y + 14) * k * ss], fill=rgba("ecd29a"))
        pts = [(x * k, (sea_y + 6 * math.sin(x / 38.0)) * k) for x in range(-20, 1960, 12)]
        a.poly([(-20 * k, -20 * k)] + pts + [(1960 * k, -20 * k)], fill=rgba("7fd0ff"), lw=6)
        for row, y in enumerate((30, 66)):
            for x in range(40 + row * 60, 1920, 150):
                a.arc((x + 30) * k, y * k, 26 * k, 9 * k, 200, 340, color=WHITE, lw=4)
        a.arc(1500 * k, 52 * k, 40 * k, 14 * k, 190, 350, color=rgba("5bb7f0"), lw=5)

    # 연못 (빈 아래쪽)
    color, (px, py, prx, pry) = t["pond"]
    a.blob(px * k, py * k, prx * k, pry * k, fill=rgba(color), lw=6, n=40)
    if theme == "snow":
        a.line([((px - 60) * k, (py - 10) * k), ((px - 20) * k, (py + 8) * k), ((px + 30) * k, (py - 6) * k)], color=WHITE, lw=4)
        a.line([((px + 40) * k, (py + 14) * k), ((px + 80) * k, (py - 4) * k)], color=WHITE, lw=4)
    else:
        a.arc((px - 30) * k, (py - 5) * k, 30 * k, 8 * k, 180, 360, color=WHITE, lw=4)
        a.arc((px + 50) * k, (py + 10) * k, 22 * k, 6 * k, 180, 360, color=WHITE, lw=4)

    draw_road(a, k, theme, path, rng)

    def free(x, y, margin):
        if y < sea_y + 16 + margin * 0.3:
            return False
        if map_data.dist_to_path((x, y), path) < map_data.PATH_WIDTH * 0.5 + margin:
            return False
        for sx, sy in m["spots"]:
            if abs(x - sx) < 95 + margin * 0.5 and -150 - margin * 0.3 < y - sy < 55 + margin * 1.4:
                return False
        wx, wy = m["wizard"]
        if abs(x - wx) < 80 and -190 < y - wy < 40:
            return False
        gx, gy = m["gate"]
        if x > gx - 150 and abs(y - gy + 60) < 190:
            return False
        if abs(x - px) < prx + 10 and abs(y - py) < pry + 10:
            return False
        return True

    # 풀 낙서 (설산은 반짝이)
    for _ in range(420):
        x, y = rng.uniform(10, 1910), rng.uniform(10, 1070)
        if not free(x, y, 8):
            continue
        s = rng.uniform(0.8, 1.3)
        color = rgba(t["scribble"])
        if theme == "snow":
            a.line([((x - 5 * s) * k, y * k), ((x + 5 * s) * k, y * k)], color=color, lw=3)
            a.line([(x * k, (y - 5 * s) * k), (x * k, (y + 5 * s) * k)], color=color, lw=3)
            continue
        a.line([((x - 7 * s) * k, (y - 6 * s) * k), ((x - 3 * s) * k, y * k)], color=color, lw=3)
        a.line([(x * k, (y - 9 * s) * k), (x * k, y * k)], color=color, lw=3)
        a.line([((x + 7 * s) * k, (y - 6 * s) * k), ((x + 3 * s) * k, y * k)], color=color, lw=3)

    for _ in range(70):
        x, y = rng.uniform(20, 1900), rng.uniform(20, 1060)
        if free(x, y, 14):
            draw_flower(a, k, theme, rng, x, y)

    # 바위 (가끔 얼굴. 설산은 눈 모자)
    for _ in range(26):
        x, y = rng.uniform(30, 1890), rng.uniform(40, 1050)
        if not free(x, y, 30):
            continue
        r = rng.uniform(16, 30)
        a.ellipse(x * k, (y + r * 0.5) * k, r * 1.2 * k, r * 0.35 * k, fill=rgba(t["shade"]), outline=None)
        a.blob(x * k, y * k, r * k, r * 0.78 * k, fill=STONE, lw=5, power=2.6, n=24)
        if theme == "snow":
            a.chord(x * k, (y - r * 0.3) * k, r * 0.85 * k, r * 0.5 * k, 180, 360, fill=SNOW, outline=None)
        else:
            a.arc((x - r * 0.3) * k, (y - r * 0.25) * k, r * 0.35 * k, r * 0.3 * k, 190, 280, color=WHITE, lw=4)
        if rng.random() < 0.45:
            a.dot((x - r * 0.25) * k, y * k, 2.6 * k)
            a.dot((x + r * 0.25) * k, y * k, 2.6 * k)
            a.line([((x - r * 0.12) * k, (y + r * 0.28) * k), ((x + r * 0.12) * k, (y + r * 0.28) * k)], lw=2.5)

    # 해변 소품: 파라솔
    if theme == "beach":
        for ux, uy in ((1250, 1010), (150, 1010)):
            if not free(ux, uy, 20):
                continue
            a.ellipse(ux * k, (uy + 4) * k, 60 * k, 14 * k, fill=rgba(t["shade"]), outline=None)
            a.rrect((ux - 40) * k, (uy - 6) * k, (ux + 40) * k, (uy + 10) * k, 6 * k, fill=rgba("7fd6ff"), lw=4)
            a.line([(ux * k, uy * k), ((ux + 6) * k, (uy - 74) * k)], lw=5)
            a.chord((ux + 6) * k, (uy - 70) * k, 58 * k, 34 * k, 180, 360, fill=WHITE, lw=5)
            for index in range(3):
                a0 = 180 + index * 60
                a.chord((ux + 6) * k, (uy - 70) * k, 58 * k, 34 * k, a0, a0 + 30, fill=RED, outline=None)

    # 나무: 아래쪽에 있는 것이 위를 덮도록 y순으로
    trees = []
    attempts = 0
    while len(trees) < (48 if theme == "beach" else 64) and attempts < 4000:
        attempts += 1
        x, y = rng.uniform(-10, 1930), rng.uniform(60, 1075)
        if not free(x, y, 52):
            continue
        # 나무는 위로 길다. 잎이 길을 덮으면 몬스터가 나무 위를 걷는 것처럼 보인다
        half = map_data.PATH_WIDTH * 0.5
        if map_data.dist_to_path((x, y - 60), path) < half + 40 or map_data.dist_to_path((x, y - 110), path) < half + 30:
            continue
        if y - 110 < sea_y:
            continue
        if any(abs(x - tx) < 46 and abs(y - ty) < 30 for tx, ty, _, _ in trees):
            continue
        # 화면 위쪽 HUD, 아래 스펠 바 자리는 비워 둔다
        if (y < 215 and x < 760) or (y > 900 and 560 < x < 1360) or (y < 150 and x > 1640):
            continue
        trees.append((x, y, rng.uniform(0.85, 1.25), rng.random()))
    trees.sort(key=lambda tr: tr[1])
    for x, y, s, kind in trees:
        a.ellipse(x * k, (y + 2) * k, 30 * s * k, 9 * s * k, fill=rgba(t["shade"]), outline=None)
        draw_tree(a, k, theme, x, y, s, kind)

    # 입구 표지판
    sx, sy = m["sign"]
    a.line([(sx * k, (sy + 40) * k), (sx * k, sy * k)], lw=7)
    a.rrect((sx - 44) * k, (sy - 34) * k, (sx + 44) * k, (sy + 6) * k, 8 * k, fill=WHITE, lw=5)
    a.poly([((sx - 20) * k, (sy - 14) * k), ((sx + 6) * k, (sy - 14) * k), ((sx + 6) * k, (sy - 24) * k), ((sx + 26) * k, (sy - 14) * k),
            ((sx + 6) * k, (sy - 4) * k), ((sx + 6) * k, (sy - 14) * k)], fill=RED, lw=4)

    gx, gy = m["gate"]
    a.ellipse((gx - 10) * k, (gy + 34) * k, 150 * k, 30 * k, fill=rgba(t["shade"]), outline=None)
    draw_gate(a, k, m["gate"])
    return a.result()


# ---------------------------------------------------------------------------------------------

def write_map_geometry(paths, csv_path):
    """지도 모양을 Content/YJJDefense/Data/MapGeometry.csv로 (게임이 DataTable로 읽는다)."""

    def vec(p):
        return "(X=%.1f,Y=%.1f)" % (p[0], p[1])

    def vec_list(points):
        return '"(' + ",".join(vec(p) for p in points) + ')"'

    lines = ["Name,Path,Spots,Wizard,Gate"]
    for m, path_points in zip(map_data.MAPS, paths):
        # 게임 규칙용으로는 24px 간격이면 충분하다
        coarse = path_points[::3]
        if coarse[-1] != path_points[-1]:
            coarse.append(path_points[-1])
        lines.append(",".join([m["key"], vec_list(coarse), vec_list(m["spots"]), '"%s"' % vec(m["wizard"]), '"%s"' % vec(m["gate"])]))
    with open(csv_path, "w", encoding="utf-8-sig", newline="\n") as f:
        f.write("\n".join(lines) + "\n")


ENEMY_ART = (
    ("Imp", make_imp), ("Chicken", make_chicken), ("Potato", make_potato), ("Toast", make_toast),
    ("Mushroom", make_mushroom), ("PuddingKing", make_pudding_king),
    ("Crab", make_crab), ("Jellyfish", make_jellyfish), ("Coconut", make_coconut), ("CocoKid", make_coco_kid), ("KingCrab", make_king_crab),
    ("Snowman", make_snowman), ("Snowball", make_snowball), ("Penguin", make_penguin), ("Yeti", make_yeti),
)


def build_all():
    sprites = {}

    for name, func in ENEMY_ART:
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


def make_sheet(sprites, path, names=None):
    """검토용 모음 시트."""
    names = names or [n for n in sprites if not n.startswith("T_Map")]
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
    geometry_path = sys.argv[2] if len(sys.argv) > 2 else None
    os.makedirs(out_dir, exist_ok=True)

    paths = []
    for m in map_data.MAPS:
        path = map_data.smooth_path(m["path"], 8.0)
        problems = map_data.check(m, path)
        if problems:
            print("WARNING %s: %s" % (m["key"], "; ".join(problems)))
        paths.append(path)
    if geometry_path:
        write_map_geometry(paths, geometry_path)
        print("map geometry ->", geometry_path)

    sprites = build_all()
    for m, path in zip(map_data.MAPS, paths):
        img = make_map(m, path)
        sprites["T_" + m["key"]] = img
        # 작은 미리보기 (큰 그림을 밉맵 없이 줄여 그리면 지글거린다)
        sprites["T_" + m["key"] + "_Thumb"] = img.resize((384, 216), Image.LANCZOS)
    for name, img in sprites.items():
        img.save(os.path.join(out_dir, name + ".png"))
    make_sheet(sprites, os.path.join(out_dir, "_sheet.png"))
    make_sheet(sprites, os.path.join(out_dir, "_sheet_enemies.png"), ["T_Enemy_%s_%d" % (n, f) for n, _ in ENEMY_ART for f in range(2)])
    print("%d sprites -> %s" % (len(sprites), out_dir))


if __name__ == "__main__":
    main()
