#include "stdafx.h"
#include "PyErrorStatusException.h"
#include "PyDbSequenceEnd.h"
#include "PyDbPolyFaceMesh.h"

PyWrapper_Cpp(DbPolyFaceMesh);

PyDbSequenceEnd PyDbPolyFaceMesh::openSequenceEnd(int mode) const {
	AcDbSequenceEnd* p = nullptr;
	if (!PyWarnIfError(impObj()->openSequenceEnd(p, static_cast<AcDb::OpenMode>(mode)),
		L"AcDbPolyFaceMesh::openSequenceEnd"))
		return PyDbSequenceEnd(nullptr);
	return PyDbSequenceEnd(p);
}

void bind_PyDbPolyFaceMesh(pym& m) {
	py::class_<PyDbPolyFaceMesh, PyDbEntity>(m, "PyDbPolyFaceMesh")
		.def(py::init<AcDbPolyFaceMesh*>(), py::arg("p"))
		PyWrapper_BIND(DbPolyFaceMesh)
		// ---- numVertices / numFaces ----
		.def("numVertices", [](PyDbPolyFaceMesh& s) { return static_cast<int>(s.impObj()->numVertices()); })
		.def("numFaces", [](PyDbPolyFaceMesh& s) { return static_cast<int>(s.impObj()->numFaces()); })
		// ---- openSequenceEnd（返回 PyDbSequenceEnd 包装，内部接管）----
		.def("openSequenceEnd", &PyDbPolyFaceMesh::openSequenceEnd, py::arg("mode"))
		;
}
