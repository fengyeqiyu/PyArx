# -*- coding: utf-8 -*-
LOG = r"G:\PyArx\log\pyarcore_diag.log"
with open(LOG, "w", encoding="utf-8") as f:
    f.write("== diag ==\n"); f.flush()
def L(msg):
    with open(LOG, "a", encoding="utf-8") as f:
        f.write(msg + "\n"); f.flush()
import PyArxCore
ap = getattr(PyArxCore, "Ap", None)
L("Ap = %s" % repr(ap))
if ap is not None:
    L("hasattr _ap_probe: %s" % hasattr(ap, "_ap_probe"))
    L("dir(Ap) = %s" % dir(ap))
L("== end ==")
