# cadmcp —— 让 AI 直接操作 AutoCAD（PyArx 驱动）

一个 stdio（+ HTTP）MCP 服务器。外部 AI（任意 MCP 客户端）通过下列工具操纵**正在运行**的 AutoCAD、并读取 PyArx 的知识源：

| 工具 | 作用 |
|---|---|
| `pyarx_run(code, timeout_sec, doc)` | 在 acad.exe 内执行 Python/PyArx 代码，回传 result / stdout / 异常 |
| `pyarx_run_file(path, ...)` | 同上，代码来自 .py 文件 |
| `pyarx_api_search(query, limit)` | 检索 PyArxCore 全量 API 清单（6.6 MB，离线） |
| `pyarx_guide()` | 返回实测用法要点（`docs/PYARX_USAGE.md`） |
| `pyarx_hotbuild(tag)` | 热编译 → **先清空已导入模块** → 导入新 pyd → 接口自检（增量约 13 秒） |
| `pyarx_build(target)` | 正式编译基础 pyd（`X64\Release\PyArxCore.pyd`，arx 启动加载的就是它） |
| `pyarx_restart_cad()` | COM 存盘 → Quit → 重启 acad.exe → 轮询 COM 就绪 |
| `pyarx_save()` | 存盘：首选 COM `Document.Save()`，失败退回 PyArx `db->saveAs/dxfOut` |
| `pyarx_saveas(path)` | 另存为：`AcDbDatabase::saveAs`（同步落盘，不弹"另存为"、不问格式） |
| `pyarx_docs()` | 列文档（COM 优先，失败退回 PyArx） |
| `pyarx_quit_cad(save_first)` | 存盘后退出 CAD（COM 通道，无弹窗） |
| `pyarx_com(action)` | 纯 COM：docs / save / quit（不依赖 PyArx 是否已加载） |
| `pyarx_autoload(mode, loader)` | CAD 启动自加载插件（注册表 `Applications\PyArx`，`LOADCTRLS=0x2`） |
| `pyarx_projects(query)` | 全部工程/目录的职责地图（`docs/PYARX_PROJECTS.md`；query 为空给全文，非空按章节过滤） |
| `cad_*`（14 个） | 业务工具层：画图元 / 建层 / 批量 / 统计 / 查询 / 删除，见下文「cad_* 业务工具层」 |

**通道选择原则**：
- 要读写图形数据库、跑 PyArx API → `pyarx_run`（插件已加载才行）。
- 只要存盘 / 退出 / 列文档 → 走 COM（`pyarx_save` / `pyarx_quit_cad` / `pyarx_com`），插件没加载也能用，且不会弹模态框。
- 改完 C++ 只想立刻生效 → `pyarx_hotbuild`；想连"下次启动"都生效 → `pyarx_build` + `pyarx_restart_cad`。

## cad_* 业务工具层（模型首选入口）

`pyarx_run` 让模型直接写 pybind：事务、`kForWrite`、`AcGePoint3d`、句柄时机、`btr open fail=82` 全压在模型身上，写成率低。`cad_ops.py` 把这些收敛成受约束接口——模型只填坐标、字高、图层名，其余由工具层兜住。

| 工具 | 作用 |
|---|---|
| `cad_line(p1, p2)` | 直线 |
| `cad_polyline(pts, closed)` | 轻量多段线 |
| `cad_text(pos, text, height, rotation_deg)` | 单行文字（角度传度，工具层转弧度） |
| `cad_mtext(pos, text, height, width)` | 多行文字 |
| `cad_circle(center, radius)` | 圆 |
| `cad_arc(center, radius, start_deg, end_deg)` | 圆弧 |
| `cad_hatch(pts, pattern, scale)` | 填充（边界点数组） |
| `cad_table(pos, rows, cols, texts, row_h, col_w)` | 表格 |
| `cad_block_ref(name, pos, scale, rotation_deg)` | 块参照（只引用已存在块定义） |
| `cad_layer(name, rgb)` | 建图层 |
| `cad_batch(ops)` | **推荐**：多个操作打包成一次事务，全有或全无 |
| `cad_stats(max_scan)` | 只读：实体总数 + 分类/图层分布 |
| `cad_query(cls, layer, limit)` | 只读：按类名/图层列实体句柄 |
| `cad_erase(handles/layer/all, confirm)` | 删除（`all=true` 必须同时 `confirm="ERASE_ALL"`） |

