# -*- coding: utf-8 -*-
"""webview.py —— 把 DWG（经 accoreconsole + PyArx 扫描）导出成单文件网页看图器。

    python webview.py <dwg 路径> [-o 输出.html]

网页端用 Canvas2D 直接画显示列表：块定义只存一份、块参照重复引用，
视口外图元跳过，文字只在放大到 5px 以上才画；平移缩放就是改一个矩阵，
不重新解析任何 XML/SVG。
"""
import json
import math
import os
import re
import sys

# 修改痕迹图层（变更批注/7月修改/变更圈出…）：上面横跨全图的长引线是噪音，不是图纸本体
_NOISE_LAYER = re.compile("批注|修改|圈出|云线|REVCLOUD|DEFPOINTS", re.I)
_MTCODE = re.compile(r"\\[A-Za-z][^;\\{}]*;")

HERE = os.path.dirname(os.path.abspath(__file__))
if HERE not in sys.path:
    sys.path.insert(0, HERE)
if r"G:\PyArx" not in sys.path:
    sys.path.insert(0, r"G:\PyArx")

import render as _render          # noqa: E402
from pyarx_vision import aci as _aci   # noqa: E402  完整 ACI 1..255 标准色表
import scan_load                  # noqa: E402

TEMPLATE = os.path.join(HERE, "webview_template.html")


def _mtext(s):
    """剥掉 MTEXT 的格式码（\\Fxcsg,hztxt|c134;\\W0.7;\\C2;…），只留可读文字。"""
    if not s:
        return ""
    s = str(s).replace("\\P", " ").replace("\\p", " ")
    for _ in range(5):
        s2 = _MTCODE.sub("", s).replace("{", "").replace("}", "")
        if s2 == s:
            break
        s = s2
    return s.strip()


def _aci_hex(i):
    """ACI 1..255 → #RRGGBB（标准 AutoCAD 色表，见 pyarx_vision/aci.py）。"""
    try:
        r, g, b = _aci.aci_to_rgb(int(i))
    except Exception:
        r, g, b = 242, 244, 246
    return '#%02X%02X%02X' % (int(r), int(g), int(b))


def _true_hex(v):
    """CAD 真彩色 → #RRGGBB。AcCmColor 的 color() 是 COLORREF 0x00BBGGRR（低字节 = 红）。"""
    r = v & 0xFF
    g = (v >> 8) & 0xFF
    b = (v >> 16) & 0xFF
    return '#%02X%02X%02X' % (r, g, b)


class _Pal(object):
    """颜色表。

    - 0..256 预置：1..255 是 ACI，256 是 ByLayer 占位，0 在网页里用 -1 表示 ByBlock
    - 真彩色按需追加到表尾（索引 > 256），网页只认索引，不需要知道来源
    """

    def __init__(self):
        self.hex = []
        self.idx = {}
        for i in range(257):
            h = _aci_hex(i) if 1 <= i <= 255 else '#E6E8EB'
            if h not in self.idx:
                self.idx[h] = i
            self.hex.append(h)

    def add(self, h):
        i = self.idx.get(h)
        if i is None:
            i = len(self.hex)
            self.hex.append(h)
            self.idx[h] = i
        return i

    def of(self, ci, layer=None, lcol=None):
        """把 CAD 色号解析成表内索引：0→-1(ByBlock)，256→图层色，>256→真彩色，其余 ACI。"""
        try:
            v = int(ci)
        except Exception:
            v = 256
        if v == 0:
            return -1
        if v == 256 or v < 0:
            lv = None
            if lcol and layer is not None:
                try:
                    lv = lcol.get(str(layer))
                except Exception:
                    lv = None
            if isinstance(lv, (list, tuple)) and len(lv) >= 3:
                # 真彩色图层：layer_col 里存的是 [r,g,b]，不是 ACI 索引
                return self.add('#%02X%02X%02X' % (int(lv[0]), int(lv[1]), int(lv[2])))
            try:
                v = int(lv) if lv is not None else 7
            except Exception:
                v = 7
            if v in (0, 256) or v < 0:
                v = 7
        if v > 256:
            return self.add(_true_hex(v))
        if v < 1:
            v = 7
        return self.add(_aci_hex(v))


