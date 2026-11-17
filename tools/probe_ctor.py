import PyArxCore as C
Db=C.Db
for c in ("Database","Circle","Text","Arc","Polyline","Ellipse","Spline","Hatch","MText","BlockReference"):
    o=getattr(Db,c)
    print("[CT] === %s ===" % c)
    print((o.__init__.__doc__ or "")[:600])