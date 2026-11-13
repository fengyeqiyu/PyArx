# -*- coding: utf-8 -*-
"""对比 =1 / =0 屏幕抓帧：不一致像素比例、包围盒、左右分区差异。"""
import os, sys
sys.path.insert(0, r'G:\PyArx\pylibs')
import numpy as np

on = np.load(r'G:\PyArx\docs\_diag_screen_on.npy')
off = np.load(r'G:\PyArx\docs\_diag_screen_off.npy')
print('on', on.shape, 'off', off.shape)
if on.shape != off.shape:
    h = min(on.shape[0], off.shape[0]); w = min(on.shape[1], off.shape[1])
    on = on[:h, :w]; off = off[:h, :w]

d = np.abs(on[:, :, :3].astype(np.int16) - off[:, :, :3].astype(np.int16))
for th in (10, 30, 60):
    mask = (d[:, :, 0] > th) | (d[:, :, 1] > th) | (d[:, :, 2] > th)
    print('th=%2d mismatch=%.4f%% count=%d' % (th, mask.mean() * 100, int(mask.sum())))

mask = (d[:, :, 0] > 30) | (d[:, :, 1] > 30) | (d[:, :, 2] > 30)
ys, xs = np.where(mask)
if len(xs):
    print('diff bbox x=(%d,%d) y=(%d,%d)' % (xs.min(), xs.max(), ys.min(), ys.max()))
    left = mask[:, :792]; right = mask[:, 792:]
    print('left(CV)  mismatch=%.4f%%' % (left.mean() * 100))
    print('right(tbl) mismatch=%.4f%%' % (right.mean() * 100))

# 每 10% 行/列差异密度，定位集中矩形
H, W = mask.shape
print('row bands:')
for i in range(10):
    y0, y1 = i * H // 10, (i + 1) * H // 10
    print('  y[%3d-%3d] %.3f%%' % (y0, y1, mask[y0:y1].mean() * 100))
print('col bands:')
for i in range(10):
    x0, x1 = i * W // 10, (i + 1) * W // 10
    print('  x[%3d-%3d] %.3f%%' % (x0, x1, mask[:, x0:x1].mean() * 100))

# 存差异可视化：红=差异，灰度=一致
vis = np.full((H, W, 3), 255, dtype=np.uint8)
gray = (off[:, :, :3].astype(np.int16).mean(axis=2)).astype(np.uint8)
vis[~mask] = np.repeat(gray[~mask, None], 3, axis=1)
vis[mask] = [255, 0, 0]
np.save(r'G:\PyArx\docs\_diag_diff_mask.npy', vis)
print('saved diff mask')
