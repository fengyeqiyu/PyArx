#include "stdafx.h"
#include "PyGiTransient.h"
#include "acgitransient.h"
#include "PyGiWorldGeometry.h"
#include "PyGiSubEntityTraits.h"
#include "PyGiViewport.h"
#include "PyGiTypes.h"
#include "PyGiDrawable.h"

// ===========================================================================
// Python 自绘桥实现
// 约定：图形系统在 CAD 主线程回调，GIL 由本线程获取；Python 回调里抛异常
// 绝不能穿透到 AutoCAD（会崩进程），统一 discard 成 unraisable 后返回安全值。
// ===========================================================================

bool PyGiPythonDrawable::bounds(AcDbExtents& out) const {
	py::gil_scoped_acquire gil;
	py::function override = py::get_override(this, "bounds");
	if (!override) return false;
	try {
		py::object r = override();
		if (r.is_none()) return false;
		const py::sequence s = py::cast<py::sequence>(r);
		if (static_cast<size_t>(py::len(s)) < 2) return false;
		out.set(AsPoint3d(s[0]), AsPoint3d(s[1]));
		return true;
	} catch (py::error_already_set& e) {
		e.discard_as_unraisable("PyGiPythonDrawable.bounds");
		return false;
	}
}

Adesk::UInt32 PyGiPythonDrawable::subSetAttributes(AcGiDrawableTraits* traits) {
	py::gil_scoped_acquire gil;
	py::function override = py::get_override(this, "setAttributes");
	if (!override) return AcGiDrawable::kDrawableNone;
	try {
		PyGiDrawableTraits t(traits, false);
		py::object r = override(py::cast(t));
		if (r.is_none()) return AcGiDrawable::kDrawableNone;
		return py::cast<Adesk::UInt32>(r);
	} catch (py::error_already_set& e) {
		e.discard_as_unraisable("PyGiPythonDrawable.setAttributes");
		return AcGiDrawable::kDrawableNone;
	}
}

Adesk::Boolean PyGiPythonDrawable::subWorldDraw(AcGiWorldDraw* wd) {
	py::gil_scoped_acquire gil;
	py::function override = py::get_override(this, "worldDraw");
	if (!override) return Adesk::kTrue;
	try {
		PyGiWorldDraw w(wd);
		py::object r = override(py::cast(w));
		if (r.is_none()) return Adesk::kTrue;
		return py::cast<bool>(r) ? Adesk::kTrue : Adesk::kFalse;
	} catch (py::error_already_set& e) {
		e.discard_as_unraisable("PyGiPythonDrawable.worldDraw");
		return Adesk::kFalse;
	}
}

void PyGiPythonDrawable::subViewportDraw(AcGiViewportDraw* vd) {
	py::gil_scoped_acquire gil;
	py::function override = py::get_override(this, "viewportDraw");
	if (!override) return;
	try {
		PyGiViewportDraw v(vd);
		override(py::cast(v));
	} catch (py::error_already_set& e) {
		e.discard_as_unraisable("PyGiPythonDrawable.viewportDraw");
	}
}

Adesk::UInt32 PyGiPythonDrawable::subViewportDrawLogicalFlags(AcGiViewportDraw* vd) {
	py::gil_scoped_acquire gil;
	py::function override = py::get_override(this, "viewportDrawLogicalFlags");
	if (!override) return 0u;
	try {
		PyGiViewportDraw v(vd);
		py::object r = override(py::cast(v));
		if (r.is_none()) return 0u;
		return py::cast<Adesk::UInt32>(r);
	} catch (py::error_already_set& e) {
		e.discard_as_unraisable("PyGiPythonDrawable.viewportDrawLogicalFlags");
		return 0u;
	}
}

// ===========================================================================
// 瞬态管理 + drawable 取值 helper + 保活登记
// ===========================================================================
namespace {

AcGiTransientManager* manager() {
	return acgiGetTransientManager();
}

AcGiDrawable* AsDrawable(const py::handle& h) {
	if (py::isinstance<PyGiPythonDrawable>(h))
		return py::cast<PyGiPythonDrawable*>(h);
	if (py::isinstance<PyGiDrawable>(h))
		return py::cast<PyGiDrawable*>(h)->impObj();
	throw py::type_error("需要 GiPythonDrawable 或 AcGiDrawable 包装对象");
}

AcArray<int> AsViewports(const py::object& viewports) {
	AcArray<int> out;
	if (viewports.is_none()) return out;
	const py::sequence s = py::cast<py::sequence>(viewports);
	const size_t n = static_cast<size_t>(py::len(s));
	out.setLogicalLength(static_cast<int>(n));
	for (size_t i = 0; i < n; ++i)
		out[i] = py::cast<int>(s[static_cast<py::ssize_t>(i)]);
	return out;
}

} // namespace

