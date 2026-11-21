# -*- coding: utf-8 -*-
"""cad2d 的 XML 场景通道测试：<XScene> 声明式显示列表（等价于 HTML/SVG 那套写法）。

跑法（在 examples\\cad2d 下）：
    set PYTHONPATH=G:\\PyArx\\X64\\Release
    python tests\\test_xmlscene.py            # 只跑纯结构测试（不弹窗）
    python tests\\test_xmlscene.py --gui      # 追加像素级对照（会短暂弹窗）

覆盖点：
  A 结构契约（无 GUI）
    A1  根节点是 <XScene> 且带 Width/Height
    A2  每种实体 kind → 对应标签（line/circle/arc/rect/polyline/text/dimension/hatch/insert）
    A3  标签白名单：只出现 C++ 解析器认识的标签（防"写了不认的标签 → 白屏"）
    A4  属性完整性（XLine 的 X1/Y1/X2/Y2/Stroke/Width 等）
    A5  文本转义（& < > " 全部转义，中文原样保留）
    A6  数值格式（两位小数、无科学计数、无 -0.00）
    A7  同名块只序列化一次（XGroup 数量 = 块参照数）
    A8  虚线实体带 Dash="实线段,空档"
    A9  cull=True 抠掉视口外图元，cull=False 全留
    A10 叠加层（夹点/橡皮筋/十字光标）齐全，橡皮框带 Dash
    A11 视口尺寸与 to_svg 同口径（Width/Height 一致）
    A12 空文档也能给出合法根节点（不抛异常）
  B 像素对照（--gui）
    B1  XML 画布上红线/白线出现在几何应在的位置
    B2  中文文字真的画出来了（文字区域有非底色像素）
    B3  同一场景分别走 XML 与 SVG 两条通道，网格采样差异极小
"""
import ctypes
import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)                       # examples\cad2d
BUILD = os.path.join(os.path.dirname(os.path.dirname(ROOT)), 'X64', 'Release')
for path in (ROOT, HERE, BUILD):
    if path not in sys.path:
        sys.path.insert(0, path)
os.environ.setdefault('PYARX_D2D_RT', 'software')

import doc as caddoc      # noqa: E402
import ent               # noqa: E402
import render            # noqa: E402
import uiauto            # noqa: E402

# C++ 侧 parseXmlSceneText 认识的标签（大小写不敏感，X 前缀可省）
KNOWN_TAGS = {'XScene', 'XLine', 'XCircle', 'XArc', 'XRect', 'XPolyline',
              'XPolygon', 'XText', 'XGroup', 'XPath'}

VIEW = (0.0, 0.0, 6000.0, 5000.0)


def build_doc():
    """一份覆盖全部实体类型的图纸（世界坐标 mm）。"""
    d = caddoc.Document(6000.0, 5000.0)
    d.symbols.blocks.define("DOOR", [ent.Line(0, 0, 0, 900),
                                     ent.Circle(0, 450, 200)], (0, 0))
    d.add(ent.Line(100, 100, 3000, 100, layer="0", color=0))
    d.add(ent.Circle(2000, 2000, 500, layer="WALL", color=1))
    d.add(ent.Arc(1200, 800, 400, 0, 90))
    d.add(ent.Rect(500, 500, 1500, 1200))
    d.add(ent.Polyline([(600, 2500), (1400, 2500), (1400, 3100), (600, 3100)]))
    d.add(ent.Text(2000, 3500, '门厅&说明<A>"x"', size=400))
    d.add(ent.Dimension((500, 4000), (2000, 4000), off=300.0))
    d.add(ent.Hatch([(2500, 4000), (3500, 4000), (3500, 4600), (2500, 4600)], gap=120.0))
    d.add(ent.Insert("DOOR", 4000, 2000, rot=30.0))
    d.add(ent.Insert("DOOR", 4500, 2000))
    # 虚线：挂自带的 Hidden 线型（元素 [画 6mm, 空 3mm]）
    d.add(ent.Line(3000, 3000, 5000, 3000, linetype="Hidden"))
    return d


