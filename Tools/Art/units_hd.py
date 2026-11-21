"""명암·재질을 넣어 다시 그린 캐릭터와 타워 (paint.Painter 사용).

톤: 귀엽고 엉뚱한 얼굴(점 눈, 멍한 표정)은 그대로, 몸과 건물은
  - 진한 고동 외곽선, 2~3단 셀 명암(빛은 왼쪽 위), 가장자리 빛 띠
  - 재질이 보이게: 나뭇결 판자, 엇갈린 돌, 비늘 기와, 금속 리벳
  - 캐릭터마다 분명한 테마 (냄비 투구 병졸, 호피 치마 도깨비, 초록 두건 궁수)
캔버스 크기와 발 위치는 예전 그림과 같다 (게임 배치가 그대로 맞게).
"""

from paint import OUTLINE, Painter, bricks, grass_tuft, ground, light_of, rgba, roof_tiles, shade_of, straw, tone, wood_planks

SKIN = rgba("ffd3a8")
CHEEK = rgba("ff8f8f", 150)
STEEL = rgba("b7c2cf")
GOLD = rgba("ffc93c")
WOOD = rgba("b9773f")
WOOD_DARK = rgba("8a5530")
PLASTER = rgba("f3e3c3")
STONE = rgba("b9b4bf")
STONE_WARM = rgba("c9b9a6")
TEAM = rgba("3d7fe0")      # 우리 편 파랑
TEAM_RED = rgba("e2493b")
LEAF = rgba("5aa83c")
THATCH = rgba("e0b65a")


# ---- 얼굴 ---------------------------------------------------------------------------------

def face(p, cx, cy, look=3.0, eye_gap=7.0, eye_r=2.4, mouth="line", blush=True):
    """멍한 점 눈 얼굴 (눈에 작은 빛)."""
    for sx in (-1, 1):
        ex = cx + sx * eye_gap + look
        p.dot(ex, cy, eye_r, OUTLINE)
        p.dot(ex - eye_r * 0.35, cy - eye_r * 0.4, eye_r * 0.38, (255, 255, 255, 235))
    if blush:
        for sx in (-1, 1):
            p.dot_e(cx + sx * (eye_gap + 4) + look, cy + 4.5, 3.2, 1.8, CHEEK)
    mx = cx + look + 1
    if mouth == "line":
        p.line([(mx - 2.5, cy + 6), (mx + 2.5, cy + 6)], OUTLINE, 1.6)
    elif mouth == "o":
        p.dot_e(mx, cy + 6.5, 2.2, 2.8, OUTLINE)
    elif mouth == "grin":
        p.arc(mx, cy + 3.5, 4, 3.5, 20, 160, OUTLINE, 1.6)


# ---- 병졸: 냄비 투구, 파란 조끼, 나무 방패, 창 -------------------------------------------

