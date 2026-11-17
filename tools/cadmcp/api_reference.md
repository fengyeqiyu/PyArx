# PyArxCore API 参考（源码提取 + 实跑验证）

生成方式：解析 `G:\PyArx\PyArxCore\*.cpp` 的 pybind11 绑定表，
参数名取自 `py::arg`、参数类型取自 C++ 成员函数定义或 `py::overload_cast` 模板参数，
再用 headless 实跑验证。**签名以本文为准，不要靠猜。**

规模：299 类 / 12799 个绑定方法。


## 一、调用约定（先看这一节）

```python
import PyArxCore as P
from PyArxCore import Db, Ge   # 推荐：与工具层一致
```

- 实体构造后**必须** `tr.addEntity(ent)` 入库，句柄才有效；返回 `bool`，
  **失败不抛异常**（句柄停在 "0"、实体数不涨），必须检查返回值。
- `addEntity` 拒绝的常见原因：xref 派生块 `$0$` 不能引用、对象未初始化、事务未打开。
- 图层**不能**在事务内建：事务持写锁时 `getSymbolTable(kForWrite)` 报 `eWasOpenForWrite`。
  正确路径（事务外）：`lt = DB.getLayerTable()` → `PyDbLayerTableRecord.newRecord()` → 
  `setName/setColor` → `lt.add(rec)` → 最后 `lt.close()`。
- 符号表一个进程只能打开一次，用完必须 `close()`。


## 二、能力边界（实测，务必遵守）

| 结论 | 证据 |
|---|---|
| `newRecord()` **只有** `PyDbLayerTableRecord` 有 | `Grep .def_static("newRecord"` 全库仅 1 处 |
| 块定义 / 标注样式记录 **无法从 Python 新建** | `PyDbBlockTableRecord` / `PyDbDimStyleTableRecord` 只有 `(p:AcDbXxx*)` 构造，且无 `newRecord` |
| `PyDbBlockReference.hasAttributes` 是**恒 false 的占位** | 源码写死 `[]() { return false; }`，真实用途用 `attributeCount()` |
| `setAttachmentMovingLocation(v)` 参数是 **int（附着点编号）**，不是点 | `py::arg("v")` + C++ `int`；传 Point3d 报 `incompatible function arguments` |
| 需要数据库上下文的几何方法要先入库 | 入库前 `getBoundingPoints` 抛 `Caught an unknown exception!`，入库后正常返回 4 个角点 |
| 块参照 transform 方法要求块定义真实存在 | 空图里只有 `*Model_Space` 等系统块，取不到用户块；真实图 `_ArchTick` 上 4 个方法全部返回正常 |
| `PyDbMline.setStyle(id)` 对无样式 mline 返回 `False`（不抛异常） | 实跑 inserted=False + setStyle→False |


## 三、基类通用方法（继承链上生效，子类直接可用）


### PyDbObject（父类 PyRxObject）

共 59 个：

- `attribData()` -> py::list  — 实例属性列表（仅块参照有值）：{tag,txt,x,y,ax,ay,h,rot,ha,va,inv,ly,ci}
- `objectId()` -> uint64_t  — Persistent objectId as uint64 (0 when null)
- `handle()` -> uint64_t  — Object handle as 64-bit uint64 (0 when null)
- `id()` -> uint64_t  — ObjectId as uint64 (alias of objectId)
- `ownerId()` -> uint64_t  — Owner (container) objectId as uint64
- `extensionDictionary()` -> uint64_t  — Extension dictionary objectId as uint64 (0 when absent)
- `database(void: )` -> PyDbDatabase  — Database that owns this object
- `clone(void: )` -> PyRxObject  — Deep copy of the object
- `copyFrom(other: const PyDbObject&)`  — ),
			
- `isNull()`  — Check whether the underlying pointer is null
- `intPtr()`  — Integer representation of the underlying pointer
- `close()`  — Close the object; returns False on failure; error printed to the CAD command line
- `cancel()`  — Cancel the open request; returns False on failure; error printed to the CAD command line
- `erase(erase_it: bool)`  — Erase/un-erase the object; returns False on failure; error printed to the CAD command line
- `upgradeOpen()`  — Upgrade open mode to kForWrite; returns False on failure; error printed to the CAD command line
- `downgradeOpen()`  — Downgrade open mode from kForWrite; returns False on failure; error printed to the CAD command line
- `createExtensionDictionary()`  — Create the extension dictionary; returns False on failure; error printed to the CAD command line
- `releaseExtensionDictionary()`  — Release the extension dictionary; returns False on failure; error printed to the CAD command line
- `assertReadEnabled(void: )` -> void  — Assert the object is open for read
- `assertWriteEnabled(auto_undo: bool, record_modified: bool)` -> void  — Assert the object is open for write
- `assertNotifyEnabled(void: )` -> void  — Assert notification is enabled
- `disableUndoRecording(disable: bool)` -> bool  — Disable/enable undo recording
- `isErased(void: )` -> bool  — Object is erased
- `isModified(void: )` -> bool  — Object has been modified since the last save
- `isNewObject(void: )` -> bool  — Object was newly created this editing session
- `isReadEnabled(void: )` -> bool  — Object is open for read
- `isWriteEnabled(void: )` -> bool  — Object is open for write
- `isNotifyEnabled(void: )` -> bool  — Notification is enabled
- `isNotifying(void: )` -> bool  — Object is currently notifying
- `isPersistent(void: )` -> bool  — Object has a database-resident id
- `isAProxy(void: )` -> bool  — Object is a proxy
- `isCancelling(void: )` -> bool  — Object is being cancelled
- `isUndoing(void: )` -> bool  — Undo operation is in progress
- `isUndoRecordingDisabled(void: )` -> bool  — Undo recording is disabled
- `isEraseStatusToggled(void: )` -> bool  — Erase status was toggled this session
- `isModifiedGraphics(void: )` -> bool  — Graphics were modified
- `isModifiedXData(void: )` -> bool  — XData was modified
- `isReallyClosing(void: )` -> bool  — Object is really closing
- `isTransactionResident(void: )` -> bool  — Object is transaction resident
- `hasFields(void: )` -> bool  — Object has fields
- `xData(regapp_name: const AcString&)` -> PyResbuf  — 读取指定 regapp 的 XData 链（Resbuf，空则 isNull()=True）
- `setXData(xdata: const PyResbuf&)`  — 写入 XData 链（用 TypedValue/TvList 构造的 Resbuf）
- `getFieldDictionary()` -> uint64_t  — 字段字典 objectId（0 时无字段字典）
- `hasSaveVersionOverride(void: )` -> bool  — 是否有保存版本覆盖
- `setHasSaveVersionOverride(set: bool)` -> void  — 设置保存版本覆盖标志
- `setAcDbObjectIdsInFlux(void: )` -> void  — 标记对象 ID 正在流动（克隆操作期间调用）
- `hasPersistentReactor(obj_id: AcDbObjectId)` -> bool  — 是否有指定对象的持久反应器
- `closeAndPage(only_when_clean: bool)`  — 关闭对象并分页
- `swapIdWith(other_id: AcDbObjectId, swap_xdata: bool, swap_reactors: bool)`  — 交换两个对象的 ID
- `upgradeFromNotify()` -> bool  — 从 notify 模式升级；返回之前是否已可写
- `handOverTo(new_object: PyDbObject&, keep_obj: bool, erase_it: bool)`  — 将对象内容移交到另一个对象（newObj 接收内容，原对象可选保留/删除）
- `setBinaryData(key: const std::wstring&, data: const std::string&)`  — 按 key 写入二进制数据
- `getBinaryData(key: const std::wstring&)` -> py::tuple  — 按 key 读取二进制数据；返回 (success, bytes)
- `getFieldAsId(prop_name: const std::wstring&)` -> uint64_t  — 获取字段的 objectId（0 时无该字段）
- `removeFieldByName(prop_name: const std::wstring&)`  — 按名称删除字段
- `removeFieldById(field_id: AcDbObjectId)`  — 按 ID 删除字段
- `addReactor(reactor: PyDbObjectReactor*)`  — 注册对象反应器
- `removeReactor(reactor: PyDbObjectReactor*)`  — 移除对象反应器
- `hasReactor(reactor: PyDbObjectReactor*)` -> bool  — 检查是否已注册指定反应器


### PyDbEntity（父类 PyDbObject）

共 61 个：