void bind_PyGiTransient(pym& m) {
	py::enum_<AcGiTransientDrawingMode>(m, "AcGiTransientDrawingMode")
		.value("kAcGiMain", kAcGiMain)
		.value("kAcGiSprite", kAcGiSprite)
		.value("kAcGiDirectShortTerm", kAcGiDirectShortTerm)
		.value("kAcGiHighlight", kAcGiHighlight)
		.value("kAcGiDirectTopmost", kAcGiDirectTopmost)
		.value("kAcGiContrast", kAcGiContrast)
		.value("kAcGiDrawingModeCount", kAcGiDrawingModeCount);

	py::enum_<AcGiDrawable::DrawableType>(m, "AcGiDrawableType")
		.value("kGeometry", AcGiDrawable::kGeometry)
		.value("kDistantLight", AcGiDrawable::kDistantLight)
		.value("kPointLight", AcGiDrawable::kPointLight)
		.value("kSpotLight", AcGiDrawable::kSpotLight)
		.value("kAmbientLight", AcGiDrawable::kAmbientLight)
		.value("kSolidBackground", AcGiDrawable::kSolidBackground)
		.value("kGradientBackground", AcGiDrawable::kGradientBackground)
		.value("kImageBackground", AcGiDrawable::kImageBackground)
		.value("kGroundPlaneBackground", AcGiDrawable::kGroundPlaneBackground)
		.value("kViewport", AcGiDrawable::kViewport)
		.value("kWebLight", AcGiDrawable::kWebLight)
		.value("kSkyBackground", AcGiDrawable::kSkyBackground)
		.value("kImageBasedLightingBackground", AcGiDrawable::kImageBasedLightingBackground);

	py::class_<PyGiPythonDrawable>(m, "GiPythonDrawable",
		"Python 自绘 AcGiDrawable：子类实现 worldDraw(self, wd) / viewportDraw(self, vd)")
		.def(py::init<>())
		.def("isPersistent", [](const PyGiPythonDrawable& d) { return d.isPersistent() == Adesk::kTrue; })
		.def("drawableType", [](const PyGiPythonDrawable& d) { return d.drawableType(); });

	// 保活表：加入瞬态的 Python 对象在这里挂一份强引用，erase 时移除。
	py::dict exposed;
	m.attr("_transient_refs") = exposed;

	auto keep = [exposed](const py::object& obj) {
		exposed[py::int_(reinterpret_cast<uintptr_t>(obj.ptr()))] = obj;
	};
	auto drop = [exposed](const py::object& obj) {
		exposed.attr("pop")(py::int_(reinterpret_cast<uintptr_t>(obj.ptr())), py::none());
	};

	m.def("transientManagerAvailable", []() { return manager() != nullptr; },
		"图形系统瞬态管理器是否可用（须在有图形上下文时调用）");

	m.def("addTransient", [keep](py::object drawable, int mode, int subMode, py::object viewports) -> bool {
		AcGiTransientManager* mgr = manager();
		if (!mgr) return false;
		const bool ok = mgr->addTransient(AsDrawable(drawable), static_cast<AcGiTransientDrawingMode>(mode),
			subMode, AsViewports(viewports));
		if (ok) keep(drawable);
		return ok;
	}, py::arg("drawable"), py::arg("mode"), py::arg("sub_mode") = 0, py::arg("viewports") = py::none(),
		"把自绘 drawable 加入瞬态图形；viewports=None 表示全部视口");

	m.def("eraseTransient", [drop](py::object drawable, py::object viewports) -> bool {
		AcGiTransientManager* mgr = manager();
		if (!mgr) return false;
		const bool ok = mgr->eraseTransient(AsDrawable(drawable), AsViewports(viewports));
		if (ok) drop(drawable);
		return ok;
	}, py::arg("drawable"), py::arg("viewports") = py::none());

	m.def("eraseTransients", [](int mode, int subMode, py::object viewports) -> bool {
		AcGiTransientManager* mgr = manager();
		if (!mgr) return false;
		return mgr->eraseTransients(static_cast<AcGiTransientDrawingMode>(mode), subMode, AsViewports(viewports));
	}, py::arg("mode"), py::arg("sub_mode") = 0, py::arg("viewports") = py::none());

	m.def("updateTransient", [](py::object drawable, py::object viewports) {
		AcGiTransientManager* mgr = manager();
		if (!mgr) return false;
		mgr->updateTransient(AsDrawable(drawable), AsViewports(viewports));
		return true;
	}, py::arg("drawable"), py::arg("viewports") = py::none());

	m.def("addChildTransient", [keep](py::object child, py::object parent) -> bool {
		AcGiTransientManager* mgr = manager();
		if (!mgr) return false;
		const bool ok = mgr->addChildTransient(AsDrawable(child), AsDrawable(parent));
		if (ok) keep(child);
		return ok;
	}, py::arg("child"), py::arg("parent"));

	m.def("eraseChildTransient", [drop](py::object child, py::object parent) -> bool {
		AcGiTransientManager* mgr = manager();
		if (!mgr) return false;
		const bool ok = mgr->eraseChildTransient(AsDrawable(child), AsDrawable(parent));
		if (ok) drop(child);
		return ok;
	}, py::arg("child"), py::arg("parent"));

	m.def("updateChildTransient", [](py::object child, py::object parent) {
		AcGiTransientManager* mgr = manager();
		if (!mgr) return false;
		mgr->updateChildTransient(AsDrawable(child), AsDrawable(parent));
		return true;
	}, py::arg("child"), py::arg("parent"));

	m.def("getFreeSubDrawingMode", [](int mode, py::object viewports) {
		AcGiTransientManager* mgr = manager();
		if (!mgr) return -1;
		int sub = 0;
		return mgr->getFreeSubDrawingMode(static_cast<AcGiTransientDrawingMode>(mode), sub, AsViewports(viewports));
	}, py::arg("mode"), py::arg("viewports") = py::none(), "返回可用的子绘制模式号");
}
