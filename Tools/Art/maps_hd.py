"""지도 배경(풀밭·해변·설산)과 성문을 명암·외곽선·재질 톤으로 그린다 (generate_art.py가 쓴다).

좌표는 1920x1080 가상 화면 단위, 저장은 2560x1440.
  1) 땅: 큰 얼룩(어두운 풀·밝은 풀) + 잔 얼룩을 노이즈로 섞어 칠한다 (지도 크기 그대로, 매끈해서 확대 불필요)
  2) 길·물·풀잎: 지도 2배 캔버스(Painter)에 그려 줄인다 (외곽선이 매끈하게)
  3) 나무·바위·꽃·성문 같은 소품: 하나씩 작은 Painter로 그려 y순으로 얹는다
배경은 캐릭터보다 외곽선을 조금 얇고 연하게 해서 유닛·몬스터가 앞으로 튀어나와 보이게 한다.
"""

import math
import random

from PIL import Image, ImageChops, ImageDraw, ImageFilter, ImageOps

import map_data
from paint import OUTLINE, Painter, bricks, grass_tuft, light_of, rgba, roof_tiles, shade_of, tone, wood_planks
from units_hd import GOLD, STEEL, STONE, TEAM, TEAM_RED, WOOD, WOOD_DARK, flag

OUT_W, OUT_H = 2560, 1440
K = OUT_W / map_data.VIEW_W          # 가상 1단위 → 저장 픽셀
GROUND_SS = 2.0                      # 길·물 캔버스 배율 (저장 픽셀 기준)
PROP_SS = 3.0                        # 소품 캔버스 배율 (저장 픽셀 기준)
BG_LINE = (62, 42, 30, 255)          # 배경 외곽선: 캐릭터 먹선보다 살짝 연한 고동
BG_LW = 2.0
SHADOW = (24, 40, 30)                # 땅에 드리우는 그림자 색 (알파는 따로)

THEMES = {
    "meadow": dict(
        ground="86c456", ground_dark="5f9f48", ground_light="b2d968",
        blade_dark="4c8a3a", blade_light="c6e67c",
        road="e0b87a", road_dark="b98953", road_light="f4d9a2", road_line="6e4526",
        leaves=("6cbf4a", "4fa957", "8ccc46"), pine="3f8f5a",
        pond=(1040, 990, 150, 46), water="5ab4e6", water_deep="3486c8", shore="b48a58",
        shadow_alpha=95,
    ),
    "beach": dict(
        ground="f1d79c", ground_dark="dcbb7e", ground_light="fbe9bf",
        blade_dark="b8975c", blade_light="fff1cc",
        road="c88b4f", road_dark="94603a", road_light="e0aa6c", road_line="54321c",
        leaves=("5cb84a", "4aa35a", "79c447"), pine="3f8f5a",
        pond=(1060, 990, 140, 40), water="56c6e6", water_deep="2f97cf", shore="d8b277",
        shadow_alpha=80,
    ),
    "snow": dict(
        ground="e9f1fb", ground_dark="c7d8ef", ground_light="ffffff",
        blade_dark="b3c8e6", blade_light="ffffff",
        road="c6d6ec", road_dark="98aecf", road_light="e2ecf8", road_line="45567a",
        leaves=("3e8a72", "357c69", "4a967a"), pine="3e8a72",
        pond=(1060, 990, 150, 42), water="a9dcf8", water_deep="7cbfe8", shore="b7c7dc",
        shadow_alpha=70,
    ),
}


# ---- 공용 ---------------------------------------------------------------------------------

def noise(w, h, cell, seed):
    """부드러운 값 노이즈 (L 이미지, 0~255). cell이 클수록 큰 얼룩."""
    rng = random.Random(seed)
    sw, sh = w // cell + 4, h // cell + 4
    small = Image.frombytes("L", (sw, sh), bytes(rng.randrange(256) for _ in range(sw * sh)))
    big = small.resize((sw * cell, sh * cell), Image.BILINEAR).filter(ImageFilter.GaussianBlur(cell * 0.45))
    return big.crop((cell, cell, cell + w, cell + h))


