// PyGiBinder.cpp —— PyArx.Gi 子模块绑定：AcGi 图形接口类。
// 依赖 PyRxObject / AcDbObjectId("ObjectId") / PyDbEntity（均注册于 PyArx.Db 子模块），
// 故 bind_PyGi 须在 PyArx.Db 之后由 PyArxBinder.cpp 调用。
// 依赖尚未绑定的值类型（AcCmEntityColor / AcCmTransparency / AcGiFillType /
// AcGiSubEntityTraits::ShadowFlags / AcGiRegenType / PyDbExtents3d）的方法暂不导出。
#include "stdafx.h"
#include "PyGiBinder.h"
#include "PyGiDrawable.h"
#include "PyGiSubEntityTraits.h"
#include "PyGiWorldGeometry.h"
#include "PyRxObject.h"
#include "PyDbTypeCaster.h"
#include "PyDbEntity.h"

// ===========================================================================
// PyGiSubEntityTraits —— 子实体外观特性
// ===========================================================================
void bind_PyGiSubEntityTraits(pym& m) {
	auto cls = py::class_<PyGiSubEntityTraits, PyRxObject>(m, "PyGiSubEntityTraits");
	cls
		PyWrapper_BIND(GiSubEntityTraits)
		.def("setColor", [](PyGiSubEntityTraits& s, int color) { s.setColor(static_cast<Adesk::UInt16>(color)); }, py::arg("color"))
		.def("color", &PyGiSubEntityTraits::color)
		.def("setLayer", &PyGiSubEntityTraits::setLayer, py::arg("layerId"))
		.def("layerId", &PyGiSubEntityTraits::layerId)
		.def("setLineType", &PyGiSubEntityTraits::setLineType, py::arg("linetypeId"))
		.def("lineTypeId", &PyGiSubEntityTraits::lineTypeId)
		.def("setSelectionMarker", [](PyGiSubEntityTraits& s, int markerId) { s.setSelectionMarker(static_cast<Adesk::LongPtr>(markerId)); }, py::arg("markerId"))
		.def("setLineWeight", [](PyGiSubEntityTraits& s, int lw) { s.setLineWeight(static_cast<AcDb::LineWeight>(lw)); }, py::arg("lw"))
		.def("lineWeight", [](const PyGiSubEntityTraits& s) { return static_cast<int>(s.lineWeight()); })
		.def("setLineTypeScale", &PyGiSubEntityTraits::setLineTypeScale, py::arg("dScale") = 1.0)
		.def("lineTypeScale", &PyGiSubEntityTraits::lineTypeScale)
		.def("setThickness", &PyGiSubEntityTraits::setThickness, py::arg("dThickness"))
		.def("thickness", &PyGiSubEntityTraits::thickness)
		.def("setVisualStyle", &PyGiSubEntityTraits::setVisualStyle, py::arg("visualStyleId"))
		.def("visualStyle", &PyGiSubEntityTraits::visualStyle)
		.def("setMaterial", &PyGiSubEntityTraits::setMaterial, py::arg("materialId"))
		.def("materialId", &PyGiSubEntityTraits::materialId)
		.def("setSectionable", &PyGiSubEntityTraits::setSectionable, py::arg("bSectionable"))
		.def("sectionable", &PyGiSubEntityTraits::sectionable)
		.def("setDrawFlags", [](PyGiSubEntityTraits& s, unsigned int flags) { return s.setDrawFlags(static_cast<Adesk::UInt32>(flags)); }, py::arg("flags"))
		.def("drawFlags", &PyGiSubEntityTraits::drawFlags)
		.def("setSelectionGeom", &PyGiSubEntityTraits::setSelectionGeom, py::arg("bSelectionflag"))
		.def("selectionGeom", &PyGiSubEntityTraits::selectionGeom);
}

// ===========================================================================
// PyGiDrawableTraits —— 图元特性（继承 PyGiSubEntityTraits）
// ===========================================================================
void bind_PyGiDrawableTraits(pym& m) {
	auto cls = py::class_<PyGiDrawableTraits, PyGiSubEntityTraits>(m, "PyGiDrawableTraits");
	cls
		PyWrapper_BIND(GiDrawableTraits)
		.def("setupForEntity", &PyGiDrawableTraits::setupForEntity, py::arg("pEntity"));
}

// ===========================================================================
// PyGiCommonDraw —— 公共绘制上下文（基类）
// ===========================================================================
void bind_PyGiCommonDraw(pym& m) {
	auto cls = py::class_<PyGiCommonDraw, PyRxObject>(m, "PyGiCommonDraw");
	cls
		PyWrapper_BIND(GiCommonDraw)
		.def("subEntityTraits", &PyGiCommonDraw::subEntityTraits)
		.def("regenAbort", &PyGiCommonDraw::regenAbort)
		.def("isDragging", &PyGiCommonDraw::isDragging);
}

