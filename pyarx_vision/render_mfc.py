# -*- utf-8 -*-
"""render_mfc —— 用项目自研 MFCXaml 画布（PyMfcXml DwgCanvas）把每张图渲染成 PNG

与 render.py(PIL) 的区别：这里是真正的画布控件 view —— SVG 交给控件解析、
文字旋转由控件绕锚点实现、再由 saveControlPng 截屏，所见即控件所见。

调用链：Window(title,w,h) → addDwgCanvas(id,0,0,w,h) → loadDwgSvg(id,svg)
        → setDwgView(id,1,0,0) → saveControlPng(id,png)
"""
import ctypes
import json
import math
import os
import re
import sys
import time

sys.path.insert(0, r"G:\PyArx")
sys.path.insert(0, r"G:\PyArx\X64\Release")
import PyMfcXml as M

from pyarx_vision import frames as F
from pyarx_vision.aci import aci_hex, is_gray

SRC = r"C:\temp\scan.json"
OUT_PNG = os.environ.get("MFC_OUT", r"G:\PyArx\docs\sheets_mfc")
OUT_SVG = os.environ.get("MFC_SVG", r"G:\PyArx\docs\sheets_mfc_svg")
# 画布底色：CAD 习惯黑底白线。DARK=0 可切回白底。
DARK = os.environ.get("MFC_DARK", "1") == "1"
BG = "#000000" if DARK else "#FFFFFF"
INK = "#e6e6e6" if DARK else "#333333"
BASE_GRAY = "#4a4a4a" if DARK else "#b9b9b9"   # 建筑底图：黑底上用暗灰退到背景
CANVAS_ID = 7200
DROP_BASE = os.environ.get("MFC_DROP_BASE", "0") == "1"
DROP_DIAG = os.environ.get("MFC_DROP_DIAG", "1") == "1"   # 剔除品红超长斜线（标注引线）


def ci_hex(ci, layer_col=None):
    """ACI → SVG 色。灰阶压成 BASE_GRAY 让底图退到背景；7 号在 CAD 是
    "白/黑随背景"，黑底上必须翻成白，否则整张图看不见。
    """
    try:
        ci = int(ci)
    except Exception:
        ci = 256
    if layer_col and ci >= 256:
        return ci_hex(layer_col, None)
    if is_gray(ci):
        return BASE_GRAY
    if ci in (0, 7):
        return INK
    return aci_hex(ci)


def is_base(ci):
    return is_gray(ci)


def esc(s):
    return (s or "").replace("&", "&amp;").replace("<", "&lt;").replace(">", "&gt;")


def safe_name(s):
    s = re.sub(r'[\\/:*?"<>|]', "", s or "").strip()
    return s[:24] or "untitled"


