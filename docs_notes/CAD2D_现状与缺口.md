# CAD2D 现状与缺口（examples/cad2d）

## 启动命令
- 正常启动：`cd G:\PyArx\X64\Release; .\python.exe G:\PyArx\examples\cad2d\app.py`
- 既有自检：`cd G:\PyArx\X64\Release; .\python.exe G:\PyArx\examples\cad2d\app.py --selftest`
- 全批实测：`cd G:\PyArx\X64\Release; .\python.exe G:\PyArx\examples\cad2d\app.py --batch`

## 能力清单
### 已实现（真实宿主内实测通过）
- 绘制命令：直线、圆、弧、矩形、多段线（interact 命令状态机 + 画布定点 + 提交入库）。
- 选择与修改：点选、窗口框选（含可视橡皮框）、夹点拖动、MOVE、ROTATE、SCALE、ERASE、选中集改图层、改颜色（COPY 命令已实现，本批 16 用例未单独断言）。
- 视图：滚轮以光标为锚点缩放、全图适配、中键拖拽平移、对象捕捉（端点 / 中点 / 圆心 / 象限 / 最近点）；CAD 风格自绘十字光标（贯穿视口横/竖线 + 中心小空心方框、交点居中对齐，静止隐藏系统指针；中键按住平移期间自绘十字隐藏、显示握紧爪型，松开恢复十字）；窗口尺寸变化经 `OnSize→reflowXml` 自动重排，主画布吃剩余宽高，侧栏/命令行/状态栏固定或横向跟随；SVG 图纸场景的平移/缩放视图在重排时保留，不被 fit 清零。
- 图层与颜色：图层符号表（颜色 / 线宽 / 可见 / 锁定）、图层表经 `setTableRows` 真实刷新（行间 `\n`、格间 `\t`）。
- 块：`BLOCK [块名]` 选对象 + 基点写块表记录（深拷贝、原图保留，未给名自动编号）；`INSERT [块名] [比例] [角度]` 落点放置块参照（默认最近块、1:1、0°）；块内实体经基点 + 缩放 + 旋转变换后渲染；块定义纳入撤销 / 重做栈；块表随图纸 JSON 存盘 / 读盘往返保留。
- 撤销 / 重做：`history.UndoStack` 覆盖 ADD / REMOVE / MODIFY 三类，命令行 UNDO / REDO 与工具动作已接线（本批 16 用例未单独断言撤销栈，仅链路接通）。
- 持久化：图纸 JSON 存盘 / 读盘，图幅、当前图层、图层全字段、图元几何 / 图层 / 颜色随存随还原。
- 栅格：`grid_on` 开关联动画布 SVG 栅格，随缩放 / 平移重绘。
- 既有基线能力保留：文字图元、属性栏、自绘工具条画布与命中表、命令行（命令名 / 别名 / 中文 / 坐标）、SVG 导出、PNG 截图。
- 外壳 UI 扁平化（2026-11-15）：右栏特性/视图只读行、命令标签/提示改用 `XDwgCanvas` 自绘 SVG（`ui.props_panel / ui.text_bar`），深色下面板底色、标签次色、值主色右对齐；原生 Label/Button 的外观属性在当前底层不生效（见 `_API_CHANGES.md` 12–14），仅 `XTextField` 与「执行」按钮保留原生。
- 特性面板分组（2026-11-15）：按 CAD 习惯分「常规 / 几何图形」两组（暂不做三维效果）；常规出颜色 / 图层 / 线型 / 线宽，几何按图元类型出明细——直线（起终点 / 增量 / 长度 / 角度）、圆（圆心 / 半径 / 直径 / 周长）、弧（圆心 / 半径 / 起止角 / 夹角 / 弧长）、矩形（角点 / 宽高 / 周长）、多段线（顶点数 / 闭合 / 周长）、文字（位置 / 内容 / 字高 / 旋转 / 样式）、块参照（块名 / 插入点 / XY 比例 / 旋转）。
- 面板可编辑（2026-11-15）：点值行选中字段后，原生下拉改图层 / 颜色 / 线型，原生输入框改线宽与几何标量（坐标 / 半径 / 角度 / 字高 / 比例 / 文字内容），写回前快照入撤销栈，可 UNDO/REDO；块名 / 多段线顶点暂为只读。下拉选图层 / 颜色 / 线型即时生效，数值经「应用」按钮提交；控件显隐与条目在 Python 侧切换。
- 功能区下沉原生控件库（2026-11-15）：顶部 Ribbon 由自绘 SVG 画布改为控件库 `xmlctl::XRibbon`（模型 `ribbonParse` / 布局 `ribbonLayout` / 绘制 `ribbonDraw` / 命中 `ribbonHitTest` 成套复用）；PyMfcXml 新增 `XRibbon` 控件类与宿主 `addRibbon / setRibbonElement / drawRibbon / ribbonMouse / fireRibbonButton`，XML 声明 `<XRibbonTab/Group/Button>`，按钮 `Name` 经 `onClick` 回传 Python 派发命令。左工具条 / 状态栏仍为自绘 `XDwgCanvas`。

