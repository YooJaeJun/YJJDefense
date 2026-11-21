"""명암·재질 톤으로 다시 그린 대포·마법사 탑·정령 제단과 마법사 (units_hd.py와 같은 톤)."""

import math

from paint import OUTLINE, Painter, bricks, ground, light_of, rgba, roof_tiles, tone, wood_planks
from units_hd import GOLD, SKIN, STONE, STONE_WARM, TEAM_RED, WOOD, WOOD_DARK, face, flag

BRONZE = rgba("d08a3a")
IRON = rgba("6d6f80")
VIOLET = rgba("7b55d6")
VIOLET_STONE = rgba("a99cc4")
CRYSTAL = rgba("7fe3ff")


def m_cloud(p, circles):
    return p.union(*[p.m_ellipse(x, y, r, r) for x, y, r in circles])


def star_pts(cx, cy, r_out, r_in, points, rot=-90.0):
    pts = []
    for i in range(points * 2):
        r = r_out if i % 2 == 0 else r_in
        t = math.radians(rot) + math.pi * i / points
        pts.append((cx + r * math.cos(t), cy + r * math.sin(t)))
    return pts


def crystal(p, cx, cy, s=1.0, color=CRYSTAL):
    """떠 있는 마력 수정 (면을 밝은 쪽·어두운 쪽으로 나눠 칠한다)."""
    p.paint(p.m_poly([(cx, cy - 17 * s), (cx + 10 * s, cy - 2 * s), (cx, cy + 17 * s), (cx - 10 * s, cy - 2 * s)]), color,
            form=0, rim=0, glint=0, lw=2.4)
    p.poly([(cx, cy - 17 * s), (cx + 10 * s, cy - 2 * s), (cx, cy + 17 * s)], tone(color, light=-0.3, sat=0.1))
    p.poly([(cx - 10 * s, cy - 2 * s), (cx, cy - 17 * s), (cx - 2 * s, cy - 2 * s)], light_of(color))
    p.line([(cx - 5 * s, cy - 6 * s), (cx - 2 * s, cy - 11 * s)], (255, 255, 255, 230), 1.6 * s)


# ---- 대포 ---------------------------------------------------------------------------------