- `colorIndex()` -> unsigned short
- `setColorIndex(colorIndex: unsigned short, doSubents: bool)` -> bool  — doSubents
- `layer()` -> AcString
- `layerId()` -> uint64_t
- `setLayer(name: const AcString&, doSubents: bool)` -> bool  — ), py::arg(
- `setLayerId(layerId: uint64_t, doSubents: bool)` -> bool  — 按图层 ObjectId（oldId uint64）设置图层，绕开图层名编码问题
- `linetype()` -> AcString
- `linetypeId()` -> uint64_t
- `setLinetype(name: const AcString&, doSubents: bool)` -> bool  — ), py::arg(
- `lineWeight()` -> int
- `setLineWeight(newVal: int, doSubents: bool)` -> bool  — doSubents
- `visibility()` -> int
- `setVisibility(newVal: int, doSubents: bool)` -> bool  — doSubents
- `linetypeScale()` -> double
- `setLinetypeScale(newval: double, doSubents: bool)` -> bool  — doSubents
- `getGeomExtents()` -> py::object  — 返回 (minPoint, maxPoint) 元组；获取失败返回 None
- `transformBy(xform: const AcGeMatrix3d&)` -> bool
- `intersectWith(other: const PyDbEntity&, intType: int)` -> py::list  — ), py::arg(
- `highlight(do_it: bool)`  — ) = true, 
- `unhighlight()`  — unhighlight
- `setRGB(r: int, g: int, b: int)`  — 设置真彩色 0-255
- `getTransformedCopy(xform: const AcGeMatrix3d&)` -> PyDbEntity  — 返回变换后的实体副本（未入库）
- `color()` -> PyColor  — 返回真彩色对象 PyColor
- `setColor(color: const PyColor&, doSubents: bool)` -> bool  — ) = true, 
- `entityColor()` -> std::tuple<int,int,int>  — 返回有效实体颜色 RGB (r,g,b)
- `colorRGB()` -> std::tuple<int,int,int>  — 返回存储的真彩色 RGB (r,g,b)
- `transparency()` -> int  — 返回透明度 alpha 0=全透明 255=不透明
- `setTransparency(alpha: int, doSubents: bool)` -> bool  — ) = true, 
- `bounds()` -> std::tuple<AcGePoint3d,AcGePoint3d>  — 返回包围盒 (minPoint, maxPoint)
- `explode()` -> std::vector<PyDbEntity>  — 炸开实体，返回新实体列表（未入库，需 appendEntity）
- `getPlane()` -> std::tuple<AcGePlane,int>  — 返回 (plane, planarity枚举)
- `isPlanar()` -> bool
- `isContentSnappable()` -> bool
- `material()` -> AcString
- `materialId()` -> uint64_t
- `setMaterial(name: const AcString&, doSubents: bool)` -> bool  — ), py::arg(
- `setDatabaseDefaults()` -> void
- `setPropertiesFrom(src: const PyDbEntity&, doSubents: bool)` -> bool  — ), py::arg(
- `castShadows()` -> bool
- `setCastShadows(newVal: bool)` -> bool  — newVal
- `receiveShadows()` -> bool
- `setReceiveShadows(newVal: bool)` -> bool  — newVal
- `collisionType()` -> int
- `recordGraphicsModified(setModified: bool)` -> void  — setModified
- `visualStyleId()` -> uint64_t
- `setVisualStyle(visualStyleId: uint64_t, doSubents: bool)` -> bool  — doSubents
- `blockId()` -> uint64_t
- `getEcs()` -> AcGeMatrix3d  — 返回实体坐标系(ECS)到 WCS 的变换矩阵
- `plotStyleName()` -> AcString
- `setPlotStyleName(name: const AcString&)` -> bool
- `list()` -> void  — 等价 CAD LIST 命令，打印实体信息
- `draw()` -> void  — 绘制到当前视图
- `getGripPoints()` -> py::tuple  — 返回 (gripPoints, osnapModes, geomIds)
- `moveGripPointsAt(indices: const std::vector<int>&, offset: const AcGeVector3d&)` -> void  — offset
- `pushHighlight(style: int)` -> bool  — ) = 0, 
- `popHighlight()` -> bool
- `getStretchPoints()` -> py::tuple
- `moveStretchPointsAt(indices: const std::vector<int>&, offset: const AcGeVector3d&)` -> void  — offset
- `intersectWith(other: const PyDbEntity&, intType: int, proj_plane: const AcGePlane&, thisGsMarker: int, otherGsMarker: int)` -> std::vector<AcGePoint3d>  — ) = 0, py::arg(
- `boundingBoxIntersectWith(other: const PyDbEntity&, intType: int, proj_plane: const AcGePlane&, thisGsMarker: int, otherGsMarker: int)` -> std::vector<AcGePoint3d>  — ) = 0, py::arg(
- `boundingBoxIntersectWith(other: const PyDbEntity&, intType: int, thisGsMarker: int, otherGsMarker: int)` -> std::vector<AcGePoint3d>  — ) = 0, py::arg(


### PyDbCurve（父类 PyDbEntity）

共 31 个：

- `isClosed()` -> bool
- `isPeriodic()` -> bool
- `getStartParam()` -> double
- `getEndParam()` -> double
- `getStartPoint()` -> AcGePoint3d
- `getEndPoint()` -> AcGePoint3d
- `getPointAtParam(param: double)` -> AcGePoint3d
- `getParamAtPoint(pt: const AcGePoint3d&)` -> double
- `getDistAtParam(param: double)` -> double
- `getParamAtDist(dist: double)` -> double
- `getDistAtPoint(pt: const AcGePoint3d&)` -> double
- `getPointAtDist(dist: double)` -> AcGePoint3d
- `getFirstDeriv(arg0: double)`
- `getFirstDeriv(pt: const AcGePoint3d&)`
- `getSecondDeriv(arg0: double)`
- `getSecondDeriv(pt: const AcGePoint3d&)`
- `getClosestPointTo(pt: const AcGePoint3d&, extend: bool)`  — ), py::arg(
- `getClosestPointTo(givenPnt: const AcGePoint3d&, direction: const AcGeVector3d&, extend: bool)`  — 沿 direction 方向投影求最近点，返回曲线上点
- `getArea()` -> double
- `reverseCurve()` -> bool
- `extend(newParam: double)` -> bool  — newParam
- `extendToPoint(extendStart: bool, toPoint: const AcGePoint3d&)` -> bool  — 延伸曲线到指定点
- `getOffsetCurves(offset_distance: double)` -> py::list  — 偏移曲线，返回新实体列表（未入库，需 appendEntity）
- `getSplitCurves(params: const std::vector<double>&)` -> py::list  — 按参数点分割曲线，返回新曲线列表（未入库）
- `getSpline()` -> PyDbSpline  — 拟合为样条曲线（未入库）
- `getSplitCurves(points: const std::vector<AcGePoint3d>&)`  — 按点列分割曲线，返回新曲线列表（未入库，需 appendEntity）
- `getOffsetCurvesGivenPlaneNormal(normal: const AcGeVector3d&, offset_dist: double)` -> std::vector<PyDbEntity>  — 按平面法向偏移，返回新曲线列表（未入库，需 appendEntity）
- `getProjectedCurve(plane: const AcGePlane&, proj_dir: const AcGeVector3d&)` -> PyDbCurve  — 沿方向投影到平面，返回新曲线（未入库）
- `getOrthoProjectedCurve(plane: const AcGePlane&)` -> PyDbCurve  — 正交投影到平面，返回新曲线（未入库）
- `getAcGeCurve()` -> AcGeCurve3d*  — 由 AcGeCurve3d 创建 AcDbCurve（未入库，需 appendEntity）
- `setFromAcGeCurve(ge_curve: const AcGeCurve3d&)` -> bool  — ge_curve


### PyDbSymbolTable（父类 PyDbObject）

共 12 个：

- `has(name: const AcString&)` -> bool
- `getAt(name: const AcString&, mode: int)` -> PyDbSymbolTableRecord  — ) = 1, 
- `getIdAt(name: const AcString&)` -> uint64_t  — 按名称取记录 ObjectId，失败返回 0
- `add(rec: PyDbSymbolTableRecord&)` -> bool  — 向符号表追加一条记录
- `toDict()` -> py::dict
- `names()` -> py::list
- `records(mode: int)` -> py::list  — ) = 1, 
- `keys()` -> py::list
- `values(mode: int)` -> py::list  — ) = 1, 
- `items(mode: int)` -> py::list  — ) = 1, 
- `get(name: const AcString&, default: py::object)` -> py::object  — ), py::arg(
- `numRecords()` -> int


### PyDbSymbolTableRecord（父类 PyDbObject）

共 6 个：

- `setName(name: const AcString&)` -> bool
- `getName()` -> AcString
- `isDependent()` -> bool
- `isResolved()` -> bool
- `isRenamable()` -> bool
- `upgradeOpen()` -> bool  — 将打开模式从读升级为写


### PyDbBlockTableRecord（父类 PyDbSymbolTableRecord）

共 43 个：

- `comments()` -> AcString
- `setComments(comments: const AcString&)` -> bool  — comments
- `pathName()` -> AcString
- `setPathName(path: const AcString&)` -> bool
- `origin()` -> AcGePoint3d
- `setOrigin(pt: const AcGePoint3d&)` -> bool
- `hasAttributeDefinitions()` -> bool
- `hasPreviewIcon()` -> bool
- `isAnonymous()` -> bool
- `isFromExternalReference()` -> bool
- `isFromOverlayReference()` -> bool
- `setIsFromOverlayReference(bIsOverlay: bool)` -> bool  — bIsOverlay
- `isLayout()` -> bool
- `getLayoutId()` -> uint64_t
- `setLayoutId(layoutId: uint64_t)` -> bool  — layoutId
- `isUnloaded()` -> bool
- `setIsUnloaded(isUnloaded: bool)` -> bool  — isUnloaded
- `xrefStatus()` -> int
- `getBlockReferenceIds(directOnly: bool, forceValidity: bool)` -> py::list  — forceValidity
- `getErasedBlockReferenceIds()` -> py::list
- `assumeOwnershipOf(entitiesToMove: const AcDbObjectIdArray&)` -> bool  — entitiesToMove
- `blockScaling()` -> int
- `setBlockScaling(blockScaling: int)` -> bool  — blockScaling
- `explodable()` -> bool
- `setExplodable(bExplodable: bool)` -> bool  — bExplodable
- `blockInsertUnits()` -> int
- `setBlockInsertUnits(insunits: int)` -> bool  — insunits
- `postProcessAnnotativeBTR(bqueryOnly: bool, bScale: bool)` -> int  — bScale
- `addAnnoScalestoBlkRefs(bScale: bool)` -> bool  — bScale
- `appendEntity(entity: PyDbEntity&)` -> bool  — 向该块记录（模型/图纸空间）追加一个实体
- `appendAcDbEntity(entity: PyDbEntity&)`  — 向该块记录追加一个实体（内部完成 append+注册+关闭，返回 bool）
- `appendAcDbEntityId(entity: PyDbEntity&)` -> uint64_t  — 追加实体并返回新实体 ObjectId（oldId），失败返回 0
- `getPreviewIcon()` -> std::vector<uint8_t>  — 返回预览图标原始字节（BMP），失败返回空 list
- `setPreviewIcon(bytes: const std::vector<uint8_t>&)` -> bool  — 设置预览图标原始字节（BMP）
- `entityIds()` -> py::list  — 遍历块记录内所有实体的 ObjectId 列表
- `numEntities()` -> int  — 块内实体数量（不含已删除）
- `attdefData()` -> py::list  — 块内 ATTDEF 列表：{tag,txt,x,y,h,rot,ha,va,inv,ly,ci}
- `iterEntities(mode: int, skipDeleted: bool)`  — ) = true,
			
- `getEntityById(id: uint64_t, mode: int)` -> PyDbEntity  — ) = 1, 
- `get(key: py::object, default: py::object)` -> py::object  — ) = py::none(), 
- `keys()` -> py::list  — 块内实体 ObjectId 列表（等价 entityIds()）
- `values(mode: int)` -> py::list  — ) = 1, 
- `items(mode: int)` -> py::list  — ) = 1, 


### PyDbLayerTableRecord（父类 PyDbSymbolTableRecord）

共 41 个：

