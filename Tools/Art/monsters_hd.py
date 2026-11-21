"""명암·재질 톤으로 다시 그린 몬스터 (오른쪽을 본다, 2프레임). 캔버스 크기·발 위치는 예전과 같다.

들판: 돌진 닭, 근육 감자, 날개 식빵, 응원 버섯, 대왕 푸딩
해변: 꽃게 병장, 둥실 해파리, 코코넛 형님, 코코 알맹이, 왕꽃게
설산: 눈사람, 눈뭉치, 썰매 펭귄, 예티
(꼬마 도깨비는 units_hd.py)
"""

import math

from paint import OUTLINE, Painter, light_of, rgba, tone
from units_hd import GOLD, STEEL, TEAM_RED, WOOD, face

WHITE = rgba("fbf8f2")
CREAM = rgba("fff1cf")
SNOW = rgba("f4f8ff")
SNOW_SHADE = rgba("a9c4ee")
ICE = rgba("bfe6ff")
CRAB = rgba("ff6f4f")
POTATO = rgba("e4b26a")
NAVY = rgba("323a5c")
BEAK = rgba("ffb02e")


def m_cloud(p, circles):
    return p.union(*[p.m_ellipse(x, y, r, r) for x, y, r in circles])


def legs(p, xs, top, foot_y, frame, color, shoe=None, stride=5.0, w=4.5):
    """짤막한 다리 + 둥근 발. frame마다 앞뒤가 바뀐다."""
    s = stride if frame % 2 == 0 else -stride
    for x, d in zip(xs, (s, -s)):
        p.paint(p.m_rrect(x - w + d * 0.3, top, x + w + d * 0.3, foot_y - 2, w), color, form=0.3, glint=0.8, lw=2.4)
        p.paint(p.m_blob(x + 2 + d, foot_y, w + 3.5, w * 0.7 + 1.5, power=2.4), shoe or tone(color, light=-0.35),
                form=0.3, glint=1.0, lw=2.4)


def goggle(p, x, y, r, look=2.0):
    """튀어나온 왕눈 (흰자 + 눈동자 + 빛)."""
    p.paint(p.m_ellipse(x, y, r, r * 1.05), rgba("fffaf0"), form=0.25, rim=0.8, glint=0, lw=2.2)
    p.dot(x + look, y + 0.5, r * 0.45, OUTLINE)
    p.dot(x + look - r * 0.18, y - r * 0.15, r * 0.16, (255, 255, 255, 240))


def brow(p, x0, y0, x1, y1, w=2.6):
    p.line([(x0, y0), (x1, y1)], OUTLINE, w)


# ---- 들판 ---------------------------------------------------------------------------------

def make_chicken(frame):
    p = Painter(116, 116, lw=2.6)
    bob = 0 if frame == 0 else -2
    # 달리는 다리 (주황 막대 + 발가락)
    pose = [((46, 76), (30, 102)), ((62, 76), (82, 98))] if frame == 0 else [((46, 76), (60, 102)), ((62, 76), (42, 96))]
    for hip, foot in pose:
        p.paint(p.m_line([hip, foot], 4), BEAK, form=0, rim=0.6, glint=0.6, lw=2.0)
        for dx in (-6, 0, 6):
            p.paint(p.m_line([foot, (foot[0] + dx + 4, foot[1] + 3)], 2.6), BEAK, form=0, rim=0, glint=0, lw=1.6)
    # 꼬리 깃
    tail = p.m_poly([(24, 46 + bob), (4, 30 + bob), (12, 48 + bob), (0, 56 + bob), (14, 60 + bob), (4, 70 + bob), (26, 64 + bob)])
    p.paint(tail, WHITE, form=0.35, glint=1.4)
    # 볏 + 몸
    p.paint(m_cloud(p, [(60, 20 + bob, 8), (71, 14 + bob, 9.5), (83, 19 + bob, 8)]), TEAM_RED, form=0.35, glint=1.4)
    body = p.m_blob(56, 54 + bob, 38, 29, angle=-12)
    p.paint(body, WHITE, form=0.35, rim=1.8, glint=2.0)
    # 부리 + 턱볏
    p.paint(p.m_poly([(92, 44 + bob), (113, 51 + bob), (92, 59 + bob)]), BEAK, form=0.35, glint=1.0, lw=2.4)
    p.line([(92, 51.5 + bob), (106, 51.5 + bob)], OUTLINE, 1.4)
    p.paint(p.m_ellipse(90, 64 + bob, 4.5, 6), TEAM_RED, form=0.3, glint=0.8, lw=2.0)
    # 비장한 눈 + 날개
    p.dot(79, 44 + bob, 3.6, OUTLINE)
    p.dot(78, 42.8 + bob, 1.2, (255, 255, 255, 240))
    brow(p, 69, 35 + bob, 86, 39 + bob, 3.2)
    wing = p.m_poly([(44, 48 + bob), (24, 58 + bob), (30, 62 + bob), (26, 68 + bob), (48, 68 + bob)])
    p.paint(wing, tone(WHITE, light=-0.06), form=0.35, glint=1.0, lw=2.4)
    return p.result()


