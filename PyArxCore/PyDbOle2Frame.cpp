#include "stdafx.h"
#include "PyErrorStatusException.h"
#include "PyDbOle2Frame.h"

PyWrapper_Cpp(DbOle2Frame)
AcString PyDbOle2Frame::toString() const { return AcString(std::format("AcDbOle2Frame[wrapper]").c_str(), AcString::Utf8); }
AcGePoint3d PyDbOle2Frame::location() const { if (!impObj()) return AcGePoint3d(); AcGePoint3d pt; impObj()->getLocation(pt); return pt; }

void PyDbOle2Frame::setLocation(const AcGePoint3d& p) { if (!impObj()) return; impObj()->setLocation(p); }

std::tuple<AcGePoint3d, AcGePoint3d, AcGePoint3d, AcGePoint3d> PyDbOle2Frame::position3d() const {
	if (!impObj()) return {};
	CRectangle3d r{};
	impObj()->position(r);
	return std::make_tuple(r.upLeft, r.upRight, r.lowLeft, r.lowRight);
}

void PyDbOle2Frame::setPosition3d(const AcGePoint3d& upLeft, const AcGePoint3d& upRight,
	const AcGePoint3d& lowLeft, const AcGePoint3d& lowRight) {
	CRectangle3d r{};
	r.upLeft = upLeft; r.upRight = upRight;
	r.lowLeft = lowLeft; r.lowRight = lowRight;
	impObj()->setPosition(r);
}

double PyDbOle2Frame::rotation() const { if (!impObj()) return 0.0; return impObj()->rotation(); }

bool  PyDbOle2Frame::setRotation(double r) { if (!impObj()) return false; return PyWarnIfError(impObj()->setRotation(r), L"AcDbOle2Frame::setRotation"); }

int PyDbOle2Frame::getType() const { if (!impObj()) return 0; return impObj()->getType(); }

AcString PyDbOle2Frame::getUserType() const { if (!impObj()) return AcString(); AcString s; if(!PyWarnIfError(impObj()->getUserType(s), L"AcDbOle2Frame::getUserType")) return AcString(); return s; }
AcString PyDbOle2Frame::getLinkName() const { if (!impObj()) return AcString(); AcString s; if(!PyWarnIfError(impObj()->getLinkName(s), L"AcDbOle2Frame::getLinkName")) return AcString(); return s; }
AcString PyDbOle2Frame::getLinkPath() const { if (!impObj()) return AcString(); AcString s; if(!PyWarnIfError(impObj()->getLinkPath(s), L"AcDbOle2Frame::getLinkPath")) return AcString(); return s; }
bool PyDbOle2Frame::isLinked() const { if (!impObj()) return false; return impObj()->isLinked(); }
int PyDbOle2Frame::outputQuality() const { if (!impObj()) return 0; return impObj()->outputQuality(); }
void PyDbOle2Frame::setOutputQuality(int v) { if (!impObj()) return; impObj()->setOutputQuality(static_cast<Adesk::UInt8>(v)); }
bool PyDbOle2Frame::autoOutputQuality() const { if (!impObj()) return false; return impObj()->autoOutputQuality() != Adesk::kFalse; }
void PyDbOle2Frame::setAutoOutputQuality(int v) { if (!impObj()) return; impObj()->setAutoOutputQuality(static_cast<Adesk::UInt8>(v)); }
double PyDbOle2Frame::wcsWidth() const { if (!impObj()) return 0.0; return impObj()->wcsWidth(); }
bool  PyDbOle2Frame::setWcsWidth(double v) { if (!impObj()) return false; return PyWarnIfError(impObj()->setWcsWidth(v), L"AcDbOle2Frame::setWcsWidth"); }
double PyDbOle2Frame::wcsHeight() const { if (!impObj()) return 0.0; return impObj()->wcsHeight(); }
bool  PyDbOle2Frame::setWcsHeight(double v) { if (!impObj()) return false; return PyWarnIfError(impObj()->setWcsHeight(v), L"AcDbOle2Frame::setWcsHeight"); }
double PyDbOle2Frame::scaleWidth() const { if (!impObj()) return 0.0; return impObj()->scaleWidth(); }
bool  PyDbOle2Frame::setScaleWidth(double v) { if (!impObj()) return false; return PyWarnIfError(impObj()->setScaleWidth(v), L"AcDbOle2Frame::setScaleWidth"); }
double PyDbOle2Frame::scaleHeight() const { if (!impObj()) return 0.0; return impObj()->scaleHeight(); }
bool  PyDbOle2Frame::setScaleHeight(double v) { if (!impObj()) return false; return PyWarnIfError(impObj()->setScaleHeight(v), L"AcDbOle2Frame::setScaleHeight"); }
bool PyDbOle2Frame::lockAspect() const { if (!impObj()) return false; return impObj()->lockAspect() != Adesk::kFalse; }
bool  PyDbOle2Frame::setLockAspect(bool v) { if (!impObj()) return false; return PyWarnIfError(impObj()->setLockAspect(v ? Adesk::kTrue : Adesk::kFalse), L"AcDbOle2Frame::setLockAspect"); }