统一返回契约（模型据此自纠，不靠猜）：

```json
{"ok": true, "count": 6, "handles": ["732", "..."], "steps": [...], "layers": {...},
 "warnings": [...], "before": 0, "after": 6,
 "failed_index": -1, "errors": [], "rolled_back": false}
```

一句代码生成 + `ast.parse` 自检，语法错误不会送进 CAD；批量失败时 `handles` 清空、`rolled_back=true`、`failed_index` 指向失败的第几步。真实错误在事务内先被捕获——`PyDbTrans.__exit__` 在 abort 时会再抛 `Caught an unknown exception!`，会盖掉原始错误。

- **保存图形（`db.saveAs`）的两个崩溃点**，都已修：
  1. `PyDbDatabase::saveAs` 原实现把 `nullptr` 当 `SecurityParams*` 传给 `AcDbDatabase::saveAs`，acdb24.dll 内部解引用空指针 → `0xc0000005`（偏移 `0x6f414`），现象是 `SendCommand rejected` 且随后进程消失。改成没有安全参数时走**不带 SecurityParams** 的重载。
  2. 工具层建层时在**事务外**打开图层表拿写锁，之前没有 `close()`，库带着"符号表打开写"的状态去存盘：headless 里返回 `eWasOpenForWrite`，文档库里直接崩。现在建层后必定 `lt.close()`（块表同理）。
  验证：空图 `saveAs` → `eOk`（32,307 字节）；2 个无层实体 → `eOk`（32,499）；4 个实体（含 `LIVE_TEST` 层 2 个）→ `eOk`（37,263 字节，entities=4）；关闭 CAD 重开后打开该 dwg，`stats` 为 `AcDbLine:2 / AcDbCircle:1 / AcDbText:1`、`by_layer` 为 `LIVE_TEST:2 / 0:2`、`query LIVE_TEST` 命中 2。
- **`document.save()` 只对已命名文档有效**：未命名图形 `fileName` 为空，直接返回 `eInvalidInput`，要先 `saveAs` 给个路径。
- **`PyApDocument.save()` 内部也走 `db->saveAs` 单参数**，和上面第 1 点同一路径，修完 pyd 后才稳定。

### 实现要点与限制（宿主实测）

- **图层**：`setLayer(name)` 走 `AcString::kwszPtr()` 转 `ACHAR`，对**已存在**的图层也返回 `False`（98 层图上实测）。已在绑定层补 `PyDbEntity::setLayerId(uint64_t)`，改用 id 版 `AcDbEntity::setLayer(AcDbObjectId)`，绕开名字编码。实测已存在层与新建层都返回 `True` 且读回一致。工具层自动探测：有 `setLayerId` 就用它，没有才退回字符串重载（并如实记 warning）。
- **块参照**：`setBlockTableRecord(uint64_t)` 本身可用（`objectId()` 返回的就是 oldId uint64）。真正的坑有两个：① 块表必须**事务外**打开，事务内 `getSymbolTable(kForWrite)` 报 `eWasOpenForWrite`；② `xref` 派生块（名字含 `$0$`）被 `appendAcDbEntity` 拒绝。工具层已把块表前置打开并在结束时 `close`，缺失/被拒都返回明确错误。
- **`tr.addEntity` 返回 bool 且失败不抛异常**：块参照被拒时实体数不增、句柄停在 `"0"`。工具层现在校验返回值，并按 `handle=="0"` / 实体数未增加做二次判定，这类"静默没画上"会进 `warnings` 且不计入 `count`。
- **块定义仍不可建**：`PyDbBlockTable.add` 事务外断言/挂死，所以 `cad_block_ref` 只能引用当前图**已有**的块定义。

端到端验证：`examples/mid_headless_cadops_test.py`——无头写 6 图元 → 统计 → 按句柄删 1 → 复查（实测 6 实体、句柄 732–737、`by_layer` 为 `CADOPS_TEST:3 / 0:3`、删除后 5）。另有 `verify_setlayerid.py`（id 版设层）、`verify_cadops_layer.py`（工具层带层全链路）、`verify_block.py`（块引用）。