- `isFrozen()` -> bool
- `setIsFrozen(frozen: bool)` -> bool  — frozen
- `isOff()` -> bool
- `setIsOff(off: bool)` -> void
- `vPDFLT()` -> bool
- `setVPDFLT(frozen: bool)` -> void  — frozen
- `isLocked()` -> bool
- `setIsLocked(locked: bool)` -> void  — locked
- `linetypeObjectId()` -> uint64_t
- `setLinetypeObjectId(id: uint64_t)` -> bool
- `materialId()` -> uint64_t
- `setMaterialId(id: uint64_t)` -> bool
- `lineWeight()` -> int
- `setLineWeight(lw: int)` -> bool
- `isPlottable()` -> bool
- `setIsPlottable(plot: bool)` -> bool
- `plotStyleName()` -> AcString
- `plotStyleNameId()` -> uint64_t
- `setPlotStyleName(newName: const AcString&)` -> bool  — newName
- `setPlotStyleNameById(newId: uint64_t)` -> bool
- `isInUse()` -> bool
- `description()` -> AcString
- `setDescription(description: const AcString&)` -> bool  — description
- `isHidden()`
- `setIsHidden(on: bool)` -> bool  — , static_cast<bool(*)(uint64_t)>(&PyDbLayerTableRecord::isHidden), py::arg(
- `removeColorOverride(viewportId: uint64_t)` -> bool  — viewportId
- `removeLinetypeOverride(viewportId: uint64_t)` -> bool  — viewportId
- `removeLineWeightOverride(viewportId: uint64_t)` -> bool  — viewportId
- `removePlotStyleOverride(viewportId: uint64_t)` -> bool  — viewportId
- `removeTransparencyOverride(viewportId: uint64_t)` -> bool  — viewportId
- `removeViewportOverrides(viewportId: uint64_t)` -> bool  — viewportId
- `removeAllOverrides()` -> bool
- `hasOverrides(viewportId: uint64_t)` -> bool  — viewportId
- `hasAnyOverrides()` -> bool
- `isReconciled()`
- `setIsReconciled(bReconcile: bool)` -> bool  — objectId
- `color()` -> std::tuple<int, int, int, int>  — 返回 (colorIndex, r, g, b)；256=ByLayer,0=ByBlock
- `setColor(r: int, g: int, b: int)` -> void  — 以真彩色 RGB 设置层颜色
- `entityColor()` -> std::tuple<int, int, int, int>  — 返回实体颜色 (colorIndex, r, g, b)
- `transparency()` -> int  — 返回 0=ByLayer,1=ByBlock,否则 alpha 0..255
- `setTransparency(trans: int)` -> bool  — 0=ByLayer,1=ByBlock,否则 alpha 0..255


### PyDbDimStyleTableRecord（父类 PyDbSymbolTableRecord）

共 73 个：

- `dimasz()`
- `setDimasz(v: double)`  — setDimasz
- `dimtxt()`
- `setDimtxt(v: double)`  — setDimtxt
- `dimscale()`
- `setDimscale(v: double)`  — setDimscale
- `dimexe()`
- `setDimexe(v: double)`  — setDimexe
- `dimexo()`
- `setDimexo(v: double)`  — setDimexo
- `dimgap()`
- `setDimgap(v: double)`  — setDimgap
- `dimdli()`
- `setDimdli(v: double)`  — setDimdli
- `dimdle()`
- `setDimdle(v: double)`  — setDimdle
- `dimrnd()`
- `setDimrnd(v: double)`  — setDimrnd
- `dimlfac()`
- `setDimlfac(v: double)`  — setDimlfac
- `dimtm()`
- `setDimtm(v: double)`  — setDimtm
- `dimtp()`
- `setDimtp(v: double)`  — setDimtp
- `dimtsz()`
- `setDimtsz(v: double)`  — setDimtsz
- `dimdec()`
- `setDimdec(v: int)`  — setDimdec
- `dimtdec()`
- `setDimtdec(v: int)`  — setDimtdec
- `dimtad()`
- `setDimtad(v: int)`  — setDimtad
- `dimjust()`
- `setDimjust(v: int)`  — setDimjust
- `dimzin()`
- `setDimzin(v: int)`  — setDimzin
- `dimlunit()`
- `setDimlunit(v: int)`  — setDimlunit
- `dimtol()`
- `setDimtol(v: bool)`  — setDimtol
- `dimtih()`
- `setDimtih(v: bool)`  — setDimtih
- `dimtix()`
- `setDimtix(v: bool)`  — setDimtix
- `dimtxstyId()` -> uint64_t  — 关联文字样式 ObjectId
- `setDimtxsty(id: uint64_t)`  — 设置关联文字样式 ObjectId
- `dimalt()`
- `setDimalt(v: bool)`  — setDimalt
- `dimaltd()`
- `setDimaltd(v: int)`  — setDimaltd
- `dimaltf()`
- `setDimaltf(v: double)`  — setDimaltf
- `dimaltu()`
- `setDimaltu(v: int)`  — setDimaltu
- `dimalttd()`
- `setDimalttd(v: int)`  — setDimalttd
- `dimaltz()`
- `setDimaltz(v: int)`  — setDimaltz
- `dimaltrnd()`
- `setDimaltrnd(v: double)`  — setDimaltrnd
- `dimblkId()` -> uint64_t  — 箭头块 ObjectId
- `setDimblk(id: uint64_t)`  — setDimblk
- `dimblk1Id()` -> uint64_t
- `dimblk2Id()` -> uint64_t
- `arrowId(whichArrow: int)` -> uint64_t  — 按箭头标识取箭头块 ObjectId（AcDb::DimArrowFlags）
- `getDimapost()` -> AcString  — getDimapost
- `getDimpost()` -> AcString  — getDimpost
- `getDimblk()` -> AcString  — getDimblk
- `getDimblk1()` -> AcString  — getDimblk1
- `getDimblk2()` -> AcString  — getDimblk2
- `isModifiedForRecompute()`
- `setDimfit(fit: int)`  — setDimfit
- `setDimunit(unit: int)`  — setDimunit


### PyDbDatabase（父类 PyRxObject）

共 176 个：

- `blockTableId()` -> uint64_t
- `layerTableId()` -> uint64_t
- `textStyleTableId()` -> uint64_t
- `linetypeTableId()` -> uint64_t
- `viewTableId()` -> uint64_t
- `ucsTableId()` -> uint64_t
- `viewportTableId()` -> uint64_t
- `regAppTableId()` -> uint64_t
- `dimStyleTableId()` -> uint64_t
- `namedObjectsDictionaryId()` -> uint64_t
- `currentSpaceId()` -> uint64_t
- `paperSpaceVportId()` -> uint64_t
- `layerZero()` -> uint64_t
- `continuousLinetype()` -> uint64_t
- `byLayerLinetype()` -> uint64_t
- `byBlockLinetype()` -> uint64_t
- `globalMaterial()` -> uint64_t
- `byLayerMaterial()` -> uint64_t
- `byBlockMaterial()` -> uint64_t
- `clayer()` -> uint64_t
- `textstyle()` -> uint64_t
- `celtype()` -> uint64_t
- `cmaterial()` -> uint64_t
- `dimstyle()` -> uint64_t
- `setClayer(layerId: uint64_t)` -> bool  — 设置当前图层；成功返回 true
- `setTextstyle(textStyleId: uint64_t)` -> bool  — 设置当前文字样式；成功返回 true
- `setCeltype(linetypeId: uint64_t)` -> bool  — 设置当前线型；成功返回 true
- `setCmaterial(materialId: uint64_t)` -> bool  — 设置当前材质；成功返回 true
- `setDimstyle(dimStyleId: uint64_t)` -> bool  — 设置当前标注样式；成功返回 true
- `ltscale()` -> double
- `setLtscale(scale: double)` -> bool
- `textsize()` -> double
- `setTextsize(size: double)` -> bool
- `thickness()` -> double
- `setThickness(thickness: double)` -> bool  — thickness
- `elevation()` -> double
- `setElevation(elev: double)` -> bool
- `lunits()` -> Adesk::Int16
- `setLunits(val: Adesk::Int16)` -> bool
- `luprec()` -> Adesk::Int16
- `setLuprec(val: Adesk::Int16)` -> bool
- `aunits()`
- `setAunits(val: Adesk::Int16)` -> bool
- `auprec()` -> Adesk::Int16
- `setAuprec(val: Adesk::Int16)` -> bool
- `dimaso()` -> bool
- `setDimaso(val: bool)` -> bool
- `dimsho()` -> bool
- `setDimsho(val: bool)` -> bool
- `fillmode()` -> bool
- `setFillmode(val: bool)` -> bool
- `limcheck()` -> bool
- `setLimcheck(val: bool)` -> bool
- `psltscale()` -> bool
- `setPsltscale(val: bool)` -> bool
- `tilemode()` -> bool
- `setTilemode(val: bool)` -> bool
- `orthomode()` -> bool
- `setOrthomode(val: bool)` -> bool
- `insunits()` -> int
- `setInsunits(units: int)` -> bool  — 设置插入单位；成功返回 true
- `approxNumObjects()` -> int
- `numberOfSaves()` -> Adesk::Int32
- `originalFileName()` -> AcString
- `isPartiallyOpened()` -> bool
- `plotStyleMode()` -> bool
- `viewportScaleDefault()` -> double
- `plinegen()` -> bool
- `setPlinegen(val: bool)` -> bool
- `regenmode()` -> bool
- `setRegenmode(val: bool)` -> bool
- `qtextmode()` -> bool
- `setQtextmode(val: bool)` -> bool
- `blipmode()` -> bool
- `setBlipmode(val: bool)` -> bool
- `mirrtext()` -> bool
- `setMirrtext(val: bool)` -> bool
- `attmode()` -> int
- `setAttmode(val: int)` -> bool
- `filletrad()` -> double
- `setFilletrad(val: double)` -> bool
- `chamfera()` -> double
- `setChamfera(val: double)` -> bool
- `chamferb()` -> double
- `setChamferb(val: double)` -> bool
- `celtscale()` -> double
- `setCeltscale(val: double)` -> bool
- `angbase()` -> double
- `setAngbase(angle: double)` -> bool
- `pdsize()` -> double
- `setPdsize(size: double)` -> bool
- `plinewid()` -> double
- `setPlinewid(width: double)` -> bool
- `userr1()` -> double
- `setUserr1(val: double)` -> bool
- `userr2()` -> double
- `setUserr2(val: double)` -> bool
- `undoRecording()` -> bool
- `disableUndoRecording(disable: bool)` -> bool  — disable
- `isEMR()` -> bool
- `getMenu()` -> AcString
- `getProjectName()` -> AcString
- `setProjectName(val: AcString)` -> bool
- `getHyperlinkBase()` -> AcString
- `setHyperlinkBase(val: AcString)` -> bool
- `getDimpost()` -> AcString
- `getDimapost()` -> AcString
- `getDimblk()` -> AcString
- `getDimblk1()` -> AcString
- `getDimblk2()` -> AcString
- `dwgFileWasSavedByAutodeskSoftware()` -> bool
- `getVisualStyleList()`
- `isValidLineWeight(weight: int)` -> bool  — weight
- `getNearestLineWeight(weight: int)` -> int  — weight
- `tdcreate()`
- `tdupdate()`
- `tducreate()`
- `tduupdate()`
- `tdindwg()`
- `tdusrtimer()`
- `resetTimes()` -> void
- `updateExt(doBestFit: bool)` -> bool  — 重算图形范围；成功返回 true
- `purge(ids: const PyDbObjectIdArray&)` -> PyDbObjectIdArray
- `setFullSaveRequired()` -> void
- `saveAs(fileName: const AcString&)` -> Acad::ErrorStatus  — 保存为 DWG 文件（返回 ErrorStatus，eOk=0）
- `saveAs(fileName: const AcString&, bBakAndRename: bool, dwgVer: int)` -> Acad::ErrorStatus  — 保存并指定版本（dwgVer 用整数，对应 AcDb::AcDbDwgVersion 枚举值）
- `readDwgFile(fileName: const AcString&, shmode: int, bAllowCPConversion: bool, wszPassword: const AcString&)` -> Acad::ErrorStatus  — 打开 DWG 文件；shmode 默认 1=AcDbDatabase::kForReadAndReadShare 共享读
- `needsRecovery()` -> bool
- `isBeingDestroyed()` -> bool
- `addAcDbObject(obj: PyDbObject&)` -> uint64_t  — 向数据库追加一个已创建的对象，返回其 ObjectId(uint64)
- `addToModelspace(entity: PyDbObject&)` -> uint64_t  — 添加实体到模型空间，返回新 ObjectId
- `addToCurrentspace(entity: PyDbObject&)` -> uint64_t  — 添加实体到当前空间，返回新 ObjectId
- `addToBlock(blockId: uint64_t, entity: PyDbObject&)` -> uint64_t  — 添加实体到指定 Block，返回新 ObjectId
- `objectIds(className: const std::string&)` -> py::list  — 遍历模型空间所有对象，返回 ObjectId 列表
- `getSysVar(name: const std::string&)` -> py::object  — 获取系统变量，如 db.getSysVar('OSMODE')
- `setSysVar(name: const std::string&, value: const py::object&)` -> bool  — 设置系统变量，如 db.setSysVar('OSMODE', 1)
- `dxfIn(filename: const std::string&)` -> bool  — 从 DXF 文件导入
- `dxfOut(filename: const std::string&, precision: int)` -> bool  — 导出为 DXF 文件
- `isObjectNonPersistent(objId: uint64_t)` -> bool  — 对象是否为非持久对象
- `markObjectNonPersistent(objId: uint64_t, value: bool)` -> bool  — 标记/取消标记对象为非持久
- `getBlockTable(mode: int)` -> PyDbBlockTable
- `getLayerTable(mode: int)` -> PyDbLayerTable
- `openObject(id: uint64_t, mode: int)` -> PyDbObject  — ) = 1,
			
