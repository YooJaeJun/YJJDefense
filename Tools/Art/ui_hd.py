"""룬 아이콘·UI(패널·디스크·꽂는 칸·작은 아이콘)를 명암·외곽선 톤으로 (generate_art.py가 쓴다).

캔버스 크기는 예전과 같다. 패널은 게임이 9칸으로 늘리고 색을 곱해 입히므로 흰 바탕 + 무채색 그늘,
모서리(UILayout PanelSourceCorner = 20px) 안에 테두리 모양이 다 들어가야 한다.
일시정지·재생·빨리 감기 아이콘은 색 버튼 위에 얹히므로 흰색.
"""

import math

from paint import OUTLINE, Painter, rgba, tone
from units_hd import GOLD, STEEL, TEAM_RED, WOOD

WHITE = (255, 255, 255, 255)
NEUTRAL_SHADE = (208, 209, 216, 255)   # 색을 곱해 입힐 흰 그림의 그늘
TILE = rgba("fff6e2")                  # 룬 판: 따뜻한 양피지
TILE_LIP = rgba("e2cfa8")
FIRE = rgba("ff8a2a")
FIRE_CORE = rgba("ffe04a")
ICE = rgba("8fdcff")
WATER = rgba("3f8dff")
SPARK = rgba("ffd93a")
POISON = rgba("7fd34a")
WIND = rgba("4fd6ae")
BLAST = rgba("ff5a3c")
CHAIN = rgba("a07bff")
SPLIT = rgba("ff9a3c")
GREEN = rgba("6fcf4a")


def star_pts(cx, cy, r_out, r_in, n, rot=-90.0):
    return [(cx + math.cos(math.radians(rot) + math.pi * k / n) * (r_out if k % 2 == 0 else r_in),
             cy + math.sin(math.radians(rot) + math.pi * k / n) * (r_out if k % 2 == 0 else r_in)) for k in range(n * 2)]


def white_paint(p, mask, lw=None, form=0.32):
    p.paint(mask, WHITE, shade=NEUTRAL_SHADE, light=WHITE, lw=lw, form=form, rim=1.2, glint=0)


# ---- 룬 -----------------------------------------------------------------------------------

def rune_tile(p):
    """양피지 판: 외곽선 + 아래 두께 + 윗면 + 안쪽 테."""
    p.paint(p.m_rrect(5, 5, 91, 91, 22), TILE_LIP, form=0, rim=1.4, glint=0, lw=3.4)
    p.paint(p.m_rrect(8, 8, 88, 80, 19), TILE, form=0, rim=0, glint=2.0, outline=None)
    p.line([(26, 80.5), (70, 80.5)], tone(TILE_LIP, light=-0.25), 1.2, cap=False)
    p.arc(48, 46, 34, 30, 0, 360, (236, 218, 184, 255), 1.4)


