# -*- coding: utf-8 -*-
"""第二轮补齐（AcGiContext / GDI ownerDraw / DrawStream / Material / Light /
VisualStyle / Environment / polyPolygon / EdgeData / FaceData / VertexData）的
运行期冒烟。由 PYRUNFILE 执行，结论写 G:\\PyArx\\log\\cad_gi_probe2.log。
"""
import traceback

LOG = r'G:\PyArx\log\cad_gi_probe2.log'
_f = open(LOG, 'w', encoding='utf-8')


def log(msg):
    _f.write(str(msg) + '\n')
    _f.flush()


FAILS = []


def check(name, fn):
    try:
        r = fn()
        log('OK   %-46s %r' % (name, r))
        return r
    except Exception:
        FAILS.append(name)
        log('FAIL %-46s\n%s' % (name, traceback.format_exc()))
        return None


from PyArxCore import Gi

log('=== 第二轮导出清单 ===')
for n in sorted(dir(Gi)):
    if not n.startswith('_'):
        log('  %s' % n)

# ---------------------------------------------------------------- mesh 数据容器
check('AcGiEdgeData.colors', lambda: (lambda d: (d.setColors([1, 2, 3, 4]), d.colors()))(Gi.AcGiEdgeData()))
check('AcGiEdgeData.trueColors', lambda: (lambda d: (
    d.setTrueColors([Gi.AcCmEntityColor(255, 0, 0), Gi.AcCmEntityColor(0, 0, 255)]),
    len(d.trueColors())))(Gi.AcGiEdgeData()))
check('AcGiEdgeData.visibility', lambda: (lambda d: (d.setVisibility([1, 0, 0, 1]), d.visibility()))(Gi.AcGiEdgeData()))
check('AcGiEdgeData.markers', lambda: (lambda d: (d.setSelectionMarkers([7, 8]), d.selectionMarkers()))(Gi.AcGiEdgeData()))
check('AcGiFaceData.normals', lambda: (lambda d: (
    d.setNormals([(0, 0, 1), (0, 0, 1)]), len(d.normals())))(Gi.AcGiFaceData()))
check('AcGiFaceData.transparency', lambda: (lambda d: (
    d.setTransparency([Gi.AcCmTransparency(120)]), len(d.transparency())))(Gi.AcGiFaceData()))
check('AcGiVertexData.orientation', lambda: (lambda d: (
    d.setOrientationFlag(int(Gi.AcGiOrientationType.kAcGiClockwise)),
    d.orientationFlag()))(Gi.AcGiVertexData()))
check('AcGiVertexData.mappingCoords', lambda: (lambda d: (
    d.setMappingCoords(int(Gi.AcGiMapChannel.kAllChannels), [(0, 0, 0), (1, 0, 0)]),
    len(d.mappingCoords(int(Gi.AcGiMapChannel.kAllChannels))))) (Gi.AcGiVertexData()))

# ---------------------------------------------------------------- 图形流
check('AcGiDrawStream', lambda: (lambda s: (s.isValid(), s.isPersistent(), s.drawableType()))(Gi.AcGiDrawStream()))
check('AcGiDrawStream.build', lambda: Gi.AcGiDrawStream.build([Gi.AcGiDrawStream(), Gi.AcGiDrawStream()], None))


# ---------------------------------------------------------------- GDI 自绘桥（本机回路）
class _Gdi(Gi.AcGiGdiDrawObject):
    def draw(self, hdc, x, y, w, h):
        log('   GDI draw 回调 hdc=%r 区域=%r' % (hdc, (x, y, w, h)))
        return True

    def width(self):
        return 64

    def height(self):
        return 32

    def addRef(self):
        log('   GDI addRef 回调')

    def delRef(self):
        log('   GDI delRef 回调')


check('AcGiGdiDrawObject.selfTest', lambda: _Gdi().selfTest())

# ---------------------------------------------------------------- 材质
check('AcGiMaterialColor', lambda: (lambda c: (
    c.setMethod(Gi.AcGiMaterialColorMethod.kOverride),
    c.setFactor(0.5),
    c.setColor(Gi.AcCmEntityColor(10, 200, 10)),
    c.method(), c.factor(), c.color()))(Gi.AcGiMaterialColor()))
check('AcGiMaterialMap', lambda: (lambda mp: (
    mp.setSource(Gi.AcGiMaterialMapSource.kProcedural),
    mp.setBlendFactor(0.35),
    mp.source(), mp.blendFactor(), mp.filter()))(Gi.AcGiMaterialMap()))
check('AcGiImageFileTexture', lambda: (lambda t: (
    t.setSourceFileName('brick.jpg'), t.sourceFileName()))(Gi.AcGiImageFileTexture()))
for en in ('AcGiMaterialIlluminationModel', 'AcGiMaterialChannelFlags', 'AcGiMaterialMode',
           'AcGiMaterialLuminanceMode', 'AcGiMaterialNormalMapMethod',
           'AcGiMaterialGlobalIlluminationMode', 'AcGiMaterialFinalGatherMode',
           'AcGiMaterialColorMethod', 'AcGiMaterialMapSource', 'AcGiMaterialMapFilter',
           'AcGiProceduralTextureType', 'AcGiVisualStyleType', 'AcGiShadowType',
           'AcGiExtendedLightShape', 'AcGiLightAttenuationType', 'AcGiMapChannel'):
    check('enum ' + en, lambda en=en: getattr(Gi, en))

