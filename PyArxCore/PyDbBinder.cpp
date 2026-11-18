#include "stdafx.h"
#include "PyRxObject.h"
#include "PyRxClass.h"
#include "PyDbObject.h"
#include "PyDbObjectReactor.h"
#include "PyDbObjectOverrule.h"
#include "PyDbGeometryOverrule.h"
#include "PyDbPropertiesOverrule.h"
#include "PyDbOsnapOverrule.h"
#include "PyDbTransformOverrule.h"
#include "PyDbGripOverrule.h"
#include "PyDbSubentityOverrule.h"
#include "PyDbHighlightOverrule.h"
#include "PyDbHighlightStateOverrule.h"
#include "PyDbVisibilityOverrule.h"
#include "PyDbDimensionStyleOverrule.h"
#include "PyDbAuditInfo.h"
#include "PyDbBackground.h"
#include "PyDbDataTable.h"
#include "PyDbDataCell.h"
#include "PyDbDataColumn.h"
#include "PyDbDataLink.h"
#include "PyDbDataLinkManager.h"
#include "PyDbDwgFiler.h"
#include "PyDbDatabase.h"
#include "PyDbTrans.h"
#include "PyDxfTypeValue.h"
#include "PyResbuf.h"
#include "PyAdsName.h"
#include "PyColor.h"
#include "PyDbMLeaderStyle.h"

// Forward declarations of per-class bind functions (one TU per class).
void bind_PyDbEntity(pym& m);
void bind_PyDbCurve(pym& m);
void bind_PyDbLine(pym& m);
void bind_PyDbDimension(pym& m);
void bind_PyDbCircle(pym& m);
void bind_PyDbArc(pym& m);
void bind_PyDbPolyline(pym& m);
void bind_PyDb2dPolyline(pym& m);
void bind_PyDb3dPolyline(pym& m);
void bind_PyDbSpline(pym& m);
void bind_PyDbEllipse(pym& m);
void bind_PyDbHelix(pym& m);
void bind_PyDbText(pym& m);
void bind_PyDbMText(pym& m);
void bind_PyDbAttribute(pym& m);
void bind_PyDbAttributeDefinition(pym& m);
void bind_PyDbAlignedDimension(pym& m);
void bind_PyDbRotatedDimension(pym& m);
void bind_PyDbRadialDimension(pym& m);
void bind_PyDbDiametricDimension(pym& m);
void bind_PyDb3PointAngularDimension(pym& m);
void bind_PyDb2LineAngularDimension(pym& m);
void bind_PyDbOrdinateDimension(pym& m);
void bind_PyDbArcDimension(pym& m);
void bind_PyDbRadialDimensionLarge(pym& m);
void bind_PyDbBlockReference(pym& m);
void bind_PyDbMInsertBlock(pym& m);
void bind_PyDbHatch(pym& m);
void bind_PyDbViewport(pym& m);
void bind_PyDbRasterImage(pym& m);
void bind_PyDbOle2Frame(pym& m);
void bind_PyDbMline(pym& m);
void bind_PyDbPoint(pym& m);
void bind_PyDbFace(pym& m);
void bind_PyDbSolid(pym& m);
void bind_PyDbTrace(pym& m);
void bind_PyDbShape(pym& m);
void bind_PyDbWipeout(pym& m);
void bind_PyDbLeader(pym& m);
void bind_PyDbMLeader(pym& m);
void bind_PyDbMLeaderStyle(pym& m);
void bind_PyDbFcf(pym& m);
void bind_PyDb3dSolid(pym& m);
void bind_PyDbSubentId(pym& m);
void bind_PyDbFullSubentPath(pym& m);
void bind_PyDbSubDMesh(pym& m);
void bind_PyDbBody(pym& m);
void bind_PyDbMPolygon(pym& m);
void bind_PyDbSection(pym& m);
void bind_PyDbVertex(pym& m);
void bind_PyDb2dVertex(pym& m);
void bind_PyDb3dPolylineVertex(pym& m);
void bind_PyDbPolygonMeshVertex(pym& m);
void bind_PyDbPolyFaceMeshVertex(pym& m);
void bind_PyDbFaceRecord(pym& m);
void bind_PyDbSequenceEnd(pym& m);
void bind_PyDbBlockBegin(pym& m);
void bind_PyDbBlockEnd(pym& m);
void bind_PyDbPolygonMesh(pym& m);
void bind_PyDbPolyFaceMesh(pym& m);
void bind_PyDbSurface(pym& m);
void bind_PyDbPlaneSurface(pym& m);
void bind_PyDbExtrudedSurface(pym& m);
void bind_PyDbRevolvedSurface(pym& m);
void bind_PyDbSweptSurface(pym& m);
void bind_PyDbLoftedSurface(pym& m);
void bind_PyDbNurbSurface(pym& m);
void bind_PyDbMaterial(pym& m);
void bind_PyDbVisualStyle(pym& m);
void bind_PyDbModelDocViewStyle(pym& m);
void bind_PyDbDetailViewStyle(pym& m);
void bind_PyDbSectionViewStyle(pym& m);
void bind_PyDbDimAssoc(pym& m);
void bind_PyDbGeoData(pym& m);
void bind_PyDbColor(pym& m);
void bind_PyColor(pym& m);   // AcCmColor 值类（非 DB 对象）
void bind_PyDbPlaceHolder(pym& m);
void bind_PyDbOleFrame(pym& m);
void bind_PyDbFilter(pym& m);
void bind_PyDbSpatialFilter(pym& m);
void bind_PyDbLayerFilter(pym& m);
void bind_PyDbDictionaryWithDefault(pym& m);
void bind_PyDbDwfDefinition(pym& m);
void bind_PyDbDgnDefinition(pym& m);
void bind_PyDbPdfDefinition(pym& m);
void bind_PyDbDwfReference(pym& m);
void bind_PyDbDgnReference(pym& m);
void bind_PyDbPdfReference(pym& m);
void bind_PyDbLongTransaction(pym& m);
void bind_PyDbObjectContext(pym& m);
void bind_PyDbImage(pym& m);
void bind_PyDbEvalVariant(pym& m);
void bind_PyDbEvalExpr(pym& m);
void bind_PyDbDynBlockReferenceProperty(pym& m);
void bind_PyDbDynBlockReference(pym& m);
void bind_PyDbDynBlockTableRecord(pym& m);
void bind_PyDbDeepCloneFiler(pym& m);
void bind_PyDbWblockCloneFiler(pym& m);
void bind_PyDb3dProfile(pym& m);
void bind_PyDbLoftProfile(pym& m);
void bind_PyDbLoftOptions(pym& m);
void bind_PyDbSweepOptions(pym& m);
void bind_PyDbBlendOptions(pym& m);
void bind_PyDbRevolveOptions(pym& m);
void bind_PyDbLayerStateManager(pym& m);
void bind_PyDbPlotSettingsValidator(pym& m);
void bind_PyDbBlockTableRecordIterator(pym& m);
void bind_PyDbFilteredBlockIterator(pym& m);
void bind_PyDbCompositeFilteredBlockIterator(pym& m);
void bind_PyDbLongTransWorkSetIterator(pym& m);
void bind_PyDbTextEditorParagraphIterator(pym& m);
void bind_PyDbIndexUpdateDataIterator(pym& m);
void bind_PyDbBlockChangeIterator(pym& m);
void bind_PyDbLayerIndexIterator(pym& m);
void bind_PyDbSpatialIndexIterator(pym& m);
void bind_PyDbIdMappingIter(pym& m);
void bind_PyDbEvalContextIterator(pym& m);