def ramp(mask, lo, hi, top=255):
    """lo 아래는 0, hi 위는 top으로 늘이는 대비 조절."""
    span = max(hi - lo, 1)
    return mask.point(lambda v: 0 if v <= lo else (top if v >= hi else (v - lo) * top // span))


def prop(w, h, lw=BG_LW):
    """소품 캔버스: w x h 단위, 저장 픽셀의 PROP_SS배로 그린다."""
    return Painter(w, h, ss=PROP_SS * K, lw=lw)


def finish(p):
    size = (max(int(round(p.w * K)), 1), max(int(round(p.h * K)), 1))
    return p.img.convert("RGBa").resize(size, Image.LANCZOS).convert("RGBA")


def put(dst, img, x, y):
    """이미지 왼쪽 위를 (x, y) 픽셀에. 화면 밖으로 나간 부분은 자른다."""
    x, y = int(round(x)), int(round(y))
    x0, y0 = max(x, 0), max(y, 0)
    x1, y1 = min(x + img.width, dst.width), min(y + img.height, dst.height)
    if x1 <= x0 or y1 <= y0:
        return
    dst.alpha_composite(img.crop((x0 - x, y0 - y, x1 - x, y1 - y)), (x0, y0))


class Sprite:
    """소품 그림 + 발 위치(단위). 지도에 놓을 때 발이 (x, y)에 오게 한다."""

    def __init__(self, img, ax, ay):
        self.img, self.ax, self.ay = img, ax, ay

    def place(self, dst, x, y):
        put(dst, self.img, (x - self.ax) * K, (y - self.ay) * K)


def erode(p, mask, px):
    """px 단위만큼 안으로 줄인 마스크."""
    blurred = mask.filter(ImageFilter.GaussianBlur(px * p.ss * 0.5))
    return blurred.point(lambda v: 255 if v > 248 else 0)


# ---- 땅 ------------------------------------------------------------------------------------

def paint_ground(t, seed):
    w, h = OUT_W, OUT_H
    base = Image.new("RGBA", (w, h), rgba(t["ground"]))
    dark = Image.new("RGBA", (w, h), rgba(t["ground_dark"]))
    light = Image.new("RGBA", (w, h), rgba(t["ground_light"]))
    # 큰 얼룩: 어두운 풀밭과 밝은 풀밭이 섞인다
    img = Image.composite(dark, base, ramp(noise(w, h, 260, seed), 150, 205, 200))
    img = Image.composite(light, img, ramp(noise(w, h, 170, seed + 1), 160, 215, 170))
    # 잔 얼룩: 붓 자국 같은 결
    img = Image.composite(dark, img, ramp(noise(w, h, 14, seed + 2), 150, 255, 70))
    img = Image.composite(light, img, ramp(noise(w, h, 10, seed + 3), 165, 255, 60))
    return img


def ground_marks(p, theme, t, rng, free):
    """풀잎 획 (해변은 모래 물결, 설산은 눈 결)."""
    dark, light = rgba(t["blade_dark"]), rgba(t["blade_light"])
    for _ in range(1500):
        x, y = rng.uniform(0, 1920), rng.uniform(0, 1080)
        if not free(x, y, 4):
            continue
        s = rng.uniform(0.8, 1.4)
        if theme == "beach":
            # 바람 물결: 짧은 곡선 두 줄 (어두운 줄 아래 밝은 줄)
            p.arc(x, y, 12 * s, 3 * s, 200, 340, dark, 1.3)
            p.arc(x, y + 1.6, 12 * s, 3 * s, 210, 330, light, 1.0)
            continue
        if theme == "snow":
            p.arc(x, y, 14 * s, 3.5 * s, 200, 340, dark, 1.4)
            p.arc(x - 2, y + 1.6, 12 * s, 3 * s, 215, 320, light, 1.1)
            continue
        # 풀: 어두운 잎 세 가닥, 왼쪽에 밝은 잎 하나
        lean = rng.uniform(-2, 2)
        for dx, hgt in ((-3, 6), (0, 9), (3, 6)):
            p.line([(x + dx * s, y), (x + (dx * 1.6 + lean) * s, y - hgt * s)], dark, 1.5, cap=False)
        p.line([(x - 1.5 * s, y), (x + (-3.5 + lean) * s, y - 7 * s)], light, 1.1, cap=False)


# ---- 길 ------------------------------------------------------------------------------------

def path_mask(p, path, width):
    m = p._blank()
    d = ImageDraw.Draw(m)
    r = width * 0.5 * p.ss
    for x, y in path:
        d.ellipse([x * p.ss - r, y * p.ss - r, x * p.ss + r, y * p.ss + r], fill=255)
    return m


def along(path, offset):
    """길 중심선에서 옆으로 offset만큼 떨어진 선."""
    out = []
    for i, (x, y) in enumerate(path):
        a = path[max(i - 1, 0)]
        b = path[min(i + 1, len(path) - 1)]
        nx, ny = b[0] - a[0], b[1] - a[1]
        length = math.hypot(nx, ny) or 1.0
        out.append((x - ny / length * offset, y + nx / length * offset))
    return out


def paint_road(p, theme, t, path, rng):
    width = map_data.PATH_WIDTH
    road = path_mask(p, path, width)
    base, dark, light = rgba(t["road"]), rgba(t["road_dark"]), rgba(t["road_light"])

    # 길가 풀 그늘 (길 둘레가 살짝 어둡다)
    halo = road.filter(ImageFilter.GaussianBlur(9 * p.ss)).point(lambda v: min(255, v * 2))
    p._put(rgba(t["ground_dark"], 150), halo)

    if theme == "beach":
        # 나무 판자 길: 아래로 두께를 한 겹 깔고 판자를 길에 가로로 놓는다
        p._put(rgba(t["road_line"]), p._grow(p._shift(road, 0, 5), BG_LW + 0.4))
        p._put(dark, p._shift(road, 0, 5))
        p._put(rgba(t["road_line"]), p._grow(road, BG_LW + 0.4))

        def planks(q):
            step = 2
            for i in range(0, len(path) - step, step):
                (x0, y0), (x1, y1) = path[i], path[i + step]
                nx, ny = x1 - x0, y1 - y0
                length = math.hypot(nx, ny) or 1.0
                px, py = -ny / length * (width * 0.6), nx / length * (width * 0.6)
                c = tone(base, light=rng.uniform(-0.12, 0.1))
                quad = [(x0 - px, y0 - py), (x1 - px, y1 - py), (x1 + px, y1 + py), (x0 + px, y0 + py)]
                q.poly(quad, c)
                q.line([(x0 - px, y0 - py), (x0 + px, y0 + py)], tone(base, light=-0.5), 1.4, cap=False)
                # 나뭇결과 못
                for f in (-0.3, 0.25):
                    gx, gy = x0 + nx * 0.5 + px * f, y0 + ny * 0.5 + py * f
                    q.line([(gx - px * 0.25, gy - py * 0.25), (gx + px * 0.2, gy + py * 0.2)], tone(c, light=-0.2), 0.8, cap=False)
                for f in (-0.8, 0.8):
                    q.dot(x0 + nx * 0.5 + px * f, y0 + ny * 0.5 + py * f, 1.3, tone(base, light=-0.55))
        p._put(base, road)
        p.clip(road, planks)
        top = ImageChops.subtract(road, p._shift(road, 0, 4))
        p._put(rgba(t["road_light"], 190), top)
        return road

    p._put(rgba(t["road_line"]), p._grow(road, BG_LW))
    p._put(base, road)

    def dirt(q):
        # 흙 얼룩
        for _ in range(len(path) // 2):
            x, y = rng.choice(path)
            x += rng.uniform(-30, 30)
            y += rng.uniform(-30, 30)
            c = light if rng.random() < 0.5 else tone(base, light=-0.1)
            q.dot_e(x, y, rng.uniform(10, 26), rng.uniform(5, 11), (c[0], c[1], c[2], 150))
        if theme == "snow":
            # 썰매 자국 두 줄 + 발자국
            for off in (-15, 15):
                q.line(along(path, off), dark, 2.2)
                q.line(along(path, off + 1.6), light, 1.0)
            for i in range(4, len(path), 5):
                x, y = along(path, rng.choice((-30, -26, 26, 30)))[i]
                q.dot_e(x, y, 2.6, 1.8, tone(dark, light=-0.1))
            return
        # 바퀴 자국
        for off in (-16, 16):
            q.line(along(path, off), (dark[0], dark[1], dark[2], 110), 3.0)
        # 자갈: 외곽선 + 색 + 반짝
        for i in range(2, len(path) - 2, 2):
            if rng.random() < 0.45:
                continue
            x, y = along(path, rng.uniform(-34, 34))[i]
            r = rng.uniform(1.6, 3.6)
            c = tone(rgba("b9ab98"), light=rng.uniform(-0.15, 0.15))
            q.dot_e(x, y, r + 0.9, r * 0.7 + 0.9, BG_LINE)
            q.dot_e(x, y, r, r * 0.7, c)
            q.dot_e(x - r * 0.3, y - r * 0.25, r * 0.4, r * 0.25, light_of(c))
    p.clip(road, dirt)

    # 위쪽 가장자리는 풀 턱이 드리운 그늘, 아래쪽은 빛 받는 턱 → 살짝 파인 길
    top = ImageChops.subtract(road, p._shift(road, 0, 7))
    p._put(rgba(t["road_dark"], 210), top.filter(ImageFilter.GaussianBlur(1.2 * p.ss)))
    edge = ImageChops.subtract(road, erode(p, road, 7))
    p._put(rgba(t["road_dark"], 90), p.both(edge, road))
    bottom = ImageChops.subtract(road, p._shift(road, 0, -3))
    p._put(rgba(t["road_light"], 200), bottom)
    return road


def make_edge_tuft(theme, t, seed):
    """길 가장자리를 덮는 풀포기 (외곽선 한 줄이 기계적으로 보이지 않게). 설산은 눈 둔덕."""
    rng = random.Random(seed)
    p = prop(34, 22, lw=1.3)
    ax, ay = 17, 18
    if theme == "snow":
        m = p.m_blob(ax, ay - 3, rng.uniform(9, 13), rng.uniform(4, 6), power=2.0, wobble=0.1, seed=seed)
        p.paint(m, rgba(t["ground"]), shade=rgba(t["ground_dark"]), outline=rgba(t["blade_dark"]), lw=1.0, form=0.4, rim=0.6, glint=0, soft=0.8)
        return Sprite(finish(p), ax, ay)
    blades = []
    for j in range(rng.randint(3, 5)):
        bx = ax + (j - 2) * 3.4 + rng.uniform(-1, 1)
        hgt = rng.uniform(6, 13)
        lean = rng.uniform(-4, 4)
        blades.append(p.m_poly([(bx - 2.4, ay + 1), (bx + lean, ay - hgt), (bx + 2.4, ay + 1)]))
    green = tone(rgba(t["ground"]), light=rng.uniform(-0.08, 0.06))
    p.paint(p.union(*blades), green, light=rgba(t["blade_light"]), outline=BG_LINE, form=0.35, rim=0.5, glint=0.7)
    return Sprite(finish(p), ax, ay)


def edge_tuft_spots(path, rng):
    spots = []
    for i in range(3, len(path) - 3, 3):
        if rng.random() < 0.6:
            continue
        side = rng.choice((-1, 1))
        spots.append(along(path, side * (map_data.PATH_WIDTH * 0.5 + rng.uniform(-1, 2)))[i])
    return spots


# ---- 물 ------------------------------------------------------------------------------------

def paint_pond(p, theme, t, rng):
    px, py, prx, pry = t["pond"]
    shore = p.m_blob(px, py + 3, prx + 12, pry + 9, power=2.1, wobble=0.04, seed=3)
    water = p.m_blob(px, py, prx, pry, power=2.1, wobble=0.04, seed=3)
    p.paint(shore, rgba(t["shore"]), outline=BG_LINE, lw=BG_LW, form=0.2, rim=1.2, glint=0, soft=1.0)
    deep = rgba(t["water_deep"])
    if theme == "snow":
        # 언 연못: 얼음판 + 금 + 비스듬한 반짝
        p.paint(water, rgba(t["water"]), outline=rgba(t["road_line"]), lw=1.6, form=0, rim=0, glint=0)
        p._put(rgba("d8f1ff", 220), erode(p, water, 10).filter(ImageFilter.GaussianBlur(6 * p.ss)))

        def ice(q):
            for x0, y0, x1, y1 in ((-90, -18, -30, 6), (-30, 6, 30, -10), (20, 14, 90, -4), (-30, 6, -10, 30)):
                q.line([(px + x0, py + y0), (px + x1, py + y1)], deep, 1.6)
                q.line([(px + x0, py + y0 + 1.4), (px + x1, py + y1 + 1.4)], rgba("ffffff", 200), 0.9)
            for i in range(3):
                q.poly([(px - 110 + i * 22, py + 30), (px - 80 + i * 22, py - 40), (px - 70 + i * 22, py - 40), (px - 100 + i * 22, py + 30)],
                       (255, 255, 255, 120))
        p.clip(water, ice)
        return
    p.paint(water, rgba(t["water"]), outline=None, form=0, rim=0, glint=0)
    p._put(deep, erode(p, water, 14).filter(ImageFilter.GaussianBlur(8 * p.ss)))
    # 물가 그늘 (물 위쪽 가장자리)과 밝은 물가 띠
    p._put(rgba(t["water_deep"], 200), ImageChops.subtract(water, p._shift(water, 0, 8)).filter(ImageFilter.GaussianBlur(1.5 * p.ss)))
    p._put(rgba("bff0ff", 220), p.both(ImageChops.subtract(water, p._shift(water, 0, -3)), water))

    def ripples(q):
        for dx, dy, r in ((-60, -8, 26), (40, 10, 20), (90, -14, 14)):
            q.arc(px + dx, py + dy, r, r * 0.28, 200, 340, rgba("ffffff", 230), 2.0)
    p.clip(water, ripples)
    if theme == "meadow":
        # 연잎
        for dx, dy, r in ((-100, 6, 13), (-82, 18, 9), (100, 4, 11)):
            pad = p.cut(p.m_ellipse(px + dx, py + dy, r, r * 0.55), p.m_poly([(px + dx, py + dy), (px + dx + r, py + dy - r * 0.3),
                                                                                (px + dx + r, py + dy + r * 0.1)]))
            p.paint(pad, rgba("5fb24a"), outline=BG_LINE, lw=1.4, form=0.3, rim=0.8, glint=1.0)
        p.dot(px - 104, py + 2, 3.4, BG_LINE)
        p.dot(px - 104, py + 2, 2.4, rgba("ff9ec4"))
    p._put(rgba(t["road_line"]), ImageChops.subtract(p._grow(water, 1.4), water))


def paint_sea(p, t, sea_y, rng):
    """해변 위쪽 바다: 젖은 모래 → 거품 → 얕은 물 → 깊은 물."""
    shore = [(x, sea_y + 6 * math.sin(x / 38.0) + 3 * math.sin(x / 13.0)) for x in range(-20, 1960, 6)]
    wet = p.m_poly([(-20, -20)] + [(x, y + 18) for x, y in shore] + [(1960, -20)])
    p._put(rgba("dcbb7e"), wet)
    p._put(rgba("cfa96c", 160), ImageChops.subtract(wet, p._shift(wet, 0, -4)))
    water = p.m_poly([(-20, -20)] + shore + [(1960, -20)])
    p._put(rgba(t["road_line"]), p._grow(water, BG_LW))
    # 위로 갈수록 깊은 색
    grad = ImageOps.invert(Image.linear_gradient("L")).resize((water.size[0], int((sea_y + 20) * p.ss)), Image.BILINEAR)
    full = Image.new("L", water.size, 0)
    full.paste(grad, (0, 0))
    p._put(rgba("7fdcea"), water)
    p._put(rgba("2f8fd0"), p.both(full, water))
    # 거품 띠: 물가를 따라 하얀 몽글몽글
    foam = p.union(*[p.m_ellipse(x, y - 3, rng.uniform(9, 15), rng.uniform(4, 6)) for x, y in shore[::3]])
    foam = p.both(foam, p._grow(water, 2))
    p.paint(foam, rgba("ffffff"), shade=rgba("bfeaf4"), outline=None, form=0.5, rim=0.8, glint=0, soft=0.6)

    def waves(q):
        for row, y in enumerate((26, 58, 88)):
            for x in range(20 + row * 70, 1940, 160):
                q.arc(x + 30, y, 22, 6, 200, 340, rgba("ffffff", 200 - row * 30), 2.2)
                q.arc(x + 30, y + 2.4, 22, 6, 215, 325, rgba("1d6fb0", 120), 1.2)
    p.clip(water, waves)


# ---- 소품 ---------------------------------------------------------------------------------

def make_round_tree(s, seed, leaf, face=False, fruit=False):
    rng = random.Random(seed)
    w, h = 112 * s, 132 * s
    ax, ay = w / 2, h - 8 * s
    p = prop(w, h)
    trunk = p.union(p.m_poly([(ax - 7 * s, ay), (ax - 4.5 * s, ay - 46 * s), (ax + 4.5 * s, ay - 46 * s), (ax + 7 * s, ay)]),
                    p.m_poly([(ax - 12 * s, ay + 1), (ax - 5 * s, ay - 8 * s), (ax - 2 * s, ay + 1)]),
                    p.m_poly([(ax + 12 * s, ay + 1), (ax + 5 * s, ay - 8 * s), (ax + 2 * s, ay + 1)]))
    p.paint(trunk, WOOD, side=0.45, rim=0.8, glint=0.8)
    # 잎 뭉치: 뒤(위)부터 앞(아래)으로. 뭉치마다 외곽선이 있어 덩어리가 읽힌다
    clumps = [(ax + rng.uniform(-4, 4) * s, ay - 82 * s, 25 * s),
              (ax - 24 * s, ay - 60 * s, 21 * s), (ax + 24 * s, ay - 62 * s, 21 * s),
              (ax + rng.uniform(-6, 6) * s, ay - 50 * s, 22 * s)]
    color = rgba(leaf)
    for i, (cx, cy, r) in enumerate(clumps):
        r *= rng.uniform(0.92, 1.08)
        m = p.m_blob(cx, cy, r, r * 0.88, power=2.0, wobble=0.07, seed=seed + i)
        p.paint(m, tone(color, light=0.04 * (i - 1)), form=0.32, rim=1.4, glint=1.6, soft=1.0)

        def flecks(q, cx=cx, cy=cy, r=r, i=i):
            # 잎 결: 그늘 쪽엔 어두운 갈매기, 빛 쪽엔 밝은 점
            for _ in range(7):
                a = rng.uniform(-0.3, 2.0)
                d = rng.uniform(0.35, 0.8) * r
                x, y = cx + math.cos(a) * d, cy + math.sin(a) * d
                q.arc(x, y, 3.5 * s, 2.2 * s, 200, 340, shade_of(color), 1.2)
            for _ in range(3):
                a = rng.uniform(3.4, 4.6)
                d = rng.uniform(0.3, 0.6) * r
                q.dot_e(cx + math.cos(a) * d, cy + math.sin(a) * d, 2.6 * s, 1.8 * s, light_of(color))
        p.clip(m, flecks)
    if fruit:
        for fx, fy in ((-18, -66), (14, -78), (22, -50), (-6, -44)):
            p.ball(ax + fx * s, ay + fy * s, 3.2 * s, 3.2 * s, rgba("ff5a4f"), lw=1.3, form=0.35, glint=0, rim=0.6)
            p.shine(ax + fx * s - 1, ay + fy * s - 1, 1.0, 0.8)
    if face:
        p.dot(ax - 6 * s, ay - 52 * s, 2.2 * s, OUTLINE)
        p.dot(ax + 6 * s, ay - 52 * s, 2.2 * s, OUTLINE)
        p.line([(ax - 2 * s, ay - 45 * s), (ax + 2 * s, ay - 45 * s)], OUTLINE, 1.6)
    return Sprite(finish(p), ax, ay)


def make_pine(s, seed, leaf, snowy=False, face=False):
    rng = random.Random(seed)
    w, h = 80 * s, 140 * s
    ax, ay = w / 2, h - 6 * s
    p = prop(w, h)
    p.paint(p.m_rrect(ax - 5 * s, ay - 26 * s, ax + 5 * s, ay, 2 * s), WOOD, side=0.45, rim=0.8, glint=0.6)
    color = rgba(leaf)
    tiers = ((34, 26, 74), (28, 52, 98), (21, 76, 124))   # (반폭, 밑, 꼭대기)
    for i, (hw, bottom, top) in enumerate(tiers):
        hw *= s
        yb, yt = ay - bottom * s, ay - top * s
        # 밑단은 톱니처럼
        teeth = 5
        pts = [(ax, yt)]
        for k in range(teeth + 1):
            x = ax + hw - 2 * hw * k / teeth
            pts.append((x, yb + (4 * s if k % 2 == 0 else -2 * s)))
        m = p.m_poly(pts)
        p.paint(m, tone(color, light=0.05 * i), side=0.42, rim=1.2, glint=1.4)
        if snowy:
            cap = p.m_poly([(ax, yt - 1)] + [(ax + hw * 0.62 - 1.24 * hw * k / 6, yt + (yb - yt) * (0.5 + (0.12 if k % 2 else -0.04)))
                                             for k in range(7)])
            p.paint(p.both(cap, p._grow(m, 1)), rgba("ffffff"), lw=1.3, form=0.35, rim=0.8, glint=0, soft=0.5)
        else:
            def needles(q, yb=yb, yt=yt, hw=hw):
                for _ in range(6):
                    f = rng.uniform(0.35, 0.95)
                    y = yt + (yb - yt) * f
                    x = ax + rng.uniform(-0.7, 0.7) * hw * f
                    q.line([(x, y), (x + 3 * s, y + 3 * s)], shade_of(color), 1.1, cap=False)
            p.clip(m, needles)
    if face:
        p.dot(ax - 5 * s, ay - 44 * s, 2.0 * s, OUTLINE)
        p.dot(ax + 5 * s, ay - 44 * s, 2.0 * s, OUTLINE)
    return Sprite(finish(p), ax, ay)


def make_palm(s, seed, leaf, lean):
    rng = random.Random(seed)
    w, h = 150 * s, 150 * s
    ax, ay = w / 2, h - 6 * s
    p = prop(w, h)
    top = (ax + lean * s, ay - 96 * s)
    trunk_pts = [(ax + lean * s * (t ** 1.6), ay - 96 * s * t) for t in [i / 10 for i in range(11)]]
    trunk = p.union(*[p.m_ellipse(x, y, (8.5 - 3 * i / 10) * s, 5 * s) for i, (x, y) in enumerate(trunk_pts)])
    bark = rgba("c99a5e")

    def rings(q):
        for x, y in trunk_pts[1:-1]:
            q.arc(x, y + 2 * s, 9 * s, 3 * s, 10, 170, tone(bark, light=-0.45), 1.4)
            q.arc(x, y + 0.6 * s, 8 * s, 2.6 * s, 190, 260, light_of(bark), 1.0)
    p.paint(trunk, bark, side=0.45, rim=1.0, glint=0.8, texture=rings)
    color = rgba(leaf)
    # 잎: 꼭대기에서 휘어져 내려오는 톱니 잎 여섯 장 (뒤쪽 잎부터)
    for i, angle in enumerate((-150, -30, -110, -70, 190, -10)):
        a = math.radians(angle)
        length = rng.uniform(48, 58) * s
        spine = []
        for k in range(9):
            f = k / 8
            x = top[0] + math.cos(a) * length * f
            y = top[1] + math.sin(a) * length * f * 0.6 + (f ** 2) * 26 * s
            spine.append((x, y))
        upper, lower = [], []
        for k, (x, y) in enumerate(spine):
            wdt = (1 - k / 8) ** 0.7 * 9 * s
            notch = 0.55 if k % 2 else 1.0
            upper.append((x, y - wdt * notch))
            lower.append((x, y + wdt * notch * 0.6))
        m = p.m_poly(upper + lower[::-1])
        p.paint(m, tone(color, light=-0.06 if i < 2 else 0.02 * i), form=0.3, rim=1.0, glint=1.2, soft=0.6)
        p.line(spine[:-1], shade_of(color), 1.2)
    for dx, dy in ((-5, 4), (5, 5), (0, 9)):
        p.ball(top[0] + dx * s, top[1] + dy * s, 5 * s, 5 * s, rgba("8f5c36"), lw=1.5, form=0.35, glint=1.0, rim=0.8)
    return Sprite(finish(p), ax, ay)


def make_bush(s, seed, leaf, berries):
    rng = random.Random(seed)
    w, h = 70 * s, 46 * s
    ax, ay = w / 2, h - 5 * s
    p = prop(w, h)
    color = rgba(leaf)
    for i, (dx, dy, r) in enumerate(((0, -22, 15), (-15, -12, 12), (15, -13, 12))):
        m = p.m_blob(ax + dx * s, ay + dy * s, r * s, r * 0.85 * s, wobble=0.08, seed=seed + i)
        p.paint(m, color, form=0.32, rim=1.2, glint=1.2, soft=0.8)
    if berries:
        for _ in range(5):
            x, y = ax + rng.uniform(-20, 20) * s, ay + rng.uniform(-28, -8) * s
            p.dot(x, y, 2.6 * s, BG_LINE)
            p.dot(x, y, 1.8 * s, rgba(berries))
            p.dot(x - 0.6, y - 0.6, 0.6 * s, (255, 255, 255, 220))
    return Sprite(finish(p), ax, ay)


def make_rock(r, seed, theme, face):
    rng = random.Random(seed)
    w, h = r * 2.6, r * 2.2
    ax, ay = w / 2, h - r * 0.3
    p = prop(w, h)
    cy = ay - r * 0.72
    base = rgba("c2bcb3") if theme != "beach" else rgba("cdb8a0")
    m = p.m_blob(ax, cy, r, r * 0.78, power=2.6, wobble=0.1, seed=seed)
    small = p.m_blob(ax + r * 0.85, ay - r * 0.3, r * 0.42, r * 0.32, power=2.4, wobble=0.1, seed=seed + 1)

    def cracks(q):
        q.line([(ax + r * 0.1, cy - r * 0.6), (ax + r * 0.2, cy - r * 0.2), (ax + r * 0.45, cy - r * 0.05)], shade_of(base), 1.3)
        q.line([(ax - r * 0.6, cy + r * 0.2), (ax - r * 0.35, cy + r * 0.35)], shade_of(base), 1.1)
        if theme == "meadow":
            q.dot_e(ax - r * 0.3, cy - r * 0.62, r * 0.5, r * 0.26, rgba("6cae4a"))
            q.dot_e(ax - r * 0.42, cy - r * 0.68, r * 0.2, r * 0.1, rgba("9ccf62"))
        if theme == "snow":
            q.dot_e(ax - r * 0.05, cy - r * 0.7, r * 0.85, r * 0.36, rgba("ffffff"))
            q.dot_e(ax + r * 0.2, cy - r * 0.48, r * 0.5, r * 0.1, rgba("d4def2"))
        if theme == "beach":
            for _ in range(3):
                x, y = ax + rng.uniform(-0.5, 0.5) * r, cy + rng.uniform(0.0, 0.5) * r
                q.dot(x, y, 1.6, BG_LINE)
                q.dot(x, y, 1.0, rgba("f4ece0"))
    p.paint(small, base, form=0.35, rim=1.2, glint=1.0)
    p.paint(m, base, form=0.38, rim=1.6, glint=1.6, texture=cracks)
    if face:
        p.dot(ax - r * 0.25, cy + r * 0.05, 1.9, OUTLINE)
        p.dot(ax + r * 0.25, cy + r * 0.05, 1.9, OUTLINE)
        p.line([(ax - r * 0.1, cy + r * 0.3), (ax + r * 0.1, cy + r * 0.3)], OUTLINE, 1.3)
    return Sprite(finish(p), ax, ay)


def make_flower(theme, seed):
    rng = random.Random(seed)
    p = prop(26, 26, lw=1.3)
    cx, cy = 13, 14
    if theme == "beach":
        if rng.random() < 0.5:
            pts = []
            rot = rng.uniform(0, 6.28)
            for k in range(10):
                rad = 9 if k % 2 == 0 else 4
                a = rot + k * math.pi / 5
                pts.append((cx + math.cos(a) * rad, cy + math.sin(a) * rad * 0.8))
            p.paint(p.m_poly(pts), rgba("ff9466"), form=0.3, rim=0.6, glint=0.8)
            for k in range(5):
                a = rot + k * 2 * math.pi / 5
                p.dot(cx + math.cos(a) * 4, cy + math.sin(a) * 3.2, 0.8, rgba("ffe1c8"))
        else:
            shell = p.m_poly([(cx, cy + 5)] + [(cx + math.cos(a) * 8, cy + 2 - abs(math.sin(a)) * 9) for a in [math.pi * k / 8 for k in range(9)]])
            p.paint(shell, rgba("ffd9e2"), form=0.3, rim=0.6, glint=0.8)
            for dx in (-4, 0, 4):
                p.line([(cx, cy + 4), (cx + dx * 1.4, cy - 4)], rgba("e39bb0"), 0.9)
        return Sprite(finish(p), cx, cy + 6)
    if theme == "snow":
        m = p.m_poly([(cx, cy - 9), (cx + 5, cy), (cx, cy + 6), (cx - 5, cy)])
        p.paint(m, rgba("9fdcff"), form=0.4, rim=0.6, glint=1.0)
        p.shine(cx - 1.5, cy - 3, 1.2, 2.0)
        return Sprite(finish(p), cx, cy + 6)
    petal = rng.choice((rgba("ffffff"), rgba("ff9ec4"), rgba("ffd84a"), rgba("b9a4ff")))
    petals = p.union(*[p.m_ellipse(cx + 5.2 * math.cos(2 * math.pi * k / 5), cy + 4.6 * math.sin(2 * math.pi * k / 5), 4.2, 3.8) for k in range(5)])
    p.paint(petals, petal, form=0.3, rim=0.6, glint=0.8)
    p.ball(cx, cy, 2.4, 2.2, rgba("ff9a3c"), lw=1.0, form=0.3, glint=0, rim=0.4)
    return Sprite(finish(p), cx, cy + 6)


def make_tuft(theme, seed):
    p = prop(30, 20, lw=1.4)
    if theme == "beach":
        # 바닷가 풀: 길고 누런 잎
        rng = random.Random(seed)
        blades = [p.m_poly([(15 + (j - 2) * 3 - 1.6, 18), (15 + (j - 2) * 5 + rng.uniform(-2, 2), 18 - rng.uniform(10, 16)), (15 + (j - 2) * 3 + 1.6, 18)])
                  for j in range(5)]
        p.paint(p.union(*blades), rgba("a9c25a"), form=0.35, rim=0.6, glint=0.6)
        return Sprite(finish(p), 15, 18)
    return make_edge_tuft(theme, THEMES[theme], seed)


def make_sign(theme):
    p = prop(110, 80)
    ax, ay = 55, 74
    p.paint(p.m_rrect(ax - 4, ay - 52, ax + 4, ay, 2), WOOD_DARK, side=0.45, rim=0.8, glint=0.6)
    board = p.m_rrect(ax - 44, ay - 72, ax + 44, ay - 36, 5)
    p.paint(board, WOOD, side=0.25, rim=1.4, glint=1.4, texture=wood_planks(ax - 44, ay - 72, ax + 44, ay - 36, 12, WOOD, vertical=False, seed=4))
    arrow = p.m_poly([(ax - 26, ay - 58), (ax + 6, ay - 58), (ax + 6, ay - 66), (ax + 30, ay - 54), (ax + 6, ay - 42), (ax + 6, ay - 50),
                      (ax - 26, ay - 50)])
    p.paint(arrow, TEAM_RED, form=0.3, rim=0.8, glint=1.0, lw=1.6)
    p.rivets([(ax - 38, ay - 66), (ax + 38, ay - 66), (ax - 38, ay - 42), (ax + 38, ay - 42)], r=1.4)
    if theme == "snow":
        p.paint(p.m_blob(ax, ay - 72, 44, 5, power=2.4), rgba("ffffff"), lw=1.4, form=0.4, rim=0.6, glint=0)
    return Sprite(finish(p), ax, ay)


def make_parasol():
    p = prop(130, 132)
    ax, ay = 65, 120
    towel = p.m_poly([(ax - 46, ay - 4), (ax + 34, ay - 10), (ax + 44, ay + 6), (ax - 36, ay + 10)])

    def stripes(q):
        for i in range(-3, 4):
            q.line([(ax + i * 14, ay - 14), (ax + i * 14 + 6, ay + 14)], rgba("ffffff"), 4.0, cap=False)
    p.paint(towel, rgba("5fc8f0"), form=0.25, rim=0.8, glint=0, texture=stripes)
    p.paint(p.m_line([(ax, ay), (ax + 6, ay - 74)], 3.2), STEEL, form=0, rim=0.6, glint=0.6, lw=1.6)
    dome = p.m_poly([(ax + 6 + 58 * math.cos(math.radians(a)), ay - 70 + 34 * math.sin(math.radians(a))) for a in range(180, 361, 6)] +
                    [(ax + 64, ay - 66), (ax - 52, ay - 66)])

    def red(q):
        for i in range(3):
            a0, a1 = 180 + i * 60, 210 + i * 60
            pts = [(ax + 6, ay - 70)] + [(ax + 6 + 70 * math.cos(math.radians(a)), ay - 70 + 44 * math.sin(math.radians(a)))
                                         for a in range(a0, a1 + 1, 3)]
            q.poly(pts, TEAM_RED)
    p.paint(dome, rgba("ffffff"), shade=rgba("d9dbe8"), form=0.3, rim=1.2, glint=1.4, texture=red)
    p.dot(ax + 6, ay - 105, 3, OUTLINE)
    p.dot(ax + 6, ay - 105, 2, GOLD)
    return Sprite(finish(p), ax, ay)


# ---- 성문 ---------------------------------------------------------------------------------

def make_castle(theme):
    """성문: 원뿔 지붕 탑 둘 + 성문 집 (얼굴 있음). 발 위치 = 성문 좌표 (map gate)."""
    w, h = 250, 410
    ax, ay = 114, 372
    p = Painter(w, h, ss=PROP_SS * K, lw=2.6)
    stone = rgba("cfc8bd")
    stone_dark = rgba("b6aea3")

    def X(x):
        return ax + x

    def Y(y):
        return ay + y

    # 바닥 돌단
    plinth = p.m_rrect(X(-108), Y(14), X(130), Y(36), 6)
    p.paint(plinth, stone_dark, side=0.3, rim=1.4, glint=1.4, texture=bricks(X(-108), Y(14), X(130), Y(36), 22, 11, stone_dark, seed=9))

    for tx, flag_color, seed in ((-100, TEAM_RED, 1), (54, TEAM, 2)):
        x0, x1 = X(tx), X(tx + 66)
        cx = (x0 + x1) / 2
        body = p.m_rrect(x0, Y(-246), x1, Y(24), 4)
        p.paint(body, stone, side=0.32, rim=1.6, glint=1.6, texture=bricks(x0, Y(-246), x1, Y(24), 16, 10, stone, seed=seed))
        # 지붕 밑 처마가 벽에 드리운 그늘
        p.cast(body, [(x0, Y(-246)), (x1, Y(-246)), (x1, Y(-232)), (x0, Y(-226))], alpha=90)
        # 화살 창
        slit = p.m_rrect(cx - 6, Y(-200), cx + 6, Y(-166), 6)
        p.paint(slit, rgba("3a3340"), form=0, rim=0, glint=0, lw=1.8)
        p._put(rgba("ffd36a", 200), p.m_rrect(cx - 2.5, Y(-190), cx + 2.5, Y(-172), 2.5))
        # 깃발 천 (탑 앞에 걸린 휘장)
        banner = p.m_poly([(cx - 14, Y(-150)), (cx + 14, Y(-150)), (cx + 14, Y(-96)), (cx, Y(-106)), (cx - 14, Y(-96))])
        p.paint(p.m_rrect(cx - 19, Y(-154), cx + 19, Y(-148), 3), WOOD_DARK, form=0, rim=0.6, glint=0.6, lw=1.6)
        p.paint(banner, flag_color, form=0.35, rim=1.2, glint=1.2, lw=2.0)
        star = [(cx + math.cos(-math.pi / 2 + k * math.pi / 5) * (7 if k % 2 == 0 else 3), Y(-128) + math.sin(-math.pi / 2 + k * math.pi / 5) * (7 if k % 2 == 0 else 3))
                for k in range(10)]
        p.paint(p.m_poly(star), GOLD, form=0.3, rim=0.5, glint=0.6, lw=1.2)
        # 원뿔 지붕: 처마가 벽보다 넓게, 기와
        roof = p.union(p.m_poly([(x0 - 9, Y(-244)), (cx, Y(-318)), (x1 + 9, Y(-244))]), p.m_ellipse(cx, Y(-244), (x1 - x0) / 2 + 9, 6))
        p.paint(roof, TEAM, form=0, side=0.4, rim=1.6, glint=1.6,
                texture=roof_tiles(x0 - 9, Y(-318), x1 + 9, Y(-238), 11, 7, TEAM, seed=seed))
        if theme == "snow":
            cap = p.m_poly([(cx, Y(-320)), (cx + 17, Y(-288)), (cx + 8, Y(-284)), (cx, Y(-290)), (cx - 10, Y(-282)), (cx - 19, Y(-286))])
            p.paint(cap, rgba("ffffff"), lw=1.6, form=0.35, rim=0.8, glint=0)
        flag(p, cx, Y(-316), 34, flag_color, wave=2.0 if seed == 1 else -2.0)

    # 성문 집
    gx0, gx1 = X(-44), X(66)
    gate_body = p.m_rrect(gx0, Y(-172), gx1, Y(26), 3)
    p.paint(gate_body, stone, side=0.28, rim=1.6, glint=1.6, texture=bricks(gx0, Y(-172), gx1, Y(26), 18, 11, stone, seed=5))
    for i, bx in enumerate((-48, -20, 8, 36)):
        merlon = p.m_rrect(X(bx), Y(-194), X(bx + 22), Y(-168), 3)
        p.paint(merlon, stone, side=0.3, rim=1.2, glint=1.2, texture=bricks(X(bx), Y(-194), X(bx + 22), Y(-168), 12, 9, stone, seed=11 + i))
        if theme == "snow":
            p.paint(p.m_blob(X(bx + 11), Y(-194), 12, 4, power=2.4), rgba("ffffff"), lw=1.4, form=0.35, rim=0.6, glint=0)
    # 문 둘레 아치 돌
    door_cx = X(11)
    arch_outer = p.union(p.m_ellipse(door_cx, Y(-50), 48, 62), p.m_rrect(door_cx - 48, Y(-50), door_cx + 48, Y(26), 0))
    arch_outer = p.both(arch_outer, p.m_rrect(door_cx - 48, Y(-112), door_cx + 48, Y(26), 0))

    def voussoirs(q):
        for k in range(11):
            a = math.radians(180 + k * 18)
            q.line([(door_cx + math.cos(a) * 30, Y(-50) + math.sin(a) * 44), (door_cx + math.cos(a) * 52, Y(-50) + math.sin(a) * 66)],
                   tone(stone_dark, light=-0.4), 1.4, cap=False)
    p.paint(arch_outer, stone_dark, form=0.2, rim=1.4, glint=1.6, texture=voussoirs)
    door = p.union(p.m_ellipse(door_cx, Y(-50), 36, 50), p.m_rrect(door_cx - 36, Y(-50), door_cx + 36, Y(26), 0))
    door = p.both(door, p.m_rrect(door_cx - 36, Y(-102), door_cx + 36, Y(26), 0))

    def door_wood(q):
        wood_planks(door_cx - 36, Y(-102), door_cx + 36, Y(26), 12, WOOD_DARK, seed=6)(q)
        for by in (-62, -18, 12):
            q.poly([(door_cx - 36, Y(by)), (door_cx + 36, Y(by)), (door_cx + 36, Y(by + 7)), (door_cx - 36, Y(by + 7))], rgba("5a5560"))
            q.line([(door_cx - 36, Y(by + 1)), (door_cx + 36, Y(by + 1))], rgba("9a96a2"), 1.0, cap=False)
            q.rivets([(door_cx - 26 + i * 13, Y(by + 3.5)) for i in range(5)], r=1.2)
        q.line([(door_cx, Y(-102)), (door_cx, Y(26))], tone(WOOD_DARK, light=-0.55), 1.6, cap=False)
    p.paint(door, WOOD_DARK, form=0, side=0.3, rim=1.2, glint=0, texture=door_wood)
    p.cast(door, [(door_cx - 40, Y(-110)), (door_cx + 40, Y(-110)), (door_cx + 40, Y(-60)), (door_cx - 40, Y(-84))], alpha=80)
    # 문고리
    for dx in (-8, 8):
        p.arc(door_cx + dx, Y(-28), 4, 4, 0, 360, GOLD, 1.6)
    # 얼굴 (냥코풍 점 눈)
    p.dot(door_cx - 12, Y(-132), 4.2, OUTLINE)
    p.dot(door_cx + 12, Y(-132), 4.2, OUTLINE)
    p.dot(door_cx - 13, Y(-133.5), 1.3, (255, 255, 255, 230))
    p.dot(door_cx + 11, Y(-133.5), 1.3, (255, 255, 255, 230))
    p.line([(door_cx - 5, Y(-118)), (door_cx + 5, Y(-118))], OUTLINE, 2.2)
    p.dot_e(door_cx - 22, Y(-122), 5, 3, rgba("ff8f8f", 130))
    p.dot_e(door_cx + 22, Y(-122), 5, 3, rgba("ff8f8f", 130))
    # 벽 밑 풀·이끼
    if theme == "meadow":
        for x in (X(-104), X(-50), X(70), X(124)):
            grass_tuft(p, x, Y(16), 1.4, int(x))
    return Sprite(finish(p), ax, ay)


# ---- 지도 ---------------------------------------------------------------------------------

def make_map(m, path):
    """지도 한 장 (2560x1440)."""
    theme = m["theme"]
    t = THEMES[theme]
    rng = random.Random(7 + len(path))
    img = paint_ground(t, 11 + len(path))

    sea_y = 118 if theme == "beach" else 0
    px, py, prx, pry = t["pond"]

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
        if abs(x - px) < prx + 18 and abs(y - py) < pry + 16:
            return False
        return True

    # 길·물·풀잎: 2배 캔버스
    p = Painter(map_data.VIEW_W, map_data.VIEW_H, ss=GROUND_SS * K, lw=BG_LW)
    ground_marks(p, theme, t, rng, free)
    if theme == "beach":
        paint_sea(p, t, sea_y, rng)
    paint_pond(p, theme, t, rng)
    paint_road(p, theme, t, path, rng)
    layer = p.img.convert("RGBa").resize((OUT_W, OUT_H), Image.LANCZOS).convert("RGBA")
    img.alpha_composite(layer)

    # 소품 자리 정하기
    props = []   # (y, sprite, x, 그림자 반폭)
    flowers = [make_flower(theme, s) for s in range(10)]
    for _ in range(35 if theme == "snow" else 80):
        x, y = rng.uniform(20, 1900), rng.uniform(20, 1060)
        if free(x, y, 14):
            props.append((y, rng.choice(flowers), x, 0))
    tufts = [make_tuft(theme, s) for s in range(8)]
    for _ in range(40):
        x, y = rng.uniform(20, 1900), rng.uniform(20, 1060)
        if free(x, y, 16):
            props.append((y, rng.choice(tufts), x, 0))
    for _ in range(26):
        x, y = rng.uniform(30, 1890), rng.uniform(40, 1050)
        if free(x, y, 30):
            r = rng.uniform(15, 26)
            props.append((y, make_rock(r, rng.randrange(1 << 20), theme, rng.random() < 0.35), x, r * 1.1))

    if theme != "beach":
        pool = [make_edge_tuft(theme, t, s) for s in range(8)]
        for x, y in edge_tuft_spots(path, rng):
            if theme == "snow" and rng.random() < 0.65:
                continue
            props.append((y, rng.choice(pool), x, 0))
    if theme == "beach":
        for ux, uy in ((1250, 1010), (150, 1010)):
            if free(ux, uy, 20):
                props.append((uy, make_parasol(), ux, 54))

    # 나무: 잎이 길을 덮지 않게, HUD 자리는 비운다
    trees = []
    attempts = 0
    while len(trees) < (48 if theme == "beach" else 64) and attempts < 4000:
        attempts += 1
        x, y = rng.uniform(-10, 1930), rng.uniform(60, 1075)
        if not free(x, y, 52):
            continue
        half = map_data.PATH_WIDTH * 0.5
        if map_data.dist_to_path((x, y - 60), path) < half + 40 or map_data.dist_to_path((x, y - 110), path) < half + 30:
            continue
        if y - 110 < sea_y:
            continue
        if any(abs(x - tx) < 46 and abs(y - ty) < 30 for tx, ty, _, _ in trees):
            continue
        if (y < 215 and x < 760) or (y > 900 and 560 < x < 1360) or (y < 150 and x > 1640):
            continue
        trees.append((x, y, rng.uniform(0.85, 1.25), rng.random()))
    for x, y, s, kind in trees:
        seed = rng.randrange(1 << 20)
        if theme == "beach":
            if kind < 0.75:
                sprite = make_palm(s, seed, rng.choice(t["leaves"]), 18 if kind < 0.38 else -18)
            else:
                sprite = make_bush(s, seed, rng.choice(t["leaves"]), "ff7aa8")
        elif theme == "snow":
            sprite = make_pine(s, seed, rng.choice(t["leaves"]), snowy=True, face=kind > 0.94)
        else:
            if kind < 0.62:
                sprite = make_round_tree(s, seed, rng.choice(t["leaves"]), face=kind > 0.58, fruit=kind < 0.12)
            elif kind < 0.82:
                sprite = make_pine(s, seed, t["pine"], face=kind > 0.8)
            else:
                sprite = make_bush(s * 1.1, seed, rng.choice(t["leaves"]), "ff5a6a" if kind < 0.9 else None)
        props.append((y, sprite, x, 30 * s))

    sx, sy = m["sign"]
    props.append((sy + 40, make_sign(theme), sx, 26))
    gx, gy = m["gate"]
    props.append((gy + 30, make_castle(theme), gx, 0))

    # 그림자를 한 번에 (부드럽게 번진 타원)
    shadow = Image.new("L", (OUT_W, OUT_H), 0)
    d = ImageDraw.Draw(shadow)
    for y, sprite, x, rx in props:
        if rx > 0:
            d.ellipse([(x - rx + 6) * K, (y - rx * 0.26 + 2) * K, (x + rx + 10) * K, (y + rx * 0.26 + 4) * K], fill=t["shadow_alpha"])
    d.ellipse([(gx - 140) * K, (gy + 16) * K, (gx + 150) * K, (gy + 56) * K], fill=t["shadow_alpha"])
    shadow = shadow.filter(ImageFilter.GaussianBlur(5 * K))
    img.paste(Image.new("RGBA", (OUT_W, OUT_H), SHADOW + (255,)), (0, 0), shadow)

    props.sort(key=lambda item: item[0])
    for y, sprite, x, _ in props:
        sprite.place(img, x, y)
    return img