## 组成

```
cadmcp/
  server.py      MCP stdio 服务端（JSON-RPC 2.0，无第三方依赖）；知识源：docs\PYARX_PROJECTS.md / PYARX_USAGE.md / pyarx_api_raw.json
  bridge.py      在 acad.exe 内跑的工人：读 req.json → exec 代码 → 写 res.json
  cad_send.ps1   COM 附加 + SendCommand(PYRUNFILE) + 等结果（带重试）
  req.json/res.json  一次请求/响应的载体（server 每次覆盖写）
```

链路：`MCP client → server.py → PowerShell → SendCommand → bridge.py in acad.exe → PyArxCore`。

`bridge.py` 里预置了 `P = PyArxCore`，代码把最终结果赋给变量 `result` 即可回传。

## 用 MCP 向 CAD 写脚本并运行

三种方式，按"要不要拿到结果"和"要不要在 CAD 命令行留输出"选：

| 方式 | 调用 | 结果回传 | 输出去处 |
|---|---|---|---|
| 内联代码 | `pyarx_run(code=...)` | `result` 变量 + 捕获的 stdout | 返回体里的 `out` |
| 脚本文件 | `pyarx_run_file(path=...)` | 同上 | 返回体里的 `out` |
| CAD 原生 | `run_script.ps1 -PyFile x.py`（发 `(command "PYRUNFILE" ...)`） | 不捕获 | AutoCAD 命令行 |

前两种把脚本源码注入 `acad.exe` 执行，能拿到异常栈；第三种是 CAD 自己的脚本入口，`print` 直接出现在命令行，适合带交互或长任务。

```python
# 一次典型写入（务必走事务，否则 btr open fail=82）
code = '''
with P.Db.PyDbTrans() as tr:
    pl = P.Db.PyDbPolyline()
    for i, (x, y) in enumerate([(0,0), (500,0), (500,500), (0,500)]):
        pl.addVertexAt(i, P.Ge.AcGePoint2d(x, y))
    pl.setClosed(True)
    tr.addEntity(pl)
result = "ok"
'''
```

**删除实体**：不要用 `P.Ed.entdel(P.Ed.handent("384"))` —— 实测 `handent` 返回 `None`，
`entdel` 要的是 `Ed.AdsName`，会抛 `TypeError: incompatible function arguments`。
可用做法是按句柄遍历 + 事务内 `erase()`：

```python
db = P.Db.curDb()
with P.Db.PyDbTrans() as tr:
    for eid in db.objectIds("AcDbEntity"):
        o = tr.openObject(eid, 1)          # 1 = 写
        if str(o.handle()).upper().lstrip("0") == "384":
            o.erase()
```

`handle()` 是十六进制字符串，比对前统一大写并去前导 0。

**注意**：`pyarx_run` / `pyarx_run_file` 每次调用都是**全新命名空间**，上一轮定义的函数/变量不会留到下一轮。
需要跨调用保留状态，就把状态落在文件（如 `G:\PyArx\log\*.json`）或 `sys.modules` 里；
脚本要复用就直接写成 `.py` 文件用 `pyarx_run_file`，不要靠连续内联调用拼状态。

`scripts\cad_inspect.py` 是一个可直接跑的样板（只读统计当前图纸），
用法 `pyarx_run_file(path="G:/PyArx/tools/cadmcp/scripts/cad_inspect.py")`。

## 开机自启

登录桌面即拉起 HTTP 服务，不弹窗：

- 启动器：`start_http.vbs`（`wscript` 以隐藏窗口跑 `python server_http.py --port 8765`，已在监听则直接退出）
- 已放入登录启动目录：`%APPDATA%\Microsoft\Windows\Start Menu\Programs\Startup\cadmcp-http.lnk`

手动起停：`wscript G:\PyArx\tools\cadmcp\start_http.vbs`；停：找到监听 8765 的 python 进程结束即可。
要排查是否在跑：`Get-NetTCPConnection -LocalPort 8765 -State Listen`。

（曾试过 Windows 计划任务，`powershell.exe` 动作在该机器的任务执行环境下启动即失败（`LastResult=4294967295`），已弃用并删除任务。）

## 两个入口