- `classNameOf(id: uint64_t)` -> std::string  — 按 ObjectId 返回类名（如 AcDbCircle）；打不开返回空字符串
- `getTextStyleTable(mode: int)` -> PyDbTextStyleTable
- `getLinetypeTable(mode: int)` -> PyDbLinetypeTable
- `getViewTable(mode: int)` -> PyDbViewTable
- `getUCSTable(mode: int)` -> PyDbUCSTable
- `getViewportTable(mode: int)` -> PyDbViewportTable
- `getRegAppTable(mode: int)` -> PyDbRegAppTable
- `registerApp(appName: const AcString&)` -> bool  — 注册 XData 应用名；成功返回 true
- `isAppRegistered(appName: const AcString&)` -> bool  — 应用名是否已注册
- `getDimStyleTable(mode: int)` -> PyDbDimStyleTable
- `getGroupDictionary(mode: int)` -> PyDbDictionary
- `getMLStyleDictionary(mode: int)` -> PyDbDictionary
- `getLayoutDictionary(mode: int)` -> PyDbDictionary
- `getPlotStyleNameDictionary(mode: int)` -> PyDbDictionary
- `getMaterialDictionary(mode: int)` -> PyDbDictionary
- `getVisualStyleDictionary(mode: int)` -> PyDbDictionary
- `getColorDictionary(mode: int)` -> PyDbDictionary
- `getScaleListDictionary(mode: int)` -> PyDbDictionary
- `getNamedObjectsDictionary(mode: int)` -> PyDbDictionary
- `getPlotSettingsDictionary(mode: int)` -> PyDbDictionary
- `getDataLinkDictionary(mode: int)` -> PyDbDictionary
- `getSectionManager()` -> uint64_t  — 区块管理器 ObjectId(uint64)
- `updateDataLink(nDir: int, nOption: int)` -> bool  — 更新数据链接；成功返回 true
- `transactionManager()`  — 获取数据库事务管理器 (AcDbTransactionManager)
- `save()` -> bool  — 保存当前图形文件（原地保存）；成功返回 true
- `getFilename()` -> AcString  — 返回当前图形文件名；失败返回空串
- `getLayerStateManager()`  — 获取图层状态管理器 (PyDbDbLayerStateManager)
- `objectContextManager()`  — 获取对象上下文管理器 (PyDbObjectContextManager)
- `closeInput()`  — 关闭挂起的输入队列
- `curDb()`  — 当前文档的 AcDbDatabase（不拥有）
- `lockDocument()`  — 手动锁当前文档（kWrite）；返回 Acad::ErrorStatus 整数值
- `unlockDocument()`  — 手动解锁当前文档
- `isDocumentLocked()`  — 当前文档是否已锁


## 四、绘图类完整签名


### PyDbLine（父类 PyDbCurve）

验证：12/12 通过

构造：
`PyDbLine()` / `PyDbLine(object: AcDbLine*)` / `PyDbLine(start: const AcGePoint3d&, end: const AcGePoint3d&)`


- `startPoint()` -> AcGePoint3d
- `setStartPoint(p: const AcGePoint3d&)` -> bool
- `endPoint()` -> AcGePoint3d
- `setEndPoint(p: const AcGePoint3d&)` -> bool
- `colorIndex()` -> Adesk::UInt16
- `setColorIndex(color: Adesk::UInt16)` -> void
- `isClosed()` -> bool
- `normal()` -> AcGeVector3d
- `setNormal(n: const AcGeVector3d&)` -> bool
- `thickness()` -> double
- `setThickness(t: double)` -> bool
- `getOffsetCurvesGivenPlaneNormal(normal: const AcGeVector3d&, offset_dist: double)` -> std::vector<PyDbEntity>  — 按平面法向偏移直线，返回新曲线列表（未入库，需 appendEntity）


### PyDbCircle（父类 PyDbCurve）

验证：12/12 通过

构造：
`PyDbCircle()` / `PyDbCircle(cntr: const AcGePoint3d&, nrm: const AcGeVector3d&, radius: double)` / `PyDbCircle(object: AcDbCircle*)`


- `center()` -> AcGePoint3d
- `setCenter(p: const AcGePoint3d&)` -> bool
- `radius()` -> double
- `setRadius(r: double)` -> bool
- `normal()` -> AcGeVector3d
- `setNormal(n: const AcGeVector3d&)` -> bool
- `thickness()` -> double
- `setThickness(t: double)` -> bool
- `circumference()` -> double
- `setCircumference(c: double)` -> bool
- `diameter()` -> double
- `setDiameter(d: double)` -> bool


### PyDbArc（父类 PyDbCurve）

验证：14/14 通过

构造：
`PyDbArc()` / `PyDbArc(center: const AcGePoint3d&, radius: double, startAngle: double, endAngle: double)` / `PyDbArc(center: const AcGePoint3d&, normal: const AcGeVector3d&, radius: double, startAngle: double, endAngle: double)` / `PyDbArc(object: AcDbArc*)`


- `center()` -> AcGePoint3d
- `setCenter(p: const AcGePoint3d&)` -> bool
- `radius()` -> double
- `setRadius(r: double)` -> bool
- `startAngle()` -> double
- `setStartAngle(a: double)` -> bool
- `endAngle()` -> double
- `setEndAngle(a: double)` -> bool
- `totalAngle()` -> double
- `length()` -> double
- `thickness()` -> double
- `setThickness(t: double)` -> bool
- `normal()` -> AcGeVector3d
- `setNormal(n: const AcGeVector3d&)` -> bool


### PyDbEllipse（父类 PyDbCurve）

验证：22/22 通过

构造：
`PyDbEllipse()` / `PyDbEllipse(object: AcDbEllipse*)`


- `center()` -> AcGePoint3d
- `setCenter(p: const AcGePoint3d&)` -> bool
- `normal()` -> AcGeVector3d
- `majorAxis()` -> AcGeVector3d
- `minorAxis()` -> AcGeVector3d
- `majorRadius()` -> double
- `setMajorRadius(r: double)` -> bool
- `minorRadius()` -> double
- `setMinorRadius(r: double)` -> bool
- `radiusRatio()` -> double
- `setRadiusRatio(v: double)` -> bool
- `startAngle()` -> double
- `setStartAngle(a: double)` -> bool
- `endAngle()` -> double
- `setEndAngle(a: double)` -> bool
- `setStartParam(v: double)` -> bool
- `setEndParam(v: double)` -> bool
- `paramAtAngle(angle: double)` -> double
- `angleAtParam(param: double)` -> double
- `isNull()` -> bool
- `get()` -> std::tuple<AcGePoint3d, AcGeVector3d, AcGeVector3d, double, double, double>
- `set(center: const AcGePoint3d&, unitNormal: const AcGeVector3d&, majorAxis: const AcGeVector3d&, radiusRatio: double, startAngle: double, endAngle: double)` -> bool  — endAngle


### PyDbPolyline（父类 PyDbCurve）

验证：32/32 通过

构造：
`PyDbPolyline()` / `PyDbPolyline(object: AcDbPolyline*)`


- `numVerts()` -> unsigned int
- `getPointAt(index: unsigned int)` -> AcGePoint2d
- `setPointAt(index: unsigned int, pt: const AcGePoint2d&)` -> bool  — ), py::arg(
- `setClosed(closed: bool)` -> void  — closed
- `getBulgeAt(index: unsigned int)` -> double
- `setBulgeAt(index: unsigned int, bulge: double)` -> bool  — ), py::arg(
- `getWidthsAt(index: unsigned int)` -> py::tuple
- `setWidthsAt(index: unsigned int, startWidth: double, endWidth: double)` -> bool  — ), py::arg(
- `addVertexAt(index: unsigned int, pt: const AcGePoint2d&, bulge: double, startWidth: double, endWidth: double)` -> bool  — ) = -1.0, py::arg(
- `removeVertexAt(index: unsigned int)` -> bool
- `elevation()` -> double
- `setElevation(v: double)` -> void
- `thickness()` -> double
- `setThickness(v: double)` -> bool
- `constantWidth()` -> double
- `setConstantWidth(v: double)` -> bool
- `normal()` -> AcGeVector3d
- `setNormal(n: const AcGeVector3d&)` -> bool
- `setPlinegen(on: bool)` -> void
- `hasBulges()` -> bool
- `hasWidth()` -> bool
- `hasPlinegen()` -> bool
- `isOnlyLines()` -> bool
- `hasVertexIdentifiers()` -> bool
- `segType(index: unsigned int)` -> int
- `getLineSegAt(index: unsigned int)` -> AcGeLineSeg2d
- `getArcSegAt(index: unsigned int)` -> AcGeCircArc2d
- `getEcs()` -> AcGeMatrix3d
- `onSegAt(index: unsigned int, pt2d: const AcGePoint2d&)` -> std::tuple<bool, double>  — ), py::arg(
- `reset(reuse: bool, numVerts: unsigned int)` -> void  — ), py::arg(
- `makeClosedIfStartAndEndVertexCoincide(distTol: double)` -> bool  — distTol
- `convertFrom(src: const PyDbEntity&, transferId: bool)` -> bool  — ), py::arg(
- `convertTo(transferId: bool)` -> PyDb2dPolyline  — , &PyDbPolyline::join, 


### PyDbSpline（父类 PyDbCurve）

验证：29 通过 + setNurbsData(9参) 通过

构造：
`PyDbSpline()` / `PyDbSpline(object: AcDbSpline*)`


- `numControlPoints()` -> int
- `degree()` -> int
- `isNull()` -> bool
- `isRational()` -> bool
- `elevateDegree(newDegree: int)` -> bool  — newDegree
- `getControlPointAt(index: int)` -> AcGePoint3d
- `setControlPointAt(index: int, point: const AcGePoint3d&)` -> bool  — ), py::arg(
- `numFitPoints()` -> int
- `getFitPointAt(index: int)` -> AcGePoint3d
- `setFitPointAt(index: int, point: const AcGePoint3d&)` -> bool  — ), py::arg(
- `insertFitPointAt(index: int, point: const AcGePoint3d&)` -> bool  — ), py::arg(
- `removeFitPointAt(index: int)` -> bool
- `fitTolerance()` -> double
- `setFitTol(tol: double)` -> bool
- `weightAt(index: int)` -> double
- `setWeightAt(index: int, weight: double)` -> bool  — ), py::arg(
- `insertKnot(param: double)` -> bool
- `getFitTangents()` -> py::tuple
- `setFitTangents(startTangent: const AcGeVector3d&, endTangent: const AcGeVector3d&)` -> bool  — endTangent
- `hasFitData()` -> bool
- `purgeFitData()` -> bool
- `updateFitData()` -> bool
- `getFitData()` -> std::tuple<AcGePoint3dArray, bool, AcGeVector3d, AcGeVector3d, int, int, double>
- `getNurbsData()` -> std::tuple<int, bool, bool, bool, AcGePoint3dArray, AcGeDoubleArray, AcGeDoubleArray, double, double>
- `setFitData(fitPoints: const AcGePoint3dArray&, startTangent: const AcGeVector3d&, endTangent: const AcGeVector3d&, knotParam: int, degree: int, fitTolerance: double)` -> bool  — fitTolerance
- `setNurbsData(degree: int, rational: bool, closed: bool, periodic: bool, controlPoints: const AcGePoint3dArray&, knots: const AcGeDoubleArray&, weights: const AcGeDoubleArray&, controlPtTol: double, knotTol: double)` -> bool  — ) = 0.0, 
- `toPolyline(precision: int, numOfVerticesThreshold: unsigned int)` -> PyDbCurve  — 样条近似为折线（新实体未入库，需 appendEntity）
- `toPolylineAs(bConvertAsArcs: bool, bToLWPolyline: bool, precision: int)` -> PyDbCurve  — 按弧/直线、2d/LW 类型近似为折线（新实体未入库）
- `insertControlPointAt(knotParam: double, ctrlPt: const AcGePoint3d&, weight: double)` -> bool  — 在给定节点参数处插入控制点
- `removeControlPointAt(index: int)` -> bool  — 删除指定控制点