def make_soldier(frame):
    p = Painter(96, 108, lw=2.6)
    attack = frame == 2
    step = 0 if attack else (4 if frame == 0 else -4)
    lean = 4 if attack else 0
    bob = 0 if frame == 0 else -1

    # 뒷다리, 앞다리 (장화)
    for x, d in ((40, -step), (52, step)):
        p.paint(p.m_rrect(x - 4 + d * 0.4, 80, x + 4 + d * 0.4, 96, 3), rgba("6b4a8c"), form=0.25, rim=1.0, glint=0)
        p.paint(p.m_blob(x + 2 + d, 98, 7.5, 4.6, power=2.6), WOOD_DARK, form=0.3, rim=1.0, glint=1.2)

    # 창 (몸 뒤)
    if attack:
        shaft = [(30, 66), (90, 60)]
        tip = (90, 60)
    else:
        shaft = [(66, 92), (76, 22)]
        tip = (76, 22)
    p.paint(p.m_line(shaft, 3.2), WOOD, form=0.0, rim=0.8, glint=0.8, lw=2.0)
    tx, ty = tip
    if attack:
        blade = [(tx - 2, ty - 5), (tx + 13, ty - 1.2), (tx - 2, ty + 4)]
    else:
        blade = [(tx - 4.5, ty + 3), (tx + 1.6, ty - 13), (tx + 5, ty + 3.8)]
    p.paint(p.m_poly(blade), STEEL, form=0.35, rim=0.8, glint=1.2, lw=2.0)

    # 몸통 (파란 조끼 + 허리띠)
    body = p.m_blob(46 + lean * 0.5, 72 + bob, 17, 15, power=2.4)
    p.paint(body, TEAM, form=0.32)
    p.clip(body, lambda q: (
        q.poly([(28, 76 + bob), (66, 76 + bob), (66, 81 + bob), (28, 81 + bob)], WOOD_DARK),
        q.poly([(43 + lean * 0.5, 75.5 + bob), (49 + lean * 0.5, 75.5 + bob), (49 + lean * 0.5, 81.5 + bob), (43 + lean * 0.5, 81.5 + bob)], GOLD),
        q.line([(46 + lean * 0.5, 59 + bob), (46 + lean * 0.5, 75 + bob)], tone(TEAM, light=0.35), 2.0, cap=False),
    ))

    # 머리 (크고 둥글게)
    hx, hy = 47 + lean, 44 + bob
    p.paint(p.m_ellipse(hx, hy, 19, 18), SKIN, form=0.28, rim=1.2, glint=1.6)
    face(p, hx + 2, hy + 3, look=3, eye_gap=6.5, eye_r=2.3, mouth="o" if attack else "line")

    # 냄비 투구: 둥근 몸통 + 테 + 손잡이 + 빨간 술
    p.paint(p.m_poly([(hx - 15, hy - 18), (hx - 3, hy - 22), (hx, hy - 32), (hx + 3, hy - 22)]), TEAM_RED, form=0.3, lw=2.0)
    pot = p.m_rrect(hx - 18, hy - 22, hx + 18, hy - 4, 6)
    p.paint(pot, STEEL, form=0.4, rim=1.2, glint=2.0)
    p.paint(p.m_rrect(hx - 22, hy - 7, hx + 22, hy - 1, 3), tone(STEEL, light=-0.1), form=0.3, glint=1.0)
    for sx in (-1, 1):
        p.paint(p.m_rrect(hx + sx * 22 - 3 + sx * 3, hy - 15, hx + sx * 22 + 3 + sx * 3, hy - 9, 2.5), tone(STEEL, light=-0.2), form=0, glint=0, lw=1.8)
    p.rivets([(hx - 10, hy - 4), (hx, hy - 4), (hx + 10, hy - 4)], r=1.1)
    p.shine(hx - 9, hy - 16, 3.5, 2.0, 170)

    # 방패 (앞)
    sx, sy = 25 + lean, 74 + bob
    p.paint(p.m_ellipse(sx, sy, 14, 15), WOOD, form=0.3, glint=1.6,
            texture=wood_planks(sx - 14, sy - 15, sx + 14, sy + 15, 6.5, WOOD, seed=3))
    rim = p.cut(p.m_ellipse(sx, sy, 14, 15), p.m_ellipse(sx, sy, 10.5, 11.5))
    p.paint(rim, STEEL, form=0.25, rim=0.8, glint=1.0, lw=1.6)
    p.paint(p.m_ellipse(sx, sy, 5.5, 6), TEAM, form=0.3, glint=1.0, lw=1.6)
    p.rivets([(sx, sy - 12.8), (sx - 12, sy), (sx + 12, sy), (sx, sy + 12.8)], r=1.0)
    return p.result()


# ---- 꼬마 도깨비: 붉은 피부, 노란 외뿔, 호피 치마, 징 박힌 방망이 -------------------------

IMP_SKIN = rgba("ff7d66")
TIGER = rgba("ffa53a")


