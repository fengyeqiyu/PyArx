import PyArxCore as C
names=["breakSpacing","hasSubSelection","horzCellMargin","isBreakEnabled","isHeaderSuppressed","isTitleSuppressed","minimumTableHeight","minimumTableWidth","tableStyle","vertCellMargin"]
have=[n for n in names if hasattr(C.Db.PyDbTable,n)]
missing=[n for n in names if not hasattr(C.Db.PyDbTable,n)]
print("[T] table getters have=%d missing=%s" % (len(have), missing))
print("[T] done")