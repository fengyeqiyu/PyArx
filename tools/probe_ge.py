# -*- coding: utf-8 -*-
import traceback
import PyArxCore as C

Ge = C.Ge
print("[T] has=%r" % hasattr(Ge.Matrix3d, "translation"))
for desc, fn in [
    ("static vec3", lambda: Ge.Matrix3d.translation(Ge.Vector3d(1, 0, 0))),
    ("static vec2", lambda: Ge.Matrix3d.translation(Ge.Vector2d(1, 0))),
]:
    try:
        r = fn()
        print("[T] %s -> %r" % (desc, r))
    except Exception:
        print("[T] %s FAIL" % desc)
        traceback.print_exc()
print("[T] done")
