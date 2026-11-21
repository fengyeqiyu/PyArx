# -*- coding: utf-8 -*-
"""HTML 元素全量对照表：MDN 元素表里每个标签都能落到一个 PyArx 控件（或明确不渲染）。

一行一个元素：(标题, HTML 片段, 期望控件名前缀, 期望个数)。
前缀写 None 表示"这个标签不产生控件"（<head>/<meta>/<col>/<script> 这类结构标签）。
"""
import os
import sys

from _kit import M, check, finish, rect_of


ELEMENTS = [
    # ---- 文档级（不渲染）
    ('html 文档根', '<html><body><p>文</p></body></html>', 'label', 1),
    ('body', '<body><p>文</p></body>', 'label', 1),
    ('head 丢弃', '<head><title>t</title><meta charset="utf-8"/></head>', None, 0),
    ('base 丢弃', '<base href="x"/>', None, 0),
    ('link 丢弃', '<link rel="icon" href="i.png"/>', None, 0),
    ('meta 丢弃', '<meta charset="utf-8"/>', None, 0),
    ('style 丢弃', '<style>p{color:red}</style>', None, 0),
    ('script 丢弃', '<script>var a=1</script>', None, 0),
    ('noscript 丢弃', '<noscript>无脚本</noscript>', None, 0),
    ('template 丢弃', '<template><p>x</p></template>', None, 0),
    ('slot 丢弃', '<slot/>', None, 0),
    ('title', '<title>标题</title>', 'label', 1),

    # ---- 区块
    ('div', '<div><p>文</p></div>', 'label', 1),
    ('section', '<section><p>文</p></section>', 'label', 1),
    ('article', '<article><p>文</p></article>', 'label', 1),
    ('aside', '<aside><p>文</p></aside>', 'label', 1),
    ('header', '<header><p>文</p></header>', 'label', 1),
    ('footer', '<footer><p>文</p></footer>', 'label', 1),
    ('nav', '<nav><p>文</p></nav>', 'label', 1),
    ('main', '<main><p>文</p></main>', 'label', 1),
    ('figure', '<figure><p>图</p></figure>', 'label', 1),
    ('figcaption', '<figcaption>说明</figcaption>', 'label', 1),
    ('center', '<center><p>居中</p></center>', 'label', 1),
    ('marquee', '<marquee><p>滚动</p></marquee>', 'label', 1),

    # ---- 内容分组
    ('p', '<p>段落</p>', 'label', 1),
    ('h1', '<h1>标题</h1>', 'label', 1),
    ('h2', '<h2>二级</h2>', 'label', 1),
    ('h3', '<h3>三级</h3>', 'label', 1),
    ('h4', '<h4>四级</h4>', 'label', 1),
    ('h5', '<h5>五级</h5>', 'label', 1),
    ('h6', '<h6>六级</h6>', 'label', 1),
    ('hr', '<hr/>', 'divider', 1),
    (('pre', '<pre>a\nb</pre>', 'label', 1)),
    ('blockquote', '<blockquote>引用</blockquote>', 'label', 1),
    ('address', '<address>地址</address>', 'label', 1),
    ('ol', '<ol><li>一</li></ol>', 'listbox', 1),
    ('ul', '<ul><li>一</li></ul>', 'listbox', 1),
    ('li 孤立', '<li>孤项</li>', 'label', 1),
    ('dl', '<dl><dt>术语</dt><dd>解释</dd></dl>', 'label', 2),
    ('dt 孤立', '<dt>术语</dt>', 'label', 1),
    ('dd 孤立', '<dd>解释</dd>', 'label', 1),
    ('menu', '<menu><li>项</li></menu>', 'label', 1),

    # ---- 表格
    ('table', '<table><tr><td>A</td><th>B</th></tr></table>', 'label', 2),
    ('caption 孤立', '<caption>表头</caption>', 'label', 1),
    ('thead 孤立', '<thead><tr><td>A</td></tr></thead>', 'label', 1),
    ('tbody 孤立', '<tbody><tr><td>A</td></tr></tbody>', 'label', 1),
    ('tfoot 孤立', '<tfoot><tr><td>A</td></tr></tfoot>', 'label', 1),
    ('tr 孤立', '<tr><td>A</td></tr>', 'label', 1),
    ('td 孤立', '<td>A</td>', 'label', 1),
    ('th 孤立', '<th>A</th>', 'label', 1),
    ('colgroup 丢弃', '<colgroup><col/></colgroup>', None, 0),
    ('col 丢弃', '<col/>', None, 0),

    # ---- 表单
    ('form', '<form><p>表单</p></form>', 'label', 1),
    ('label', '<label>标签</label>', 'label', 1),
    ('input text', '<input type="text"/>', 'textfield', 1),
    ('textarea', '<textarea rows="2">内容</textarea>', 'textarea', 1),
    ('select', '<select><option>甲</option></select>', 'combobox', 1),
    ('option 孤立', '<option>甲</option>', 'label', 1),
    ('optgroup 孤立', '<optgroup><option>甲</option></optgroup>', 'label', 1),
    ('datalist 丢弃', '<datalist><option>甲</option></datalist>', None, 0),
    ('button', '<button>确定</button>', 'button', 1),
    ('fieldset', '<fieldset><legend>组</legend><p>内容</p></fieldset>', 'group', 1),
    ('legend 孤立', '<legend>组</legend>', 'label', 1),
    ('output', '<output>结果</output>', 'label', 1),
    ('progress', '<progress value="40" max="100"/>', 'progressbar', 1),
    ('meter', '<meter value="20" max="100"/>', 'progressbar', 1),

    # ---- 交互
    ('details', '<details><summary>展开</summary><p>内容</p></details>', 'group', 1),
    ('summary 孤立', '<summary>摘要</summary>', 'label', 1),
    ('dialog', '<dialog><p>对话框</p></dialog>', 'label', 1),

    # ---- 行内语义
    (('a', '<a href="https://x">链接</a>', 'label', 1)),
    ('span', '<span>行内</span>', 'label', 1),
    ('mark', '<mark>标记</mark>', 'label', 1),
    ('b', '<b>粗</b>', 'label', 1),
    ('i', '<i>斜</i>', 'label', 1),
    ('u', '<u>下划线</u>', 'label', 1),
    ('s', '<s>删除线</s>', 'label', 1),
    ('em', '<em>强调</em>', 'label', 1),
    ('strong', '<strong>强</strong>', 'label', 1),
    ('small', '<small>小</small>', 'label', 1),
    ('sub', '<sub>下标</sub>', 'label', 1),
    ('sup', '<sup>上标</sup>', 'label', 1),
    (('code', '<code>x=1</code>', 'label', 1)),
    ('kbd', '<kbd>Ctrl</kbd>', 'label', 1),
    ('samp', '<samp>样例</samp>', 'label', 1),
    ('var', '<var>变量</var>', 'label', 1),
    ('q', '<q>引文</q>', 'label', 1),
    ('cite', '<cite>出处</cite>', 'label', 1),
    ('abbr', '<abbr>缩写</abbr>', 'label', 1),
    ('time', '<time>10:00</time>', 'label', 1),
    ('data', '<data value="1">一</data>', 'label', 1),
    ('bdi', '<bdi>文本</bdi>', 'label', 1),
    ('bdo', '<bdo>文本</bdo>', 'label', 1),
    ('del', '<del>删除</del>', 'label', 1),
    ('ins', '<ins>插入</ins>', 'label', 1),
    ('wbr', '<wbr/>', 'label', 1),
    ('br', '<br/>', 'label', 1),
    ('acronym', '<acronym>缩</acronym>', 'label', 1),
    ('big', '<big>大</big>', 'label', 1),
    ('tt', '<tt>等宽</tt>', 'label', 1),
    ('strike', '<strike>删除</strike>', 'label', 1),
    ('font', '<font>字体</font>', 'label', 1),
    ('nobr', '<nobr>不换行</nobr>', 'label', 1),

    # ---- 图片 / 媒体 / 嵌入
    ('img', '<img src="a.png" alt="图"/>', 'image', 1),
    ('picture', '<picture><source srcset="b.png"/><img src="a.png" alt="图"/></picture>',
     'image', 1),
    ('source 丢弃', '<source src="a.mp4"/>', None, 0),
    ('track 丢弃', '<track src="a.vtt"/>', None, 0),
    ('video', '<video src="a.mp4"></video>', 'pane', 1),
    ('audio', '<audio src="a.mp3"></audio>', 'pane', 1),
    ('canvas', '<canvas width="120" height="80"></canvas>', 'qcanvas', 1),
    ('svg', '<svg><circle r="4"/><rect/></svg>', 'qcanvas', 1),
    ('iframe', '<iframe src="x"></iframe>', 'pane', 1),
    ('embed', '<embed src="x"/>', 'pane', 1),
    ('object', '<object data="x"></object>', 'pane', 1),
    ('param 丢弃', '<param name="a" value="b"/>', None, 0),
    ('applet 丢弃', '<applet code="x"/>', None, 0),
    ('frameset 丢弃', '<frameset><frame/></frameset>', None, 0),
    ('frame 丢弃', '<frame/>', None, 0),
]

