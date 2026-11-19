# PyArx 实测用法（AI 直接操作 CAD 的操作手册）

> 全部条目均在本机 AutoCAD 2024 + PyArxEngine.arx 实测，不是推测。

## 1. 运行环境

- PyArxCore 只能在 `acad.exe` 进程内 import；独立 python 进程 import 会 access violation。
- 入口：`(command "PYRUNFILE" "G:/xxx.py")`，或 `(ARXLOAD "G:/PyArx/X64/Release/PyArxEngine.arx")` 后同法。
- PYRUNFILE 脚本里的 `print` 走 CAD 命令行；需要回传结果就写 JSON 文件。
- 命令发送必须带**尾部空格**，并且要重试：GUI 忙时 `SendCommand` 抛 `RPC_E_CALL_REJECTED`。

## 2. 拿数据库（关键：不是 current()）

```python
db = P.Db.curDb()          # ✔ 当前文档 AcDbDatabase（getFilename 正确、entityIds 正确）
db = P.Db.PyAcadDatabase.current()   # ✘ 返回空库：getFilename 为空、entityIds 恒 0
```

`P.Db` 模块级还有：`lockDocument()` / `unlockDocument()` / `isDocumentLocked()`。

## 3. 写实体：必须走事务

```python
with P.Db.PyDbTrans() as tr:          # 构造即加写锁 + startTransaction
    pl = P.Db.PyDbPolyline()
    for i, (x, y) in enumerate(pts):
        pl.addVertexAt(i, P.Ge.AcGePoint2d(x, y))   # 点是 AcGePoint2d，不是 PyGePoint2d
    pl.setClosed(True)
    tr.addEntity(pl)                  # ✔ 追加到当前空间
# 退出 with 自动 commit 并解锁；异常时 abort
```

**坑**：
- `db.addToModelspace(pl)` / `space.appendEntity(pl)` 在只读打开 BTR 的情况下失败（`log\rawdb.log` 记 `btr open fail=82` = eWasOpenForRead）。用 PyDbTrans 就不踩。
- 读实体同理：`tr.openObject(obj_id, 0)`（0 读 / 1 写 / 2 通知）。

## 4. 取实体

```python
ids = db.objectIds("AcDbText")        # 按类名过滤，返回 ObjectId 列表
ids = db.objectIds("AcDbEntity")      # 全部
```

ADS 级（等价 LISP）：`P.Ed.handent("B9")`、`P.Ed.entget(ename)`（返回 `{组码: 值}`）、
`P.Ed.ssget("X", None, None, [(0, "TEXT")])`、`P.Ed.sslength/ssname/sssetfirst`、
`P.Ed.entmake(dxf)`、`P.Ed.entmod(dxf)`、`P.Ed.entdel(ename)`、`P.Ed.command(...)`。

**删除实体的坑（实测 2026-11-14）**：`P.Ed.handent("384")` 返回 `None`（不是 `AdsName`），
`P.Ed.entdel(None)` 抛 `TypeError: incompatible function arguments`。
可靠做法是事务内按句柄遍历后 `erase()`：

```python
db = P.Db.curDb()
with P.Db.PyDbTrans() as tr:
    for eid in db.objectIds("AcDbEntity"):
        o = tr.openObject(eid, 1)
        if str(o.handle()).upper().lstrip("0") == "384":
            o.erase()
```


## 5. 文字 → 几何（两条路径，按字体分）

| 字体 | 方法 | 说明 |
|---|---|---|
| SHX (`txt.shx`/`gbcbig.shx` …) | `AcGiTextEngine::tessellate` | 引擎精确矢量化；`ShxProbe.pyd` 已封装 `explodeTexts` / `tessellate` |
| TrueType (`.ttf`) | 引擎不支持（头文件注明 future work） | 必须走 GDI：`GetGlyphOutlineW(GGO_NATIVE)` 或栅格化+边界追踪 |

`tessellate` 回调语义：整段文字只回调一次，第 1 个 int = 折线条数 N，第 2 参数 = 长度 N 的顶点数数组，
第 3 参数 = 首尾相接的顶点数组（`AcGePoint3d`）。

