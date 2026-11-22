"""자세(pose)로 그리는 애니메이션: 병졸·꼬마 도깨비·돌진 닭 (generate_art.py가 쓴다).

예전 그림은 다리 두 장 번갈아 = 걷기 두 장뿐이었다. 여기서는 몸을 부품(다리·팔·무기·머리…)으로 나누고
자세 값(발 위치, 몸 들썩임, 무기 각도, 눈 깜빡임…)을 프레임마다 바꿔 그린 뒤,
마지막에 그림 전체를 발 기준으로 눌렀다 늘이고(squash & stretch) 기울인다.

동작 종류와 프레임 수·속도·타격 프레임은 Content/YJJDefense/Data/Anims.csv (게임과 같은 표를 읽는다).
  Idle   : 숨쉬기·눈 깜빡임 (반복)
  Walk   : 걷기·달리기 (반복)
  Attack : 준비 → 타격(HitFrame) → 되돌아오기. 게임이 공격 간격에 맞춰 넘긴다
"""

import csv
import math
import os

from PIL import Image

from paint import OUTLINE, Painter, rgba, tone, wood_planks
from units_hd import GOLD, SKIN, STEEL, TEAM, TEAM_RED, WOOD, WOOD_DARK, IMP_SKIN, TIGER, face
from monsters_hd import BEAK, WHITE, brow, m_cloud

ANIMS_CSV = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", "Content", "YJJDefense", "Data", "Anims.csv")
BOOT = rgba("6b4a8c")
DUST = (236, 226, 204, 200)


def read_anims():
    """Anims.csv → {이름: (프레임 수, 타격 프레임)}"""
    out = {}
    with open(ANIMS_CSV, encoding="utf-8-sig", newline="") as f:
        for row in csv.DictReader(f):
            out[row["Name"]] = (int(row["Frames"]), int(row["HitFrame"]))
    return out


def posed(img, feet, sx=1.0, sy=1.0, tilt=0.0):
    """발(feet)을 고정점으로 눌렀다 늘이고(sx, sy) 기울인다(tilt°, +면 앞으로)."""
    if sx == 1.0 and sy == 1.0 and tilt == 0.0:
        return img
    fx, fy = feet
    t = math.radians(tilt)
    c, s = math.cos(t), math.sin(t)
    a, b = c / sx, s / sx
    d, e = -s / sy, c / sy
    matrix = (a, b, fx - a * fx - b * fy, d, e, fy - d * fx - e * fy)
    return img.convert("RGBa").transform(img.size, Image.AFFINE, matrix, resample=Image.BICUBIC).convert("RGBA")


def dust(p, x, y, s=1.0):
    """발 밑에서 피어오르는 흙먼지 세 덩이."""
    for dx, dy, r in ((0, 0, 4.2), (-6, -2, 3.2), (-11, 1, 2.4)):
        p.dot_e(x + dx * s, y + dy * s, r * s + 0.9, r * 0.75 * s + 0.9, (120, 96, 70, 110))
        p.dot_e(x + dx * s, y + dy * s, r * s, r * 0.75 * s, DUST)


def speed_lines(p, x, y, length, gap=6.0, n=3):
    for i in range(n):
        yy = y + (i - (n - 1) / 2) * gap
        x0 = x - length * (0.6 + 0.4 * ((i + 1) % 2))
        p.line([(x0, yy), (x, yy)], (255, 255, 255, 210), 2.2)


def impact(p, x, y, r=9.0):
    """맞는 순간 번쩍 (별 + 짧은 선)."""
    pts = [(x + math.cos(math.pi * k / 5 - math.pi / 2) * (r if k % 2 == 0 else r * 0.45),
            y + math.sin(math.pi * k / 5 - math.pi / 2) * (r if k % 2 == 0 else r * 0.45)) for k in range(10)]
    p.paint(p.m_poly(pts), rgba("fff3a0"), shade=rgba("ffd23a"), lw=1.8, form=0.25, rim=0, glint=0)
    for k in range(6):
        a = math.pi * 2 * k / 6 + 0.3
        p.line([(x + math.cos(a) * r * 1.3, y + math.sin(a) * r * 1.3), (x + math.cos(a) * r * 1.9, y + math.sin(a) * r * 1.9)], OUTLINE, 1.6)