### 已接线命令
- 线性标注 DIM：三点（第一原点 → 第二原点 → 尺寸线位置）生成 `ent.Dimension`，第三点投影到 p1p2 法向得垂直偏移；渲染出延伸线 / 尺寸线 / 箭头 / 长度文字；命令行别名「标注」，Ribbon 标注按钮派发。
- 区域填充 HATCH：点选闭合边界（矩形 / 闭合多段线）生成 `ent.Hatch`（边界顶点 + 角度 + 间距）；底层 nanosvg 不支持 `pattern/clipPath`，渲染改由 Python 扫描线在多边形内求交、成对输出剖面 `<line>`；命令行别名「填充」。当前仅支持矩形 / 闭合多段线边界，圆 / 弧围成区域未纳入。

### 未实现（停止在当前状态，不计通过）
- 物理 Esc 键 / 右键取消：底层未派发键盘与右键事件，仅命令行 ESC / 点空白兜底。
- DWG 读写：仅完成路线调研（见末节），未引入任何 DWG 库、未写解析器。
- 圆 / 弧围成区域的 HATCH：扫描线兜底当前只接多边形边界，曲线边界未离散纳入。

## 与既有 examples/cad2d 内容的继承关系
- 既有 `app.py / doc.py / geom / render / db / ent / interact / cmdline / ui / cad_shell.xml / res` 全部保留，未重写、未删除或改名用户文件。
- 改动方式为追加：实体新增方法（`snapshot / to_dict / from_dict / set_grip / rotate_by / scale_by`）；`render.to_svg(...)` 新增参数均带默认值，既有调用兼容；`history.py` 为新增模块；`doc.restore() / to_dict() / load_dict()` 为新增门面方法。
- 既有 `--selftest` 断言未改，回归通过；`docs/_*.png / _*.txt` 历史产物未触碰。
- C++ 绑定与控件库（PyArxCore / PyArxEngine / MfcXmlCore / PyMfcXml）本轮零改动；底层缺口只在 `docs/_API_CHANGES.md` 登记，Python 侧兜底。

## 已知缺口（接口层，详见 docs/_API_CHANGES.md，共 5 条）
1. XDwgCanvas 未派发键盘事件与右键事件，Esc / 右键取消无底层入口。中键已接入：WM_MBUTTONDOWN/UP 转发到画布，按住中键拖拽平移，按下为握紧爪型；静止时系统指针隐藏，由 Python 在 SVG 上自绘贯穿视口的十字线 + 中心方块（CAD 风格），鼠标 move 时同步 C++ 视图偏移供命中换算。
2. 画布控件尺寸 API 缺失，`getSize(id)` 未绑定，fit 走兜底尺寸。
3. XML `XTableView` 的 `tableViewRows` 视口行计数返回 0（写入与读取单元格正常）。
4. SVG 场景（nanosvg）只识别纯色 / 渐变填充，`pattern / clipPath / fill-rule` 不支持；HATCH 已改用 Python 扫描线剖面线兜底，不再依赖该链路。
5. `setTableRows(id, "格\t格\n行\t行")` 分隔符契约已探明并备注。

## 图元增改测试结果
> 执行方式：真实宿主会话（`X64\Release\python.exe` 启动 app、走消息泵与画布事件、命令状态机，无 mock）；一次跑完整批，失败项当场修后仅重跑失败项。
> 界面观察：`cad2d_batch.png`（批测后）与 `cad_shot.png`（既有 selftest 后）均为干净本应用窗口，Ribbon / 左工具条 / 特性栏 / 图层表 / 视图栏 / 命令行 / 状态栏齐全，未见闪烁、拖影、布局错乱或文字残影。

