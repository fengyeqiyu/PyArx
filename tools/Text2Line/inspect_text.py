"""inspect_text.py —— 打印文字实体的字体/样式/几何参数，判断是 SHX 还是 TTF。"""

import json
import sys
import traceback

for _p in (r"G:\PyArx\X64\Release",):
    if _p not in sys.path:
        sys.path.insert(0, _p)

OUT = r"G:\PyArx\tools\Text2Line\inspect_out.json"


def main():
    import PyArxCore as P

    rep = {"file": None, "items": []}
    db = P.Db.curDb()
    rep["file"] = db.getFilename()
    ids = db.objectIds("AcDbText")
    rep["texts"] = len(ids)

    seen = 0
    with P.Db.PyDbTrans() as tr:
        for tid in ids:
            t = tr.openObject(tid, 0)
            s = t.textString()
            if "YJLHV" not in s:
                continue
            if seen >= 4:
                break
            seen += 1
            st = t.textStyle()
            font = big = ""
            try:
                rec = tr.openObject(st, 0)
                font = rec.fileName()
                big = rec.bigFontFileName()
            except Exception as e:
                font = "ERR " + repr(e)[:80]
            rep["items"].append({
                "handle": t.handle(),
                "text": s,
                "style": str(st),
                "font": font,
                "big": big,
                "height": t.height(),
                "rot": t.rotation(),
                "wf": t.widthFactor(),
                "obl": t.oblique(),
            })
    return rep


if __name__ == "__main__":
    try:
        rep = main()
    except Exception:
        rep = {"exception": traceback.format_exc()}
    with open(OUT, "w", encoding="utf-8") as f:
        json.dump(rep, f, ensure_ascii=False, indent=1)
    print("[INSPECT]", len(rep.get("items", [])))
