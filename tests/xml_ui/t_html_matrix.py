# -*- coding: utf-8 -*-
"""HTML ↔ MFC 控件对照矩阵。

一条数据表把"HTML 写法"和"它落到哪个 MFC 控件、读回来是什么"钉在一起：
新增映射就往 MATRIX 里加一行，不用写用例代码。

每行：(标题, HTML 片段, 期望控件名前缀, 期望值字典)
期望值字典支持的键：
    count   该前缀的控件个数（默认 1）
    text    读回文本
    value   读回 Value（进度/滑块）
    checked 勾选状态（True/False）
    enabled 可用状态（True/False）
    items   下拉/列表框的当前选中序号
"""
import sys

from _kit import M, check, finish


MATRIX = [
    # ---- 文本 / 排版
    ('p 段落', '<p>正文</p>', 'label', {'text': '正文'}),
    ('h1 标题', '<h1>标题</h1>', 'label', {'text': '标题'}),
    ('span 行内', '<span>行内</span>', 'label', {'text': '行内'}),
    ('strong 强调', '<strong>加粗</strong>', 'label', {'text': '加粗'}),
    ('em 斜体', '<em>斜体</em>', 'label', {'text': '斜体'}),
    ('small 小号', '<small>小字</small>', 'label', {'text': '小字'}),
    ('code 代码块', '<code>x = 1</code>', 'label', {}),
    ('pre 预排版', '<pre>a\nb</pre>', 'label', {}),
    ('blockquote 引用', '<blockquote>引用</blockquote>', 'label', {'text': '引用'}),
    ('time 时间', '<time>10:00</time>', 'label', {'text': '10:00'}),
    ('kbd 键帽', '<kbd>Ctrl</kbd>', 'label', {'text': 'Ctrl'}),
    ('br 换行', '<p>上<br/>下</p>', 'label', {'count': 2}),

    # ---- 链接 / 媒体
    ('a 超链接', '<a href="https://example.com">链接</a>', 'label', {}),
    ('img 图片', '<img src="res/a.png" alt="图"/>', 'image', {}),
    ('hr 分隔线', '<hr/>', 'divider', {}),
    ('progress 进度', '<progress value="40" max="100"/>', 'progressbar', {'value': '40'}),
    ('meter 仪表', '<meter value="20" max="100"/>', 'progressbar', {'value': '20'}),
    ('canvas 画布', '<canvas width="120" height="80"></canvas>', 'qcanvas', {}),
    ('iframe 占位', '<iframe src="x"></iframe>', 'pane', {}),
    ('video 占位', '<video src="a.mp4"></video>', 'pane', {}),

    # ---- 按钮族
    ('button 按钮', '<button>确定</button>', 'button', {'text': '确定'}),
    ('input submit', '<input type="submit" value="提交"/>', 'button', {'text': '提交'}),
    ('input reset', '<input type="reset" value="重置"/>', 'button', {'text': '重置'}),
    ('input file', '<input type="file"/>', 'button', {}),
    ('input color', '<input type="color" value="#ff0000"/>', 'button', {'text': '#ff0000'}),

    # ---- 输入族
    ('input text', '<input type="text" value="abc"/>', 'textfield', {'text': 'abc'}),
    ('input search', '<input type="search" value="q"/>', 'textfield', {'text': 'q'}),
    ('input email', '<input type="email" value="a@b.c"/>', 'textfield', {'text': 'a@b.c'}),
    ('input url', '<input type="url" value="https://x"/>', 'textfield', {'text': 'https://x'}),
    ('input tel', '<input type="tel" value="123"/>', 'textfield', {'text': '123'}),
    ('input password', '<input type="password" value="p"/>', 'textfield', {}),
    ('input number', '<input type="number" value="7"/>', 'spinbox', {}),
    ('input range', '<input type="range" min="0" max="9" value="3"/>', 'slider', {'value': '3'}),
    ('input checkbox', '<input type="checkbox" checked="checked" value="记住"/>',
     'checkbox', {'checked': True}),
    ('input radio', '<input type="radio" name="g" checked="checked" value="甲"/>',
     'radio', {'checked': True, 'text': '甲'}),
    ('input date', '<input type="date" value="2026-01-02"/>', 'datepicker', {}),
    ('textarea 文本域', '<textarea rows="3">多行</textarea>', 'textarea', {}),
    ('select 下拉', '<select><option>甲</option><option selected="selected">乙</option></select>',
     'combobox', {'items': '1', 'text': '乙'}),
    ('select multiple', '<select multiple="multiple"><option>甲</option><option>乙</option>'
                        '</select>', 'listbox', {}),
    ('select optgroup', '<select><optgroup><option>甲</option><option selected="selected">乙'
                        '</option></optgroup></select>', 'combobox', {'text': '乙'}),
    ('ul 列表', '<ul><li>一</li><li>二</li></ul>', 'listbox', {}),

    # ---- 容器 / 结构
    ('fieldset 分组', '<fieldset><legend>组</legend><p>内容</p></fieldset>', 'group', {}),
    ('details 折叠', '<details><summary>展开</summary><p>内容</p></details>', 'group', {}),
    ('table 表格', '<table><tr><td>A</td><td>B</td></tr></table>', 'label', {'count': 2}),
]

PREFIX = {
    'textfield': 'textfield', 'button': 'button', 'label': 'label',
}


def load(xml):
    names = {}
    win = M.guiload_text('<div>' + xml + '</div>', context=globals(), names=names)
    win.show()
    M.pump(80)
    return win, names


def ids_of(names, prefix):
    return [cid for name, cid in names.items() if name.startswith(prefix)]


def read(win, cid, prop):
    try:
        return win.get_xml_value(cid, prop)
    except Exception:
        return None


def run_case(title, html, prefix, expect):
    win, names = load(html)
    try:
        ids = ids_of(names, prefix)
        want_count = expect.get('count', 1)
        if len(ids) != want_count:
            return False, 'want %d 个 %s, got %r' % (want_count, prefix, list(names))
        if not ids:
            return False, list(names)
        cid = ids[0]
        if 'text' in expect and read(win, cid, 'text') != expect['text']:
            return False, 'text=%r' % (read(win, cid, 'text'),)
        if 'value' in expect and str(read(win, cid, 'value')) != expect['value']:
            return False, 'value=%r' % (read(win, cid, 'value'),)
        if 'checked' in expect:
            got = str(read(win, cid, 'checked')).lower()
            if got != ('true' if expect['checked'] else 'false'):
                return False, 'checked=%r' % (got,)
        if 'items' in expect and str(read(win, cid, 'curSel')) != expect['items']:
            return False, 'curSel=%r' % (read(win, cid, 'curSel'),)
        if 'enabled' in expect:
            got = str(read(win, cid, 'enabled')).lower() in ('true', '1')
            if got != expect['enabled']:
                return False, 'enabled=%r' % (got,)
        return True, '%s <- %s' % (prefix, title)
    finally:
        win.close()


def main():
    for title, html, prefix, expect in MATRIX:
        try:
            ok, got = run_case(title, html, prefix, expect)
        except Exception as exc:
            ok, got = False, '%s: %s' % (type(exc).__name__, exc)
        check(ok, 'HTML %s -> MFC %s' % (title, prefix), got)
    return finish('t_html_matrix')


if __name__ == '__main__':
    sys.exit(main())
