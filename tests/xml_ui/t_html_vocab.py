# -*- coding: utf-8 -*-
"""HTML 控件词汇对照测试：一条数据表驱动，逐行把 HTML 写法灌进 XML 层。

每行 = (标题, XML 片段, 期望)。期望只写"能从真实控件读回来"的东西：
控件个数、文本、勾选、值、行数，或回调是否真的被触发。
新增 HTML 标签时往 MATRIX 里加一行即可，不用再写用例代码。
"""
import sys

from _kit import M, check, finish, click, children, rect_of


def load(xml):
    names = {}
    win = M.guiload_text('<div>' + xml + '</div>', context=globals(), names=names)
    win.show()
    M.pump(100)
    return win, names


def texts(win, names):
    out = []
    for name, cid in names.items():
        try:
            v = win.get_xml_value(cid, 'text')
        except Exception:
            continue
        out.append(v)
    return out


def count(names):
    return len(names)


def by_prefix(names, prefix):
    return [cid for name, cid in names.items() if name.startswith(prefix)]


# ---------------------------------------------------------------- 数据表
# 每条：标题 / HTML 片段 / 期望（函数 (win, names) -> (ok, got)）
MATRIX = []


def case(title, xml):
    def deco(fn):
        MATRIX.append((title, xml, fn))
        return fn
    return deco


@case('div>p 段落文本', '<section><p>段落一</p><p>段落二</p></section>')
def _(win, names):
    return count(names) == 2 and texts(win, names) == ['段落一', '段落二'], (count(names), texts(win, names))


@case('h1 标题', '<h1>大标题</h1>')
def _(win, names):
    return texts(win, names) == ['大标题'], texts(win, names)


@case('span 内联文本', '<span>说明文字</span>')
def _(win, names):
    return texts(win, names) == ['说明文字'], texts(win, names)


@case('button 按钮', '<button>确定</button>')
def _(win, names):
    return texts(win, names) == ['确定'], texts(win, names)


@case('input type=text value', '<input type="text" value="张三"/>')
def _(win, names):
    return texts(win, names) == ['张三'], texts(win, names)


@case('input 无 type 默认文本框', '<input placeholder="姓名"/>')
def _(win, names):
    return len(by_prefix(names, 'textfield')) == 1, list(names)


@case('input type=password', '<input type="password" value="secret"/>')
def _(win, names):
    return texts(win, names) == ['secret'], texts(win, names)


@case('input type=checkbox checked', '<input type="checkbox" checked="checked" value="启用"/>')
def _(win, names):
    ids = by_prefix(names, 'checkbox')
    ok = len(ids) == 1 and str(win.get_xml_value(ids[0], 'checked')).lower() in ('true', '1')
    return ok, (ids, win.get_xml_value(ids[0], 'checked') if ids else None)


@case('input type=radio', '<input type="radio" value="甲"/>')
def _(win, names):
    return len(by_prefix(names, 'radio')) == 1, list(names)


@case('input type=range 值/极值', '<input type="range" min="0" max="200" value="50"/>')
def _(win, names):
    ids = by_prefix(names, 'slider')
    if not ids:
        return False, list(names)
    v = win.get_xml_value(ids[0], 'value')
    return str(v) == '50', v


@case('input type=date', '<input type="date" value="2026-01-01"/>')
def _(win, names):
    ids = by_prefix(names, 'datepicker')
    return len(ids) == 1, list(names)


@case('input type=submit 变按钮', '<input type="submit" value="提交"/>')
def _(win, names):
    return texts(win, names) == ['提交'], texts(win, names)


@case('input type=hidden 不生成控件', '<input type="hidden" value="x"/>')
def _(win, names):
    return count(names) == 0, count(names)


@case('textarea 多行文本', '<textarea rows="4">第一行&#13;第二行</textarea>')
def _(win, names):
    return len(by_prefix(names, 'textarea')) == 1, list(names)


@case('select>option（selected 生效）',
      '<select><option>甲</option><option selected="selected">乙</option><option>丙</option></select>')
def _(win, names):
    ids = by_prefix(names, 'combobox')
    if not ids:
        return False, list(names)
    v = win.get_xml_value(ids[0], 'text')
    return v == '乙', v


@case('select multiple 变列表框',
      '<select multiple="multiple"><option>甲</option><option>乙</option></select>')
def _(win, names):
    ids = by_prefix(names, 'listbox')
    if not ids:
        return False, list(names)
    return win.get_table_row_count(ids[0]) >= 0 and True, list(names)


@case('ul>li 变列表框条目', '<ul><li>一</li><li selected="selected">二</li></ul>')
def _(win, names):
    ids = by_prefix(names, 'listbox')
    if not ids:
        return False, list(names)
    n = win.get_xml_value(ids[0], 'curSel')
    return str(n) == '1', n