void bind_PyDbOle2Frame(pym& m) {
	py::class_<PyDbOle2Frame, PyDbEntity>(m, "PyDbOle2Frame")
		.def(py::init<>())
		.def(py::init<AcDbOle2Frame*>(), py::arg("object"))
		.def("location", &PyDbOle2Frame::location)
		.def("setLocation", &PyDbOle2Frame::setLocation, py::arg("p"))
		.def("position3d", &PyDbOle2Frame::position3d,
			"返回 OLE 对象四角世界坐标 (upLeft, upRight, lowLeft, lowRight)。")
		.def("setPosition3d", &PyDbOle2Frame::setPosition3d,
			py::arg("upLeft"), py::arg("upRight"), py::arg("lowLeft"), py::arg("lowRight"),
			"按世界坐标四角设置 OLE 对象位置。")
		.def("rotation", &PyDbOle2Frame::rotation)
		.def("setRotation", &PyDbOle2Frame::setRotation, py::arg("r"))
		.def("getType", &PyDbOle2Frame::getType)
		.def("getUserType", &PyDbOle2Frame::getUserType)
		.def("getLinkName", &PyDbOle2Frame::getLinkName)
		.def("getLinkPath", &PyDbOle2Frame::getLinkPath)
		.def("isLinked", &PyDbOle2Frame::isLinked)
		.def("outputQuality", &PyDbOle2Frame::outputQuality)
		.def("setOutputQuality", &PyDbOle2Frame::setOutputQuality, py::arg("v"))
		.def("autoOutputQuality", &PyDbOle2Frame::autoOutputQuality)
		.def("setAutoOutputQuality", &PyDbOle2Frame::setAutoOutputQuality, py::arg("v"))
		.def("wcsWidth", &PyDbOle2Frame::wcsWidth)
		.def("setWcsWidth", &PyDbOle2Frame::setWcsWidth, py::arg("v"))
		.def("wcsHeight", &PyDbOle2Frame::wcsHeight)
		.def("setWcsHeight", &PyDbOle2Frame::setWcsHeight, py::arg("v"))
		.def("scaleWidth", &PyDbOle2Frame::scaleWidth)
		.def("setScaleWidth", &PyDbOle2Frame::setScaleWidth, py::arg("v"))
		.def("scaleHeight", &PyDbOle2Frame::scaleHeight)
		.def("setScaleHeight", &PyDbOle2Frame::setScaleHeight, py::arg("v"))
		.def("lockAspect", &PyDbOle2Frame::lockAspect)
		.def("setLockAspect", &PyDbOle2Frame::setLockAspect, py::arg("v"))
		PyWrapper_BIND(DbOle2Frame)
		.def("__repr__", &PyDbOle2Frame::toString)
		;
}