// ===========================================================================
// PyGiWorldDraw / PyGiViewportDraw —— 世界/视口绘制上下文
// ===========================================================================
void bind_PyGiWorldDraw(pym& m) {
	auto cls = py::class_<PyGiWorldDraw, PyGiCommonDraw>(m, "PyGiWorldDraw");
	cls
		PyWrapper_BIND(GiWorldDraw)
		.def("geometry", &PyGiWorldDraw::geometry);
}

void bind_PyGiViewportDraw(pym& m) {
	auto cls = py::class_<PyGiViewportDraw, PyGiCommonDraw>(m, "PyGiViewportDraw");
	cls
		PyWrapper_BIND(GiViewportDraw)
		.def("geometry", &PyGiViewportDraw::geometry);
}

// ===========================================================================
// PyGiGeometry 族 —— 几何绘制接口
// ===========================================================================
void bind_PyGiGeometry(pym& m) {
	auto cls = py::class_<PyGiGeometry, PyRxObject>(m, "PyGiGeometry");
	cls PyWrapper_BIND(GiGeometry);
}

void bind_PyGiWorldGeometry(pym& m) {
	auto cls = py::class_<PyGiWorldGeometry, PyGiGeometry>(m, "PyGiWorldGeometry");
	cls PyWrapper_BIND(GiWorldGeometry);
}

void bind_PyGiViewportGeometry(pym& m) {
	auto cls = py::class_<PyGiViewportGeometry, PyGiGeometry>(m, "PyGiViewportGeometry");
	cls PyWrapper_BIND(GiViewportGeometry);
}

// ===========================================================================
// PyGiDrawable —— 可绘制对象（依赖上述类型均已注册）
// ===========================================================================
void bind_PyGiDrawable(pym& m) {
	auto cls = py::class_<PyGiDrawable, PyRxObject>(m, "PyGiDrawable");
	cls
		PyWrapper_BIND(GiDrawable)
		.def("isPersistent", &PyGiDrawable::isPersistent)
		.def("worldDraw", &PyGiDrawable::worldDraw, py::arg("wd"))
		.def("subWorldDraw", &PyGiDrawable::subWorldDraw, py::arg("wd"))
		.def("viewportDraw", &PyGiDrawable::viewportDraw, py::arg("vpd"))
		.def("subViewportDraw", &PyGiDrawable::subViewportDraw, py::arg("vd"))
		.def("viewportDrawLogicalFlags", &PyGiDrawable::viewportDrawLogicalFlags, py::arg("vd"))
		.def("subViewportDrawLogicalFlags", &PyGiDrawable::subViewportDrawLogicalFlags, py::arg("svd"))
		.def("setAttributes", &PyGiDrawable::setAttributes, py::arg("traits"))
		.def("subSetAttributes", &PyGiDrawable::subSetAttributes, py::arg("traits"))
		.def("rolloverHit", [](PyGiDrawable& d, int nSubentId, int nMouseFlags, bool bReset) {
			return d.rolloverHit(static_cast<Adesk::ULongPtr>(nSubentId), static_cast<Adesk::ULongPtr>(nMouseFlags), bReset ? Adesk::kTrue : Adesk::kFalse);
		}, py::arg("nSubentId"), py::arg("nMouseFlags"), py::arg("bReset"));
}

// ===========================================================================
// bind_PyGi: PyArx.Gi 子模块注册函数（由 PyArxBinder.cpp 调用）；依赖 PyArx.Db
// （PyRxObject/ObjectId/PyDbEntity 均注册于 PyArx.Db），
// 绑定顺序按类继承：SubEntityTraits → CommonDraw → WorldDraw/ViewportDraw →
// Geometry 族 → Drawable（依赖前者返回值/参数）。
// ===========================================================================
void bind_PyGi(pym& m) {
	m.doc() = "AutoCAD ARX AcGi Graphics Interface";
	py::module_::import("PyArxCore.Db"); // PyRxObject / ObjectId / PyDbEntity type_info（PyArxCore.Db 子模块）

	bind_PyGiSubEntityTraits(m);
	bind_PyGiDrawableTraits(m);
	bind_PyGiCommonDraw(m);
	bind_PyGiWorldDraw(m);
	bind_PyGiViewportDraw(m);
	bind_PyGiGeometry(m);
	bind_PyGiWorldGeometry(m);
	bind_PyGiViewportGeometry(m);
	bind_PyGiDrawable(m);
}