SHX 的样式字体从 `AcDbTextStyleTableRecord::fileName()` / `bigFontFileName()` 取。
TTF 必须**按文字实际外框反算比例**（`getGeomExtents()`），不要按 cap height 归一化：
CAD 渲染比 1em 宽约 1.33×。

旋转文字（`rotation≠0`）的标定：把 em 轮廓按 `sx,sy` 缩放后旋转，用
`measW = rawW·sx·|c| + rawH·sy·|s|`、`measH = rawW·sx·|s| + rawH·sy·|c|` 解 `sx,sy`，
再把结果 AABB 平移对齐到实测 bbox 最小点。

## 6. 常用对象入口

- 块表：`db.getBlockTable()` → `bt.getAt("*Model_Space", 1)`（1=kForWrite）、`bt.names()`
- 建块定义（**踩坑点**）：`bt.add(btr)` 入库后会自动关闭记录（`isWriteEnabled()==False`），
  此时直接 `btr.appendAcDbEntity(ent)` 会触发 ARX `eNotOpenForWrite` 致命断言（无头 accoreconsole 直接崩进程）。
  正确写法是按 id 以写模式重开再 append，实体入库后自动关闭、**不要再补 `addNewlyCreatedDBObject`**：
  ```python
  with P.Db.PyDbTrans() as tr:
      bt  = tr.openObject(db.blockTableId(), 1)
      btr = P.Db.PyDbBlockTableRecord(); btr.setName("BLK")
      bt.add(btr)
      btr_w = tr.openObject(btr.objectId(), 1)      # 关键：重开
      btr_w.appendAcDbEntity(P.Db.PyDbCircle(pt, vec, 20.0))
  # 插块参照：bt.getIdAt("BLK") → PyDbBlockReference().setBlockTableRecord(id) → tr.addEntity(br)
  ```
  （绑定期已加防呆：未以写模式打开的块记录 append 只报错返回 False，不再进 ARX 断言。）
- 多段线：`PyDbPolyline()` 无参构造；`addVertexAt(i, AcGePoint2d, bulge=0, sw=-1, ew=-1)`、`setClosed(True)`
- 文字：`textString/height/rotation/widthFactor/oblique/position/textStyle/getGeomExtents`
- 句柄：`PyDbObject.handle()`

## 7. 保存（全部实测，结论：走 AcDbDatabase::saveAs）

优先级从高到低：

1. **`Document.database.saveAs(path)`（默认参数）** —— 同步落盘、不弹任何框，返回 `ErrorStatus.eOk`。
   注意：**不能传 `bBakAndRename=false`**，对文档库会返回失败（实测 `eUnknown`）。
   ```python
   d = P.Ap.PyApp.documentManager.curDocument
   db = d.database
   print(db.saveAs(d.name))     # -> ErrorStatus.eOk
   ```
2. **`Document.save()`**（PyArx 已内置）：取 `fileName()`，`.dxf` 走 `db->dxfOut(path)`，其余走 `db->saveAs(path)`。
   未命名图形（`fileName()` 为空）直接返回 `eInvalidInput`，要另存先用 saveAs。
3. **COM `Document.Save()`** —— 同样同步、无弹窗，且**不依赖 PyArx 插件是否加载**（外部 PowerShell 5.1 走
   `[Runtime.InteropServices.Marshal]::GetActiveObject("AutoCAD.Application")`）。

不要用：
- `_.QSAVE`：后台文档返回 eOk 但不落盘；
- `_.SAVEAS`：会停在"输入文件格式"提示上，把命令上下文挂死；
- `AcDbDatabase::save()`：对文档库固定返回 `eFileInternalErr(74)`；
- `Document.SaveAs("x.dwg")`：会产出 `x.dwg.dxf`。

## 8. 程序化驱动（外部进程）

