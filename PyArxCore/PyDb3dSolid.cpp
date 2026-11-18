#include "stdafx.h"
#include "PyErrorStatusException.h"
#include "PyDb3dSolid.h"
#include "PyDbRegion.h"
#include "PyDbCurve.h"

PyWrapper_Cpp(Db3dSolid)
AcString PyDb3dSolid::toString() const { return AcString(std::format("AcDb3dSolid[wrapper]").c_str(), AcString::Utf8); }
bool PyDb3dSolid::isNull() const { if (!impObj()) return false; return impObj()->isNull() == Adesk::kTrue; }

bool PyDb3dSolid::createBox(double x, double y, double z)  { if (!impObj()) return false; return PyWarnIfError(impObj()->createBox(x, y, z), L"AcDb3dSolid::createBox"); }
bool PyDb3dSolid::createSphere(double r)  { if (!impObj()) return false; return PyWarnIfError(impObj()->createSphere(r), L"AcDb3dSolid::createSphere"); }
bool PyDb3dSolid::createTorus(double major, double minor)  { if (!impObj()) return false; return PyWarnIfError(impObj()->createTorus(major, minor), L"AcDb3dSolid::createTorus"); }
bool PyDb3dSolid::createWedge(double x, double y, double z)  { if (!impObj()) return false; return PyWarnIfError(impObj()->createWedge(x, y, z), L"AcDb3dSolid::createWedge"); }
bool PyDb3dSolid::createFrustum(double height, double xRadius, double yRadius, double topXRadius)  { if (!impObj()) return false; return PyWarnIfError(impObj()->createFrustum(height, xRadius, yRadius, topXRadius), L"AcDb3dSolid::createFrustum"); }
bool PyDb3dSolid::createPyramid(double height, int sides, double radius, double topRadius)  { if (!impObj()) return false; return PyWarnIfError(impObj()->createPyramid(height, sides, radius, topRadius), L"AcDb3dSolid::createPyramid"); }
bool PyDb3dSolid::booleanOper(int operation, PyDb3dSolid& other)  { if (!impObj()) return false; return PyWarnIfError(impObj()->booleanOper(static_cast<AcDb::BoolOperType>(operation), other.impObj()), L"AcDb3dSolid::booleanOper"); }
bool PyDb3dSolid::cleanBody()  { if (!impObj()) return false; return PyWarnIfError(impObj()->cleanBody(), L"AcDb3dSolid::cleanBody"); }
bool PyDb3dSolid::offsetBody(double d)  { if (!impObj()) return false; return PyWarnIfError(impObj()->offsetBody(d), L"AcDb3dSolid::offsetBody"); }
unsigned int PyDb3dSolid::numChanges() const { if (!impObj()) return 0; return impObj()->numChanges(); }
bool PyDb3dSolid::recordHistory() const { if (!impObj()) return false; return impObj()->recordHistory(); }
bool PyDb3dSolid::setRecordHistory(bool v)  { if (!impObj()) return false; return PyWarnIfError(impObj()->setRecordHistory(v), L"AcDb3dSolid::setRecordHistory"); }
bool PyDb3dSolid::showHistory() const { if (!impObj()) return false; return impObj()->showHistory(); }
bool PyDb3dSolid::setShowHistory(bool v)  { if (!impObj()) return false; return PyWarnIfError(impObj()->setShowHistory(v), L"AcDb3dSolid::setShowHistory"); }
double PyDb3dSolid::getArea() const { if (!impObj()) return 0.0; double v = 0.0; if(!PyWarnIfError(impObj()->getArea(v), L"AcDb3dSolid::getArea")) return 0.0; return v; }
double PyDb3dSolid::volume() const {
	if (!impObj()) return 0.0;
	double volume = 0.0; AcGePoint3d centroid; double mom[3] = {0}, prod[3] = {0}, prin[3] = {0}, rad[3] = {0}; AcGeVector3d axes[3]; AcDbExtents ext;
	if(!PyWarnIfError(impObj()->getMassProp(volume, centroid, mom, prod, prin, axes, rad, ext), L"AcDb3dSolid::getMassProp")) return 0.0;
	return volume;
}
bool PyDb3dSolid::createFrom(PyDbEntity& src)  { if (!impObj()) return false; return PyWarnIfError(impObj()->createFrom(src.impObj()), L"AcDb3dSolid::createFrom"); }
bool PyDb3dSolid::extrudeAlongPath(PyDbRegion& region, PyDbCurve& path, double taperAngle) {
	if (!impObj()) return false;
	return PyWarnIfError(impObj()->extrudeAlongPath(region.impObj(), path.impObj(), taperAngle), L"AcDb3dSolid::extrudeAlongPath");
}