def make_rune(kind):
    p = Painter(96, 96, lw=2.8)
    rune_tile(p)
    if kind == "Fire":
        flame = p.m_poly([(48, 12), (56, 26), (64, 22), (66, 40), (72, 54), (64, 72), (48, 80), (32, 72), (24, 54), (30, 38), (34, 26), (41, 32)])
        p.paint(flame, FIRE, shade=rgba("e0401f"), form=0.32, rim=1.4, glint=1.4)
        p.paint(p.m_poly([(48, 42), (56, 56), (58, 68), (48, 74), (38, 68), (40, 56)]), FIRE_CORE, shade=rgba("ffb02a"), outline=None, form=0.3, rim=0, glint=1.0)
    elif kind == "Ice":
        arms = []
        for a in (90, 30, 150):
            dx, dy = 28 * math.cos(math.radians(a)), 28 * math.sin(math.radians(a))
            arms.append(p.m_line([(48 - dx, 46 - dy), (48 + dx, 46 + dy)], 7))
            for sgn in (-1, 1):
                bx, by = 48 + sgn * dx * 0.62, 46 + sgn * dy * 0.62
                for side in (-1, 1):
                    b = math.radians(a + side * 50)
                    arms.append(p.m_line([(bx, by), (bx + sgn * 9 * math.cos(b), by + sgn * 9 * math.sin(b))], 4.4))
        p.paint(p.union(*arms), ICE, shade=rgba("4fa6e0"), form=0.3, rim=1.0, glint=1.2)
        p.paint(p.m_poly(star_pts(48, 46, 8, 4, 6, rot=0)), WHITE, shade=rgba("cfeeff"), lw=1.6, form=0.2, rim=0, glint=0)
    elif kind == "Water":
        drop = p.m_poly([(48, 12)] + [(48 + 21 * math.cos(math.radians(a)), 56 + 21 * math.sin(math.radians(a))) for a in range(-30, 211, 10)])
        p.paint(drop, WATER, form=0.34, rim=1.4, glint=1.6)
        p.shine(40, 50, 4.5, 7, alpha=200)
        p.shine(40, 63, 1.8, 1.8, alpha=210)
    elif kind == "Lightning":
        bolt = p.m_poly([(58, 10), (26, 52), (44, 52), (34, 86), (72, 40), (53, 40), (64, 10)])
        p.paint(bolt, SPARK, shade=rgba("f29a1e"), form=0.3, rim=1.2, glint=1.6)
    elif kind == "Poison":
        # 독 병: 둥근 병 + 목 + 코르크, 안에 끓는 초록 물
        p.paint(p.m_rrect(40, 14, 56, 24, 3), WOOD, form=0.3, rim=0.8, glint=0.8, lw=2.2)
        bottle = p.union(p.m_ellipse(48, 58, 24, 22), p.m_rrect(41, 22, 55, 42, 3))
        p.paint(bottle, rgba("e8f6ff"), shade=rgba("b8d6e6"), form=0.25, rim=1.0, glint=0)
        liquid = p.both(p.m_ellipse(48, 58, 21, 19), p.m_rrect(20, 50, 76, 80, 0))
        p.paint(liquid, POISON, shade=rgba("3f8f2f"), outline=None, form=0.3, rim=0.8, glint=0)
        p.line([(28, 50), (68, 50)], tone(POISON, light=0.3), 1.6, cap=False)
        for bx, by, r in ((42, 64, 3.4), (54, 58, 2.4), (50, 70, 2)):
            p.dot(bx, by, r, tone(POISON, light=0.4))
        p.shine(36, 48, 3, 6, alpha=210)
        for bx, by, r in ((62, 22, 3.4), (68, 12, 2.4)):
            p.ball(bx, by, r, r, POISON, lw=1.4, form=0.3, rim=0.4, glint=0.6)
    elif kind == "Wind":
        swirl = [(48 + (6 + t * 2.2) * math.cos(t), 40 + (6 + t * 2.2) * 0.75 * math.sin(t)) for t in [i * 0.25 for i in range(26)]]
        p.paint(p.m_line(swirl, 6.5), WIND, shade=rgba("2fa088"), form=0.25, rim=0.8, glint=1.0)
        for y, x0, x1 in ((64, 20, 70), (76, 32, 62)):
            p.paint(p.m_line([(x0, y), (x1, y)], 6.5), WIND, shade=rgba("2fa088"), form=0.25, rim=0.8, glint=1.0)
    elif kind == "Explosion":
        p.paint(p.m_poly(star_pts(48, 47, 36, 18, 9)), BLAST, shade=rgba("c9301f"), form=0.3, rim=1.4, glint=1.6)
        p.paint(p.m_poly(star_pts(48, 47, 20, 11, 9, rot=-70)), FIRE_CORE, shade=rgba("ffb02a"), outline=None, form=0.3, rim=0, glint=0)
        p.dot(48, 47, 6, WHITE)
    elif kind == "Chain":
        for cx, cy, a in ((36, 38, -30), (60, 58, -30)):
            ring = p.cut(p.m_blob(cx, cy, 19, 13, angle=a), p.m_blob(cx, cy, 11, 5.5, angle=a))
            p.paint(ring, CHAIN, shade=rgba("6a48d0"), form=0.32, rim=1.2, glint=1.6)
        # 앞 고리가 뒤 고리를 다시 덮는 한 조각 (서로 엮인 느낌)
        p.paint(p.both(p.cut(p.m_blob(36, 38, 19, 13, angle=-30), p.m_blob(36, 38, 11, 5.5, angle=-30)), p.m_rrect(42, 40, 60, 60, 0)),
                CHAIN, shade=rgba("6a48d0"), form=0.32, rim=1.2, glint=1.6)
    elif kind == "Split":
        for dx, dy in ((-24, -34), (0, -46), (24, -34)):
            tip = (48 + dx, 78 + dy)
            p.paint(p.m_line([(48, 78), tip], 6.5), SPLIT, shade=rgba("d96a1f"), form=0.2, rim=0.8, glint=0.8)
        for dx, dy in ((-24, -34), (0, -46), (24, -34)):
            p.ball(48 + dx, 78 + dy, 7.5, 7.5, SPLIT, form=0.35, rim=0.8, glint=1.2)
        p.ball(48, 78, 6, 6, tone(SPLIT, light=-0.15), form=0.3, rim=0.6, glint=1.0)
    return p.result()


