#include "stdafx.h"
#include "PyErrorStatusException.h"
#include "PyDbSubDMesh.h"
#include "PyDbFullSubentPath.h"

PyWrapper_Cpp(DbSubDMesh)
AcString PyDbSubDMesh::toString() const { return AcString(std::format("AcDbSubDMesh[wrapper]").c_str(), AcString::Utf8); }
int PyDbSubDMesh::subdLevel() const { if (!impObj()) return 0; Adesk::Int32 r = 0; if(!PyWarnIfError(impObj()->subdLevel(r), L"AcDbSubDMesh::subdLevel")) return 0; return static_cast<int>(r); }
bool PyDbSubDMesh::isWatertight() const { if (!impObj()) return false; bool r = false; if(!PyWarnIfError(impObj()->isWatertight(r), L"AcDbSubDMesh::isWatertight")) return false; return r; }
int PyDbSubDMesh::numOfFaces() const { if (!impObj()) return 0; Adesk::Int32 r = 0; if(!PyWarnIfError(impObj()->numOfFaces(r), L"AcDbSubDMesh::numOfFaces")) return 0; return static_cast<int>(r); }
int PyDbSubDMesh::numOfVertices() const { if (!impObj()) return 0; Adesk::Int32 r = 0; if(!PyWarnIfError(impObj()->numOfVertices(r), L"AcDbSubDMesh::numOfVertices")) return 0; return static_cast<int>(r); }
int PyDbSubDMesh::numOfEdges() const { if (!impObj()) return 0; Adesk::Int32 r = 0; if(!PyWarnIfError(impObj()->numOfEdges(r), L"AcDbSubDMesh::numOfEdges")) return 0; return static_cast<int>(r); }
int PyDbSubDMesh::numOfSubDividedFaces() const { if (!impObj()) return 0; Adesk::Int32 r = 0; if(!PyWarnIfError(impObj()->numOfSubDividedFaces(r), L"AcDbSubDMesh::numOfSubDividedFaces")) return 0; return static_cast<int>(r); }
int PyDbSubDMesh::numOfSubDividedVertices() const { if (!impObj()) return 0; Adesk::Int32 r = 0; if(!PyWarnIfError(impObj()->numOfSubDividedVertices(r), L"AcDbSubDMesh::numOfSubDividedVertices")) return 0; return static_cast<int>(r); }
AcGePoint3d PyDbSubDMesh::getVertexAt(int i) const { if (!impObj()) return AcGePoint3d(); AcGePoint3d v; if(!PyWarnIfError(impObj()->getVertexAt(static_cast<Adesk::Int32>(i), v), L"AcDbSubDMesh::getVertexAt")) return AcGePoint3d(); return v; }
bool  PyDbSubDMesh::setVertexAt(int i, const AcGePoint3d& v) { if (!impObj()) return false; return PyWarnIfError(impObj()->setVertexAt(static_cast<Adesk::Int32>(i), v), L"AcDbSubDMesh::setVertexAt"); }
double PyDbSubDMesh::computeVolume() const { if (!impObj()) return 0.0; double v = 0.0; if(!PyWarnIfError(impObj()->computeVolume(v), L"AcDbSubDMesh::computeVolume")) return 0.0; return v; }
double PyDbSubDMesh::computeSurfaceArea() const { if (!impObj()) return 0.0; double v = 0.0; if(!PyWarnIfError(impObj()->computeSurfaceArea(v), L"AcDbSubDMesh::computeSurfaceArea")) return 0.0; return v; }
bool  PyDbSubDMesh::subdDivideUp() { if (!impObj()) return false; return PyWarnIfError(impObj()->subdDivideUp(), L"AcDbSubDMesh::subdDivideUp"); }
bool  PyDbSubDMesh::subdDivideDown() { if (!impObj()) return false; return PyWarnIfError(impObj()->subdDivideDown(), L"AcDbSubDMesh::subdDivideDown"); }
bool  PyDbSubDMesh::subdRefine() { if (!impObj()) return false; return PyWarnIfError(impObj()->subdRefine(), L"AcDbSubDMesh::subdRefine"); }
bool  PyDbSubDMesh::setSphere(double r, int da, int dh, int lvl) { if (!impObj()) return false; return PyWarnIfError(impObj()->setSphere(r, da, dh, lvl), L"AcDbSubDMesh::setSphere"); }
bool  PyDbSubDMesh::setBox(double x, double y, double z, int dx, int dy, int dz, int lvl) { if (!impObj()) return false; return PyWarnIfError(impObj()->setBox(x, y, z, dx, dy, dz, lvl), L"AcDbSubDMesh::setBox"); }
bool  PyDbSubDMesh::setTorus(double major, int ds, int dp, double ratio, double rot, int lvl) { if (!impObj()) return false; return PyWarnIfError(impObj()->setTorus(major, ds, dp, ratio, rot, lvl), L"AcDbSubDMesh::setTorus"); }
bool  PyDbSubDMesh::setCylinder(double major, double minor, double h, int da, int dh, int dc, int lvl) { if (!impObj()) return false; return PyWarnIfError(impObj()->setCylinder(major, minor, h, da, dh, dc, lvl), L"AcDbSubDMesh::setCylinder"); }
bool  PyDbSubDMesh::setCone(double major, double minor, double h, int da, int dh, int dc, double ratio, int lvl) { if (!impObj()) return false; return PyWarnIfError(impObj()->setCone(major, minor, h, da, dh, dc, ratio, lvl), L"AcDbSubDMesh::setCone"); }
bool  PyDbSubDMesh::setWedge(double x, double y, double z, int dl, int dw, int dh, int ds, int dc, int lvl) { if (!impObj()) return false; return PyWarnIfError(impObj()->setWedge(x, y, z, dl, dw, dh, ds, dc, lvl), L"AcDbSubDMesh::setWedge"); }
bool  PyDbSubDMesh::setPyramid(double radius, double h, int dl, int dh, int dc, int nSides, double ratio, int lvl) { if (!impObj()) return false; return PyWarnIfError(impObj()->setPyramid(radius, h, dl, dh, dc, nSides, ratio, lvl), L"AcDbSubDMesh::setPyramid"); }