def make_imp(frame):
    p = Painter(112, 124, lw=2.8)
    bob = 0 if frame == 0 else -3
    step = 5 if frame == 0 else -5

    # 다리
    for x, d in ((48, step), (64, -step)):
        p.paint(p.m_rrect(x - 4.5 + d * 0.3, 92 + bob, x + 4.5 + d * 0.3, 112, 4), IMP_SKIN, form=0.3, glint=0.8)
        p.paint(p.m_blob(x + 2 + d, 114, 7.5, 4.5, power=2.4), tone(IMP_SKIN, light=-0.35), form=0.3, glint=1.0)

    # 뒷팔
    p.paint(p.m_line([(28, 68 + bob), (14, 82 + step * 0.6)], 7.5), tone(IMP_SKIN, light=-0.12), form=0.2, glint=0.8)

    # 몸 (찹쌀떡) + 배
    body = p.m_blob(56, 66 + bob, 37, 33, power=2.2)
    p.paint(body, IMP_SKIN, form=0.3, rim=1.8, glint=2.2)
    p.paint(p.m_ellipse(60, 76 + bob, 18, 12), tone(IMP_SKIN, light=0.3), form=0.25, rim=0.8, glint=0, lw=0)

    # 호피 치마
    skirt = p.m_poly([(26, 82 + bob), (88, 82 + bob), (92, 98 + bob), (82, 94 + bob), (72, 100 + bob), (60, 94 + bob),
                      (48, 100 + bob), (36, 94 + bob), (22, 98 + bob)])
    def stripes(q):
        for i, x in enumerate(range(24, 96, 9)):
            q.poly([(x, 82 + bob), (x + 4, 82 + bob), (x + 1 + (i % 2) * 2, 99 + bob), (x - 2, 99 + bob)], rgba("4a2a1a"))
    p.paint(skirt, TIGER, form=0.3, rim=1.2, glint=1.0, texture=stripes)

    # 뿔 (고리 무늬)
    horn = p.m_poly([(47, 38 + bob), (58, 6 + bob), (71, 38 + bob)])
    def rings(q):
        for y in (16, 24, 32):
            q.line([(44, y + bob), (74, y + 2 + bob)], rgba("d88a1a"), 2.2, cap=False)
    p.paint(horn, GOLD, form=0.35, glint=1.6, texture=rings)

    # 머리털 한 줌
    p.paint(p.m_poly([(36, 40 + bob), (42, 30 + bob), (46, 38 + bob), (50, 28 + bob), (52, 37 + bob)]), rgba("3a2a3a"), form=0.2, glint=1.0, lw=2.0)

    # 큰 왕눈 (흰자 + 눈동자) + 송곳니
    for ex, er in ((48, 9.5), (70, 10.5)):
        p.paint(p.m_ellipse(ex, 55 + bob, er, er + 0.5), rgba("fffaf0"), form=0.25, rim=0.8, glint=0, lw=2.2)
        p.dot(ex + 3, 56 + bob, er * 0.42, OUTLINE)
        p.dot(ex + 1.6, 54 + bob, er * 0.15, (255, 255, 255, 240))
    p.paint(p.m_blob(62, 71 + bob, 9, 4.6, power=2.2), rgba("6b1f2a"), form=0, rim=0, glint=0, lw=2.0)
    p.paint(p.m_poly([(55, 68 + bob), (58, 75 + bob), (60, 68 + bob)]), rgba("fffaf0"), form=0, rim=0, glint=0, lw=1.4)
    p.paint(p.m_poly([(65, 68 + bob), (67, 74 + bob), (70, 68 + bob)]), rgba("fffaf0"), form=0, rim=0, glint=0, lw=1.4)
    p.dot_e(38, 66 + bob, 4.5, 2.5, rgba("ff4f5a", 120))

    # 앞팔 + 방망이
    swing = 0 if frame == 0 else 6
    hand = (96, 74 + bob - swing * 0.3)
    club_top = (104 + swing * 0.4, 44 + bob - swing)
    p.paint(p.m_line([club_top, (hand[0] - 1, hand[1] + 6)], 8.5), WOOD, form=0.3, glint=1.2,
            texture=wood_planks(80, 30, 112, 90, 40, WOOD, seed=5))
    p.paint(p.m_ellipse(club_top[0], club_top[1] + 4, 8.5, 11), WOOD, form=0.35, glint=1.6)
    for dx, dy in ((-5, 0), (5, 2), (0, 10), (-3, 9), (6, 9)):
        p.paint(p.m_poly([(club_top[0] + dx - 1.8, club_top[1] + dy), (club_top[0] + dx, club_top[1] + dy - 4),
                          (club_top[0] + dx + 1.8, club_top[1] + dy)]), STEEL, form=0, rim=0, glint=0.6, lw=1.4)
    p.paint(p.m_line([(84, 68 + bob), hand], 7.5), IMP_SKIN, form=0.2, glint=0.8)
    p.ball(hand[0], hand[1], 5.5, 5, IMP_SKIN, form=0.3, glint=1.0)
    return p.result()


# ---- 궁수 (타워 위) -----------------------------------------------------------------------

