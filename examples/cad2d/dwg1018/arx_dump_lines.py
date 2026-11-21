# -*- coding: utf-8 -*-
"""ARX 无头导出：用 PyArxCore(AcDbDatabase 官方解码) 直接读源 DWG，
遍历全部块表记录，把 LINE 实体的权威字段按 handle 导出 TSV，供离线 Python 解析包按 handle 比对。

运行环境：accoreconsole 进程内（PyArxEngine.crx 已加载），PYRUNFILE 执行本脚本。
输出：TSV，tab 分隔；列见 HEADER。
"""
import json
import os
import sys
import traceback

import PyArxCore as P

SRC = os.environ.get(
    "PYARX_DUMP_SRC",
    r"I:\xwechat_files\wxid_jbg0jgp7km6m21_d2a3\msg\file\2026-10\中试车间扩建-工艺配电及系统2026.10.12_t3.dwg")
OUT = os.environ.get(
    "PYARX_DUMP_OUT",
    r"G:\PyArx\examples\cad2d\dwg1018\arx_lines.tsv")
ERRLOG = os.path.join(os.path.dirname(OUT), "arx_dump_err.log")

HEADER = ["handle", "class",
          "sx", "sy", "sz", "ex", "ey", "ez",
          "colorIndex", "r", "g", "b",
          "layer", "ltype", "lineWeight", "ltypeScale", "visibility", "plotStyle"]

stat = {"total_ents": 0, "line": 0, "by_class": {}, "errors": []}


def _safe(fn, default=None):
    try:
        return fn()
    except Exception as ex:
        stat["errors"].append("%s: %r" % (getattr(fn, "__name__", "?"), ex))
        return default


def main():
    # 文档库由 accoreconsole /i 打开；无头取不到当前库时再自建库 readDwgFile。
    db = None
    try:
        db = P.Db.curDb()
    except Exception as ex:
        stat["errors"].append("curDb: %r" % ex)
    if db is None:
        db = P.Db.PyDbDatabase(False, False)
        st = db.readDwgFile(SRC, 1, True, "")
        stat["read_status"] = str(st)
    stat["filename"] = _safe(lambda: str(db.getFilename()), "")

    bt = db.getBlockTable(0)
    names = _safe(lambda: bt.names(), [])
    stat["block_records"] = len(names)

    rows = []
    tr = P.Db.PyDbTrans(db)
    for name in names:
        try:
            rec = bt.getAt(name, 0)
            if rec is None:
                continue
            ids = rec.entityIds()
        except Exception as ex:
            stat["errors"].append("entityIds %s: %r" % (name, ex))
            continue
        for eid in ids:
            stat["total_ents"] += 1
            o = None
            try:
                o = tr.openObject(eid, 0)
                if o is None:
                    continue
                cls = str(o.className())
                stat["by_class"][cls] = stat["by_class"].get(cls, 0) + 1
                if cls != "AcDbLine":
                    continue

                sp = o.startPoint()
                ep = o.endPoint()

                def rgb():
                    t = o.colorRGB()
                    return (int(t[0]), int(t[1]), int(t[2]))

                r, g, b = _safe(rgb, (-1, -1, -1))
                rows.append([
                    str(int(o.handle())),
                    cls,
                    repr(float(sp.x)), repr(float(sp.y)), repr(float(sp.z)),
                    repr(float(ep.x)), repr(float(ep.y)), repr(float(ep.z)),
                    str(int(o.colorIndex())),
                    str(r), str(g), str(b),
                    str(_safe(o.layer, "?")),
                    str(_safe(o.linetype, "?")),
                    str(int(_safe(o.lineWeight, -1))),
                    repr(float(_safe(o.linetypeScale, -1.0))),
                    str(int(_safe(o.visibility, -1))),
                    str(_safe(o.plotStyleName, "?")),
                ])
                stat["line"] += 1
            except Exception as ex:
                stat["errors"].append("handle=%s: %r" % (
                    _safe(lambda: str(int(o.handle())) if o is not None else "?"), ex))
            finally:
                try:
                    if o is not None:
                        o.close()
                except Exception:
                    pass
    try:
        tr.abort()
    except Exception:
        pass

    with open(OUT, "w", encoding="utf-8") as f:
        f.write("\t".join(HEADER) + "\n")
        for row in rows:
            f.write("\t".join(row) + "\n")

    with open(ERRLOG, "w", encoding="utf-8") as f:
        f.write(json.dumps(stat, ensure_ascii=False, indent=1, default=str))

    print("[DUMP] ents=%d line=%d blocks=%d out=%s"
          % (stat["total_ents"], stat["line"], stat["block_records"], OUT), flush=True)


if __name__ == "__main__":
    try:
        main()
    except Exception:
        stat["fatal"] = traceback.format_exc()[-1500:]
        with open(ERRLOG, "w", encoding="utf-8") as f:
            f.write(json.dumps(stat, ensure_ascii=False, indent=1, default=str))
        print("[DUMP] FATAL\n" + stat["fatal"], flush=True)
