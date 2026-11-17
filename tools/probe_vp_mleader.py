import PyArxCore as C
vp=["background","brightness","contrast","customScale","elevation","isDefaultLightingOn","isGridAdaptive","isGridBoundToLimits","isGridFollow","isGridSubdivisionRestricted","isNonRectClipOn","isTransparent","isUcsSavedWithViewport","nonRectClipEntityId","plotAsRaster","plotWireframe","previousBackgroundForcedSwitch","shadePlotId","snapIsoPair","sunId","ucsName","visualStyle"]
ml=["arrowSymbolId","doglegLength","enableFrameText","leaderLineTypeId","textStyleId"]
for cls,names in (("PyDbViewport",vp),("PyDbMLeader",ml)):
    c=getattr(C.Db,cls)
    missing=[n for n in names if not hasattr(c,n)]
    print("[V] %s have=%d missing=%s" % (cls,len(names)-len(missing),missing))
print("[V] done")