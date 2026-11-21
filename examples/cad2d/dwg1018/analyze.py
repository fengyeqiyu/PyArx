# -*- coding: utf-8 -*-
"""离线解析 AC1018 (R2004) DWG：输出文件头、页地图、节地图结构报告。

容器层（头解密 / 页地图 / 节地图定位）已打通；压缩流解压与对象语义解码
属于后续迭代，本脚本在解压失败时仍保留已解析的容器信息。
"""
import json
import struct
import sys

sys.path.insert(0, r"G:\PyArx\examples\cad2d\dwg1018")
from container import open_dwg, SIG_PAGE_MAP, SIG_SECTION_MAP


def main(path, out_json=None):
    report = {"path": path, "error": None, "header": None, "sections": []}
    try:
        c = open_dwg(path)
    except Exception as e:
        report["error"] = f"{type(e).__name__}: {e}"
        _dump(report, out_json)
        return report

    report["version"] = c.version
    report["maintenance"] = c.maintenance
    report["codepage"] = c.codepage
    report["security"] = hex(c.security)
    report["header"] = {k: (hex(v) if isinstance(v, int) and v > 9 else v)
                        for k, v in c.header.items()}
    report["page_map_entries"] = len(c.page_map)
    for name, d in c.sections.items():
        report["sections"].append({
            "name": name,
            "section_id": d.section_id,
            "size": d.size,
            "page_count": d.page_count,
            "max_page_size": hex(d.max_page_size),
            "compressed": d.compressed,
            "encrypted": d.encrypted,
            "pages": d.pages,
        })
    _dump(report, out_json)
    return report


def _dump(report, out_json):
    print(json.dumps(report, ensure_ascii=False, indent=2, default=str))
    if out_json:
        with open(out_json, "w", encoding="utf-8") as f:
            json.dump(report, f, ensure_ascii=False, indent=2, default=str)


if __name__ == "__main__":
    out = sys.argv[2] if len(sys.argv) > 2 else None
    main(sys.argv[1], out)
