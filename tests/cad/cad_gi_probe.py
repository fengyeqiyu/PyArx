# -*- coding: utf-8 -*-
"""Gi 子模块冒烟：对齐 acgi.h 后新增的类/方法是否真的可用，
并在 AutoCAD 里通过瞬态图形把 AcGiGeometry 的绘制接口跑一遍真实渲染。

由 PYRUNFILE 执行；结论写入 G:\\PyArx\\log\\cad_gi_probe.log。
"""
import traceback

LOG = r'G:\PyArx\log\cad_gi_probe.log'

_f = open(LOG, 'w', encoding='utf-8')


def log(msg):
    _f.write(str(msg) + '\n')
    _f.flush()


FAILS = []


def check(name, fn):
    try:
        r = fn()
        log('OK   %-42s %r' % (name, r))
        return r
    except Exception:
        FAILS.append(name)
        log('FAIL %-42s\n%s' % (name, traceback.format_exc()))
        return None


try:
    from PyArxCore import Gi
except Exception:
    log('import PyArxCore.Gi 失败\n' + traceback.format_exc())
    _f.close()
    raise

log('=== Gi 子模块导出清单 ===')
for n in sorted(dir(Gi)):
    if not n.startswith('_'):
        log('  %s' % n)

# ---------------------------------------------------------------- 值类型
check('AcCmEntityColor', lambda: Gi.AcCmEntityColor(200, 30, 30).rgb())
check('AcCmEntityColor.byLayer', lambda: Gi.AcCmEntityColor.byLayer().isByLayer())
check('AcCmTransparency', lambda: (Gi.AcCmTransparency(120).alpha(),
                                   Gi.AcCmTransparency(0.5).alphaPercent()))
check('AcGiVariant', lambda: (Gi.AcGiVariant(3).asInt(),
                              Gi.AcGiVariant('文本').asString()))
check('AcGiMapper', lambda: (Gi.AcGiMapper().projection(),
                             Gi.AcGiMapper().uTiling()))
check('AcGiPolyline', lambda: Gi.AcGiPolyline([(0, 0, 0), (10, 0, 0), (10, 10, 0)]).points())
check('AcGiPolyline.bulge', lambda: (lambda p: (p.setBulge([0.0, 1.0, 0.0]), p.hasBulges(), p.getBulgeAt(1)))(Gi.AcGiPolyline([(0, 0, 0), (10, 0, 0), (10, 10, 0)])))
check('AcGiPolyline.arcFlags', lambda: (lambda p: (p.setArcSegmentFlags([False, True, False]), p.arcSegmentFlags()))(Gi.AcGiPolyline([(0, 0, 0), (10, 0, 0), (10, 10, 0)])))
check('AcGiHatchPattern(nested dash)', lambda: Gi.AcGiHatchPattern([(0.0, 0.0, 0.0, 5.0, 5.0, [3.0, -3.0])]).patternLines())
check('AcGiHatchPattern(flat dash)', lambda: Gi.AcGiHatchPattern([(45.0, 0.0, 0.0, 4.0, 4.0, 2.0, -2.0)]).patternLines())
check('AcGiGradientFill', lambda: Gi.AcGiGradientFill(0.0, 0.0, True, [(255, 0, 0), (0, 0, 255)], 0).gradientColors())
check('AcGiImageBGRA32', lambda: (lambda np: Gi.AcGiImageBGRA32(np).width())(
    __import__('numpy').zeros((4, 6, 4), dtype='uint8')))

# ---------------------------------------------------------------- 枚举
for en in ('AcGiFillType', 'AcGiRegenType', 'AcGiArcType', 'AcGiImageOrg', 'AcGiVisibility',
           'AcGiDrawFlags', 'AcGiShadowFlags', 'AcGiSelectionFlags', 'AcGiHighlightProperty',
           'AcGiLineType', 'AcGiTransientDrawingMode', 'AcGiDrawableType', 'AcGiGradientType',
           'AcGiTransparencyMode'):
    check('enum ' + en, lambda en=en: getattr(Gi, en))

# ---------------------------------------------------------------- 瞬态绘制
class Demo(Gi.GiPythonDrawable):
    def worldDraw(self, wd):
        try:
            g = wd.geometry()
            g.circle((0, 0, 0), 50, (0, 0, 1))
            g.polyline([(0, 0, 0), (100, 0, 0), (100, 60, 0)], None, -1)
            g.worldLine((0, 0, 0), (80, 80, 0))
            g.text((10, 10, 0), (0, 0, 1), (1, 0, 0), 8.0, 1.0, 0.0, 'PyArx Gi')
            g.ellipticalArc((0, 0, 0), (0, 0, 1), 40.0, 20.0, 0.0, 3.14159, 0.0, Gi.AcGiArcType.kAcGiArcSimple)
            g.rowOfDots(5, (0, 0, 0), (10, 10, 0))
            log('   worldDraw 内 7 个图元调用完成')
            return True
        except Exception:
            log('   worldDraw 内失败\n' + traceback.format_exc())
            return False

    def setAttributes(self, traits):
        traits.setTrueColor(Gi.AcCmEntityColor(255, 0, 0))
        traits.setFillType(Gi.AcGiFillType.kAcGiFillAlways)
        traits.setTransparency(Gi.AcCmTransparency(0.5))
        traits.setDrawFlags(Gi.AcGiDrawFlags.kDrawBackfaces)
        return Gi.AcGiDrawableType.kGeometry

    def viewportDraw(self, vd):
        try:
            vp = vd.viewport()
            log('   viewport valid=%r perspective=%r' % (vp.valid(), vp.isPerspective()))
            vg = vd.geometry()
            vg.polylineDc([(10, 10, 0), (60, 60, 0)])
            log('   viewportDraw 完成')
        except Exception:
            log('   viewportDraw 失败\n' + traceback.format_exc())

    def bounds(self):
        return (0, 0, 0), (100, 100, 0)


demo = Demo()
check('transientManagerAvailable', lambda: Gi.transientManagerAvailable())
check('addTransient', lambda: Gi.addTransient(demo, Gi.AcGiTransientDrawingMode.kAcGiMain, 0, None))
check('updateTransient', lambda: Gi.updateTransient(demo, None))
check('eraseTransient', lambda: Gi.eraseTransient(demo, None))

log('=== FAILS: %d %s' % (len(FAILS), FAILS))
log('=== done ===')
_f.close()