# ---------------------------------------------------------------- 灯光 / 环境
check('AcGiColorRGB', lambda: Gi.AcGiColorRGB(1.0, 0.5, 0.2).blue)
check('AcGiColorRGBA', lambda: Gi.AcGiColorRGBA(1.0, 0.5, 0.2, 0.8).alpha)
check('AcGiShadowParameters', lambda: (lambda p: (
    p.setShadowsOn(True), p.setShadowType(Gi.AcGiShadowType.kShadowMaps),
    p.shadowsOn(), p.shadowType(), p.setExtendedLightShape(Gi.AcGiExtendedLightShape.kRectangle)))(Gi.AcGiShadowParameters()))
check('AcGiLightAttenuation', lambda: (lambda a: (
    a.setAttenuationType(Gi.AcGiLightAttenuationType.kInverseSquare),
    a.setLimits(10.0, 500.0), a.attenuationType(), a.startLimit(), a.endLimit()))(Gi.AcGiLightAttenuation()))
check('AcGiToneOperatorParameters', lambda: (lambda t: (
    t.setIsActive(True), t.isActive(), t.setChromatin if False else t.chromaticAdaptation()))(Gi.AcGiToneOperatorParameters()))
check('AcGiPhotographicExposureParameters', lambda: (lambda p: (
    p.setExposure(12.5), p.exposure(),
    Gi.AcGiPhotographicExposureParameters.convertExposureToBrightness(12.5))) (Gi.AcGiPhotographicExposureParameters()))

# ---------------------------------------------------------------- 绘制上下文中实证
class Demo(Gi.GiPythonDrawable):
    def worldDraw(self, wd):
        try:
            g = wd.geometry()
            ctx = wd.context()
            log('   context valid=%r supportsOwnerDraw=%r supportsFillTrait=%r nested=%r'
                % (ctx.valid(), ctx.supportsOwnerDraw(), ctx.supportsFillTrait(), ctx.isNesting()))

            # mesh + 逐边/逐面/逐顶点属性
            pts = [(0, 0, 0), (50, 0, 0), (0, 50, 0), (50, 50, 0)]
            ed = Gi.AcGiEdgeData(); ed.setColors([1, 2, 3, 4])
            fd = Gi.AcGiFaceData(); fd.setColors([5, 6])
            vd = Gi.AcGiVertexData(); vd.setNormals([(0, 0, 1)] * 4)
            log('   mesh(data)=%r  mesh(noData)=%r  polygon=%r' % (g.mesh(2, 2, pts, ed, fd, vd, True), g.mesh(2, 2, pts, True), g.polygon([(0,0,0),(10,0,0),(10,10,0)])))

            # shell + faceList
            sp = [(0, 0, 0), (40, 0, 0), (40, 40, 0), (0, 40, 0)]
            faces = [4, 0, 1, 2, 3]
            sfd = Gi.AcGiFaceData(); sfd.setColors([3])
            log('   shell(data)=%r  shell(noData)=%r' % (g.shell(sp, faces, None, sfd, None, True), g.shell(sp, faces, True)))

            # polyPolygon：两个三角形
            npos = [3, 3]
            ppos = [(0, 0, 0), (10, 0, 0), (5, 10, 0), (20, 0, 0), (30, 0, 0), (25, 10, 0)]
            npts = [3, 3]
            ppts = list(ppos)
            log('   polyPolygon=%r' % g.polyPolygon(npos, ppos, npts, ppts,
                                                    [Gi.AcCmEntityColor(0, 200, 0)] * 2))

            # ownerDraw(HDC)：只有驱动支持时才有意义
            if ctx.supportsOwnerDraw():
                log('   ownerDraw=%r' % g.ownerDraw(_Gdi(), (60, 60, 0), (1, 0, 0), (0, 1, 0)))
            else:
                log('   ownerDraw 跳过（当前驱动不支持；已用 AcGiGdiDrawObject.selfTest 验证桥接）')
            return True
        except Exception:
            FAILS.append('worldDraw')
            log('   worldDraw 内失败\n' + traceback.format_exc())
            return False

    def setAttributes(self, traits):
        traits.setTrueColor(Gi.AcCmEntityColor(0, 120, 255))
        traits.setFillType(Gi.AcGiFillType.kAcGiFillAlways)
        return Gi.AcGiDrawableType.kGeometry

    def bounds(self):
        return (0, 0, 0), (100, 100, 0)


demo = Demo()
for _mode in (Gi.AcGiTransientDrawingMode.kAcGiMain, Gi.AcGiTransientDrawingMode.kAcGiDirectShortTerm, Gi.AcGiTransientDrawingMode.kAcGiDirectTopmost):
    log('--- 瞬态绘制模式 %r ---' % _mode)
    check('addTransient %r' % _mode, lambda m=_mode: Gi.addTransient(demo, m, 0, None))
    check('updateTransient', lambda: Gi.updateTransient(demo, None))
    check('eraseTransient', lambda: Gi.eraseTransient(demo, None))

log('=== FAILS: %d %s' % (len(FAILS), FAILS))
log('=== done ===')
_f.close()
