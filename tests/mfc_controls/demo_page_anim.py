# -*- coding: utf-8 -*-
"""demo_page_anim.py — 页容器 + 每页一个"动画开关"（纯 Python 预览）。

语义：
  · 每页有自己的 Switch，打开才跑本页动画；
  · 切到别的页，离开那页的动画自动停（不留后台空转）；
  · 回到那页时开关还开着就自动续跑，关着就保持静止。

动画靠主循环每 1200ms 排一轮（A→B、B→A 各 600ms）；开关与页号靠轮询，
因为引擎目前没有"页切换"回调。
"""
import os
import tempfile
import time

import PyMfcXml as p

XML = '''<?xml version="1.0" encoding="utf-8"?>
<XWindow xmlns:x="http://schemas.microsoft.com/winfx/2006/xml" Title="页容器 + 动画开关" Width="600" Height="440">
  <XLayout x:Name="root">
    <XStackView x:Name="sv" Width="520" Height="330" Canvas.Left="20" Canvas.Top="20" />

    <!-- 第 1 页：宽度 + 进度 -->
    <XSwitch x:Name="s1" Width="96" Height="40" Value="0" Canvas.Left="40" Canvas.Top="52" />
    <XLabel x:Name="p1t" Text="第 1 页 · 动画已停止" Width="420" Height="24" Canvas.Left="150" Canvas.Top="64" />
    <XButton x:Name="p1b" Width="140" Height="42" Text="宽度动画" Canvas.Left="40" Canvas.Top="110" />
    <XProgressBar x:Name="p1p" Width="420" Height="18" Canvas.Left="40" Canvas.Top="170" Value="0" />

    <!-- 第 2 页：数值 -->
    <XSwitch x:Name="s2" Width="96" Height="40" Value="0" Canvas.Left="40" Canvas.Top="52" />
    <XLabel x:Name="p2t" Text="第 2 页 · 动画已停止" Width="420" Height="24" Canvas.Left="150" Canvas.Top="64" />
    <XSpinBox x:Name="p2s" Width="220" Height="34" Value="0" Minimum="0" Maximum="99" Canvas.Left="40" Canvas.Top="110" />

    <!-- 第 3 页：表盘 -->
    <XSwitch x:Name="s3" Width="96" Height="40" Value="0" Canvas.Left="40" Canvas.Top="52" />
    <XLabel x:Name="p3t" Text="第 3 页 · 动画已停止" Width="420" Height="24" Canvas.Left="150" Canvas.Top="64" />
    <XDial x:Name="p3d" Width="120" Height="120" Value="0" Minimum="0" Maximum="100" Canvas.Left="40" Canvas.Top="104" />
  </XLayout>
</XWindow>'''

PATH = os.path.join(tempfile.mkdtemp(prefix='pymfc_pganim_'), 'pa.xml')
SEG = 600           # 单程
CYC = 1200          # 一轮 ping-pong


def build():
    with open(PATH, 'w', encoding='utf-8') as f:
        f.write(XML)
    return PATH


class PageAnim:
    """把"开关 → 动画"的绑法收在一处，演示与验收共用。"""

    def __init__(self, w, names):
        self.w = w
        self.sv = names['sv']
        self.sw = [names['s1'], names['s2'], names['s3']]
        self.tgt = [names['p1b'], names['p2s'], names['p3d']]
        self.lbl = [names['p1t'], names['p2t'], names['p3t']]
        self.prog = names['p1p']
        self.on_text = ['第 %d 页 · 动画运行中' % (i + 1) for i in range(3)]
        self.off_text = ['第 %d 页 · 动画已停止' % (i + 1) for i in range(3)]
        self.want = [False, False, False]
        self.running = [False, False, False]
        self.page = 0
        self.t0 = time.time()

    def schedule(self, i):
        w = self.w
        if i == 0:
            w.animateFrom(self.tgt[0], 'Width', 140, 320, SEG, 'linear', 0, False)
            w.animateFrom(self.tgt[0], 'Width', 320, 140, SEG, 'linear', SEG, False)
            w.animateFrom(self.prog, 'Value', 0, 100, SEG, 'linear', 0, False)
            w.animateFrom(self.prog, 'Value', 100, 0, SEG, 'linear', SEG, False)
        elif i == 1:
            w.animateFrom(self.tgt[1], 'Value', 0, 99, SEG, 'outCubic', 0, False)
            w.animateFrom(self.tgt[1], 'Value', 99, 0, SEG, 'outCubic', SEG, False)
        else:
            w.animateFrom(self.tgt[2], 'Value', 0, 100, SEG, 'outCubic', 0, False)
            w.animateFrom(self.tgt[2], 'Value', 100, 0, SEG, 'outCubic', SEG, False)

    def start(self, i):
        if self.running[i]:
            return
        self.running[i] = True
        self.w.setText(self.lbl[i], self.on_text[i])
        self.schedule(i)

    def stop(self, i):
        if not self.running[i]:
            return
        self.running[i] = False
        self.w.stopAnimations(self.tgt[i])
        if i == 0:
            self.w.stopAnimations(self.prog)
        self.w.setText(self.lbl[i], self.off_text[i])

    def tick(self):
        """每帧调用：读开关、读页号、到点续排。"""
        for i in range(3):
            on = bool(self.w.getSwitch(self.sw[i]))
            if on != self.want[i]:
                self.want[i] = on
                if not on:
                    self.stop(i)
                elif i == self.page:
                    self.start(i)

        page = self.w.getViewIndex(self.sv)
        if page != self.page:
            self.stop(self.page)          # 离开的页先停，别在后台空转
            self.page = page
            if self.want[page]:
                self.start(page)          # 回来且开关还开着 → 续跑

        if time.time() - self.t0 > (CYC - 260) / 1000.0:
            for i in range(3):
                if self.running[i]:
                    self.schedule(i)
            self.t0 = time.time()


def main():
    names = {}
    w = p.guiload(build(), names=names)
    w.show()
    p.pump(300)
    w.addPage(names['sv'], [names['s1'], names['p1t'], names['p1b'], names['p1p']])
    w.addPage(names['sv'], [names['s2'], names['p2t'], names['p2s']])
    w.addPage(names['sv'], [names['s3'], names['p3t'], names['p3d']])
    pa = PageAnim(w, names)
    while True:
        # 消息泵粒度直接决定动画帧率：40ms 等于把上限锁在 25fps，切页看着就"发滞"。
        p.pump(10)
        pa.tick()


if __name__ == '__main__':
    main()
