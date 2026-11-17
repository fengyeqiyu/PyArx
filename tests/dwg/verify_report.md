# DWG 解析器对拍报告

## 楼层平面-10KV电缆桥架.dwg
- 状态: **pass**；执行项 19，失败 0

| 编号 | 比对项 | 根因层 | 状态 | 期望 | 实际 |
|---|---|---|---|---|---|
| version | 文件版本 | 容器层 | pass | "AC1027" | "AC1021" |
| header | HEADER 变量（按 $VAR 全量逐值） | 容器层 | pass | "DXF 中 209 个可比 $VAR 逐值相等" | "逐值相等 207 / 断言 209" |
| classes | 类表 AcDb:Classes：类名 + 实体/对象标志 | 对象层 | pass | {"count": 12, "rows": 12} | {"count": 12, "rows": 12} |
| objects.coverage | 对象层覆盖率：无未知类型、无解码报错 | 对象层 | pass | 0 | {"total": 516, "unknown": 0, "error": 0} |
| dictionaries | DICTIONARY 字典树：键名逐个 + 句柄全部可解析 | 对象层 | pass | "DXF 字典 30 个 / 键 87 个（去口径差）" | "解析器字典 53 个 / 键 87 个，未解析句柄 0" |
| linetypes | LTYPE 表：名字/描述/图案段长度/段标志 | 对象层 | pass | 4 | 4 |
| count.model | 模型空间顶层实体总数 | 对象层 | pass | 80 | 80 |
| count.block | 块内实体总数 | 对象层 | pass | 210 | 210 |
| type.model | 模型空间按类型计数 | 对象层 | pass | {"INSERT": 6, "LINE": 68, "MTEXT": 6} | {"INSERT": 6, "LINE": 68, "MTEXT": 6} |
| type.block | 块内按类型计数 | 对象层 | pass | {"LINE": 210} | {"LINE": 210} |
| layers | 图层名集合 + ACI 颜色 + 线型 | 对象层 | pass | ["0", "E-CABL-TRAY", "E-CABL-TRAY-CNTR", "E-CABL-TRAY-IDEN"] | ["0", "E-CABL-TRAY", "E-CABL-TRAY-CNTR", "E-CABL-TRAY-IDEN"] |
| blocks | 块名集合 + 块基点 + 匿名标志 | 引用层 | pass | ["*Model_Space", "*Paper_Space", "*Paper_Space0", "槽式电缆桥架... | ["*Model_Space", "*Paper_Space", "*Paper_Space0", "槽式电缆桥架... |
| geometry | 按 handle 配对几何误差（坐标/角度/bulge） | 模型层 | pass | {"matched": 290, "only_truth": 0, "only_parser": 0} | {"matched": 290, "only_truth": 0, "only_parser": 0} |
| text | TEXT/MTEXT 文字内容逐 handle 比对 | 对象层 | pass | 6 | 6 |
| dimension | DIMENSION 全字段：定义点/文字中点/界线/类型/样式/附着点/实测值/73-74-75/挤出方向 | 对象层 | pass | 0 | 0 |
| styles | STYLE 表：名字 + 字体 + 字高/宽/倾斜 | 对象层 | pass | 3 | 3 |
| dimstyles | DIMSTYLE 表：名字集合 + DIM* 变量 67 字段 | 对象层 | pass | [["Annotative", "Standard"], "67 fields"] | [["Annotative", "Standard"], "67 fields"] |
| appids | APPID 表名字（真值超集，DXF 可能补写） | 对象层 | pass | ["ACAD", "ACAD_MLEADERVER", "ACAD_NAV_VCDISPLAY", "AcadAn... | ["ACAD", "ACAD_MLEADERVER", "AcadAnnotative", "REVIT"] |
| unknown | 未知类型占比 | 对象层 | pass | 0 | {"unknown": 0, "total": 290, "ratio": 0.0} |

- 几何最大绝对误差: 7.276e-11 （容差 1e-06）

## 楼层平面-强电.dwg
- 状态: **pass**；执行项 19，失败 0

| 编号 | 比对项 | 根因层 | 状态 | 期望 | 实际 |
|---|---|---|---|---|---|
| version | 文件版本 | 容器层 | pass | "AC1027" | "AC1021" |
| header | HEADER 变量（按 $VAR 全量逐值） | 容器层 | pass | "DXF 中 211 个可比 $VAR 逐值相等" | "逐值相等 209 / 断言 211" |
| classes | 类表 AcDb:Classes：类名 + 实体/对象标志 | 对象层 | pass | {"count": 12, "rows": 12} | {"count": 12, "rows": 12} |
| objects.coverage | 对象层覆盖率：无未知类型、无解码报错 | 对象层 | pass | 0 | {"total": 29657, "unknown": 0, "error": 0} |
| dictionaries | DICTIONARY 字典树：键名逐个 + 句柄全部可解析 | 对象层 | pass | "DXF 字典 189 个 / 键 87 个（去口径差）" | "解析器字典 212 个 / 键 87 个，未解析句柄 0" |
| linetypes | LTYPE 表：名字/描述/图案段长度/段标志 | 对象层 | pass | 7 | 7 |
| count.model | 模型空间顶层实体总数 | 对象层 | pass | 1635 | 1635 |
| count.block | 块内实体总数 | 对象层 | pass | 20307 | 20307 |
| type.model | 模型空间按类型计数 | 对象层 | pass | {"DIMENSION": 1, "INSERT": 130, "LINE": 1339, "MTEXT": 165} | {"DIMENSION": 1, "INSERT": 130, "LINE": 1339, "MTEXT": 165} |
| type.block | 块内按类型计数 | 对象层 | pass | {"ARC": 768, "CIRCLE": 137, "ELLIPSE": 5, "HATCH": 162, "... | {"ARC": 768, "CIRCLE": 137, "ELLIPSE": 5, "HATCH": 162, "... |
| layers | 图层名集合 + ACI 颜色 + 线型 | 对象层 | pass | ["0", "101-AR-LAY-L1_t3$0$1", "101-AR-LAY-L1_t3$0$10", "1... | ["0", "101-AR-LAY-L1_t3$0$1", "101-AR-LAY-L1_t3$0$10", "1... |
| blocks | 块名集合 + 块基点 + 匿名标志 | 引用层 | pass | ["*Model_Space", "*Paper_Space", "*Paper_Space0", "DIMBLO... | ["*Model_Space", "*Paper_Space", "*Paper_Space0", "DIMBLO... |
| geometry | 按 handle 配对几何误差（坐标/角度/bulge） | 模型层 | pass | {"matched": 21942, "only_truth": 0, "only_parser": 0} | {"matched": 21942, "only_truth": 0, "only_parser": 0} |
| text | TEXT/MTEXT 文字内容逐 handle 比对 | 对象层 | pass | 1443 | 1443 |
| dimension | DIMENSION 全字段：定义点/文字中点/界线/类型/样式/附着点/实测值/73-74-75/挤出方向 | 对象层 | pass | 1 | 1 |
| styles | STYLE 表：名字 + 字体 + 字高/宽/倾斜 | 对象层 | pass | 58 | 58 |
| dimstyles | DIMSTYLE 表：名字集合 + DIM* 变量 67 字段 | 对象层 | pass | [["Annotative", "Standard", "对角线_-_3mm_RomanD"], "67 fiel... | [["Annotative", "Standard", "对角线_-_3mm_RomanD"], "67 fiel... |
| appids | APPID 表名字（真值超集，DXF 可能补写） | 对象层 | pass | ["ACAD", "ACAD_MLEADERVER", "ACAD_NAV_VCDISPLAY", "AcadAn... | ["ACAD", "ACAD_MLEADERVER", "AcadAnnotative", "REVIT"] |
| unknown | 未知类型占比 | 对象层 | pass | 0 | {"unknown": 0, "total": 21942, "ratio": 0.0} |

- 几何最大绝对误差: 9.313e-10 （容差 1e-06）