# ---- 패널·디스크·칸 -------------------------------------------------------------------------

def make_panel():
    """테두리 있는 패널 (9칸으로 늘림, 색을 곱함). 흰 윗면 + 아래 두께 + 위쪽 빛 테."""
    p = Painter(64, 64, lw=0)
    p._put(OUTLINE, p.m_rrect(3, 3, 61, 61, 18))
    p._put(NEUTRAL_SHADE, p.m_rrect(7, 7, 57, 57, 14))
    p._put((236, 237, 242, 255), p.m_rrect(7, 7, 57, 52, 14))
    p._put(WHITE, p.m_rrect(9, 9, 55, 50, 12))
    return p.result()


def make_panel_flat():
    p = Painter(64, 64, lw=0)
    p._put(WHITE, p.m_rrect(3, 3, 61, 61, 18))
    return p.result()


def make_disk():
    """스펠 디스크: 금 테 + 박힌 보석 + 새김 무늬 양피지 판 + 쇠 축."""
    p = Painter(240, 240, lw=4.0)
    c = 120
    p.paint(p.m_ellipse(c, c, 114, 114), GOLD, shade=rgba("d98a1a"), form=0.25, rim=2.4, glint=2.6)
    face = p.m_ellipse(c, c, 96, 96)
    p.paint(face, TILE, shade=rgba("ead7b0"), form=0.22, rim=1.6, glint=0, lw=3.0)

    def engrave(q):
        for r, w in ((80, 2.2), (62, 1.4)):
            q.arc(c, c, r, r, 0, 360, rgba("d8c094"), w)
            q.arc(c + 1, c + 1.4, r, r, 0, 360, (255, 255, 255, 160), 1.0)
        for i in range(24):
            t = 2 * math.pi * i / 24
            r0, r1 = (84, 92) if i % 2 == 0 else (86, 90)
            q.line([(c + r0 * math.cos(t), c + r0 * math.sin(t)), (c + r1 * math.cos(t), c + r1 * math.sin(t))], rgba("c9ac78"), 2.0, cap=False)
    p.clip(face, engrave)
    # 테에 박힌 보석 12개 (빨강·파랑 번갈아)
    for i in range(12):
        t = 2 * math.pi * i / 12
        x, y = c + 105 * math.cos(t), c + 105 * math.sin(t)
        gem = TEAM_RED if i % 2 == 0 else rgba("3d7fe0")
        p.ball(x, y, 5.6, 5.6, gem, lw=2.2, form=0.35, rim=0.8, glint=1.2)
        p.dot(x - 1.8, y - 1.8, 1.4, WHITE)
    p.ball(c, c, 17, 17, STEEL, form=0.35, rim=1.4, glint=2.0)
    p.ball(c, c, 6, 6, tone(STEEL, light=-0.3), form=0, rim=0, glint=0, lw=2.0)
    return p.result()