void bind_PyDb3dSolid(pym& m) {
	py::class_<PyDb3dSolid, PyDbEntity>(m, "PyDb3dSolid")
		.def(py::init<>())
		.def(py::init<AcDb3dSolid*>(), py::arg("object"))
		.def("isNull", &PyDb3dSolid::isNull)
		.def("createBox", &PyDb3dSolid::createBox, py::arg("xLen"), py::arg("yLen"), py::arg("zLen"))
		.def("createSphere", &PyDb3dSolid::createSphere, py::arg("radius"))
		.def("createTorus", &PyDb3dSolid::createTorus, py::arg("majorRadius"), py::arg("minorRadius"))
		.def("createWedge", &PyDb3dSolid::createWedge, py::arg("xLen"), py::arg("yLen"), py::arg("zLen"))
		.def("createFrustum", &PyDb3dSolid::createFrustum, py::arg("height"), py::arg("xRadius"), py::arg("yRadius"), py::arg("topXRadius"))
		.def("createPyramid", &PyDb3dSolid::createPyramid, py::arg("height"), py::arg("sides"), py::arg("radius"), py::arg("topRadius") = 0.0)
		.def("booleanOper", &PyDb3dSolid::booleanOper, py::arg("operation"), py::arg("other"))
		.def("cleanBody", &PyDb3dSolid::cleanBody)
		.def("offsetBody", &PyDb3dSolid::offsetBody, py::arg("offsetDistance"))
		.def("numChanges", &PyDb3dSolid::numChanges)
		.def("recordHistory", &PyDb3dSolid::recordHistory)
		.def("setRecordHistory", &PyDb3dSolid::setRecordHistory, py::arg("v"))
		.def("showHistory", &PyDb3dSolid::showHistory)
		.def("setShowHistory", &PyDb3dSolid::setShowHistory, py::arg("v"))
		.def("getArea", &PyDb3dSolid::getArea)
		.def("volume", &PyDb3dSolid::volume)
		.def("createFrom", &PyDb3dSolid::createFrom, py::arg("src"))
		.def("extrudeAlongPath", &PyDb3dSolid::extrudeAlongPath, py::arg("region"), py::arg("path"), py::arg("taperAngle") = 0.0, "由 Region 沿曲线路径拉伸成实体")
		// ---- 从 Region 拉伸/旋转 ----
		.def("extrude", [](PyDb3dSolid& self, PyDbRegion& region, double height, double taper) {
	if (!self.impObj()) return false;
			return PyWarnIfError(self.impObj()->extrude(region.impObj(), height, taper), L"extrude");
		}, py::arg("region"), py::arg("height"), py::arg("taperAngle") = 0.0, "由 Region 沿法向拉伸成实体")
		.def("revolve", [](PyDb3dSolid& self, PyDbRegion& region, const AcGePoint3d& axisPt, const AcGeVector3d& axisDir, double angle) {
			return PyWarnIfError(self.impObj()->revolve(region.impObj(), axisPt, axisDir, angle), L"revolve");
		}, py::arg("region"), py::arg("axisPoint"), py::arg("axisDir"), py::arg("angle"), "由 Region 绕轴旋转成实体")
		PyWrapper_BIND(Db3dSolid)
		.def("__repr__", &PyDb3dSolid::toString)
		;
}