| 用例 | 预期 | 实际 | 通过 | 失败根因 |
| --- | --- | --- | --- | --- |
| 直线绘制 | line (1000,1000)-(3000,1200) | line (1000,1000)-(3000,1200) | 通过 | — |
| 圆绘制 | circle 圆心(5000,2000) r=800 | circle (5000,2000) r=800.0 | 通过 | — |
| 圆弧绘制 | arc 三点成弧，半径>0 | arc 圆心(7500,1244) r=556.2 | 通过 | — |
| 矩形绘制 | rect 对角 (1000,3000)-(2500,4000) | rect bbox=(1000,3000,2500,4000) | 通过 | — |
| 多段线绘制 | polyline 3 个顶点 | polyline 顶点数=3 | 通过 | — |
| 夹点拖动 | 直线终点移到 (3200,1600) | 终点=(3200.0,1600.0) | 通过 | — |
| 移动 MOVE | 整体偏移 (+500,+300) | 起点=(1500.0,1300.0) | 通过 | — |
| 旋转 ROTATE | 绕起点相对旋转 90°，长度不变 | 15.3°→105.3°（Δ90.0°），长度 2280.4→2280.4 | 通过 | — |
| 缩放 SCALE | 圆半径 800 → 1200 | r=1200.0 | 通过 | — |
| 删除 ERASE | 圆被移除，图元数 -1 | 剩余 4 个，圆存在=False | 通过 | — |
| 改图层 | 直线图层 = TESTLAYER | layer=TESTLAYER | 通过 | — |
| 改颜色 | 直线颜色索引 = 3 | color=3 | 通过 | — |
| 窗口选择 | 框内至少选中 1 个图元 | 选中 3 个 | 通过 | — |
| 存盘读盘往返 | 数量 / 类型 / 图层 / 颜色 / 几何一致 | 存 4 / 读 4，逐项一致 | 通过 | — |
| 线性标注 DIM | 三点线性标注实体 | 未实现 | 未实现 | Dimension 实体与渲染桩就位，DIM 命令状态机未接线 |
| 区域填充 HATCH | 闭合区域图案填充 | 未实现 | 未实现 | SVG pattern/clipPath 能力未验证，命令未接线 |

汇总：用例 16 · 通过 14 · 失败 0 · 未实现 2。

## 后续建议
- DIM 复用 interact 命令状态机补三点定点，实体与渲染分支已就绪。
- HATCH 待 A 线确认 SVG pattern / clipPath 能力后再做，或退回纯 Python 多边形裁剪填充。
- Esc / 右键取消、`getSize(id)`、表格视口行计数等待 A 线补底层 API；示例不反向改 C++。
- DXF 读写作为持久化下一步（见末节选型），DWG 不做原生解析。

## DWG 导入路线与开源库选型
- 两级路线：一级 DXF（原生可写、成本低、优先落地）；二级 DWG（只读优先，靠外部转换器或商业 SDK）。
- LibreDWG（C，GPLv3）：读 R13-R2000 稳、R2004+ 部分支持、写不完整。
- libdxfrw（C++，GPLv2）：读 DWG、写 DXF。
- ACadSharp（C#/.NET，MIT）：读 DWG，读写 DXF。
- ezdxf（Python，MIT）：仅 DXF 读写，2D 首选。
- ODA Drawings SDK（C++，商业授权）：DWG 读写最全。
- ODA File Converter（免费工具）：DWG↔DXF 批处理，适合进程外转换。
- 许可红线：GPL 系（LibreDWG、libdxfrw）不得链进闭源产品，只能进程外调用且分发受限，需用户决策。
- 推荐路线：DXF 用 ezdxf(MIT) 或自研子集解析器落地；DWG 先用 ODA File Converter 进程外转 DXF 再走同一解析管线；写 DWG 不做。
- 原生读 DWG 最小知识面：版本标识 AC10xx、段表、对象映射与 handle、R2004+ 段页压缩与 MC 解压、实体公共属性 handle/owner/layer/color/linetype、几何实体类名 AcDbLine/AcDbCircle/AcDbArc/AcDbPolyline/AcDbText/AcDbBlockReference/AcDbDimension、块嵌套变换、多段线 bulge 弧段、ACI→RGB、代理图形回退。
- 本节为路线建议，未实现代码，落地需等用户选定授权路线。