bool PyDbSubDMesh::setSubDMesh(const AcGePoint3dArray& vertexArray, const AcArray<Adesk::Int32>& faceArray, int subDLevel) {
	if (!impObj()) return false;
	return PyWarnIfError(impObj()->setSubDMesh(vertexArray, faceArray, subDLevel), L"AcDbSubDMesh::setSubDMesh");
}
bool PyDbSubDMesh::splitFace(const PyDbSubentId& faceId, const PyDbSubentId& sub0, const AcGePoint3d& p0, const PyDbSubentId& sub1, const AcGePoint3d& p1) {
	if (!impObj()) return false;
	return PyWarnIfError(impObj()->splitFace(static_cast<AcDbSubentId>(faceId), static_cast<AcDbSubentId>(sub0), p0, static_cast<AcDbSubentId>(sub1), p1), L"AcDbSubDMesh::splitFace");
}
static AcDbFullSubentPathArray ToFspArray(const std::vector<PyDbFullSubentPath>& paths) {
	AcDbFullSubentPathArray arr;
	for (const auto& p : paths) arr.append(p.impObj());
	return arr;
}
bool PyDbSubDMesh::extrudeFaces(const std::vector<PyDbFullSubentPath>& paths, const AcGePoint3dArray& alongPath, double taper) {
	if (!impObj()) return false;
	return PyWarnIfError(impObj()->extrudeFaces(ToFspArray(paths), alongPath, taper), L"AcDbSubDMesh::extrudeFaces");
}
bool PyDbSubDMesh::extrudeConnectedFaces(const std::vector<PyDbFullSubentPath>& paths, const AcGePoint3dArray& alongPath, double taper) {
	if (!impObj()) return false;
	return PyWarnIfError(impObj()->extrudeConnectedFaces(ToFspArray(paths), alongPath, taper), L"AcDbSubDMesh::extrudeConnectedFaces");
}

