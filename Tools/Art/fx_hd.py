"""건설 자리·투사체·원소 구슬·이펙트를 명암·외곽선 톤으로 (generate_art.py가 쓴다).

캔버스 크기와 무게중심은 예전 그림과 같다 (게임은 UILayout.csv 크기로 늘려 그리고, 건설 자리만 그림 크기를 쓴다).
이펙트(연기·폭발·불꽃·별)는 게임이 색을 곱해 입히므로 흰 바탕 + 무채색 그늘로 칠한다 → 어떤 색을 입혀도 명암이 산다.
"""

import math
import random

from paint import OUTLINE, Painter, bricks, grass_tuft, light_of, rgba, shade_of, tone, wood_planks
from units_hd import GOLD, STEEL, WOOD, WOOD_DARK, face

WHITE = (255, 255, 255, 255)
FX_SHADE = (196, 198, 204, 255)     # 색 입힐 이펙트의 그늘 (무채색이라 색을 해치지 않는다)
FX_DEEP = (150, 152, 160, 255)
FIRE = rgba("ff8a2a")
FIRE_CORE = rgba("ffe04a")
ICE = rgba("9fe3ff")
WATER = rgba("3f8dff")
SPARK = rgba("ffd93a")
POISON = rgba("7fd34a")
WIND = rgba("5fdcb4")
ARCANE = rgba("a07bff")
IRON = rgba("4c4a58")


def white_fx(p, mask, lw=None, form=0.32):
    p.paint(mask, WHITE, shade=FX_SHADE, light=WHITE, lw=lw, form=form, rim=1.4, glint=0)


def ball_face(p, cx, cy, s=1.0, mouth="o", x_eyes=False):
    """원소 구슬 얼굴: 냥코풍 점 눈 + 작은 입."""
    if x_eyes:
        for ex in (cx - 8 * s, cx + 8 * s):
            p.line([(ex - 3.4 * s, cy - 3.4 * s), (ex + 3.4 * s, cy + 3.4 * s)], OUTLINE, 2.4 * s)
            p.line([(ex + 3.4 * s, cy - 3.4 * s), (ex - 3.4 * s, cy + 3.4 * s)], OUTLINE, 2.4 * s)
    else:
        for ex in (cx - 8 * s, cx + 8 * s):
            p.dot(ex, cy, 3.4 * s, OUTLINE)
            p.dot(ex - 1.1 * s, cy - 1.2 * s, 1.1 * s, (255, 255, 255, 230))
    if mouth == "o":
        p.dot_e(cx, cy + 8 * s, 2.6 * s, 3.2 * s, OUTLINE)
    else:
        p.line([(cx - 3 * s, cy + 8 * s), (cx + 3 * s, cy + 8 * s)], OUTLINE, 2.0 * s)


# ---- 건설 자리 -----------------------------------------------------------------------------

