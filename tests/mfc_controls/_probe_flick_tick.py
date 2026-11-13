# -*- coding: utf-8 -*-
"""临时探针：抓 Flickable 惯性收敛的每帧偏移。"""
import ctypes
import PyMfcXaml as p
from demo_flickable import CONTENT_H, build
from _verify_flickable import drag, release, find_child_by_id

path = build()
names = {}
w = p.guiload(path, names=names)
fid = names['fl']
kids = [names['r%d' % i] for i in range(8)]
w.setFlickContent(fid, kids, 340, CONTENT_H)
w.show()
p.pump(300)
hv = find_child_by_id(w, fid)

# 惯性
w.setFlickOffset(fid, 0, 0)
p.pump(100)
drag(hv, [(120, 190), (120, 170), (120, 150), (120, 130), (120, 110), (120, 90)], 6)
print('release at', w.getFlickOffsetY(fid))
release(hv, (120, 90))
for i in range(22):
    p.pump(50)
    print(i * 50 + 50, w.getFlickOffsetY(fid))

# 顶部回弹
print('--- top bounce')
w.setFlickOffset(fid, 0, 0)
p.pump(100)
drag(hv, [(120, 60), (120, 95), (120, 130)], 10)
print('after drag', w.getFlickOffsetY(fid))
release(hv, (120, 130))
for i in range(12):
    p.pump(50)
    print(i * 50 + 50, w.getFlickOffsetY(fid))
w.close()