def build_svg(scan, frame, CW, CH, margin=14):
    x0, y0, x1, y1 = frame
    SPANX, SPANY = x1 - x0, y1 - y0
    k = min((CW - 2 * margin) / SPANX, (CH - 2 * margin) / SPANY)
    imgW, imgH = SPANX * k, SPANY * k
    px0 = (CW - imgW) / 2.0
    py0 = (CH - imgH) / 2.0

    def mp(x, y):
        return (px0 + (x - x0) * k, py0 + imgH - (y - y0) * k)

    laycol = scan.get("layer_col") or {}

    def eff_ci(ci, layer):
        """ByLayer(256) 解析成图层真实色号，剔除判断必须用它而不是 256。"""
        try:
            ci = int(ci)
        except Exception:
            ci = 256
        if ci >= 256:
            v = laycol.get(layer)
            try:
                return int(v) if v is not None else 7
            except Exception:
                return 7
        return ci

    P = ['<svg xmlns="http://www.w3.org/2000/svg" width="%d" height="%d" viewBox="0 0 %d %d">'
         % (CW, CH, CW, CH),
         '<rect width="%d" height="%d" fill="%s"/>' % (CW, CH, BG)]

    def ln(a, b, col=INK, w=None):
        # 线宽分级：底图灰压细，避免密集墙体糊成一片；电气保持 1px
        if w is None:
            w = 0.7 if col == BASE_GRAY else 1.0
        P.append('<line x1="%.1f" y1="%.1f" x2="%.1f" y2="%.1f" stroke="%s" stroke-width="%.1f"/>'
                 % (a[0], a[1], b[0], b[1], col, w))

    def poly(pts, closed, col=INK):
        q = [mp(p[0], p[1]) for p in pts]
        if closed:
            q.append(q[0])
        for i in range(len(q) - 1):
            ln(q[i], q[i + 1], col)

    def ring(cx, cy, r, a0=0.0, a1=2 * math.pi, seg=24, col=INK):
        p = mp(cx, cy)
        rr = r * k
        if rr < 0.5:
            return
        step = (a1 - a0) / seg
        q = [(p[0] + rr * math.cos(a0 + step * i), p[1] - rr * math.sin(a0 + step * i))
             for i in range(seg + 1)]
        for i in range(seg):
            ln(q[i], q[i + 1], col)

    # 折线 / 直线
    for p in scan.get("plines", []):
        pts = p.get("p") or []
        if len(pts) < 2:
            continue
        if not (max(q[0] for q in pts) < x0 or min(q[0] for q in pts) > x1 or
                max(q[1] for q in pts) < y0 or min(q[1] for q in pts) > y1):
            eci = eff_ci(p.get("ci", 256), p.get("l"))
            if DROP_BASE and is_base(eci):
                continue
            poly(pts, p.get("c"), ci_hex(eci))
    for l in scan.get("lines", []):
        if max(l[0], l[2]) < x0 or min(l[0], l[2]) > x1:
            continue
        if max(l[1], l[3]) < y0 or min(l[1], l[3]) > y1:
            continue
        eci = eff_ci(l[5] if len(l) > 5 else 256, l[4] if len(l) > 4 else "0")
        if DROP_BASE and is_base(eci):
            continue
        # 标注线：品红(6)、长过半幅、明显倾斜（dx 与 dy 都大）。
        # 这类是变更批注的斜引线，横穿图面把图纸切开，不是图纸内容。
        # 桥架/母线那种长横线 dx 大 dy≈0，不受影响。
        if DROP_DIAG and eci == 6:
            dx, dy = abs(l[2] - l[0]), abs(l[3] - l[1])
            if (dx > 0.1 * SPANX and dy > 0.1 * SPANY and
                    math.hypot(dx, dy) > 0.45 * max(SPANX, SPANY)):
                continue
        ln(mp(l[0], l[1]), mp(l[2], l[3]), ci_hex(eci))

    for c in scan.get("circles", []):
        eci = eff_ci(c[4] if len(c) > 4 else 256, c[3] if len(c) > 3 else "0")
        if DROP_BASE and is_base(eci):
            continue
        if x0 - c[2] <= c[0] <= x1 + c[2] and y0 - c[2] <= c[1] <= y1 + c[2]:
            ring(c[0], c[1], c[2], seg=16, col=ci_hex(eci))
    for a in scan.get("arcs", []):
        eci = eff_ci(a.get("ci", 256), a.get("l"))
        if DROP_BASE and is_base(eci):
            continue
        if x0 - a["r"] <= a["c"][0] <= x1 + a["r"] and y0 - a["r"] <= a["c"][1] <= y1 + a["r"]:
            ring(a["c"][0], a["c"][1], a["r"], a["a0"], a["a1"], seg=12, col=ci_hex(eci))

    # 块：递归展开 + 按世界坐标裁剪
    # 关键：块定义 bbox 全是 null，实例插入点又在图框外几万单位、几何摊开 70 万宽，
    # 靠插入点估框必然错（会把整块底图塞进封面，同时让局部图漏画）。
    # 所以 bbox 从 geom 自己算，展开后每条图元再按图框裁一次。
    defs = scan.get("block_defs") or {}
    bom = []
    budget = [0]
    _LIMIT = 600000
    GB, _busy = {}, []

    def gbox(nm):
        if nm in GB:
            return GB[nm]
        if nm in _busy:
            return None
        _busy.append(nm)
        g = ((defs.get(nm) or {}).get("geom") or {})
        xs, ys = [], []
        for l in g.get("lines", []):
            xs += [l[0], l[2]]
            ys += [l[1], l[3]]
        for p in g.get("plines", []):
            for q in (p.get("p") or []):
                xs.append(q[0])
                ys.append(q[1])
        for c in g.get("circles", []):
            xs += [c[0] - c[2], c[0] + c[2]]
            ys += [c[1] - c[2], c[1] + c[2]]
        for a in g.get("arcs", []):
            xs += [a[0] - a[2], a[0] + a[2]]
            ys += [a[1] - a[2], a[1] + a[2]]
        for t2 in g.get("texts", []):
            xs.append(t2["x"])
            ys.append(t2["y"])
        for ins in g.get("ins", []):
            sub = gbox(ins.get("n"))
            if sub:
                xs += [ins["p"][0] + sub[0], ins["p"][0] + sub[2]]
                ys += [ins["p"][1] + sub[1], ins["p"][1] + sub[3]]
        _busy.pop()
        GB[nm] = (min(xs), min(ys), max(xs), max(ys)) if xs else None
        return GB[nm]

    def _A(r, s1, s2):
        c, sg = math.cos(r), math.sin(r)
        return ((s1 * c, -s2 * sg), (s1 * sg, s2 * c))

    def _mul(X, Y):
        return ((X[0][0] * Y[0][0] + X[0][1] * Y[1][0], X[0][0] * Y[0][1] + X[0][1] * Y[1][1]),
                (X[1][0] * Y[0][0] + X[1][1] * Y[1][0], X[1][0] * Y[0][1] + X[1][1] * Y[1][1]))

    def _ap(X, p):
        return (X[0][0] * p[0] + X[0][1] * p[1], X[1][0] * p[0] + X[1][1] * p[1])

    def _near(qx, qy, rr):
        return not (qx + rr < x0 or qx - rr > x1 or qy + rr < y0 or qy - rr > y1)

    def ent_col(ci, ly, top_col):
        """块内图元的最终色。
        ci=0   ByBlock   → 取**最顶层** blockref 的色（不是当前这一层块的色）
        ci=256 ByLayer   → 取图元自己所在图层的色
        其余               → 显式索引色，照用
        """
        try:
            ci = int(ci)
        except Exception:
            ci = 256
        if ci == 0:
            return top_col
        if ci >= 256:
            e = eff_ci(ci, ly)
            if e in (0, 256):
                e = 7
            return ci_hex(e)
        return ci_hex(ci)

    def emit(name, Am, t, rot_acc, depth, top_col=INK):
        if depth > 5 or budget[0] > _LIMIT:
            return
        g = ((defs.get(name) or {}).get("geom") or {})
        if not g:
            return
        sc = (abs(Am[0][0] * Am[0][0] + Am[1][0] * Am[1][0])) ** 0.5 or 1.0

        def T(px, py):
            return (Am[0][0] * px + Am[0][1] * py + t[0], Am[1][0] * px + Am[1][1] * py + t[1])

        for l in g.get("lines", []):
            ax, ay = T(l[0], l[1])
            bx, by = T(l[2], l[3])
            if (ax < x0 and bx < x0) or (ax > x1 and bx > x1):
                continue
            if (ay < y0 and by < y0) or (ay > y1 and by > y1):
                continue
            budget[0] += 1
            ln(mp(ax, ay), mp(bx, by),
               ent_col(l[4] if len(l) > 4 else 256, l[5] if len(l) > 5 else "0", top_col))
        for pl in g.get("plines", []):
            pts0 = pl.get("p") or []
            if len(pts0) < 2:
                continue
            wpts = [T(q[0], q[1]) for q in pts0]
            if max(q[0] for q in wpts) < x0 or min(q[0] for q in wpts) > x1:
                continue
            if max(q[1] for q in wpts) < y0 or min(q[1] for q in wpts) > y1:
                continue
            budget[0] += len(wpts)
            poly(wpts, pl.get("c"), ent_col(pl.get("ci", 256), pl.get("ly", "0"), top_col))
        for cc in g.get("circles", []):
            qx, qy = T(cc[0], cc[1])
            rr = abs(cc[2] * sc)
            if not _near(qx, qy, rr):
                continue
            budget[0] += 1
            ring(qx, qy, rr, seg=12,
                 col=ent_col(cc[3] if len(cc) > 3 else 256, cc[4] if len(cc) > 4 else "0", top_col))
        for a in g.get("arcs", []):
            qx, qy = T(a[0], a[1])
            rr = abs(a[2] * sc)
            if not _near(qx, qy, rr):
                continue
            budget[0] += 1
            ring(qx, qy, rr, a[3] + rot_acc, a[4] + rot_acc, seg=10,
                 col=ent_col(a[5] if len(a) > 5 else 256, a[6] if len(a) > 6 else "0", top_col))
        for t2 in g.get("texts", []):
            wx, wy = T(t2["x"], t2["y"])
            if not (x0 <= wx <= x1 and y0 <= wy <= y1):
                continue
            bom.append({"x": wx, "y": wy, "t": t2["t"], "h": t2["h"] * sc,
                        "r": float(t2.get("r", 0.0)) + rot_acc, "ha": t2.get("ha", 0),
                        "col": ent_col(t2.get("ci", 256), t2.get("ly", "0"), top_col)})
        for ins in g.get("ins", []):
            p2 = ins.get("p")
            if not p2:
                continue
            sub = gbox(ins.get("n"))
            if sub:
                cx, cy = (sub[0] + sub[2]) * 0.5, (sub[1] + sub[3]) * 0.5
                rr0 = 0.5 * math.hypot(sub[2] - sub[0], sub[3] - sub[1])
                rr0 *= max(abs(float(ins.get("s", 1.0) or 1.0)), abs(float(ins.get("sy", 1.0) or 1.0)))
                rr0 *= sc
                qw = T(p2[0] + cx, p2[1] + cy)
                if not _near(qw[0], qw[1], rr0):
                    continue
            ip = _ap(Am, p2)
            A2 = _mul(Am, _A(float(ins.get("r", 0.0) or 0.0),
                             float(ins.get("s", 1.0) or 1.0), float(ins.get("sy", 1.0) or 1.0)))
            # 深层块的 ByBlock 一直穿透到最外层 ref，所以传 top_col 下去，不传自己的色
            emit(ins.get("n"), A2, (ip[0] + t[0], ip[1] + t[1]),
                 rot_acc + float(ins.get("r", 0.0) or 0.0), depth + 1, top_col)

    for b in scan.get("blocks", []):
        nm, pos = b.get("n"), b.get("p")
        if not pos:
            continue
        ci = b.get("ci", 256)
        eci = eff_ci(ci, b.get("l"))
        if DROP_BASE and is_base(eci):
            continue      # 剔除底图：灰阶块整块不画，只留电气
        bb = gbox(nm)
        if not bb:
            continue
        rot = float(b.get("r", 0.0) or 0.0)
        sx = float(b.get("s", 1.0) or 1.0)
        sy = float(b.get("sy", sx) or sx)
        cx, cy = (bb[0] + bb[2]) * 0.5, (bb[1] + bb[3]) * 0.5
        rr = 0.5 * math.hypot(bb[2] - bb[0], bb[3] - bb[1]) * max(abs(sx), abs(sy))
        c_, s_ = math.cos(rot), math.sin(rot)
        wx = pos[0] + sx * cx * c_ - sy * cy * s_
        wy = pos[1] + sx * cx * s_ + sy * cy * c_
        if not _near(wx, wy, rr):
            continue
        emit(nm, _A(rot, sx, sy), (pos[0], pos[1]), rot, 0, ci_hex(eci))

    # 文字：真旋转（CAD 逆时针 → SVG 顺时针取负），对齐按 ha
    # 0 左 / 1 中 / 2 右 / 3 两点对齐(起点) / 4 居中 / 5 两端拟合(起点)
    ANCH = {0: "start", 1: "middle", 2: "end", 3: "start", 4: "middle", 5: "start"}

    def _cw(s):
        """CAD 字宽近似（em）：全角 1.0，ASCII 0.55。与 sysmode.enrich 同口径。"""
        return sum(1.0 if ord(c) > 127 else 0.55 for c in s)

    texts = [t for t in scan.get("texts", []) if x0 <= t["x"] <= x1 and y0 <= t["y"] <= y1] + bom
    cap = max(10.0, CW * 0.030)
    nt = 0
    for t in texts:
        if not (t.get("t") or "").strip():
            continue
        fs = t["h"] * k * 0.95
        if fs < 2.2:
            continue
        fs = min(fs, cap)
        px, py = mp(t["x"], t["y"])
        # SVG 的 y 是基线；CAD 的插入点随 va 变（0 基线 / 1 底 / 2 中 / 3 顶），
        # 不补这一项，顶部与居中锚点的文字会整体偏半个字高。
        va = int(t.get("va", 0) or 0)
        if va == 2:
            py += fs * 0.33
        elif va == 3:
            py += fs * 0.66
        elif va == 1:
            py -= fs * 0.15
        a = ANCH.get(int(t.get("ha", 0)), "start")
        deg = -math.degrees(float(t.get("r", 0.0) or 0.0))
        if abs(deg) < 0.5:
            deg = 0.0
        # CAD 里文字宽 = Σem宽 · 字高 · widthFactor，SVG 默认按字体自然宽度排，
        # 图纸里 8277 条 wf 为 0.7 一类的压缩就被丢掉，文字比图上宽约 1.4 倍，
        # text-anchor 的 middle/end 于是落错位置。用 textLength 强制回 CAD 宽度。
        wpx = _cw(t["t"]) * fs * float(t.get("wf", 1.0) or 1.0)
        col = t.get("col") or ci_hex(t.get("ci", 256), laycol.get(t.get("l")))
        if DROP_BASE and t.get("ci") is not None and is_base(t.get("ci")):
            continue
        tf = (' transform="rotate(%.2f,%.1f,%.1f)"' % (deg, px, py)) if deg else ""
        P.append('<text x="%.1f" y="%.1f" font-size="%.1f" fill="%s" text-anchor="%s"'
                 ' textLength="%.1f" lengthAdjust="spacingAndGlyphs"%s>%s</text>'
                 % (px, py, fs, col, a, wpx, tf, esc(t["t"])))
        nt += 1
    P.append("</svg>")
    return "".join(P), k, nt


