"""无头探针：验证本轮补齐的 PyDbDatabase getter 可读、无异常。

在 accoreconsole 中经 PYRUN 调用，打印 [G] 行供日志断言。
"""
import traceback

import PyArxCore as C

PROBES = ['getters_in_class','UCSTableId','angdir','annoAllVisible','annotativeDwg','attdia','attreq','cDynDisplayMode','cameraDisplay','cameraHeight','chamferc','chamferd','cmljust','cmlscale','cmlstyleID','colorDictionaryId','coords','dataLinkDictionaryId','delUsedObj','detailViewStyle','detailViewStyleDictionaryId','dimfit','dimunit','dispSilh','dragVisStyle','dragmode','dxeVal','facetres','geoCoordinateSystemId','geoMarkerVisibility','get3dDwfPrec','getSymbolTableId','groupDictionaryId','hpInherit','interfereObjVisStyle','interfereVpVisStyle','intersectColor','isolines','latitude','layerNotify','layoutDictionaryId','lensLength','lightsInBlocks','lineWeightDisplay','loftAng1','loftAng2','loftMag1','loftMag2','loftParam','longitude','mLStyleDictionaryId','materialDictionaryId','maxactvp','mleaderStyleDictionaryId','mleaderscale','mleaderstyle','msOleScale','msltscale','northDirection','obscuredColor','oleStartUp','pdmode','pelevation','pickstyle','plimcheck','plineEllipse','plotSettingsDictionaryId','plotStyleNameDictionaryId','previewType','psolHeight','psolWidth','pucsBase','pucsname','realWorldScale','retainOriginalThumbnailBitmap','saveproxygraphics','scaleListDictionaryId','sectionViewStyle','sectionViewStyleDictionaryId','shadedge','shadedif','shadowPlaneLocation','sketchinc','skpoly','splframe','splinesegs','splinetype','stepSize','stepsPerSec','surftab1','surftab2','surftype','surfu','surfv','tableStyleDictionaryId','tablestyle','tracewid','treedepth','tstackalign','tstacksize','ucsBase','ucsname','unitmode','useri1','useri2','useri3','useri4','useri5','userr3','userr4','userr5','usrtimer','visretain','visualStyleDictionaryId','worldview','xrefBlockId','xrefEditEnabled']


def main():
    db = C.Db.curDb()
    ok = 0
    bad = []
    for name in sorted(set(PROBES)):
        try:
            val = getattr(db, name)()
            ok += 1
        except Exception:
            bad.append("%s: %s" % (name, traceback.format_exc().splitlines()[-1]))
    print("[G] getters ok=%d fail=%d" % (ok, len(bad)))
    for item in bad:
        print("[G] FAIL", item)
    print("[G] done")


main()
