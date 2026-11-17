import PyArxCore as C
Gi = C.Gi
enums = [n for n in dir(Gi) if isinstance(getattr(getattr(Gi, n), "__members__", None), dict)]
mapper_enums = [n for n in enums if "Mapper" in n]
print("[GE] mapper_enums=" + ",".join(mapper_enums))
try:
    proj = getattr(Gi, "AcGiMapperProjection")
    first = list(proj.__members__)[0]
    mp = Gi.AcGiMapper()
    mp.setProjection(getattr(proj, first))
    print("[GE] enum_member=" + first + " setProjection(enum)=OK")
except Exception as ex:
    print("[GE] enum_set=" + type(ex).__name__ + ":" + str(ex)[:60])