# -*- coding: utf-8 -*-
"""生成三级配电大表 CSV（列口径与 powerplan.rules.json 一致：11 列）。

用法：python _gen_big_csv.py [N1] [M2] [K3]
默认输出两份：_tree3_20_10_40.csv（8220 行）、_big_20_10_50.csv（10220 行）
"""
import csv
import math
import os
import sys

DOC = r"G:\PyArx\docs"
HEADER = ["起点", "回路", "终点", "型号规格", "电压等级", "电缆根数",
          "水平长度", "垂直长度", "余量", "合计", "功率"]


def row(start, loop, end, spec, volt, num, h, v, spare, power):
    total = int(math.ceil(h + v + spare))
    return [start, loop, end, spec, volt, num, h, v, spare, total, power]


def build(n1=20, m2=10, k3=40):
    rows = []
    # 一级：进线变压器 → 一级配电柜（起点留空，表示根）
    for i in range(1, n1 + 1):
        rows.append(row("", "WL-TR%d" % i, "TR%d" % i, "YJV22-4×95", "10kV 中压", 1,
                        12 + i % 7, 2, 2, 0))
    # 二级：一级柜 → 二级柜
    for i in range(1, n1 + 1):
        for j in range(1, m2 + 1):
            code = "AT%d-%d" % (i, j)
            rows.append(row("TR%d" % i, "WL-AT%d-%02d" % (i, j), code,
                            "YJV22-4×35", "380V 三相", 1, 5 + (i + j) % 6, 2, 2,
                            round(1.5 * j, 1)))
    # 三级：二级柜 → 终端
    for i in range(1, n1 + 1):
        for j in range(1, m2 + 1):
            parent = "AT%d-%d" % (i, j)
            for k in range(1, k3 + 1):
                code = "AT%d-%d-%d" % (i, j, k)
                rows.append(row(parent, "WL-AT%d-%d-%02d" % (i, j, k), code,
                                "BV-3×4" if k % 3 == 0 else "YJV22-4×25",
                                "220V 单相" if k % 2 else "380V 三相", 1,
                                4 + (i + j + k) % 9, 2, 2, round(1.0 * k % 5 + 1, 1)))
    return rows


def write(path, rows):
    # 不写 BOM：导入解析按首字段名匹配，BOM 会让表头首列对不上
    with open(path, "w", newline="", encoding="utf-8") as f:
        w = csv.writer(f)
        w.writerow(HEADER)
        w.writerows(rows)
    print("%s  %d 行" % (path, len(rows)))


if __name__ == "__main__":
    n1 = int(sys.argv[1]) if len(sys.argv) > 1 else 20
    m2 = int(sys.argv[2]) if len(sys.argv) > 2 else 10
    k3 = int(sys.argv[3]) if len(sys.argv) > 3 else 40
    rows = build(n1, m2, k3)
    write(os.path.join(DOC, "_tree3_%d_%d_%d.csv" % (n1, m2, k3)), rows)
    # 顺带出一份 1 万行以上的：一级 20、二级 10、三级 50 = 20+200+10000 = 10220
    if k3 == 40:
        write(os.path.join(DOC, "_big_20_10_50.csv"), build(20, 10, 50))
