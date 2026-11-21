"""명암·외곽선·재질을 입히는 그리기 도구 (generate_art.py가 쓴다).

평면 도형 + 검은 선 대신, 도형 하나(마스크)마다
  1) 굵은 고동색 외곽선 (도형을 부풀려서 깐다 → 겹친 부품마다 또렷한 테두리)
  2) 바탕색
  3) 셀 명암: 빛은 왼쪽 위. 오른쪽 아래에 큰 그림자 면 + 가장자리 어두운 띠
  4) 왼쪽 위 가장자리에 밝은 띠 (+ 원하면 반짝 점)
  5) 재질 무늬 (나뭇결, 벽돌, 기와, 금속 리벳…)는 마스크 안에만 그린다
순서로 칠한다. 그림자는 붉은 쪽, 빛은 노란 쪽으로 색상을 살짝 돌려서 따뜻한 느낌을 낸다.

좌표는 완성 크기 기준 픽셀, 실제로는 SS배로 크게 그린 뒤 줄인다.
"""

import colorsys
import math

from PIL import Image, ImageChops, ImageDraw, ImageFilter

OUTLINE = (46, 28, 22, 255)   # 먹선: 검정 대신 아주 진한 고동


def rgba(value, alpha=255):
    value = value.lstrip("#")
    return (int(value[0:2], 16), int(value[2:4], 16), int(value[4:6], 16), alpha)


def tone(color, light=0.0, sat=0.0, hue=0.0):
    """밝기·채도·색상 조절. light -1(검정)~+1(흰색), hue는 도(°)."""
    r, g, b = (c / 255.0 for c in color[:3])
    h, l, s = colorsys.rgb_to_hls(r, g, b)
    h = (h + hue / 360.0) % 1.0
    l = l + (1.0 - l) * light if light > 0 else l * (1.0 + light)
    s = min(max(s + sat, 0.0), 1.0)
    r, g, b = colorsys.hls_to_rgb(h, min(max(l, 0.0), 1.0), s)
    return (int(r * 255 + 0.5), int(g * 255 + 0.5), int(b * 255 + 0.5), color[3] if len(color) > 3 else 255)


def shade_of(color):
    """그림자 색: 어둡게, 채도 올리고, 붉은 보라 쪽으로. 회색 계열은 따뜻한 회갈색으로."""
    r, g, b = color[:3]
    if max(r, g, b) - min(r, g, b) < 40:
        return tone((r, g, b, 255), light=-0.32, sat=-0.02, hue=0.0)
    hue_shift = 10.0 if (r >= g and r >= b) else (-12.0 if b >= r else 12.0)
    return tone(color, light=-0.36, sat=0.1, hue=hue_shift * -1 if b >= r else hue_shift * -0.5)


def deep_of(color):
    return tone(shade_of(color), light=-0.3, sat=0.05)


def light_of(color):
    """빛 받는 색: 밝게, 노란 쪽으로."""
    return tone(color, light=0.38, sat=0.02, hue=4.0 if color[2] > color[0] else -4.0)