def _pct(vals, f):
    if not vals:
        return 0.0
    i = min(len(vals) - 1, max(0, int(len(vals) * f)))
    return vals[i]


def _union(boxes):
    return [min(b[0] for b in boxes), min(b[1] for b in boxes),
            max(b[2] for b in boxes), max(b[3] for b in boxes)]


def _pad(b, f=0.02):
    w = max(1e-6, b[2] - b[0])
    h = max(1e-6, b[3] - b[1])
    return [b[0] - w * f, b[1] - h * f, b[2] + w * f, b[3] + h * f]


def _area(b):
    return max(1e-6, b[2] - b[0]) * max(1e-6, b[3] - b[1])


def _cell(b, allb, w, h, g=16):
    cx, cy = (b[0] + b[2]) * 0.5, (b[1] + b[3]) * 0.5
    i = min(g - 1, max(0, int((cx - allb[0]) / w * g)))
    j = min(g - 1, max(0, int((cy - allb[1]) / h * g)))
    return i, j


def compute_views(boxes):
    """图元外框 → (全图范围, [分区视图...])。

    两种候选范围各算一遍，取"更小但没丢掉内容"的那个：
    1) 中心点 1%~99% 分位窗口 + 30% 宽容差的并集——能挡住画在 x≈-520 万的
       残留多段线；
    2) 16×16 密度网格，只保留图元数 ≥3 的格子求并集——能挡住散落在图面
       四周的零星点位（这类点击穿了画面高度却没有任何内容）。
    图元之间若存在足够大的纵向空隙，就认为图面上本来画着两处独立的图，
    各给一个视图。
    """
    boxes = [b for b in boxes if (b[2] - b[0]) + (b[3] - b[1]) > 0] or [[0, 0, 1, 1]]
    allb = _union(boxes)

    # 候选 1：分位窗口
    cxs = sorted((b[0] + b[2]) * 0.5 for b in boxes)
    cys = sorted((b[1] + b[3]) * 0.5 for b in boxes)
    lx, hx = _pct(cxs, 0.01), _pct(cxs, 0.99)
    ly, hy = _pct(cys, 0.01), _pct(cys, 0.99)
    gx, gy = (hx - lx) * 0.30, (hy - ly) * 0.30
    lx, hx, ly, hy = lx - gx, hx + gx, ly - gy, hy + gy
    ka = [b for b in boxes
          if lx <= (b[0] + b[2]) * 0.5 <= hx and ly <= (b[1] + b[3]) * 0.5 <= hy]

    def _clip(b):
        """框尺寸超过内容范围 2 倍的（图面外的长引线、残留多段线）裁到窗口，
        否则单个跨 500 万的框就能把全图范围撑开，全图缩成中间一小块。"""
        bw, bh = b[2] - b[0], b[3] - b[1]
        if bw > (hx - lx) * 2.0:
            b = [max(b[0], lx), b[1], min(b[2], hx), b[3]]
        if bh > (hy - ly) * 2.0:
            b = [b[0], max(b[1], ly), b[2], min(b[3], hy)]
        return b

    if ka:
        box_a, keep_a = _union([_clip(b) for b in ka]), ka
    else:
        box_a, keep_a = allb, boxes

    # 候选 2：密度网格
    w = max(1e-6, allb[2] - allb[0])
    h = max(1e-6, allb[3] - allb[1])
    cnt = {}
    for b in boxes:
        k = _cell(b, allb, w, h)
        cnt[k] = cnt.get(k, 0) + 1
    box_b, keep_b = allb, boxes
    for thr in (3, 2):
        sub = [b for b in boxes if cnt.get(_cell(b, allb, w, h), 0) >= thr]
        if len(sub) >= len(boxes) * 0.9:
            box_b, keep_b = _union([_clip(b) for b in sub]), sub
            break

    if _area(box_b) < _area(box_a) and len(keep_b) >= len(boxes) * 0.9:
        box, kept = box_b, keep_b
    else:
        box, kept = box_a, keep_a

    main = _pad(box, 0.03)

    # ---- 分图幅：一张图上常并排放好几处独立平面（普通照明/空调配电…）。
    # 按"图元中心落入格子 + 8 邻接"做连通聚类，每个连通块给一个视图，
    # 这样看单幅时只画该幅的图元，全图只当缩略。
    views = _cluster_views(kept, main)
    if views:
        # 全图范围收紧成"实际图幅的并集"：分位窗口容易被图面外散落的残留图元撑大，
        # 一撑大全图就缩得只剩中间一小块，图外长引线还会满屏拉成蜘蛛网。
        # 面积不足最大图幅 15%、或长宽比 > 20 的细长条（图外长引线尾巴/标题条/残留件）
        # 只作独立视图，不进全图取景
        vs = sorted(views, key=lambda b: -_area(b))

        def _solid(b):
            w, h = b[2] - b[0], b[3] - b[1]
            if w <= 0 or h <= 0:
                return False
            ratio = max(w / h, h / w)
            return _area(b) >= _area(vs[0]) * 0.15 and ratio <= 20

        big = [b for b in vs if _solid(b)] or [vs[0]]
        return _pad(_union(big), 0.02), views

    keep = [((b[0] + b[2]) * 0.5, (b[1] + b[3]) * 0.5, b) for b in kept]

    keep.sort(key=lambda k: k[1])
    gap, at = 0.0, -1
    for i in range(len(keep) - 1):
        g = keep[i + 1][1] - keep[i][1]
        if g > gap:
            gap, at = g, i
    if at < 0 or gap < (main[3] - main[1]) * 0.12:
        return main, [main]
    return main, [_pad(_union([k[2] for k in keep[:at + 1]]), 0.03),
                  _pad(_union([k[2] for k in keep[at + 1:]]), 0.03)]


