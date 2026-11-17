# -*- coding: utf-8 -*-
import json
import sys
import math

sys.path.insert(0, r"G:\PyArx\tools\train\tests")
import PyArxCore as P
import mlab

Ge = P.Ge
Gi = P.Gi

# Scale3d.extractScale 签名原文
s = Ge.AcGeScale3d(2.0, 3.0, 4.0)
sig = mlab.method_typeerror(s, "extractScale", "x")

# 尝试常见重载：无参返回对象 / 矩阵+尺度出参
variant = []
try:
    r = s.extractScale()
    variant.append("无参返回: %r" % (r,))
except Exception as e:
    variant.append("无参: %s: %s" % (type(e).__name__, str(e)[:120]))

mlab.check("Ge", "AcGeScale3d", "extractScale_signature",
           lambda: "签名: %s | 尝试: %s" % (sig[:300], "; ".join(variant)))

# CircArc.length(fromParam, toParam, tol)
c = Ge.AcGeCircArc3d(Ge.AcGePoint3d(0, 0, 0), Ge.AcGeVector3d(0, 0, 1), 2.0)
Ln = c.length(0.0, 2.0 * math.pi)
mlab.check("Ge", "AcGeCircArc3d", "length_param_range",
           lambda: "length(0,2π) r=2 -> %.6f 期望 2πr=%.6f" % (Ln, 2 * math.pi * 2))

out = mlab.out_path(r"G:\PyArx\tools\train\data\shard_ge_gi.json")
mlab.dump(out)
print(json.dumps({"written": out}))