TYPES = [
    ('text', 'textfield'), ('search', 'textfield'), ('tel', 'textfield'),
    ('url', 'textfield'), ('email', 'textfield'), ('password', 'textfield'),
    ('date', 'datepicker'), ('datetime-local', 'datepicker'), ('datetime', 'datepicker'),
    ('month', 'datepicker'), ('week', 'datepicker'), ('time', 'textfield'),
    ('number', 'spinbox'), ('range', 'slider'), ('color', 'button'),
    ('checkbox', 'checkbox'), ('radio', 'radio'), ('file', 'button'),
    ('submit', 'button'), ('image', 'button'), ('reset', 'button'),
    ('button', 'button'),
]


def load(xml):
    names = {}
    win = M.guiload_text(xml, context=globals(), names=names)
    win.show()
    M.pump(60)
    return win, names


def run_element(title, html, prefix, count):
    win, names = load(html)
    try:
        if prefix is None:
            return (not names), names
        ids = [cid for name, cid in names.items() if name.startswith(prefix + '_')]
        return (len(ids) == count), 'want %d %s got %r' % (count, prefix, sorted(names))
    finally:
        win.close()


def run_type(title, ty, prefix):
    win, names = load('<input type="%s"/>' % ty)
    try:
        ids = [cid for name, cid in names.items() if name.startswith(prefix + '_')]
        return (len(ids) == 1), sorted(names)
    finally:
        win.close()


