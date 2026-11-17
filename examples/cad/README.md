# PyArxCore CAD 接入 sample 与坑点清单

面向"让 AI / 新脚本一次写对"的最小用法集，全部在 AutoCAD 2024 + 本工程热更模块上实测通过。

## 环境与入口

| 事项 | 做法 |
| --- | --- |
| 本地 Python | `G:\PyArx\X64\Release\python.exe`（PATH 里没有 python） |
| 模块加载 | 工程只产出热更模块 `PyArxCoreH<时间戳>.pyd`；每个新 CAD 进程先跑 `G:\PyArx\tools\hotbuild\reload.py`，之后 `import PyArxCore as P` |
| 模块名 | `G:\PyArx\tools\hotbuild\module.txt` |
| 附着运行 | `powershell.exe -NoProfile -ExecutionPolicy Bypass -File "G:\PyArx\tests\cad\cad_attach_run.ps1" -Script "<py>" -ResultJson "<json>" -TimeoutSec 90 -ExpectPid <pid>` |
| 编译 | `G:\PyArx\tools\hotbuild\hotbuild.ps1`（增量约 15s） |

热重载注意：新老两个热模块同时存在同一进程会触发 pybind11 断言 `types->empty()` 弹框并卡死 CAD。**每次重新编译后必须重启 CAD，再只加载新模块。**

## sample

- `01_draw_basic.py` — 8 类图元入库（`db.addToCurrentspace`）
- `02_layer_block.py` — 图层、块定义、块参照插入与按名读回
- `03_read_modify_delete.py` — 按类读、改属性、删除与恢复
- `04_type_dispatch.py` — 判类型 → `cast` → 调类型特有方法（含类名映射 helper）

## 已修复的绑定缺陷（本次改动，共 4 处 + 1 处新增）

1. `PyDbDatabase::addToBlock`（`PyDbDatabase.cpp`）
   原来只做裸 `appendAcDbEntity`，缺 `addNewlyCreatedDBRObject` 与 `detachDbObject`。
   后果：`db.addToCurrentspace/addToModelspace/addToBlock` 返回的 id 当时有效，命令结束后
   `openObject(id)` 得到空包装，库里只剩打不开的僵尸条目。
   修复后与 `PyDbBlockTableRecord::appendEntity` 对齐三步，`addToCurrentspace` 留存正常。

2. `PyDbSymbolTable::add`（`PyDbSymbolTable.cpp`）
   原来 `add()` 后不关闭记录，记录保持写打开状态。
   后果：同一会话内新建的图层/块按名读不到（`getAt` 返回空包装、`keys()/records()` 静默返回空），
   还会让整个表的迭代在第一条锁住的记录处中断。
   修复后 `add` 内部关闭记录，`keys()/getAt` 正常。

3. `PyDbBlockTableRecord::appendAcDbEntity`（`PyDbBlockTableRecord.cpp`）
   原来是 `appendAcDbEntityId` 的裸别名（返回 bool、不注册、不 detach）。改为转发到正规
   `appendEntity`；同时 `appendEntity` 只在"自己开了事务"时 detach，并挡住"记录尚未入库"的情况。

4. `PyDbBlockReference::setBlockTableRecord`（`PyDbBlockReference.cpp`）
   传入 id=0（块表重名导致 `bt.add` 失败后的典型值）直接返回 `eInvalidInput`，不再把 0 带进数据库。

新增（非修复）：`db.classNameOf(objectId) -> str`，读模式打开取 `isA()->name()`，失败返回空串。
用来"直接从 ObjectId 判类型"，替代"先 cast 再试错"。

## 坑点清单（都踩过，附正确写法）

1. **图元写入三种入口的区别**
   `btr.appendAcDbEntity(ent)` 返回 bool，只是裸 `AcDbBlockTableRecord::appendAcDbEntity`，不注册新对象、不 detach，命令结束后实体被丢弃（重启后 8 个图元变 0）。
   可用入口：`db.addToCurrentspace(ent)` / `db.addToModelspace(ent)` / `db.addToBlock(btrId, ent)` / `btr.appendAcDbEntityId(ent)`，都返回新 ObjectId。

2. **入库后原包装不可再用**：这些入口内部会 `detachDbObject`（m_ptr 置空）。要再操作按 id 重新打开。

3. **不开事务、直接用 database 也能落库**：`lt = db.getLayerTable(1)` → `lt.add(rec)` → `rec.close()` → `lt.close()`，
   实测跨命令 `getAt` 非空、`names()` 含它（`probe_notx.json` / `probe_notx_verify.json`）。
   `PyDbTrans` 不是必须的；要用就用，但 `add` 之后必须自己 `close()` 记录，否则记录保持写打开。

4. **要用事务的话必须 try/finally**：事务里抛异常，traceback 会持有局部变量（表/记录包装），表保持写打开，之后所有符号表操作连锁报
   `getSymbolTable(kForWrite) 失败: eWasOpenForWrite`。

5. **`isNull()` 必须先判**：`getAt` 找不到、实体已被 erase、按名解析失败时都返回空包装，对它调任何方法都是
   `RuntimeError: Caught an unknown exception!`。

6. **`PyDbXxx.cast()` 自带类型检查，失败给空包装**：`dbents.h:835` 的 `ACDBCORE2D_DECLARE_MEMBERS` →
   `dbboiler.h:42-50` → `ACRX_DECLARE_MEMBERS_EXPIMP`，声明的是 `static AcDbXxx* cast(const AcRxObject*)`，
   内部 `isKindOf` 不符返回空指针，Python 侧拿到 `isNull()==True` 的包装。
   所以 cast 不需要改写；坑在"没判 `isNull()` 就调方法"（实测 `probe_type.json`：Line cast 成 Circle → `isNull=True`，不崩溃）。
   判断类型更省事的路子：`db.classNameOf(id)`；Python 侧也可 `isinstance`——`PyDbBinder.cpp:235-237` 明确写了
   `isKindOf/isDerivedFrom` 故意不暴露，类型判断交给 Python。

