# -*- coding: utf-8 -*-
"""xml_view.py —— 参照 webview_template.html（HTML/Canvas 看图器），用 XML 显示同一张 CAD 图纸。

三者分工：
  · 数据：复用 webview.build_scene（同一份显示列表：线 / 多段线 / 圆 / 弧 / 文字 / 块 + 图层 + 调色板）
  · 场景：输出 <XScene>（XLine / XPolyline / XCircle / XArc / XText），画布直扫标签，不再走 SVG 文本往返
  · 界面：一份纯 XML（flex 排版：顶栏 / 图层栏 / 画布 / 状态栏），没有手算坐标

LOD（细节层级）：块参照按屏幕尺寸分三档，和 HTML 版"全图只当缩略"同思路
  · < 8px   → 画一个小方块（缩略时块内细节本来就看不清）
  · < 48px  → 画块外框矩形
  · ≥ 48px  → 按块参照矩阵展开块内图元

用法：python xml_view.py [--dwg <路径>] [--view N] [--out xxx.png]
"""
import glob
import json
import math
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.abspath(os.path.join(HERE, '..', '..'))
BIN = os.environ.get('PYARX_BIN') or os.path.join(ROOT, 'X64', 'Release')
for p in (HERE, ROOT, BIN):
    if p not in sys.path:
        sys.path.insert(0, p)
os.add_dll_directory(BIN)
os.environ.setdefault('PYARX_D2D_RT', 'software')

import PyMfcXml as M          # noqa: E402
import webview                # noqa: E402

WIN_W, WIN_H = 1280, 866      # 含标题栏留量：页面按客户区 1280×820 排版，状态栏才不会被裁掉
PAD = 12


# ---------------------------------------------------------------- 数据
def find_cache(dwg=None):
    """扫描缓存：%TEMP%\\pyarx_scan\\<dwg>_<hash>.json（webview.py 同一份）。"""
    cdir = os.path.join(os.environ.get('TEMP', ''), 'pyarx_scan')
    if dwg:
        stem = os.path.splitext(os.path.basename(dwg))[0]
        hit = [p for p in glob.glob(os.path.join(cdir, '*.json')) if stem in os.path.basename(p)]
        return hit[0] if hit else None
    js = sorted(glob.glob(os.path.join(cdir, '*.json')), key=os.path.getmtime)
    return js[-1] if js else None


def load_scene(dwg=None):
    cache = find_cache(dwg)
    if not cache:
        raise SystemExit('没有扫描缓存，先用 webview.py 扫一次 DWG')
    with open(cache, 'r', encoding='utf-8') as f:
        data = json.load(f)
    return webview.build_scene(data), os.path.basename(cache), data


def lw_context(data, on=False):
    """线宽显示上下文：图元枚举 + 图层枚举（扫描缓存 v6 起才有 'lw'/'layer_lw'）。"""
    lines = [int(r[6]) if len(r) > 6 else -1 for r in (data.get('lines') or ())]
    layers = {k: int(v) for k, v in (data.get('layer_lw') or {}).items()}
    # has：本缓存是否带线宽数据。老缓存（v5 之前）全是 -1，打开开关也不会变粗，
    # 按钮要显式告诉用户"无数据"，否则会以为开关失效。
    has = bool(layers) or any((len(r) > 6 and int(r[6]) > 0) for r in (data.get('lines') or ()))
    return {'on': bool(on), 'lines': lines, 'layers': layers,
            'dwg_default': int(data.get('lwd') or 0), 'has': has}


# ---------------------------------------------------------------- 视图
def fit_of(box, w, h, pad=PAD):
    """世界框 → (k, ox, oy)：屏幕 x = wx*k+ox，屏幕 y = oy-wy*k。"""
    bw = max(1e-6, box[2] - box[0])
    bh = max(1e-6, box[3] - box[1])
    k = min((w - 2 * pad) / bw, (h - 2 * pad) / bh)
    ox = pad + ((w - 2 * pad) - bw * k) * 0.5 - box[0] * k
    oy = pad + ((h - 2 * pad) - bh * k) * 0.5 + box[3] * k
    return k, ox, oy


