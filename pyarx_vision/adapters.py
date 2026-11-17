# -*- utf-8 -*-
"""pyarx_vision.adapters —— 把各种图元导出格式归一成 EntitySet

已支持：
  sys     系统图导出：lines [x1,y1,x2,y2,lay]；plines {p,c,l}；texts {x,y,t,h,r,l}
  strong  强电桥架导出：lines {lay,a,b}；texts {lay,text,pos,rot,h}
  tray    桥架分段导出：lines {layer,start,end,label...}
新增格式只需在这里加一个 from_xxx 并注册到 LOADERS。
"""
import json
import os

from . import schema


def _num(v):
    try:
        return float(v)
    except (TypeError, ValueError):
        return 0.0


def from_sys(d, source=""):
    es = schema.empty(source or d.get("file", "sys"))
    if isinstance(d.get("frame"), (list, tuple)) and len(d["frame"]) == 4:
        es["frame"] = tuple(float(x) for x in d["frame"])
    for l in d.get("lines") or []:
        if isinstance(l, (list, tuple)) and len(l) >= 4:
            schema.add_line(es, (l[0], l[1]), (l[2], l[3]), l[4] if len(l) > 4 else "")
        elif isinstance(l, dict):
            schema.add_line(es, l.get("a", l.get("start")), l.get("b", l.get("end")),
                            l.get("layer", l.get("lay", "")))
    for p in d.get("plines") or []:
        pts = p.get("p") or p.get("pts") or []
        schema.add_pline(es, pts, p.get("c", p.get("closed", False)),
                         p.get("l", p.get("layer", "")))
    for t in d.get("texts") or []:
        schema.add_text(es, (t.get("x", t.get("pos", (0, 0))[0] if isinstance(t.get("pos"), (list, tuple)) else 0),
                             t.get("y", t.get("pos", (0, 0))[1] if isinstance(t.get("pos"), (list, tuple)) else 0)),
                        t.get("t", t.get("text", "")), t.get("h", t.get("height", 1)),
                        t.get("r", t.get("rot", t.get("rotation", 0))),
                        t.get("l", t.get("lay", t.get("layer", "")))
                        if isinstance(t.get("l", t.get("lay", "")), str) else "")
    for c in d.get("circles") or []:
        if isinstance(c, (list, tuple)) and len(c) >= 3:
            schema.add_circle(es, (c[0], c[1]), c[2], c[3] if len(c) > 3 else "")
        elif isinstance(c, dict):
            schema.add_circle(es, c.get("center", (0, 0)), c.get("r", 0), c.get("l", ""))
    if es["frame"] is None:
        es["frame"] = schema.bbox(es)
    return es


def from_strong(d, source=""):
    """strong_2d.json：line 用 a/b，text 用 pos/rot/text"""
    es = schema.empty(source or d.get("file", "strong"))
    for l in d.get("lines") or []:
        schema.add_line(es, l.get("a", (0, 0)), l.get("b", (0, 0)), l.get("lay", ""))
    for t in d.get("texts") or []:
        schema.add_text(es, t.get("pos", (0, 0)), t.get("text", ""),
                        t.get("h", 1), t.get("rot", 0), t.get("lay", ""))
    es["frame"] = schema.bbox(es)
    return es


def from_tray(d, source=""):
    """tray_out.json：line 用 start/end，带 length/dir"""
    es = schema.empty(source or d.get("file", "tray"))
    for l in d.get("lines") or []:
        schema.add_line(es, l.get("start", (0, 0)), l.get("end", (0, 0)), l.get("layer", ""))
    es["frame"] = schema.bbox(es)
    return es


LOADERS = {
    "sys": from_sys,
    "strong": from_strong,
    "tray": from_tray,
}


def load(path, kind=None):
    """读文件并按格式适配；kind 省略时按文件名猜"""
    d = json.load(open(path, encoding="utf-8"))
    if kind is None:
        name = os.path.basename(path).lower()
        kind = ("sys" if "sys" in name else
                "strong" if "strong" in name else
                "tray" if "tray" in name else "sys")
    return LOADERS[kind](d, path)