6b. **块定义内加子实体：先入库，再 `addToBlock`**
   在事务里对块记录 `appendAcDbEntity` 必崩，先后撞两个 ARX 断言：
   `!dbspace.cpp@4033: eWrongDatabase`（块记录未入库/库不匹配）和 `!dbobj.cpp@8668: eNotOpenForWrite`
   （子实体在调用方事务里已被 detach，commit 再关一次）。
   可行顺序：`bt.add(btr)` → `btr.close()` → `db.addToBlock(blkId, ent)`（`probe_blockdef2.json`：ents=2）。

6c. **块表中不能有同名**：`bt.add` 遇到同名返回错误，`btr.objectId()` 为 0，再拿 0 去 `setBlockTableRecord`/insert 就是
   `eWrongDatabase`。建块前先 `bt.has(name)`，已有就复用其 id（`01_layer_block.py` 的 `add_block()` 就是这个写法）。

6d. **包装类名映射：`PyDb` + 类名去掉 `AcDb` 前缀**
   `AcDbCircle → P.Db.PyDbCircle`（不是 `PyAcDbCircle`）。所以"判类型 → 拿特有方法"的完整链路是：
   `cls = db.classNameOf(oid)` → `pycls = getattr(P.Db, "PyDb" + cls[4:])` → `obj = db.openObject(oid, 0)`
   → `typed = pycls.cast(obj)` → 判 `typed.isNull()` → 调特有方法 → `typed.close()`。
   照抄见 `04_type_dispatch.py`：8 类图元实测全部拿到值（Circle `radius()`、Text `textString()`、Polyline `numVerts()`、
   BlockReference `position()` 等，`sample04_steps.json`）。

7. **`erase` 之后不能重新打开**：`getEntityById(oid, 1)` 在已删实体上返回空包装。恢复删除要在**同一个包装**上 `erase(False)`。

8. **读模式显式传 0**：`db.getLayerTable(0)` / `getBlockTable(0)` / `getEntityById(id, 0)` / `openObject(id, 0)`；
   `PyDbLayerTable.getAt(name)` 只有 1 个参数（内部固定 kForRead，改属性需 `upgradeOpen()`），
   `PyDbBlockTable.getAt(name, mode)` 带 mode。

9. **不要在自己写打开的空间/表上统计**：`db.objectIds()` 内部按读模式打开模型空间，失败时**静默返回空列表**（不是报错）。
   拿着写打开的 BTR 时统计会得到 0，容易误判"数据丢了"。

10. **PYRUNFILE 后脚本模块常驻**：脚本全局作用域留下的包装不会及时析构，会把数据库锁住，后续脚本全部失败（`objectIds` 返回 0、`addToCurrentspace` 返回 0）。
    规范：包装在函数内创建、函数内 close，不要挂在模块全局。

11. **遍历用 `iterEntities(mode, skipDeleted)`**：`for ent in btr` 恒空转；`btr[id]` / `keys()` / `len()` 计数互相矛盾，别混用。

12. **`Ed.handent(handle)` 对有效句柄返回 None**；`db.openObject` 才是按 id 打开的正路（`getEntityById` 只用于块内实体）。

13. **构造签名差异**：`PyDb3dSolid.createBox(x,y,z)` 无中心点，定位靠 `transformBy`；`PyDbRay/PyDbXline` 只能无参构造再 `setBasePoint/setUnitDir`。

14. **PyWarnIfError 看不到原因**：它是非抛路径，只 `acutPrintf` 到 CAD 命令行；`PyEngineCompat.h:15` 把 `PyEngine_DbgLog` 编译成空桩，
    所以 `G:\PyArx\log\pyengine.log` 从来不会生成。脚本侧能看到的只有 bool/0 返回值和 pybind 的 unknown exception。
    要真实错误码只能读 CAD 命令行。

15. **保存**：`PyDbDatabase::saveAs` 对文档库恒 `eFileAccessErr`，`save()` 内部就是 `saveAs(getFilename())`；
    正确路径是 `PyApDocument::save()`（同步落盘、不弹对话框，`bBakAndRename` 用默认 true）。
    未命名图形（`Drawing1.dwg`，`getFilename()` 给模板 `acadiso.dwt` 路径）无磁盘文件名，不要直接存。

## 结果证据

- `G:\PyArx\log\probe_type.json` — `classNameOf` 判出 `AcDbCircle`/`AcDbLine`；Line cast 成 Circle → `isNull=True`；正路 `radius=7.0`
- `G:\PyArx\log\run_samples.json` — 模块 `PyArxCoreH20261115_173207` 新会话 `Drawing1.dwg` 上三支 sample 连跑全 ok
- `G:\PyArx\log\sample02_steps.json` — 块流程 13 步逐步结果（图层 created、块 created、ents 0→2、块参照 id 有效）
- `G:\PyArx\log\probe_blockdef2.json` — 块定义入库后 `addToBlock` 追加子实体，ents=2
- `G:\PyArx\log\probe_notx.json` / `probe_notx_verify.json` — 不开事务建图层并跨命令核对
- `G:\PyArx\log\pyarx_test_batch14.json` / `pyarx_verify.json` — 9 项功能逐项全绿 + 另起命令核对留存
- `G:\PyArx\log\pyarx_learn.log` — 过程日志（START/OK/FAIL/坑点）
