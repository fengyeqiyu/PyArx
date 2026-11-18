#include "stdafx.h"
#include "PyErrorStatusException.h"
#include "PyDbMline.h"

PyWrapper_Cpp(DbMline)
AcString PyDbMline::toString() const { return AcString(std::format("AcDbMline[wrapper]").c_str(), AcString::Utf8); }
double PyDbMline::scale() const { if (!impObj()) return 0.0; return impObj()->scale(); }

bool  PyDbMline::setScale(double s) { if (!impObj()) return false; return PyWarnIfError(impObj()->setScale(s), L"AcDbMline::setScale"); }

int PyDbMline::numVertices() const { if (!impObj()) return 0; return impObj()->numVertices(); }

AcGePoint3d PyDbMline::vertexAt(int i) const { if (!impObj()) return AcGePoint3d(); return impObj()->vertexAt(i); }

bool PyDbMline::closedMline() const { if (!impObj()) return false; return impObj()->closedMline(); }

// SDK 原型为 void（非 ErrorStatus），无状态可回传；按包装约定恒返回 true。
bool PyDbMline::setClosedMline(bool c) { if (!impObj()) return false; impObj()->setClosedMline(c); return true; }

int PyDbMline::justification() const { if (!impObj()) return 0; return static_cast<int>(impObj()->justification()); }
bool  PyDbMline::setJustification(int v) { if (!impObj()) return false; return PyWarnIfError(impObj()->setJustification(static_cast<Mline::MlineJustification>(v)), L"AcDbMline::setJustification"); }
AcGeVector3d PyDbMline::normal() const { if (!impObj()) return AcGeVector3d(); return impObj()->normal(); }
bool  PyDbMline::setNormal(const AcGeVector3d& v) { if (!impObj()) return false; return PyWarnIfError(impObj()->setNormal(v), L"AcDbMline::setNormal"); }
bool  PyDbMline::appendSeg(const AcGePoint3d& p) { if (!impObj()) return false; return PyWarnIfError(impObj()->appendSeg(p), L"AcDbMline::appendSeg"); }
bool  PyDbMline::moveVertexAt(int i, const AcGePoint3d& p) { if (!impObj()) return false; return PyWarnIfError(impObj()->moveVertexAt(i, p), L"AcDbMline::moveVertexAt"); }
bool PyDbMline::supressStartCaps() const { if (!impObj()) return false; return impObj()->supressStartCaps(); }
bool PyDbMline::setSupressStartCaps(bool v) { if (!impObj()) return false; impObj()->setSupressStartCaps(v); return true; }
bool PyDbMline::supressEndCaps() const { if (!impObj()) return false; return impObj()->supressEndCaps(); }
bool PyDbMline::setSupressEndCaps(bool v) { if (!impObj()) return false; impObj()->setSupressEndCaps(v); return true; }
int PyDbMline::element(const AcGePoint3d& p) const { if (!impObj()) return 0; return impObj()->element(p); }
AcGeVector3d PyDbMline::axisAt(int i) const { if (!impObj()) return AcGeVector3d(); return impObj()->axisAt(i); }
AcGeVector3d PyDbMline::miterAt(int i) const { if (!impObj()) return AcGeVector3d(); return impObj()->miterAt(i); }

