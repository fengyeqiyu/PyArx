import PyArxCore as C
Db = C.Db
print("[DXF] LayerName=" + repr(int(Db.DxfCode.kDxfLayerName)))
print("[DXF] Text=" + repr(int(Db.DxfCode.kDxfText)) + " Angle=" + repr(int(Db.DxfCode.kDxfAngle)))
tv1 = Db.TypedValue(Db.DxfCode.kDxfLayerName, "0")
tv2 = Db.TypedValue(8, "0")
print("[DXF] enum_arg=" + repr(tv1.code) + " int_arg=" + repr(tv2.code) + " same=" + repr(tv1.code == tv2.code))
try:
    rev = Db.DxfCode(8)
    print("[DXF] reverse_lookup=" + rev.name + " int=" + repr(int(rev)))
except Exception as ex:
    print("[DXF] reverse_lookup_fail=" + repr(ex)[:80])