def _cluster_views(boxes, main, cell=48, max_views=8):
    """按密度网格的 8 邻接把图元聚成若干图幅，返回 [bbox…]（按图元数降序）。"""
    if len(boxes) < 40:
        return []
    w = max(1e-6, main[2] - main[0])
    h = max(1e-6, main[3] - main[1])
    g = max(w, h) / float(cell)
    cells = {}
    # 跨图引线（长度远大于格子）只参与绘制、不参与聚类：它的中心点落在图幅之间的
    # 空白里，会把互不相干的两块图粘成一个巨大 bbox，结果默认视图缩得太小、文字看不清。
    far = max(w, h) / float(cell) * 6
    for b in boxes:
        if (b[2] - b[0]) > far or (b[3] - b[1]) > far:
            continue
        cx, cy = (b[0] + b[2]) * 0.5, (b[1] + b[3]) * 0.5
        i = int((cx - main[0]) / g)
        j = int((cy - main[1]) / g)
        c = cells.get((i, j))
        if c is None:
            cells[(i, j)] = [b]
        else:
            c.append(b)
    # 并查集：8 邻接合并
    par = {}

    def find(a):
        while par[a] != a:
            par[a] = par[par[a]]
            a = par[a]
        return a

    for k in cells:
        par[k] = k
    for (i, j) in cells:
        for di in (-1, 0, 1):
            for dj in (-1, 0, 1):
                nk = (i + di, j + dj)
                if nk in cells:
                    ra, rb = find((i, j)), find(nk)
                    if ra != rb:
                        par[rb] = ra
    groups = {}
    for k, c in cells.items():
        groups.setdefault(find(k), []).extend(c)
    res = []
    area_main = _area(main)
    for bs in groups.values():
        u = _union(bs)
        if _area(u) < area_main * 0.008 or len(bs) < len(boxes) * 0.02:
            continue
        res.append((len(bs), _pad(u, 0.02)))
    if len(res) < 2:
        return []
    res.sort(key=lambda t: -t[0])
    return [b for _, b in res[:max_views]]


def _bbox_pts(p):
    xs = p[0::2]
    ys = p[1::2]
    return [min(xs), min(ys), max(xs), max(ys)]