def archer(p, cx, cy, s=1.0, look=3.0):
    """초록 두건 궁수 상반신 (cy = 머리 중심)."""
    p.paint(p.m_blob(cx, cy + 22 * s, 15 * s, 12 * s, power=2.4), rgba("4e8f3a"), form=0.3)
    p.paint(p.m_ellipse(cx, cy, 15 * s, 14 * s), SKIN, form=0.28, glint=1.4)
    face(p, cx + 1.5 * s, cy + 2 * s, look=look * s, eye_gap=5.2 * s, eye_r=2.0 * s)
    hood = p.m_poly([(cx - 17 * s, cy + 1 * s), (cx - 12 * s, cy - 13 * s), (cx + 4 * s, cy - 26 * s),
                     (cx + 12 * s, cy - 12 * s), (cx + 17 * s, cy - 1 * s), (cx + 9 * s, cy - 6 * s), (cx - 9 * s, cy - 6 * s)])
    p.paint(hood, LEAF, form=0.32, glint=1.6)
    p.paint(p.m_poly([(cx + 3 * s, cy - 22 * s), (cx + 14 * s, cy - 34 * s), (cx + 8 * s, cy - 19 * s)]), TEAM_RED, form=0.2, lw=1.8)
    # 활
    bx = cx + 19 * s
    p.arc(bx - 6 * s, cy + 6 * s, 9 * s, 22 * s, -75, 75, OUTLINE, 4.2 * s)
    p.arc(bx - 6 * s, cy + 6 * s, 9 * s, 22 * s, -73, 73, WOOD, 2.2 * s)
    p.line([(bx - 3.5 * s, cy - 15 * s), (bx - 3.5 * s, cy + 27 * s)], rgba("f4efe4"), 0.9 * s, cap=False)


def flag(p, x, y_base, height, color, wave=0.0):
    p.paint(p.m_line([(x, y_base), (x, y_base - height)], 3.0), WOOD_DARK, form=0, rim=0.6, glint=0.6, lw=1.8)
    p.dot(x, y_base - height - 1, 3.2, OUTLINE)
    p.dot(x, y_base - height - 1, 2.2, GOLD)
    y = y_base - height + 2
    cloth = p.m_poly([(x + 1, y), (x + 18, y + 3 + wave), (x + 30, y + 1), (x + 24, y + 10), (x + 31, y + 19),
                      (x + 17, y + 18 + wave), (x + 1, y + 20)])
    p.paint(cloth, color, form=0.35, glint=1.2, lw=2.2)


def shield_badge(p, cx, cy, r, color=TEAM):
    m = p.m_poly([(cx - r, cy - r * 0.9), (cx + r, cy - r * 0.9), (cx + r * 0.95, cy + r * 0.2), (cx, cy + r * 1.15),
                  (cx - r * 0.95, cy + r * 0.2)])
    p.paint(m, color, form=0.35, glint=1.4)
    inner = p.m_poly([(cx - r * 0.55, cy - r * 0.55), (cx + r * 0.55, cy - r * 0.55), (cx + r * 0.5, cy + r * 0.1),
                      (cx, cy + r * 0.7), (cx - r * 0.5, cy + r * 0.1)])
    p.paint(inner, rgba("f6f1e6"), form=0.25, rim=0.8, glint=0, lw=1.6)
    p.paint(p.m_poly([(cx, cy - r * 0.45), (cx + r * 0.22, cy), (cx, cy + r * 0.45), (cx - r * 0.22, cy)]), color, form=0, rim=0, glint=0, lw=0)


def stone_base(p, x0, y0, x1, y1, seed, base=STONE):
    m = p.m_rrect(x0, y0, x1, y1, 6)
    p.paint(m, base, side=0.3, rim=2.0, glint=1.6, texture=bricks(x0, y0, x1, y1, 18, 11, base, seed=seed))
    return m


# ---- 보병 막사 ----------------------------------------------------------------------------