def main():
    d = json.load(open(SRC, encoding="utf-8"))
    fr = F.find_frames(d)
    os.makedirs(OUT_PNG, exist_ok=True)
    os.makedirs(OUT_SVG, exist_ok=True)

    u = ctypes.windll.user32
    CW0, CH0 = u.GetSystemMetrics(0), u.GetSystemMetrics(1)
    WW, WH = int(CW0 * 0.93), int(CH0 * 0.86)

    win = M.Window("PyArx 图纸 view 核对", WW, WH)
    win.set_theme(False)
    win.add_dwg_canvas(CANVAS_ID, 0, 0, WW, WH)
    win.show()
    M.pump(900)
    CW, CH = win.client_size()
    win.move(CANVAS_ID, 0, 0, CW, CH)        # 画布对齐客户区，否则截图留黑边
    M.pump(300)
    print("canvas client = %dx%d" % (CW, CH))

    rows = []
    t0 = time.time()
    for f in fr:
        title = F.sheet_title(d, f["frame"])
        kind, _ = F.classify(d, f["frame"])
        svg, k, nt = build_svg(d, f["frame"], CW, CH)
        name = "sheet_%02d_%s" % (f["no"], safe_name(title))
        sp = os.path.join(OUT_SVG, name + ".svg")
        with open(sp, "w", encoding="utf-8") as fp:
            fp.write(svg)
        win.load_dwg_svg(CANVAS_ID, svg)
        M.pump(120)
        win.set_dwg_view(CANVAS_ID, 1.0, 0, 0)
        M.pump(60)
        pp = os.path.join(OUT_PNG, name + ".png")
        win.save_control_png(CANVAS_ID, pp)
        rows.append({"no": f["no"], "title": title, "kind": kind, "png": pp, "svg": sp,
                     "texts": nt, "k": round(k, 5), "scale": "1:%d" % round(1.0 / k),
                     "kb": round(os.path.getsize(pp) / 1024), "svg_kb": round(len(svg) / 1024)})
        print("%2d %-24s %-6s text%4d 1:%d %4dKB" % (f["no"], title[:22], kind, nt,
                                                     round(1.0 / k), os.path.getsize(pp) // 1024))
    json.dump(rows, open(OUT_PNG.rstrip("\\") + "_index.json", "w", encoding="utf-8"),
              ensure_ascii=False, indent=1)
    print("\n共 %d 张  %.1fs  PNG %.1fMB" % (len(rows), time.time() - t0,
                                            sum(r["kb"] for r in rows) / 1024))
    KEEP = float(os.environ.get("MFC_KEEP", "0"))
    if KEEP > 0:
        te = time.time() + KEEP
        while time.time() < te:
            M.pump(30)
    win.close()
    M.pump(200)


if __name__ == "__main__":
    main()
