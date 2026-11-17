"""맵 01 배치의 원본. 좌표는 1920x1080 가상 화면 (y는 아래로).

generate_art.py가 이 값으로 배경 그림(T_Map01)을 그리고,
같은 값으로 Source/YJJDefense/Game/YJJMap01.inl을 써서 게임 규칙과 그림이 어긋나지 않게 한다.
"""

import math

VIEW_W = 1920
VIEW_H = 1080

# 길의 조절점. 왼쪽 밖에서 들어와 S자로 세 번 꺾고 오른쪽 성문으로 나간다
PATH_CONTROL = [
    (-80, 270), (160, 272), (480, 258), (800, 282), (1100, 248), (1380, 262),
    (1545, 330), (1560, 450), (1450, 545), (1220, 572), (900, 548), (620, 572),
    (400, 610), (300, 715), (360, 825), (560, 868), (860, 846), (1160, 868),
    (1440, 838), (1640, 806), (1760, 796), (1832, 794),
]

# 병졸을 세울 수 있는 자리
SPOTS = [
    (170, 432), (760, 165), (1180, 150),
    (620, 430), (980, 420), (1330, 425),
    (1725, 420), (165, 660),
    (640, 716), (1010, 706), (1385, 696),
]

# 마법사(플레이어)가 서 있는 기둥의 바닥 중심, 성문의 바닥 중심
WIZARD = (1610, 700)
GATE = (1800, 800)

PATH_WIDTH = 86.0


def _catmull(p0, p1, p2, p3, t):
    t2, t3 = t * t, t * t * t
    return tuple(
        0.5 * ((2 * p1[i]) + (-p0[i] + p2[i]) * t + (2 * p0[i] - 5 * p1[i] + 4 * p2[i] - p3[i]) * t2 + (-p0[i] + 3 * p1[i] - 3 * p2[i] + p3[i]) * t3)
        for i in range(2)
    )


def smooth_path(step=8.0):
    """조절점을 지나는 부드러운 곡선을 step 간격의 점으로."""
    pts = PATH_CONTROL
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