def make_potato(frame):
    p = Painter(148, 152, lw=3.0)
    bob = 0 if frame == 0 else -3
    legs(p, (60, 88), 122 + bob, 144, frame, POTATO, shoe=rgba("5b4a7a"), stride=5, w=6)
    flex = 0 if frame == 0 else 5
    spots = lambda q, ox, oy: [q.dot_e(ox + x, oy + y, r, r * 0.7, tone(POTATO, light=-0.25)) for x, y, r in ((-6, 4, 2.2), (5, -6, 1.6))]
    for side in (-1, 1):
        cx = 74 + side * 52
        p.paint(p.m_ellipse(cx, 66 + bob, 18, 17), POTATO, form=0.35, glint=1.6)                       # 알통
        p.paint(p.m_ellipse(cx + side * 4, 40 + bob + flex, 11, 16), POTATO, form=0.3, glint=1.2)       # 팔뚝
        p.paint(p.m_ellipse(cx + side * 4, 22 + bob + flex, 11, 10), POTATO, form=0.3, glint=1.2)       # 주먹
    body = p.m_blob(74, 82 + bob, 44, 52, power=2.4, wobble=0.04, seed=3)
    p.paint(body, POTATO, form=0.34, rim=2.0, glint=2.2,
            texture=lambda q: [spots(q, x, y + bob) for x, y in ((52, 108), (96, 58), (92, 116), (48, 54), (100, 92))])
    # 민소매 (빨강) + 이마 띠
    shirt = p.both(body, p.m_rrect(20, 96 + bob, 130, 140 + bob, 0))
    p.paint(shirt, TEAM_RED, form=0.3, rim=1.4, glint=1.0, lw=2.4)
    p.paint(p.m_rrect(32, 50 + bob, 116, 60 + bob, 4), rgba("ffffff"), form=0.25, glint=0.8, lw=2.4)
    p.paint(p.m_poly([(32, 52 + bob), (16, 46 + bob), (20, 58 + bob)]), rgba("ffffff"), form=0.2, glint=0, lw=2.0)
    # 화난 얼굴
    for sx in (-1, 1):
        p.dot(76 + sx * 12, 76 + bob, 3.6, OUTLINE)
        p.dot(76 + sx * 12 - 1, 74.8 + bob, 1.2, (255, 255, 255, 240))
    brow(p, 58, 64 + bob, 72, 69 + bob, 3.6)
    brow(p, 94, 64 + bob, 80, 69 + bob, 3.6)
    p.paint(p.m_rrect(66, 88 + bob, 88, 94 + bob, 3), rgba("fffaf0"), form=0, rim=0, glint=0, lw=2.2)
    return p.result()