def closed_eyes(p, cx, cy, gap, look=3.0, w=2.6):
    for sx in (-1, 1):
        ex = cx + sx * gap + look
        p.arc(ex, cy - 1, w, 1.8, 20, 160, OUTLINE, 1.6)


def cycle(i, n):
    return 2 * math.pi * i / n


# ---- 병졸 ---------------------------------------------------------------------------------

def soldier(pose):
    """병졸 한 장. pose 키: bob, lean, legs[(dx, lift)×2], spear(grip, angle, front), arm_front, blink, mouth, fx."""
    p = Painter(120, 112, lw=2.6)
    bob = pose.get("bob", 0.0)
    lean = pose.get("lean", 0.0)
    cx = 58 + lean * 0.5

    # 다리: 뒷다리 → 앞다리. 엉덩이에서 발까지 + 장화
    for (hx, (dx, lift)) in zip((52, 64), pose["legs"]):
        hip = (hx + lean * 0.3, 86 + bob * 0.6)
        foot = (hx + dx, 100 - lift)
        p.paint(p.m_line([hip, foot], 8), BOOT, form=0.25, rim=1.0, glint=0)
        p.paint(p.m_blob(foot[0] + 2, foot[1] + 2, 7.5, 4.6, power=2.6), WOOD_DARK, form=0.3, rim=1.0, glint=1.2)
    for x, y in pose.get("dust", []):
        dust(p, x, y)

    (gx, gy), angle, spear_front = pose["spear"]
    dx, dy = math.sin(math.radians(angle)), -math.cos(math.radians(angle))

    def draw_spear():
        back, front = pose.get("spear_len", (24, 46))
        tail = (gx - dx * back, gy - dy * back)
        end = (gx + dx * front, gy + dy * front)
        p.paint(p.m_line([tail, end], 3.2), WOOD, form=0.0, rim=0.8, glint=0.8, lw=2.0)
        px, py = -dy, dx
        tip = (end[0] + dx * 14, end[1] + dy * 14)
        blade = [(end[0] + px * 4.6, end[1] + py * 4.6), tip, (end[0] - px * 4.6, end[1] - py * 4.6), (end[0] - dx * 2, end[1] - dy * 2)]
        p.paint(p.m_poly(blade), STEEL, form=0.35, rim=0.8, glint=1.2, lw=2.0)
        p.shine(end[0] + dx * 5 + px * 1.5, end[1] + dy * 5 + py * 1.5, 1.6, 1.0, 200)
        # 빨간 술
        sway = pose.get("tassel", 0.0)
        p.paint(p.m_poly([(end[0], end[1]), (end[0] - dx * 3 + px * 5 + sway, end[1] - dy * 3 + py * 5 + 6),
                          (end[0] - dx * 5 - px * 2 + sway, end[1] - dy * 5 - py * 2 + 8)]), TEAM_RED, form=0.3, rim=0.5, glint=0, lw=1.6)
        return tip

    tip = None
    if not spear_front:
        tip = draw_spear()

    # 몸통 (파란 조끼 + 허리띠)
    by = 76 + bob
    body = p.m_blob(cx, by, 17, 15 * pose.get("chest", 1.0), power=2.4)
    p.paint(body, TEAM, form=0.32)
    p.clip(body, lambda q: (
        q.poly([(cx - 18, by + 4), (cx + 20, by + 4), (cx + 20, by + 9), (cx - 18, by + 9)], WOOD_DARK),
        q.poly([(cx - 3, by + 3.5), (cx + 3, by + 3.5), (cx + 3, by + 9.5), (cx - 3, by + 9.5)], GOLD),
        q.line([(cx, by - 13), (cx, by + 3)], tone(TEAM, light=0.35), 2.0, cap=False),
    ))

    # 머리
    hx, hy = 59 + lean, 48 + bob + pose.get("head_bob", 0.0)
    p.paint(p.m_ellipse(hx, hy, 19, 18), SKIN, form=0.28, rim=1.2, glint=1.6)
    if pose.get("blink"):
        closed_eyes(p, hx + 2, hy + 3, 6.5, look=3)
        p.line([(hx + 3.5, hy + 9), (hx + 6.5, hy + 9)], OUTLINE, 1.6)
    else:
        face(p, hx + 2, hy + 3, look=3, eye_gap=6.5, eye_r=2.3, mouth=pose.get("mouth", "line"))
    # 냄비 투구 + 술
    plume = pose.get("plume", 0.0)
    p.paint(p.m_poly([(hx - 15 - plume, hy - 18 - abs(plume) * 0.3), (hx - 3, hy - 22), (hx, hy - 32), (hx + 3, hy - 22)]), TEAM_RED, form=0.3, lw=2.0)
    pot = p.m_rrect(hx - 18, hy - 22, hx + 18, hy - 4, 6)
    p.paint(pot, STEEL, form=0.4, rim=1.2, glint=2.0)
    p.paint(p.m_rrect(hx - 22, hy - 7, hx + 22, hy - 1, 3), tone(STEEL, light=-0.1), form=0.3, glint=1.0)
    for sx in (-1, 1):
        p.paint(p.m_rrect(hx + sx * 22 - 3 + sx * 3, hy - 15, hx + sx * 22 + 3 + sx * 3, hy - 9, 2.5), tone(STEEL, light=-0.2), form=0, glint=0, lw=1.8)
    p.rivets([(hx - 10, hy - 4), (hx, hy - 4), (hx + 10, hy - 4)], r=1.1)
    p.shine(hx - 9, hy - 16, 3.5, 2.0, 170)

    # 방패 (왼팔에)
    sx_, sy_ = pose.get("shield", (37 + lean, 78 + bob))
    p.paint(p.m_ellipse(sx_, sy_, 14, 15), WOOD, form=0.3, glint=1.6, texture=wood_planks(sx_ - 14, sy_ - 15, sx_ + 14, sy_ + 15, 6.5, WOOD, seed=3))
    rim = p.cut(p.m_ellipse(sx_, sy_, 14, 15), p.m_ellipse(sx_, sy_, 10.5, 11.5))
    p.paint(rim, STEEL, form=0.25, rim=0.8, glint=1.0, lw=1.6)
    p.paint(p.m_ellipse(sx_, sy_, 5.5, 6), TEAM, form=0.3, glint=1.0, lw=1.6)
    p.rivets([(sx_, sy_ - 12.8), (sx_ - 12, sy_), (sx_ + 12, sy_), (sx_, sy_ + 12.8)], r=1.0)

    if spear_front:
        tip = draw_spear()
    # 창 든 팔 (소매 + 손)
    shoulder = (cx + 11, by - 7)
    p.paint(p.m_line([shoulder, (gx, gy)], 6.5), tone(TEAM, light=-0.08), form=0.2, rim=0.6, glint=0.8, lw=2.0)
    p.ball(gx, gy, 4.4, 4.2, SKIN, form=0.3, rim=0.6, glint=0.8, lw=2.0)

    if pose.get("fx") == "thrust" and tip:
        speed_lines(p, tip[0] - 20, tip[1], 26)
        impact(p, min(tip[0] + 2, 112), tip[1], 7)
    return posed(p.result(), (60, 106), *pose.get("squash", (1.0, 1.0)), pose.get("tilt", 0.0))


