# -*- utf-8 -*-
"""逐个配电箱出一张 XML 画布图，用 loadDwgLayout 核对箱内回路识别。

一箱一图理由：31 个箱共 610 条回路，全塞一张图会挤成一团没法核字；
拆开后每张就是"箱 → 它的回路"，箱号、断路器、电缆、功率都能一眼对上。
"""

import io
import json
import os
import re
import sys
import time

sys.path.insert(0, r"G:\PyArx\X64\Release")
os.add_dll_directory(r"G:\PyArx\X64\Release")
import PyMfcXml as M

PLAN = r"G:\PyArx\docs\drb018_powerplan_full.json"
OUT = r"G:\PyArx\docs\boxes_mfc"
CANVAS = 7200


def safe(s):
    s = re.sub(r'[\\/:*?"<>|]', "", s or "").strip()
    return s[:40] or "box"


def main():
    nodes = json.load(io.open(PLAN, encoding="utf-8"))["nodes"]
    boxes = [n for n in nodes if n.get("kind") == "box"]
    loops = [n for n in nodes if n.get("kind") == "loop"]
    by_up = {}
    for l in loops:
        by_up.setdefault(l["upstream"], []).append(l)

    os.makedirs(OUT, exist_ok=True)
    WW, WH = 1680, 960
    win = M.Window("配电箱回路核对", WW, WH)
    win.set_theme(False)
    win.add_dwg_canvas(CANVAS, 0, 0, WW, WH)
    win.show()
    M.pump(900)
    cw, ch = win.client_size()
    if cw > 0 and ch > 0:
        win.move(CANVAS, 0, 0, cw, ch)
        M.pump(300)
        WW, WH = cw, ch

    index = []
    for i, b in enumerate(sorted(boxes, key=lambda n: (n.get("sheet", 0), n["code"]))):
        sub = [{"code": b["code"], "name": b["name"], "upstream": "",
                "cable": "", "loadKw": b.get("loadKw", 0.0), "voltage": "380"}]
        ls = sorted(by_up.get(b["code"], []), key=lambda n: n["code"])
        for l in ls:
            sub.append({"code": l["code"], "name": l["name"],
                        "upstream": b["code"], "cable": l.get("cable", ""),
                        "loadKw": l.get("loadKw", 0.0),
                        "voltage": l.get("voltage", "380")})
        title = "#%d %s %s (%d回路)" % (b.get("sheet", 0), b["code"],
                                        b["name"] or "", len(ls))
        plan = M.powerplan.build(sub, M.powerplan.Style(), title, True)   # 横向
        win.load_dwg_layout(CANVAS, plan["json"])
        M.pump(120)
        # 保持 loadDwgLayout 自动 fit 的视图，不覆盖成 1:1
        name = "%02d_%s.png" % (i + 1, safe(b["code"]))
        png = os.path.join(OUT, name)
        win.save_control_png(CANVAS, png)
        kb = os.path.getsize(png) // 1024 if os.path.exists(png) else 0
        print("%-42s %-3d回路 %dx%d dir=%s %dKB" % (name, len(ls), plan["width"],
                                                 plan["height"], plan["dir"], kb))
        index.append({"no": i + 1, "sheet": b.get("sheet"), "box": b["code"],
                      "name": b["name"], "loops": len(ls),
                      "omitted": b.get("omitted", 0),
                      "expand": b.get("expand_count", 0),
                      "note": b.get("note", ""),
                      "w": plan["width"], "h": plan["height"],
                      "png": png, "kb": kb})

    io.open(os.path.join(OUT + "_index.json"), "w", encoding="utf-8").write(
        json.dumps(index, ensure_ascii=False, indent=1))
    win.close()
    print("共 %d 张  输出 %s" % (len(index), OUT))


if __name__ == "__main__":
    main()