def make_toast(frame):
    p = Painter(136, 116, lw=2.8)
    flap = -34 if frame == 0 else 16
    for side, x in ((-1, 24), (1, 112)):
        wing = p.m_blob(x, 54 + flap * 0.3, 22, 11, angle=flap * -side)
        p.paint(wing, WHITE, form=0.35, glint=1.4)
        p.line([(x - 10 * side, 54 + flap * 0.3), (x + 8 * side, 50 + flap * 0.3 - side * flap * 0.15)], tone(WHITE, light=-0.25), 1.2)
    crust = p.union(p.m_ellipse(52, 34, 22, 20), p.m_ellipse(84, 34, 22, 20), p.m_rrect(32, 30, 104, 100, 12))
    p.paint(crust, rgba("c47e3c"), form=0.35, rim=2.0, glint=2.0)
    inner = p.union(p.m_ellipse(52, 35, 16, 14), p.m_ellipse(84, 35, 16, 14), p.m_rrect(38, 34, 98, 93, 8))
    def crumbs(q):
        for x, y in ((48, 50), (60, 82), (86, 70), (76, 88), (92, 52), (44, 74)):
            q.dot_e(x, y, 1.6, 1.0, tone(CREAM, light=-0.2))
    p.paint(inner, rgba("fbe6b4"), form=0.25, rim=1.0, glint=1.2, lw=0, texture=crumbs)
    # 녹아내리는 버터
    butter = p.union(p.m_rrect(72, 36, 96, 52, 4), p.m_rrect(80, 48, 86, 60, 3))
    p.paint(butter, rgba("ffd84a"), form=0.3, rim=1.0, glint=1.6, lw=2.2)
    p.shine(77, 40, 3, 1.8, 220)
    face(p, 63, 66, look=1, eye_gap=12, eye_r=3.4, mouth="grin")
    return p.result()


def make_mushroom(frame):
    p = Painter(128, 140, lw=2.8)
    bob = 0 if frame == 0 else -3
    legs(p, (54, 74), 114 + bob, 132, frame, CREAM, shoe=rgba("8a5530"), stride=5)
    up = (-1, 1) if frame == 0 else (1, -1)
    for side, lift in zip((-1, 1), up):
        hx, hy = 64 + side * 48, 84 + bob + lift * 20
        p.paint(p.m_line([(64 + side * 24, 92 + bob), (hx, hy)], 5), CREAM, form=0, rim=0.6, glint=0.6, lw=2.2)
        pom = m_cloud(p, [(hx - 7, hy, 8), (hx + 7, hy, 8), (hx, hy - 8, 8), (hx, hy + 7, 7.5), (hx - 6, hy - 6, 6), (hx + 6, hy + 5, 6)])
        p.paint(pom, rgba("ff8ec0"), form=0.35, glint=1.2, lw=2.2)
    p.paint(p.m_ellipse(64, 96 + bob, 26, 28), CREAM, form=0.32, rim=1.6, glint=1.8)
    cap = p.union(p.m_ellipse(64, 62 + bob, 52, 36), p.m_rrect(14, 60 + bob, 114, 66 + bob, 3))
    cap = p.cut(cap, p.m_rrect(0, 66 + bob, 128, 140, 0))
    def spots(q):
        for x, y, r in ((40, 44, 8), (68, 34, 10), (94, 48, 7), (52, 58, 5), (82, 58, 5)):
            q.dot_e(x, y + bob, r + 1.2, r * 0.8 + 1.2, tone(WHITE, light=-0.3))
            q.dot_e(x, y + bob, r, r * 0.8, WHITE)
    p.paint(cap, TEAM_RED, form=0.34, rim=2.0, glint=2.2, texture=spots)
    p.paint(p.m_rrect(22, 62 + bob, 106, 68 + bob, 3), tone(CREAM, light=-0.15), form=0.2, glint=0.6, lw=2.2)
    face(p, 64, 92 + bob, look=2, eye_gap=10, eye_r=3.2, mouth="grin")
    return p.result()