### PyDbText（父类 PyDbEntity）

验证：40/40 通过

构造：
`PyDbText()` / `PyDbText(object: AcDbText*)`


- `position()` -> AcGePoint3d
- `setPosition(p: const AcGePoint3d&)` -> bool
- `alignmentPoint()` -> AcGePoint3d
- `setAlignmentPoint(p: const AcGePoint3d&)` -> bool
- `isDefaultAlignment()` -> bool
- `normal()` -> AcGeVector3d
- `setNormal(n: const AcGeVector3d&)` -> bool
- `thickness()` -> double
- `setThickness(t: double)` -> bool
- `oblique()` -> double
- `setOblique(v: double)` -> bool
- `height()` -> double
- `setHeight(h: double)` -> bool
- `rotation()` -> double
- `setRotation(r: double)` -> bool
- `widthFactor()` -> double
- `setWidthFactor(v: double)` -> bool
- `textString()` -> AcString
- `setTextString(text: const AcString&)` -> bool
- `isMirroredInX()` -> bool
- `mirrorInX(v: bool)` -> bool
- `isMirroredInY()` -> bool
- `mirrorInY(v: bool)` -> bool
- `horizontalMode()` -> int
- `setHorizontalMode(v: int)` -> bool
- `verticalMode()` -> int
- `setVerticalMode(v: int)` -> bool
- `justification()` -> int
- `setJustification(v: int)` -> bool
- `hitTest(pt: const AcGePoint3d&)` -> bool
- `correctSpelling()` -> int
- `textStyle()` -> uint64_t
- `setTextStyle(styleId: uint64_t)` -> bool  — styleId
- `adjustAlignment()` -> bool
- `getBoundingPoints()` -> std::vector<AcGePoint3d>
- `convertFieldToText()` -> bool
- `getPlane()` -> py::tuple
- `isPlanar()` -> bool
- `removeField(propName: const AcString&)`  — propName
- `removeField(fieldId: uint64_t)`  — fieldId


### PyDbMText（父类 PyDbEntity）

验证：47 通过 / getBoundingPoints 需入库

构造：
`PyDbMText()` / `PyDbMText(object: AcDbMText*)`


- `location()` -> AcGePoint3d
- `setLocation(p: const AcGePoint3d&)` -> bool
- `normal()` -> AcGeVector3d
- `setNormal(n: const AcGeVector3d&)` -> bool
- `direction()` -> AcGeVector3d
- `setDirection(v: const AcGeVector3d&)` -> bool
- `rotation()` -> double
- `setRotation(r: double)` -> bool
- `width()` -> double
- `setWidth(v: double)` -> bool
- `ascent()` -> double
- `descent()` -> double
- `textHeight()` -> double
- `setTextHeight(h: double)` -> bool
- `attachment()` -> int
- `setAttachment(v: int)` -> bool
- `flowDirection()` -> int
- `setFlowDirection(v: int)` -> bool
- `contents()` -> AcString
- `setContents(text: const AcString&)` -> bool
- `contentsRTF()` -> AcString
- `setContentsRTF(rtf: const AcString&)` -> bool
- `text()` -> AcString
- `actualHeight()` -> double
- `actualWidth()` -> double
- `lineSpacingStyle()` -> int
- `setLineSpacingStyle(v: int)` -> bool
- `lineSpacingFactor()` -> double
- `setLineSpacingFactor(v: double)` -> bool
- `showBorders()` -> bool
- `setShowBorders(v: bool)` -> bool
- `backgroundFillOn()` -> bool
- `setBackgroundFill(v: bool)` -> bool
- `backgroundFillColor()` -> PyColor  — 背景填充颜色（Color 值对象）
- `setBackgroundFillColor(color: const PyColor&)` -> bool  — 设置背景填充颜色
- `useBackgroundColorOn()` -> bool
- `setUseBackgroundColor(v: bool)` -> bool
- `useWordBreak()` -> bool
- `setUseWordBreak(v: bool)` -> bool
- `hitTest(pt: const AcGePoint3d&)` -> bool
- `correctSpelling()` -> int
- `height()` -> double
- `setHeight(h: double)` -> bool
- `textStyle()` -> uint64_t
- `setTextStyle(styleId: uint64_t)` -> bool  — styleId
- `getBoundingPoints()` -> std::vector<AcGePoint3d>
- `getBoundingPointsByLine()` -> std::vector<std::vector<AcGePoint3d>>
- `setAttachmentMovingLocation(v: int)` -> bool
- `getPlane()` -> py::tuple
- `isPlanar()` -> bool
- `explodeFragments(callback: const py::object&)` -> void  — underlineOff


### PyDbHatch（父类 PyDbEntity）

验证：62/62 通过

构造：
`PyDbHatch()` / `PyDbHatch(object: AcDbHatch*)`


- `associative()` -> bool
- `setAssociative(a: bool)` -> bool
- `numLoops()` -> int
- `loopTypeAt(loopIndex: int)` -> int  — loopIndex
- `elevation()` -> double
- `setElevation(v: double)` -> bool
- `normal()` -> AcGeVector3d
- `setNormal(v: const AcGeVector3d&)` -> bool
- `hatchObjectType()` -> int
- `setHatchObjectType(v: int)` -> bool
- `isGradient()` -> bool
- `isHatch()` -> bool
- `isSolidFill()` -> bool
- `originPoint()` -> AcGePoint2d
- `setOriginPoint(p: const AcGePoint2d&)` -> bool
- `patternType()` -> int
- `patternName()` -> AcString
- `setPattern(patType: int, patName: const AcString&)` -> bool  — patName
- `patternAngle()` -> double
- `setPatternAngle(v: double)` -> bool
- `patternSpace()` -> double
- `setPatternSpace(v: double)` -> bool
- `patternScale()` -> double
- `setPatternScale(v: double)` -> bool
- `patternDouble()` -> bool
- `setPatternDouble(v: bool)` -> bool
- `numPatternDefinitions()` -> int  — 图案定义数量
- `getPatternDefinitionAt(index: int)` -> std::tuple<double, double, double, double, double, AcGeDoubleArray>  — 读取图案定义: (angle, baseX, baseY, offsetX, offsetY, dashes)
- `hatchStyle()` -> int
- `setHatchStyle(v: int)` -> bool
- `numHatchLines()` -> int
- `area()` -> double
- `evaluateHatch(underestimate: bool)` -> bool  — underestimate
- `evaluateHatchAllScales(underestimate: bool)` -> bool  — underestimate
- `lineGenerationEnabled()` -> bool
- `setLineGenerationEnabled(v: bool)` -> bool
- `gradientType()` -> int
- `gradientName()` -> AcString
- `gradientAngle()` -> double
- `setGradientAngle(v: double)` -> bool
- `gradientOneColorMode()` -> bool
- `setGradientOneColorMode(v: bool)` -> bool
- `shadeTintValue()` -> float
- `setShadeTintValue(v: float)` -> bool
- `gradientShift()` -> float
- `setGradientShift(v: float)` -> bool
- `appendLoopPoints(loop_type: int, points: const AcGePoint2dArray&, bulges: const AcGeDoubleArray&)`  — 按 Point2d 点列+凸度追加边界环
- `appendLoopEntities(loop_type: int, entity_ids: const AcDbObjectIdArray&)`  — 按实体 id 数组追加关联边界环
- `removeLoopAt(index: int)`  — 删除指定索引的边界环
- `backgroundColor()` -> std::tuple<int, int, int>  — 返回背景色 (r,g,b) 0-255
- `setBackgroundColor(r: int, g: int, b: int)` -> bool  — 设置背景色 0-255
- `getAssocObjIds()` -> std::vector<uint64_t>  — 所有关联边界对象 ID 列表
- `getAssocObjIdsAt(loop_index: int)` -> std::vector<uint64_t>  — 指定环的关联边界对象 ID 列表
- `removeAssocObjIds()` -> bool  — 解除所有边界对象关联
- `getLoopAt(loop_index: int)` -> std::tuple<int, AcGePoint2dArray, AcGeDoubleArray>  — 读取环: 返回 (loopType, 顶点列, 凸度列)
- `insertLoopAtPoints(loop_index: int, loop_type: int, vertices: const AcGePoint2dArray&, bulges: const AcGeDoubleArray&)` -> bool  — 在指定索引插入点列环
- `insertLoopAtEntities(loop_index: int, loop_type: int, entity_ids: const AcDbObjectIdArray&)` -> bool  — 在指定索引插入实体关联环
- `setGradient(grad_type: int, grad_name: const AcString&)` -> bool  — 设置渐变填充图案
- `setGradientColors(colors: const std::vector<std::tuple<int, int, int>>&, values: const std::vector<float>&)` -> bool  — 设置渐变色 [(r,g,b),...] 与归一化位置 [0..1]
- `evaluateGradientColorAt(value: float)` -> std::tuple<int, int, int>  — 评估渐变在 value[0..1] 处的颜色 (r,g,b)
- `setShadeTintValueAndColor2(value: float)` -> bool  — 单色模式下设置亮度并据此计算第二渐变颜色
- `getGradientColors()` -> std::tuple<std::vector<std::tuple<int, int, int>>, std::vector<float>>  — 读取渐变色 [(r,g,b),...] 与位置 [0..1]


### PyDbSolid（父类 PyDbEntity）

验证：6/6 通过

构造：
`PyDbSolid()` / `PyDbSolid(pt0: const AcGePoint3d&, pt1: const AcGePoint3d&, pt2: const AcGePoint3d&)` / `PyDbSolid(pt0: const AcGePoint3d&, pt1: const AcGePoint3d&, pt2: const AcGePoint3d&, pt3: const AcGePoint3d&)` / `PyDbSolid(object: AcDbSolid*)`


