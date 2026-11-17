# -*- coding: utf-8 -*-
import json
import sys

sys.path.insert(0, r"G:\PyArx\tools\train\tests")
import PyArxCore as P
import mlab

Ge = P.Ge


def loop():
    s = Ge.AcGeScale3d(2.0, 3.0, 4.0)
    m = s.getMatrix()
    sc = s.extractScale(m)
    return "Scale3d(2,3,4).getMatrix() -> extractScale(matrix) 回读=(%.3f,%.3f,%.3f)" % (
        sc.sx, sc.sy, sc.sz)


mlab.check("Ge", "AcGeScale3d", "extractScale_matrix_loop", loop)

out = mlab.out_path(r"G:\PyArx\tools\train\data\shard_ge_gi.json")
mlab.dump(out)
print(json.dumps({"written": out}))