def make_pudding_king(frame):
    p = Painter(264, 264, lw=4.2)
    sq = 0 if frame == 0 else 8
    legs(p, (108, 156), 216, 250, frame, rgba("ffd36b"), shoe=rgba("a3622f"), stride=6, w=8)
    for side in (-1, 1):
        p.paint(p.m_line([(132 + side * (70 + sq), 160), (132 + side * 102, 190 - sq)], 8), rgba("ffd36b"), form=0, rim=1.0, glint=1.0, lw=3.0)
    top = 92 + sq
    body = p.m_poly([(56 - sq, 224), (82, top), (182, top), (208 + sq, 224), (132, 236)])
    p.paint(body, rgba("ffe08a"), form=0.36, rim=2.6, glint=3.0)
    # 접시
    p.paint(p.m_blob(132, 230, 92 + sq, 12), rgba("eef4ff"), form=0.3, glint=1.6, lw=3.4)
    # 흘러내리는 카라멜 (반짝)
    caramel = p.m_poly([(80, top + 6), (84, top - 12), (180, top - 12), (184, top + 6), (176, top + 30), (164, top + 10), (150, top + 40),
                        (136, top + 12), (118, top + 32), (104, top + 10), (92, top + 26)])
    p.paint(caramel, rgba("a3582a"), form=0.3, rim=1.6, glint=2.4)
    p.shine(104, top - 4, 10, 3, 200)
    p.shine(96, 150, 5, 22, 120)
    # 왕관
    cy = top - 14
    crown = p.m_poly([(98, cy), (94, cy - 40), (112, cy - 22), (132, cy - 52), (152, cy - 22), (170, cy - 40), (166, cy)])
    p.paint(crown, GOLD, form=0.35, rim=1.6, glint=2.0)
    for x in (112, 132, 152):
        p.ball(x, cy - 10, 5, 5, TEAM_RED, form=0.3, glint=1.0, lw=2.0)
    face(p, 132, 158, look=0, eye_gap=24, eye_r=5.5, mouth="grin")
    p.arc(132, 166, 16, 12, 20, 160, OUTLINE, 4.0)
    return p.result()


# ---- 해변 ---------------------------------------------------------------------------------