- `getPointAt(index: Adesk::UInt16)` -> AcGePoint3d
- `setPointAt(index: Adesk::UInt16, p: const AcGePoint3d&)` -> bool  — ), py::arg(
- `thickness()` -> double
- `setThickness(t: double)` -> bool
- `normal()` -> AcGeVector3d
- `setNormal(n: const AcGeVector3d&)` -> bool


### PyDbPoint（父类 PyDbEntity）

验证：8/8 通过

构造：
`PyDbPoint()` / `PyDbPoint(position: const AcGePoint3d&)` / `PyDbPoint(object: AcDbPoint*)`


- `position()` -> AcGePoint3d
- `setPosition(p: const AcGePoint3d&)` -> bool
- `thickness()` -> double
- `setThickness(t: double)` -> bool
- `normal()` -> AcGeVector3d
- `setNormal(n: const AcGeVector3d&)` -> bool
- `ecsRotation()` -> double
- `setEcsRotation(r: double)` -> bool


### PyDbBlockReference（父类 PyDbEntity）

验证：19 通过 / 4 个 transform 需真实块定义（已验证可用）

构造：
`PyDbBlockReference()` / `PyDbBlockReference(object: AcDbBlockReference*)`


- `position()` -> AcGePoint3d
- `setPosition(p: const AcGePoint3d&)` -> bool
- `rotation()` -> double
- `setRotation(r: double)` -> bool
- `blockTableRecordId()` -> uint64_t
- `setBlockTableRecord(blockRecordId: uint64_t)` -> bool  — blockRecordId
- `scaleFactors()` -> AcGeScale3d
- `setScaleFactors(s: const AcGeScale3d&)` -> bool
- `nonAnnotationScaleFactors()` -> AcGeScale3d
- `blockTransform()` -> AcGeMatrix3d
- `nonAnnotationBlockTransform()` -> AcGeMatrix3d
- `normal()` -> AcGeVector3d
- `setNormal(n: const AcGeVector3d&)` -> bool
- `treatAsAcDbBlockRefForExplode()` -> bool
- `explodeToOwnerSpace()` -> bool
- `geomExtentsBestFit(parentXform: const AcGeMatrix3d&)` -> std::tuple<AcGePoint3d, AcGePoint3d>  — 返回块参照最佳拟合包围盒 (minPoint, maxPoint)
- `setBlockTransform(xform: const AcGeMatrix3d&)` -> bool  — 设置块参照变换矩阵
- `getPlane()` -> std::tuple<AcGePlane, int>  — 返回 (plane, planarity 枚举 int)
- `isPlanar()` -> bool
- `openAttribute(attId: uint64_t, mode: AcDb::OpenMode, openErasedOne: bool)` -> PyDbAttribute  — ) = false,
			
- `attributeIds()` -> py::list  — 返回块参照上所有属性的 id 列表
- `hasAttributes()`  — 占位：ARX 无 hasAttributes，请用 attributeCount
- `attributeCount()` -> int  — 属性数量（-1 表示迭代器创建失败）
- `attribData()` -> py::list  — 实例属性列表：{tag,txt,x,y,ax,ay,h,rot,ha,va,inv,ly,ci}
- `appendAttribute(attribute: PyDbAttribute&)`  — 向块参照追加一个属性


### PyDbAttribute（父类 PyDbText）

验证：18/18 通过

构造：
`PyDbAttribute()` / `PyDbAttribute(position: const AcGePoint3d&, text: const AcString&, tag: const AcString&)` / `PyDbAttribute(object: AcDbAttribute*)`


- `tag()` -> AcString
- `setTag(tag: const AcString&)` -> bool
- `isInvisible()` -> bool
- `setInvisible(v: bool)` -> bool
- `isConstant()` -> bool
- `isVerifiable()` -> bool
- `isPreset()` -> bool
- `fieldLength()` -> int
- `setFieldLength(v: int)` -> bool
- `lockPositionInBlock()` -> bool
- `setLockPositionInBlock(v: bool)` -> bool
- `isReallyLocked()` -> bool
- `convertIntoMTextAttribute(val: bool)` -> bool  — ) = true, 
- `isMTextAttribute()` -> bool  — 是否为多行属性
- `setAttributeFromBlock(blkXform: const AcGeMatrix3d&)`  — 按块变换矩阵更新属性
- `setAttributeFromBlock(pAttdef: const PyDbAttributeDefinition*, blkXform: const AcGeMatrix3d&)`  — 按属性定义与块变换矩阵更新属性
- `setMTextAttribute(pMText: PyDbMText*)` -> bool  — 替换内部多行文本（成功后由属性接管所有权）
- `setMTextAttributeConst(pMText: const PyDbMText*)` -> bool  — 替换内部多行文本（const 版本，成功后由属性接管所有权）
- `updateMTextAttribute()` -> bool  — 从属性基本属性刷新内部多行文本
- `getMTextAttribute()` -> PyDbMText  — 获取内部多行文本的克隆（调用方负责释放）
- `getMTextAttributeConst()` -> PyDbMText  — 获取内部多行文本的 const 克隆（调用方负责释放）


### PyDbAttributeDefinition（父类 PyDbText）

构造：
`PyDbAttributeDefinition()` / `PyDbAttributeDefinition(position: const AcGePoint3d&, text: const AcString&, tag: const AcString&, prompt: const AcString&)` / `PyDbAttributeDefinition(object: AcDbAttributeDefinition*)`


- `tag()` -> AcString
- `setTag(tag: const AcString&)` -> bool
- `prompt()` -> AcString
- `setPrompt(prompt: const AcString&)` -> bool  — prompt
- `isInvisible()` -> bool
- `setInvisible(v: bool)` -> bool
- `isConstant()` -> bool
- `setConstant(v: bool)` -> bool
- `isVerifiable()` -> bool
- `setVerifiable(v: bool)` -> bool
- `isPreset()` -> bool
- `setPreset(v: bool)` -> bool
- `fieldLength()` -> int
- `setFieldLength(v: int)` -> bool
- `lockPositionInBlock()` -> bool
- `setLockPositionInBlock(v: bool)` -> bool
- `adjustAlignment(db: const PyDbDatabase*)` -> bool  — 对齐调整，可传入数据库（不传则继承自 PyDbText 的无参版本）
- `convertIntoMTextAttributeDefinition(val: bool)` -> bool  — ) = true, 
- `isMTextAttributeDefinition()` -> bool  — 是否为多行属性定义
- `setMTextAttributeDefinition(pMText: PyDbMText*)` -> bool  — 替换内部多行文本（成功后由属性定义接管所有权）
- `setMTextAttributeDefinitionConst(pMText: const PyDbMText*)` -> bool  — 替换内部多行文本（const 版本，成功后由属性定义接管所有权）
- `updateMTextAttributeDefinition()` -> bool  — 从属性定义基本属性刷新内部多行文本
- `getMTextAttributeDefinition()` -> PyDbMText  — 获取内部多行文本的克隆（调用方负责释放）
- `getMTextAttributeDefinitionConst()` -> PyDbMText  — 获取内部多行文本的 const 克隆（调用方负责释放）


### PyDbMline（父类 PyDbEntity）

验证：24 通过 / setStyle 返回 False

构造：
`PyDbMline()` / `PyDbMline(object: AcDbMline*)`


