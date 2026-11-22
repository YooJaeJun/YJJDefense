"""YJJDefense 아트 생성기.

방향: 냥코 대전쟁처럼 귀엽지만 엉뚱한 얼굴 + 킹덤러쉬급 색·명암·재질 (paint.py, units_hd.py로 옮겨 가는 중).
모든 그림을 코드로 그린다 (4배 크게 그린 뒤 줄여서 매끈한 선). 외부 이미지 없음.

실행 (Tools/Art/BuildArt.ps1이 대신 해 준다):
    Tools/Art/.venv/Scripts/python.exe Tools/Art/generate_art.py <출력 폴더> <맵 inl 경로>

스프라이트는 화면 표시 크기의 2배로 만든다 (1080p보다 큰 화면에서도 선명하게).
"""

import math
import os
import sys

from PIL import Image, ImageDraw, ImageFilter

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import map_data  # noqa: E402
# 명암·재질을 넣어 다시 그린 것 (점점 늘려 간다)
from units_hd import make_imp, make_soldier, make_tower_archer, make_tower_barracks  # noqa: E402
from fx_hd import (make_arrow, make_ball, make_bomb, make_build_spot, make_burst, make_flame_fx, make_ice_block,  # noqa: E402
                   make_magic_bolt, make_puff, make_star_fx)
from maps_hd import make_map  # noqa: E402
from ui_hd import make_disk, make_icon, make_panel, make_panel_flat, make_rune, make_socket  # noqa: E402
from anim_hd import build_anims  # noqa: E402
from towers_hd import make_tower_bombard, make_tower_mage, make_tower_shrine, make_wizard  # noqa: E402
from monsters_hd import (make_chicken, make_coco_kid, make_coconut, make_crab, make_jellyfish, make_king_crab,  # noqa: E402
                         make_mushroom, make_penguin, make_potato, make_pudding_king, make_snowball, make_snowman,
                         make_toast, make_yeti)

SS = 4
INK = (36, 30, 40, 255)
WHITE = (255, 255, 255, 255)


def rgba(value, alpha=255):
    value = value.lstrip("#")
    return (int(value[0:2], 16), int(value[2:4], 16), int(value[4:6], 16), alpha)


YELLOW = rgba("ffd84a")


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
# 캐릭터·타워(units_hd, towers_hd, monsters_hd), 지도(maps_hd), 건설 자리·투사체·이펙트(fx_hd), 룬·UI(ui_hd)는 명암 톤, 자세히 움직이는 동작은 anim_hd. 여기는 링·빛 같은 기본 도형

# ---------------------------------------------------------------------------------------------
# 링·빛·그림자 (흰색으로 만들고 게임에서 색을 입힌다)

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

    # 자세히 움직이는 동작 (Content/YJJDefense/Data/Anims.csv)
    sprites.update(build_anims())
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
