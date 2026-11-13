# -*- coding: utf-8 -*-
"""_probe_page_frames.py — 切页动画期间连拍 4 帧 + 量紧泵一帧的真实成本。

紧泵（pump(1)）的切片≈单帧绘制成本本身，能看出这台机器上到底能跑多少帧；
连拍用 savePng（真屏幕抓图），用来确认没有残影/空洞/缺控件。
"""
import os
import time

import PyMfcXaml as p
from demo_page_anim import build, PageAnim

OUT = r'G:\PyArx\docs'
names = {}
w = p.guiload(build(), names=names)
w.show()
p.pump(300)
sv = names['sv']
w.addPage(sv, [names['s1'], names['p1t'], names['p1b'], names['p1p']])
w.addPage(sv, [names['s2'], names['p2t'], names['p2s']])
w.addPage(sv, [names['s3'], names['p3t'], names['p3d']])
pa = PageAnim(w, names)
pa.tick()
p.pump(1000)

# 紧泵成本：动画进行中，每片 1ms，看单帧绘制本身多少钱
tight = []
t_end = time.perf_counter() + 1.0
while time.perf_counter() < t_end:
    s = time.perf_counter()
    p.pump(1)
    tight.append((time.perf_counter() - s) * 1000.0)
tight.sort()
n = len(tight)
print('紧泵：样本%d 平均%.2fms 中位%.2fms p95%.2fms 最慢%.2fms → 上限约%.0f fps'
      % (n, sum(tight) / n, tight[n // 2], tight[int(n * .95) - 1], tight[-1],
         1000.0 / (sum(tight) / n)), flush=True)

# 连拍：切页过程中每 70ms 抓一帧
w.animatePage(sv, 1, 260)
for i in range(4):
    p.pump(70)
    ok = w.savePng(os.path.join(OUT, '_pgf_%d.png' % (i + 1)))
    print('frame %d saved=%s' % (i + 1, ok), flush=True)
p.pump(300)
w.savePng(os.path.join(OUT, '_pgf_end.png'))
print('saved _pgf_end.png', flush=True)
p.pump(200)
w.close()
p.pump(200)
p.shutdown()