void bind_PyDbTable(pym& m);
void bind_PyDbRegion(pym& m);
void bind_PyDbRay(pym& m);
void bind_PyDbXline(pym& m);
void bind_PyDbSymbolTableRecord(pym& m);
void bind_PyDbBlockTableRecord(pym& m);
void bind_PyDbDimStyleTableRecord(pym& m);
void bind_PyDbLayerTableRecord(pym& m);
void bind_PyDbLinetypeTableRecord(pym& m);
void bind_PyDbTextStyleTableRecord(pym& m);
void bind_PyDbSymbolTable(pym& m);
void bind_PyDbBlockTable(pym& m);
void bind_PyDbLayerTable(pym& m);
void bind_PyDbTextStyleTable(pym& m);
void bind_PyDbLinetypeTable(pym& m);
void bind_PyDbViewTable(pym& m);
void bind_PyDbUCSTable(pym& m);
void bind_PyDbViewportTable(pym& m);
void bind_PyDbRegAppTable(pym& m);
void bind_PyDbDimStyleTable(pym& m);
void bind_PyDbDictionary(pym& m);
void bind_PyDbGroup(pym& m);
void bind_PyDbLayout(pym& m);
// P1 system table records
void bind_PyDbRegAppTableRecord(pym& m);
void bind_PyDbUcsTableRecord(pym& m);
void bind_PyDbViewTableRecord(pym& m);
void bind_PyDbViewportTableRecord(pym& m);
// P1 styles / objects
void bind_PyDbMlineStyle(pym& m);
void bind_PyDbTableStyle(pym& m);
void bind_PyDbPlotSettings(pym& m);
void bind_PyDbXrecord(pym& m);
void bind_PyDbDatabaseSummaryInfo(pym& m);
void bind_PyDbHyperlink(pym& m);
void bind_PyDbHyperlinkCollection(pym& m);
// Database split-out classes (2026-10-20 refactor)
void bind_PyDbDate(pym& m);
void bind_PyDbIdMapping(pym& m);
void bind_PyDbAnnotationScale(pym& m);
void bind_PyDbTransactionManager(pym& m);
void bind_PyDbObjectContextManager(pym& m);
void bind_PyDbDbLayerStateManager(pym& m);
void bind_PyDbDatabaseReactor(pym& m);
void bind_PyAcadDatabase(pym& m);
void bind_PyObjectIdGraph(pym& m);
// DXF 组码与 resbuf 链表（框架核心类型，先于其它类注册）
void bind_PyTypedValue(pym& m);
void bind_PyTvList(pym& m);
void bind_PyResbuf(pym& m);

