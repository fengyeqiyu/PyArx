# -*- coding: utf-8 -*-
"""HTML 默认值表：<button>/<input> 不写尺寸时的默认几何。

HTML 里 <button>OK</button> 是个居中圆角小按钮（inline-block，宽由内容决定）；
XML 通道此前不写 Width 就会被父容器拉满成一条 384px 的长条。
这里只量几何（尺寸），外观默认态（圆角/配色/hover）另立条目。
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
  <button Name="b1" Text="OK"/>
  <XButton Name="b2" Text="OK"/>
  <input Name="i1" Text="I"/>
  <XTextField Name="i2" Text="I"/>
</XWindow>"""
    win, n = load(xml)
    b1, b2 = rect(win, n, 'b1'), rect(win, n, 'b2')
    i1, i2 = rect(win, n, 'i1'), rect(win, n, 'i2')
    check(b1[2] == 48, '<button> 不写 Width → 按内容宽（最小 48），不被父容器拉满', b1)
    check(b1[3] == 30, '<button> 不写 Height → 30', b1)
    check(b2[2] == 88 and b2[3] == 30, '<XButton> 同 HTML 默认', b2)
    # 第19轮：<input> 默认值改成 HTML 语义（size=20 字符宽、1.3 倍行高），
    # 14 号字 = 162 宽 / 20 高；老写法 <XTextField> 仍 160x26（见 t_html_input.py）
    check(i1[2] == 162, '<input> 不写 Width → 162', i1)
    check(i1[3] == 20, '<input> 不写 Height → 20', i1)
    check(i2[2] == 160 and i2[3] == 26, '<XTextField> 同 HTML 默认', i2)
    try:
        win.close()
    except Exception:
        pass

    # 显式尺寸必须优先于默认表
    xml2 = """<XWindow Name="root" Width="400" Height="200">
  <button Name="b3" Width="120" Height="40" Text="OK"/>
</XWindow>"""
    win2, n2 = load(xml2)
    b3 = rect(win2, n2, 'b3')
    check(b3[2] == 120 and b3[3] == 40, '显式 Width/Height 覆盖默认表', b3)
    try:
        win2.close()
    except Exception:
        pass

    # 在 flex 容器里也一样：button 不写尺寸不吃满剩余主轴
    xml3 = """<XWindow Name="root" Width="520" Height="200">
  <FlexBox Name="p" Width="400" Height="100" gap="0">
    <button Name="b4" Text="OK"/>
    <label Name="l4" Width="100" Text="L"/>
  </FlexBox>
</XWindow>"""
    win3, n3 = load(xml3)
    b4, l4 = rect(win3, n3, 'b4'), rect(win3, n3, 'l4')
    # HTML 里 <button> 宽度按内容（文字宽 + 左右 padding），"OK" 走最小宽 48
    check(b4[2] == 48, '<button>OK 在 flex 行里按内容宽（最小 48）', b4)
    check(l4[0] == 48, 'flex 行里 next 项紧接 button（x=48）', l4)
    try:
        win3.close()
    except Exception:
        pass

    # ---- button 默认宽度按内容：中文短文案 / 长文案 / style 定宽 / <XButton> 老写法 ----
    # 每个场景单独一行，避免 flex-shrink 压缩干扰断言
    xml4 = """<XWindow Name="root" Width="520" Height="260">
  <FlexBox Name="p1" Width="480" Height="60" gap="8">
    <button Name="a" >确定</button>
    <button Name="b" >取消</button>
  </FlexBox>
  <FlexBox Name="p2" Width="480" Height="60" gap="8" Top="60">
    <button Name="c" >应用全部修改到当前图层</button>
  </FlexBox>
  <FlexBox Name="p3" Width="480" Height="60" gap="8" Top="120">
    <button Name="d" style="width:120px">定宽</button>
  </FlexBox>
  <FlexBox Name="p4" Width="480" Height="60" gap="8" Top="180">
    <XButton Name="e" Text="老写法"/>
  </FlexBox>
</XWindow>"""
    win4, n4 = load(xml4)
    qa, qb, qc, qd, qe = (rect(win4, n4, k) for k in ('a', 'b', 'c', 'd', 'e'))
    check(qa[2] == 52 and qb[2] == 52, '<button>确定/取消 宽 52（2×14 + 20 + 4）', (qa, qb))
    check(qc[2] == 178, '<button>长文案按内容撑到 178', qc)
    check(qd[2] == 120, 'style="width:120px" 覆盖默认内容宽', qd)
    check(qe[2] == 88, '<XButton> 老写法仍 88，不被 HTML 默认值表影响', qe)
    try:
        win4.close()
    except Exception:
        pass

    print('\nt_html_defaults: PASS=%d FAIL=%d' % (CHECKS[0] - len(FAILS), len(FAILS)))
    if FAILS:
        for f in FAILS:
            print('  - ' + f)
    return 1 if FAILS else 0


if __name__ == '__main__':
    sys.exit(main())
