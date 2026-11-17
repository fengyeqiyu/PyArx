import PyArxCore as C
Db = C.Db
print("[F] cls=" + repr(Db.Field.className()))
f = Db.Field()
print("[F] inst=" + repr(f.className()) + " child=" + repr(f.childCount()) + " text=" + repr(f.isTextField()))
print("[F] enum=" + repr(int(Db.FieldCodeFlag.kFieldCode)) + " state=" + repr(int(Db.FieldState.kInitialized)))
print("[F] code=" + repr(Db.Field("%<\\AcVar Date>%").getFieldCode(Db.FieldCodeFlag.kFieldCode)))