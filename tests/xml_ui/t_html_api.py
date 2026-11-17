# -*- coding: utf-8 -*-
"""HTML 风格入口：字符串 -> 控件。覆盖文本、尺寸、嵌套、自动 id、横排、多窗口不撞 id。"""
import sys

from _kit import M, check, finish, ROOT

sys.path.insert(0, ROOT)
from MfcXmlRuntime import html, DEFAULT_HEIGHT  # noqa: E402


def main():
    M.clear_diagnostics()
    win = M.Window('t-html', 420, 320)

    # 1) 基本：文本、数量、尺寸
    ui = html('<div padding="12">'
              '<label id="t">标题</label>'
              '<row><input id="n" placeholder="名称"/><button id="ok">确定</button></row>'
              '<hr id="sep"/><checkbox id="c" checked="1">启用</checkbox>'
              '</div>', app=win, w=360)
    win.show()
    M.pump(100)

    check(sorted(ui.ids) == ['c', 'n', 'ok', 'sep', 't'], '控件全建出来（4 个 + hr）', ui.ids)
    check(win.getXmlValue(ui.ids['t'], 'text') == '标题', '文本内容 -> Text',
          win.getXmlValue(ui.ids['t'], 'text'))
    check(win.getXmlValue(ui.ids['c'], 'text') == '启用', 'checkbox 文本')
    # Placeholder 没有 getXmlValue 通道（宿主只回 text/checked/index/value 等少量键），
    # 改从最终 XML 校验属性确实透传给了 XTextField 的 Placeholder 属性
    check('Placeholder="名称"' in ui.xml, 'placeholder 透传', ui.xml[:120])

    sz_ok = tuple(win.getSize(ui.ids['ok']))
    sz_n = tuple(win.getSize(ui.ids['n']))
    check(sz_ok[1] == DEFAULT_HEIGHT['XButton'], 'button 默认高 32', sz_ok)
    check(sz_n == (180, DEFAULT_HEIGHT['XTextField']), '横排里 input 用自然宽 180', sz_n)
    check(tuple(win.getSize(ui.ids['t'])) == (336, 22), '块级 label 撑满内容宽 336',
          tuple(win.getSize(ui.ids['t'])))
    check(tuple(win.getSize(ui.ids['sep'])) == (336, 1), 'hr 撑满且高 1',
          tuple(win.getSize(ui.ids['sep'])))

    # 2) 显式 width/height 压过默认
    ui2 = html('<div><button id="wide" width="200" height="48">宽</button></div>',
               app=win, w=360)
    M.pump(60)
    check(tuple(win.getSize(ui2.ids['wide'])) == (200, 48), '显式宽高压过默认',
          tuple(win.getSize(ui2.ids['wide'])))

    # 3) 自动 id：没写 id 也要能拿到
    ui3 = html('<div><label>一</label><label>二</label></div>', app=win, w=300)
    M.pump(60)
    check(sorted(ui3.ids) == ['el1', 'el2'], '自动 id 从 el1 起、容器不占号', ui3.ids)

    # 4) 两次创建 id 不相撞
    check(set(ui.ids).isdisjoint(ui2.ids), '两次 html() 的 id 不重叠',
          (ui.ids, ui2.ids))
    check(ui.ids['ok'] != ui2.ids['wide'], '具体 id 值不同', (ui.ids['ok'], ui2.ids['wide']))

    # 5) 未知标签要报错，且提示可用标签
    try:
        html('<div><xxxbad/></div>')
        check(False, '未知标签应抛 ValueError')
    except ValueError as e:
        check('<xxxbad>' in str(e) and 'button' in str(e), '未知标签报错并给出候选',
              str(e)[:80])

    # 6) 不传宿主：只出结构不建控件
    ui4 = html('<div><button id="b">x</button></div>', w=200)
    check(ui4.ids == {} and '<XButton' in ui4.xml, '不传宿主只算结构', ui4.xml[:60])

    win.savePng(r'G:\PyArx\log\after\t_html_api.png')
    win.close()
    return finish('t_html_api')


if __name__ == '__main__':
    sys.exit(main())