void bind_PyRx(pym& m)
{
	auto RxObject = py::class_<PyRxObject>(m, "PyRxObject");
	auto RxClass = py::class_<PyRxClass, PyRxObject>(m,"PyRxClass");
	RxObject.def(py::init<>())
		// copyFrom-backed copy constructor
		.def(py::init<const PyRxObject&>(), py::arg("rxobj"))
		PyWrapper_BIND(RxObject)

		.def("clone", &PyRxObject::clone,
			"Deep copy of the object (virtual method)")
		.def("copyFrom", &PyRxObject::copyFrom,
			py::arg("other"), "Copy properties from another object")
		.def("queryX", &PyRxObject::queryX,
			py::arg("protocolClass"), "Query extended protocol interface")
		.def("comparedTo", &PyRxObject::comparedTo,
			py::arg("other"), "Compare ordering with another object")
			// Type checks are done natively in Python via isinstance/issubclass/type;
			// isKindOf/isDerivedFrom are intentionally not exposed.
			// queryX is the ARX protocol-extension interface query and stays.

		.def("__eq__", &PyRxObject::operator==, py::arg("other"))
		.def("__ne__", &PyRxObject::operator!=, py::arg("other"))
		.def("__hash__", &PyRxObject::hash)

		// Pointer state accessor
		.def("intPtr", &PyRxObject::intPtr,
			"Get integer representation of the underlying pointer")
		.def("isNull", &PyRxObject::isNull,
			"Check if the object wraps a nullptr");


	RxClass.def(py::init<const PyRxClass&>())
		PyWrapper_BIND(RxClass)
		// Type checks are native Python; isDerivedFrom intentionally not exposed here.
		.def("proxyFlags", &PyRxClass::proxyFlags)
		.def("myParent", &PyRxClass::myParent)

		// Static protocol extension table
		.def("addX", &PyRxClass::addX, py::arg("rxclass"), py::arg("rxobj"))
		.def("getX", &PyRxClass::getX)
		.def("delX", &PyRxClass::delX)
		.def("queryX", py::overload_cast<const PyRxClass&>(&PyRxClass::queryX))
		.def("queryX", py::overload_cast<const PyRxClass&>(&PyRxClass::queryX, py::const_));
}

