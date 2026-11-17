import PyArxCore as C
Db, Gi, Ed = C.Db, C.Gi, C.Ed
print("[EK] ColorMethod_alias=%s" % (Db.ColorMethod is Gi.AcCmColorMethod))
print("[EK] TransparencyMethod_alias=%s" % (Db.TransparencyMethod is Gi.AcCmTransparencyMethod))
print("[EK] new_cell=%d %s" % (len(Db.CellAlignment.__members__), Db.CellAlignment.kMiddleCenter.value))
print("[EK] OpenMode_ForRead=%d kForRead=%d" % (Db.OpenMode.ForRead.value, Db.OpenMode.kForRead.value))
print("[EK] ErrorStatus_Ok=%d eOk=%d" % (Db.ErrorStatus.Ok.value, Db.ErrorStatus.eOk.value))
print("[EK] PromptStatus_eOk=%d eNormal=%d" % (Ed.PromptStatus.eOk.value, Ed.PromptStatus.eNormal.value))
print("[EK] RegenType_shaded=%d render=%d" % (Gi.AcGiRegenType.kAcGiShadedDisplay.value, Gi.AcGiRegenType.kAcGiRenderCommand.value))
print("[EK] Shadow_kNoDraw=%d" % Gi.AcGiShadowFlags.kNoDrawFlags.value)