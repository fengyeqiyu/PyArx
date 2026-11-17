import PyArxCore as C
Db=C.Db
def f(o,n,tag):
    if not hasattr(o,n):
        print("[S3] %-30s MISSING" % tag); return
    print("[S3] === %s ===\n%s" % (tag,(getattr(o,n).__doc__ or "")[:800]))
f(Db.SymbolTable,"get","SymbolTable.get")
f(Db.BlockTableRecord,"__init__","BlockTableRecord.init")
f(Db.Entity,"__init__","Entity.init")
f(Db.DbObject,"close","DbObject.close")
f(Db.ObjectId,"__init__","ObjectId.init")
f(Db.Database,"getBlockTable","getBlockTable")