void pybind11_init_PyDb(::pybind11::module_& m) {
	// Import sibling submodules first: PyDb methods return AcGe* / enum
	// types whose pybind type_info must already be registered.
	py::module_::import("PyArxCore.Ge");
	py::module_::import("PyArxCore.Enum");

	// DXF 组码 / resbuf 链表核心类型（其它类的方法可能返回它们）
	bind_PyTypedValue(m);
	bind_PyTvList(m);
	bind_PyResbuf(m);

	// PyRxObject/PyRxClass 已由 PyArx.Rx 子模块先行注册（PyArx 聚合顺序 Rx 先于 Db）。
	bind_PyDbObject(m);
	bind_PyDbObjectReactor(m);
	bind_PyDbObjectOverrule(m);
	bind_PyDbGeometryOverrule(m);
	bind_PyDbPropertiesOverrule(m);
	bind_PyDbOsnapOverrule(m);
	bind_PyDbTransformOverrule(m);
	bind_PyDbGripOverrule(m);
	bind_PyDbSubentityOverrule(m);
	bind_PyDbHighlightOverrule(m);
	bind_PyDbHighlightStateOverrule(m);
	bind_PyDbVisibilityOverrule(m);
	bind_PyDbDimensionStyleOverrule(m);
	bind_PyDbAuditInfo(m);
	bind_PyDbBackground(m);
	bind_PyDbDataTable(m);
	bind_PyDbDataCell(m);
	bind_PyDbDataColumn(m);
	bind_PyDbDataLink(m);
	bind_PyDbDataLinkManager(m);
	bind_PyDbDwgFiler(m);
	bind_PyDbDatabase(m);
	// Database split-out classes: register after PyDbDatabase so factory
	// methods returning these wrappers have their types available.
	bind_PyDbDate(m);
	bind_PyDbIdMapping(m);
	bind_PyDbAnnotationScale(m);
	bind_PyDbTransactionManager(m);
	bind_PyDbObjectContextManager(m);
	bind_PyDbDbLayerStateManager(m);
	bind_PyDbDatabaseReactor(m);
	bind_PyAcadDatabase(m);
	bind_PyObjectIdGraph(m);
	bind_PyDbEntity(m);
	bind_PyDbCurve(m);
	bind_PyDbLine(m);
	bind_PyDbTrans(m);

	// Record classes before tables (getAt returns these wrappers).
	bind_PyDbSymbolTableRecord(m);
	bind_PyDbBlockTableRecord(m);
	bind_PyDbDimStyleTableRecord(m);
	bind_PyDbLayerTableRecord(m);
	bind_PyDbLinetypeTableRecord(m);
	bind_PyDbTextStyleTableRecord(m);
	bind_PyDbRegAppTableRecord(m);
	bind_PyDbUcsTableRecord(m);
	bind_PyDbViewTableRecord(m);
	bind_PyDbViewportTableRecord(m);

	// Dimension abstract base, then curve/text/dimension/entity wrappers.
	bind_PyDbDimension(m);
	bind_PyDbCircle(m);
	bind_PyDbArc(m);
	bind_PyDbPolyline(m);
	bind_PyDb2dPolyline(m);
	bind_PyDb3dPolyline(m);
	bind_PyDbSpline(m);
	bind_PyDbEllipse(m);
	bind_PyDbHelix(m);
	bind_PyDbText(m);
	bind_PyDbMText(m);
	bind_PyDbAttribute(m);
	bind_PyDbAttributeDefinition(m);
	bind_PyDbAlignedDimension(m);
	bind_PyDbRotatedDimension(m);
	bind_PyDbRadialDimension(m);
	bind_PyDbDiametricDimension(m);
	bind_PyDb3PointAngularDimension(m);
	bind_PyDb2LineAngularDimension(m);
	bind_PyDbOrdinateDimension(m);
	bind_PyDbArcDimension(m);
	bind_PyDbRadialDimensionLarge(m);
	bind_PyDbBlockReference(m);
	bind_PyDbMInsertBlock(m);
	bind_PyDbHatch(m);
	bind_PyDbViewport(m);
	bind_PyDbRasterImage(m);
	bind_PyDbOle2Frame(m);
	bind_PyDbMline(m);
	bind_PyDbPoint(m);
	bind_PyDbFace(m);
	bind_PyDbSolid(m);
	bind_PyDbTrace(m);
	bind_PyDbShape(m);
	bind_PyDbWipeout(m);
	bind_PyDbLeader(m);
	bind_PyDbMLeader(m);
	bind_PyColor(m);   // AcCmColor 值类（须先于依赖它的类注册）
	bind_PyDbMLeaderStyle(m);
	bind_PyDbFcf(m);
	bind_PyDb3dSolid(m);
	bind_PyDbSubentId(m);
	bind_PyDbFullSubentPath(m);
	bind_PyDbSubDMesh(m);
	bind_PyDbBody(m);
	bind_PyDbMPolygon(m);
	bind_PyDbSection(m);
	bind_PyDbTable(m);

	// Region (static factory), Ray, Xline.
	bind_PyDbRegion(m);
	bind_PyDbRay(m);
	bind_PyDbXline(m);

	// Symbol table base, built-in tables, dictionary, group, layout.
	bind_PyDbSymbolTable(m);
	bind_PyDbBlockTable(m);
	bind_PyDbLayerTable(m);
	bind_PyDbTextStyleTable(m);
	bind_PyDbLinetypeTable(m);
	bind_PyDbViewTable(m);
	bind_PyDbUCSTable(m);
	bind_PyDbViewportTable(m);
	bind_PyDbRegAppTable(m);
	bind_PyDbDimStyleTable(m);
	bind_PyDbDictionary(m);
	bind_PyDbGroup(m);
	bind_PyDbLayout(m);

	// P1 styles / standalone objects (Hyperlink before Collection which returns it).
	bind_PyDbXrecord(m);
	bind_PyDbDatabaseSummaryInfo(m);
	bind_PyDbMlineStyle(m);
	bind_PyDbTableStyle(m);
	bind_PyDbPlotSettings(m);
	bind_PyDbHyperlink(m);
	bind_PyDbHyperlinkCollection(m);
	bind_PyDbVertex(m);
	bind_PyDb2dVertex(m);
	bind_PyDb3dPolylineVertex(m);
	bind_PyDbPolygonMeshVertex(m);
	bind_PyDbPolyFaceMeshVertex(m);
	bind_PyDbFaceRecord(m);
	bind_PyDbSequenceEnd(m);
	bind_PyDbBlockBegin(m);
	bind_PyDbBlockEnd(m);
	bind_PyDbPolygonMesh(m);
	bind_PyDbPolyFaceMesh(m);
	bind_PyDbSurface(m);
	bind_PyDbPlaneSurface(m);
	bind_PyDbExtrudedSurface(m);
	bind_PyDbRevolvedSurface(m);
	bind_PyDbSweptSurface(m);
	bind_PyDbLoftedSurface(m);
	bind_PyDbNurbSurface(m);
	bind_PyDbMaterial(m);
	bind_PyDbVisualStyle(m);
	bind_PyDbModelDocViewStyle(m);
	bind_PyDbDetailViewStyle(m);
	bind_PyDbSectionViewStyle(m);
	bind_PyDbDimAssoc(m);
	bind_PyDbGeoData(m);
	bind_PyDbColor(m);
	bind_PyDbPlaceHolder(m);
	bind_PyDbOleFrame(m);
	bind_PyDbFilter(m);
	bind_PyDbSpatialFilter(m);
	bind_PyDbLayerFilter(m);
	bind_PyDbDictionaryWithDefault(m);
	bind_PyDbDwfDefinition(m);
	bind_PyDbDgnDefinition(m);
	bind_PyDbPdfDefinition(m);
	bind_PyDbDwfReference(m);
	bind_PyDbDgnReference(m);
	bind_PyDbPdfReference(m);
	bind_PyDbLongTransaction(m);
	bind_PyDbObjectContext(m);
	bind_PyDbImage(m);
	bind_PyDbEvalVariant(m);
	bind_PyDbEvalExpr(m);
	bind_PyDbDynBlockReferenceProperty(m);
	bind_PyDbDynBlockReference(m);
	bind_PyDbDynBlockTableRecord(m);
	bind_PyDbDeepCloneFiler(m);
	bind_PyDbWblockCloneFiler(m);
	bind_PyDb3dProfile(m);
	bind_PyDbLoftProfile(m);
	bind_PyDbLoftOptions(m);
	bind_PyDbSweepOptions(m);
	bind_PyDbBlendOptions(m);
	bind_PyDbRevolveOptions(m);
	bind_PyDbLayerStateManager(m);
	bind_PyDbPlotSettingsValidator(m);
	bind_PyDbBlockTableRecordIterator(m);
	bind_PyDbFilteredBlockIterator(m);
	bind_PyDbCompositeFilteredBlockIterator(m);
	bind_PyDbLongTransWorkSetIterator(m);
	bind_PyDbTextEditorParagraphIterator(m);
	bind_PyDbIndexUpdateDataIterator(m);
	bind_PyDbBlockChangeIterator(m);
	bind_PyDbLayerIndexIterator(m);
	bind_PyDbSpatialIndexIterator(m);
	bind_PyDbIdMappingIter(m);
	bind_PyDbEvalContextIterator(m);

	// === 自动类型推导：注册常用类到全局类注册表 ===
	// openObject 会根据 AcRxClass* 自动查找并 cast 到对应 Python 类
	auto pyDbModule = m;
	registerPyClass(AcDbLine::desc(), pyDbModule.attr("PyDbLine"));
	registerPyClass(AcDbCircle::desc(), pyDbModule.attr("PyDbCircle"));
	registerPyClass(AcDbArc::desc(), pyDbModule.attr("PyDbArc"));
	registerPyClass(AcDbEllipse::desc(), pyDbModule.attr("PyDbEllipse"));
	registerPyClass(AcDbPolyline::desc(), pyDbModule.attr("PyDbPolyline"));
	registerPyClass(AcDbSpline::desc(), pyDbModule.attr("PyDbSpline"));
	registerPyClass(AcDbMText::desc(), pyDbModule.attr("PyDbMText"));
	registerPyClass(AcDbText::desc(), pyDbModule.attr("PyDbText"));
	registerPyClass(AcDbHatch::desc(), pyDbModule.attr("PyDbHatch"));
	registerPyClass(AcDbDimension::desc(), pyDbModule.attr("PyDbDimension"));
	registerPyClass(AcDbBlockReference::desc(), pyDbModule.attr("PyDbBlockReference"));
	registerPyClass(AcDbPoint::desc(), pyDbModule.attr("PyDbPoint"));
	registerPyClass(AcDbRay::desc(), pyDbModule.attr("PyDbRay"));
	registerPyClass(AcDbXline::desc(), pyDbModule.attr("PyDbXline"));
	registerPyClass(AcDbLeader::desc(), pyDbModule.attr("PyDbLeader"));
	registerPyClass(AcDbTable::desc(), pyDbModule.attr("PyDbTable"));
	registerPyClass(AcDbBlockTableRecord::desc(), pyDbModule.attr("PyDbBlockTableRecord"));
	registerPyClass(AcDbLayerTableRecord::desc(), pyDbModule.attr("PyDbLayerTableRecord"));
	registerPyClass(AcDbLinetypeTableRecord::desc(), pyDbModule.attr("PyDbLinetypeTableRecord"));
	registerPyClass(AcDbTextStyleTableRecord::desc(), pyDbModule.attr("PyDbTextStyleTableRecord"));
	registerPyClass(AcDbDimStyleTableRecord::desc(), pyDbModule.attr("PyDbDimStyleTableRecord"));
	registerPyClass(AcDbDictionary::desc(), pyDbModule.attr("PyDbDictionary"));
	registerPyClass(AcDbLayout::desc(), pyDbModule.attr("PyDbLayout"));

}

