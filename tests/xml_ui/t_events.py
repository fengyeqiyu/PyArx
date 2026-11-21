# -*- coding: utf-8 -*-
"""事件：HTML 的 onclick（XML 声明）与 Python 的 child.click = fn 都必须真的触发。"""
import sys

from _kit import M, check, finish, click, ROOT

sys.path.insert(0, ROOT)
from MfcXmlRuntime import html  # noqa: E402

HITS = []


def on_xml_click(*a):
    HITS.append('xml')


def main():
    win = M.Window('t-events', 360, 200)
    ui = html('<div padding="12"><button id="a" onclick="on_xml_click">XML 绑定</button>'
              '<button id="b">Python 绑定</button></div>', app=win, w=300)
    win.show()
    M.pump(120)

    # 1) XML 声明的事件（走 guiload 的事件解释器 + 调用方作用域）
    base = len(HITS)
    click(win, ui.ids['a'])
    M.pump(60)
    check(len(HITS) == base + 1 and HITS[-1] == 'xml', 'onclick="fn" 触发', HITS)

    # 2) Python 侧 child.click = fn
    got = []
    ui.b.click = lambda comp, name: got.append(name)
    click(win, ui.ids['b'])
    M.pump(60)
    check(got == ['b'], 'child.click = fn 触发', got)

    # 3) 两个按钮各走各的，不串
    base = len(HITS)
    click(win, ui.ids['b'])
    M.pump(60)
    check(len(HITS) == base and got == ['b', 'b'], '互不串台', (HITS, got))

    # 4) HTML 层没有死绑定：整个加载过程零诊断
    check(M.event_warnings() == [], 'HTML 层无事件诊断', M.event_warnings())

    win.save_png(r'G:\PyArx\log\after\t_events.png')
    win.close()
    return finish('t_events')


if __name__ == '__main__':
    sys.exit(main())