def build_scene(data, doc=None):
    pal = _Pal()
    lcol = data.get("layer_col") or {}

    def col(ci, layer=None):
        return pal.of(ci, layer, lcol)

    out = {"ln": [], "lnb": [], "cr": [], "crb": [],
           "ar": [], "arb": [], "pl": [], "tx": [], "blk": {}, "ins": []}

    # ---- 图层表：画布按图层画，图层可单独开关（关掉"变更批注"就是去噪） ----
    LY, LYI, LYC = [], {}, []

    def li(name, n=1):
        nm = str(name if name not in (None, "") else "0")
        i = LYI.get(nm)
        if i is None:
            i = len(LY)
            LYI[nm] = i
            LY.append(nm)
            LYC.append(0)
        LYC[i] += n
        return i

    fr = data.get("frame") or [0, 0, 1, 1]
    diag = math.hypot(fr[2] - fr[0], fr[3] - fr[1]) or 1.0

    # ---- 普通图元 ----
    for ln in data.get("lines", ()):
        x1, y1, x2, y2 = ln[0], ln[1], ln[2], ln[3]
        c = col(ln[5] if len(ln) > 5 else 0, ln[4] if len(ln) > 4 else None)
        out["ln"] += [round(x1, 2), round(y1, 2), round(x2, 2), round(y2, 2), c,
                      li(ln[4] if len(ln) > 4 else None)]
        out["lnb"].append([round(min(x1, x2), 2), round(min(y1, y2), 2),
                           round(max(x1, x2), 2), round(max(y1, y2), 2)])

    for pl in data.get("plines", ()):
        pts = pl.get("p") or []
        if len(pts) < 2:
            continue
        flat = []
        for p in pts:
            flat += [round(p[0], 2), round(p[1], 2)]
        out["pl"].append({"p": flat, "z": 1 if pl.get("c") else 0,
                          "c": col(pl.get("ci", 0), pl.get("l")), "b": _bbox_pts(flat),
                          "li": li(pl.get("l"))})

    for cr in data.get("circles", ()):
        cx, cy, r = cr[0], cr[1], cr[2]
        out["cr"] += [round(cx, 2), round(cy, 2), round(r, 2),
                      col(cr[4] if len(cr) > 4 else 0, cr[3] if len(cr) > 3 else None),
                      li(cr[3] if len(cr) > 3 else None)]
        out["crb"].append([round(cx - r, 2), round(cy - r, 2), round(cx + r, 2), round(cy + r, 2)])

    for ar in data.get("arcs", ()):
        cx, cy = ar["c"]
        r = ar["r"]
        a0 = float(ar.get("a0") or 0.0)   # 弧度
        a1 = float(ar.get("a1") or 0.0)
        out["ar"] += [round(cx, 2), round(cy, 2), round(r, 2),
                      round(a0 * 57.29578, 2), round(a1 * 57.29578, 2),
                      col(ar.get("ci", 0), ar.get("l")),
                      li(ar.get("l"))]
        out["arb"].append([round(cx - r, 2), round(cy - r, 2), round(cx + r, 2), round(cy + r, 2)])

    # 轴号圈这类属性块的文字：扫描器直接读 attributeIds()/openAttribute 拿到 ATTRIB 值，
    # 不再从 DXF 旁路取（用户要求只允许 PyArx 读取）。

    # 属性块文字（轴号圈的 1/2/3、图名标签等）：扫描器直读 ATTRIB 得到的世界坐标文字。
    # 字高：顶层块的 height() 是块内值（0.01 量级），嵌套块才是已缩放值，这里按图框宽度归一，
    # 保证无论来自哪一层都能按图面比例正常显示。
    _fr = data.get("frame") or [0, 0, 1000, 1000]
    _fw = max(1.0, float(_fr[2]) - float(_fr[0]))
    _hmin = _fw / 700.0
    _hdef = _fw / 420.0
    # 有效区域：用已采集的线/圆/弧包围盒建成，外扩 35%。
    # 少数属性落在图例块的局部坐标系里（未随实例矩阵落到世界坐标），
    # x 会跑到 5e6 量级，画上去就是图面外的一堆孤立字，还会把全图取景撑大。
    _gbox = [b for b in (out["lnb"] + out["crb"] + out["arb"])
             if (b[2] - b[0]) > 0 or (b[3] - b[1]) > 0]
    if _gbox:
        _ext = [min(b[0] for b in _gbox), min(b[1] for b in _gbox),
                max(b[2] for b in _gbox), max(b[3] for b in _gbox)]
    else:
        _ext = list(_fr)
    _ew = max(1.0, _ext[2] - _ext[0])
    _eh = max(1.0, _ext[3] - _ext[1])
    _lim = [_ext[0] - _ew * 0.35, _ext[1] - _eh * 0.35,
            _ext[2] + _ew * 0.35, _ext[3] + _eh * 0.35]
    for a in (data.get("attrib") or ()):
        txt = str(a[6] or "").strip()
        if not txt:
            continue
        ax, ay = float(a[0] or 0), float(a[1] or 0)
        if not (_lim[0] <= ax <= _lim[2] and _lim[1] <= ay <= _lim[3]):
            continue
        hh = float(a[2] or 0)
        # 字高真实值分布是 200~3600（轴号 200/300/320/400/500、门窗编号 595、轴网 741）。
        # 唯一异常的是图例/设备块内属性：字高按块内单位存成 0.01，乘实例缩放也只有几个单位，
        # CAD 里本来就不可见；旧逻辑把它抬到 _fw/420（≈3400）会与材料表叠成一坨，故直接丢弃。
        if hh < 5.0:
            continue
        if hh > _fw / 40.0:
            hh = _fw / 40.0
        out["tx"].append([round(float(a[0]), 2), round(float(a[1]), 2),
                          round(hh, 2), round(float(a[3]) * 57.29578, 2),
                          int(a[4] or 0), col(int(a[5]), a[8]), txt,
                          li(a[8]), int(a[7] or 0)])

    for t in data.get("texts", ()):
        txt = (t.get("t") or "").strip()
        if not txt:
            continue
        ha = int(t.get("ha") or 0)
        va = int(t.get("va") or 0)
        out["tx"].append([round(t["x"], 2), round(t["y"], 2), round(float(t.get("h") or 0), 1),
                          round(float(t.get("r") or 0) * 57.29578, 1), ha,
                          col(t.get("ci", 0), t.get("l")), txt, li(t.get("l")), va])

    # ---- 块定义（块内坐标，基点 = 原点） ----
    def build_def(name, depth=0):
        if name in out["blk"]:
            return
        bd = data.get("block_defs", {}).get(name)
        out["blk"][name] = {"ln": [], "pl": [], "cr": [], "ar": [], "tx": [], "ins": []}
        if not bd or depth > 12:
            return
        g = bd.get("geom") or {}
        d = out["blk"][name]
        for ln in g.get("lines", ()):
            d["ln"] += [round(ln[0], 2), round(ln[1], 2), round(ln[2], 2), round(ln[3], 2),
                        col(ln[4] if len(ln) > 4 else 0, ln[5] if len(ln) > 5 else None)]
        for pl in g.get("plines", ()):
            pts = pl.get("p") or []
            if len(pts) >= 2:
                flat = []
                for p in pts:
                    flat += [round(p[0], 2), round(p[1], 2)]
                d["pl"].append({"p": flat, "z": 1 if pl.get("c") else 0,
                                "c": col(pl.get("ci", 0), pl.get("ly"))})
        for cr in g.get("circles", ()):
            d["cr"] += [round(cr[0], 2), round(cr[1], 2), round(cr[2], 2),
                        col(cr[3] if len(cr) > 3 else 0, cr[4] if len(cr) > 4 else None)]
        for ar in g.get("arcs", ()):
            d["ar"] += [round(ar[0], 2), round(ar[1], 2), round(ar[2], 2),
                        round(float(ar[3]) * 57.29578, 2), round(float(ar[4]) * 57.29578, 2),
                        col(ar[5] if len(ar) > 5 else 0, ar[6] if len(ar) > 6 else None)]
        for t in g.get("texts", ()):
            txt = (t.get("t") or "").strip()
            if not txt:
                continue
            # 设备符号块（$equip$…）里同时挂着“设备中文名”和一个 ATTDEF 代号（t/E/H/FW/Wh）。
            # ATTDEF 的 position 读不出来，全部落在块原点 (0,0)，再乘实例矩阵画出去，
            # 平面上每个设备符号上都会多出一坨和属性值重复的短字——这就是用户说的“画重了”。
            # 只丢这种“原点 + 极短”的代号，中文名称照常保留。
            if name.startswith("$equip") and abs(t["x"]) < 2 and abs(t["y"]) < 2 and len(txt) <= 4:
                continue
            # 块内文字必须写真实图层索引：以前把 va 写在第 8 位，渲染端按图层下标读，
            # 命中 LOFF[0]（"7月修改"层默认隐藏）后整块文字全被跳过（标高数字就是这么没的）。
            lay = t.get("ly") or t.get("l")
            d["tx"].append([round(t["x"], 2), round(t["y"], 2), round(float(t.get("h") or 0), 1),
                            round(float(t.get("r") or 0) * 57.29578, 1), int(t.get("ha") or 0),
                            col(t.get("ci", 0), lay), txt,
                            (li(lay) if lay else -1), int(t.get("va") or 0)])
        for sub in g.get("ins", ()):
            n = sub.get("n")
            if not n:
                continue
            build_def(n, depth + 1)
            p = sub.get("p") or [0, 0]
            d["ins"].append({"k": n, "x": round(p[0], 2), "y": round(p[1], 2),
                             "r": round(float(sub.get("r") or 0), 3),
                             "sx": float(sub.get("s") or 1.0), "sy": float(sub.get("sy") or 1.0),
                             "m": sub.get("m"),
                             "atx": sub.get("atx") or []})

    _bb_cache = {}

    def def_bbox(name, depth=0):
        key = name
        if key in _bb_cache:
            return _bb_cache[key]
        d = out["blk"].get(name)
        if not d:
            return [0, 0, 0, 0]
        xs, ys = [], []
        for i in range(0, len(d["ln"]), 5):
            xs += [d["ln"][i], d["ln"][i + 2]]
            ys += [d["ln"][i + 1], d["ln"][i + 3]]
        for e in d["pl"]:
            xs += e["p"][0::2]
            ys += e["p"][1::2]
        for i in range(0, len(d["cr"]), 4):
            xs += [d["cr"][i] - d["cr"][i + 2], d["cr"][i] + d["cr"][i + 2]]
            ys += [d["cr"][i + 1] - d["cr"][i + 2], d["cr"][i + 1] + d["cr"][i + 2]]
        for i in range(0, len(d["ar"]), 6):
            xs += [d["ar"][i] - d["ar"][i + 2], d["ar"][i] + d["ar"][i + 2]]
            ys += [d["ar"][i + 1] - d["ar"][i + 2], d["ar"][i + 1] + d["ar"][i + 2]]
        for t in d["tx"]:
            w = len(t[6]) * abs(t[2]) * 0.8
            xs += [t[0], t[0] + w]
            ys += [t[1], t[1] + abs(t[2])]
        for s in d["ins"]:
            # 嵌套块：子块坐标要平移 + 缩放（旋转按保守外接处理），否则外框只算到第一层，
            # 柜内 3894 个嵌套符号会被裁掉，看起来就是"块显示不全"
            if depth < 12:
                sb = def_bbox(s["k"], depth + 1)
            else:
                sb = [0, 0, 0, 0]
            mx, my = s["x"], s["y"]
            kx, ky = abs(s.get("sx") or 1.0), abs(s.get("sy") or 1.0)
            r = abs(s.get("r") or 0.0)
            cx, cy = (sb[0] + sb[2]) * 0.5, (sb[1] + sb[3]) * 0.5
            hx, hy = (sb[2] - sb[0]) * 0.5 * kx, (sb[3] - sb[1]) * 0.5 * ky
            if r:
                rad = math.hypot(hx, hy)
                hx = hy = rad
            xs += [mx + cx - hx, mx + cx + hx]
            ys += [my + cy - hy, my + cy + hy]
        if not xs:
            _bb_cache[key] = [0, 0, 0, 0]
            return [0, 0, 0, 0]
        bb = [min(xs), min(ys), max(xs), max(ys)]
        _bb_cache[key] = bb
        return bb

    for name, bd in (data.get("block_defs") or {}).items():
        build_def(name)

    # ---- 块参照：位置 + 旋转 + 缩放，bbox 按块定义推算 ----
    cache = {}
    for b in data.get("blocks", ()):
        n = b.get("n")
        if n not in out["blk"]:
            continue
        p = b.get("p") or [0, 0]
        r = float(b.get("r") or 0.0)
        sx = float(b.get("s") or 1.0)
        sy = float(b.get("sy") or sx)
        if n not in cache:
            cache[n] = def_bbox(n)
        bb = cache[n]
        mm = b.get("m")
        if mm and len(mm) == 6:
            # 用 CAD 原始矩阵算包围盒：镜像/非正交的块也能框准
            m00, m01, m10, m11, tx, ty = mm
            pts = []
            for cx, cy in ((bb[0], bb[1]), (bb[2], bb[1]), (bb[2], bb[3]), (bb[0], bb[3])):
                pts.append((m00 * cx + m01 * cy + tx, m10 * cx + m11 * cy + ty))
            xs = [q[0] for q in pts]
            ys = [q[1] for q in pts]
            box = [round(min(xs), 2), round(min(ys), 2), round(max(xs), 2), round(max(ys), 2)]
            x, y = round(tx, 2), round(ty, 2)
        else:
            w, h = (bb[2] - bb[0]) * abs(sx), (bb[3] - bb[1]) * abs(sy)
            ex = abs(w * 0.5) + abs(h * 0.5)   # 旋转后取保守外接
            x, y = round(p[0], 2), round(p[1], 2)
            box = [round(p[0] - ex, 2), round(p[1] - ex, 2),
                   round(p[0] + ex, 2), round(p[1] + ex, 2)]
        Lb = li(b.get("l") if b.get("l") not in (None, "") else b.get("layer"))
        out["ins"].append({
            "k": n, "x": x, "y": y,
            "r": round(r, 4), "sx": sx, "sy": sy,
            "c": col(b.get("ci", 0), b.get("l")),
            "m": mm, "b": box, "li": Lb})
        # 块属性统一在 build_scene 里按世界坐标画（见上方 data["attrib"]），这里不再重复采集

    # ---- 视图范围：图元外框分位窗口内求并集，并自动拆出图面上的两处图 ----
    boxes = []
    for bb in out["lnb"] + out["crb"] + out["arb"]:
        boxes.append(bb)
    for e in out["pl"]:
        boxes.append(e["b"])
    for t in out["tx"]:
        boxes.append([t[0], t[1], t[0] + abs(t[2]), t[1] + abs(t[2])])
    # 块参照参与聚类与并集，但外框会被超大块定义带偏，这里截到图元级别
    # ---- 表格（ACAD_TABLE）：系统图的设备参数（Pe=/Kx=/COS∅=/Pjs=/Ijs=）都在这里，
    # 之前整块没画，看着就是"文字都没渲染"。单元格只有文字没有坐标，按行列均分铺。
    tb = []
    for t in data.get("tables", ()):
        b = t.get("b")
        rows, cols = int(t.get("rows") or 0), int(t.get("cols") or 0)
        cells = t.get("cells") or []
        if not b or rows <= 0 or cols <= 0 or not cells:
            continue
        tb.append({"b": [round(v, 2) for v in b], "r": rows, "c": cols,
                   "t": [[_mtext(cells[r][c]) if r < len(cells) and c < len(cells[r]) else ""
                          for c in range(cols)] for r in range(rows)],
                   "li": li(t.get("l"))})
    out["tb"] = tb

    for e in out["ins"]:
        x, y = e["x"], e["y"]
        boxes.append([x, y, x, y])
    main, views = compute_views(boxes)
    # 退化兜底：main 只收"块参照中心点"这类零尺寸 box 时，宽或高会塌成 0，
    # fitBox 于是把 k 算到 1e9 量级 —— 画布放大到看不见任何东西。
    # 出现退化就用视图并集（再退一步用裁剪窗）顶上。
    if (main[2] - main[0]) < 1 or (main[3] - main[1]) < 1:
        cand = _union(views) if views else _union([[b[0], b[1], b[2], b[3]] for b in boxes])
        if (cand[2] - cand[0]) >= 1 and (cand[3] - cand[1]) >= 1:
            main = cand
    out["b"] = [round(v, 2) for v in main]
    # 内容框（裁剪 + 视图归位的边界）：用真实图元外接并集，而不是 main 再外扩 15%。
    # 这张图的图形铺到了图框之外（右侧厂区、远端轴号），main 是"更小但没丢内容"的
    # 分位窗口，用它当边界等于把真实内容挡在外面——放大/拖动一到那儿就被拽回图框里。
    # 只剔两类异常：跨半张图的巨框（块 bbox 估坏，能到 4.6 亿）、离主内容中心 3 倍主尺寸的孤点（残留多段线）。
    mw, mh = main[2] - main[0], main[3] - main[1]
    mcx, mcy = (main[0] + main[2]) * 0.5, (main[1] + main[3]) * 0.5
    keep = []
    for bb in boxes:
        bw, bh = bb[2] - bb[0], bb[3] - bb[1]
        if bw > mw * 2.0 or bh > mh * 2.0:
            continue
        cx, cy = (bb[0] + bb[2]) * 0.5, (bb[1] + bb[3]) * 0.5
        if abs(cx - mcx) > mw * 3 or abs(cy - mcy) > mh * 3:
            continue
        keep.append(bb)
    cb = _union(keep) if keep else main
    # 尺寸塌了（只剩零尺寸点）就用 main，避免 clamp 退化成一条线
    if (cb[2] - cb[0]) < 1 or (cb[3] - cb[1]) < 1:
        cb = main
    out["cb"] = [round(v, 2) for v in cb]
    if len(views) > 1:
        # 面积大的那块当默认视图（这张图上就是顶部整条系统图带）
        views.sort(key=lambda b: -(b[2] - b[0]) * (b[3] - b[1]))
        out["v"] = [[round(v, 2) for v in vb] for vb in views]
    else:
        out["v"] = []
    # 图层清单 + 每层图元数 + 每层颜色 + 默认关闭的图层（修改痕迹类，等于给画布做了去噪）
    out["ly"] = LY
    out["lyc"] = LYC
    out["lycol"] = []
    for nm in LY:
        v = lcol.get(nm)
        out["lycol"].append(pal.of(v if v is not None else 7))
    out["lyh"] = [i for i, nm in enumerate(LY) if _NOISE_LAYER.search(nm)]
    # ACI 7 是 CAD 的"随背景反转"自适应色：白底下是黑、黑底下是白。aci_hex 按白底算成了
    # #000000，直接铺到深色画布上就是一团黑——101 图 6175 条线和几乎全部标注都是 7 号色，
    # 连模板的 DEFC 也取 COL[7]，于是文字整片消失。深底渲染必须把它落到近白。
    pal.hex[7] = "#E6E8EB"
    out["c"] = pal.hex            # 真彩色按需追加，必须最后取
    return out


