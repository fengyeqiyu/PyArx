import PyArxCore as C
Db, Ed = C.Db, C.Ed
def d(o,n,tag):
    print("[S2] %-34s %s" % (tag,(getattr(o,n).__doc__ or "").replace("\n"," ")[:150] if hasattr(o,n) else "MISSING"))
d(Ed.Core,"setVar","Core.setVar"); d(Ed.Core,"getVar","Core.getVar")
d(Db.Database,"saveAs","Database.saveAs"); d(Db.Database,"dxfOut","Database.dxfOut"); d(Db.Database,"readDwgFile","Database.readDwgFile")
d(Db.Database,"addToModelspace","Database.addToModelspace")
d(Db.SymbolTable,"get","SymbolTable.get")
d(Db.BlockTable,"get","BlockTable.get")
d(Db.BlockTableRecord,"__init__","BlockTableRecord.init")
d(Db.Entity,"close","Entity.close")
d(Db.Circle,"radius","Circle.radius")
d(Db.DbObject,"cast","DbObject.cast")