def make_socket():
    """빈 룬 칸: 돌 판에 파인 홈 (위쪽 턱이 그림자를 드리운다) + 새긴 +."""
    p = Painter(96, 96, lw=3.2)
    stone = rgba("d3cddc")
    p.paint(p.m_rrect(5, 5, 91, 91, 22), stone, form=0, side=0.0, rim=1.4, glint=2.0)
    hole = p.m_rrect(15, 15, 81, 81, 15)
    p.paint(hole, rgba("b7b0c6"), outline=tone(stone, light=-0.35), lw=1.6, form=0, rim=0, glint=0)
    p.cast(hole, [(10, 10), (86, 10), (86, 24), (10, 26)], alpha=70)
    p._put((255, 255, 255, 140), p.both(hole, p.cut(p.m_rrect(15, 15, 81, 81, 15), p.m_rrect(15, 13, 81, 79, 15))))
    plus = p.union(p.m_line([(48, 36), (48, 60)], 6), p.m_line([(36, 48), (60, 48)], 6))
    p.paint(plus, rgba("ece8f2"), shade=rgba("d0cada"), outline=tone(stone, light=-0.3), lw=1.4, form=0.3, rim=0, glint=0)
    return p.result()


# ---- 작은 아이콘 (64x64) -------------------------------------------------------------------

