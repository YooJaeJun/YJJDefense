"""맵 배치의 원본. 좌표는 1920x1080 가상 화면 (y는 아래로).

generate_art.py가 이 값으로 배경 그림(T_Map01, T_Map02...)을 그리고,
같은 값으로 Source/YJJDefense/Game/YJJMaps.inl을 써서 게임 규칙과 그림이 어긋나지 않게 한다.

화면에서 비워 둘 곳: 왼쪽 위 정보(x<720, y<80), 오른쪽 위 버튼(x>1760, y<80),
아래 스펠 바(430<x<1490, y>905), 오른쪽 아래 웨이브 버튼(x>1690, y>945).
"""

import math

VIEW_W = 1920
VIEW_H = 1080
PATH_WIDTH = 86.0

MAPS = [
    dict(
        key="Map01",
        theme="meadow",
        # 왼쪽 밖에서 들어와 S자로 세 번 꺾고 오른쪽 성문으로
        path=[
            (-80, 270), (160, 272), (480, 258), (800, 282), (1100, 248), (1380, 262),
            (1545, 330), (1560, 450), (1450, 545), (1220, 572), (900, 548), (620, 572),
            (400, 610), (300, 715), (360, 825), (560, 868), (860, 846), (1160, 868),
            (1440, 838), (1640, 806), (1760, 796), (1832, 794),
        ],
        spots=[
            (170, 432), (760, 165), (1180, 150),
            (620, 430), (980, 420), (1330, 425),
            (1725, 420), (165, 660),
            (640, 716), (1010, 706), (1385, 696),
        ],
        wizard=(1610, 700),
        gate=(1800, 800),
        sign=(70, 170),
    ),
    dict(
        key="Map02",
        theme="beach",
        # 왼쪽 아래에서 들어와 위아래로 네 번 굽이치는 모래길. 위쪽은 바다
        path=[
            (-80, 700), (150, 705), (330, 680), (430, 590), (450, 450), (470, 320), (570, 250),
            (720, 250), (820, 320), (850, 450), (860, 590), (940, 690), (1080, 725), (1220, 685),
            (1290, 580), (1300, 450), (1320, 330), (1420, 255), (1560, 255), (1650, 330),
            (1680, 460), (1700, 600), (1750, 720), (1832, 794),
        ],
        spots=[
            (240, 300), (220, 540), (240, 860), (650, 420), (650, 600),
            (1075, 380), (1075, 560),
            (1490, 430), (1490, 600), (1850, 470),
        ],
        wizard=(1540, 830),
        gate=(1800, 800),
        sign=(90, 590),
    ),
    dict(
        key="Map03",
        theme="snow",
        # 왼쪽 아래 골짜기에서 올라와 산비탈을 네 번 오르내린다
        path=[
            (300, 1160), (300, 960), (290, 800), (300, 620), (290, 440), (310, 290), (380, 180),
            (510, 140), (640, 180), (710, 290), (720, 440), (720, 600), (760, 720), (900, 790),
            (1050, 745), (1110, 620), (1120, 460), (1130, 300), (1190, 190), (1320, 140),
            (1450, 190), (1510, 300), (1520, 450), (1530, 600), (1590, 720), (1700, 785), (1832, 794),
        ],
        spots=[
            (120, 470), (510, 350), (510, 580), (510, 790),
            (920, 225), (920, 400), (920, 585),
            (1320, 350), (1320, 580), (1420, 800), (1720, 330),
        ],
        wizard=(1660, 600),
        gate=(1800, 800),
        sign=(170, 1000),
    ),
]


def _catmull(p0, p1, p2, p3, t):
    t2, t3 = t * t, t * t * t
    return tuple(
        0.5 * ((2 * p1[i]) + (-p0[i] + p2[i]) * t + (2 * p0[i] - 5 * p1[i] + 4 * p2[i] - p3[i]) * t2 + (-p0[i] + 3 * p1[i] - 3 * p2[i] + p3[i]) * t3)
        for i in range(2)
    )


def smooth_path(control, step=8.0):
    """조절점을 지나는 부드러운 곡선을 step 간격의 점으로."""
    pts = control
    dense = []
    for i in range(len(pts) - 1):
        p0 = pts[max(i - 1, 0)]
        p1, p2 = pts[i], pts[i + 1]
        p3 = pts[min(i + 2, len(pts) - 1)]
        for k in range(32):
            dense.append(_catmull(p0, p1, p2, p3, k / 32.0))
    dense.append(pts[-1])

    # 같은 간격으로 다시 찍는다
    out = [dense[0]]
    carry = 0.0
    for a, b in zip(dense, dense[1:]):
        seg = math.dist(a, b)
        if seg <= 1e-6:
            continue
        d = step - carry
        while d <= seg:
            t = d / seg
            out.append((a[0] + (b[0] - a[0]) * t, a[1] + (b[1] - a[1]) * t))
            d += step
        carry = (carry + seg) % step
    if math.dist(out[-1], dense[-1]) > 1.0:
        out.append(dense[-1])
    return out


def dist_to_path(point, path):
    best = 1e9
    for a, b in zip(path, path[1:]):
        abx, aby = b[0] - a[0], b[1] - a[1]
        length_sq = abx * abx + aby * aby or 1e-9
        t = max(0.0, min(1.0, ((point[0] - a[0]) * abx + (point[1] - a[1]) * aby) / length_sq))
        best = min(best, math.dist(point, (a[0] + abx * t, a[1] + aby * t)))
    return best


def check(m, path):
    """배치 검사: 자리·마법사가 길과 겹치지 않는지, 길이 스스로 너무 가깝게 지나가지 않는지."""
    problems = []
    half = PATH_WIDTH * 0.5
    for i, s in enumerate(m["spots"]):
        d = dist_to_path(s, path)
        if d < half + 50:
            problems.append("spot %d %s too close to path (%.0f)" % (i, s, d))
        if d > 230:
            problems.append("spot %d %s too far from path (%.0f)" % (i, s, d))
        for j, o in enumerate(m["spots"][:i]):
            if abs(s[0] - o[0]) < 130 and abs(s[1] - o[1]) < 110:
                problems.append("spot %d overlaps spot %d" % (i, j))
    for zone in ((0, 0, 720, 80), (1760, 0, 1920, 80), (430, 905, 1490, 1080), (1690, 945, 1920, 1080)):
        for i, s in enumerate(m["spots"]):
            if zone[0] - 60 < s[0] < zone[2] + 60 and zone[1] - 40 < s[1] < zone[3] + 30:
                problems.append("spot %d %s under UI %s" % (i, s, zone))
        for p in path[::6]:
            if zone[0] < p[0] < zone[2] and zone[1] < p[1] < zone[3] and 0 < p[0] < VIEW_W and 0 < p[1] < VIEW_H:
                problems.append("path %s under UI %s" % ((int(p[0]), int(p[1])), zone))
                break
    w = m["wizard"]
    if dist_to_path((w[0], w[1] - 60), path) < half + 50:
        problems.append("wizard too close to path")
    # 길끼리: 길 위 거리로 400 이상 떨어진 두 점이 화면에서 200보다 가까우면 자리 놓을 틈이 없다
    step = 8.0
    for i in range(0, len(path), 6):
        for j in range(i + int(500 / step), len(path), 6):
            if math.dist(path[i], path[j]) < 190:
                problems.append("path folds too close near %s / %s" % ((int(path[i][0]), int(path[i][1])), (int(path[j][0]), int(path[j][1]))))
                return problems
    return problems