| 入口 | 用法 | 适用 |
|---|---|---|
| `server.py`（stdio） | MCP 客户端用 `command` + `args` 直接拉起 | Claude/Cursor 等支持本地 stdio 的客户端 |
| `server_http.py`（streamable HTTP） | `python server_http.py --port 8765`，端点 `POST /mcp`，`GET /health` | 豆包等只接受 `transport_type=2 + url` 的导入通道 |

HTTP 版只是 `server.py` 的传输层封装，工具与行为完全一致（复用同一份 `TOOLS` 与 `dispatch`），默认只绑 `127.0.0.1`。
后台常驻用 `start_http.cmd`（隐藏窗口拉起）。

自检：`python _selftest.py`（stdio）、`python _selftest_http.py`（HTTP，含一次真实 `pyarx_run`）。

## 注册到 MCP 客户端

```json
{
  "mcpServers": {
    "cad": {
      "command": "G:\\PyArx\\X64\\Release\\python.exe",
      "args": ["G:\\PyArx\\tools\\cadmcp\\server.py"]
    }
  }
}
```

注意：`python.exe` 只跑 server 端（不需要 PyArxCore）；真正的 PyArx 代码在 CAD 里执行。

## 知识工具

- `pyarx_guide()`：实测用法（入口、已知坑、推荐范式），源文件 `docs\PYARX_USAGE.md`。
- `pyarx_projects(query)`：全部工程/目录的职责地图（谁负责什么、产物、依赖、边界、改动落点），
  源文件 `docs\PYARX_PROJECTS.md`。改代码前先调 `pyarx_projects(query="PyMfcXml")` 之类定位归属。

## 典型用法

```python
# 1) 清点
code = '''
db = P.Db.curDb()
result = {"file": db.getFilename(), "ents": len(db.objectIds("AcDbEntity"))}
'''

# 2) 建一条闭合多段线
code = '''
with P.Db.PyDbTrans() as tr:
    pl = P.Db.PyDbPolyline()
    for i, (x, y) in enumerate([(0,0), (500,0), (500,500), (0,500)]):
        pl.addVertexAt(i, P.Ge.AcGePoint2d(x, y))
    pl.setClosed(True)
    tr.addEntity(pl)
result = "ok"
'''

# 3) 文字→线条（SHX 走引擎，TTF 走 GDI 追踪）
#    见 G:\\PyArx\\tools\\Text2Line\\explode_text2.py 与 ShxProbe.pyd
```

## 已实测的边界

- **热加载残留**：`hotbuild`/`pyreload` 之后 `sys.modules['PyArxCore']` 可能指向 `PyArxCoreHtN` 顶模块而只挂了 `Ge`，
  `P.Db` 直接 AttributeError（实测 `module 'PyArxCoreHt1' has no attribute 'Db'`）。真子模块仍在
  `sys.modules['PyArxCore.Db']` 等位置，`bridge.py` 的 `load_pyarx()` 已按门面对象自动回填；根治要重启 CAD 或重跑正常版 pyd。
- `P.Db.PyAcadDatabase.current()` 返回**空库**，一律用 `P.Db.curDb()`。
- 没有文档锁/事务直接 append 会失败（`btr open fail=82`）；统一用 `P.Db.PyDbTrans()`。
- 删除实体只能事务内 `tr.openObject(id, 1)` 后 `obj.erase()`；`P.Ed.handent()` 返回 `None`，`entdel` 不接受 None。
- 图纸被同会话其他标签占用时 SAVEAS 失败；先关标签。
- CAD 处于交互命令提示（"指定对角点…"）时 COM 枚举会返回空壳文档，需要先结束该命令。

## 加了新工具之后

server.py 里 TOOLS / dispatch 改动后，正在跑的 8765 进程不会自己刷新（它在启动时把 TOOLS 读进内存了）：

```powershell
Stop-Process -Id (Get-NetTCPConnection -LocalPort 8765 -State Listen).OwningProcess -Force; wscript G:\PyArx\tools\cadmcp\start_http.vbs; (Invoke-WebRequest http://127.0.0.1:8765/health -UseBasicParsing).Content
```

/health 会列出当前真实生效的工具名。客户端那侧的工具表是握手时拉的，重开一个对话（或断开重连该连接器）才会看到新工具。