def make_build_spot():
    """빈 건설 자리: 포석 깐 둥근 돌단 + 망치 그린 나무 표지판. 땅과 확실히 구분되게 밝은 돌로."""
    p = Painter(220, 156, lw=3.2)
    cx, cy = 110, 100
    rx, ry = 92, 38
    stone = rgba("d9d3c8")
    side = rgba("a9a196")
    # 돌단 옆면 (두께): 벽돌 띠
    body = p.union(p.m_ellipse(cx, cy + 13, rx, ry), p.m_rrect(cx - rx, cy, cx + rx, cy + 13, 0))
    p.paint(body, side, form=0, side=0.3, rim=1.6, glint=0, texture=bricks(cx - rx, cy - 4, cx + rx, cy + 52, 20, 13, side, seed=3))
    # 윗면: 가운데 둥근 돌 + 바깥 고리 돌 8장 (돌마다 명암)
    top = p.m_ellipse(cx, cy, rx, ry)
    p.paint(top, tone(stone, light=-0.25), form=0, rim=0, glint=0, lw=3.0)
    irx, iry = 48, 19
    for i in range(8):
        a0 = 2 * math.pi * i / 8 + 0.06
        a1 = 2 * math.pi * (i + 1) / 8 - 0.06
        pts = []
        for k in range(7):
            a = a0 + (a1 - a0) * k / 6
            pts.append((cx + (rx - 5) * math.cos(a), cy + (ry - 4) * math.sin(a)))
        for k in range(7):
            a = a1 - (a1 - a0) * k / 6
            pts.append((cx + (irx + 4) * math.cos(a), cy + (iry + 2) * math.sin(a)))
        c = tone(stone, light=random.Random(i).uniform(-0.08, 0.06))
        p.paint(p.m_poly(pts), c, outline=None, form=0.3, rim=1.0, glint=1.2, soft=0.4)
    p.paint(p.m_ellipse(cx, cy, irx, iry), tone(stone, light=0.12), outline=tone(stone, light=-0.45), lw=1.6, form=0.35, rim=1.0, glint=1.4)
    # 돌 틈 이끼와 풀
    for x, y in ((cx - 70, cy + 14), (cx + 62, cy + 20)):
        p.dot_e(x, y, 7, 3, rgba("7fb64a", 200))
    for x, sd in ((cx - rx + 6, 1), (cx + rx - 4, 2), (cx - 30, 3)):
        grass_tuft(p, x, cy + 26 + (sd == 3) * 22, 1.1, sd)
    # 표지판: 말뚝 + 노란 판 + 망치
    p.paint(p.m_rrect(cx - 4.5, 50, cx + 4.5, cy + 2, 2), WOOD_DARK, side=0.45, rim=0.8, glint=0.8)
    board = p.m_rrect(cx - 36, 6, cx + 36, 58, 9)
    yellow = rgba("ffcf3f")
    p.paint(board, yellow, side=0.22, rim=1.6, glint=1.6,
            texture=wood_planks(cx - 36, 6, cx + 36, 58, 13, yellow, vertical=False, seed=5))
    p.rivets([(cx - 29, 13), (cx + 29, 13), (cx - 29, 51), (cx + 29, 51)], r=1.5)
    handle = p.m_line([(cx - 15, 47), (cx + 7, 22)], 5.0)
    p.paint(handle, WOOD, form=0.3, rim=0.8, glint=0.8, lw=2.2)
    head = p.m_blob(cx + 9, 21, 14, 7, angle=-48, power=4.0)
    p.paint(head, STEEL, form=0.35, rim=1.0, glint=1.4, lw=2.2)
    p.shine(cx + 4, 18, 2.4, 1.2)
    return p.result()


# ---- 투사체 -------------------------------------------------------------------------------

def make_arrow():
    p = Painter(64, 20, lw=2.0)
    p.paint(p.m_line([(10, 10), (50, 10)], 3.0), WOOD, form=0.35, rim=0.6, glint=0.6)
    feather = rgba("e2493b")
    for dy in (-1, 1):
        p.paint(p.m_poly([(3, 10 + dy * 7), (15, 10 + dy * 1), (19, 10), (8, 10)]), feather, form=0.3, rim=0.5, glint=0.5, lw=1.6)
    p.paint(p.m_poly([(47, 3.5), (62, 10), (47, 16.5), (50, 10)]), STEEL, form=0.4, rim=0.6, glint=0.8, lw=1.8)
    p.shine(52, 8, 2.0, 0.8)
    return p.result()