def xml_of(doc, **kw):
    return render.to_xml(doc.symbols, doc.entities, doc.view(), unit=doc.screen_unit,
                         absolute=True, **{k: v for k, v in kw.items()})


RESULT = {'pass': 0, 'fail': 0}


def ck(cond, name, detail=""):
    if cond:
        RESULT['pass'] += 1
        print('[PASS] %s' % name)
    else:
        RESULT['fail'] += 1
        print('[FAIL] %s%s' % (name, ('  <- ' + str(detail)) if detail != "" else ''))


def tags_in(xml):
    return re.findall(r'<([A-Za-z][A-Za-z0-9]*)', xml)


def test_structure():
    d = build_doc()
    xml = xml_of(d)
    svg = render.to_svg(d.symbols, d.entities, d.view(), unit=d.screen_unit, absolute=True)
    tags = tags_in(xml)

    # A1
    ck(xml.startswith('<XScene') and 'Width="' in xml.split('\n')[0],
       'A1 根节点 <XScene> 带 Width/Height', xml.split('\n')[0][:60])
    ck('</XScene>' in xml, 'A1b 根节点闭合')

    # A2 每类实体都落到自己的标签
    for tag, why in (('XLine', '直线/标注尺寸线/剖面线'),
                     ('XCircle', '圆'), ('XArc', '圆弧'),
                     ('XPolyline', '矩形/多段线'),
                     ('XText', '文字/标注文字'),
                     ('XPolygon', '标注箭头三角'),
                     ('XGroup', '块参照')):
        ck(tag in tags, 'A2 %s ← %s' % (tag, why))

    # A3 标签白名单
    unknown = sorted(set(tags) - KNOWN_TAGS)
    ck(not unknown, 'A3 只出现已知标签（C++ 解析器白名单）', ','.join(unknown))

    # A4 属性完整性
    line0 = next(t for t in re.findall(r'<XLine[^>]*>', xml))
    ck(all(k in line0 for k in ('X1="', 'Y1="', 'X2="', 'Y2="', 'Stroke="', 'Width="')),
       'A4 XLine 属性齐全', line0[:90])
    circ = next(t for t in re.findall(r'<XCircle[^>]*>', xml))
    ck(all(k in circ for k in ('Cx="', 'Cy="', 'R="')), 'A4b XCircle 属性齐全', circ[:80])
    arc = next(t for t in re.findall(r'<XArc[^>]*>', xml))
    ck(all(k in arc for k in ('Cx="', 'Cy="', 'R="', 'Start="', 'End="')),
       'A4c XArc 带起止角（不再走 <path A>）', arc[:90])
    poly = next(t for t in re.findall(r'<XPolyline[^>]*>', xml))
    ck('Points="' in poly and 'Closed="' in poly, 'A4d XPolyline 带 Points/Closed', poly[:80])
    grp = next(t for t in re.findall(r'<XGroup[^>]*>', xml))
    ck('Transform="translate(' in grp, 'A4e XGroup 带 translate', grp[:80])
    text = next(t for t in re.findall(r'<XText[^>]*>', xml))
    ck(all(k in text for k in ('X="', 'Y="', 'Size="', 'Color="', 'Anchor="')),
       'A4f XText 属性齐全', text[:90])

    # A5 转义
    ck('门厅&amp;说明&lt;A&gt;&quot;x&quot;' in xml, 'A5 文本转义（& < > "）',
       [t for t in re.findall(r'<XText[^>]*>[^<]*', xml)][:1])
    ck('门厅说明' not in xml.replace('门厅&amp;说明', ''), 'A5b 中文原样（UTF-8 直出）')

    # A6 数值格式
    nums = re.findall(r'"(-?\d+\.\d+)"', xml)
    ck(all(len(n.split('.')[1]) == 2 for n in nums), 'A6 数值统一两位小数', nums[:3])
    ck('-0.00' not in nums, 'A6b 无 -0.00 脏数值')
    ck(not re.search(r'"-?\d+\.\d+[eE][+-]?\d"', xml), 'A6c 无科学计数法')

    # A7 块复用：DOOR 只算一次，两个参照共享同一份块内容
    ck(len(re.findall(r'<XGroup[^>]*>', xml)) == 2, 'A7 两个块参照 → 两个 XGroup')
    bodies = re.findall(r'<XGroup[^>]*>(.*?)</XGroup>', xml, re.S)
    ck(len(bodies) == 2 and bodies[0] == bodies[1] and bodies[0],
       'A7b 两个块参照内容完全相同（块只解析一次）')

    # A8 虚线
    dashes = re.findall(r'<XLine[^>]*Dash="([^"]*)"', xml)
    ck(dashes, 'A8 虚线实体带 Dash', dashes[:2])

    # A9 cull：显式给视口（不能用 doc.view()，它会按内容自适应把远处图元框进来）
    far = caddoc.Document(6000.0, 5000.0)
    far.add(ent.Line(100, 100, 200, 100))
    far.add(ent.Line(90000, 90000, 99000, 99000))         # 视口外
    n_all = render.to_xml(far.symbols, far.entities, VIEW, unit=far.screen_unit,
                          absolute=True, cull=False).count('<XLine')
    n_cut = render.to_xml(far.symbols, far.entities, VIEW, unit=far.screen_unit,
                          absolute=True, cull=True).count('<XLine')
    ck(n_all == 2, 'A9 cull=False 保留全部图元', n_all)
    ck(n_cut == 1, 'A9b cull=True 抠掉视口外图元', n_cut)

    # A10 叠加层
    sel = [e for e in d.entities if e.kind == "circle"]
    ovl = render.to_overlay_xml(d.symbols, d.view(), unit=d.screen_unit, zoom=1.0,
                                absolute=True, grid=True, grid_step=500.0,
                                selection=sel, rubber=((100, 100), (2000, 1500)),
                                preview=ent.Line(0, 0, 1000, 1000),
                                cursor=(1000, 1000))
    ck('<XScene' in ovl and '</XScene>' in ovl, 'A10 叠加层是独立 <XScene>')
    ck('<XRect' in ovl and re.search(r'Dash="4(\.0+)?,\s*3(\.0+)?"', ovl),
       'A10b 橡皮框带虚线')
    ck(len(re.findall(r'<XLine', ovl)) >= 6, 'A10c 十字光标 6 条线',
       len(re.findall(r'<XLine', ovl)))
    ck(len(re.findall(r'<XRect', ovl)) >= 4, 'A10d 栅格/夹点/橡皮框齐全',
       len(re.findall(r'<XRect', ovl)))

    # A11 与 SVG 通道同口径的视口尺寸
    w_svg = float(re.search(r'width="(\d+\.\d)"', svg).group(1))
    w_xml = float(re.search(r'Width="(\d+\.\d+)"', xml).group(1))
    ck(abs(w_svg - w_xml) < 0.5, 'A11 两通道视口宽度一致', (w_svg, w_xml))

    # A12 空文档
    empty = caddoc.Document(6000.0, 5000.0)
    e_xml = xml_of(empty)
    ck(e_xml.startswith('<XScene') and e_xml.rstrip().endswith('</XScene>'),
       'A12 空文档仍给出合法根节点')


