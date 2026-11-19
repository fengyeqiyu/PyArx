# PyArxCore 防呆审计自动修复日志

2026-11-18 00:20 | PyArxCore.vcxproj:542 | ⑥登记缺失头文件 | 追加 ClInclude PyDbOpenCache.h（此前新增未登记）
2026-11-18 01:02 | PyDbDatabase.cpp:2513 | ⑤重复注册 | 删除 7 个与下方 impObj 直连同签名重复的 sysvar 绑定（angdir/chamferc/chamferd/cmljust/cmlscale/dimfit/dimunit），保留 celweight/dimassoc
2026-11-18 01:03 | PyDbLayerTableRecord.cpp:14 ③④ void setIsOff 补 PyWarnIfError 判 es
2026-11-18 01:03 | PyDbLayerTableRecord.cpp:16 ③④ void setVPDFLT 补 PyWarnIfError
2026-11-18 01:03 | PyDbLayerTableRecord.cpp:18 ③④ void setIsLocked 补 PyWarnIfError
2026-11-18 01:03 | PyDbLayerTableRecord.cpp:110 ③④ setColor 补 PyWarnIfError
2026-11-18 01:03 | 回退 setIsOff/setVPDFLT/setIsLocked：ARX 返回 void 无 es，原写法正确；仅保留 setColor 补判 es
2026-11-18 01:04 | 回退 setColor：ARX 亦返回 void 无 es；PyDbLayerTableRecord 全部 setter 为 void，无 es 丢弃问题，文件恢复原状
2026-11-18 01:05 | PyArxCore.vcxproj:543 | ⑥ | 补登记 HotModName.h/HotModuleName.h 到 ClInclude（stdafx 与 PyArxBinder 实际引用）
2026-11-18 01:07 | PyDbBlockReference.cpp:163 | ④⑤ | hasAttributes 不再恒返 false 占位，改按 attributeCount()>0 实现，接收实例
2026-11-18 01:19 | PyDbSymbolTableRecord.cpp:9-11 | ② | isDependent/isResolved/isRenamable 补 impObj 判空（三元兜底 false）
2026-11-18 01:19 | PyDbBody.cpp:7-10 | ② | isNull/numChanges/invalidateCachedBodySubentIds/usesGraphicsCache 补判空
2026-11-18 01:19 | PyDbDgnReference.cpp:12 | ② | setXrefDepth 补 impObj 判空，空返回 false
2026-11-18 01:19 | PyDbDgnDefinition.cpp:15,24,33 | ② | setUseMasterUnits/setXrefDepth/setShowRasterRef 补判空
2026-11-18 01:19 | PyDbAttribute.cpp:全文件 | ② | 21 处 impObj()-> 无判空，超 20 行上限，只报不改
2026-11-18 01:19 | PyDbTable.cpp:全文件 | ② | 47 处无判空，只报不改
2026-11-18 01:19 | PyDbHyperlink.cpp:16,25,34,55 | ③ | setter 后直接 return true，ARX 头缺失无法确认返回类型，只报不改
2026-11-18 02:18 | PyDbColor.cpp:16,27,35 | ② | entityColor/getColorRGB/setRGB 三个 lambda 补 impObj 判空
2026-11-18 02:18 | PyDb3dPolylineVertex.cpp:7,9 | ② | position/vertexType 补判空（返回默认值）
2026-11-18 02:18 | PyDbPolyFaceMeshVertex.cpp:7 | ② | position 补判空
2026-11-18 02:18 | PyDbPoint.cpp:7,11,15,17 | ② | position/thickness/normal/ecsRotation 四个 getter 补判空
2026-11-18 03:18 | PyDbRay.cpp:7-16 | ② | 3 getter 三元兜底、3 setter 补 impObj 判空
2026-11-18 03:18 | PyDbXline.cpp:7-16 | ② | 3 getter 三元兜底、3 setter 补 impObj 判空
2026-11-18 04:17 | PyDbTrace.cpp:7-16 | ② | 3 getter 三元兜底、3 setter 补 impObj 判空
2026-11-18 04:17 | PyDbSolid.cpp:7-16 | ② | 3 getter 三元兜底、3 setter 补 impObj 判空
2026-11-18 05:17 | PyDbBlendOptions.cpp:16-29 | ② | 3 getter 判空兜底、3 setter 前置判空
2026-11-18 06:18 | PyDbRegion.cpp:7-33 | ② | 9 处补 impObj 判空（isNull 空指针视为 true）
2026-11-18 06:18 | PyDb3dProfile.cpp:10-36 | ② | 构造/set/copyFrom/convertProfile 共 4 处补判空
2026-11-18 06:18 | PyDbSpatialFilter.cpp:13-46 | ② | 4 处 lambda 补 impObj/ext 判空
2026-11-18 07:20 | PyDbXrecord.cpp:8-45 | ② | 7 处 impObj 判空，rbChain/setFromRbChain/appendRbChain/mergeStyle 等空对象安全返回
2026-11-18 07:20 | PyDbVisualStyle.cpp:7-29 | ② | type 兜底 0，6 个 setter 前置判空
2026-11-18 07:20 | PyDbUcsTableRecord.cpp:7-19 | ② | 6 getter/setter 判空，ucsBaseOrigin 与 setUcsBaseOrigin 前置 return
2026-11-18 07:20 | PyDbGeoData.cpp:7-31 | ② | 8 处 impObj 判空，postToDb 空对象返回 (false,0)
2026-11-18 08:18 | PyDbMInsertBlock.cpp:7-21 | ② | 8 处判空，getter 兜底 0/0.0，setter 前置 return false
2026-11-18 08:18 | PyDbModelDocViewStyle.cpp:8-50 | ② | 9 处判空含 db 入参，getDescription 空返回默认 AcString
2026-11-18 08:18 | PyDbRevolvedSurface.cpp:7-33 | ② | 7 处判空，点/向量 getter 兜底默认几何
2026-11-18 08:18 | PyDbRevolveOptions.cpp:17-34 | ② | 7 处判空，checkRevolveCurve 空实体报错返回全 false
2026-11-18 08:18 | PyRxClass.cpp:11-55 | ② | 9 处判空，空对象返回默认 PyRxObject/PyRxClass
2026-11-18 09:18 | PyDbShape.cpp:7-35 | ② | 22 处判空，几何/数值 getter 兜底默认
2026-11-18 09:18 | PyDbFcf.cpp:8-34 | ② | 22 处判空，含 getDimstyleData 空返 nullptr
2026-11-18 09:18 | PyDb2dVertex.cpp:7-21 | ② | 15 处判空，getter 兜底 0/0.0
2026-11-18 09:18 | PyDbArc.cpp:7-28 | ② | 14 处判空，几何/数值 getter 兜底默认
2026-11-18 10:18 | PyDbSection.cpp:8-23 | ② | 16 处判空，几何/数值 getter 兜底默认
2026-11-18 10:18 | PyDbHelix.cpp:7-39 | ② | 23 处判空，getter 兜底 0/0.0，setter/as 板前置
2026-11-18 10:18 | PyDbEllipse.cpp:7-52 | ② | 22 处判空，isNull 空视为 true，get 空返默认元组
2026-11-18 11:19 | PyDbViewport.cpp:7-122 | ② | 84 处 getter/setter 补 impObj() 判空（getter 返回默认值，setter 直接 return false），行为不变
2026-11-18 11:19 | PyDbViewport.cpp:216-237 | ② | 21 处 PyRx 对齐 lambda 直接解 o.impObj()，需改写表达式，本轮只报不改
2026-11-18 11:33 | PyDbSpatialFilter.cpp:14 | ② | 去掉对按值返回 impObj() 的 PyDbExtents3d 误判空（!ext.impObj() 无法编译），仅保留 s.impObj() 判空
2026-11-18 11:33 | PyDbGeoData.cpp:38-53 | ⑤ | 删除历史判空脚本遗留的 5 个重复函数体（C2084 重复定义），保留已判空版本