class Painter:
    """명암 있는 스프라이트 캔버스."""

    def __init__(self, w, h, ss=4, lw=3.2):
        self.w, self.h, self.ss, self.lw = w, h, ss, lw
        self.img = Image.new("RGBA", (int(w * ss), int(h * ss)), (0, 0, 0, 0))
        self.light_dir = (-0.6, -0.8)   # 빛이 오는 쪽 (왼쪽 위)

    # ---- 마스크 만들기 (완성 크기 좌표) ----------------------------------------------------

    def _blank(self):
        return Image.new("L", self.img.size, 0)

    def _p(self, pts):
        return [(x * self.ss, y * self.ss) for x, y in pts]

    def m_poly(self, pts):
        m = self._blank()
        ImageDraw.Draw(m).polygon(self._p(pts), fill=255)
        return m

    def m_ellipse(self, cx, cy, rx, ry):
        m = self._blank()
        s = self.ss
        ImageDraw.Draw(m).ellipse([(cx - rx) * s, (cy - ry) * s, (cx + rx) * s, (cy + ry) * s], fill=255)
        return m

    def m_rrect(self, x0, y0, x1, y1, r):
        m = self._blank()
        s = self.ss
        ImageDraw.Draw(m).rounded_rectangle([x0 * s, y0 * s, x1 * s, y1 * s], radius=r * s, fill=255)
        return m

    def m_line(self, pts, width):
        m = self._blank()
        d = ImageDraw.Draw(m)
        p = self._p(pts)
        w = max(int(width * self.ss), 1)
        d.line(p, fill=255, width=w, joint="curve")
        for x, y in (p[0], p[-1]):
            d.ellipse([x - w / 2, y - w / 2, x + w / 2, y + w / 2], fill=255)
        return m

    def m_blob(self, cx, cy, rx, ry, angle=0.0, power=2.0, n=64, wobble=0.0, seed=0):
        """기울인 (각진) 타원. wobble이 있으면 울퉁불퉁하게."""
        ca, sa = math.cos(math.radians(angle)), math.sin(math.radians(angle))
        pts = []
        for i in range(n):
            t = 2 * math.pi * i / n
            c, s = math.cos(t), math.sin(t)
            k = 1.0 + wobble * math.sin(t * 5 + seed) * 0.5 + wobble * math.sin(t * 3 + seed * 2.1) * 0.5
            x = rx * k * math.copysign(abs(c) ** (2.0 / power), c)
            y = ry * k * math.copysign(abs(s) ** (2.0 / power), s)
            pts.append((cx + x * ca - y * sa, cy + x * sa + y * ca))
        return self.m_poly(pts)

    @staticmethod
    def union(*masks):
        out = masks[0]
        for m in masks[1:]:
            out = ImageChops.lighter(out, m)
        return out

    @staticmethod
    def cut(mask, hole):
        return ImageChops.subtract(mask, hole)

    @staticmethod
    def both(a, b):
        return ImageChops.multiply(a, b)

    def _shift(self, mask, dx, dy):
        out = Image.new("L", mask.size, 0)
        out.paste(mask, (int(round(dx * self.ss)), int(round(dy * self.ss))))
        return out

    def _grow(self, mask, px):
        """px만큼 부풀린 마스크 (가우스 번짐 + 문턱값: 둥근 외곽선)."""
        if px <= 0:
            return mask
        radius = px * self.ss * 0.5
        blurred = mask.filter(ImageFilter.GaussianBlur(radius))
        return blurred.point(lambda v: 255 if v > 6 else int(v * 42))

    # ---- 칠하기 -----------------------------------------------------------------------

    def _put(self, color, mask):
        layer = Image.new("RGBA", self.img.size, color)
        if len(color) > 3 and color[3] < 255:
            mask = mask.point(lambda v, a=color[3]: v * a // 255)
        self.img.paste(layer, (0, 0), mask)

    def paint(self, mask, base, shade=None, light=None, outline=OUTLINE, lw=None,
              form=0.3, rim=1.6, glint=2.2, soft=0.35, texture=None, side=None):
        """마스크 하나를 명암 넣어 칠한다.

        form  : 큰 그림자 면 크기 (도형 크기 대비 비율, 0이면 없음)
        rim   : 오른쪽 아래 가장자리 어두운 띠 두께(px)
        glint : 왼쪽 위 밝은 띠 두께(px)
        texture(painter, mask): 바탕 위에 무늬를 그리는 함수 (마스크 밖은 잘린다)
        side  : 건물 벽처럼 오른쪽 면만 그늘지게 할 때 그늘 너비 비율 (0~1). 주면 form 대신 쓴다
        """
        bbox = mask.getbbox()
        if not bbox:
            return
        lw = self.lw if lw is None else lw
        shade = shade_of(base) if shade is None else shade
        light = light_of(base) if light is None else light
        lx, ly = self.light_dir

        if outline is not None and lw > 0:
            self._put(outline, self._grow(mask, lw))
        self._put(base, mask)

        if texture is not None:
            self.clip(mask, texture)

        if side:
            x0, y0, x1, y1 = bbox
            band = Image.new("L", mask.size, 0)
            ImageDraw.Draw(band).rectangle([x1 - (x1 - x0) * side, y0, x1, y1], fill=255)
            self._put(shade, self.both(band, mask))
        elif form > 0:
            size = max(bbox[2] - bbox[0], bbox[3] - bbox[1]) / self.ss
            d = size * form
            moved = self._shift(mask, lx * d, ly * d)          # 빛 쪽으로 옮긴 마스크
            region = ImageChops.subtract(mask, moved)          # 그 밖으로 남는 곳 = 빛 반대편
            if soft > 0:
                region = self.both(region.filter(ImageFilter.GaussianBlur(soft * self.ss)), mask)
            self._put(shade, region)
        if rim > 0:
            moved = self._shift(mask, lx * rim, ly * rim)
            self._put(deep_of(base) if shade is None else tone(shade, light=-0.25), ImageChops.subtract(mask, moved))
        if glint > 0:
            moved = self._shift(mask, -lx * glint, -ly * glint)
            band = ImageChops.subtract(mask, moved)
            # 빛 띠는 위·왼쪽 절반에서만 (아래까지 두르면 플라스틱처럼 보인다)
            self._put(light, self.both(band, self._half(mask, bbox)))

    def _half(self, mask, bbox):
        """도형의 왼쪽 위 절반 쪽 영역."""
        m = Image.new("L", mask.size, 0)
        x0, y0, x1, y1 = bbox
        cx, cy = (x0 + x1) / 2, (y0 + y1) / 2
        big = max(x1 - x0, y1 - y0) * 2
        ImageDraw.Draw(m).polygon([(cx + big, cy - big), (cx - big, cy + big), (cx - big * 2, cy - big * 2)], fill=255)
        return m

    def cast(self, mask, pts, alpha=110):
        """위 물체가 드리운 그림자를 mask(벽 등) 안에만."""
        shadow = self.both(self.m_poly(pts), mask)
        self._put((40, 18, 30, alpha), shadow)

    def clip(self, mask, draw_fn):
        """mask 안에만 그리기. draw_fn(layer_painter)는 이 Painter의 그리기 함수들을 쓴다."""
        saved = self.img
        self.img = Image.new("RGBA", saved.size, (0, 0, 0, 0))
        draw_fn(self)
        layer = self.img
        self.img = saved
        alpha = self.both(layer.getchannel("A"), mask)
        layer.putalpha(alpha)
        self.img.alpha_composite(layer)

    # ---- 단순 그리기 (명암 없이) -------------------------------------------------------

    def line(self, pts, color=OUTLINE, width=2.0, cap=True):
        d = ImageDraw.Draw(self.img)
        p = self._p(pts)
        w = max(int(width * self.ss), 1)
        d.line(p, fill=color, width=w, joint="curve")
        if cap:
            for x, y in (p[0], p[-1]):
                d.ellipse([x - w / 2, y - w / 2, x + w / 2, y + w / 2], fill=color)

    def dot(self, cx, cy, r, color=OUTLINE):
        s = self.ss
        ImageDraw.Draw(self.img).ellipse([(cx - r) * s, (cy - r) * s, (cx + r) * s, (cy + r) * s], fill=color)

    def poly(self, pts, color):
        ImageDraw.Draw(self.img).polygon(self._p(pts), fill=color)

    def arc(self, cx, cy, rx, ry, a0, a1, color=OUTLINE, width=2.0):
        s = self.ss
        ImageDraw.Draw(self.img).arc([(cx - rx) * s, (cy - ry) * s, (cx + rx) * s, (cy + ry) * s], a0, a1,
                                     fill=color, width=max(int(width * s), 1))

    # ---- 자주 쓰는 부품 ------------------------------------------------------------------

    def ball(self, cx, cy, rx, ry, base, **kw):
        self.paint(self.m_ellipse(cx, cy, rx, ry), base, **kw)

    def shine(self, cx, cy, rx, ry, alpha=200):
        """반짝 점 (금속·눈알·구슬)."""
        self.dot_e(cx, cy, rx, ry, (255, 255, 255, alpha))

    def dot_e(self, cx, cy, rx, ry, color):
        s = self.ss
        ImageDraw.Draw(self.img).ellipse([(cx - rx) * s, (cy - ry) * s, (cx + rx) * s, (cy + ry) * s], fill=color)

    def rivets(self, pts, r=1.6, color=None):
        for x, y in pts:
            self.dot(x, y, r + 0.7, OUTLINE)
            self.dot(x, y, r, color or rgba("d8dde6"))
            self.dot(x - r * 0.35, y - r * 0.35, r * 0.4, (255, 255, 255, 230))

    def result(self):
        # 알파를 미리 곱한 상태로 줄여야 투명한 가장자리가 검게 번지지 않는다
        return self.img.convert("RGBa").resize((int(self.w), int(self.h)), Image.LANCZOS).convert("RGBA")


# ---- 재질 무늬 ----------------------------------------------------------------------------

def wood_planks(x0, y0, x1, y1, step, base, vertical=True, seed=0):
    """판자: 판마다 색을 살짝 다르게 + 틈 + 나뭇결."""
    import random
    rng = random.Random(seed)
    gap = tone(base, light=-0.55, sat=0.1)
    grain = tone(base, light=-0.22)

    def draw(p):
        n = int(((x1 - x0) if vertical else (y1 - y0)) / step) + 1
        for i in range(n):
            if vertical:
                a, b = x0 + i * step, x0 + (i + 1) * step
                p.poly([(a, y0), (b, y0), (b, y1), (a, y1)], tone(base, light=rng.uniform(-0.1, 0.08)))
                p.line([(a, y0), (a, y1)], gap, 1.3, cap=False)
                for _ in range(2):
                    gx = rng.uniform(a + 2, b - 2)
                    gy = rng.uniform(y0, y1 - 10)
                    p.line([(gx, gy), (gx + rng.uniform(-1, 1), gy + rng.uniform(6, 14))], grain, 0.8, cap=False)
            else:
                a, b = y0 + i * step, y0 + (i + 1) * step
                p.poly([(x0, a), (x1, a), (x1, b), (x0, b)], tone(base, light=rng.uniform(-0.1, 0.08)))
                p.line([(x0, a), (x1, a)], gap, 1.3, cap=False)
                for _ in range(2):
                    gy = rng.uniform(a + 2, b - 2)
                    gx = rng.uniform(x0, x1 - 12)
                    p.line([(gx, gy), (gx + rng.uniform(8, 18), gy + rng.uniform(-0.6, 0.6))], grain, 0.8, cap=False)
    return draw


def bricks(x0, y0, x1, y1, bw, bh, base, seed=0, mortar=None):
    """벽돌·돌: 줄마다 엇갈리고, 돌마다 밝기와 윗면 빛이 다르다."""
    import random
    rng = random.Random(seed)
    mortar = mortar or tone(base, light=-0.45, sat=0.05)

    def draw(p):
        p.poly([(x0, y0), (x1, y0), (x1, y1), (x0, y1)], mortar)
        row = 0
        y = y0
        while y < y1:
            x = x0 - (bw * 0.5 if row % 2 else 0)
            while x < x1:
                c = tone(base, light=rng.uniform(-0.12, 0.1))
                g = 0.9
                p.poly([(x + g, y + g), (x + bw - g, y + g), (x + bw - g, y + bh - g), (x + g, y + bh - g)], c)
                p.line([(x + g + 1, y + g + 0.6), (x + bw - g - 1, y + g + 0.6)], light_of(c), 1.0, cap=False)
                p.line([(x + g + 1, y + bh - g - 0.6), (x + bw - g - 1, y + bh - g - 0.6)], shade_of(c), 1.0, cap=False)
                x += bw
            y += bh
            row += 1
    return draw


def roof_tiles(x0, y0, x1, y1, tw, th, base, seed=0):
    """비늘 기와: 반원 줄을 엇갈려 쌓는다."""
    import random
    rng = random.Random(seed)

    def draw(p):
        p.poly([(x0, y0), (x1, y0), (x1, y1), (x0, y1)], shade_of(base))
        row = 0
        y = y0
        while y < y1 + th:
            x = x0 - (tw * 0.5 if row % 2 else 0)
            while x < x1 + tw:
                c = tone(base, light=rng.uniform(-0.08, 0.08))
                s = p.ss
                d = ImageDraw.Draw(p.img)
                d.chord([x * s, (y - th) * s, (x + tw) * s, (y + th) * s], 0, 180, fill=c)
                d.arc([x * s, (y - th) * s, (x + tw) * s, (y + th) * s], 0, 180, fill=tone(base, light=-0.5), width=max(int(1.1 * s), 1))
                d.arc([(x + 2) * s, (y - th + 2) * s, (x + tw - 2) * s, (y + th - 3) * s], 200, 250,
                      fill=light_of(c), width=max(int(1.0 * s), 1))
                x += tw
            y += th * 0.75
            row += 1
    return draw


def ground(p, cx, cy, rx, ry, seed=0):
    """건물 아래 흙바닥 + 자갈 + 풀포기."""
    import random
    rng = random.Random(seed)
    dirt = rgba("c79a5e")
    m = p.m_blob(cx, cy, rx, ry, power=2.0, wobble=0.06, seed=seed)
    def pebbles(q):
        for _ in range(int(rx * ry / 60)):
            a = rng.uniform(0, 6.283)
            r = rng.uniform(0.2, 0.95)
            x, y = cx + math.cos(a) * rx * r, cy + math.sin(a) * ry * r
            c = tone(rgba("b9b0a6"), light=rng.uniform(-0.15, 0.2))
            q.dot_e(x, y, rng.uniform(1.6, 3.2), rng.uniform(1.0, 2.0), OUTLINE)
            q.dot_e(x - 0.3, y - 0.3, rng.uniform(1.0, 2.4), rng.uniform(0.7, 1.4), c)
    p.paint(m, dirt, side=None, form=0.25, rim=1.2, glint=0, lw=2.4, texture=pebbles)
    for sx in (-1, 1):
        grass_tuft(p, cx + sx * rx * 0.92, cy + ry * 0.1, 1.0, seed + sx)


def grass_tuft(p, x, y, s=1.0, seed=0):
    import random
    rng = random.Random(seed)
    green = rgba("5fae3a")
    blades = []
    for i in range(5):
        bx = x + (i - 2) * 3.2 * s
        h = rng.uniform(8, 13) * s
        lean = rng.uniform(-3, 3) * s
        blades.append([(bx - 2 * s, y), (bx + lean, y - h), (bx + 2 * s, y)])
    m = p.union(*[p.m_poly(b) for b in blades])
    p.paint(m, green, form=0.3, glint=0.8, rim=0.8, lw=2.0)


def straw(x0, y0, x1, y1, base, seed=0):
    """초가: 아래로 흐르는 짚 줄."""
    import random
    rng = random.Random(seed)

    def draw(p):
        p.poly([(x0, y0), (x1, y0), (x1, y1), (x0, y1)], base)
        for _ in range(int((x1 - x0) * (y1 - y0) / 9)):
            x = rng.uniform(x0, x1)
            y = rng.uniform(y0, y1)
            c = tone(base, light=rng.uniform(-0.5, 0.35))
            p.line([(x, y), (x + rng.uniform(-1.5, 1.5), y + rng.uniform(6, 13))], c, 1.3, cap=False)
        # 짚단 층 (가로 띠마다 아래가 어둡다)
        y = y0 + 14
        while y < y1:
            p.line([(x0, y), (x1, y)], tone(base, light=-0.45), 1.6, cap=False)
            p.line([(x0, y + 1.6), (x1, y + 1.6)], tone(base, light=0.3), 1.0, cap=False)
            y += 16
    return draw