def make_tower_barracks(level):
    p = Painter(240, 300, lw=3.4)
    cx = 120
    # 바닥 돌 테두리
    ground(p, cx, 280, 94, 15, seed=11)

    if level == 0:
        # 통나무 오두막 + 초가 지붕
        wall = p.m_rrect(60, 196, 180, 280, 4)
        p.paint(wall, WOOD, side=0.3, rim=2.0, texture=wood_planks(60, 196, 180, 280, 10, WOOD, vertical=False, seed=1))
        p.cast(wall, [(60, 196), (180, 196), (180, 228), (60, 222)])
        roof_top, roof_y = 120, 206
        roof = p.m_poly([(38, roof_y), (cx, roof_top), (202, roof_y), (196, roof_y + 10), (44, roof_y + 10)])
        p.paint(roof, THATCH, form=0.3, rim=2.0, glint=2.0, texture=straw(38, roof_top, 202, roof_y + 12, THATCH, seed=2))
        door_top = 228
        badge_y = 168
    else:
        stone_top = 230 if level == 1 else 196
        base_mask = stone_base(p, 54, stone_top, 186, 282, seed=4 + level, base=STONE if level == 1 else STONE_WARM)
        if level == 1:
            # 회벽 + 나무 기둥 (반목조)
            upper = p.m_rrect(60, 176, 180, 234, 2)
            def timber(q):
                q.poly([(60, 176), (180, 176), (180, 234), (60, 234)], PLASTER)
                for x in (60, 98, 142, 174):
                    q.poly([(x, 176), (x + 7, 176), (x + 7, 234), (x, 234)], WOOD_DARK)
                q.line([(67, 230), (98, 182)], WOOD_DARK, 5, cap=False)
                q.line([(149, 182), (174, 230)], WOOD_DARK, 5, cap=False)
                q.poly([(60, 176), (180, 176), (180, 183), (60, 183)], WOOD_DARK)
            p.paint(upper, PLASTER, side=0.3, rim=2.0, texture=timber)
            p.cast(upper, [(60, 176), (180, 176), (180, 206), (60, 200)])
            p.cast(base_mask, [(54, 230), (186, 230), (186, 238), (54, 236)], 90)
            roof_top, roof_y = 96, 184
            roof_color = rgba("e0663a")
        else:
            roof_top, roof_y = 78, 200
            roof_color = rgba("3f6fd0")
        roof = p.m_poly([(32, roof_y), (cx, roof_top), (208, roof_y), (200, roof_y + 10), (40, roof_y + 10)])
        p.paint(roof, roof_color, form=0.3, rim=2.2, glint=2.2,
                texture=roof_tiles(28, roof_top, 212, roof_y + 12, 16, 9, roof_color, seed=7))
        if level == 2:
            p.cast(base_mask, [(54, 196), (186, 196), (186, 226), (54, 220)])
        # 용마루
        p.paint(p.m_poly([(cx - 4, roof_top - 4), (cx + 4, roof_top - 4), (206, roof_y + 2), (198, roof_y + 6), (cx, roof_top + 6),
                          (42, roof_y + 6), (34, roof_y + 2)]), GOLD if level == 2 else WOOD_DARK, form=0.3, glint=1.2, lw=2.2)
        door_top = 238 if level == 1 else 224
        badge_y = 150 if level == 1 else 140

    # 창문 (덧문)
    if level >= 1:
        wy = 196 if level == 1 else 212
        for wx in (78, 162):
            p.paint(p.m_rrect(wx - 11, wy - 12, wx + 11, wy + 12, 3), rgba("3a2b3f"), form=0, rim=0, glint=0)
            p.dot_e(wx - 3, wy - 4, 3, 4, rgba("ffd36b", 230))
            for sx in (-1, 1):
                p.paint(p.m_rrect(wx + sx * 13 - 4, wy - 12, wx + sx * 13 + 4, wy + 12, 1.5), TEAM, form=0.3, glint=0.8, lw=2.0)

    # 문: 아치 판자 + 쇠띠 + 손잡이
    door = p.union(p.m_rrect(100, door_top + 12, 140, 282, 2), p.m_ellipse(120, door_top + 14, 20, 16))
    p.paint(door, WOOD_DARK, form=0.3, rim=1.6, glint=1.2, texture=wood_planks(100, door_top - 4, 140, 282, 8, WOOD_DARK, seed=9))
    for y in (door_top + 22, 266):
        p.paint(p.m_rrect(100, y - 3, 140, y + 3, 1), tone(STEEL, light=-0.25), form=0, rim=0.6, glint=0.6, lw=1.8)
    p.rivets([(106, door_top + 22), (134, door_top + 22), (106, 266), (134, 266)], r=1.3)
    p.ball(132, 250, 3.2, 3.2, GOLD, form=0.3, glint=0.8, lw=1.8)

    # 방패 문장 + 깃발
    shield_badge(p, cx, badge_y, 15 + level * 2)
    if level == 2:
        flag(p, cx - 2, roof_top - 2, 50, TEAM_RED)
        flag(p, 40, roof_y + 4, 40, TEAM)
        flag(p, 202, roof_y + 4, 40, TEAM, wave=2)
    else:
        flag(p, cx - 2, roof_top - 2, 42, TEAM if level == 0 else TEAM_RED)
    return p.result()