AcGePoint3d PyDbMline::getClosestPointTo(const AcGePoint3d& givenPoint, bool extend, bool excludeCaps) const {
	if (!impObj()) return AcGePoint3d();
	AcGePoint3d pt;
	if (!PyWarnIfError(impObj()->getClosestPointTo(givenPoint, pt, extend, excludeCaps), L"AcDbMline::getClosestPointTo"))
		return AcGePoint3d();
	return pt;
}
AcGePoint3d PyDbMline::getClosestPointTo(const AcGePoint3d& givenPoint, const AcGeVector3d& normal, bool extend, bool excludeCaps) const {
	if (!impObj()) return AcGePoint3d();
	AcGePoint3d pt;
	if (!PyWarnIfError(impObj()->getClosestPointTo(givenPoint, normal, pt, extend, excludeCaps), L"AcDbMline::getClosestPointTo"))
		return AcGePoint3d();
	return pt;
}
AcGePlane PyDbMline::getPlane() const { AcGePlane pl; impObj()->getPlane(pl); return pl; }
AcGePoint3d PyDbMline::removeLastSeg() {
	if (!impObj()) return AcGePoint3d();
	AcGePoint3d last;
	if (!PyWarnIfError(impObj()->removeLastSeg(last), L"AcDbMline::removeLastSeg")) return AcGePoint3d();
	return last;
}
uint64_t PyDbMline::style() const { if (!impObj()) return 0ULL; auto id = impObj()->style(); return id.isNull() ? 0ULL : static_cast<uint64_t>(id.asOldId()); }
bool PyDbMline::setStyle(uint64_t styleId) {
	if (!impObj()) return false;
	AcDbObjectId id; id.setFromOldId(static_cast<Adesk::IntDbId>(styleId));
	return PyWarnIfError(impObj()->setStyle(id), L"AcDbMline::setStyle");
}

void bind_PyDbMline(pym& m) {
	py::class_<PyDbMline, PyDbEntity>(m, "PyDbMline")
		.def(py::init<>())
		.def(py::init<AcDbMline*>(), py::arg("object"))
		.def("scale", &PyDbMline::scale)
		.def("setScale", &PyDbMline::setScale, py::arg("s"))
		.def("numVertices", &PyDbMline::numVertices)
		.def("vertexAt", &PyDbMline::vertexAt, py::arg("index"))
		.def("closedMline", &PyDbMline::closedMline)
		.def("setClosedMline", &PyDbMline::setClosedMline, py::arg("c"))
		.def("justification", &PyDbMline::justification)
		.def("setJustification", &PyDbMline::setJustification, py::arg("v"))
		.def("normal", &PyDbMline::normal)
		.def("setNormal", &PyDbMline::setNormal, py::arg("v"))
		.def("appendSeg", &PyDbMline::appendSeg, py::arg("p"))
		.def("moveVertexAt", &PyDbMline::moveVertexAt, py::arg("index"), py::arg("p"))
		.def("supressStartCaps", &PyDbMline::supressStartCaps)
		.def("setSupressStartCaps", &PyDbMline::setSupressStartCaps, py::arg("v"))
		.def("supressEndCaps", &PyDbMline::supressEndCaps)
		.def("setSupressEndCaps", &PyDbMline::setSupressEndCaps, py::arg("v"))
		.def("element", &PyDbMline::element, py::arg("pt"))
		.def("axisAt", &PyDbMline::axisAt, py::arg("index"))
		.def("miterAt", &PyDbMline::miterAt, py::arg("index"))
		.def("getClosestPointTo", py::overload_cast<const AcGePoint3d&, bool, bool>(&PyDbMline::getClosestPointTo, py::const_),
			py::arg("givenPoint"), py::arg("extend"), py::arg("excludeCaps") = false)
		.def("getClosestPointTo", py::overload_cast<const AcGePoint3d&, const AcGeVector3d&, bool, bool>(&PyDbMline::getClosestPointTo, py::const_),
			py::arg("givenPoint"), py::arg("normal"), py::arg("extend"), py::arg("excludeCaps") = false)
		.def("getPlane", &PyDbMline::getPlane)
		.def("removeLastSeg", &PyDbMline::removeLastSeg, "删除最后一段，返回新的末点；失败返回默认点")
		.def("style", &PyDbMline::style, "返回当前 MlineStyle 的 ObjectId (uint64)，空为 0")
		.def("setStyle", &PyDbMline::setStyle, py::arg("styleId"), "设置 MlineStyle 的 ObjectId (uint64)")
		PyWrapper_BIND(DbMline)
		.def("__repr__", &PyDbMline::toString)
		;
}

