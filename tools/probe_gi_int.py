import PyArxCore as C
Gi = C.Gi

mp = Gi.AcGiMapper()
for name, val in (("setProjection", 1), ("setAutoTransform", 1), ("setUTiling", 0), ("setVTiling", 0)):
    try:
        getattr(mp, name)(val)
        print("[GI] %s(int)=OK" % name)
    except Exception as ex:
        print("[GI] %s(int)=%s" % (name, type(ex).__name__))
print("[GI] doc_projection=" + (Gi.AcGiMapper.setProjection.__doc__ or "").splitlines()[0][:90])

tm = Gi.AcCmTransparency()
try:
    tm.setMethod(1)
    print("[GI] setMethod(int)=OK")
except Exception as ex:
    print("[GI] setMethod(int)=" + type(ex).__name__)

sh = Gi.AcGiShadowParameters()
for name, val in (("setShadowType", 1), ("setExtendedLightShape", 1)):
    try:
        getattr(sh, name)(val)
        print("[GI] %s(int)=OK" % name)
    except Exception as ex:
        print("[GI] %s(int)=%s" % (name, type(ex).__name__))

try:
    mm = Gi.AcGiMaterialMap()
    mm.setSource(1); mm.setFilter(1)
    print("[GI] materialmap(int)=OK")
except Exception as ex:
    print("[GI] materialmap(int)=" + type(ex).__name__)

try:
    lat = Gi.AcGiLightAttenuation()
    lat.setAttenuationType(1)
    print("[GI] attenuation(int)=OK")
except Exception as ex:
    print("[GI] attenuation(int)=" + type(ex).__name__)