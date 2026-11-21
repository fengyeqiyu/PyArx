#include "stdafx.h"
#include "PyErrorStatusException.h"
#include "PyDb3dPolyline.h"
#include "PyDb3dPolylineVertex.h"

PyWrapper_Cpp(Db3dPolyline)
AcString PyDb3dPolyline::toString() const { return AcString(std::format("AcDb3dPolyline[wrapper]").c_str(), AcString::Utf8); }
bool PyDb3dPolyline::setClosed(bool c)  { if (!impObj()) return false; return PyWarnIfError(impObj()->setClosed(c ? Adesk::kTrue : Adesk::kFalse), L"AcDb3dPolyline::setClosed"); }

unsigned int PyDb3dPolyline::numVerts() const { if (!impObj()) return 0; AcDbObjectIterator* it = impObj()->vertexIterator(); unsigned int n = 0; if (it) { for (; !it->done(); it->step()) ++n; delete it; } return n; }

double PyDb3dPolyline::length() const { if (!impObj()) return 0.0; double v = 0.0; if(!PyWarnIfError(impObj()->length(v), L"AcDb3dPolyline::length")) return 0.0; return v; }
int PyDb3dPolyline::polyType() const { if (!impObj()) return 0; return static_cast<int>(impObj()->polyType()); }
bool PyDb3dPolyline::setPolyType(int type)  { if (!impObj()) return false; return PyWarnIfError(impObj()->setPolyType(static_cast<AcDb::Poly3dType>(type)), L"AcDb3dPolyline::setPolyType"); }
bool PyDb3dPolyline::convertToPolyType(int newVal)  { if (!impObj()) return false; return PyWarnIfError(impObj()->convertToPolyType(static_cast<AcDb::Poly3dType>(newVal)), L"AcDb3dPolyline::convertToPolyType"); }
bool PyDb3dPolyline::makeClosed()  { if (!impObj()) return false; return PyWarnIfError(impObj()->makeClosed(), L"AcDb3dPolyline::makeClosed"); }
bool PyDb3dPolyline::makeOpen()  { if (!impObj()) return false; return PyWarnIfError(impObj()->makeOpen(), L"AcDb3dPolyline::makeOpen"); }
bool PyDb3dPolyline::straighten()  { if (!impObj()) return false; return PyWarnIfError(impObj()->straighten(), L"AcDb3dPolyline::straighten"); }
bool PyDb3dPolyline::splineFit()  { if (!impObj()) return false; return PyWarnIfError(impObj()->splineFit(), L"AcDb3dPolyline::splineFit"); }
bool PyDb3dPolyline::splineFit(int splineType, int splineSegs)  { if (!impObj()) return false; return PyWarnIfError(impObj()->splineFit(static_cast<AcDb::Poly3dType>(splineType), static_cast<Adesk::Int16>(splineSegs)), L"AcDb3dPolyline::splineFit"); }

// 追加顶点：SDK 接管顶点对象指针，调用方包装不可再析构删除该顶点
bool PyDb3dPolyline::appendVertex(PyDb3dPolylineVertex& vertex) {
	if (!impObj()) return false;
	return PyWarnIfError(impObj()->appendVertex(vertex.impObj()), L"AcDb3dPolyline::appendVertex");
}
uint64_t PyDb3dPolyline::appendVertexId(PyDb3dPolylineVertex& vertex) {
	if (!impObj()) return 0ULL;
	AcDbObjectId outId;
	if (!PyWarnIfError(impObj()->appendVertex(outId, vertex.impObj()), L"AcDb3dPolyline::appendVertexId")) return 0ULL;
	return outId.isNull() ? 0ULL : static_cast<uint64_t>(outId.asOldId());
}
bool PyDb3dPolyline::insertVertexAt(const PyDb3dPolylineVertex& indexVertex, PyDb3dPolylineVertex& newVertex) {
	if (!impObj()) return false;
	return PyWarnIfError(impObj()->insertVertexAt(indexVertex.impObj(), newVertex.impObj()), L"AcDb3dPolyline::insertVertexAt");
}
uint64_t PyDb3dPolyline::insertVertexAtById(uint64_t indexVertexId, PyDb3dPolylineVertex& newVertex) {
	if (!impObj()) return 0ULL;
	AcDbObjectId indexId;
	indexId.setFromOldId(static_cast<Adesk::IntDbId>(indexVertexId));
	AcDbObjectId newId;
	if (!PyWarnIfError(impObj()->insertVertexAt(newId, indexId, newVertex.impObj()), L"AcDb3dPolyline::insertVertexAtById")) return 0ULL;
	return newId.isNull() ? 0ULL : static_cast<uint64_t>(newId.asOldId());
}

void bind_PyDb3dPolyline(pym& m) {
	py::class_<PyDb3dPolyline, PyDbCurve>(m, "PyDbPolyline3d")
		.def(py::init<>())
		.def(py::init<AcDb3dPolyline*>(), py::arg("object"))
		.def("numVerts", &PyDb3dPolyline::numVerts)
		.def("setClosed", &PyDb3dPolyline::setClosed, py::arg("closed"))
		.def("length", &PyDb3dPolyline::length)
		.def("polyType", &PyDb3dPolyline::polyType)
		.def("setPolyType", &PyDb3dPolyline::setPolyType, py::arg("type"))
		.def("convertToPolyType", &PyDb3dPolyline::convertToPolyType, py::arg("newVal"))
		.def("makeClosed", &PyDb3dPolyline::makeClosed)
		.def("makeOpen", &PyDb3dPolyline::makeOpen)
		.def("straighten", &PyDb3dPolyline::straighten)
		.def("splineFit", py::overload_cast<>(&PyDb3dPolyline::splineFit))
		.def("splineFit", py::overload_cast<int, int>(&PyDb3dPolyline::splineFit), py::arg("splineType"), py::arg("splineSegs"))
		.def("appendVertex", &PyDb3dPolyline::appendVertex, py::arg("vertex"),
			"追加顶点（顶点交由多段线接管，包装不可再删除该顶点）")
		.def("appendVertexId", &PyDb3dPolyline::appendVertexId, py::arg("vertex"),
			"追加顶点并返回新顶点 ObjectId(oldId)，失败返回 0")
		.def("insertVertexAt", &PyDb3dPolyline::insertVertexAt, py::arg("indexVertex"), py::arg("newVertex"),
			"在 indexVertex 之前插入 newVertex")
		.def("insertVertexAtById", &PyDb3dPolyline::insertVertexAtById, py::arg("indexVertexId"), py::arg("newVertex"),
			"按 indexVertexId(oldId) 在其前插入 newVertex，返回新顶点 ObjectId(oldId)，失败返回 0")
		PyWrapper_BIND(Db3dPolyline)
		.def("__repr__", &PyDb3dPolyline::toString)
		;
}

