# -*- coding: utf-8 -*-
"""Gi 瞬态图形可见性实证：往当前视图加一个瞬态，不擦除，供抓图确认。"""
LOG = r'G:\PyArx\log\cad_gi_visual.log'


def log(msg):
    with open(LOG, 'a', encoding='utf-8') as f:
        f.write(str(msg) + '\n')


from PyArxCore import Gi  # noqa: E402

log('--- visual run ---')


class Box(Gi.GiPythonDrawable):
    def worldDraw(self, wd):
        g = wd.geometry()
        g.circle((0, 0, 0), 60, (0, 0, 1))
        g.polyline([(-80, -80, 0), (80, -80, 0), (80, 80, 0), (-80, 80, 0)], None, -1)
        g.text((0, 70, 0), (0, 0, 1), (1, 0, 0), 20.0, 1.0, 0.0, 'AcGi OK')
        log('worldDraw drawn')
        return True

    def setAttributes(self, traits):
        traits.setTrueColor(Gi.AcCmEntityColor(255, 0, 0))
        return Gi.AcGiDrawableType.kGeometry

    def bounds(self):
        return (-100, -100, 0), (100, 100, 0)


box = Box()
log('add=%r' % Gi.addTransient(box, Gi.AcGiTransientDrawingMode.kAcGiMain, 0, None))
log('done, transient kept')