def run_props():
    """HTML 属性层：style 尺寸、hidden、select size、以及一堆"认得但不生效"的属性。"""
    ok_rect = True
    win, names = load('<input id="a" type="text" style="width:180px;height:32px"/>')
    try:
        cid = names.get('a')
        r = rect_of(win, cid) if cid else None
        ok_rect = bool(r) and r[2] == 180 and r[3] == 32
    finally:
        win.close()
    check(ok_rect, 'style 里的 width/height 变成真实尺寸', r)

    win, names = load('<p id="h" hidden="hidden">隐</p>')
    try:
        cid = names.get('h')
        vis = win.get_xml_value(cid, 'visible') if cid else None
        ok = str(vis).lower() in ('false', '0')
    finally:
        win.close()
    check(ok, 'hidden 属性 -> Visible=False', vis)

    win, names = load('<select size="4"><option>甲</option><option>乙</option></select>')
    try:
        ok = any(n.startswith('listbox_') for n in names)
    finally:
        win.close()
    check(ok, 'select size>1 -> 列表框', sorted(names))

    win, names = load('<div style="padding:10;gap:6"><p id="p1">A</p><p id="p2">B</p></div>')
    try:
        r1 = rect_of(win, names.get('p1'))
        r2 = rect_of(win, names.get('p2'))
        ok = bool(r1) and bool(r2) and r1[1] >= 10 and (r2[1] - r1[1]) >= 24 + 6
    finally:
        win.close()
    check(ok, 'style padding/gap 生效', (r1, r2))

    # 严格模式：HTML 里那些"我们不做"的属性必须被清干净，否则会以未知属性报错
    os.environ['PYARX_STRICT_XML'] = '1'
    try:
        def strict(xml):
            names2 = {}
            w = M.guiload_text(xml, context=globals(), names=names2)
            w.close()
            return names2

        n1 = strict('<form action="/x" method="post" novalidate="novalidate">'
                    '<label for="a">名</label>'
                    '<input id="a" name="a" type="text" required="required" pattern="\\d+"'
                    ' step="1" autocomplete="off" data-k="v" aria-label="名称"'
                    ' tabindex="1" draggable="true" spellcheck="false" lang="zh" dir="ltr"/>'
                    '</form>')
        # id="a" 会变成控件名，所以这里按"控件总数 = 标签 + 输入框"判
        check(len(n1) == 2 and 'a' in n1, '严格模式下 HTML 表单属性全部清干净', sorted(n1))
        n2 = strict('<img src="a.png" srcset="b.png 2x" sizes="100vw" alt="图"/>')
        check(any(n.startswith('image_') for n in n2), 'img 的 srcset/sizes 被忽略', sorted(n2))
        n3 = strict('<video controls="controls" loop="loop" muted="muted" poster="p.png"'
                    ' preload="none" src="a.mp4"></video>')
        check(any(n.startswith('pane_') for n in n3), 'video 的媒体属性被忽略', sorted(n3))
        n4 = strict('<input type="number" min="1" max="9" step="2" value="3"/>')
        check(any(n.startswith('spinbox_') for n in n4), 'number 的 step 被忽略', sorted(n4))
        n5 = strict('<div><button id="bb" Text="好"/><label id="ll">名</label></div>')
        check(len(n5) == 2, '默认外观标记不产生未知属性告警（严格模式）', sorted(n5))
    except Exception as exc:
        check(False, '严格模式下 HTML 属性清理', '%s: %s' % (type(exc).__name__, exc))
    finally:
        os.environ.pop('PYARX_STRICT_XML', None)