def make_bomb():
    p = Painter(56, 60, lw=2.6)
    # 도화선 + 불똥
    p.paint(p.m_line([(36, 18), (40, 12), (44, 7)], 3.2), rgba("c9a46a"), form=0, rim=0.5, glint=0.5, lw=1.6)
    spark = [(46 + math.cos(k * math.pi / 4 - math.pi / 2) * (9 if k % 2 == 0 else 3.6),
              6 + math.sin(k * math.pi / 4 - math.pi / 2) * (9 if k % 2 == 0 else 3.6)) for k in range(8)]
    p.paint(p.m_poly(spark), FIRE_CORE, form=0.3, rim=0.5, glint=0, lw=1.6)
    p.dot(46, 6, 2.4, WHITE)
    # 쇠공 + 마개
    p.paint(p.m_rrect(29, 11, 41, 19, 2), STEEL, form=0.3, rim=0.6, glint=0.6, lw=2.0)
    p.paint(p.m_ellipse(28, 35, 21, 21), IRON, form=0.36, rim=1.6, glint=2.0, soft=0.8)
    p.shine(19, 26, 4.5, 3.2, alpha=170)
    p.shine(16, 31, 1.4, 1.4, alpha=200)
    for ex in (24, 36):
        p.dot(ex, 34, 3.2, WHITE)
        p.dot(ex + 0.8, 34.5, 1.9, OUTLINE)
    p.line([(24, 43), (36, 43)], (255, 255, 255, 220), 1.6)
    return p.result()


def make_magic_bolt():
    p = Painter(56, 56, lw=2.6)
    star = [(28 + math.cos(k * math.pi / 4 - math.pi / 2) * (25 if k % 2 == 0 else 9),
             28 + math.sin(k * math.pi / 4 - math.pi / 2) * (25 if k % 2 == 0 else 9)) for k in range(8)]
    p.paint(p.m_poly(star), ARCANE, form=0.35, rim=1.2, glint=1.6)
    p.ball(28, 28, 8, 8, rgba("f2eaff"), lw=0, form=0.3, rim=0, glint=0, shade=rgba("cbb8ff"))
    p.dot(25.5, 25.5, 2.4, WHITE)
    return p.result()