# ---- 궁수 초소 ----------------------------------------------------------------------------

def make_tower_archer(level):
    p = Painter(240, 300, lw=3.4)
    cx = 120
    deck = 150 - level * 14          # 발판 높이

    if level == 0:
        # 통나무 망루: 기둥 넷 + X 버팀목
        for x0, x1 in ((72, 82), (158, 168)):
            p.paint(p.m_poly([(x0, 284), (x0 + 10, 284), (x1 - 2, deck + 10), (x1 - 12, deck + 10)]), WOOD, form=0.3, glint=1.4,
                    texture=wood_planks(60, deck, 180, 290, 30, WOOD, seed=x0))
        p.paint(p.m_line([(84, 270), (154, deck + 24)], 6), WOOD_DARK, form=0.2, glint=0.8)
        p.paint(p.m_line([(156, 270), (86, deck + 24)], 6), WOOD_DARK, form=0.2, glint=0.8)
        ground(p, cx, 284, 66, 11, seed=21)
    else:
        top = deck + 18
        ground(p, cx, 284, 74, 12, seed=22 + level)
        tower_mask = stone_base(p, 70, top, 170, 286, seed=20 + level, base=STONE if level == 1 else STONE_WARM)
        p.cast(tower_mask, [(70, top), (170, top), (170, top + 24), (70, top + 16)])
        # 화살 구멍 창
        p.paint(p.m_rrect(114, top + 34, 126, top + 62, 6), rgba("2e2433"), form=0, rim=0, glint=0)
        p.paint(p.m_rrect(108, 240, 132, 286, 10), WOOD_DARK, form=0.3, glint=1.0,
                texture=wood_planks(108, 236, 132, 286, 8, WOOD_DARK, seed=3))
        if level == 2:
            shield_badge(p, cx, top + 100 if top + 100 < 232 else 226, 13)

    # 궁수들 (난간 뒤)
    lift = 18 if level == 2 else 8
    if level >= 2:
        archer(p, 88, deck - 32 - lift, 0.9, look=3)
    archer(p, cx + 6, deck - 36 - lift, 1.0, look=4)

    # 발판 + 난간
    if level == 2:
        # 돌 성가퀴
        p.paint(p.m_rrect(56, deck - 8, 184, deck + 22, 4), STONE_WARM, form=0.25, rim=2.0, glint=1.6,
                texture=bricks(56, deck - 8, 184, deck + 22, 16, 10, STONE_WARM, seed=31))
        for x in (56, 90, 124, 158):
            p.paint(p.m_rrect(x, deck - 26, x + 26, deck - 4, 3), STONE_WARM, form=0.3, rim=1.6, glint=1.4,
                    texture=bricks(x, deck - 26, x + 26, deck - 4, 13, 11, STONE_WARM, seed=x))
    else:
        p.paint(p.m_rrect(58, deck - 4, 182, deck + 20, 3), WOOD, form=0.25, rim=2.0, glint=1.6,
                texture=wood_planks(58, deck - 4, 182, deck + 20, 8, WOOD, vertical=True, seed=13))
        p.paint(p.m_rrect(52, deck - 10, 188, deck - 2, 3), WOOD_DARK, form=0.25, glint=1.2,
                texture=wood_planks(52, deck - 10, 188, deck - 2, 40, WOOD_DARK, vertical=False, seed=14))
        if level == 1:
            for x in (60, 120, 180):
                p.rivets([(x, deck - 6)], r=1.6)
        for x in (60, 180):
            p.paint(p.m_rrect(x - 4, deck - 22, x + 4, deck + 20, 2), WOOD_DARK, form=0.25, glint=1.0)

    flag(p, 178, deck - 22, 52, TEAM_RED if level else TEAM)
    if level >= 1:
        flag(p, 60, deck - 22, 40, TEAM, wave=2)
    return p.result()
