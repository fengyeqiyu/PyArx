# ACAD 精简清单（实测：crx 真负载 probe 通过）

原目录 `G:\PyArx\ACAD` 共 5351 文件 / 2.84 GB。
判定方法：`accoreconsole` + `(arxload PyArxEngine.crx)` + `(command "PYRUNFILE" <probe.py>)`，
以 probe 写出的标记文件是否存在为准。整组删除后 probe 仍通过 = 该组可删。

## 可删：3313 文件 / 1777.14 MB

| 类别 | 文件数 | 体积 (MB) |
|---|---|---|
| `.shx 形/字体文件` | 2202 | 1048.99 |
| `dll:qt*` | 22 | 155.55 |
| `.onnx AI 模型` | 2 | 142.28 |
| `.hdr 环境贴图` | 15 | 74.97 |
| `.otf 字体` | 30 | 62.29 |
| `.hdi 打印/绘图驱动` | 32 | 41.59 |
| `.arx ObjectARX 插件（镜像自带，非本引擎依赖）` | 125 | 35.05 |
| `.exe 其它可执行` | 31 | 33.39 |
| `.js 面板脚本` | 45 | 28.70 |
| `.xmx 材质索引` | 2 | 27.82 |
| `dll:adsk*` | 25 | 27.72 |
| `.dwg 示例图纸` | 104 | 18.37 |
| `.pak 面板资源包` | 18 | 15.36 |
| `.ogsf 材质库` | 1 | 15.32 |
| `.drc 材质数据` | 10 | 12.73 |
| `.dat 缓存数据` | 4 | 10.54 |
| `dll:aec*` | 60 | 7.85 |
| `.qm Qt/Autodesk 翻译包` | 83 | 3.98 |
| `.txt 说明文本` | 34 | 3.47 |
| `.lsp LISP` | 149 | 2.97 |
| `.xml ecschema/配置` | 33 | 2.83 |
| `.dvb VBA` | 16 | 1.83 |
| `.ttf 字体` | 8 | 1.01 |
| `.map 符号映射` | 9 | 0.67 |
| `.json UI 布局` | 28 | 0.49 |
| `.bmp 位图` | 21 | 0.48 |
| `.dcl DCL 对话框` | 21 | 0.21 |
| `.fas LISP 编译` | 5 | 0.18 |
| `.png UI 图标` | 58 | 0.17 |
| `.h C++ 头文件` | 58 | 0.08 |
| `.vlx LISP 包` | 3 | 0.05 |
| `.html 帮助页` | 14 | 0.05 |
| `.htm 帮助页` | 12 | 0.05 |
| `.css 样式` | 10 | 0.04 |
| `.ctb 打印样式表` | 9 | 0.04 |
| `.pc3 打印配置` | 11 | 0.02 |
| `.pmp 打印样式` | 2 | 0.01 |
| `.mln 菜单` | 1 | 0.00 |

## 必须保留：2038 文件 / 1126.56 MB

引擎可运行最小集（实测）：

- `accoreconsole.exe`
- 基础加载模块 12 个：`adhostableui / UserData / ac1st24 / AcPal / acdb24 / AcGe24 / AcDs` + CRTs
- OGS 图形栈 4 个：`ogsglobals / OGSDevices / OGSGraphics / OGSObjects`（合计 21.4 MB，删了引擎起不来）
- `accore / acadres / acbol / acdb24res / AcDrawBridge / AcUt / AdImaging / AdIntImgServices / dswhip / hcreg16 / heidi16 / plotcfg16 / tbb / tbbmalloc`
- `.crx` 78 个（40.2 MB，含 `acismui.crx`，缺了 arxload 静默失败）
- `.dbx` 39 个（46.1 MB，DWG 数据库支撑）
- 其余非 dll 配置/资源（体积小，见上方未列入可删者）