def soldier_idle(i, n):
    t = cycle(i, n)
    breath = math.sin(t)
    return soldier(dict(bob=-0.9 * breath, legs=[(0, 0), (0, 0)], spear=((76, 78 - 0.9 * breath), 6 + 1.6 * breath, False),
                        chest=1.0 + 0.03 * breath, blink=(i == n - 2), plume=1.5 * math.sin(t + 1.0), tassel=1.2 * math.sin(t + 0.6),
                        squash=(1.0 - 0.012 * breath, 1.0 + 0.018 * breath)))


def soldier_walk(i, n):
    t = cycle(i, n)
    s = math.sin(t)
    c = math.cos(t)
    stride = 8.0
    legs = [(-stride * s, max(0.0, -c) * 5.0), (stride * s, max(0.0, c) * 5.0)]
    bob = -2.2 * abs(math.cos(t))          # 다리가 모일 때 가장 높다
    contact = [i == 0, i == n // 2]
    dust_at = [(58 - stride - 6, 102)] if contact[0] or contact[1] else []
    return soldier(dict(bob=bob, lean=2.5, legs=legs, dust=dust_at,
                        spear=((77, 77 + bob + 1.2 * s), 14 + 5 * s, False),
                        shield=(38 + 2.5 + 1.5 * s, 78 + bob + 1.0 * math.sin(2 * t)),
                        plume=3.0 * s, tassel=3.0 * s, head_bob=0.6 * math.sin(2 * t), tilt=3.0,
                        squash=(1.0 + 0.02 * math.cos(2 * t), 1.0 - 0.025 * math.cos(2 * t))))


def soldier_attack(i, n, hit):
    # 준비 → 뒤로 당김 → 웅크림 → 찌르기(타격) → 멈춤 → 되돌림
    keys = [
        dict(grip=(66, 76), angle=68, lean=0, legs=[(-2, 0), (3, 0)], squash=(1.0, 1.0), tilt=0, mouth="line"),
        dict(grip=(54, 74), angle=76, lean=-3, legs=[(-5, 0), (2, 0)], squash=(1.02, 0.98), tilt=-4, mouth="line"),
        dict(grip=(48, 74), angle=80, lean=-5, legs=[(-6, 0), (2, 0)], squash=(1.05, 0.95), tilt=-6, mouth="o"),
        dict(grip=(76, 72), angle=86, lean=6, legs=[(-6, 0), (8, 0)], squash=(0.96, 1.05), tilt=7, mouth="o", fx="thrust"),
        dict(grip=(72, 72), angle=86, lean=5, legs=[(-6, 0), (8, 0)], squash=(0.98, 1.02), tilt=5, mouth="grin"),
        dict(grip=(68, 75), angle=74, lean=1, legs=[(-3, 0), (4, 0)], squash=(1.0, 1.0), tilt=1, mouth="line"),
    ]
    k = keys[min(i * len(keys) // n, len(keys) - 1)] if n != len(keys) else keys[i]
    return soldier(dict(bob=0.0, lean=k["lean"], legs=k["legs"], spear=(k["grip"], k["angle"], True), spear_len=(30, 34),
                        shield=(36 + k["lean"] * 0.6, 79), mouth=k["mouth"], fx=k.get("fx"), plume=-k["lean"] * 0.8,
                        tassel=-k["lean"] * 0.6, squash=k["squash"], tilt=k["tilt"]))


# ---- 꼬마 도깨비 ----------------------------------------------------------------------------

def imp(pose):
    """꼬마 도깨비 한 장 (164x136, 발 = (82, 130))."""
    p = Painter(164, 136, lw=2.8)
    bob = pose.get("bob", 0.0)
    X = lambda v: v + 26           # 예전 112x124 그림 좌표 → 새 캔버스
    Y = lambda v: v + 12 + bob

    # 다리
    for hx, (dx, lift) in zip((48, 64), pose["legs"]):
        hip = (X(hx), Y(94))
        foot = (X(hx) + dx, 126 - lift)
        p.paint(p.m_line([hip, foot], 9), IMP_SKIN, form=0.3, glint=0.8)
        p.paint(p.m_blob(foot[0] + 2, foot[1], 7.5, 4.5, power=2.4), tone(IMP_SKIN, light=-0.35), form=0.3, glint=1.0)
    for x, y in pose.get("dust", []):
        dust(p, x, y)

    # 뒷팔
    g = math.radians(pose.get("back_arm", -45))
    s0 = (X(28), Y(68))
    p.paint(p.m_line([s0, (s0[0] + 18 * math.sin(g), s0[1] + 18 * math.cos(g))], 7.5), tone(IMP_SKIN, light=-0.12), form=0.2, glint=0.8)

    # 몸 + 배
    squash_body = pose.get("belly", 1.0)
    body = p.m_blob(X(56), Y(66), 37, 33 * squash_body, power=2.2)
    p.paint(body, IMP_SKIN, form=0.3, rim=1.8, glint=2.2)
    p.paint(p.m_ellipse(X(60), Y(76), 18, 12 * squash_body), tone(IMP_SKIN, light=0.3), form=0.25, rim=0.8, glint=0, lw=0)

    # 호피 치마 (걸을 때 살랑)
    sw = pose.get("skirt", 0.0)
    skirt = p.m_poly([(X(26), Y(82)), (X(88), Y(82)), (X(92) + sw, Y(98)), (X(82) + sw, Y(94)), (X(72) + sw, Y(100)), (X(60) + sw, Y(94)),
                      (X(48) + sw, Y(100)), (X(36) + sw, Y(94)), (X(22) + sw, Y(98))])

    def stripes(q):
        for k, x in enumerate(range(24, 96, 9)):
            q.poly([(X(x), Y(82)), (X(x) + 4, Y(82)), (X(x) + 1 + (k % 2) * 2 + sw, Y(99)), (X(x) - 2 + sw, Y(99))], rgba("4a2a1a"))
    p.paint(skirt, TIGER, form=0.3, rim=1.2, glint=1.0, texture=stripes)

    # 뿔 + 머리털
    hb = pose.get("horn", 0.0)
    horn = p.m_poly([(X(47), Y(38)), (X(58) + hb, Y(6)), (X(71), Y(38))])

    def rings(q):
        for yy in (16, 24, 32):
            q.line([(X(44), Y(yy)), (X(74), Y(yy + 2))], rgba("d88a1a"), 2.2, cap=False)
    p.paint(horn, GOLD, form=0.35, glint=1.6, texture=rings)
    p.paint(p.m_poly([(X(36), Y(40)), (X(42) + hb * 0.5, Y(30)), (X(46), Y(38)), (X(50) + hb * 0.5, Y(28)), (X(52), Y(37))]), rgba("3a2a3a"), form=0.2, glint=1.0, lw=2.0)

    # 눈 (왕눈) + 입
    look = pose.get("look", 3.0)
    for ex, er in ((48, 9.5), (70, 10.5)):
        if pose.get("blink"):
            p.paint(p.m_ellipse(X(ex), Y(56), er, 2.6), tone(IMP_SKIN, light=-0.2), form=0, rim=0, glint=0, lw=2.0)
            continue
        p.paint(p.m_ellipse(X(ex), Y(55), er, er + 0.5), rgba("fffaf0"), form=0.25, rim=0.8, glint=0, lw=2.2)
        p.dot(X(ex) + look, Y(56), er * 0.42, OUTLINE)
        p.dot(X(ex) + look - 1.4, Y(54), er * 0.15, (255, 255, 255, 240))
    if pose.get("angry"):
        brow(p, X(38), Y(42), X(54), Y(47), 3.4)
        brow(p, X(80), Y(42), X(64), Y(47), 3.4)
    mouth_h = pose.get("mouth", 4.6)
    p.paint(p.m_blob(X(62), Y(71) + mouth_h * 0.3, 9, mouth_h, power=2.2), rgba("6b1f2a"), form=0, rim=0, glint=0, lw=2.0)
    p.paint(p.m_poly([(X(55), Y(68)), (X(58), Y(75)), (X(60), Y(68))]), rgba("fffaf0"), form=0, rim=0, glint=0, lw=1.4)
    p.paint(p.m_poly([(X(65), Y(68)), (X(67), Y(74)), (X(70), Y(68))]), rgba("fffaf0"), form=0, rim=0, glint=0, lw=1.4)
    p.dot_e(X(38), Y(66), 4.5, 2.5, rgba("ff4f5a", 120))

    # 앞팔 + 방망이
    s1 = (X(84), Y(68))
    b = math.radians(pose.get("arm", 60))
    hand = (s1[0] + 14 * math.sin(b), s1[1] + 14 * math.cos(b))
    a = math.radians(pose["club"])
    cd = (math.sin(a), -math.cos(a))
    head = (hand[0] + cd[0] * 32, hand[1] + cd[1] * 32)
    handle_end = (hand[0] - cd[0] * 6, hand[1] - cd[1] * 6)
    p.paint(p.m_line([handle_end, head], 8.5), WOOD, form=0.3, glint=1.2,
            texture=wood_planks(min(hand[0], head[0]) - 20, min(hand[1], head[1]) - 20, max(hand[0], head[0]) + 20, max(hand[1], head[1]) + 20, 60, WOOD, seed=5))
    hc = (head[0] + cd[0] * 3, head[1] + cd[1] * 3)
    club_head = p.m_blob(hc[0], hc[1], 11, 8.5, angle=math.degrees(a) - 90 + 90)
    p.paint(club_head, WOOD, form=0.35, glint=1.6)
    px, py = -cd[1], cd[0]
    for along, side in ((-5, -1), (0, 1), (5, -1), (3, 1.2), (-3, 1.2)):
        sx = hc[0] + cd[0] * along + px * side * 8
        sy = hc[1] + cd[1] * along + py * side * 8
        ox, oy = px * side * 4.5, py * side * 4.5
        p.paint(p.m_poly([(sx - cd[0] * 1.8, sy - cd[1] * 1.8), (sx + ox, sy + oy), (sx + cd[0] * 1.8, sy + cd[1] * 1.8)]), STEEL, form=0, rim=0, glint=0.6, lw=1.4)
    p.paint(p.m_line([s1, hand], 7.5), IMP_SKIN, form=0.2, glint=0.8)
    p.ball(hand[0], hand[1], 5.5, 5, IMP_SKIN, form=0.3, glint=1.0)

    if pose.get("fx") == "slam":
        impact(p, min(hc[0] + cd[0] * 8, 152), min(hc[1] + cd[1] * 8, 126), 9)
    if pose.get("fx") == "whoosh":
        for k in range(3):
            r = 34 + k * 5
            p.arc(hand[0], hand[1], r, r, -100 + k * 6, -40 + k * 6, (255, 255, 255, 200 - k * 50), 2.4)
    return posed(p.result(), (82, 130), *pose.get("squash", (1.0, 1.0)), pose.get("tilt", 0.0))


def imp_idle(i, n):
    t = cycle(i, n)
    breath = math.sin(t)
    return imp(dict(bob=-1.2 * breath, legs=[(0, 0), (0, 0)], club=18 + 3 * breath, arm=60 - 3 * breath, back_arm=-40 + 4 * breath,
                    belly=1.0 + 0.03 * breath, blink=(i == n - 1), look=3.0 if i < n // 2 else 1.0, mouth=4.6 + 0.6 * breath,
                    squash=(1.0 - 0.02 * breath, 1.0 + 0.03 * breath)))


def imp_walk(i, n):
    t = cycle(i, n)
    s, c = math.sin(t), math.cos(t)
    stride = 7.0
    legs = [(stride * s, max(0.0, c) * 6.0), (-stride * s, max(0.0, -c) * 6.0)]
    bob = -3.2 * abs(c)
    dust_at = [(68, 128)] if i in (0, n // 2) else []
    return imp(dict(bob=bob, legs=legs, dust=dust_at, club=22 + 8 * math.sin(2 * t), arm=58 + 6 * math.sin(2 * t), back_arm=-45 + 18 * s,
                    skirt=-2.5 * s, horn=1.2 * s, look=3.5, mouth=4.0 + 1.4 * abs(s), tilt=4.0 * s + 2.0,
                    squash=(1.0 + 0.035 * math.cos(2 * t), 1.0 - 0.045 * math.cos(2 * t))))


def imp_attack(i, n, hit):
    keys = [
        dict(club=24, arm=58, tilt=0, squash=(1.0, 1.0), mouth=4.6, back_arm=-45),
        dict(club=-30, arm=150, tilt=-5, squash=(0.97, 1.05), mouth=7.0, back_arm=-70, angry=True),
        dict(club=-60, arm=165, tilt=-8, squash=(0.95, 1.08), mouth=8.0, back_arm=-80, angry=True, fx="whoosh"),
        dict(club=112, arm=70, tilt=8, squash=(1.1, 0.88), mouth=8.0, back_arm=-20, angry=True, fx="slam"),
        dict(club=104, arm=72, tilt=6, squash=(1.05, 0.94), mouth=6.0, back_arm=-25, angry=True),
        dict(club=45, arm=62, tilt=2, squash=(1.0, 1.0), mouth=4.6, back_arm=-40),
    ]
    k = keys[i] if n == len(keys) else keys[min(i * len(keys) // n, len(keys) - 1)]
    return imp(dict(bob=0.0, legs=[(-3, 0), (4, 0)], club=k["club"], arm=k["arm"], back_arm=k["back_arm"], mouth=k["mouth"],
                    angry=k.get("angry", False), fx=k.get("fx"), look=4.0, tilt=k["tilt"], squash=k["squash"]))


# ---- 돌진 닭 ------------------------------------------------------------------------------

def chicken(pose):
    """돌진 닭 한 장 (170x124, 발 = (85, 116))."""
    p = Painter(170, 124, lw=2.6)
    bob = pose.get("bob", 0.0)
    X = lambda v: v + 27
    Y = lambda v: v + 8 + bob

    # 다리 (엉덩이 → 무릎 → 발) + 발가락
    for (hx, (dx, lift)) in zip((46, 62), pose["legs"]):
        hip = (X(hx), Y(76))
        foot = (X(hx) + dx, 110 - lift)
        knee = ((hip[0] + foot[0]) / 2 + 5, (hip[1] + foot[1]) / 2 - 2)
        p.paint(p.m_line([hip, knee, foot], 4), BEAK, form=0, rim=0.6, glint=0.6, lw=2.0)
        spread = 1.0 if lift < 2 else 0.5
        for tdx in (-6, 0, 6):
            p.paint(p.m_line([foot, (foot[0] + (tdx + 4) * spread, foot[1] + 3)], 2.6), BEAK, form=0, rim=0, glint=0, lw=1.6)
    for x, y in pose.get("dust", []):
        dust(p, x, y, 1.1)

    # 꼬리 깃 (흔들림)
    ts = pose.get("tail", 0.0)
    tail = p.m_poly([(X(24), Y(46)), (X(4) + ts * 0.5, Y(30) - ts), (X(12), Y(48)), (X(0) + ts * 0.3, Y(56) - ts * 0.5), (X(14), Y(60)),
                     (X(4), Y(70) + ts * 0.4), (X(26), Y(64))])
    p.paint(tail, WHITE, form=0.35, glint=1.4)
    # 볏 + 몸
    cw = pose.get("comb", 0.0)
    p.paint(m_cloud(p, [(X(60) - cw, Y(20), 8), (X(71) - cw * 0.6, Y(14) - abs(cw) * 0.3, 9.5), (X(83), Y(19), 8)]), TEAM_RED, form=0.35, glint=1.4)
    body = p.m_blob(X(56), Y(54), 38, 29 * pose.get("puff", 1.0), angle=-12)
    p.paint(body, WHITE, form=0.35, rim=1.8, glint=2.0)
    # 부리 (벌리면 위아래로 갈라진다)
    op = pose.get("beak", 0.0)
    p.paint(p.m_poly([(X(92), Y(44)), (X(113), Y(51) - op * 0.4), (X(92), Y(52))]), BEAK, form=0.35, glint=1.0, lw=2.4)
    p.paint(p.m_poly([(X(92), Y(52) + op * 0.3), (X(110), Y(52) + op), (X(92), Y(59) + op * 0.5)]), tone(BEAK, light=-0.1), form=0.3, glint=0.6, lw=2.4)
    if op > 1:
        p.paint(p.m_poly([(X(93), Y(52)), (X(104), Y(52) + op * 0.4), (X(93), Y(53) + op * 0.4)]), rgba("8a2a2a"), form=0, rim=0, glint=0, lw=0)
    p.paint(p.m_ellipse(X(90) + pose.get("wattle", 0.0), Y(64) + op * 0.4, 4.5, 6), TEAM_RED, form=0.3, glint=0.8, lw=2.0)
    # 눈 + 눈썹
    if pose.get("blink"):
        p.arc(X(79), Y(44), 3.4, 2, 20, 160, OUTLINE, 1.8)
    else:
        p.dot(X(79), Y(44), 3.6, OUTLINE)
        p.dot(X(78), Y(42.8), 1.2, (255, 255, 255, 240))
    brow(p, X(69), Y(35) + pose.get("brow", 0.0), X(86), Y(39), 3.2)
    # 날개 (어깨 기준으로 퍼덕)
    wa = math.radians(pose.get("wing", 0.0))
    shoulder = (X(46), Y(52))

    def rot(x, y):
        dx, dy = x - shoulder[0], y - shoulder[1]
        return (shoulder[0] + dx * math.cos(wa) - dy * math.sin(wa), shoulder[1] + dx * math.sin(wa) + dy * math.cos(wa))
    wing = p.m_poly([rot(X(44), Y(48)), rot(X(24), Y(58)), rot(X(30), Y(62)), rot(X(26), Y(68)), rot(X(48), Y(68))])
    p.paint(wing, tone(WHITE, light=-0.04), shade=rgba("c9c4dc"), form=0.35, glint=1.0, lw=2.4)
    for k in range(2):
        a0 = rot(X(32 + k * 6), Y(61 + k))
        a1 = rot(X(42 + k * 3), Y(64 + k))
        p.line([a0, a1], rgba("b4aecb"), 1.2)

    if pose.get("fx") == "peck":
        impact(p, X(118), Y(56), 8)
    if pose.get("fx") == "speed":
        speed_lines(p, X(4), Y(50), 22, gap=8)
    return posed(p.result(), (85, 116), *pose.get("squash", (1.0, 1.0)), pose.get("tilt", 0.0))


def chicken_idle(i, n):
    t = cycle(i, n)
    s = math.sin(t)
    return chicken(dict(bob=-0.8 * s, legs=[(0, 0), (0, 0)], tail=2.0 * s, comb=0.8 * s, wing=-2 * s, beak=1.5 if i == 1 else 0.0,
                        blink=(i == n - 1), puff=1.0 + 0.03 * s, tilt=2.0 * math.sin(t + 0.8)))


def chicken_walk(i, n):
    # 전력 질주: 다리가 원을 그리고, 몸은 앞으로 기울고, 날개를 퍼덕인다
    t = cycle(i, n)
    s, c = math.sin(t), math.cos(t)
    stride = 13.0
    legs = [(stride * c, max(0.0, s) * 9.0), (-stride * c, max(0.0, -s) * 9.0)]
    return chicken(dict(bob=-3.0 * abs(s), legs=legs, dust=[(56, 112)] if i in (1, 1 + n // 2) else [],
                        tail=-4 + 3 * math.sin(2 * t), comb=3.0 * math.sin(2 * t + 0.5), wing=-28 + 26 * math.sin(2 * t), beak=2.0,
                        wattle=-1.5 * math.sin(2 * t), brow=1.0, fx="speed" if i % 2 == 0 else None, tilt=9.0 + 2.0 * math.sin(2 * t),
                        squash=(1.0 + 0.04 * math.cos(2 * t), 1.0 - 0.05 * math.cos(2 * t))))


def chicken_attack(i, n, hit):
    keys = [
        dict(tilt=3, beak=0, wing=-6, squash=(1.0, 1.0), puff=1.0),
        dict(tilt=-10, beak=4, wing=-30, squash=(0.96, 1.05), puff=1.06),
        dict(tilt=-14, beak=6, wing=-40, squash=(0.94, 1.07), puff=1.08),
        dict(tilt=18, beak=7, wing=-10, squash=(1.08, 0.92), puff=0.96, fx="peck"),
        dict(tilt=14, beak=3, wing=-8, squash=(1.04, 0.96), puff=0.98),
        dict(tilt=5, beak=0, wing=-4, squash=(1.0, 1.0), puff=1.0),
    ]
    k = keys[i] if n == len(keys) else keys[min(i * len(keys) // n, len(keys) - 1)]
    return chicken(dict(legs=[(-4, 0), (6, 0)], tail=-k["tilt"] * 0.3, comb=-k["tilt"] * 0.2, wing=k["wing"], beak=k["beak"], brow=1.5,
                        puff=k["puff"], fx=k.get("fx"), tilt=k["tilt"], squash=k["squash"]))


# ---- 모으기 ---------------------------------------------------------------------------------

ANIM_DRAW = {
    "Soldier_Idle": soldier_idle, "Soldier_Walk": soldier_walk, "Soldier_Attack": soldier_attack,
    "Enemy_Imp_Idle": imp_idle, "Enemy_Imp_Walk": imp_walk, "Enemy_Imp_Attack": imp_attack,
    "Enemy_Chicken_Idle": chicken_idle, "Enemy_Chicken_Walk": chicken_walk, "Enemy_Chicken_Attack": chicken_attack,
}


def build_anims():
    """{텍스처 이름: 그림}. Anims.csv에 있는 동작만 그린다."""
    sprites = {}
    for name, (frames, hit) in read_anims().items():
        draw = ANIM_DRAW.get(name)
        if draw is None:
            raise KeyError("Anims.csv %s: anim_hd.py에 그리는 함수가 없다" % name)
        for i in range(frames):
            sprites["T_%s_%d" % (name, i)] = draw(i, frames, hit) if name.endswith("_Attack") else draw(i, frames)
    return sprites
