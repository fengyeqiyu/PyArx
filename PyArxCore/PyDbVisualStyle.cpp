#include "stdafx.h"
#include "PyErrorStatusException.h"
#include "PyDbVisualStyle.h"

PyWrapper_Cpp(DbVisualStyle);

int PyDbVisualStyle::type() const { if (!impObj()) return 0; return impObj() ? (int)impObj()->type() : 0; }
bool PyDbVisualStyle::setType(int type) {
	if (!impObj()) return false;
	return PyWarnIfError(impObj()->setType(static_cast<AcGiVisualStyle::Type>(type)), L"AcDbVisualStyle::setType");
}
bool PyDbVisualStyle::setDescription(const AcString& s) {
	if (!impObj()) return false;
	return PyWarnIfError(impObj()->setDescription(s.kwszPtr()), L"AcDbVisualStyle::setDescription");
}
bool PyDbVisualStyle::setTraitInt(int prop, int nVal, int op) {
	if (!impObj()) return false;
	return PyWarnIfError(impObj()->setTrait(static_cast<AcGiVisualStyleProperties::Property>(prop), nVal,
		static_cast<AcGiVisualStyleOperations::Operation>(op)), L"AcDbVisualStyle::setTrait(int)");
}
bool PyDbVisualStyle::setTraitBool(int prop, bool bVal, int op) {
	if (!impObj()) return false;
	return PyWarnIfError(impObj()->setTrait(static_cast<AcGiVisualStyleProperties::Property>(prop), bVal,
		static_cast<AcGiVisualStyleOperations::Operation>(op)), L"AcDbVisualStyle::setTrait(bool)");
}
bool PyDbVisualStyle::setTraitDouble(int prop, double dVal, int op) {
	if (!impObj()) return false;
	return PyWarnIfError(impObj()->setTrait(static_cast<AcGiVisualStyleProperties::Property>(prop), dVal,
		static_cast<AcGiVisualStyleOperations::Operation>(op)), L"AcDbVisualStyle::setTrait(double)");
}
bool PyDbVisualStyle::setTraitColorF(int prop, double r, double g, double b, int op) {
	if (!impObj()) return false;
	return PyWarnIfError(impObj()->setTrait(static_cast<AcGiVisualStyleProperties::Property>(prop), r, g, b,
		static_cast<AcGiVisualStyleOperations::Operation>(op)), L"AcDbVisualStyle::setTrait(color)");
}

void bind_PyDbVisualStyle(pym& m) {
	py::class_<PyDbVisualStyle, PyDbObject>(m, "PyDbVisualStyle")
		.def(py::init<>())
		.def(py::init<AcDbVisualStyle*>(), py::arg("p"))
		.def(py::init([]() { return std::make_unique<PyDbVisualStyle>(new AcDbVisualStyle()).release(); }), "无参构造（new 默认 AcDbVisualStyle）")
		PyWrapper_BIND(DbVisualStyle)
		.def("type", &PyDbVisualStyle::type)
		.def("setType", &PyDbVisualStyle::setType, py::arg("type"))
		.def("setDescription", &PyDbVisualStyle::setDescription, py::arg("s"))
		.def("setTrait", &PyDbVisualStyle::setTraitInt, py::arg("prop"), py::arg("nVal"), py::arg("op") = 0)
		.def("setTrait", &PyDbVisualStyle::setTraitBool, py::arg("prop"), py::arg("bVal"), py::arg("op") = 0)
		.def("setTrait", &PyDbVisualStyle::setTraitDouble, py::arg("prop"), py::arg("dVal"), py::arg("op") = 0)
		.def("setTraitColor", &PyDbVisualStyle::setTraitColorF, py::arg("prop"), py::arg("r"), py::arg("g"), py::arg("b"), py::arg("op") = 0,
			"按 RGB 浮点(0.0~1.0+)设置颜色类 trait")
		// 未绑重载：setTrait(Property, const AcGiVariant*, Operation)（AcGiVariant 未封装）；
		//          setTrait(Property, const AcCmColor*, Operation)（AcCmColor 未独立封装为 Python 类，
		//          颜色类属性请用 setTraitColor 走 RGB 浮点重载）。
		;
}