def make_tower_bombard(level):
    p = Painter(240, 300, lw=3.4)
    cx = 120
    ground(p, cx, 278, 88, 14, seed=40 + level)

    # 둥근 돌 포대 + 나무 바닥
    stone = STONE if level < 2 else STONE_WARM
    base = p.union(p.m_rrect(50, 226, 190, 284, 10), p.m_ellipse(cx, 226, 70, 14))
    p.paint(base, stone, side=0.3, rim=2.0, glint=1.4, texture=bricks(50, 210, 190, 286, 20, 12, stone, seed=41 + level))
    p.paint(p.m_ellipse(cx, 224, 66, 13), WOOD, form=0.2, rim=1.4, glint=1.6,
            texture=wood_planks(54, 210, 186, 240, 12, WOOD, seed=42))

    # 포가 + 바퀴
    p.paint(p.m_poly([(84, 226), (150, 226), (142, 196), (96, 200)]), WOOD_DARK, form=0.3, glint=1.4,
            texture=wood_planks(84, 190, 150, 228, 9, WOOD_DARK, vertical=False, seed=43))
    for wx in (92, 146):
        p.paint(p.m_ellipse(wx, 222, 15, 15), WOOD, form=0.3, glint=1.4)
        for k in range(6):
            t = k * math.pi / 3
            p.line([(wx + 5 * math.cos(t), 222 + 5 * math.sin(t)), (wx + 13 * math.cos(t), 222 + 13 * math.sin(t))], OUTLINE, 1.6)
        p.paint(p.m_ellipse(wx, 222, 5, 5), IRON, form=0.3, glint=1.0, lw=1.8)

    # 포신 (단계마다 길고 굵게: 청동 → 쇠 → 검은 쇠 + 금테)
    length = 96 + level * 16
    width = 40 + level * 6
    bx, by = 118, 206
    ang = math.radians(-59)
    ux, uy = math.cos(ang), math.sin(ang)
    nx, ny = -uy, ux
    ex, ey = bx + ux * length, by + uy * length
    barrel_color = (BRONZE, IRON, rgba("4f5266"))[level]
    rear, front = width * 0.5, width * 0.42
    barrel = p.union(p.m_poly([(bx + nx * rear, by + ny * rear), (ex + nx * front, ey + ny * front),
                               (ex - nx * front, ey - ny * front), (bx - nx * rear, by - ny * rear)]),
                     p.m_ellipse(bx, by, rear, rear))
    p.paint(barrel, barrel_color, form=0.42, rim=2.0, glint=2.6)
    band = GOLD if level >= 1 else tone(BRONZE, light=-0.25)
    for k in [0.2, 0.62] + ([0.41] if level >= 2 else []):
        kx, ky = bx + ux * length * k, by + uy * length * k
        r = rear + (front - rear) * k + 3
        p.paint(p.m_poly([(kx + nx * r - ux * 4, ky + ny * r - uy * 4), (kx + nx * r + ux * 4, ky + ny * r + uy * 4),
                          (kx - nx * r + ux * 4, ky - ny * r + uy * 4), (kx - nx * r - ux * 4, ky - ny * r - uy * 4)]),
                band, form=0.35, rim=0.8, glint=1.0, lw=2.0)
    p.paint(p.m_blob(ex, ey, front + 5, (front + 5) * 0.48, angle=31), band, form=0.3, glint=1.4, lw=2.6)
    p.paint(p.m_blob(ex + ux * 2, ey + uy * 2, front - 1, (front - 1) * 0.42, angle=31), rgba("221820"), form=0, rim=0, glint=0, lw=0)
    p.shine(bx + ux * length * 0.45 - nx * width * 0.22, by + uy * length * 0.45 - ny * width * 0.22, 3, 6, 150)

    # 포수: 고글 쓴 콩 + 불붙은 막대
    gx, gy = 66, 186
    p.paint(p.m_blob(gx, gy + 22, 14, 12, power=2.4), rgba("c0563a"), form=0.3)
    p.paint(p.m_ellipse(gx, gy, 15, 14), SKIN, form=0.28, glint=1.4)
    face(p, gx + 2, gy + 3, look=3, eye_gap=5.5, eye_r=2.0, mouth="o")
    p.paint(p.m_rrect(gx - 15, gy - 15, gx + 15, gy - 5, 5), rgba("6b4a2e"), form=0.3, glint=1.2)
    for sx in (-1, 1):
        p.paint(p.m_ellipse(gx + sx * 6 + 2, gy - 11, 5.5, 5), rgba("9fe3ff"), form=0.3, rim=0.8, glint=1.0, lw=2.0)
    p.paint(p.m_line([(gx + 12, gy + 22), (gx + 30, gy + 4)], 3), WOOD, form=0, glint=0.6, lw=1.8)
    p.dot(gx + 31, gy + 2, 3.8, rgba("ff9a3c"))
    p.dot(gx + 31, gy + 1, 1.9, rgba("fff3a0"))
    if level >= 2:
        flag(p, 178, 214, 50, TEAM_RED)
    return p.result()


# ---- 마법사 탑 ----------------------------------------------------------------------------