def _shot_colors(html, tag):
    """渲染一段 XML 并回统计颜色（默认外观类断言用像素，不靠属性回读）。"""
    from PIL import Image
    win, names = load(html)
    try:
        win.show()
        M.pump(220)
        png = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..', 'log',
                           '_t_elem_def_%s.png' % tag)
        win.save_png(png)
        return sorted(Image.open(png).convert('RGB').get_flattened_data()) if hasattr(
            Image.Image, 'get_flattened_data') else list(Image.open(png).convert('RGB').getdata())
    finally:
        win.close()


def _has(color_list, rgb):
    # 背景元素面积很小，按"颜色出现次数 >= 200"判，避免抗锯齿边缘误命中
    return color_list.count(rgb) >= 200


def run_defaults():
    """HTML 原生 <button>/<label> 的默认外观（UA 样式等价物）。

    这是"HTML 能做而 XML 做不到"的最典型一处：浏览器里 <button> 什么都不写就是
    浅灰底 + 灰框 + 圆角 + 居中，XML 早先什么都不写只剩几个字浮在背景上。
    """
    btn = ('<XWindow Width="260" Height="160">'
           '<button id="b" Text="确定" Left="10" Top="10" Width="100" Height="34"/></XWindow>')
    cols = _shot_colors(btn, 'btn')
    check(_has(cols, (239, 239, 239)), '<button> 不写样式也有 UA 浅灰底 #EFEFEF', 'miss')
    check(_has(cols, (118, 118, 118)), '<button> 不写样式也有 UA 灰框 #767676', 'miss')

    btn2 = ('<XWindow Width="260" Height="160">'
            '<button id="b" Text="确定" Left="10" Top="10" Width="100" Height="34"'
            ' style="background:#FF0000"/></XWindow>')
    cols2 = _shot_colors(btn2, 'btn2')
    check(_has(cols2, (255, 0, 0)), '<button> 显式背景覆盖 UA 默认', 'miss')
    check(not _has(cols2, (239, 239, 239)), '显式背景时不残留 UA 底色', 'left')

    old = ('<XWindow Width="260" Height="160">'
           '<XButton id="b" Text="确定" Left="10" Top="10" Width="100" Height="34"/></XWindow>')
    cols3 = _shot_colors(old, 'xbtn')
    check(not _has(cols3, (239, 239, 239)), '<XButton> 老写法不吃 UA 默认（行为不变）', 'changed')

    # <input> 的 UA 外观：白底 + 1px #767676 + 4px 圆角。只读回属性，不依赖像素位置。
    win, n = load('<XWindow Width="260" Height="160">'
                  '<input id="i" Left="10" Top="10" Width="150" Height="28"/></XWindow>')
    try:
        cid = n.get('i')
        got = (win.get_xml_value(cid, 'Background'), win.get_xml_value(cid, 'BorderThickness'),
               win.get_xml_value(cid, 'BorderColor'), win.get_xml_value(cid, 'Radius'))
    finally:
        win.close()
    check(got == ('#ffffff', '1', '#767676', '4'), '<input> 不写样式也有 UA 白底/灰框/圆角', got)

    win, n = load('<XWindow Width="260" Height="160">'
                  '<input id="i" Left="10" Top="10" Width="150" Height="28"'
                  ' style="background:#FF0000"/></XWindow>')
    try:
        cid = n.get('i')
        got2 = win.get_xml_value(cid, 'Background')
    finally:
        win.close()
    check(got2 == '#FF0000', '<input> 显式背景覆盖 UA 默认', got2)

    # 圆角盒子必须"四角真圆"：弧外那圈像素得是父底色，不能是方角的背色方块。
    # 这是整条圆角链路最容易退化的地方（描边圆了、填充仍是方的，浅色面板上像贴了块方纸）。
    from PIL import Image
    win, names = load('<XWindow Width="200" Height="120" Background="#2B3445">'
                      '<button id="b" Text="" Left="20" Top="20" Width="100" Height="40"/></XWindow>')
    try:
        win.show()
        M.pump(220)
        png = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..', 'log',
                           '_t_elem_corner.png')
        win.save_png(png)
        im = Image.open(png).convert('RGB')
        fill = (239, 239, 239)
        xs, ys = [], []
        for y in range(im.size[1]):
            for x in range(im.size[0]):
                if im.getpixel((x, y)) == fill:
                    xs.append(x)
                    ys.append(y)
        corner = im.getpixel((min(xs), min(ys))) if xs else None
    finally:
        win.close()
    check(corner is not None and corner != fill,
          '圆角背板四角不吃满（弧外露出父底色）', corner)

    # <select>/<textarea> 与 <input> 同族的 UA 外观：深色背景上不写样式也必须是白底灰框
    cols4 = _shot_colors('<XWindow Width="360" Height="220" Background="#2B3445">'
                         '<select id="s" Left="10" Top="10" Width="160" Height="30">'
                         '<option Text="A"/><option Text="B"/></select>'
                         '<textarea id="t" Left="10" Top="50" Width="200" Height="80">多行</textarea>'
                         '</XWindow>', 'selfield')
    check(_has(cols4, (255, 255, 255)) and _has(cols4, (118, 118, 118)),
          '<select>/<textarea> 不写样式也有 UA 白底灰框', 'white/border missing')

    # 行盒：①大字号的 <h1>/大字号 label 不被 24 裁掉；②开了折行的定宽 label 按行数长高
    long_text = '这是一段比较长的中文说明文字用来测试换行以后行盒是否会自动长高，浏览器里它会占三行左右。'
    hnm = {}
    hw = M.guiload_text('<XWindow Width="320" Height="260" Background="#FFFFFF">'
                        '<h1 id="h" Left="10" Top="10">一级标题</h1>'
                        '<label id="c" Left="10" Top="80" style="font-size:34px">大字</label>'
                        '<label id="w" Left="10" Top="150" Width="180" TextWrapping="Wrap">'
                        + long_text + '</label></XWindow>', context=globals(), names=hnm)
    try:
        hw.show()
        M.pump(220)
        rects = {k: hw.get_rect(hnm[k]) for k in ('h', 'c', 'w') if k in hnm}
    finally:
        hw.close()
    check(rects.get('h') and rects['h'][3] >= 44, '行盒：<h1> 32px 不被 24 裁掉', rects.get('h'))
    check(rects.get('c') and rects['c'][3] >= 47, '行盒：font-size:34px 的 label 跟着长高', rects.get('c'))
    check(rects.get('w') and rects['w'][3] >= 60, '折行：定宽 label 按行数长高', rects.get('w'))

    # CSS line-height：倍数 / 绝对 px 都要体现在行盒高
    lhn = {}
    lw = M.guiload_text('<XWindow Width="300" Height="200" Background="#FFFFFF">'
                        '<label id="m" Left="10" Top="10" Width="160" style="font-size:16px;line-height:2">倍数</label>'
                        '<label id="x" Left="10" Top="80" Width="160" style="font-size:16px;line-height:32px">绝对</label>'
                        '</XWindow>', context=globals(), names=lhn)
    try:
        lw.show()
        M.pump(200)
        lrect = {k: lw.get_rect(lhn[k]) for k in ('m', 'x')}
    finally:
        lw.close()
    check(lrect['m'][3] == 32, 'line-height:2 → 行盒 32（16×2）', lrect['m'])
    check(lrect['x'][3] == 32, 'line-height:32px → 行盒 32', lrect['x'])


def main():
    for title, html, prefix, count in ELEMENTS:
        try:
            ok, got = run_element(title, html, prefix, count)
        except Exception as exc:
            ok, got = False, '%s: %s' % (type(exc).__name__, exc)
        check(ok, '元素 %s' % title, got)
    for ty, prefix in TYPES:
        try:
            ok, got = run_type(ty, ty, prefix)
        except Exception as exc:
            ok, got = False, '%s: %s' % (type(exc).__name__, exc)
        check(ok, 'input type=%s' % ty, got)
    run_props()
    run_defaults()
    return finish('t_html_elements')


if __name__ == '__main__':
    sys.exit(main())

