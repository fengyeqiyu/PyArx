# -*- coding: utf-8 -*-
"""CSS flex 子项声明：flex-basis / flex-shrink / flex 简写。

为什么单独一个文件：t_flexpanel 量的是容器自己排的结果（wrap/justify/align-self），
这里量的是"子项的 CSS 声明有没有真的进到容器算法里"。此前 XmlCore 把 shrink 硬编码
成 1、basis 拿自然尺寸顶替，等于 flex-basis 写了也不生效——HTML 里 `flex:1 1 120px`
一行的事，XML 只能靠 Width 近似，做不到 basis 与 grow 分离。
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
    # ---- 1. flex-basis 属性覆盖 Width：basis 是主轴尺寸，跟 Width 不是一回事 ----
    xml = """<XWindow Name="root" Width="520" Height="200">
  <FlexBox Name="p1" Width="400" Height="100" gap="0">
    <XLabel Name="a" Width="60" flex-basis="120" Text="A"/>
    <XLabel Name="b" Width="60" Text="B"/>
  </FlexBox>
</XWindow>"""
    win, n = load(xml)
    a, b = rect(win, n, 'a'), rect(win, n, 'b')
    check(a[2] == 120, 'flex-basis=120 覆盖 Width=60', a)
    check(b[2] == 60, '未写 basis 的项仍用 Width=60', b)

    # ---- 2. style 里的 flex 简写：flex:1 吃掉剩余主轴 ----
    xml2 = """<XWindow Name="root" Width="520" Height="200">
  <FlexBox Name="p2" Width="400" Height="100" gap="0">
    <XLabel Name="a" Width="100" Text="A"/>
    <XLabel Name="b" Width="50" style="flex:1" Text="B"/>
  </FlexBox>
</XWindow>"""
    win2, n2 = load(xml2)
    a2, b2 = rect(win2, n2, 'a'), rect(win2, n2, 'b')
    check(a2[2] == 100, 'a 保持自然宽', a2)
    check(b2[2] == 300, 'flex:1 → basis 0 + grow 1 吃满剩余 300', b2)
    check(b2[0] == 100, 'b 紧跟在 a 之后 x=100', b2)

    # ---- 3. flex 简写的三分量：flex:2 0 auto ----
    xml3 = """<XWindow Name="root" Width="520" Height="200">
  <FlexBox Name="p3" Width="400" Height="100" gap="0">
    <XLabel Name="a" Width="100" Text="A"/>
    <XLabel Name="b" Width="50" style="flex:2 0 auto" Text="B"/>
  </FlexBox>
</XWindow>"""
    win3, n3 = load(xml3)
    b3 = rect(win3, n3, 'b')
    check(b3[2] == 300, 'flex:2 0 auto → basis auto(50) + grow2 独占剩余', b3)

    # ---- 4. 老写法不变：只写 Grow 的行为必须和以前一致 ----
    xml4 = """<XWindow Name="root" Width="520" Height="200">
  <FlexBox Name="p4" Width="400" Height="100" gap="0">
    <XLabel Name="a" Width="100" Text="A"/>
    <XLabel Name="b" Width="100" Grow="1" Text="B"/>
  </FlexBox>
</XWindow>"""
    win4, n4 = load(xml4)
    b4 = rect(win4, n4, 'b')
    check(b4[2] == 300, 'Grow="1" 老写法仍吃满剩余（回归红线）', b4)

    # ---- 5. style 任意百分比：width:50% 按容器内容盒换算（旧实现只认 100%） ----
    xml5 = """<XWindow Name="root" Width="520" Height="200">
  <FlexBox Name="p5" Width="400" Height="100" gap="0">
    <XLabel Name="a" style="width:50%" Text="A"/>
    <XLabel Name="b" style="width:25%" Text="B"/>
  </FlexBox>
</XWindow>"""
    win5, n5 = load(xml5)
    a5, b5 = rect(win5, n5, 'a'), rect(win5, n5, 'b')
    check(a5[2] == 200, 'width:50% → 200（容器 400）', a5)
    check(b5[2] == 100, 'width:25% → 100（容器 400）', b5)

    # ---- 6. height:50% 且 align 默认 stretch：显式尺寸不该被拉满 ----
    xml6 = """<XWindow Name="root" Width="520" Height="200">
  <FlexBox Name="p6" Width="400" Height="100" gap="0">
    <XLabel Name="a" style="height:50%" Text="A"/>
    <XLabel Name="b" Text="B"/>
  </FlexBox>
</XWindow>"""
    win6, n6 = load(xml6)
    a6, b6 = rect(win6, n6, 'a'), rect(win6, n6, 'b')
    check(a6[3] == 50, 'height:50% → 50（容器 100）', a6)
    check(b6[3] == 100, '未写高度的项仍被 stretch 拉满 100', b6)

    print('\nt_cssflex: PASS=%d FAIL=%d' % (CHECKS[0] - len(FAILS), len(FAILS)))
    if FAILS:
        for f in FAILS:
            print('  - ' + f)
    return 1 if FAILS else 0


if __name__ == '__main__':
    sys.exit(main())
