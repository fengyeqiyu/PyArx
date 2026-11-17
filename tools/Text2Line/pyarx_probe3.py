"""PyArx 原生路线：Ed.entmake 建 LWPOLYLINE / Ed.entget 读实体 / Ed.entdel 删除。"""

import json
import sys
import traceback

for _p in (r"G:\PyArx\X64\Release",):
    if _p not in sys.path:
        sys.path.insert(0, _p)

OUT = r"G:\PyArx\tools\Text2Line\pyarx_probe3.json"


def main():
    import PyArxCore as P
    Ed = P.Ed
    rep = {}

    def t(k, fn):
        try:
            rep[k] = str(fn())[:200]
        except Exception as e:
            rep[k] = "ERR " + repr(e)[:240]

    # 1) 用列表版 dxf 建一条闭合多段线（每点需要重复 10/20 组码）
    dxf = [(0, "LWPOLYLINE"), (100, "AcDbEntity"), (8, "0"),
           (100, "AcDbPolyline"), (90, 4), (70, 1)]
    for x, y in [(0.0, 0.0), (400.0, 0.0), (400.0, 400.0), (0.0, 400.0)]:
        dxf.append((10, x))
        dxf.append((20, y))

    rep["entmake"] = t("entmake", lambda: Ed.entmake(dxf))
    last = None
    try:
        last = Ed.entlast()
    except Exception as e:
        rep["entlast"] = "ERR " + repr(e)[:120]
    rep["entlast"] = str(last)
    if last is not None:
        rep["entget(0)"] = t("entget", lambda: Ed.entget(last)[0])
        rep["entget keys"] = t("keys", lambda: sorted(Ed.entget(last).keys()))

    # 2) 句柄查找（提前验证选择集/句柄路线可用）
    rep["handent(B9)"] = t("handent", lambda: Ed.handent("B9"))
    return rep


if __name__ == "__main__":
    try:
        rep = main()
    except Exception:
        rep = {"fatal": traceback.format_exc()}
    with open(OUT, "w", encoding="utf-8") as f:
        json.dump(rep, f, ensure_ascii=False, indent=1)
    print("[PROBE3]", list(rep.items()))
