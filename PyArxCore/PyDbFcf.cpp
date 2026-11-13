#include "stdafx.h"
#include "PyErrorStatusException.h"
#include "PyDbFcf.h"
#include "PyDbDimStyleTableRecord.h"

PyWrapper_Cpp(DbFcf)
AcString PyDbFcf::toString() const { return AcString(std::format("AcDbFcf[wrapper]").c_str(), AcString::Utf8); }
AcString PyDbFcf::text(int lineNo) const { AcString s; if(!PyWarnIfError(impObj()->text(s, lineNo), L"AcDbFcf::text")) return AcString(); return s; }

bool PyDbFcf::setText(const AcString& s) { impObj()->setText(s.kwszPtr()); return true; }
AcGePoint3d PyDbFcf::location() const { return impObj()->location(); }
bool PyDbFcf::setLocation(const AcGePoint3d& p) { impObj()->setLocation(p); return true; }
AcGeVector3d PyDbFcf::normal() const { return impObj()->normal(); }
AcGeVector3d PyDbFcf::direction() const { return impObj()->direction(); }
bool PyDbFcf::setOrientation(const AcGeVector3d& n, const AcGeVector3d& d) { impObj()->setOrientation(n, d); return true; }
double PyDbFcf::dimgap() const { return impObj()->dimgap(); }
bool  PyDbFcf::setDimgap(double v) { return PyWarnIfError(impObj()->setDimgap(v), L"AcDbFcf::setDimgap"); }
double PyDbFcf::dimscale() const { return impObj()->dimscale(); }
bool  PyDbFcf::setDimscale(double v) { return PyWarnIfError(impObj()->setDimscale(v), L"AcDbFcf::setDimscale"); }
double PyDbFcf::dimtxt() const { return impObj()->dimtxt(); }
bool  PyDbFcf::setDimtxt(double v) { return PyWarnIfError(impObj()->setDimtxt(v), L"AcDbFcf::setDimtxt"); }
bool PyDbFcf::setDimVars() { impObj()->setDimVars(); return true; }
uint64_t PyDbFcf::dimensionStyle() const { auto id = impObj()->dimensionStyle(); return id.isNull() ? 0ULL : static_cast<uint64_t>(id.asOldId()); }
uint64_t PyDbFcf::dimtxsty() const { auto id = impObj()->dimtxsty(); return id.isNull() ? 0ULL : static_cast<uint64_t>(id.asOldId()); }
AcGePoint3dArray PyDbFcf::getBoundingPline() const { AcGePoint3dArray pts; impObj()->getBoundingPline(pts); return pts; }
AcGePoint3dArray PyDbFcf::getBoundingPoints() const { AcGePoint3dArray pts; impObj()->getBoundingPoints(pts); return pts; }
bool PyDbFcf::setDimensionStyle(uint64_t styleId) { AcDbObjectId oid; oid.setFromOldId(static_cast<Adesk::UIntPtr>(styleId)); impObj()->setDimensionStyle(AcDbHardPointerId(oid)); return true; }
bool  PyDbFcf::setDimstyleData(uint64_t styleId) { AcDbObjectId id; id.setFromOldId(static_cast<Adesk::UIntPtr>(styleId)); return PyWarnIfError(impObj()->setDimstyleData(id), L"AcDbFcf::setDimstyleData"); }
bool  PyDbFcf::setDimtxsty(uint64_t txstyId) { AcDbObjectId id; id.setFromOldId(static_cast<Adesk::UIntPtr>(txstyId)); return PyWarnIfError(impObj()->setDimtxsty(id), L"AcDbFcf::setDimtxsty"); }
PyDbDimStyleTableRecord* PyDbFcf::getDimstyleData() const {
	AcDbDimStyleTableRecord* pRec = nullptr;
	if (!PyWarnIfError(impObj()->getDimstyleData(pRec), L"AcDbFcf::getDimstyleData")) return nullptr;
	return new PyDbDimStyleTableRecord(pRec);
}

void bind_PyDbFcf(pym& m) {
	py::class_<PyDbFcf, PyDbEntity>(m, "PyDbFcf")
		.def(py::init<>())
		.def(py::init<AcDbFcf*>(), py::arg("object"))
		.def("text", &PyDbFcf::text, py::arg("lineNo") = -1)
		.def("setText", &PyDbFcf::setText, py::arg("s"))
		.def("location", &PyDbFcf::location)
		.def("setLocation", &PyDbFcf::setLocation, py::arg("p"))
		.def("normal", &PyDbFcf::normal)
		.def("direction", &PyDbFcf::direction)
		.def("setOrientation", &PyDbFcf::setOrientation, py::arg("norm"), py::arg("dir"))
		.def("dimgap", &PyDbFcf::dimgap)
		.def("setDimgap", &PyDbFcf::setDimgap, py::arg("v"))
		.def("dimscale", &PyDbFcf::dimscale)
		.def("setDimscale", &PyDbFcf::setDimscale, py::arg("v"))
		.def("dimtxt", &PyDbFcf::dimtxt)
		.def("setDimtxt", &PyDbFcf::setDimtxt, py::arg("v"))
		.def("setDimVars", &PyDbFcf::setDimVars)
		.def("dimensionStyle", &PyDbFcf::dimensionStyle)
		.def("dimtxsty", &PyDbFcf::dimtxsty)
		.def("getBoundingPline", &PyDbFcf::getBoundingPline)
		.def("getBoundingPoints", &PyDbFcf::getBoundingPoints)
		.def("setDimensionStyle", &PyDbFcf::setDimensionStyle, py::arg("style_id"))
		.def("setDimstyleData", &PyDbFcf::setDimstyleData, py::arg("style_id"))
		.def("setDimtxsty", &PyDbFcf::setDimtxsty, py::arg("txsty_id"))
		.def("getDimstyleData", &PyDbFcf::getDimstyleData, py::return_value_policy::take_ownership, "取出副本样式记录（包装类接管，自动释放）")
		PyWrapper_BIND(DbFcf)
		.def("__repr__", &PyDbFcf::toString)
		;
}

