# -*- coding: utf-8 -*-
"""逐场景对照 =1 / =0 整窗帧：不一致像素数、比例、差异集中矩形。"""
import os
import sys

sys.path.insert(0, r'G:\PyArx\pylibs')
import numpy as np

DOC = r'G:\PyArx\docs'
SCENES = ['initial', 'winmove', 'split', 'pan', 'scroll', 'detail']
TH = 10


def analyze(a, b):
    if a.shape != b.shape:
        return None
    exact = (a != b).any(axis=2)
    diff = np.abs(a.astype(np.int16) - b.astype(np.int16))
    mask = (diff[:, :, 0] > TH) | (diff[:, :, 1] > TH) | (diff[:, :, 2] > TH)
    tot = int(mask.size)
    out = {'shape': a.shape, 'exact_n': int(exact.sum()), 'th_n': int(mask.sum()),
           'tot': tot, 'th_ratio': float(mask.sum()) / tot,
           'exact_ratio': float(exact.sum()) / tot}
    for name, m in (('th', mask), ('exact', exact)):
        ys, xs = np.where(m)
        if len(xs):
            out[name + '_bbox'] = (int(xs.min()), int(ys.min()),
                                   int(xs.max()), int(ys.max()))
        else:
            out[name + '_bbox'] = None
    return out


lines = ['scene, shape, exact_n, exact_ratio, th_n(>10), th_ratio, th_bbox, exact_bbox']
for sc in SCENES:
    pa = os.path.join(DOC, '_s2a_1_%s.npy' % sc)
    pb = os.path.join(DOC, '_s2a_0_%s.npy' % sc)
    a = np.load(pa)
    b = np.load(pb)
    r = analyze(a, b)
    print('== %s ==' % sc)
    print('  shape=%s exact=%d/%d=%.6f%%  th>10=%d=%.6f%%'
          % (r['shape'], r['exact_n'], r['tot'], 100 * r['exact_ratio'],
             r['th_n'], 100 * r['th_ratio']))
    print('  th bbox=%s  exact bbox=%s' % (r['th_bbox'], r['exact_bbox']))
    lines.append('%s, %s, %d, %.6f%%, %d, %.6f%%, %s, %s'
                 % (sc, r['shape'], r['exact_n'], 100 * r['exact_ratio'],
                    r['th_n'], 100 * r['th_ratio'], r['th_bbox'], r['exact_bbox']))

with open(os.path.join(DOC, '_s2a_scene_diff.txt'), 'w', encoding='utf-8') as f:
    f.write('\n'.join(lines) + '\n')
print('written _s2a_scene_diff.txt')
