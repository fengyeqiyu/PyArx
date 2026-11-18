#include "stdafx.h"
#include "PyErrorStatusException.h"
#include "PyDbDetailViewStyle.h"

PyWrapper_Cpp(DbDetailViewStyle);

std::tuple<int,int,int> PyDbDetailViewStyle::arrowSymbolColor() const {
	AcCmColor c = impObj()->arrowSymbolColor();
	return std::make_tuple((int)c.red(), (int)c.green(), (int)c.blue());
}
uint64_t PyDbDetailViewStyle::arrowSymbolId() const {
	if (!impObj()) return 0ULL;
	AcDbObjectId id = impObj()->arrowSymbolId();
	return id.isNull() ? 0ULL : static_cast<uint64_t>(id.asOldId());
}
double PyDbDetailViewStyle::arrowSymbolSize() const { if (!impObj()) return 0.0; return impObj()->arrowSymbolSize(); }

std::tuple<int,int,int> PyDbDetailViewStyle::identifierColor() const {
	AcCmColor c = impObj()->identifierColor();
	return std::make_tuple((int)c.red(), (int)c.green(), (int)c.blue());
}
double PyDbDetailViewStyle::identifierHeight() const { if (!impObj()) return 0.0; return impObj()->identifierHeight(); }
double PyDbDetailViewStyle::identifierOffset() const { if (!impObj()) return 0.0; return impObj()->identifierOffset(); }
int PyDbDetailViewStyle::identifierPlacement() const { if (!impObj()) return 0; return (int)impObj()->identifierPlacement(); }
uint64_t PyDbDetailViewStyle::identifierStyleId() const {
	if (!impObj()) return 0ULL;
	AcDbObjectId id = impObj()->identifierStyleId();
	return id.isNull() ? 0ULL : static_cast<uint64_t>(id.asOldId());
}

bool PyDbDetailViewStyle::setArrowSymbolColor(int r, int g, int b) {
	if (!impObj()) return false;
	AcCmColor c; c.setRGB((Adesk::UInt8)r, (Adesk::UInt8)g, (Adesk::UInt8)b);
	return PyWarnIfError(impObj()->setArrowSymbolColor(c), L"AcDbDetailViewStyle::setArrowSymbolColor");
}
bool PyDbDetailViewStyle::setArrowSymbolId(uint64_t idv) {
	if (!impObj()) return false;
	AcDbObjectId id; id.setFromOldId(static_cast<Adesk::UIntPtr>(idv));
	return PyWarnIfError(impObj()->setArrowSymbolId(id), L"AcDbDetailViewStyle::setArrowSymbolId");
}
bool PyDbDetailViewStyle::setArrowSymbolSize(double size) {
	if (!impObj()) return false;
	return PyWarnIfError(impObj()->setArrowSymbolSize(size), L"AcDbDetailViewStyle::setArrowSymbolSize");
}
bool PyDbDetailViewStyle::setIdentifierColor(int r, int g, int b) {
	if (!impObj()) return false;
	AcCmColor c; c.setRGB((Adesk::UInt8)r, (Adesk::UInt8)g, (Adesk::UInt8)b);
	return PyWarnIfError(impObj()->setIdentifierColor(c), L"AcDbDetailViewStyle::setIdentifierColor");
}
bool PyDbDetailViewStyle::setIdentifierHeight(double h) {
	if (!impObj()) return false;
	return PyWarnIfError(impObj()->setIdentifierHeight(h), L"AcDbDetailViewStyle::setIdentifierHeight");
}
bool PyDbDetailViewStyle::setIdentifierOffset(double v) {
	if (!impObj()) return false;
	return PyWarnIfError(impObj()->setIdentifierOffset(v), L"AcDbDetailViewStyle::setIdentifierOffset");
}
bool PyDbDetailViewStyle::setIdentifierPlacement(int placement) {
	if (!impObj()) return false;
	return PyWarnIfError(impObj()->setIdentifierPlacement(static_cast<AcDbDetailViewStyle::IdentifierPlacement>(placement)),
		L"AcDbDetailViewStyle::setIdentifierPlacement");
}
bool PyDbDetailViewStyle::setIdentifierStyleId(uint64_t idv) {
	if (!impObj()) return false;
	AcDbObjectId id; id.setFromOldId(static_cast<Adesk::UIntPtr>(idv));
	return PyWarnIfError(impObj()->setIdentifierStyleId(id), L"AcDbDetailViewStyle::setIdentifierStyleId");
}

void bind_PyDbDetailViewStyle(pym& m) {
	py::class_<PyDbDetailViewStyle, PyDbModelDocViewStyle>(m, "PyDbDetailViewStyle")
		.def(py::init<AcDbDetailViewStyle*>(), py::arg("p"))
		PyWrapper_BIND(DbDetailViewStyle)
		.def("arrowSymbolColor", &PyDbDetailViewStyle::arrowSymbolColor, "返回箭头符号色 (r,g,b)")
		.def("arrowSymbolId", &PyDbDetailViewStyle::arrowSymbolId)
		.def("arrowSymbolSize", &PyDbDetailViewStyle::arrowSymbolSize)
		.def("identifierColor", &PyDbDetailViewStyle::identifierColor, "返回标识色 (r,g,b)")
		.def("identifierHeight", &PyDbDetailViewStyle::identifierHeight)
		.def("identifierOffset", &PyDbDetailViewStyle::identifierOffset)
		.def("identifierPlacement", &PyDbDetailViewStyle::identifierPlacement)
		.def("identifierStyleId", &PyDbDetailViewStyle::identifierStyleId)
		.def("setArrowSymbolColor", &PyDbDetailViewStyle::setArrowSymbolColor, py::arg("r"), py::arg("g"), py::arg("b"))
		.def("setArrowSymbolId", &PyDbDetailViewStyle::setArrowSymbolId, py::arg("id"))
		.def("setArrowSymbolSize", &PyDbDetailViewStyle::setArrowSymbolSize, py::arg("size"))
		.def("setIdentifierColor", &PyDbDetailViewStyle::setIdentifierColor, py::arg("r"), py::arg("g"), py::arg("b"))
		.def("setIdentifierHeight", &PyDbDetailViewStyle::setIdentifierHeight, py::arg("h"))
		.def("setIdentifierOffset", &PyDbDetailViewStyle::setIdentifierOffset, py::arg("v"))
		.def("setIdentifierPlacement", &PyDbDetailViewStyle::setIdentifierPlacement, py::arg("placement"))
		.def("setIdentifierStyleId", &PyDbDetailViewStyle::setIdentifierStyleId, py::arg("id"))
		;
}
