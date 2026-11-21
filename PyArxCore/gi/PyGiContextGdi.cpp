// ===========================================================================
// PyGiContextGdi.cpp —— AcGiContext 访问器 + AcGiGdiDrawObject 的 Python 桥
// ===========================================================================
#include "stdafx.h"
#include "PyGiContextGdi.h"
#include "PyDbDatabase.h"

bool GiContextRef::isPsOut() const { return m_p && m_p->isPsOut() == Adesk::kTrue; }
bool GiContextRef::isPlotGeneration() const { return m_p && m_p->isPlotGeneration() == Adesk::kTrue; }
bool GiContextRef::isBoundaryClipping() const { return m_p && m_p->isBoundaryClipping(); }
bool GiContextRef::isNesting() const { return m_p && m_p->isNesting(); }
bool GiContextRef::supportsTrueTypeText() const { return m_p && m_p->supportsTrueTypeText(); }
bool GiContextRef::supportsOwnerDraw() const { return m_p && m_p->supportsOwnerDraw(); }
bool GiContextRef::supportsFillTrait() const { return m_p && m_p->supportsFillTrait(); }
bool GiContextRef::supportsFastRenderBuffer() const { return m_p && m_p->supportsFastRenderBuffer() == Adesk::kTrue; }
bool GiContextRef::supportsNativePointCloudRender() const { return m_p && m_p->supportsNativePointCloudRender() == Adesk::kTrue; }

void GiContextRef::disableFastMoveDrag() const
{
	if (m_p) m_p->disableFastMoveDrag();
}

AcCmEntityColor GiContextRef::effectiveColor() const
{
	return m_p ? m_p->effectiveColor() : AcCmEntityColor();
}

int GiContextRef::byBlockLineWeight() const
{
	return m_p ? static_cast<int>(m_p->byBlockLineWeight()) : 0;
}

unsigned long long GiContextRef::byBlockPlotStyleNameId() const
{
	if (!m_p) return 0ull;
	return static_cast<unsigned long long>(m_p->byBlockPlotStyleNameId().asOldId());
}

unsigned long long GiContextRef::byBlockMaterialId() const
{
	if (!m_p) return 0ull;
	return static_cast<unsigned long long>(m_p->byBlockMaterialId().asOldId());
}

PyDbDatabase* GiContextRef::database() const
{
	if (!m_p) return nullptr;
	AcDbDatabase* db = m_p->database();
	if (!db) return nullptr;
	return new PyDbDatabase(db, false);
}

// ---------------------------------------------------------------------------
// AcGiGdiDrawObject 的 Python 桥
// ---------------------------------------------------------------------------
Adesk::Boolean AcGiGdiDrawObjectPy::draw(HDC hDC, int x, int y, int w, int h)
{
	py::gil_scoped_acquire gil;
	py::function override = py::get_override(this, "draw");
	if (!override) return Adesk::kTrue;
	try {
		py::object r = override(py::int_(static_cast<long long>(reinterpret_cast<intptr_t>(hDC))), x, y, w, h);
		if (r.is_none()) return Adesk::kTrue;
		return py::cast<bool>(r) ? Adesk::kTrue : Adesk::kFalse;
	} catch (py::error_already_set& e) {
		e.discard_as_unraisable("AcGiGdiDrawObject.draw");
		return Adesk::kFalse;
	}
}

int AcGiGdiDrawObjectPy::width() const
{
	py::gil_scoped_acquire gil;
	py::function override = py::get_override(this, "width");
	if (!override) return 0;
	try {
		return py::cast<int>(override());
	} catch (py::error_already_set& e) {
		e.discard_as_unraisable("AcGiGdiDrawObject.width");
		return 0;
	}
}

int AcGiGdiDrawObjectPy::height() const
{
	py::gil_scoped_acquire gil;
	py::function override = py::get_override(this, "height");
	if (!override) return 0;
	try {
		return py::cast<int>(override());
	} catch (py::error_already_set& e) {
		e.discard_as_unraisable("AcGiGdiDrawObject.height");
		return 0;
	}
}

void AcGiGdiDrawObjectPy::addRef()
{
	py::gil_scoped_acquire gil;
	py::function override = py::get_override(this, "addRef");
	if (!override) return;
	try {
		override();
	} catch (py::error_already_set& e) {
		e.discard_as_unraisable("AcGiGdiDrawObject.addRef");
	}
}

void AcGiGdiDrawObjectPy::delRef()
{
	py::gil_scoped_acquire gil;
	py::function override = py::get_override(this, "delRef");
	if (!override) return;
	try {
		override();
	} catch (py::error_already_set& e) {
		e.discard_as_unraisable("AcGiGdiDrawObject.delRef");
	}
}

// ---------------------------------------------------------------------------
// 绑定
// ---------------------------------------------------------------------------
void bind_PyGiContextGdi(pym& m)
{
	py::class_<GiContextRef>(m, "AcGiContext",
		"绘制上下文（只读，由 AcGiCommonDraw.context() 取得）")
		.def("valid", &GiContextRef::valid)
		.def("isPsOut", &GiContextRef::isPsOut)
		.def("isPlotGeneration", &GiContextRef::isPlotGeneration)
		.def("isBoundaryClipping", &GiContextRef::isBoundaryClipping)
		.def("isNesting", &GiContextRef::isNesting)
		.def("supportsTrueTypeText", &GiContextRef::supportsTrueTypeText)
		.def("supportsOwnerDraw", &GiContextRef::supportsOwnerDraw)
		.def("supportsFillTrait", &GiContextRef::supportsFillTrait)
		.def("supportsFastRenderBuffer", &GiContextRef::supportsFastRenderBuffer)
		.def("supportsNativePointCloudRender", &GiContextRef::supportsNativePointCloudRender)
		.def("disableFastMoveDrag", &GiContextRef::disableFastMoveDrag)
		.def("effectiveColor", &GiContextRef::effectiveColor)
		.def("byBlockLineWeight", &GiContextRef::byBlockLineWeight)
		.def("byBlockPlotStyleNameId", &GiContextRef::byBlockPlotStyleNameId)
		.def("byBlockMaterialId", &GiContextRef::byBlockMaterialId)
		.def("database", &GiContextRef::database, py::return_value_policy::take_ownership);

	py::class_<AcGiGdiDrawObjectPy>(m, "AcGiGdiDrawObject",
		"HDC 自绘对象：子类实现 draw(self, hdc, x, y, w, h)、width()、height()")
		.def(py::init<>())
		.def("selfTest", [](AcGiGdiDrawObjectPy& o) {
			// 本机回路：C++ 调虚函数 -> py::get_override -> Python 子类实现，
			// 用于在没有支持 ownerDraw 的驱动时也能自检桥接是否通。
			o.addRef();
			int w = o.width();
			int h = o.height();
			bool d = o.draw(reinterpret_cast<HDC>(static_cast<intptr_t>(0)), 0, 0, w, h) == Adesk::kTrue;
			o.delRef();
			return py::make_tuple(w, h, d);
		}, "selfTest() -> (width, height, draw_ok)：走 Python 子类实现自检");
}