def draw_crab(p, cx, cy, s, frame, king=False):
    bob = 0 if frame == 0 else -3 * s
    # 다리 셋씩
    for side in (-1, 1):
        for index in range(3):
            x0 = cx + side * (18 + index * 13) * s
            swing = (5 if (frame + index) % 2 == 0 else -5) * s
            p.paint(p.m_line([(x0, cy + 10 * s + bob), (x0 + side * 14 * s + swing, cy + 34 * s)], 4.5 * s), tone(CRAB, light=-0.15),
                    form=0, rim=0.8 * s, glint=0.8 * s, lw=2.2 * s)
    # 집게
    lift = (0 if frame == 0 else -10) * s
    for side in (-1, 1):
        ax, ay = cx + side * 40 * s, cy - 4 * s + bob
        hx, hy = cx + side * 58 * s, cy - 34 * s + bob + lift
        p.paint(p.m_line([(ax, ay), (hx, hy + 10 * s)], 6 * s), CRAB, form=0, rim=0.8 * s, glint=0.8 * s, lw=2.4 * s)
        p.paint(p.m_blob(hx, hy, 15 * s, 12 * s, angle=side * 20), CRAB, form=0.35, glint=1.6 * s, lw=2.6 * s)
        p.paint(p.m_poly([(hx - 4 * s * side, hy - 2 * s), (hx + 2 * s * side, hy - 17 * s), (hx + 13 * s * side, hy - 8 * s)]), CRAB,
                form=0.3, glint=1.0 * s, lw=2.4 * s)
    # 눈자루
    for side in (-1, 1):
        p.paint(p.m_line([(cx + side * 12 * s, cy - 14 * s + bob), (cx + side * 14 * s, cy - 36 * s + bob)], 4 * s), CRAB,
                form=0, rim=0.6 * s, glint=0.6 * s, lw=2.2 * s)
    # 등딱지 (점박이)
    shell = p.m_ellipse(cx, cy + bob, 46 * s, 28 * s)
    def dots(q):
        for x, y, r in ((-24, -10, 3.2), (-30, 6, 2.4), (24, -8, 2.8), (30, 8, 2.2), (-8, -18, 2.0)):
            q.dot_e(cx + x * s, cy + y * s + bob, r * s, r * 0.8 * s, tone(CRAB, light=-0.25))
    p.paint(shell, CRAB, form=0.34, rim=2.0 * s, glint=2.4 * s, lw=3.0 * s, texture=dots)
    for side in (-1, 1):
        goggle(p, cx + side * 14 * s, cy - 40 * s + bob, 8 * s, look=2 * s)
    if king:
        crown = p.m_poly([(cx - 26 * s, cy - 22 * s + bob), (cx - 30 * s, cy - 54 * s + bob), (cx - 14 * s, cy - 38 * s + bob),
                          (cx, cy - 62 * s + bob), (cx + 14 * s, cy - 38 * s + bob), (cx + 30 * s, cy - 54 * s + bob),
                          (cx + 26 * s, cy - 22 * s + bob)])
        p.paint(crown, GOLD, form=0.35, glint=1.6 * s, lw=2.6 * s)
        for x in (-14, 0, 14):
            p.ball(cx + x * s, cy - 30 * s + bob, 3.6 * s, 3.6 * s, rgba("3fb6e8"), form=0.3, glint=0.8 * s, lw=1.8 * s)
        for side in (-1, 1):
            stache = p.m_blob(cx + side * 13 * s, cy + 6 * s + bob, 13 * s, 5 * s, angle=side * -12)
            p.paint(stache, rgba("3a2420"), form=0.25, glint=0.8 * s, lw=1.6 * s)
        brow(p, cx - 22 * s, cy - 12 * s + bob, cx - 6 * s, cy - 6 * s + bob, 3 * s)
        brow(p, cx + 22 * s, cy - 12 * s + bob, cx + 6 * s, cy - 6 * s + bob, 3 * s)
        p.line([(cx - 8 * s, cy + 16 * s + bob), (cx + 8 * s, cy + 16 * s + bob)], OUTLINE, 3 * s)
    else:
        # 병장 군모 + 화난 눈썹
        cap = p.union(p.m_rrect(cx - 18 * s, cy - 30 * s + bob, cx + 18 * s, cy - 18 * s + bob, 5 * s),
                      p.m_blob(cx + 4 * s, cy - 18 * s + bob, 24 * s, 4 * s))
        p.paint(cap, rgba("5f7a3a"), form=0.3, glint=1.2 * s, lw=2.4 * s)
        p.paint(p.m_poly([(cx - 4 * s, cy - 28 * s + bob), (cx + 4 * s, cy - 28 * s + bob), (cx, cy - 21 * s + bob)]), GOLD,
                form=0, rim=0, glint=0, lw=1.4 * s)
        brow(p, cx - 16 * s, cy - 8 * s + bob, cx - 4 * s, cy - 4 * s + bob, 3 * s)
        brow(p, cx + 16 * s, cy - 8 * s + bob, cx + 4 * s, cy - 4 * s + bob, 3 * s)
        p.line([(cx - 9 * s, cy + 7 * s + bob), (cx + 9 * s, cy + 8 * s + bob)], OUTLINE, 3 * s)


def make_crab(frame):
    p = Painter(170, 140, lw=2.6)
    draw_crab(p, 85, 82, 1.0, frame)
    return p.result()


def make_king_crab(frame):
    p = Painter(300, 264, lw=4.0)
    draw_crab(p, 150, 160, 1.85, frame, king=True)
    return p.result()