void bind_PyDbSubDMesh(pym& m) {
	py::class_<PyDbSubDMesh, PyDbEntity>(m, "PyDbSubDMesh")
		.def(py::init<>())
		.def(py::init<AcDbSubDMesh*>(), py::arg("object"))
		.def("subdLevel", &PyDbSubDMesh::subdLevel)
		.def("isWatertight", &PyDbSubDMesh::isWatertight)
		.def("numOfFaces", &PyDbSubDMesh::numOfFaces)
		.def("numOfVertices", &PyDbSubDMesh::numOfVertices)
		.def("numOfEdges", &PyDbSubDMesh::numOfEdges)
		.def("numOfSubDividedFaces", &PyDbSubDMesh::numOfSubDividedFaces)
		.def("numOfSubDividedVertices", &PyDbSubDMesh::numOfSubDividedVertices)
		.def("getVertexAt", &PyDbSubDMesh::getVertexAt, py::arg("index"))
		.def("setVertexAt", &PyDbSubDMesh::setVertexAt, py::arg("index"), py::arg("v"))
		.def("computeVolume", &PyDbSubDMesh::computeVolume)
		.def("computeSurfaceArea", &PyDbSubDMesh::computeSurfaceArea)
		.def("subdDivideUp", &PyDbSubDMesh::subdDivideUp)
		.def("subdDivideDown", &PyDbSubDMesh::subdDivideDown)
		.def("subdRefine", &PyDbSubDMesh::subdRefine)
		.def("setSphere", &PyDbSubDMesh::setSphere, py::arg("radius"), py::arg("divAxis"), py::arg("divHeight"), py::arg("subDLevel"))
		.def("setBox", &PyDbSubDMesh::setBox, py::arg("xLen"), py::arg("yLen"), py::arg("zLen"), py::arg("divX"), py::arg("divY"), py::arg("divZ"), py::arg("subDLevel"))
		.def("setTorus", &PyDbSubDMesh::setTorus, py::arg("majorRadius"), py::arg("divSection"), py::arg("divSweepPath"), py::arg("sectionRadiusRatio"), py::arg("sectionRotate"), py::arg("subDLevel"))
		.def("setCylinder", &PyDbSubDMesh::setCylinder, py::arg("majorRadius"), py::arg("minorRadius"), py::arg("height"), py::arg("divAxis"), py::arg("divHeight"), py::arg("divCap"), py::arg("subDLevel"))
		.def("setCone", &PyDbSubDMesh::setCone, py::arg("majorRadius"), py::arg("minorRadius"), py::arg("height"), py::arg("divAxis"), py::arg("divHeight"), py::arg("divCap"), py::arg("radiusRatio"), py::arg("subDLevel"))
		.def("setWedge", &PyDbSubDMesh::setWedge, py::arg("xLen"), py::arg("yLen"), py::arg("zLen"), py::arg("divLength"), py::arg("divWidth"), py::arg("divHeight"), py::arg("divSlope"), py::arg("divCap"), py::arg("subDLevel"))
		.def("setPyramid", &PyDbSubDMesh::setPyramid, py::arg("radius"), py::arg("height"), py::arg("divLength"), py::arg("divHeight"), py::arg("divCap"), py::arg("nSides"), py::arg("radiusRatio"), py::arg("subDLevel"))
		.def("setSubDMesh", &PyDbSubDMesh::setSubDMesh, py::arg("vertexArray"), py::arg("faceArray"), py::arg("subDLevel"))
		.def("splitFace", &PyDbSubDMesh::splitFace, py::arg("faceId"), py::arg("sub0"), py::arg("point0"), py::arg("sub1"), py::arg("point1"))
		.def("extrudeFaces", &PyDbSubDMesh::extrudeFaces, py::arg("subentPaths"), py::arg("alongPath"), py::arg("taper"))
		.def("extrudeConnectedFaces", &PyDbSubDMesh::extrudeConnectedFaces, py::arg("subentPaths"), py::arg("alongPath"), py::arg("taper"))
		PyWrapper_BIND(DbSubDMesh)
		.def("__repr__", &PyDbSubDMesh::toString);
}
