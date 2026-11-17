import PyArxCore as C
Db = C.Db
print("[EP] doc=" + repr(Db.Field.__init__.__doc__)[:160])
print("[EP] enum==int: " + repr(Db.OpenMode.kForRead == 1) + " int(): " + repr(int(Db.OpenMode.kForRead)))
try:
    Db.Field(Db.ObjectId(1), 1, False)
    print("[EP] int->enum=ACCEPT")
except TypeError as ex:
    print("[EP] int->enum=REJECT " + str(ex).splitlines()[0][:80])
except Exception as ex:
    print("[EP] int->enum=" + type(ex).__name__ + " " + str(ex)[:60])
try:
    Db.Field(Db.ObjectId(1), Db.OpenMode.kForRead, False)
    print("[EP] enum->enum=ACCEPT")
except Exception as ex:
    print("[EP] enum->enum=" + type(ex).__name__ + " " + str(ex)[:60])
try:
    Db.TypedValue(Db.DxfCode.kDxfLayerName, "0")
    print("[EP] enum->int=ACCEPT")
except Exception as ex:
    print("[EP] enum->int=" + type(ex).__name__ + " " + str(ex)[:60])