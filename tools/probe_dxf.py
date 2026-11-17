import io, PyArxCore as C
m = C.Db.DxfCode.__members__
io.open(r"G:\PyArx\log\dxfcode_have.txt","w",encoding="utf-8").write("\n".join("%s=%d" % (k, int(getattr(C.Db.DxfCode,k))) for k in m))
print("[DXF] have=%d" % len(m))