// ---------------------------------------------------------------------------
// PyTypedValue / PyTvList / PyResbuf 绑定（DXF 组码与 resbuf 链表）
// ---------------------------------------------------------------------------

void bind_PyTypedValue(pym& m) {
	// === 先绑定 DxfValue 类 ===
	py::class_<DxfValue>(m, "DxfValue", "DXF 组值（变体类型）")
		.def("getType", &DxfValue::getType, "获取值类型")
		.def("hasValue", &DxfValue::hasValue, "是否有值")
		.def("asInt8", &DxfValue::asInt8, "转为 int8")
		.def("asInt16", &DxfValue::asInt16, "转为 int16")
		.def("asInt32", &DxfValue::asInt32, "转为 int32")
		.def("asInt64", &DxfValue::asInt64, "转为 int64")
		.def("asDouble", &DxfValue::asDouble, "转为 double")
		.def("asString", &DxfValue::asString, "转为字符串")
		.def("asPoint2d", &DxfValue::asPoint2d, "转为 2D 点")
		.def("asPoint3d", &DxfValue::asPoint3d, "转为 3D 点")
		.def("asBinary", &DxfValue::asBinary, "转为二进制")
		.def("asHandle", &DxfValue::asHandle, "转为句柄")
		.def("asAdsName", &DxfValue::asAdsName, "转为 AdsName")
		.def("__repr__", [](const DxfValue& v) -> py::object {
			switch (v.getType()) {
				case DxfValueType::kNull: return py::str("None");
				case DxfValueType::kInt8: case DxfValueType::kInt16:
				case DxfValueType::kInt32: case DxfValueType::kInt64:
					return py::str(std::to_string(v.asInt64()));
				case DxfValueType::kDouble:
					return py::str(std::to_string(v.asDouble()));
				case DxfValueType::kString:
					return py::cast(v.asString());
				case DxfValueType::kPoint2d: {
					auto p = v.asPoint2d();
					return py::str("(" + std::to_string(p[0]) + ", " + std::to_string(p[1]) + ")");
				}
				case DxfValueType::kPoint3d: {
					auto p = v.asPoint3d();
					return py::str("(" + std::to_string(p[0]) + ", " + std::to_string(p[1]) + ", " + std::to_string(p[2]) + ")");
				}
				default: return py::str("DxfValue");
			}
		})
		;

	// === 再绑定 TypedValue ===
	py::class_<TypedValue>(m, "TypedValue", "DXF 组码对 (code, value)")
		.def(py::init([](int code, const py::object& v) {
			return TypedValue((short)code, DxfValue::fromPy(v, code));
		}), py::arg("code"), py::arg("value"), "构造组码对；value 类型按组码自动转换")
		.def_readwrite("code", &TypedValue::code, "组码")
		.def_property("value",
			[](const TypedValue& tv) { return tv.value.toPy(); },
			[](TypedValue& tv, const py::object& v) { tv.value = DxfValue::fromPy(v, tv.code); },
			"组码值（自动转换为 Python int/float/str/tuple）")
		.def("__str__", [](const TypedValue& tv) {
			return py::cast(tv.toString());
		})
		.def("__repr__", [](const TypedValue& tv) {
			return py::cast(tv.toString());
		})
		// ---- 比较运算符（过滤器 DSL）----
		.def("__eq__", [](const TypedValue& tv, const py::object& value) {
			// TypedValue(code, 0) == value → 等于过滤器
			TypedValue newTv(tv.code, DxfValue::fromPy(value, tv.code));
			return py::cast(PyEq(newTv));
		}, py::arg("value"), "组码 == 值（等于过滤器）")
		.def("__ne__", [](const TypedValue& tv, const py::object& value) {
			// TypedValue(code, 0) != value → 不等于过滤器
			TypedValue newTv(tv.code, DxfValue::fromPy(value, tv.code));
			return py::cast(PyNe(newTv));
		}, py::arg("value"), "组码 != 值（不等于过滤器）")
		.def("__lt__", [](const TypedValue& tv, const py::object& value) {
			// TypedValue(code, 0) < value → 小于过滤器
			TypedValue newTv(tv.code, DxfValue::fromPy(value, tv.code));
			return py::cast(PyLt(newTv));
		}, py::arg("value"), "组码 < 值（小于过滤器）")
		.def("__le__", [](const TypedValue& tv, const py::object& value) {
			// TypedValue(code, 0) <= value → 小于等于过滤器
			TypedValue newTv(tv.code, DxfValue::fromPy(value, tv.code));
			return py::cast(PyLe(newTv));
		}, py::arg("value"), "组码 <= 值（小于等于过滤器）")
		.def("__gt__", [](const TypedValue& tv, const py::object& value) {
			// TypedValue(code, 0) > value → 大于过滤器
			TypedValue newTv(tv.code, DxfValue::fromPy(value, tv.code));
			return py::cast(PyGt(newTv));
		}, py::arg("value"), "组码 > 值（大于过滤器）")
		.def("__ge__", [](const TypedValue& tv, const py::object& value) {
			// TypedValue(code, 0) >= value → 大于等于过滤器
			TypedValue newTv(tv.code, DxfValue::fromPy(value, tv.code));
			return py::cast(PyGe(newTv));
		}, py::arg("value"), "组码 >= 值（大于等于过滤器）")
		// ---- 比较方法（与运算符等价）----
		.def("eq", [](const TypedValue& tv) { return py::cast(PyEq(tv)); },
			"eq() → 等于过滤器")
		.def("ne", [](const TypedValue& tv) { return py::cast(PyNe(tv)); },
			"ne() → 不等于过滤器")
		.def("lt", [](const TypedValue& tv) { return py::cast(PyLt(tv)); },
			"lt() → 小于过滤器")
		.def("le", [](const TypedValue& tv) { return py::cast(PyLe(tv)); },
			"le() → 小于等于过滤器")
		.def("gt", [](const TypedValue& tv) { return py::cast(PyGt(tv)); },
			"gt() → 大于过滤器")
		.def("ge", [](const TypedValue& tv) { return py::cast(PyGe(tv)); },
			"ge() → 大于等于过滤器")
		// ---- 逻辑运算符（过滤器 DSL）----
		.def("__invert__", [](const TypedValue& a) { return ~a; },
			"~cond → NOT(cond) 包裹")
		.def("__and__", [](const TypedValue& a, const TypedValue& b) { return a & b; },
			"a & b → AND(a, b)")
		.def("__and__", [](const TypedValue& a, const TVList& b) { return a & b; },
			"a & list → AND(a, list...)")
		.def("__or__", [](const TypedValue& a, const TypedValue& b) { return a | b; },
			"a | b → OR(a, b)")
		.def("__or__", [](const TypedValue& a, const TVList& b) { return a | b; },
			"a | list → OR(a, list...)");
}

