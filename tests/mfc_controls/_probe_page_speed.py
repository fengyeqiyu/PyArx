# -*- coding: utf-8 -*-
"""_probe_page_speed.py — 量"切页一帧"的真实耗时（改前/改后对比用）。

做法：页容器 3 页，每页 6 个控件；连续切页 6 次，每次动画期间按 16ms 切片泵消息，
记录每一片的实际墙钟耗时。片耗时明显大于 16ms 就说明一帧的绘制成本超过了帧预算。
"""
import time

import PyMfcXaml as p
from demo_page_anim import build

names = {}
w = p.guiload(build(), names=names)
w.show()
p.pump(300)

sv = names['sv']
w.addPage(sv, [names['s1'], names['p1t'], names['p1b'], names['p1p']])
w.addPage(sv, [names['s2'], names['p2t'], names['p2s']])
w.addPage(sv, [names['s3'], names['p3t'], names['p3d']])
p.pump(800)

samples = []
for i in range(6):
    target = (i + 1) % 3
    t0 = time.perf_counter()
    w.animatePage(sv, target, 260)
    deadline = t0 + 0.30
    while time.perf_counter() < deadline:
        s = time.perf_counter()
        p.pump(16)
        samples.append((time.perf_counter() - s) * 1000.0)
    p.pump(120)


# ---- 对照组：不切页、只让第 1 页动画跑着，量同样的帧预算 ----
base = []
t_end = time.perf_counter() + 1.0
while time.perf_counter() < t_end:
    s = time.perf_counter()
    p.pump(16)
    base.append((time.perf_counter() - s) * 1000.0)
base.sort()
nb = len(base)
print('对照(不切页)=帧数%d 平均%.1fms 中位%.1fms 最慢%.1fms'
      % (nb, sum(base)/nb, base[nb//2], base[-1]), flush=True)
samples.sort()
n = len(samples)
avg = sum(samples) / n
p95 = samples[int(n * 0.95) - 1]
print('帧数=%d 平均=%.1fms 中位=%.1fms p95=%.1fms 最慢=%.1fms'
      % (n, avg, samples[n // 2], p95, samples[-1]), flush=True)
over = sum(1 for v in samples if v > 25)
print('超过 25ms 的帧：%d / %d' % (over, n), flush=True)
print('PAGE SPEED %s' % ('OK' if avg < 22 else 'SLOW'), flush=True)

p.pump(200)
w.close()
p.pump(200)
p.shutdown()