def make_ball(kind):
    """원소 구슬 (스펠·정령 제단 탄). 원소마다 모양이 다르고 얼굴이 있다."""
    p = Painter(80, 80, lw=3.0)
    cx, cy = 40, 42
    if kind == "Fire":
        flame = p.m_poly([(40, 3), (49, 18), (60, 14), (60, 30), (68, 44), (62, 64), (40, 75), (18, 64), (12, 44), (20, 28),
                          (22, 14), (31, 22)])
        p.paint(flame, FIRE, shade=rgba("e0401f"), form=0.32, rim=1.6, glint=1.6, soft=0.8)
        core = p.m_poly([(40, 30), (50, 44), (54, 58), (40, 68), (26, 58), (30, 44)])
        p.paint(core, FIRE_CORE, shade=rgba("ffb02a"), outline=None, form=0.3, rim=0, glint=1.2, soft=0.8)
        cy = 52
        ball_face(p, cx, cy - 2, 0.9)
        return p.result()
    if kind == "Ice":
        # 각진 얼음 결정: 면마다 밝기가 다르다
        pts = [(cx + 30 * math.cos(math.radians(a)), cy + 30 * math.sin(math.radians(a))) for a in range(-90, 270, 60)]
        p.paint(p.m_poly(pts), ICE, shade=rgba("5fb2e6"), form=0, rim=1.2, glint=0)
        for i in range(6):
            a, b = pts[i], pts[(i + 1) % 6]
            c = tone(ICE, light=(0.45, 0.25, 0.0, -0.2, -0.28, 0.1)[i])
            p.poly([(cx, cy), a, b], c)
        p.shine(cx - 14, cy - 12, 4, 2.2, alpha=200)
        ball_face(p, cx, cy, 0.95, mouth="line")
        return p.result()
    if kind == "Water":
        drop = p.m_poly([(40, 5)] + [(cx + 24 * math.cos(math.radians(a)), 48 + 24 * math.sin(math.radians(a))) for a in range(-30, 211, 10)])
        p.paint(drop, WATER, form=0.34, rim=1.6, glint=1.8, soft=0.8)
        p.shine(30, 36, 5, 8, alpha=190)
        p.shine(30, 50, 2, 2, alpha=200)
        ball_face(p, cx + 2, 50, 0.9)
        return p.result()
    if kind == "Lightning":
        spikes = 9
        star = [(cx + math.cos(math.pi * k / spikes - math.pi / 2) * (35 if k % 2 == 0 else 22),
                 cy + math.sin(math.pi * k / spikes - math.pi / 2) * (35 if k % 2 == 0 else 22)) for k in range(spikes * 2)]
        p.paint(p.m_poly(star), SPARK, shade=rgba("f2a21e"), form=0.32, rim=1.4, glint=1.6)
        bolt = p.m_poly([(47, 14), (33, 38), (42, 38), (30, 64), (52, 32), (43, 32), (52, 14)])
        p.paint(bolt, WHITE, shade=rgba("fff2b0"), outline=rgba("d98a14"), lw=1.4, form=0, rim=0, glint=0)
        ball_face(p, cx, cy, 0.85)
        return p.result()
    if kind == "Poison":
        blob = p.m_blob(cx, cy + 2, 26, 25, wobble=0.06, seed=4)
        drips = p.union(blob, p.m_ellipse(26, 66, 4, 6), p.m_ellipse(52, 68, 3.4, 5))
        p.paint(drips, POISON, shade=rgba("4f9f3a"), form=0.32, rim=1.6, glint=1.6, soft=0.8)
        for bx, by, r in ((66, 14, 6), (14, 20, 4.5), (58, 4, 3)):
            p.ball(bx, by, r, r, POISON, lw=2.0, form=0.3, rim=0.6, glint=0.8)
            p.dot(bx - r * 0.35, by - r * 0.35, r * 0.3, WHITE)
        for sx, sy in ((30, 30), (50, 52)):
            p.dot(sx, sy, 3.2, rgba("b6f07a"))
        ball_face(p, cx, cy, 0.9, x_eyes=True, mouth="line")
        return p.result()
    if kind == "Wind":
        p.paint(p.m_ellipse(cx, cy, 27, 27), WIND, form=0.34, rim=1.6, glint=1.6)
        for r, a0, a1 in ((20, 200, 420), (12, 30, 250)):
            p.arc(cx, cy, r, r, a0, a1, WHITE, 3.2)
        # 바람 꼬리: 구슬 바깥을 감도는 휘어진 줄
        for r, a0, a1 in ((34, 150, 220), (33, 330, 30)):
            p.arc(cx, cy, r, r * 0.9, a0, a1, OUTLINE, 4.6)
            p.arc(cx, cy, r, r * 0.9, a0 + 3, a1 - 3, WHITE, 2.4)
        ball_face(p, cx, cy, 0.9)
        return p.result()
    # Arcane: 보라 구슬 + 반짝 별
    p.paint(p.m_ellipse(cx, cy, 27, 27), ARCANE, form=0.34, rim=1.6, glint=1.8)
    p.shine(30, 30, 6, 4, alpha=150)
    for sx, sy, r in ((58, 20, 9), (18, 64, 6)):
        star = [(sx + math.cos(k * math.pi / 4 - math.pi / 2) * (r if k % 2 == 0 else r * 0.4),
                 sy + math.sin(k * math.pi / 4 - math.pi / 2) * (r if k % 2 == 0 else r * 0.4)) for k in range(8)]
        p.paint(p.m_poly(star), WHITE, shade=rgba("e6dcff"), lw=1.6, form=0.3, rim=0.4, glint=0)
    ball_face(p, cx, cy, 0.9)
    return p.result()


# ---- 이펙트 (흰색 + 무채색 그늘, 게임이 색을 곱한다) -----------------------------------------

