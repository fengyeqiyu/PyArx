#include "stdafx.h"
#include "PyErrorStatusException.h"
#include "PyDbLayerFilter.h"

PyWrapper_Cpp(DbLayerFilter);

// 自身方法：add / remove / getAt / isValid / layerCount
void bind_PyDbLayerFilter(pym& m) {
	py::class_<PyDbLayerFilter, PyDbFilter>(m, "PyDbLayerFilter")
		.def(py::init<AcDbLayerFilter*>(), py::arg("p"))
		PyWrapper_BIND(DbLayerFilter)
		// ---- add(layerName) -> bool ----
		.def("add", [](PyDbLayerFilter& s, const AcString& name) -> bool {
			if (!s.impObj()) return false;
			return PyWarnIfError(s.impObj()->add(name.kwszPtr()), L"AcDbLayerFilter::add");
		}, py::arg("layer_name"), "向过滤器加入一个层名")
		// ---- remove(layerName) -> bool ----
		.def("remove", [](PyDbLayerFilter& s, const AcString& name) -> bool {
			if (!s.impObj()) return false;
			return PyWarnIfError(s.impObj()->remove(name.kwszPtr()), L"AcDbLayerFilter::remove");
		}, py::arg("layer_name"), "从过滤器移除一个层名")
		// ---- getAt(index) -> str（内部指针，不释放） ----
		.def("getAt", [](const PyDbLayerFilter& s, int index) -> AcString {
			if (!s.impObj()) return AcString();
			const ACHAR* pName = nullptr;
			if (!PyWarnIfError(s.impObj()->getAt(index, pName), L"AcDbLayerFilter::getAt"))
				return AcString();
			return pName ? AcString(pName) : AcString();
		}, py::arg("index"), "按索引取层名")
		// ---- isValid() -> bool ----
		.def("isValid", [](const PyDbLayerFilter& s) -> bool {
			if (!s.impObj()) return false;
			return s.impObj()->isValid() != Adesk::kFalse;
		}, "过滤器是否有效")
		// ---- layerCount() -> int ----
		.def("layerCount", [](const PyDbLayerFilter& s) -> int {
			if (!s.impObj()) return 0;
			return s.impObj()->layerCount();
		}, "过滤器包含的层数量")
		;
}