def make_tower_mage(level):
    p = Painter(240, 300, lw=3.4)
    cx = 120
    ground(p, cx, 282, 70, 12, seed=50 + level)
    top = 150 - level * 14
    body = p.m_poly([(82, 286), (92, top), (148, top), (158, 286)])
    p.paint(body, VIOLET_STONE, side=0.32, rim=2.0, glint=1.6, texture=bricks(80, top, 160, 288, 17, 12, VIOLET_STONE, seed=51 + level))
    p.cast(body, [(82, top), (158, top), (158, top + 26), (82, top + 18)])
    # 문 + 빛나는 창
    p.paint(p.m_rrect(106, 240, 134, 288, 12), WOOD_DARK, form=0.3, glint=1.0, texture=wood_planks(106, 236, 134, 288, 7, WOOD_DARK, seed=52))
    p.rivets([(128, 266)], r=1.6, color=GOLD)
    win = p.union(p.m_rrect(110, top + 36, 130, top + 62, 2), p.m_ellipse(120, top + 37, 10, 9))
    p.paint(win, rgba("3a2a55"), form=0, rim=0, glint=0)
    p.dot_e(120, top + 47, 6, 9, rgba("9fe8ff", 230))
    p.dot_e(118, top + 43, 2.5, 3.5, (255, 255, 255, 220))
    if level >= 1:
        p.paint(p.m_rrect(86, top + 82, 154, top + 96, 3), VIOLET, form=0.3, glint=1.0)
        p.rivets([(98, top + 89), (120, top + 89), (142, top + 89)], r=1.4, color=GOLD)
    # 고깔 지붕 (기와) + 금테 + 별 + 수정
    tip = top - 100
    roof = p.m_poly([(64, top + 10), (cx, tip), (176, top + 10), (168, top + 18), (72, top + 18)])
    p.paint(roof, VIOLET, form=0.3, rim=2.2, glint=2.2, texture=roof_tiles(60, tip, 180, top + 20, 14, 8, VIOLET, seed=53))
    p.paint(p.m_rrect(66, top + 8, 174, top + 18, 4), GOLD, form=0.3, glint=1.2, lw=2.4)
    p.paint(p.m_poly(star_pts(cx, top - 28, 13, 6, 5)), GOLD, form=0.3, glint=1.2, lw=2.4)
    crystal(p, cx, tip - 22, 1.1)
    if level >= 2:
        crystal(p, 60, top - 26, 0.8, rgba("ff9ee6"))
        crystal(p, 180, top - 26, 0.8, rgba("b6ff8a"))
    return p.result()


# ---- 정령 제단: 얼굴 새긴 토템 돌 + 청동 그릇 ----------------------------------------------

def rune_marks(x0, y0, x1, y1, color):
    def draw(q):
        c = light_of(color)
        x, k = x0 + 6, 0
        while x < x1 - 6:
            if k % 3 == 0:
                q.line([(x, y0 + 2), (x + 4, y1 - 2)], c, 1.4, cap=False)
            elif k % 3 == 1:
                q.dot(x + 2, (y0 + y1) / 2, 1.8, c)
            else:
                q.line([(x, y1 - 2), (x + 3, y0 + 2), (x + 6, y1 - 2)], c, 1.4, cap=False)
            x += 11
            k += 1
    return draw


def make_tower_shrine(level):
    p = Painter(240, 300, lw=3.4)
    cx = 120
    ground(p, cx, 282, 80, 13, seed=60 + level)
    bands = (TEAM_RED, rgba("3fb6e8"), GOLD)
    stones = (STONE_WARM, STONE, rgba("c7bfa8"))
    y = 284
    for index in range(level + 1):
        stone = stones[index]
        hollow = tone(stone, light=-0.55)
        p.paint(p.m_rrect(60, y - 78, 180, y, 14), stone, side=0.28, rim=2.0, glint=1.8,
                texture=bricks(60, y - 78, 180, y, 40, 26, stone, seed=61 + index))
        p.paint(p.m_rrect(60, y - 78, 180, y - 60, 7), bands[index], form=0.3, glint=1.2, lw=2.6,
                texture=rune_marks(64, y - 74, 176, y - 64, bands[index]))
        for sx in (-1, 1):
            p.paint(p.m_ellipse(cx + sx * 24, y - 36, 9, 10), hollow, form=0, rim=0, glint=0, lw=2.4)
            p.dot(cx + sx * 24 + 2, y - 35, 3.5, rgba("fff3b0"))
        mouth = ("line", "o", "w")[index]
        if mouth == "line":
            p.paint(p.m_rrect(104, y - 20, 136, y - 14, 3), hollow, form=0, rim=0, glint=0, lw=2.2)
        elif mouth == "o":
            p.paint(p.m_ellipse(cx, y - 17, 8, 9), hollow, form=0, rim=0, glint=0, lw=2.2)
        else:
            p.arc(cx - 9, y - 22, 9, 9, 0, 180, OUTLINE, 4.5)
            p.arc(cx + 9, y - 22, 9, 9, 0, 180, OUTLINE, 4.5)
        y -= 76
    # 그릇 위 가장자리 = y-4 (게임이 원소 구슬을 여기 띄운다: UILayout ShrineOrb*)
    bowl = p.union(p.m_poly([(78, y - 4), (162, y - 4), (146, y + 14), (94, y + 14)]), p.m_ellipse(cx, y - 4, 42, 8))
    p.paint(bowl, BRONZE, form=0.4, rim=1.6, glint=2.0)
    p.paint(p.m_ellipse(cx, y - 4, 36, 5.5), rgba("5a3020"), form=0, rim=0, glint=0, lw=0)
    p.paint(p.m_rrect(108, y + 12, 132, y + 22, 3), tone(BRONZE, light=-0.2), form=0.3, glint=0.8, lw=2.2)
    return p.result()