def make_puff(frame):
    p = Painter(160, 160, lw=3.4)
    rng = random.Random(40 + frame)
    if frame < 2:
        radius = 30 + frame * 16
        circles = [(80 + radius * 0.7 * math.cos(t), 80 + radius * 0.7 * math.sin(t), radius * rng.uniform(0.5, 0.6))
                   for t in [2 * math.pi * i / 7 for i in range(7)]]
        circles.append((80, 80, radius * 0.8))
        cloud = p.union(*[p.m_ellipse(x, y, r, r) for x, y, r in circles])
        white_fx(p, cloud)
        # 안쪽 뭉게 결
        for x, y, r in circles[:4]:
            p.arc(x + r * 0.1, y + r * 0.1, r * 0.6, r * 0.6, 20, 110, FX_SHADE, 2.4)
    else:
        spread = 46 + (frame - 2) * 14
        size = 11 - (frame - 2) * 5
        for i in range(7):
            t = 2 * math.pi * i / 7 + 0.3
            white_fx(p, p.m_ellipse(80 + spread * math.cos(t), 80 + spread * math.sin(t), size, size), lw=2.6)
    return p.result()


def make_burst():
    p = Painter(160, 160, lw=3.6)
    n = 11
    star = [(80 + math.cos(math.pi * k / n) * (74 if k % 2 == 0 else 40), 80 + math.sin(math.pi * k / n) * (74 if k % 2 == 0 else 40))
            for k in range(n * 2)]
    white_fx(p, p.m_poly(star), form=0.22)
    inner = [(80 + math.cos(math.pi * k / n + 0.14) * (46 if k % 2 == 0 else 26), 80 + math.sin(math.pi * k / n + 0.14) * (46 if k % 2 == 0 else 26))
             for k in range(n * 2)]
    p.poly(inner, (255, 255, 255, 255))
    p.dot(80, 80, 16, (255, 255, 255, 255))
    return p.result()


def make_star_fx():
    p = Painter(48, 48, lw=2.4)
    star = [(24 + math.cos(k * math.pi / 4 - math.pi / 2) * (21 if k % 2 == 0 else 8),
             24 + math.sin(k * math.pi / 4 - math.pi / 2) * (21 if k % 2 == 0 else 8)) for k in range(8)]
    white_fx(p, p.m_poly(star))
    return p.result()


def make_ice_block():
    """얼린 몬스터를 덮는 반투명 얼음 덩어리 (색 그대로 그린다)."""
    p = Painter(120, 132, lw=3.0)
    outer = [(18, 34), (60, 10), (102, 30), (108, 104), (62, 124), (14, 106)]
    m = p.m_poly(outer)
    p.paint(m, rgba("bfeaff", 170), shade=rgba("7cc6f0", 190), light=rgba("ffffff", 210), form=0, side=0.35, rim=1.6, glint=2.0)
    # 면 나누기: 위쪽 밝은 면, 가운데 모서리
    p.poly([(18, 34), (60, 10), (102, 30), (62, 46)], (255, 255, 255, 110))
    p.line([(62, 46), (62, 122)], (255, 255, 255, 170), 2.0, cap=False)
    p.line([(18, 34), (62, 46), (102, 30)], (255, 255, 255, 190), 2.0, cap=False)
    p.line([(30, 52), (42, 46)], WHITE, 3.0)
    p.line([(28, 64), (33, 61)], WHITE, 3.0)
    p.line([(80, 80), (92, 70), (96, 84)], (90, 160, 210, 200), 1.4, cap=False)
    return p.result()


def make_flame_fx():
    p = Painter(48, 56, lw=2.4)
    flame = p.m_poly([(24, 2), (31, 14), (38, 12), (44, 32), (38, 50), (24, 54), (10, 50), (4, 34), (10, 22), (16, 28), (18, 14)])
    white_fx(p, flame, form=0.3)
    core = p.m_poly([(24, 24), (31, 36), (30, 47), (24, 50), (18, 47), (17, 36)])
    p.paint(core, WHITE, shade=WHITE, outline=None, form=0, rim=0, glint=0)
    p.line([(20, 38), (22, 32)], FX_SHADE, 1.6)
    return p.result()
