# -*- coding: utf-8 -*-
"""HTML 行内文本标签：<label>/<span>/<strong>… 宽按内容；<XLabel> 保持铺满（老行为）。

HTML 里 <label>运行状态</label> 只有字那么宽；XML 通道此前不写 Width 就被父容器
拉满成整行，等于每个标签都得手写 Width 才像 HTML —— 这正是"XML 明显更啰嗦"的典型。
"""
import os
import sys

ROOT = os.path.abspath(os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..'))
BIN = os.environ.get('PYARX_BIN') or os.path.join(ROOT, 'X64', 'Release')
sys.path.insert(0, BIN)
os.add_dll_directory(BIN)
os.environ.setdefault('PYARX_D2D_RT', 'software')

import PyMfcXml as M  # noqa: E402

FAILS = []
CHECKS = [0]


def check(cond, what, got=None):
    CHECKS[0] += 1
    print(('PASS ' if cond else 'FAIL ') + what + ('' if got is None else '  got=%r' % (got,)))
    if not cond:
        FAILS.append(what)
    return bool(cond)


def load(xml):
    names = {}
    win = M.guiload_text(xml, context=globals(), names=names)
    return win, names


def rect(win, names, key):
    return win.get_rect(names[key])


def main():
    xml = """<XWindow Name="root" Width="400" Height="200">
  <label Name="l1">运行状态</label>
  <span Name="l2">OK</span>
  <XLabel Name="l3" Text="X"/>
</XWindow>"""
    win, n = load(xml)
    l1, l2, l3 = rect(win, n, 'l1'), rect(win, n, 'l2'), rect(win, n, 'l3')
    check(l1[2] < 200, '<label> 中文按内容宽度（不铺满）', l1)
    check(l1[2] >= 40, '<label> 宽度够放下 4 个中文字', l1)
    check(l2[2] < 60, '<span> 短文本按内容宽度', l2)
    check(l3[2] == 384, '<XLabel> 仍铺满（老界面语义不变）', l3)
    try:
        win.close()
    except Exception:
        pass

    # 显式 Width 优先
    xml2 = """<XWindow Name="root" Width="400" Height="200">
  <label Name="l4" Width="220">标签</label>
</XWindow>"""
    win2, n2 = load(xml2)
    l4 = rect(win2, n2, 'l4')
    check(l4[2] == 220, '显式 Width 覆盖行内自然宽', l4)
    try:
        win2.close()
    except Exception:
        pass

    # flex 行里：label 不占满剩余主轴
    xml3 = """<XWindow Name="root" Width="520" Height="200">
  <FlexBox Name="p" Width="400" Height="100" gap="8">
    <label Name="l5">A</label>
    <label Name="l6">B</label>
  </FlexBox>
</XWindow>"""
    win3, n3 = load(xml3)
    l5, l6 = rect(win3, n3, 'l5'), rect(win3, n3, 'l6')
    check(l5[2] < 400, 'flex 行里 <label> 仍按内容宽', l5)
    check(l6[0] == l5[2] + 8, '第二个 label 紧接第一个（x=w+gap）', (l5, l6))
    try:
        win3.close()
    except Exception:
        pass

    print('\nt_inline_text: PASS=%d FAIL=%d' % (CHECKS[0] - len(FAILS), len(FAILS)))
    if FAILS:
        for f in FAILS:
            print('  - ' + f)
    return 1 if FAILS else 0


if __name__ == '__main__':
    sys.exit(main())