# ---- 마법사 (성 앞 돌기둥 위) -------------------------------------------------------------

def make_wizard(frame):
    p = Painter(200, 320, lw=3.2)
    pillar = p.m_rrect(64, 186, 136, 316, 6)
    p.paint(pillar, STONE, side=0.3, rim=2.0, glint=1.6, texture=bricks(64, 186, 136, 316, 18, 13, STONE, seed=70))
    p.cast(pillar, [(64, 186), (136, 186), (136, 204), (64, 198)])
    cap = tone(STONE, light=0.08)
    p.paint(p.m_rrect(50, 170, 150, 194, 6), cap, form=0.3, rim=1.8, glint=1.8, texture=bricks(50, 170, 150, 194, 25, 12, cap, seed=71))

    # 지팡이 (휘두르면 수정이 더 빛난다)
    staff = [(56, 172), (60, 76)] if frame == 0 else [(60, 168), (34, 80)]
    p.paint(p.m_line(staff, 5), WOOD, form=0, rim=0.8, glint=1.0, lw=2.2)
    ox, oy = staff[1]
    p.paint(p.m_poly([(ox - 9, oy + 4), (ox - 6, oy - 8), (ox + 6, oy - 8), (ox + 9, oy + 4)]), GOLD, form=0.3, glint=1.0, lw=2.2)
    glow = 1.0 if frame == 0 else 1.3
    p.dot_e(ox, oy - 16, 20 * glow, 20 * glow, rgba("9fe8ff", 90))
    p.ball(ox, oy - 16, 12, 12, CRYSTAL, form=0.35, glint=2.0)
    p.shine(ox - 4, oy - 20, 3.5, 3, 230)

    # 로브 + 금띠
    p.paint(p.m_poly([(76, 172), (82, 128), (128, 126), (136, 172)]), VIOLET, form=0.35, glint=1.6)
    p.paint(p.m_rrect(78, 150, 134, 157, 2), GOLD, form=0.2, glint=0.8, lw=2.2)
    # 머리 + 수염
    p.paint(p.m_ellipse(106, 128, 24, 22), SKIN, form=0.28, glint=1.6)
    face(p, 109, 126, look=3, eye_gap=7, eye_r=2.4, mouth="line")
    beard = m_cloud(p, [(92, 146, 9), (102, 152, 10), (114, 152, 10), (124, 145, 9), (108, 162, 9)])
    p.paint(beard, rgba("f4f4fa"), form=0.3, rim=1.2, glint=1.4)
    p.paint(p.m_blob(111, 138, 13, 4.5), rgba("f4f4fa"), form=0.25, glint=0.8, lw=2.2)
    # 끝이 꺾인 고깔 모자 + 별
    p.paint(p.m_poly([(70, 116), (96, 58), (106, 30), (126, 50), (114, 62), (142, 116)]), VIOLET, form=0.34, rim=2.0, glint=2.0)
    p.paint(p.m_blob(106, 116, 48, 10), tone(VIOLET, light=-0.1), form=0.3, glint=1.6)
    p.paint(p.m_rrect(80, 102, 134, 110, 3), GOLD, form=0.2, glint=0.8, lw=2.2)
    p.paint(p.m_poly(star_pts(108, 84, 10, 4.5, 5)), GOLD, form=0.2, glint=1.0, lw=2.0)
    p.dot(92, 94, 2.2, light_of(GOLD))
    p.dot(118, 66, 1.8, light_of(GOLD))
    return p.result()