void bind_PyTvList(pym& m) {
	py::class_<TVList>(m, "TvList", "DXF 组码列表（顺序容器）")
		.def(py::init<>(), "空列表")
		.def("__len__", [](const TVList& t) { return t.size(); })
		.def("__getitem__", [](const TVList& t, size_t i) -> const TypedValue& {
			if (i >= t.size()) throw py::index_error();
			return t[i];
		}, py::arg("i"), py::return_value_policy::reference_internal)
		.def("__iter__", [](const TVList& t) {
			return py::make_iterator(t.begin(), t.end());
		}, py::return_value_policy::reference_internal)
		.def("append", [](TVList& t, int code, const py::object& v) {
			t.emplace_back((short)code, DxfValue::fromPy(v, code));
		}, py::arg("code"), py::arg("value"), "追加 (code, value)，value 自动转换")
		.def("clear", [](TVList& t) { t.clear(); }, "清空")
		// ---- 容器运算符 ----
		.def("__setitem__", [](TVList& t, size_t i, const TypedValue& tv) {
			if (i >= t.size()) throw py::index_error();
			t[i] = tv;
		}, py::arg("i"), py::arg("tv"), "按索引赋值 TypedValue")
		.def("__add__", [](const TVList& a, const TVList& b) {
			TVList r(a);
			r.insert(r.end(), b.begin(), b.end());
			return r;
		}, py::arg("other"), "两个 TvList 拼接（返回新列表）")
		.def("__contains__", [](const TVList& t, int code) {
			for (auto& tv : t) if (tv.code == code) return true;
			return false;
		}, py::arg("code"), "是否包含指定组码")
		// ---- 逻辑运算符（过滤器 DSL）----
		.def("__invert__", [](const TVList& a) { return ~a; },
			"~list → NOT(list) 包裹")
		.def("__and__", [](const TVList& a, const TypedValue& b) { return a & b; },
			"list & cond → AND(list..., cond)")
		.def("__and__", [](const TVList& a, const TVList& b) { return a & b; },
			"list & list → AND(list..., list...)")
		.def("__or__", [](const TVList& a, const TypedValue& b) { return a | b; },
			"list | cond → OR(list..., cond)")
		.def("__or__", [](const TVList& a, const TVList& b) { return a | b; },
			"list | list → OR(list..., list...)")
		// ---- 比较方法（与运算符等价）----
		.def("eq", [](const TVList& t) {
			if (t.empty()) return py::cast(t);
			return py::cast(PyEq(t[0]));
		}, "eq() → 对第一个元素应用等于过滤器")
		.def("ne", [](const TVList& t) {
			if (t.empty()) return py::cast(t);
			return py::cast(PyNe(t[0]));
		}, "ne() → 对第一个元素应用不等于过滤器")
		.def("lt", [](const TVList& t) {
			if (t.empty()) return py::cast(t);
			return py::cast(PyLt(t[0]));
		}, "lt() → 对第一个元素应用小于过滤器")
		.def("le", [](const TVList& t) {
			if (t.empty()) return py::cast(t);
			return py::cast(PyLe(t[0]));
		}, "le() → 对第一个元素应用小于等于过滤器")
		.def("gt", [](const TVList& t) {
			if (t.empty()) return py::cast(t);
			return py::cast(PyGt(t[0]));
		}, "gt() → 对第一个元素应用大于过滤器")
		.def("ge", [](const TVList& t) {
			if (t.empty()) return py::cast(t);
			return py::cast(PyGe(t[0]));
		}, "ge() → 对第一个元素应用大于等于过滤器")
		// ---- 比较运算符 ----
		.def("__eq__", [](const TVList& a, const TVList& b) {
			if (a.size() != b.size()) return false;
			for (size_t i = 0; i < a.size(); i++) {
				if (a[i].code != b[i].code || !a[i].value.toPy().equal(b[i].value.toPy()))
					return false;
			}
			return true;
		}, "两个列表完全相等才视为相等")
		.def("__ne__", [](const TVList& a, const TVList& b) {
			if (a.size() != b.size()) return true;
			for (size_t i = 0; i < a.size(); i++) {
				if (a[i].code != b[i].code || !a[i].value.toPy().equal(b[i].value.toPy()))
					return true;
			}
			return false;
		}, "两个列表不等")
		.def("__str__", [](const TVList& t) {
			return py::cast(t.toString());
		});

	// ---- 比较工厂（模块级）----
	m.def("eq", [](const TypedValue& c) { return PyEq(c); }, py::arg("cond"), "EQ 过滤器");
	m.def("ne", [](const TypedValue& c) { return PyNe(c); }, py::arg("cond"), "NEQ 过滤器");
	m.def("lt", [](const TypedValue& c) { return PyLt(c); }, py::arg("cond"), "LT 过滤器");
	m.def("le", [](const TypedValue& c) { return PyLe(c); }, py::arg("cond"), "LE 过滤器");
	m.def("gt", [](const TypedValue& c) { return PyGt(c); }, py::arg("cond"), "GT 过滤器");
	m.def("ge", [](const TypedValue& c) { return PyGe(c); }, py::arg("cond"), "GE 过滤器");

	// ---- 过滤器逻辑常量（模块级）----
	m.attr("AND_START") = AND_START;
	m.attr("AND_END") = AND_END;
	m.attr("OR_START") = OR_START;
	m.attr("OR_END") = OR_END;
	m.attr("NOT_START") = NOT_START;
	m.attr("NOT_END") = NOT_END;
}