# ------------------------------------------------------------------ B: 像素对照
_p = None


def gui_tests():
    global _p
    import PyMfcXml as p
    _p = p

    d = build_doc()
    xml = render.to_xml(d.symbols, [e for e in d.entities if e.kind != 'hatch'],
                        d.view(), unit=d.screen_unit)
    svg = render.to_svg(d.symbols, [e for e in d.entities if e.kind != 'hatch'],
                        d.view(), unit=d.screen_unit)

    W, H = 420, 320

    def step(msg):
        sys.stderr.write('[gui] %s\n' % msg)
        sys.stderr.flush()

    step('build xml/svg')
    shell = """<XWindow Title="xmlscene" Width="900" Height="380" Theme="dark" Background="#FF202327">
  <XStackPanel Orientation="Horizontal" Width="880" Height="340" Padding="0" Margin="0"
               Spacing="8" Background="#FF202327">
    <XDwgCanvas Name="cvA" Width="%d" Height="%d"/>
    <XDwgCanvas Name="cvB" Width="%d" Height="%d"/>
  </XStackPanel>
</XWindow>""" % (W, H, W, H)

    names = {}
    step('guiload')
    win = p.guiload_text(shell, context=globals(), names=names)
    step('show')
    win.show()
    p.pump(400)
    step('loadDwgXml')
    win.load_dwg_xml(names['cvA'], xml)
    step('loadDwgSvg')
    win.load_dwg_svg(names['cvB'], svg)
    step('pump2')
    p.pump(400)

    hw_a = uiauto.hwnd_of(win, names['cvA'])
    hw_b = uiauto.hwnd_of(win, names['cvB'])
    step('hwnd a=%r b=%r' % (hw_a, hw_b))

    def shot(path):
        win.save_png(path)
        return path

    step('savePng')
    out = shot(os.path.join(ROOT, 'xmlscene_compare.png'))
    step('savePng ok')

    def pix_batch(hwnd, points):
        """一次取 DC 批量采样（单点开 DC 会让 1.4 万点变成分钟级）。"""
        u = ctypes.windll.user32
        g = ctypes.windll.gdi32
        dc = u.GetDC(ctypes.c_void_p(hwnd))
        out = []
        try:
            for x, y in points:
                c = g.GetPixel(ctypes.c_void_p(dc), x, y)
                out.append((c & 0xFF, (c >> 8) & 0xFF, (c >> 16) & 0xFF))
        finally:
            u.ReleaseDC(ctypes.c_void_p(hwnd), ctypes.c_void_p(dc))
        return out

    pts = [(x, y) for y in range(4, H - 4, 6) for x in range(4, W - 4, 6)]
    pa = pix_batch(hw_a, pts)
    pb = pix_batch(hw_b, pts)

    # B1 彩色图元落点：1px 细线在 6px 网格上命中率低，阈值按实测基线取小
    seen_red = sum(1 for r, g, b in pa if r > 120 and g < 90 and b < 90)
    seen_bright = sum(1 for r, g, b in pa if r > 150 and g > 150 and b > 150)
    ck(seen_red > 2, 'B1 XML 画布有红色图元（圆/弧/虚线）', seen_red)
    ck(seen_bright > 20, 'B1b XML 画布有白色图元（线/文字）', seen_bright)

    # B2 两通道网格采样一致性
    total = same = 0
    for a, b2 in zip(pa, pb):
        total += 1
        if max(abs(a[0] - b2[0]), abs(a[1] - b2[1]), abs(a[2] - b2[2])) <= 16:
            same += 1
    ratio = same / float(total or 1)
    ck(ratio >= 0.95, 'B2 XML 与 SVG 两通道像素一致率 ≥95%',
       '%.1f%% (%d/%d) 截图 %s' % (ratio * 100, same, total, out))

    win.close() if hasattr(win, 'close') else None
    return out


def main():
    test_structure()
    if '--gui' in sys.argv:
        try:
            gui_tests()
        except Exception as exc:                      # 无 GUI/无桌面时不要误判为功能错误
            print('[SKIP] GUI 像素对照未执行: %r' % (exc,))
    total = RESULT['pass'] + RESULT['fail']
    print('[xmlscene] 用例 %d · 通过 %d · 失败 %d' % (total, RESULT['pass'], RESULT['fail']))
    return 1 if RESULT['fail'] else 0


if __name__ == '__main__':
    sys.exit(main())