```powershell
$app = [Runtime.InteropServices.Marshal]::GetActiveObject("AutoCAD.Application")
$app.ActiveDocument.Activate()
$app.ActiveDocument.SendCommand('(command "PYRUNFILE" "G:/x.py") ')   # 结尾带空格
```
- 必须用 PowerShell 5.1（pwsh7 没有 `GetActiveObject`）。
- PS1 里含中文必须以 **UTF-8 带 BOM** 保存，否则按 ANSI 读取、中文字面量静默失效。
- `Documents.Open(.bak)` 不行，先 `Copy-Item` 成 `.dwg`。

## 9. 热编译 pyd（改 C++ 不重启 CAD）

```powershell
powershell -File G:\PyArx\tools\hotbuild\hotbuild.ps1 -Tag h1    # 增量约 13 秒
powershell -File G:\PyArx\tools\cadmcp\cad_send.ps1 -PyFile G:\PyArx\tools\hotbuild\reload.py
# 或者直接调 cadmcp 的 pyarx_hotbuild 工具
```

要点：
- **热替换前必须先清空已导入模块**：把 `sys.modules` 里所有 `PyArxCore` / `PyArxCore.*` 摘掉
  （`reload.py` 已实现，旧对象存进 `__pyarx_old__` 保活），再 import 新 `PyArxCoreH<tag>`。
  不清就直接 import，旧模块里活跃的对象会撞上 pybind11 的
  `type_caster_base.h:293 / itypes->empty()` 断言，弹出模态框把 CAD 挂死。
- 模块名必须真改（编成 `PyArxCoreH<tag>.pyd`），因为 pybind 要求 `PyInit_<stem>` 与文件名一致；
  名字来自 `PyArxCore\HotModName.h`，只被 `PyArxBinder.cpp` 包含 → 只重编该 TU + 链接。
- 类型重复注册靠入口处清 `registered_types_cpp/py`，并要求 `PYBIND11_INTERNALS_KIND` 恒定（`_dev`），
  恒定值才不会每次全量重编。
- 入口必须把每个子模块也登记成 `sys.modules["PyArxCore.X"]`，否则 binder 内部的
  `py::module_::import("PyArxCore.Ge")` 会失败。
- 新旧两代模块是两套 C++ 类型：跨代对象不能互传，热替换后整段脚本重跑。
- 详见 `G:\PyArx\tools\hotbuild\README.md`。

## 10. 正式编译与开机自加载

- `PyArxCore.pyd` 被运行中的 `acad.exe` 占用时，MSBuild 链接会 `LNK1104: 无法打开文件 ...PyArxCore.pyd`，
  而且旧文件可能已被删。**顺序必须是：先关 CAD → 编译 → 再启动 CAD**（MCP：`pyarx_build` + `pyarx_restart_cad`）。
- 开机自加载插件：注册表
  `HKCU\Software\Autodesk\AutoCAD\R24.3\<ACAD-xxxx:xxx>\Applications\PyArx`
  写 `LOADER=<arx 全路径>`、`LOADCTRLS=0x2`、`MANAGED=0`。
  脚本 `G:\PyArx\tools\autoload\set_autoload.ps1`（`-Status` / `-Remove` / `-Loader`），MCP 工具 `pyarx_autoload`。
- 验证方式（无需 GUI）：命令行发 `_.COPYHIST `，再读剪贴板即可看到启动日志。

## 11. 命令行输出乱码（中文变"耪壹鞣…"）

`acutPrintf` 的格式串是 `ACHAR`（宽），`%s` 吃的是**宽串**。把 `char*`（UTF-8 窄串）直接喂进去，
会被当成 `wchar_t*` 读取 → 整段 CJK 乱码（典型症状：`PYBIND11_MODULE(耪壹鞣鲐鞣簧澶基开_efx_)`）。
所有窄串先转宽再传：

```cpp
namespace { std::wstring W(const char* s); }   // MultiByteToWideChar(CP_UTF8, ...)
acutPrintf(L"\n[PyArx] PYBIND11_MODULE(%s) 入口执行\n", W(selfName).c_str());
```
`std::wofstream` 写日志同理，`char*` 不能直接 `<<`。