def export(dwg, out_html=None, as_json=False):
    dwg = os.path.abspath(dwg)
    cache = scan_load._cache_path(dwg)          # 扫描结果落在 %TEMP%\pyarx_scan
    if not os.path.isfile(cache):
        scan_load.scan_dwg(dwg)                 # 没缓存就跑一次 accoreconsole
    with open(cache, "r", encoding="utf-8") as f:
        data = json.load(f)
    doc = scan_load.build_document(data)
    scene = build_scene(data, doc)
    scene["f"] = os.path.basename(dwg)
    scene["n"] = (len(data.get("lines", ())) + len(data.get("plines", ()))
                  + len(data.get("circles", ())) + len(data.get("arcs", ()))
                  + len(data.get("texts", ())) + len(data.get("blocks", ())))

    if as_json:
        if out_html is None:
            out_html = os.path.splitext(dwg)[0] + ".scene.json"
        with open(out_html, "w", encoding="utf-8") as f:
            json.dump(scene, f, ensure_ascii=False, separators=(",", ":"))
        return out_html, os.path.getsize(out_html) / 1024.0, scene

    with open(TEMPLATE, "r", encoding="utf-8") as f:
        html = f.read()
    body = json.dumps(scene, ensure_ascii=False, separators=(",", ":"))
    html = html.replace("__SCENE__", body)

    if out_html is None:
        out_html = os.path.splitext(dwg)[0] + ".html"
    with open(out_html, "w", encoding="utf-8") as f:
        f.write(html)
    return out_html, len(html) / 1024.0, scene


if __name__ == "__main__":
    args = [a for a in sys.argv[1:]]
    out = None
    as_json = False
    for flag in ("-j", "--json"):
        if flag in args:
            args.remove(flag)
            as_json = True
    if "-o" in args:
        i = args.index("-o")
        out = args[i + 1]
        del args[i:i + 2]
    path, kb, scene = export(args[0], out, as_json)
    print("写出 %s  %.0f KB  图元 %d  块定义 %d  块参照 %d  图层 %d（默认隐藏 %d 层批注）"
          % (path, kb, scene["n"], len(scene["blk"]), len(scene["ins"]),
             len(scene.get("ly") or []), len(scene.get("lyh") or [])))