2026-11-18 12:29 | PyDbHatch.cpp:1-209 | ② | 45 处 getter/setter 补 impObj() 判空（43 处批量 + float shadeTintValue/gradientShift 手改），行为不变；115-194 的 16 处 PyRx self.impObj() lambda 只报不改
2026-11-18 12:29 | PyDbMLeaderStyle.cpp:1-329 | ② | 63 处补 impObj() 判空（18 处单行 + 43 处多行函数体 + PyColor textColor/blockColor 与 postMLeaderStyleToDb 手改），行为不变

2026-11-18 13:18 | PyDb2dPolyline.cpp:1-118 | ② | 34 处 getter/setter 补 impObj() 判空（28 单行 + 5 多行体 + numVerts 手改），uint64_t 返回改为 0ULL
2026-11-18 14:20 | PyDb2LineAngularDimension.cpp:1-50 | ② | 12 处 getter/setter 补 impObj() 判空
2026-11-18 14:20 | PyDb3PointAngularDimension.cpp:1-40 | ② | 10 处 getter/setter 补 impObj() 判空
2026-11-18 14:20 | PyDb3dPolyline.cpp:1-69 | ② | 15 处补判空（10 单行 + 4 多行体 + numVerts 手改），uint64_t 返回 0ULL
2026-11-18 14:20 | PyDb3dSolid.cpp:1-70 | ② | 21 处补判空（16 单行 + 3 多行体 + numChanges 手改 + extrude lambda 内 self.impObj()
2026-11-18 15:19 | PyDbDwfDefinition.cpp:1-16 | ② | isDWFx lambda 补 s.impObj() 判空，空返 false
2026-11-18 15:19 | PyDbFilter.cpp:1-20 | ② | indexClass lambda 补 s.impObj() 判空，空返 PyRxClass(nullptr,false)
2026-11-18 15:19 | PyDbDimStyleTableIterator.cpp:1-17 | ② | 构造函数补 owner.impObj() 判空，空则提前返回
2026-11-18 15:19 | PyDbLayerTableIterator.cpp:1-17 | ② | 构造函数补 owner.impObj() 判空，空则提前返回
2026-11-18 15:19 | PyDbLinetypeTableIterator.cpp:1-17 | ② | 构造函数补 owner.impObj() 判空，空则提前返回
2026-11-18 15:19 | PyDbRegAppTableIterator.cpp:1-17 | ② | 构造函数补 owner.impObj() 判空，空则提前返回
2026-11-18 15:19 | PyDbTextStyleTableIterator.cpp:1-17 | ② | 构造函数补 owner.impObj() 判空，空则提前返回
2026-11-18 15:19 | PyDbBlockTableIterator.cpp:1-17 | ② | 构造函数补 owner.impObj() 判空，空则提前返回
2026-11-18 16:19 | PyDbBlockTable.cpp:1-15 | ② | has/getAt 补 impObj 判空，空返 false / PyDbBlockTableRecord(nullptr)
2026-11-18 16:19 | PyDbDictionary.cpp:1-40 | ② | has/getAt/getIdAt 补 impObj 判空，空返 false / PyDbObject(nullptr) / 0ULL
2026-11-18 16:19 | PyDbDimStyleTable.cpp:1-15 | ② | has/getAt 补 impObj 判空
2026-11-18 16:19 | PyDbLayerTable.cpp:1-15 | ② | has/getAt 补 impObj 判空，getAt 空返 PyDbLayerTableRecord(nullptr)
2026-11-18 16:19 | PyDbLinetypeTable.cpp:1-15 | ② | has/getAt 补 impObj 判空
2026-11-18 16:19 | PyDbRegAppTable.cpp:1-15 | ② | has/getAt 补 impObj 判空
2026-11-18 16:19 | PyDbSymbolTable.cpp:1-40 | ② | has/getAt 补 impObj 判空
2026-11-18 16:19 | PyDbTextStyleTable.cpp:1-15 | ② | has/getAt 补 impObj 判空
2026-11-18 16:19 | PyDbUCSTable.cpp:1-15 | ② | has/getAt 补 impObj 判空
2026-11-18 16:19 | PyDbViewportTable.cpp:1-15 | ② | has/getAt 补 impObj 判空
2026-11-18 16:19 | PyDbViewTable.cpp:1-15 | ② | has/getAt 补 impObj 判空
2026-11-18 16:19 | PyDbSurface.cpp:8-14 | ② | createFrom 补 srcEntity.impObj() 判空，空返 std::nullopt
2026-11-18 17:21 | PyDbDynBlockTableRecord.cpp:21 | ② | isDynamicBlock lambda 补 if(!impObj()) return false
2026-11-18 17:21 | PyDbDynBlockTableRecord.cpp:25 | ② | blockTableRecordId lambda 补判空返回 0ULL
2026-11-18 17:21 | PyDbDynBlockTableRecord.cpp:31 | ② | getAnonymousBlockIds lambda 补判空返回空列表
2026-11-18 17:21 | PyDbDynBlockTableRecord.cpp:44 | ② | updateAnonymousBlocks lambda 补判空返回 false
2026-11-18 18:19 | PyDbExtrudedSurface.cpp:8-24 | ② | 5 个成员方法补 if(!impObj()) 返回 0.0/AcGeVector3d()/false
2026-11-18 18:19 | PyDbLayerFilter.cpp:14-33 | ② | add/remove/getAt/isValid/layerCount 五个 lambda 补判空
2026-11-18 18:19 | PyDbFaceRecord.cpp:9-28 | ② | 5 个成员方法补 if(!impObj()) 返回 0/false
2026-11-18 19:18 | PyDbLoftProfile.cpp:21-45 | ② | copyFrom/continuity/setContinuity/magnitude/setMagnitude 五个 lambda 补判空（src 同时判空）返回 false/0/0.0
2026-11-18 19:18 | PyDbCircle.cpp:7-24 | ② | 12 个单行 getter/setter 内联补 if(!impObj()) 返回原类型默认值
2026-11-18 19:41 | PyDbDiametricDimension.cpp:7-22 | ② | 10 个单行 getter/setter 内联补判空，返回 AcGePoint3d()/0.0/false
2026-11-18 19:41 | PyDbRadialDimension.cpp:7-22 | ② | 10 个单行 getter/setter 内联补判空，返回 AcGePoint3d()/0.0/false
2026-11-18 19:42 | PyDbLine.cpp:7-33 | ② | 11 个单行成员方法内联补判空（void setColorIndex 用 return;），getOffsetCurvesGivenPlaneNormal 条件前置判空
2026-11-18 19:44 | PyDbWipeout.cpp:7-21 | ② | 10 个单行成员方法内联补判空 + append 前置判空返回 false
2026-11-18 19:45 | PyDbLayout.cpp:8-69 | ② | 13 个成员函数体首行补判空，返回 0/false/AcString()/py::list()/py::tuple()/return;
2026-11-18 19:46 | PyDbOrdinateDimension.cpp:7-24 | ② | 12 个单行方法内联补判空，返回 AcGePoint3d()/false
2026-11-18 19:46 | PyDbAlignedDimension.cpp:7-24 | ② | 12 个单行方法内联补判空，返回 AcGePoint3d()/0.0/false
2026-11-18 20:00 | 74 文件 | ② | 全库批量补 if(!impObj()) 判空净新增 1672 处，逐文件 check_one 编译 PASS
2026-11-18 20:23 | 31 文件 | ② | 补齐此前跳过的返回类型默认值，新增 132 处守卫，逐文件编译 PASS
2026-11-18 22:18 | PyDbDatabaseReactor.cpp:20-148 | ⑧ | 9 处 Python 回调 catch 与 9 处 catch(...) 补 acutPrintf 异常提示
2026-11-19 00:18 | PyClipBoard.cpp:51-93 | ④ | getText/setText 6 处失败分支补 acutPrintf 原因提示
2026-11-19 01:18 | PyDbField.cpp:16-139 | ② | 21 个成员函数补 if (!impObj()) 守卫并给默认返回
2026-11-19 02:20 | PyDbFullSubentPath.cpp:65-115 | ② | PyDbSubentId 6 处裸解引用 m_ptr 补判空并给默认返回
2026-11-19 03:18 | PyDbLongTransaction.cpp:12-60 | ② | 8 个绑定 lambda 内裸调用 s.impObj() 补判空并给默认返回
2026-11-19 05:19 | PyDbPlotSettingsValidator.cpp:30-167 | ② | 23 个绑定 lambda 补 self/ps 判空并给默认返回
2026-11-19 07:18 | PyDbDimStyleTableRecord.cpp:12-137 | ② | 73 个绑定 lambda 首行补 s.impObj() 判空并返回同型默认值
2026-11-19 08:54 | PyEdEnt.cpp:tail | ⑤ | 接入 TextFinder：新增 Ed.textFind(ids,keyword,mode)，顺手给 QTextFind 构造补 openStatus/空指针判断
2026-11-19 08:56 | PyEdEnt.cpp:tail | ⑤ | Ed.textFind 改用 SDK acdbTextFind（支持 replace/AC_SRCH_* 位标志/sel 限定），并导出 8 个 AC_SRCH_* 常量
2026-11-19 09:00 | PyDbBinder.cpp:258 | ⑤ | acdbTextFind 迁入 Db.textFind（lambda 内联，含 AC_SRCH_* 常量），并在 PyEd/PyDb 两侧删除/清理
2026-11-19 09:00 | TextFinder.h:1 | ⑤ | 删除自研 QTextFind 及 vcxproj ClInclude 登记
2026-11-19 09:05 | PyDbBinder.cpp:262 | ⑤ | AC_SRCH_* 8 个裸常量收为 Db.TextFindFlags 强类型枚举（支持 | 组合）
2026-11-19 09:06 | PyDbBinder.cpp:38 | ⑤ | 枚举成员统一 k 前缀（kBLOCK/kDIM/kTEXT/…）
2026-11-19 09:18 | PyEdEditor.cpp:117 | ③ | acedCoordFromPixelToWorld 无窗号分支补返回值判断，失败返回 None
2026-11-19 09:19 | PyColor.cpp:34 | ④ | setColorIndex 对 256 取模归一（负值亦归一），fromACI 改走 setColorIndex，文案改为 0=ByBlock
2026-11-19 09:24 | PyColor.cpp:34 | ④ | setColorIndex 分流：256 走 setByLayer、257 走 setNone、其余对 256 取模；文案更新
2026-11-19 10:04 | PyColor.cpp:34 | ② | red/green/blue 对 ACI 色改用 acedGetRGB 解析真实 RGB（1-255），真彩色仍走分量；绑定文案更新
2026-11-19 10:07 | PyColor.cpp:54 | 防呆 | fromRGB/setRGB 分量夹紧 0-255，消除 UInt8 静默回绕
2026-11-19 10:07 | PyColor.cpp:86 | ④ | describe() 空描述兜底为 ByLayer/ByBlock/None/RGB/ACI
2026-11-19 10:07 | PyColor.cpp:142 | 易用 | 补 Push none() 工厂与 __eq__/__ne__
2026-11-19 10:10 | PyColor.cpp:55 | ③/④ | setRGB/setColorIndex 改返回 bool，走 PyWarnIfError 判 es
2026-11-19 10:20 | PyDbEntity.cpp:250 | ⑤ | 删除重复的 setRGB lambda，统一走 setColorByRgb(r,g,b,doSubents)
2026-11-19 10:20 | PyDbEntity.cpp:85 | 命名 | setColor(PyColor) 改为 setColorByRgb(r,g,b)，颜色设置入口收敛为 setColorIndex + setColorByRgb
2026-11-19 10:20 | PyDbLayerTableRecord.cpp:120 | 命名/③ | setColor(r,g,b) 改为 setColorByRgb，补 clamp 并补回 bool
2026-11-19 10:20 | PyDbColor.cpp:33 | 命名 | setRGB 改为 setColorByRgb，补 clamp 与 bool 返回
2026-11-19 10:24 | PyDbHighlightOverrule.cpp:14 | ⑧/④ | catch(...) 与 error_already_set 分支改为 restore+PyErr_Print，异常不再静默
2026-11-19 10:24 | PyDbGripOverrule.cpp:52 | ⑧/④ | 6 处回调 catch 补 PyErr_Print，reg_* 降级仍保留
2026-11-19 10:24 | PyDbObjectReactor.cpp:19 | ⑧/④ | 12 处回调 catch 补 PyErr_Print，异常不再静默
2026-11-19 11:19 | PyDbGeometryOverrule.cpp:16,53,90,109 | ⑧ | 静默 catch 改为 restore+PyErr_Print，回退 SDK 默认实现不变
2026-11-19 11:19 | PyDbHighlightStateOverrule.cpp:14 | ⑧ | 同上，compat 回调异常可见化
2026-11-19 11:19 | PyDbObjectOverrule.cpp:22,40,58,76,93 | ⑧ | 同上，5 处 open/close/cancel/erase/isApplicable 异常改为可见
2026-11-19 14:28 | PyGeCurveCurveInt2d.cpp:22 | ⑦临时残留 | 删除已失效的 deferred NOTE 注释（相关绑定实际已实现）
2026-11-19 15:18 | PyDbBinder.cpp:520 | ⑥/防呆 | 补登记 9 个符号表 (AcDb*Table) 到 g_classMap，openObject 不再回落 PyDbObject，Python 可拿 table.add()/getAt()
2026-11-19 15:18 | PyDbTrans.cpp:298 | ④/缺口 | 新增 addNewlyCreatedDBObject(obj, add) 绑定，内部走 AcDbTransactionManager::addNewlyCreatedDBRObject 并判 es
2026-11-19 15:18 | PyMfcXml/PyMfcBind.cpp:90 | ⑨ | PYBIND11_MODULE 补第三参 py::mod_gil_not_used()，与 PyArxCore 一致
2026-11-19 15:18 | tools/cadmcp/headless_entity_test.py:130/137/153 | 脚本 | addEntry→add、getRecord→getAt、图层改用 newRecord()
2026-11-19 15:26 | PyDbObject.cpp:256 | ② | hasFields 补 isNull 守卫，空包装返回 false
2026-11-19 15:26 | PyDbPolygonMesh.cpp:45/52 | ② | isMClosed/isNClosed 补 isNull 守卫
2026-11-19 15:26 | PyDbLinetypeTableRecord.cpp:29/34/74 | ② | patternLength/numDashes/isScaledToFit 补 isNull 守卫(分别返 0.0/0/false)
