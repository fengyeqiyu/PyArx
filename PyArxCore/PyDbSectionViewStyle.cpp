#include "stdafx.h"
#include "PyErrorStatusException.h"
#include "PyDbSectionViewStyle.h"

PyWrapper_Cpp(DbSectionViewStyle);

uint64_t PyDbSectionViewStyle::arrowEndSymbolId() const {
	if (!impObj()) return 0ULL;
	AcDbObjectId id = impObj()->arrowEndSymbolId();
	return id.isNull() ? 0ULL : static_cast<uint64_t>(id.asOldId());
}
uint64_t PyDbSectionViewStyle::arrowStartSymbolId() const {
	if (!impObj()) return 0ULL;
	AcDbObjectId id = impObj()->arrowStartSymbolId();
	return id.isNull() ? 0ULL : static_cast<uint64_t>(id.asOldId());
}
std::tuple<int,int,int> PyDbSectionViewStyle::arrowSymbolColor() const {
	if (!impObj()) return {};
	AcCmColor c = impObj()->arrowSymbolColor();
	return std::make_tuple((int)c.red(), (int)c.green(), (int)c.blue());
}
double PyDbSectionViewStyle::arrowSymbolSize() const { if (!impObj()) return 0.0; return impObj()->arrowSymbolSize(); }

std::tuple<int,int,int> PyDbSectionViewStyle::identifierColor() const {
	if (!impObj()) return {};
	AcCmColor c = impObj()->identifierColor();
	return std::make_tuple((int)c.red(), (int)c.green(), (int)c.blue());
}
double PyDbSectionViewStyle::identifierHeight() const { if (!impObj()) return 0.0; return impObj()->identifierHeight(); }
uint64_t PyDbSectionViewStyle::identifierStyleId() const {
	if (!impObj()) return 0ULL;
	AcDbObjectId id = impObj()->identifierStyleId();
	return id.isNull() ? 0ULL : static_cast<uint64_t>(id.asOldId());
}

bool PyDbSectionViewStyle::setArrowEndSymbolId(uint64_t idv) {
	if (!impObj()) return false;
	AcDbObjectId id; id.setFromOldId(static_cast<Adesk::UIntPtr>(idv));
	return PyWarnIfError(impObj()->setArrowEndSymbolId(id), L"AcDbSectionViewStyle::setArrowEndSymbolId");
}
bool PyDbSectionViewStyle::setArrowStartSymbolId(uint64_t idv) {
	if (!impObj()) return false;
	AcDbObjectId id; id.setFromOldId(static_cast<Adesk::UIntPtr>(idv));
	return PyWarnIfError(impObj()->setArrowStartSymbolId(id), L"AcDbSectionViewStyle::setArrowStartSymbolId");
}
bool PyDbSectionViewStyle::setArrowSymbolColor(int r, int g, int b) {
	if (!impObj()) return false;
	AcCmColor c; c.setRGB((Adesk::UInt8)r, (Adesk::UInt8)g, (Adesk::UInt8)b);
	return PyWarnIfError(impObj()->setArrowSymbolColor(c), L"AcDbSectionViewStyle::setArrowSymbolColor");
}
bool PyDbSectionViewStyle::setArrowSymbolSize(double size) {
	if (!impObj()) return false;
	return PyWarnIfError(impObj()->setArrowSymbolSize(size), L"AcDbSectionViewStyle::setArrowSymbolSize");
}
bool PyDbSectionViewStyle::setIdentifierColor(int r, int g, int b) {
	if (!impObj()) return false;
	AcCmColor c; c.setRGB((Adesk::UInt8)r, (Adesk::UInt8)g, (Adesk::UInt8)b);
	return PyWarnIfError(impObj()->setIdentifierColor(c), L"AcDbSectionViewStyle::setIdentifierColor");
}
bool PyDbSectionViewStyle::setIdentifierExcludeCharacters(const AcString& s) {
	if (!impObj()) return false;
	return PyWarnIfError(impObj()->setIdentifierExcludeCharacters(s.kwszPtr()), L"AcDbSectionViewStyle::setIdentifierExcludeCharacters");
}
bool PyDbSectionViewStyle::setIdentifierHeight(double h) {
	if (!impObj()) return false;
	return PyWarnIfError(impObj()->setIdentifierHeight(h), L"AcDbSectionViewStyle::setIdentifierHeight");
}
bool PyDbSectionViewStyle::setIdentifierStyleId(uint64_t idv) {
	if (!impObj()) return false;
	AcDbObjectId id; id.setFromOldId(static_cast<Adesk::UIntPtr>(idv));
	return PyWarnIfError(impObj()->setIdentifierStyleId(id), L"AcDbSectionViewStyle::setIdentifierStyleId");
}

void bind_PyDbSectionViewStyle(pym& m) {
	py::class_<PyDbSectionViewStyle, PyDbModelDocViewStyle>(m, "PyDbSectionViewStyle")
		.def(py::init<>())
		.def(py::init<AcDbSectionViewStyle*>(), py::arg("p"))
		PyWrapper_BIND(DbSectionViewStyle)
		.def("arrowEndSymbolId", &PyDbSectionViewStyle::arrowEndSymbolId)
		.def("arrowStartSymbolId", &PyDbSectionViewStyle::arrowStartSymbolId)
		.def("arrowSymbolColor", &PyDbSectionViewStyle::arrowSymbolColor, "返回箭头符号色 (r,g,b)")
		.def("arrowSymbolSize", &PyDbSectionViewStyle::arrowSymbolSize)
		.def("identifierColor", &PyDbSectionViewStyle::identifierColor, "返回标识色 (r,g,b)")
		.def("identifierHeight", &PyDbSectionViewStyle::identifierHeight)
		.def("identifierStyleId", &PyDbSectionViewStyle::identifierStyleId)
		.def("setArrowEndSymbolId", &PyDbSectionViewStyle::setArrowEndSymbolId, py::arg("id"))
		.def("setArrowStartSymbolId", &PyDbSectionViewStyle::setArrowStartSymbolId, py::arg("id"))
		.def("setArrowSymbolColor", &PyDbSectionViewStyle::setArrowSymbolColor, py::arg("r"), py::arg("g"), py::arg("b"))
		.def("setArrowSymbolSize", &PyDbSectionViewStyle::setArrowSymbolSize, py::arg("size"))
		.def("setIdentifierColor", &PyDbSectionViewStyle::setIdentifierColor, py::arg("r"), py::arg("g"), py::arg("b"))
		.def("setIdentifierExcludeCharacters", &PyDbSectionViewStyle::setIdentifierExcludeCharacters, py::arg("s"))
		.def("setIdentifierHeight", &PyDbSectionViewStyle::setIdentifierHeight, py::arg("h"))
		.def("setIdentifierStyleId", &PyDbSectionViewStyle::setIdentifierStyleId, py::arg("id"))
		;
}