def make_icon(kind):
    p = Painter(64, 64, lw=3.0)
    if kind == "Heart":
        heart = p.union(p.m_ellipse(22, 25, 14, 14), p.m_ellipse(42, 25, 14, 14), p.m_poly([(9, 30), (32, 58), (55, 30), (32, 24)]))
        p.paint(heart, rgba("ff4f5a"), form=0.32, rim=1.6, glint=1.8)
        p.shine(19, 21, 4.5, 3.2, alpha=220)
    elif kind == "Coin":
        p.paint(p.m_ellipse(32, 35, 26, 25), rgba("d98a1a"), form=0, rim=0, glint=0)
        face = p.m_ellipse(32, 31, 26, 25)
        p.paint(face, GOLD, shade=rgba("e8a51e"), form=0.3, rim=1.4, glint=1.8)
        p.arc(32, 31, 17, 16, 0, 360, rgba("d98a1a"), 2.6)
        p.paint(p.m_poly(star_pts(32, 31, 9, 4, 5)), rgba("ffe07a"), shade=rgba("e8a51e"), lw=1.4, form=0.3, rim=0, glint=0)
        p.shine(22, 20, 4, 2.6, alpha=220)
    elif kind == "Flag":
        p.paint(p.m_line([(17, 58), (17, 8)], 5), WOOD, form=0, side=0.4, rim=0.6, glint=0.6, lw=2.2)
        p.ball(17, 7, 4, 4, GOLD, lw=2.0, form=0.3, rim=0.4, glint=0.8)
        cloth = p.m_poly([(19, 10), (36, 14), (54, 11), (48, 22), (55, 33), (36, 32), (19, 36)])
        p.paint(cloth, TEAM_RED, form=0.32, rim=1.2, glint=1.4, lw=2.4)
    elif kind == "Lock":
        shackle = p.m_line([(20, 30)] + [(32 + 12 * math.cos(math.radians(a)), 22 + 14 * math.sin(math.radians(a))) for a in range(180, 361, 15)] + [(44, 30)], 6)
        p.paint(shackle, STEEL, form=0.3, rim=0.8, glint=1.0, lw=2.4)
        body = p.m_rrect(11, 27, 53, 58, 8)
        p.paint(body, GOLD, shade=rgba("d98a1a"), form=0.3, rim=1.4, glint=1.6)
        p.dot(32, 40, 5, OUTLINE)
        p.paint(p.m_poly([(29, 41), (35, 41), (34, 51), (30, 51)]), OUTLINE, form=0, rim=0, glint=0, lw=0)
    elif kind in ("Star", "StarEmpty"):
        color = GOLD if kind == "Star" else rgba("d6d2e0")
        p.paint(p.m_poly(star_pts(32, 34, 28, 13, 5)), color, shade=rgba("e89a1a") if kind == "Star" else rgba("aaa4bc"),
                form=0.3, rim=1.4, glint=1.6)
        if kind == "Star":
            p.shine(25, 24, 3, 2, alpha=220)
    elif kind == "Up":
        arrow = p.m_poly([(32, 5), (58, 33), (43, 33), (43, 58), (21, 58), (21, 33), (6, 33)])
        p.paint(arrow, GREEN, shade=rgba("3f9a3a"), form=0.3, rim=1.4, glint=1.6)
    elif kind == "Sell":
        bag = p.union(p.m_ellipse(32, 43, 22, 17), p.m_poly([(22, 20), (42, 20), (48, 34), (16, 34)]))
        p.paint(bag, rgba("d9a86a"), form=0.32, rim=1.4, glint=1.4)
        p.paint(p.m_rrect(20, 17, 44, 23, 3), rgba("b98450"), form=0.2, rim=0.6, glint=0, lw=2.2)
        p.paint(p.m_poly([(24, 18), (18, 8), (32, 14), (46, 8), (40, 18)]), rgba("d9a86a"), form=0.25, rim=0.6, glint=0.8, lw=2.2)
        p.ball(32, 43, 9, 9, GOLD, lw=2.2, form=0.3, rim=0.6, glint=1.2)
        p.line([(32, 38), (32, 48)], rgba("d98a1a"), 2.2)
    elif kind == "Pause":
        white_paint(p, p.union(p.m_rrect(14, 10, 28, 54, 5), p.m_rrect(36, 10, 50, 54, 5)))
    elif kind == "Play":
        white_paint(p, p.m_poly([(16, 8), (54, 32), (16, 56)]))
    elif kind == "Fast":
        white_paint(p, p.union(p.m_poly([(6, 10), (32, 32), (6, 54)]), p.m_poly([(32, 10), (58, 32), (32, 54)])))
    elif kind == "Cycle":
        arc = p.m_line([(32 + 19 * math.cos(math.radians(a)), 33 + 19 * math.sin(math.radians(a))) for a in range(40, 301, 10)], 7)
        head = p.m_poly([(42, 3), (58, 20), (36, 26)])
        white_paint(p, p.union(arc, head))
    elif kind == "Wand":
        p.paint(p.m_line([(13, 55), (38, 26)], 6), WOOD, form=0.3, rim=0.8, glint=0.8, lw=2.4)
        p.paint(p.m_poly(star_pts(44, 19, 16, 7, 5)), GOLD, shade=rgba("e89a1a"), form=0.3, rim=1.2, glint=1.4)
        p.dot(40, 14, 1.8, WHITE)
    elif kind == "Check":
        p.paint(p.m_line([(12, 34), (26, 48), (52, 16)], 9), GREEN, shade=rgba("3f9a3a"), form=0.25, rim=1.0, glint=1.2)
    elif kind == "Skull":
        skull = p.union(p.m_ellipse(32, 28, 22, 20), p.m_rrect(21, 38, 43, 56, 5))
        p.paint(skull, rgba("fbf8f2"), form=0.3, rim=1.4, glint=0)
        for sx in (-1, 1):
            p.dot_e(32 + sx * 9, 29, 5.5, 6, OUTLINE)
            p.dot(32 + sx * 9 - 1.6, 27, 1.4, (255, 255, 255, 220))
        p.paint(p.m_poly([(32, 35), (35, 41), (29, 41)]), OUTLINE, form=0, rim=0, glint=0, lw=0)
        for x in (27, 32, 37):
            p.line([(x, 47), (x, 55)], tone(rgba("fbf8f2"), light=-0.4), 1.4, cap=False)
    return p.result()
