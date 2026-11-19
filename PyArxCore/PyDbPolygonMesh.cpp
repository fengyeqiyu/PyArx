#include "stdafx.h"
#include "PyErrorStatusException.h"
#include "PyDbSequenceEnd.h"
#include "PyDbPolygonMesh.h"

PyWrapper_Cpp(DbPolygonMesh);

PyDbSequenceEnd PyDbPolygonMesh::openSequenceEnd(int mode) const {
	if (!impObj()) return PyDbSequenceEnd(nullptr);
	AcDbSequenceEnd* p = nullptr;
	if (!PyWarnIfError(impObj()->openSequenceEnd(p, static_cast<AcDb::OpenMode>(mode)),
		L"AcDbPolygonMesh::openSequenceEnd"))
		return PyDbSequenceEnd(nullptr);
	return PyDbSequenceEnd(p);
}

void bind_PyDbPolygonMesh(pym& m) {
	py::class_<PyDbPolygonMesh, PyDbEntity>(m, "PyDbPolygonMesh")
		.def(py::init<AcDbPolygonMesh*>(), py::arg("p"))
		PyWrapper_BIND(DbPolygonMesh)
		// ---- polyMeshType（AcDb::PolyMeshType：0=kSimpleMesh 5=kQuad 6=kCubic 8=kBezier）----
		.def("polyMeshType", [](PyDbPolygonMesh& s) {
			return static_cast<int>(s.impObj()->polyMeshType());
		})
		.def("setPolyMeshType", [](PyDbPolygonMesh& s, int v) {
			return PyWarnIfError(s.impObj()->setPolyMeshType(static_cast<AcDb::PolyMeshType>(v)),
				L"AcDbPolygonMesh::setPolyMeshType");
		}, py::arg("v"))
		.def("convertToPolyMeshType", [](PyDbPolygonMesh& s, int v) {
			return PyWarnIfError(s.impObj()->convertToPolyMeshType(static_cast<AcDb::PolyMeshType>(v)),
				L"AcDbPolygonMesh::convertToPolyMeshType");
		}, py::arg("v"))
		// ---- mSize / nSize ----
		.def("mSize", [](PyDbPolygonMesh& s) { return static_cast<int>(s.impObj()->mSize()); })
		.def("setMSize", [](PyDbPolygonMesh& s, int v) {
			return PyWarnIfError(s.impObj()->setMSize(static_cast<Adesk::Int16>(v)),
				L"AcDbPolygonMesh::setMSize");
		}, py::arg("v"))
		.def("nSize", [](PyDbPolygonMesh& s) { return static_cast<int>(s.impObj()->nSize()); })
		.def("setNSize", [](PyDbPolygonMesh& s, int v) {
			return PyWarnIfError(s.impObj()->setNSize(static_cast<Adesk::Int16>(v)),
				L"AcDbPolygonMesh::setNSize");
		}, py::arg("v"))
		// ---- M/N 闭合 ----
		.def("isMClosed", [](PyDbPolygonMesh& s) {
			if (s.isNull()) return false;
			return s.impObj()->isMClosed() != Adesk::kFalse;
		})
		.def("makeMClosed", [](PyDbPolygonMesh& s) {
			return PyWarnIfError(s.impObj()->makeMClosed(), L"AcDbPolygonMesh::makeMClosed");
		})
		.def("makeMOpen", [](PyDbPolygonMesh& s) {
			return PyWarnIfError(s.impObj()->makeMOpen(), L"AcDbPolygonMesh::makeMOpen");
		})
		.def("isNClosed", [](PyDbPolygonMesh& s) {
			if (s.isNull()) return false;
			return s.impObj()->isNClosed() != Adesk::kFalse;
		})
		.def("makeNClosed", [](PyDbPolygonMesh& s) {
			return PyWarnIfError(s.impObj()->makeNClosed(), L"AcDbPolygonMesh::makeNClosed");
		})
		.def("makeNOpen", [](PyDbPolygonMesh& s) {
			return PyWarnIfError(s.impObj()->makeNOpen(), L"AcDbPolygonMesh::makeNOpen");
		})
		// ---- 曲面密度 ----
		.def("mSurfaceDensity", [](PyDbPolygonMesh& s) { return static_cast<int>(s.impObj()->mSurfaceDensity()); })
		.def("setMSurfaceDensity", [](PyDbPolygonMesh& s, int v) {
			return PyWarnIfError(s.impObj()->setMSurfaceDensity(static_cast<Adesk::Int16>(v)),
				L"AcDbPolygonMesh::setMSurfaceDensity");
		}, py::arg("v"))
		.def("nSurfaceDensity", [](PyDbPolygonMesh& s) { return static_cast<int>(s.impObj()->nSurfaceDensity()); })
		.def("setNSurfaceDensity", [](PyDbPolygonMesh& s, int v) {
			return PyWarnIfError(s.impObj()->setNSurfaceDensity(static_cast<Adesk::Int16>(v)),
				L"AcDbPolygonMesh::setNSurfaceDensity");
		}, py::arg("v"))
		// ---- straighten / surfaceFit ----
		.def("straighten", [](PyDbPolygonMesh& s) {
			return PyWarnIfError(s.impObj()->straighten(), L"AcDbPolygonMesh::straighten");
		})
		.def("surfaceFit", [](PyDbPolygonMesh& s, int surfType, int surfu, int surfv) {
			return PyWarnIfError(s.impObj()->surfaceFit(static_cast<AcDb::PolyMeshType>(surfType),
				static_cast<Adesk::Int16>(surfu), static_cast<Adesk::Int16>(surfv)),
				L"AcDbPolygonMesh::surfaceFit");
		}, py::arg("surfType"), py::arg("surfu"), py::arg("surfv"))
		// ---- openSequenceEnd（返回 PyDbSequenceEnd 包装，内部接管）----
		.def("openSequenceEnd", &PyDbPolygonMesh::openSequenceEnd, py::arg("mode"))
		;
}