def make_jellyfish(frame):
    p = Painter(120, 150, lw=2.6)
    phase = 0.0 if frame == 0 else math.pi
    for index in range(4):
        x = 30 + index * 20
        pts = [(x + 6 * math.sin(phase + t * 0.25 + index), 64 + t * 7) for t in range(10)]
        p.paint(p.m_line(pts, 4), rgba("ff9ec4"), form=0, rim=0.8, glint=0.8, lw=2.2)
    bell = p.cut(p.m_ellipse(60, 66, 46, 50), p.m_rrect(0, 66, 120, 150, 0))
    p.paint(bell, rgba("ffb6d6"), form=0.36, rim=2.0, glint=2.4)
    # 아래 주름
    frill = p.union(*[p.m_ellipse(23 + i * 18.5, 66, 10, 6.5) for i in range(5)])
    p.paint(frill, rgba("ff8ec0"), form=0.25, glint=0.8, lw=2.4)
    p.shine(42, 38, 9, 5, 170)
    p.shine(84, 30, 3, 2, 200)
    face(p, 62, 48, look=0, eye_gap=12, eye_r=3.4, mouth="grin")
    return p.result()


def make_coconut(frame):
    p = Painter(120, 134, lw=2.8)
    bob = 0 if frame == 0 else -3
    legs(p, (50, 70), 96 + bob, 126, frame, rgba("8f5c36"), shoe=rgba("f2e2c0"), stride=6)
    # 야자잎 머리
    for index, ang in enumerate((-60, -25, 10, 45)):
        t = math.radians(ang - 90)
        ex, ey = 60 + math.cos(t) * 34, 30 + bob + math.sin(t) * 22
        p.paint(p.m_poly([(60, 34 + bob), (ex - 6, ey + 2), (ex, ey), (ex + 6, ey + 4)]), rgba("4fa83a"), form=0.3, glint=1.0, lw=2.2)
    def fibers(q):
        for k in range(26):
            a = k * 0.53
            x, y = 60 + math.cos(a) * (14 + (k * 7) % 22), 66 + bob + math.sin(a) * (12 + (k * 5) % 20)
            q.line([(x, y), (x + 4 * math.cos(a + 1.3), y + 4 * math.sin(a + 1.3))], tone(rgba("8f5c36"), light=-0.3), 1.2)
    p.paint(p.m_ellipse(60, 66 + bob, 40, 38), rgba("8f5c36"), form=0.36, rim=2.0, glint=2.4, texture=fibers)
    # 선글라스
    for x in (36, 64):
        p.paint(p.m_rrect(x, 54 + bob, x + 22, 70 + bob, 6), rgba("1e1a2a"), form=0, rim=0, glint=0, lw=2.4)
        p.line([(x + 5, 58 + bob), (x + 12, 57 + bob)], (180, 220, 255, 220), 2.0)
    p.line([(56, 59 + bob), (66, 59 + bob)], OUTLINE, 3)
    p.arc(64, 82 + bob, 10, 6, 10, 150, OUTLINE, 3)
    return p.result()


def make_coco_kid(frame):
    p = Painter(84, 96, lw=2.4)
    bob = 0 if frame == 0 else -3
    legs(p, (34, 50), 70 + bob, 90, frame, rgba("fffaf0"), shoe=rgba("8f5c36"), stride=6, w=3.8)
    p.paint(p.m_ellipse(42, 54 + bob, 26, 24), rgba("fffaf0"), form=0.32, rim=1.6, glint=1.8)
    shell = p.cut(p.m_ellipse(42, 44 + bob, 28, 22), p.m_rrect(0, 44 + bob, 84, 96, 0))
    p.paint(shell, rgba("8f5c36"), form=0.35, glint=1.6)
    p.paint(p.m_rrect(14, 41 + bob, 70, 46 + bob, 2), tone(rgba("fffaf0"), light=-0.1), form=0, glint=0, lw=1.8)
    face(p, 44, 57 + bob, look=1, eye_gap=8, eye_r=2.8, mouth="o")
    return p.result()


# ---- 설산 ---------------------------------------------------------------------------------

def snow_paint(p, mask, **kw):
    p.paint(mask, SNOW, shade=SNOW_SHADE, light=rgba("ffffff"), **kw)