void bind_PyResbuf(pym& m) {
	py::class_<PyResbuf>(m, "Resbuf", "resbuf 链表封装（DXF 数据链）")
		.def(py::init<>(), "空链表")
		.def(py::init<const PyResbuf&>(), "拷贝构造（深克隆）")
		.def(py::init([](const TVList& tvs) { return PyResbuf(tvs); }),
			py::arg("tvs"), "从 TvList 构造")
		.def("toList", [](const PyResbuf& rb) { return rb.toList(); },
			py::return_value_policy::move, "转 TvList")
		.def("isNull", &PyResbuf::isNull, "是否为空链表")
		.def("__bool__", [](const PyResbuf& rb) { return !rb.isNull(); })
		.def("__len__", [](const PyResbuf& rb) { return (size_t)rb.toList().size(); })
		.def("__getitem__", [](const PyResbuf& rb, int i) {
			auto t = rb.toList();   // 按值返回，避免把局部 list 的元素引用交给 Python（悬垂引用）
			const int n = (int)t.size();
			if (i < 0) i += n;
			if (i < 0 || i >= n) throw py::index_error("Resbuf 索引越界");
			return t[i];
		}, py::arg("i"), "rb[i] 取第 i 个组码对（支持负索引）")
		.def("__iter__", [](const PyResbuf& rb) {
			py::list out;
			for (const auto& tv : rb.toList()) out.append(py::cast(tv));
			return out.attr("__iter__")();
		}, "for tv in rb：遍历组码对")
		.def("__str__", [](const PyResbuf& rb) {
			return py::cast(rb.toString());
		})
		.def("print", &PyResbuf::print, "打印链表到 CAD 命令行");
}