def _xe(s):
    return (str(s).replace('&', '&amp;').replace('<', '&lt;')
            .replace('>', '&gt;').replace('"', '&quot;'))


# ---------------------------------------------------------------- 场景 XML
def merge_iv(ivs, tol=1.5):
    """合并一维区间（像素坐标）：首尾相接或重叠的段并成一条，避免叠线/重复描边。"""
    ivs.sort()
    out = []
    for a, b in ivs:
        if out and a <= out[-1][1] + tol:
            if b > out[-1][1]:
                out[-1][1] = b
        else:
            out.append([a, b])
    return out


def scene_xml(sc, w, h, box, layer_off=(), text_min_px=4.0, lw=None):
    """显示列表 → <XScene>（屏幕像素坐标）。返回 (xml, 统计)。

    lw = {'on': bool, 'lines': [每线枚举], 'layers': {图层名: 枚举}}：CAD 线宽显示开关。
      on=False（默认）→ 全部 1px 细线；on=True → 按 AcDb::LineWeight 换算像素宽度。
      ByLayer(-1) 走图层值，ByBlock/Default(-2/-3) 按 1px。
    """
    lw_on = bool((lw or {}).get('on'))
    lw_lines = (lw or {}).get('lines') or []
    lw_layers = (lw or {}).get('layers') or {}
    lnames = sc.get('ly') or []

    def lwpx(i, li):
        if not lw_on:
            return 1.0
        v = lw_lines[i] if 0 <= i < len(lw_lines) else -1
        if v == -1:
            nm = lnames[li] if 0 <= li < len(lnames) else '0'
            v = int(lw_layers.get(nm, -3))
        if v < 0:
            return 1.0
        # 0.25mm(25) → 2.2px，0.35mm(35) → 2.8px，0.50mm(50) → 3.5px，2.11mm(211) → 8px（上限）
        return max(1.0, min(8.0, 1.0 + v * 0.05))

    k, ox, oy = fit_of(box, w, h)
    pal = sc['c']
    cnt = {'ln': 0, 'pl': 0, 'cr': 0, 'ar': 0, 'tx': 0, 'blk': 0, 'skip': 0}
    out = ['<XScene Width="%d" Height="%d">' % (w, h)]

    def px(x):
        return x * k + ox

    def py(y):
        return oy - y * k

    def on_screen(x0, y0, x1, y1, m=2.0):
        sx0, sx1 = sorted((px(x0), px(x1)))
        sy0, sy1 = sorted((py(y0), py(y1)))
        return not (sx1 < -m or sx0 > w + m or sy1 < -m or sy0 > h + m)

    def col(i):
        try:
            return pal[int(i)]
        except Exception:
            return '#E6E8EB'

    def line(x1, y1, x2, y2, c, wpx=1.0):
        out.append('<XLine X1="%.2f" Y1="%.2f" X2="%.2f" Y2="%.2f" Stroke="%s" Width="%.2f"/>'
                   % (px(x1), py(y1), px(x2), py(y2), c, wpx))
        cnt['ln'] += 1

    def rect(x0, y0, x1, y1, c, wpx=1.0):
        line(x0, y0, x1, y0, c, wpx)
        line(x1, y0, x1, y1, c, wpx)
        line(x1, y1, x0, y1, c, wpx)
        line(x0, y1, x0, y0, c, wpx)

    def text(x, y, s, hpx, rot_deg, ha, c, va=0):
        # 字高按屏幕像素判：低于阈值直接不画（HTML 版同口径）。
        # 曾经把字高抬到最小 4px，全图缩略时几千条标注糊成一片花花绿绿的噪点。
        if hpx < text_min_px or not s:
            cnt['skip'] += 1
            return
        anchor = {1: 'middle', 2: 'end', 4: 'middle'}.get(int(ha or 0), 'start')
        dy = {2: hpx * 0.5, 3: hpx}.get(int(va or 0), 0.0)
        out.append('<XText X="%.2f" Y="%.2f" Size="%.2f" Color="%s" Anchor="%s" Rotate="%.2f"'
                   ' Family="Microsoft YaHei">%s</XText>'
                   % (px(x), py(y) + dy, hpx, c, anchor, -float(rot_deg or 0), _xe(s)))
        cnt['tx'] += 1

    # ---- 顶层线：按屏幕像素行/列合并共线 ----
    # 缩略时多条不同标高/不同分段、但落在同一屏幕像素行上的线会叠成 4-6px 白带
    #（实测一行上最多压了 176 条线）。按像素行合并成一整条后仍是 1px，几何等价。
    LN = sc['ln']
    hseg = {}
    vseg = {}
    diag = []
    for i in range(0, len(LN), 6):
        x1, y1, x2, y2, ci, li = LN[i:i + 6]
        if li in layer_off:
            continue
        if not on_screen(min(x1, x2), min(y1, y2), max(x1, x2), max(y1, y2)):
            continue
        c = col(ci)
        lwf = round(lwpx(i // 6, li), 2)     # 注意：不能叫 w —— w 是画布宽度，覆盖会让 on_screen 失效
        X1, Y1 = px(x1), py(y1)
        X2, Y2 = px(x2), py(y2)
        if abs(Y1 - Y2) < 0.6:
            hseg.setdefault((int(round((Y1 + Y2) * 0.5)), c, lwf), []).append(
                (min(X1, X2), max(X1, X2)))
        elif abs(X1 - X2) < 0.6:
            vseg.setdefault((int(round((X1 + X2) * 0.5)), c, lwf), []).append(
                (min(Y1, Y2), max(Y1, Y2)))
        else:
            diag.append((X1, Y1, X2, Y2, c))
    def seg(x1, y1, x2, y2, c, wpx=1.0):
        """屏幕坐标直接输出（合并后的线段已是像素坐标，不能再走 line() 的二次转换）。"""
        out.append('<XLine X1="%.2f" Y1="%.2f" X2="%.2f" Y2="%.2f" Stroke="%s" Width="%.2f"/>'
                   % (x1, y1, x2, y2, c, wpx))
        cnt['ln'] += 1

    for (yy, c, lwf), ivs in hseg.items():
        for a, b in merge_iv(ivs):
            seg(a, yy, b, yy, c, lwf)
    for (xx, c, lwf), ivs in vseg.items():
        for a, b in merge_iv(ivs):
            seg(xx, a, xx, b, c, lwf)
    for X1, Y1, X2, Y2, c in diag:
        seg(X1, Y1, X2, Y2, c)

    # ---- 多段线 ----
    for e in sc['pl']:
        if e.get('li') in layer_off:
            continue
        b = e['b']
        if not on_screen(b[0], b[1], b[2], b[3]):
            continue
        pts = e['p']
        s = ' '.join('%.2f,%.2f' % (px(pts[j]), py(pts[j + 1])) for j in range(0, len(pts), 2))
        out.append('<XPolyline Points="%s" Closed="%d" Stroke="%s" Width="%.2f"/>'
                   % (s, 1 if e.get('z') else 0, col(e['c']),
                      lwpx(-1, e.get('li', 0))))
        cnt['pl'] += 1

    # ---- 圆 ----
    CR = sc['cr']
    for i in range(0, len(CR), 5):
        cx, cy, r, ci, li = CR[i:i + 5]
        if li in layer_off or not on_screen(cx - r, cy - r, cx + r, cy + r):
            continue
        out.append('<XCircle Cx="%.2f" Cy="%.2f" R="%.2f" Stroke="%s" Width="%.2f"/>'
                   % (px(cx), py(cy), r * k, col(ci), lwpx(-1, li)))
        cnt['cr'] += 1

    # ---- 弧：CAD 角度逆时针、y 向上；屏幕 y 向下 → 角度取反并交换起止，方向仍为逆时针 ----
    AR = sc['ar']
    for i in range(0, len(AR), 7):
        cx, cy, r, a0, a1, ci, li = AR[i:i + 7]
        if li in layer_off or not on_screen(cx - r, cy - r, cx + r, cy + r):
            continue
        out.append('<XArc Cx="%.2f" Cy="%.2f" R="%.2f" Start="%.2f" End="%.2f" Stroke="%s" Width="%.2f"/>'
                   % (px(cx), py(cy), r * k, -a1, -a0, col(ci), lwpx(-1, li)))
        cnt['ar'] += 1

    # ---- 文字 ----
    for t in sc['tx']:
        x, y, hh, rot, ha, ci, s, li, va = (list(t) + [0] * 9)[:9]
        if li in layer_off:
            continue
        if not on_screen(x, y, x + abs(hh) * 6, y + abs(hh)):
            continue
        text(x, y, str(s).strip(), abs(hh) * k, rot, ha, col(ci), va)

    # ---- 块参照：按屏幕尺寸分档，小图只画外框，避免全图时展开上百万图元 ----
    defs = sc['blk']
    for e in sc['ins']:
        if e.get('li') in layer_off:
            continue
        b = e['b']
        if not on_screen(b[0], b[1], b[2], b[3]):
            cnt['skip'] += 1
            continue
        sx = (b[2] - b[0]) * k
        sy = (b[3] - b[1]) * k
        size = max(sx, sy)
        c = col(e['c'])
        d = defs.get(e['k'])
        # LOD：块定义图元少就展开（缩略时也能看到符号形状，HTML 版正是这样），
        # 图元太多才退回外框/小点，避免全图时展开成百万级图元。
        nprim = 0
        if d:
            nprim = (len(d.get('ln', [])) // 5 + len(d.get('pl', [])) + len(d.get('cr', [])) // 4
                     + len(d.get('ar', [])) // 6 + len(d.get('tx', [])))
        thin = size < 48.0
        if size < 4.0:
            out.append('<XRect X="%.2f" Y="%.2f" W="2.00" H="2.00" Fill="%s"/>'
                       % (px(b[0]), py(b[3]), c))
            cnt['blk'] += 1
            continue
        # 缩略时的像素密度上限：块在屏幕上只有十几像素却要塞进上百个图元，
        # 画出来就是一团粗绿块。按 size 给预算，超预算的只画外框，放大后自动恢复细节。
        budget = 2000 if not thin else 600
        if not d or nprim == 0 or nprim > budget:
            out.append('<XRect X="%.2f" Y="%.2f" W="%.2f" H="%.2f" Stroke="%s" Width="1.00"/>'
                       % (px(b[0]), py(b[3]), sx, sy, c))
            cnt['blk'] += 1
            continue
        # 坏框保护：块定义 bbox 估坏时外框能盖住半张图，这种不展开，只画外框
        if size > min(w, h) * 0.45:
            out.append('<XRect X="%.2f" Y="%.2f" W="%.2f" H="%.2f" Stroke="%s" Width="1.00"/>'
                       % (px(b[0]), py(b[3]), sx, sy, c))
            cnt['blk'] += 1
            continue
        mm = e.get('m')
        if not (isinstance(mm, (list, tuple)) and len(mm) == 6):
            mm = (e.get('sx') or 1.0, 0.0, 0.0, e.get('sy') or 1.0, e.get('x') or 0.0, e.get('y') or 0.0)
        m00, m01, m10, m11, atx, aty = mm
        cnt['blk'] += 1

        def wx(lx, ly):
            return m00 * lx + m01 * ly + atx

        def wy(lx, ly):
            return m10 * lx + m11 * ly + aty

        bl, bpl, bcr, bar = d.get('ln', []), d.get('pl', []), d.get('cr', []), d.get('ar', [])
        for j in range(0, len(bl), 5):
            x1, y1, x2, y2, ci = bl[j:j + 5]
            ax, ay, bx, by = wx(x1, y1), wy(x1, y1), wx(x2, y2), wy(x2, y2)
            if on_screen(min(ax, bx), min(ay, by), max(ax, bx), max(ay, by)):
                line(ax, ay, bx, by, col(ci))
        for e2 in bpl:
            pts = e2['p']
            # 必须先成对算出 (x,y) 再交错写入：旧写法把全部 x 拼在全部 y 前面，
            # 之后按 (wp[2i], wp[2i+1]) 取点等于 (x[i], x[i+1])，
            # 块内多段线全被画成跨屏斜线（截图里那把"扇形长斜线"就是这么来的）。
            wp = []
            for j in range(0, len(pts) - 1, 2):
                lx, ly = pts[j], pts[j + 1]
                wp.append(wx(lx, ly))
                wp.append(wy(lx, ly))
            if len(wp) < 4:
                continue
            xs, ys = wp[0::2], wp[1::2]
            if not on_screen(min(xs), min(ys), max(xs), max(ys)):
                continue
            s = ' '.join('%.2f,%.2f' % (px(wp[2 * i]), py(wp[2 * i + 1])) for i in range(len(xs)))
            out.append('<XPolyline Points="%s" Closed="%d" Stroke="%s" Width="1.00"/>'
                       % (s, 1 if e2.get('z') else 0, col(e2['c'])))
            cnt['pl'] += 1
        for j in range(0, len(bcr), 4):
            cx, cy, r, ci = bcr[j:j + 4]
            sc_ = math.hypot(m00, m10) or 1.0
            ccx, ccy = wx(cx, cy), wy(cx, cy)
            if on_screen(ccx - r, ccy - r, ccx + r, ccy + r):
                out.append('<XCircle Cx="%.2f" Cy="%.2f" R="%.2f" Stroke="%s" Width="1.00"/>'
                           % (px(ccx), py(ccy), max(0.6, r * k * sc_), col(ci)))
                cnt['cr'] += 1
        for j in range(0, len(bar), 6):
            cx, cy, r, a0, a1, ci = bar[j:j + 6]
            sc_ = math.hypot(m00, m10) or 1.0
            ccx, ccy = wx(cx, cy), wy(cx, cy)
            if on_screen(ccx - r, ccy - r, ccx + r, ccy + r):
                out.append('<XArc Cx="%.2f" Cy="%.2f" R="%.2f" Start="%.2f" End="%.2f"'
                           ' Stroke="%s" Width="1.00"/>'
                           % (px(ccx), py(ccy), max(0.6, r * k * sc_), -a1, -a0, col(ci)))
                cnt['ar'] += 1
        for t2 in d.get('tx', []):
            tx, ty = t2[0], t2[1]
            s = str(t2[6]).strip()
            if not s:
                continue
            qx, qy = wx(tx, ty), wy(tx, ty)
            if not on_screen(qx, qy, qx + abs(t2[2]) * 6, qy + abs(t2[2])):
                continue
            text(qx, qy, s, abs(t2[2]) * k * (math.hypot(m00, m10) or 1.0),
                 t2[3], t2[4], col(t2[5]), t2[8] if len(t2) > 8 else 0)

    out.append('</XScene>')
    return ''.join(out), cnt, (k, ox, oy)


# ---------------------------------------------------------------- 界面 XML
def shell_xml(fname, header, rows, status, title):
    return """<?xml version="1.0" encoding="utf-8"?>
<XWindow Title="%(title)s" Width="%(w)d" Height="%(h)d" Theme="dark" Background="#FF14181B">
  <!-- page 全部用 HTML 写法：div + style 里的 display:flex 与 100%% 宽高，
       尺寸随窗口走（QML 的 anchors.fill 已删除） -->
  <div Name="page" style="display:flex;flex-direction:column;width:100%%;height:100%%">

    <div Name="top" style="display:flex;flex-direction:row;align-items:center;gap:10px;height:40px;padding:0 12px;background:#22262A">
      <XLabel Name="lbTitle" Text="%(fname)s" Width="360" Height="24" ForeColor="#FFE6EDF3"
              FontSize="12" FontWeight="Bold" TextAlign="Left"/>
      <XLabel Name="lbInfo" Text="%(header)s" Grow="1" Height="24"
              ForeColor="#FF8A94A6" FontSize="10" TextAlign="Left"/>
      <XButton Name="btAnno" Text="批注层" Width="70" Height="26" Background="#FF2B3036"
               ForeColor="#FFE6EDF3"/>
      <XButton Name="btNext" Text="下一幅" Width="70" Height="26" Background="#FF2B3036"
               ForeColor="#FFE6EDF3"/>
      <XButton Name="btFit" Text="全图" Width="62" Height="26" Background="#FF2B3036"
               ForeColor="#FFE6EDF3"/>
      <XButton Name="btZoomOut" Text="缩小" Width="62" Height="26" Background="#FF2B3036"
               ForeColor="#FFE6EDF3"/>
      <XButton Name="btZoomIn" Text="放大" Width="62" Height="26" Background="#FF2B3036"
               ForeColor="#FFE6EDF3"/>
    </div>

    <div Name="mid" style="display:flex;flex-direction:row;flex:1">
      <div Name="side" style="display:flex;flex-direction:column;gap:6px;width:232px;padding:8px;background:#1A1E22">
        <XLabel Name="lbLy" Text="图层 · 修改痕迹默认隐藏" Height="20" ForeColor="#FF6BA5FF"
                FontSize="10" FontWeight="Bold" TextAlign="Left"/>
        <XTableView Name="tblLayer" Columns="显&#9;名称&#9;图元" Rows="%(rows)s" Grow="1"
                    Background="#FF1A1E22" ForeColor="#FFC9D1D9" FontSize="10"/>
      </div>
      <XDwgCanvas Name="cvMain" Grow="1" Background="#FF14181B"/>
    </div>

    <div Name="sta" style="display:flex;flex-direction:row;align-items:center;gap:18px;height:26px;padding:0 12px;background:#22262A">
      <XLabel Name="stView" Text="%(status)s" Width="470" Height="20" ForeColor="#FFC9D1D9"
              FontSize="10" TextAlign="Left"/>
      <XLabel Name="stTip" Text="XML 场景 · 画布直扫 &lt;XScene&gt; 标签" Width="300" Height="20"
              ForeColor="#FF8A94A6" FontSize="10" TextAlign="Right"/>
      <XButton Name="btLW" Text="线宽 关" Width="84" Height="20" Background="#FF2B3036"
               ForeColor="#FFC9D1D9" FontSize="10"/>
    </div>
  </div>
</XWindow>
""" % {'w': WIN_W, 'h': WIN_H, 'ph': WIN_H - 46, 'title': _xe(title), 'fname': _xe(fname),
       'header': _xe(header), 'rows': rows, 'status': _xe(status)}


# ---------------------------------------------------------------- main
def main(argv):
    dwg = None
    view_idx = 0
    out_png = os.path.join(HERE, 'xml_view.png')
    i = 1
    while i < len(argv):
        if argv[i] == '--dwg':
            dwg = argv[i + 1]; i += 2
        elif argv[i] == '--view':
            view_idx = int(argv[i + 1]); i += 2
        elif argv[i] == '--out':
            out_png = argv[i + 1]; i += 2
        else:
            i += 1

    sc, cache_name, raw = load_scene(dwg)
    lwctx = lw_context(raw, on=('--lw' in argv or '--lw=1' in argv))
    ly, lyc, lycol = sc['ly'], sc['lyc'], sc.get('lycol') or []
    views = sc.get('v') or [sc['b']]
    box = views[min(view_idx, len(views) - 1)]
    st = {'box': list(box), 'off': set(sc.get('lyh') or ()),
          'vidx': min(view_idx, len(views) - 1)}

    # 图层表：显 + 名称 + 图元数（默认隐藏的用「·」标出）
    # 单元格必须是 &#9; 实体（属性值里的裸 Tab 会被 XML 规范成空格），所以这里预拼实体，
    # 整串不再做 XML 转义（否则 & 被二次转义成 &amp;，表格里就显示成字面 &#9;）。
    def layer_rows():
        out = []
        for i2, nm in enumerate(ly[:40]):
            out.append('%s&#9;%s&#9;%d' % ('·' if i2 in st['off'] else '✓', _xe(nm), lyc[i2]))
        return '\n'.join(out)

    rows_txt = layer_rows()
    import re
    shown = re.sub(r'_[0-9a-f]{6,}\.json$', '', cache_name)
    header = '图元 %s · 图层 %d · 视图 %d · 隐藏 %d 层' % (
        format(sc.get('n') or 0, ','), len(ly), len(views), len(st['off']))

    shell_path = os.path.join(HERE, 'xml_view_shell.xml')
    with open(shell_path, 'w', encoding='utf-8') as f:
        f.write(shell_xml(shown, header, rows_txt, '视图 %d/%d' % (view_idx + 1, len(views)),
                          'XML 看图 · %s' % shown))

    names = {}
    win = M.guiload(shell_path, context=globals(), names=names)
    win.show()
    M.pump(150)
    st['w'], st['h'] = win.get_size(names['cvMain'])
    print('canvas %dx%d' % (st['w'], st['h']))

    # ---- 一份场景 = 一个视图范围。缩放/平移就换视图范围重灌场景（HTML 版瓦片 zlevel 同思路）：
    #      文字是否够大、块是否展开，全部按「当前 k」重算，而不是在旧场景上硬拉伸。
    def render():
        b = st['box']
        txt2, cnt2, kk2 = scene_xml(sc, st['w'], st['h'], b, st['off'], lw=lwctx)
        win.load_dwg_xml(names['cvMain'], txt2)
        win.set_dwg_view(names['cvMain'], 1, 0, 0)
        win.set_text(names['stView'], '视图 %d/%d  ·  ×%.1f  ·  可见 %s 图元  ·  %d 块  ·  线宽 %s' % (
            st['vidx'] + 1, len(views), kk2[0] / max(1e-12, k_fit),
            format(cnt2['ln'] + cnt2['pl'] + cnt2['cr'] + cnt2['ar'] + cnt2['tx'], ','),
            cnt2['blk'], lw_label()))
        try:
            win.set_text(names['btLW'], '线宽 ' + lw_label())
        except Exception:
            pass
        # 图层表跟着隐藏集合实时刷新（行间 \n、格间 \t，底层契约）
        win.set_table_rows(names['tblLayer'], '\n'.join(
            '\t'.join([('·' if i3 in st['off'] else '✓'), nm, str(lyc[i3])])
            for i3, nm in enumerate(ly[:40])))
        return txt2, cnt2, kk2

    def zoom_about(f):
        b = st['box']
        cx, cy = (b[0] + b[2]) * 0.5, (b[1] + b[3]) * 0.5
        hw, hh = (b[2] - b[0]) * 0.5 * f, (b[3] - b[1]) * 0.5 * f
        st['box'] = [cx - hw, cy - hh, cx + hw, cy + hh]
        render()

    def fit():
        st['box'] = list(views[st['vidx']])
        render()

    def next_view():
        st['vidx'] = (st['vidx'] + 1) % len(views)
        st['box'] = list(views[st['vidx']])
        render()

    def toggle_anno():
        # 「批注层」只在有修改痕迹层时才有意义：一次全开、一次全关
        st['off'] = set() if st['off'] else set(sc.get('lyh') or ())
        render()

    def lw_label():
        # 三态：无数据（老缓存扫出来都是 ByLayer/Default 且图层也没线宽）/ 开 / 关
        if not lwctx.get('has'):
            return '无数据'
        return '开' if lwctx['on'] else '关'

    def toggle_lw():
        # CAD 的 LWDISPLAY：关掉全部 1px 细线（快、干净），打开按 AcDb::LineWeight 显示粗细
        if not lwctx.get('has'):
            # 缓存里没有线宽字段：切换也不会有任何粗细变化，直接说清楚，别让用户以为开关坏了
            try:
                win.set_text(names['stTip'], '此图缓存无线宽数据，请重新拖入 DWG 重扫')
            except Exception:
                pass
            return
        lwctx['on'] = not lwctx['on']
        render()

    win.on_click(names['btZoomIn'], lambda _id=None: zoom_about(1 / 1.6))
    win.on_click(names['btZoomOut'], lambda _id=None: zoom_about(1.6))
    win.on_click(names['btFit'], lambda _id=None: fit())
    win.on_click(names['btNext'], lambda _id=None: next_view())
    win.on_click(names['btAnno'], lambda _id=None: toggle_anno())
    win.on_click(names['btLW'], lambda _id=None: toggle_lw())

    k_fit = fit_of(box, st['w'], st['h'])[0]
    txt, cnt, kk = render()
    scene_path = os.path.join(HERE, 'xml_view_scene.xml')
    with open(scene_path, 'w', encoding='utf-8') as f:
        f.write(txt)
    print('scene xml %.1f KB · k=%.5f' % (len(txt) / 1024.0, kk[0]))
    print('  line=%d pline=%d circle=%d arc=%d text=%d block=%d skip=%d' %
          (cnt['ln'], cnt['pl'], cnt['cr'], cnt['ar'], cnt['tx'], cnt['blk'], cnt['skip']))

    M.pump(600)
    win.save_png(out_png)
    print('png', out_png)
    if any(a.startswith('--zoom') for a in argv):   # 连放大到能读清设备位号，验证 LOD 跟着 k 走
        times = 6
        for a in argv:
            if a.startswith('--zoom='):
                times = int(a.split('=')[1])
        for _ in range(times):
            zoom_about(1 / 1.6)
        M.pump(500)
        zoomed = out_png.replace('.png', '_zoom.png')
        win.save_png(zoomed)
        print('png', zoomed)
    # 窗口缩放：C++ 的 reflowXml 会按真实客户区重排 XML（page 用 Width/Height=100% 才会跟着长），
    # 但场景是按像素尺寸一次生成的，所以尺寸变了必须重灌一次场景，否则画布内容永远停在老尺寸。
    st['dirty'] = 0

    def on_resize(cw, ch):
        st['dirty'] = 8                     # 等尺寸稳定 8 帧（约 240ms）再重画，避免拖动时反复重生成

    try:
        win.on_resize(on_resize)
    except Exception as ex:
        print('onResize 不可用：', ex)

    M.pump(80)
    if '--batch' not in argv:               # 交互模式：进消息循环，直到关窗（--batch 截完就退）
        print('窗口已打开，可手动操作：全图 / 放大 / 缩小 / 下一幅 / 批注层；拖动窗口边框画布会跟着缩放')
        while M.pump(30):
            if st['dirty']:
                st['dirty'] -= 1
                if st['dirty'] == 0:
                    try:
                        cur = win.get_size(names['cvMain'])
                    except Exception:
                        break
                    if cur != (st['w'], st['h']):
                        st['w'], st['h'] = cur
                        k_fit = fit_of(st['box'], st['w'], st['h'])[0]
                        render()
                        win.set_dwg_view(names['cvMain'], 1, 0, 0)   # reflow 后 C++ 可能补一次 fit，再压回 1:1
                        print('resize -> canvas %dx%d' % cur)
    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv))