def make_snowman(frame):
    p = Painter(130, 168, lw=2.8)
    bob = 0 if frame == 0 else -6
    sway = 10 if frame == 0 else -10
    for x0, x1, y1 in ((36, 8, 86 + bob + sway), (94, 122, 86 + bob - sway)):
        p.paint(p.m_line([(x0, 104 + bob), (x1, y1)], 3.5), WOOD, form=0, rim=0.6, glint=0.6, lw=2.2)
        p.paint(p.m_line([(x1 + (x0 - x1) * 0.25, y1 + (104 + bob - y1) * 0.25), (x1 + (x0 - x1) * 0.25, y1 - 9)], 2.4), WOOD,
                form=0, rim=0, glint=0, lw=1.8)
    snow_paint(p, p.m_ellipse(65, 128 + bob * 0.5, 38, 34), form=0.36, rim=2.0, glint=2.2)
    for y in (116, 134):
        p.ball(65, y + bob * 0.5, 3.6, 3.6, rgba("3a3340"), form=0.3, glint=0.8, lw=1.4)
    snow_paint(p, p.m_ellipse(65, 72 + bob, 28, 26), form=0.36, rim=1.8, glint=2.0)
    # 줄무늬 목도리
    def stripes(q):
        for x in range(40, 96, 8):
            q.poly([(x, 88 + bob), (x + 4, 88 + bob), (x + 4, 120 + bob), (x, 120 + bob)], rgba("ffffff"))
    scarf = p.union(p.m_rrect(40, 90 + bob, 90, 102 + bob, 5), p.m_rrect(70, 94 + bob, 84, 118 + bob, 4))
    p.paint(scarf, TEAM_RED, form=0.3, glint=1.2, lw=2.4, texture=stripes)
    face(p, 62, 66 + bob, look=2, eye_gap=9, eye_r=3.4, mouth="none", blush=True)
    p.paint(p.m_poly([(66, 73 + bob), (94, 80 + bob), (66, 81 + bob)]), rgba("ff8a3c"), form=0.3, glint=0.8, lw=2.0)
    # 찌그러진 양동이 모자
    bucket = p.m_poly([(42, 50 + bob), (48, 20 + bob), (80, 22 + bob), (88, 52 + bob)])
    p.paint(bucket, rgba("6fa8dc"), form=0.36, glint=2.0, lw=2.6)
    p.paint(p.m_rrect(39, 47 + bob, 91, 54 + bob, 3), rgba("4f86bd"), form=0.25, glint=0.8, lw=2.2)
    p.rivets([(48, 50.5 + bob), (82, 50.5 + bob)], r=1.2)
    p.shine(53, 30 + bob, 2, 7, 160)
    return p.result()


def make_snowball(frame):
    p = Painter(84, 92, lw=2.4)
    bob = 0 if frame == 0 else -4
    legs(p, (34, 50), 66 + bob, 86, frame, rgba("dfe8f8"), shoe=rgba("7fa6d8"), stride=6, w=3.8)
    snow_paint(p, p.m_blob(42, 50 + bob, 28, 26, wobble=0.05, seed=4), form=0.36, rim=1.6, glint=2.0)
    face(p, 44, 50 + bob, look=1, eye_gap=9, eye_r=3.0, mouth="o")
    return p.result()