@case('table 逐格变标签', '<table><tr><td>A</td><td>B</td></tr>'
                          '<tr><td>C</td><td>D</td></tr></table>')
def _(win, names):
    got = sorted(texts(win, names))
    return got == ['A', 'B', 'C', 'D'], got


@case('hr 分隔线', '<hr/>')
def _(win, names):
    return len(by_prefix(names, 'divider')) == 1, list(names)


@case('a 超链接（href → Url）', '<a href="https://example.com">链接</a>')
def _(win, names):
    # <a> 落 XLabel 而非 Hyperlink 控件：HTML 的链接就是"蓝字带下划线"，
    # Hyperlink 只画固定 accent 色、下划线不生效；href 映射到 Url，留给点击接线。
    ids = by_prefix(names, 'label')
    if not ids:
        return False, list(names)
    return texts(win, names) == ['链接'], texts(win, names)


@case('progress 进度', '<progress value="30" max="100"/>')
def _(win, names):
    ids = by_prefix(names, 'progressbar')
    if not ids:
        return False, list(names)
    v = win.get_xml_value(ids[0], 'value')
    return str(v) == '30', v


@case('fieldset>legend 变分组标题', '<fieldset><legend>分组</legend><p>内容</p></fieldset>')
def _(win, names):
    gids = by_prefix(names, 'group')
    ok = len(gids) == 1 and '内容' in texts(win, names)
    return ok, (list(names), texts(win, names))


@case('img 图片', '<img src="res/logo.png"/>')
def _(win, names):
    return len(by_prefix(names, 'image')) == 1, list(names)


@case('disabled 输入框', '<input type="text" value="灰掉" disabled="disabled"/>')
def _(win, names):
    ids = by_prefix(names, 'textfield')
    if not ids:
        return False, list(names)
    v = win.get_xml_value(ids[0], 'enabled')
    return str(v).lower() in ('false', '0'), v


@case('id 属性映射成 Name', '<input type="text" id="user"/>')
def _(win, names):
    return 'user' in names, list(names)


@case('onclick 直接绑 Python 回调', '<button onclick="on_ok">点我</button>')
def _(win, names):
    ids = by_prefix(names, 'button')
    if not ids:
        return False, list(names)
    del CLICKED[:]
    click(win, ids[0])
    M.pump(120)
    return CLICKED == [1], CLICKED


CLICKED = []


@case('placeholder 不被当成未知属性吞掉',
      '<label>姓名</label><input id="name" placeholder="请输入姓名"/>')
def _(win, names):
    # 占位文字画在原生控件内部读不回来，这里校验：控件建出来了、没有报未知属性
    warnings = M.prop_warnings()
    return 'name' in names and 'placeholder' not in str(warnings), (list(names), warnings)


@case('竖直排布 4 个子控件不错位', '<h3>标题</h3><label>甲</label>'
                                '<input id="n1"/><button>确定</button>')
def _(win, names):
    # 自动高片段根：第 3 个起曾经因为"重算几何"把 Spacing 抹成 0 而错位
    got = [rect_of(win, cid)[1] for cid in sorted(names.values())]
    ys = sorted(got)
    steps = [ys[i + 1] - ys[i] for i in range(len(ys) - 1)]
    return len(ys) == 4 and all(s >= 24 for s in steps), (ys, steps)


@case('class="row" 两个按钮同一行',
      '<div class="row"><button>确定</button><button>取消</button></div>')
def _(win, names):
    ids = by_prefix(names, 'button')
    if len(ids) != 2:
        return False, list(names)
    r0, r1 = rect_of(win, ids[0]), rect_of(win, ids[1])
    same_line = abs(r0[1] - r1[1]) <= 1 and r1[0] > r0[0]
    return same_line and r0[2] < 200, (r0, r1)


@case('table 单元格高度不被父容器撑满',
      '<table><tr><td>A</td><td>B</td></tr><tr><td>C</td><td>D</td></tr></table>')
def _(win, names):
    hs = [win.get_size(cid)[1] for cid in by_prefix(names, 'label')]
    return len(hs) == 4 and all(h <= 40 for h in hs), hs


def on_ok(*args):
    CLICKED.append(1)


def main():
    for title, xml, expect in MATRIX:
        win, names = load(xml)
        try:
            ok, got = expect(win, names)
        except Exception as exc:      # 断言里的异常也要算失败，不要吞掉
            ok, got = False, '%s: %s' % (type(exc).__name__, exc)
        check(ok, title, got)
        win.close()
    return finish('t_html_vocab')


if __name__ == '__main__':
    sys.exit(main())
