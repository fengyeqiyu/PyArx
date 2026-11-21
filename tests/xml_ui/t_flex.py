# -*- coding: utf-8 -*-
"""CSS flex 布局测试（Display="flex"）：只量矩形，不开窗口。

HTML 之所以"方便"，是因为排版靠 flex：Gap 定间距、Justify 定主轴分布、
Align/Self 定交叉轴对齐、Grow 按权重吃掉剩余空间。这套语义现在已经
在 XML 里（同样写属性），本文件逐条量真实矩形验证。
"""
import os
import sys
import tempfile

from _kit import M, check, finish

W, H = 400, 200


def layout(xml, w=W, h=H):
    path = os.path.join(tempfile.gettempdir(), 'pymfc_flex_case.xml')
    with open(path, 'w', encoding='utf-8') as f:
        f.write(xml)
    return M.xml_layout(path, w, h)


def box(rects, name):
    return rects.get(name)


def main():
    # ---- 1. row + Gap：主轴只受间距影响 ----
    r = layout("""<XWindow Name="root" Width="400" Height="200">
  <XStackPanel Name="row" Display="flex" Flex="row" Gap="10" Width="320" Height="60">
    <XButton Name="a" Width="100" Height="40"/>
    <XButton Name="b" Width="100" Height="40"/>
    <XButton Name="c" Width="100" Height="40"/>
  </XStackPanel>
</XWindow>""")
    check(box(r, 'a')[0] == 0, 'flex row 首项 x=0', box(r, 'a'))
    check(box(r, 'b')[0] == 110, 'flex Gap=10 递推第二项 x=110', box(r, 'b'))
    check(box(r, 'c')[0] == 220, 'flex Gap=10 递推第三项 x=220', box(r, 'c'))
    check(box(r, 'a')[1] == 0 and box(r, 'b')[1] == 0, 'flex row 同一行 y 相同',
          (box(r, 'a')[1], box(r, 'b')[1]))

    # ---- 2. Justify 主轴分布 ----
    for word, exp in (('between', (0, 150, 300)), ('center', (40, 150, 260)),
                      ('end', (80, 190, 300))):
        r = layout("""<XWindow Name="root" Width="400" Height="200">
  <XStackPanel Name="row" Display="flex" Flex="row" Gap="10" Justify="%s" Width="400" Height="60">
    <XButton Name="a" Width="100" Height="40"/>
    <XButton Name="b" Width="100" Height="40"/>
    <XButton Name="c" Width="100" Height="40"/>
  </XStackPanel>
</XWindow>""" % word)
        got = (box(r, 'a')[0], box(r, 'b')[0], box(r, 'c')[0])
        check(got == exp, 'flex Justify=%s 三项 x=%s' % (word, exp), got)

    # ---- 3. Align / Self 交叉轴 ----
    r = layout("""<XWindow Name="root" Width="400" Height="200">
  <XStackPanel Name="row" Display="flex" Flex="row" Align="center" Width="400" Height="100">
    <XButton Name="a" Width="80" Height="30"/>
  </XStackPanel>
</XWindow>""")
    check(box(r, 'a')[1] == 35 and box(r, 'a')[3] == 30, 'flex Align=center 垂直居中 y=35',
          box(r, 'a'))

    r = layout("""<XWindow Name="root" Width="400" Height="200">
  <XStackPanel Name="row" Display="flex" Flex="row" Width="400" Height="100">
    <XButton Name="a" Width="80" Height="30"/>
    <XButton Name="b" Width="80" Height="30" Self="end"/>
  </XStackPanel>
</XWindow>""")
    check(box(r, 'a')[1] == 0, 'flex 默认 Align=stretch/start 贴顶', box(r, 'a'))
    check(box(r, 'b')[1] == 70, 'flex Self=end 贴底 y=70', box(r, 'b'))

    # ---- 4. Grow 按权重分配剩余主轴空间 ----
    r = layout("""<XWindow Name="root" Width="400" Height="200">
  <XStackPanel Name="row" Display="flex" Flex="row" Width="400" Height="60">
    <XButton Name="a" Width="100" Grow="1"/>
    <XButton Name="b" Width="100" Grow="2"/>
  </XStackPanel>
</XWindow>""")
    wa, wb = box(r, 'a')[2], box(r, 'b')[2]
    check(wa + wb == 400, 'flex Grow 吃满主轴宽度（和 = 400）', (wa, wb))
    # 两个子项各 100，剩余 200 按 1:2 分配 → 差约 200/3 ≈ 67
    check(abs((wb - wa) - 67) <= 2, 'flex Grow=1:2 分掉剩余 200（宽差约 67）', (wa, wb))

    # ---- 5. column 方向 ----
    r = layout("""<XWindow Name="root" Width="400" Height="200">
  <XStackPanel Name="col" Display="flex" Flex="column" Gap="8" Width="200" Height="200">
    <XButton Name="a" Height="40"/>
    <XButton Name="b" Height="40"/>
  </XStackPanel>
</XWindow>""")
    check(box(r, 'a')[0] == 0 and box(r, 'b')[0] == 0, 'flex column 两项 x 相同（同列）',
          (box(r, 'a')[0], box(r, 'b')[0]))
    check(box(r, 'b')[1] == 48, 'flex column Gap=8 第二项 y=48', box(r, 'b'))

    # ---- 6. 反向行 ----
    r = layout("""<XWindow Name="root" Width="400" Height="200">
  <XStackPanel Name="row" Display="flex" Flex="row-reverse" Width="400" Height="60">
    <XButton Name="a" Width="50"/>
    <XButton Name="b" Width="50"/>
  </XStackPanel>
</XWindow>""")
    check(box(r, 'a')[0] == 350 and box(r, 'b')[0] == 300, 'flex row-reverse 从右往左排',
          (box(r, 'a')[0], box(r, 'b')[0]))

    # ---- 7. div 就是 flex 容器（class="row"）----
    r = layout("""<div Name="root" Width="400" Height="80">
  <div Name="bar" class="row" Gap="6" Height="40">
    <span Name="t1">甲</span>
    <span Name="t2">乙</span>
  </div>
</div>""", 400, 80)
    check(box(r, 't1')[0] < box(r, 't2')[0] and box(r, 't1')[1] == box(r, 't2')[1],
          'div class="row" 自动横排（flex row）',
          (box(r, 't1'), box(r, 't2')))

    # ---- 8. 不写 Display 的容器保持老排版（回归）----
    r = layout("""<XWindow Name="root" Width="400" Height="200">
  <XStackPanel Name="col" Width="400" Height="120">
    <XButton Name="a" Height="40"/>
    <XButton Name="b" Height="40"/>
  </XStackPanel>
</XWindow>""")
    check(box(r, 'a')[1] == 0 and box(r, 'b')[1] == 40,
          '未声明 flex 的 StackPanel 仍按老规则竖排', (box(r, 'a'), box(r, 'b')))

    # ---- 9. Margin 参与主轴排布 ----
    r = layout("""<XWindow Name="root" Width="400" Height="200">
  <XStackPanel Name="row" Display="flex" Flex="row" Gap="5" Width="400" Height="60">
    <XButton Name="a" Width="100" Margin="10"/>
    <XButton Name="b" Width="100"/>
  </XStackPanel>
</XWindow>""")
    check(box(r, 'a')[0] == 10, 'flex Margin 左 10 影响首项 x=10', box(r, 'a'))
    check(box(r, 'b')[0] == 10 + 100 + 10 + 5, 'flex 第二项 = 左margin+宽+右margin+Gap',
          box(r, 'b'))

    # ---- 10. 容器不写主轴尺寸时按内容撑开（width:auto）----
    r = layout("""<XWindow Name="root" Width="400" Height="200">
  <XStackPanel Name="row" Display="flex" Flex="row" Justify="end" Width="400" Height="60">
    <XStackPanel Name="tools" Display="flex" Flex="row" Gap="8" Height="32">
      <XButton Name="a" Width="76"/>
      <XButton Name="b" Width="76"/>
    </XStackPanel>
  </XStackPanel>
</XWindow>""")
    check(box(r, 'tools')[2] == 160,
          'row 容器的自然宽 = 子项宽之和 + Gap（160）', box(r, 'tools'))
    check(box(r, 'a')[0] == 240 and box(r, 'b')[0] == 240 + 76 + 8,
          'Justify=end 下 tools 贴右，(a,b) 不溢出窗口', (box(r, 'a'), box(r, 'b')))

    return finish('flex layout')


if __name__ == '__main__':
    sys.exit(main())