def make_penguin(frame):
    """배로 미끄러지는 펭귄."""
    p = Painter(150, 110, lw=2.6)
    flap = -14 if frame == 0 else 6
    for index, y in enumerate((48, 62, 76)):
        x0 = 4 + (index * 6 if frame == 0 else 12 - index * 4)
        p.line([(x0, y), (x0 + 22, y)], ICE, 3.2)
    for fx, fy in ((40, 82), (46, 74)):
        p.paint(p.m_ellipse(fx, fy, 9, 5), BEAK, form=0.3, glint=0.8, lw=2.0)
    p.paint(p.m_blob(80, 72, 50, 24), NAVY, form=0.32, rim=2.0, glint=2.2)
    p.paint(p.m_blob(84, 82, 40, 12), WHITE, form=0.3, rim=1.0, glint=0.8, lw=0)
    p.paint(p.m_blob(80, 58, 16, 7, angle=flap), NAVY, form=0.3, glint=1.2, lw=2.4)
    p.paint(p.m_ellipse(122, 60, 22, 21), NAVY, form=0.32, rim=1.6, glint=2.0)
    p.paint(p.m_ellipse(126, 64, 13, 13), WHITE, form=0.25, glint=0, lw=0)
    p.paint(p.m_rrect(100, 68, 116, 84, 5), TEAM_RED, form=0.3, glint=1.0, lw=2.2)
    p.paint(p.m_poly([(102, 80), (92, 92), (104, 88)]), TEAM_RED, form=0.3, glint=0.6, lw=2.0)
    p.paint(p.m_poly([(140, 62), (151, 67), (140, 72)]), BEAK, form=0.3, glint=0.8, lw=2.0)
    for x in (124, 134):
        p.dot(x, 58, 3, OUTLINE)
        p.dot(x - 0.8, 57, 1.0, (255, 255, 255, 240))
    brow(p, 118, 50, 128, 53, 2.6)
    # 고글 (머리 위)
    p.paint(p.m_rrect(108, 40, 136, 48, 4), rgba("ffb02e"), form=0.3, glint=1.0, lw=2.0)
    p.paint(p.m_ellipse(124, 44, 5, 4), rgba("9fe3ff"), form=0.3, glint=0.8, lw=1.6)
    return p.result()


def make_yeti(frame):
    p = Painter(264, 270, lw=4.0)
    up = 0 if frame == 0 else -20
    legs(p, (106, 158), 210, 256, frame, SNOW, shoe=rgba("8fa9d6"), stride=6, w=11)
    fur = dict(form=0.36, rim=2.6, glint=2.6)
    for side in (-1, 1):
        hx, hy = 132 + side * 104, 120 + up * (1 if side < 0 else -0.4)
        snow_paint(p, p.m_line([(132 + side * 62, 150), (hx, hy)], 24), form=0.3, rim=2.0, glint=1.6)
        snow_paint(p, m_cloud(p, [(hx, hy, 18), (hx + side * 8, hy - 12, 12), (hx - side * 6, hy + 10, 10)]), **fur)
    body = m_cloud(p, [(132, 140, 70), (90, 170, 44), (174, 170, 44), (100, 110, 40), (164, 110, 40), (132, 96, 44), (132, 196, 50),
                       (78, 140, 30), (186, 140, 30)])
    def tufts(q):
        for x, y in ((96, 92), (120, 70), (150, 74), (172, 96), (84, 190), (180, 192), (132, 220)):
            q.line([(x, y), (x + 6, y + 10), (x + 12, y)], SNOW_SHADE, 2.4)
    snow_paint(p, body, texture=tufts, **fur)
    p.paint(p.m_ellipse(132, 136, 44, 40), rgba("8cc8f5"), form=0.32, rim=1.8, glint=2.0, lw=3.4)
    for sx in (-1, 1):
        p.dot(132 + sx * 16, 128, 5.5, OUTLINE)
        p.dot(132 + sx * 16 - 1.6, 126.4, 1.8, (255, 255, 255, 240))
    brow(p, 110, 114, 124, 120, 4)
    brow(p, 154, 114, 140, 120, 4)
    mouth = p.cut(p.m_ellipse(132, 150, 22, 16), p.m_rrect(100, 120, 164, 150, 0))
    p.paint(mouth, rgba("5a1f2e"), form=0, rim=0, glint=0, lw=3.0)
    for x in (118, 140):
        p.paint(p.m_poly([(x, 150), (x + 6, 161), (x + 12, 150)]), rgba("fffaf0"), form=0, rim=0, glint=0, lw=1.8)
    return p.result()