- `scale()` -> double
- `setScale(s: double)` -> bool
- `numVertices()` -> int
- `vertexAt(index: int)` -> AcGePoint3d
- `closedMline()` -> bool
- `setClosedMline(c: bool)` -> bool
- `justification()` -> int
- `setJustification(v: int)` -> bool
- `normal()` -> AcGeVector3d
- `setNormal(v: const AcGeVector3d&)` -> bool
- `appendSeg(p: const AcGePoint3d&)` -> bool
- `moveVertexAt(index: int, p: const AcGePoint3d&)` -> bool  — ), py::arg(
- `supressStartCaps()` -> bool
- `setSupressStartCaps(v: bool)` -> bool
- `supressEndCaps()` -> bool
- `setSupressEndCaps(v: bool)` -> bool
- `element(pt: const AcGePoint3d&)` -> int
- `axisAt(index: int)` -> AcGeVector3d
- `miterAt(index: int)` -> AcGeVector3d
- `getClosestPointTo(givenPoint: const AcGePoint3d&, extend: bool, excludeCaps: bool)`  — excludeCaps
- `getClosestPointTo(givenPoint: const AcGePoint3d&, normal: const AcGeVector3d&, extend: bool, excludeCaps: bool)`  — excludeCaps
- `getPlane()` -> AcGePlane
- `removeLastSeg()` -> AcGePoint3d  — 删除最后一段，返回新的末点；失败返回默认点
- `style()` -> uint64_t  — 返回当前 MlineStyle 的 ObjectId (uint64)，空为 0
- `setStyle(styleId: uint64_t)` -> bool  — 设置 MlineStyle 的 ObjectId (uint64)


### PyDbLeader（父类 PyDbCurve）

验证：49 通过 / setDimstyleData 需已有 DimStyleTableRecord

构造：
`PyDbLeader()` / `PyDbLeader(object: AcDbLeader*)`


- `numVertices()` -> int
- `appendVertex(pt: const AcGePoint3d&)` -> bool
- `firstVertex()` -> AcGePoint3d
- `lastVertex()` -> AcGePoint3d
- `vertexAt(i: int)` -> AcGePoint3d
- `setVertexAt(i: int, pt: const AcGePoint3d&)` -> bool  — ), py::arg(
- `removeLastVertex()` -> void
- `hasArrowHead()` -> bool
- `enableArrowHead()` -> void
- `disableArrowHead()` -> void
- `setHasArrowHead(enable: bool)` -> void  — enable
- `hasHookLine()` -> bool
- `setToSplineLeader()` -> void
- `setToStraightLeader()` -> void
- `isSplined()` -> bool
- `setSplined(b: bool)` -> void
- `annotationObjId()` -> uint64_t
- `attachAnnotation(anno_id: uint64_t)`  — 把注解实体 id 关联到引线
- `detachAnnotation()` -> bool
- `evaluateLeader()` -> bool
- `annotationOffset()` -> AcGeVector3d
- `setAnnotationOffset(offset: const AcGeVector3d&)` -> bool  — offset
- `annoType()` -> int
- `annoHeight()` -> double
- `annoWidth()` -> double
- `dimasz()` -> double
- `setDimasz(v: double)` -> bool
- `dimgap()` -> double
- `setDimgap(v: double)` -> bool
- `dimscale()` -> double
- `setDimscale(v: double)` -> bool
- `dimtxt()` -> double
- `setDimtxt(v: double)` -> bool
- `dimtad()` -> int
- `setDimtad(v: int)` -> bool
- `dimsah()` -> bool
- `setDimsah(v: bool)` -> bool
- `dimensionStyle()` -> uint64_t
- `dimldrblk()` -> uint64_t
- `dimlwd()` -> int
- `dimtxsty()` -> uint64_t
- `normal()` -> AcGeVector3d
- `setAnnotationObjId(anno_id: uint64_t)` -> bool  — anno_id
- `setDimVars()` -> void
- `setDimensionStyle(style_id: uint64_t)` -> void  — style_id
- `setDimldrblk(blk_id: uint64_t)` -> bool  — blk_id
- `setDimlwd(lw: int)` -> bool
- `setDimstyleData(style_id: uint64_t)` -> bool  — style_id
- `setDimtxsty(txsty_id: uint64_t)` -> bool  — txsty_id
- `setPlane(plane: const AcGePlane&)` -> void
- `getDimstyleData()` -> PyDbDimStyleTableRecord*  — 取出副本样式记录（包装类接管，自动释放）


### PyDbTable（父类 PyDbBlockReference）

验证：37/37 通过

构造：
`PyDbTable()` / `PyDbTable(object: AcDbTable*)`


- `numRows()` -> Adesk::UInt32
- `numColumns()` -> Adesk::UInt32
- `setSize(nRows: int, nCols: int)` -> bool  — ), py::arg(
- `width()` -> double
- `setWidth(v: double)` -> bool
- `height()` -> double
- `setHeight(v: double)` -> bool
- `columnWidth(col: int)` -> double
- `setColumnWidth(col: int, width: double)` -> bool  — ), py::arg(
- `rowHeight(row: int)` -> double
- `setRowHeight(row: int, height: double)` -> bool  — ), py::arg(
- `textString(row: int, col: int)` -> AcString  — ), py::arg(
- `setTextString(row: int, col: int, text: const AcString&)` -> bool  — ), py::arg(
- `textHeight(row: int, col: int)` -> double  — ), py::arg(
- `setTextHeight(row: int, col: int, h: double)` -> bool  — ), py::arg(
- `insertColumns(col: int, width: double, nCols: int)` -> bool  — ), py::arg(
- `deleteColumns(col: int, nCols: int)` -> bool  — ), py::arg(
- `insertRows(row: int, height: double, nRows: int)` -> bool  — ), py::arg(
- `deleteRows(row: int, nRows: int)` -> bool  — ), py::arg(
- `mergeCells(minRow: int, maxRow: int, minCol: int, maxCol: int)` -> bool  — maxCol
- `unmergeCells(minRow: int, maxRow: int, minCol: int, maxCol: int)` -> bool  — maxCol
- `isMergedCell(row: int, col: int)` -> bool  — ), py::arg(
- `isRegenerateTableSuppressed()` -> bool
- `suppressRegenerateTable(v: bool)` -> void
- `setPosition(p: const AcGePoint3d&)` -> bool
- `flowDirection()` -> int
- `setFlowDirection(v: int)` -> bool
- `generateLayout()` -> bool
- `tableStyleId()` -> uint64_t
- `setNumRows(nRows: int)` -> bool
- `setNumColumns(nCols: int)` -> bool
- `setTableStyle(id: uint64_t)` -> bool
- `recomputeTableBlock(forceUpdate: bool)` -> bool  — forceUpdate
- `direction()` -> AcGeVector3d
- `setDirection(v: const AcGeVector3d&)` -> bool
- `rotation()` -> double
- `setRotation(v: double)` -> bool


### PyDbAlignedDimension（父类 PyDbDimension）

验证：12/12 通过

构造：
`PyDbAlignedDimension()` / `PyDbAlignedDimension(object: AcDbAlignedDimension*)`


- `xLine1Point()` -> AcGePoint3d
- `setXLine1Point(p: const AcGePoint3d&)` -> bool
- `xLine2Point()` -> AcGePoint3d
- `setXLine2Point(p: const AcGePoint3d&)` -> bool
- `dimLinePoint()` -> AcGePoint3d
- `setDimLinePoint(p: const AcGePoint3d&)` -> bool
- `oblique()` -> double
- `setOblique(v: double)` -> bool
- `jogSymbolOn()` -> bool
- `setJogSymbolOn(v: bool)` -> bool
- `jogSymbolPosition()` -> AcGePoint3d
- `setJogSymbolPosition(p: const AcGePoint3d&)` -> bool


### PyDbRotatedDimension（父类 PyDbDimension）

验证：14/14 通过

构造：
`PyDbRotatedDimension()` / `PyDbRotatedDimension(object: AcDbRotatedDimension*)`


- `xLine1Point()` -> AcGePoint3d
- `setXLine1Point(p: const AcGePoint3d&)` -> bool
- `xLine2Point()` -> AcGePoint3d
- `setXLine2Point(p: const AcGePoint3d&)` -> bool
- `dimLinePoint()` -> AcGePoint3d
- `setDimLinePoint(p: const AcGePoint3d&)` -> bool
- `oblique()` -> double
- `setOblique(v: double)` -> bool
- `rotation()` -> double
- `setRotation(r: double)` -> bool
- `jogSymbolOn()` -> bool
- `setJogSymbolOn(v: bool)` -> bool
- `jogSymbolPosition()` -> AcGePoint3d
- `setJogSymbolPosition(p: const AcGePoint3d&)` -> bool


### PyDbRadialDimension（父类 PyDbDimension）

构造：
`PyDbRadialDimension()` / `PyDbRadialDimension(object: AcDbRadialDimension*)`


- `center()` -> AcGePoint3d
- `setCenter(p: const AcGePoint3d&)` -> bool
- `chordPoint()` -> AcGePoint3d
- `setChordPoint(p: const AcGePoint3d&)` -> bool
- `leaderLength()` -> double
- `setLeaderLength(l: double)` -> bool
- `extArcStartAngle()` -> double
- `setExtArcStartAngle(a: double)` -> bool
- `extArcEndAngle()` -> double
- `setExtArcEndAngle(a: double)` -> bool


### PyDbDiametricDimension（父类 PyDbDimension）

构造：
`PyDbDiametricDimension()` / `PyDbDiametricDimension(object: AcDbDiametricDimension*)`


- `chordPoint()` -> AcGePoint3d
- `setChordPoint(p: const AcGePoint3d&)` -> bool
- `farChordPoint()` -> AcGePoint3d
- `setFarChordPoint(p: const AcGePoint3d&)` -> bool
- `leaderLength()` -> double
- `setLeaderLength(l: double)` -> bool
- `extArcStartAngle()` -> double
- `setExtArcStartAngle(a: double)` -> bool
- `extArcEndAngle()` -> double
- `setExtArcEndAngle(a: double)` -> bool


### PyDbOrdinateDimension（父类 PyDbDimension）

构造：
`PyDbOrdinateDimension()` / `PyDbOrdinateDimension(object: AcDbOrdinateDimension*)`


- `definingPoint()` -> AcGePoint3d
- `setDefiningPoint(p: const AcGePoint3d&)` -> bool
- `leaderEndPoint()` -> AcGePoint3d
- `setLeaderEndPoint(p: const AcGePoint3d&)` -> bool
- `origin()` -> AcGePoint3d
- `setOrigin(p: const AcGePoint3d&)` -> bool
- `isUsingXAxis()` -> bool
- `isUsingYAxis()` -> bool
- `useXAxis()` -> bool
- `useYAxis()` -> bool
- `setUsingXAxis(v: bool)` -> bool
- `setUsingYAxis(v: bool)` -> bool


### PyDbWipeout（父类 PyDbEntity）

构造：
`PyDbWipeout()` / `PyDbWipeout(object: AcDbWipeout*)`


- `frame()` -> bool
- `isClipped()` -> bool
- `brightness()` -> int
- `setBrightness(value: int)` -> bool
- `contrast()` -> int
- `setContrast(value: int)` -> bool
- `fade()` -> int
- `setFade(value: int)` -> bool
- `scale()` -> AcGeVector2d
- `imageSize(cached: bool)` -> AcGeVector2d  — cached
- `append(id: uint64_t)` -> bool  — 追加裁剪边界对象 id (uint64 oldId)


## 五、其余类索引

| 类 | 父类 | 方法数 |
|---|---|---|
| AcGiMaterialColor | - | 169 |
| AcGiMaterialTexture | - | 169 |
| AcGiImageTexture | AcGiMaterialTexture | 169 |
| AcGiImageFileTexture | AcGiImageTexture | 169 |
| AcGiProceduralTexture | AcGiMaterialTexture | 169 |
| AcGiGenericTexture | AcGiProceduralTexture | 169 |
| AcGiWoodTexture | AcGiProceduralTexture | 169 |
| AcGiMarbleTexture | AcGiProceduralTexture | 169 |
| AcGiMaterialMap | - | 169 |
| AcGiMaterialTraits | - | 169 |
| AcGiColorRGB | - | 169 |
| AcGiColorRGBA | - | 169 |
| AcGiShadowParameters | - | 169 |
| AcGiLightAttenuation | - | 169 |
| AcGiLightTraits | - | 169 |
| AcGiStandardLightTraits | AcGiLightTraits | 169 |
| AcGiPointLightTraits | AcGiStandardLightTraits | 169 |
| AcGiSpotLightTraits | AcGiStandardLightTraits | 169 |
| AcGiDistantLightTraits | AcGiStandardLightTraits | 169 |
| AcGiWebLightTraits | AcGiPointLightTraits | 169 |
| AcGiSolidBackgroundTraits | - | 169 |
| AcGiGradientBackgroundTraits | - | 169 |
| AcGiImageBackgroundTraits | - | 169 |
| AcGiGroundPlaneBackgroundTraits | - | 169 |
| AcGiSkyBackgroundTraits | - | 169 |
| AcGiIBLBackgroundTraits | - | 169 |
| AcGiRenderEnvironmentTraits | - | 169 |
| AcGiRenderSettingsTraits | - | 169 |
| AcGiMentalRayRenderSettingsTraits | AcGiRenderSettingsTraits | 169 |
| AcGiRapidRTRenderSettingsTraits | - | 169 |
| AcGiToneOperatorParameters | - | 169 |
| AcGiPhotographicExposureParameters | AcGiToneOperatorParameters | 169 |
| AdsName | - | 152 |
| SelectionSet | - | 152 |
| PyEdResult | - | 152 |
| PromptOptions | - | 152 |
| EntityValue | - | 152 |
| SubEntityValue | - | 152 |
| PyGiSubEntityTraits | PyRxObject | 145 |
| PyGiDrawableTraits | PyGiSubEntityTraits | 145 |
| PyGiCommonDraw | PyRxObject | 145 |
| PyGiWorldDraw | PyGiCommonDraw | 145 |
| PyGiViewportDraw | PyGiCommonDraw | 145 |
| PyGiGeometry | PyRxObject | 145 |
| PyGiWorldGeometry | PyGiGeometry | 145 |
| PyGiViewportGeometry | PyGiGeometry | 145 |
| PyGiDrawable | PyRxObject | 145 |
| PyGiTextStyle | - | 145 |
| PyGiTextEngine | - | 145 |
| PyRxObject | - | 85 |
| PyRxClass | PyRxObject | 85 |
| DxfValue | - | 85 |
| TypedValue | - | 85 |
| TvList | - | 85 |
| Resbuf | - | 85 |
| PyDbViewport | PyDbEntity | 85 |
| PyDbViewportTableRecord | PyDbSymbolTableRecord | 73 |
| AcCmEntityColor | - | 73 |
| AcCmTransparency | - | 73 |
| AcGiVariant | - | 73 |
| AcGiMapper | - | 73 |
| PyDbMLeaderStyle | PyDbObject | 65 |
| PyDbDimension | PyDbEntity | 62 |
| PyDbDeepCloneFiler | PyRxObject | 56 |
| PyDbMTextFragment | - | 52 |
| PyDbMPolygon | PyDbEntity | 51 |
| PyDbViewTableRecord | PyDbSymbolTableRecord | 49 |
| PyDbMLeader | PyDbEntity | 46 |
| PyDbTableStyle | PyDbObject | 42 |
| PyDbPlotSettings | PyDbObject | 41 |
| PyDbMaterial | PyDbObject | 39 |
| AcGeMatrix3d | - | 39 |
| PyDbNurbSurface | PyDbSurface | 38 |
| AcGiEdgeData | - | 38 |
| AcGiFaceData | - | 38 |
| AcGiVertexData | - | 38 |
| PyDbDate | - | 37 |
| PyDbPolyline2d | PyDbCurve | 35 |
| AcGeKnotVector | - | 35 |
| AcGeVector3d | - | 35 |
| PyDbMlineStyle | PyDbObject | 34 |
| PyDbSweepOptions | - | 34 |
| PyDbRasterImage | PyDbEntity | 33 |
| AcGeInterval | - | 33 |
| PyDbLoftOptions | - | 31 |
| AcGeCurve3d | AcGeEntity3d | 31 |
| AcGiPolyline | - | 31 |
| PyDbGroup | PyDbObject | 30 |
| AcGePoint3d | - | 30 |
| AcGeMatrix2d | - | 29 |
| AcGeNurbCurve2d | AcGeSplineEnt2d | 29 |
| AcGeNurbCurve3d | AcGeSplineEnt3d | 29 |
| AcGeCurve2d | AcGeEntity2d | 28 |
| PyDbDimAssoc | PyDbObject | 26 |
| PyDbLinetypeTableRecord | PyDbSymbolTableRecord | 26 |
| PyDbOle2Frame | PyDbEntity | 26 |
| PyDbSubDMesh | PyDbEntity | 26 |
| AcGeVector2d | - | 26 |
| PyDbDatabaseSummaryInfo | - | 25 |
| PyDbHelix | PyDbSpline | 24 |
| PyDbPlotSettingsValidator | - | 24 |
| PyDbArcDimension | PyDbDimension | 23 |
| PyDbFcf | PyDbEntity | 23 |
| PyDbShape | PyDbEntity | 23 |
| PyDb3dSolid | PyDbEntity | 22 |
| AcGeEllipArc3d | AcGeCurve3d | 22 |
| AcGePoint2d | - | 21 |
| PyDbPolygonMesh | PyDbEntity | 20 |
| PyDbTextStyleTableRecord | PyDbSymbolTableRecord | 20 |
| AcGiViewport | - | 20 |
| PyDbDictionary | PyDbObject | 19 |
| AcEdDragStyle | - | 19 |
| AcEdJig | PyEdJig | 19 |
| AcGeCircArc3d | AcGeCurve3d | 19 |
| AcGeEllipArc2d | AcGeCurve2d | 19 |
| PyDbRadialDimensionLarge | PyDbDimension | 18 |
| AcGeCurveCurveInt3d | AcGeEntity3d | 18 |
| PyDbSection | PyDbEntity | 17 |
| AcGeCurveCurveInt2d | AcGeEntity2d | 17 |
| AcGeScale2d | - | 17 |
| AcGeScale3d | - | 17 |
| AcGeSplineEnt2d | AcGeCurve2d | 17 |
| AcGeSplineEnt3d | AcGeCurve3d | 17 |
| AcGeTorus | AcGeSurface | 17 |
| AcGiContext | - | 17 |
| AcGiGdiDrawObject | - | 17 |
| PyDb2dVertex | PyDbVertex | 16 |
| PyDbPolyline3d | PyDbCurve | 16 |
| PyDbDetailViewStyle | PyDbModelDocViewStyle | 16 |
| AcGeCircArc2d | AcGeCurve2d | 16 |
| PyDbLayerStateManager | PyRxObject | 15 |
| PyDbSectionViewStyle | PyDbModelDocViewStyle | 15 |
| AcGiPixelBGRA32 | - | 15 |
| AcGiImageBGRA32 | - | 15 |
| Document | - | 14 |
| PyDbLayout | PyDbObject | 14 |
| AcGePointOnSurface | - | 14 |
| PyDb2LineAngularDimension | PyDbDimension | 13 |
| PyDbAnnotationScale | - | 13 |
| AcGeEntity2d | - | 13 |
| AcGeEntity3d | - | 13 |
| AcGeSphere | AcGeSurface | 13 |
| DocumentManager | - | 12 |
| SubentId | - | 12 |
| FullSubentPath | - | 12 |
| PyDbHyperlink | - | 12 |
| PyDbIdMapping | - | 12 |
| PyDbAngularDimension | PyDbDimension | 11 |
| DatabaseReactor | - | 11 |
| PyDbLongTransaction | PyDbObject | 11 |
| PyDbTransactionManager | - | 11 |
| AcGeCone | AcGeSurface | 11 |
| AcGeSurface | AcGeEntity3d | 11 |
| GiPythonDrawable | - | 11 |
| PyDbDbLayerStateManager | - | 10 |
| PyDbRegion | PyDbEntity | 10 |
| AcGeCylinder | AcGeSurface | 10 |
| AcGiFill | - | 10 |
| AcGiHatchPattern | PyGiFill | 10 |
| AcGiGradientFill | PyGiFill | 10 |
| Color | - | 9 |
| PyDb3dProfile | - | 9 |
| PyDbHyperlinkCollection | - | 9 |
| PyDbMInsertBlock | PyDbBlockReference | 9 |
| PyDbModelDocViewStyle | PyDbObject | 9 |
| PyDbSpatialFilter | PyDbFilter | 9 |
| PyDbTrans | - | 9 |
| AcGeLineSeg2d | AcGeLinearEnt2d | 9 |
| AcGeLineSeg3d | AcGeLinearEnt3d | 9 |
| AcGePlanarEnt | AcGeSurface | 9 |
| AcGePointOnCurve2d | - | 9 |
| AcGePointOnCurve3d | - | 9 |
| AcGiDrawStream | - | 9 |
| AcGiVisualStyle | - | 9 |
| AcGiVisualStyleTraits | - | 9 |
| Palette | - | 9 |
| PaletteSet | - | 9 |
| DynBlockReference | - | 8 |
| DynBlockReferenceProperty | - | 8 |
| PyDbEvalVariant | - | 8 |
| PyDbGeoData | PyDbObject | 8 |
| PyDbUcsTableRecord | PyDbSymbolTableRecord | 8 |
| PyDbXrecord | PyDbObject | 8 |
| AcGeOffsetCurve3d | AcGeCurve3d | 8 |
| PyObjectIdGraph | - | 8 |
| PyDbRay | PyDbCurve | 7 |
| PyDbRevolveOptions | - | 7 |
| PyDbRevolvedSurface | PyDbSurface | 7 |
| PyDbTrace | PyDbEntity | 7 |
| PyDbVisualStyle | PyDbObject | 7 |
| PyDbXline | PyDbCurve | 7 |
| AcGeLinearEnt2d | AcGeCurve2d | 7 |
| AcGeLinearEnt3d | AcGeCurve3d | 7 |
| PyDbBlendOptions | - | 6 |
| PyDbDgnDefinition | PyDbObject | 6 |
| PyDbFace | PyDbEntity | 6 |
| PyDbFaceRecord | PyDbVertex | 6 |
| AcGeCompositeCurve2d | AcGeCurve2d | 6 |
| AcGeCompositeCurve3d | AcGeCurve3d | 6 |
| AcGeOffsetCurve2d | AcGeCurve2d | 6 |
| AcGePlane | AcGePlanarEnt | 6 |
| PyDbBlockReferenceIdIterator | - | 5 |
| PyDbBody | PyDbEntity | 5 |
| PyDbCompositeFilteredBlockIterator | - | 5 |
| PyDbExtrudedSurface | PyDbSurface | 5 |
| PyDbFilteredBlockIterator | - | 5 |
| PyDbLayerFilter | PyDbFilter | 5 |
| PyDbLayerTable | PyDbSymbolTable | 5 |
| PyDbLoftProfile | PyDb3dProfile | 5 |
| AcGeRay2d | AcGeLinearEnt2d | 5 |
| AcGeRay3d | AcGeLinearEnt3d | 5 |
| PyDb3dPolylineVertex | PyDbVertex | 4 |
| PyDbBlockChangeIterator | - | 4 |
| DynBlockTableRecord | - | 4 |
| PyDbEvalContextIterator | - | 4 |
| PyDbIdMappingIter | - | 4 |
| PyDbIndexUpdateDataIterator | - | 4 |
| PyDbLayerIndexIterator | - | 4 |
| PyDbLongTransWorkSetIterator | - | 4 |
| PyDbObjectContext | PyRxObject | 4 |
| PyDbPolygonMeshVertex | PyDbVertex | 4 |
| PyDbSpatialIndexIterator | - | 4 |
| PyDbTextEditorParagraphIterator | - | 4 |
| PyDbWblockCloneFiler | PyDbDeepCloneFiler | 4 |
| AcGePolyline2d | AcGeSplineEnt2d | 4 |
| AcGePolyline3d | AcGeSplineEnt3d | 4 |
| AcGePosition2d | - | 4 |
| AcGePosition3d | - | 4 |
| PyApp | - | 3 |
| StatusBar | - | 3 |
| PyDbColor | PyDbObject | 3 |
| PyDbEvalExpr | PyDbObject | 3 |
| PyDbObjectContextManager | - | 3 |
| PyDbPolyFaceMesh | PyDbEntity | 3 |
| PyDbPolyFaceMeshVertex | PyDbVertex | 3 |
| PyDbRegAppTable | PyDbSymbolTable | 3 |
| PyDbViewportTable | PyDbSymbolTable | 3 |
| AcGeLine2d | AcGeLinearEnt2d | 3 |
| AcGeLine3d | AcGeLinearEnt3d | 3 |
| PyDbBlockTable | PyDbSymbolTable | 2 |
| _PyDbBlockTableRecordEntityIter | - | 2 |
| PyDbDictionaryWithDefault | PyDbDictionary | 2 |
| PyDbDimStyleTable | PyDbSymbolTable | 2 |
| PyDbLinetypeTable | PyDbSymbolTable | 2 |
| PyDbLoftedSurface | PyDbSurface | 2 |
| PyDbOleFrame | PyDbEntity | 2 |
| PyDbPlaneSurface | PyDbSurface | 2 |
| PyDbSweptSurface | PyDbSurface | 2 |
| PyDbTextStyleTable | PyDbSymbolTable | 2 |
| PyDbUCSTable | PyDbSymbolTable | 2 |
| PyDbViewTable | PyDbSymbolTable | 2 |
| AcGeTol | - | 2 |
| PyAcadDatabase | - | 1 |
| PyDbDgnReference | PyDbEntity | 1 |
| PyDbDwfDefinition | PyDbObject | 1 |
| PyDbFilter | PyDbObject | 1 |
| PyDbBlockBegin | PyDbEntity | 0 |
| PyDbBlockEnd | PyDbEntity | 0 |
| PyDbDwfReference | PyDbEntity | 0 |
| PyDbImage | PyDbEntity | 0 |
| PyDbPdfDefinition | PyDbObject | 0 |
| PyDbPdfReference | PyDbEntity | 0 |
| PyDbPlaceHolder | PyDbObject | 0 |
| PyDbRegAppTableRecord | PyDbSymbolTableRecord | 0 |
| PyDbSequenceEnd | PyDbEntity | 0 |
| PyDbSurface | PyDbEntity | 0 |
| PyDbVertex | PyDbEntity | 0 |