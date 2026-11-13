#include "stdafx.h"
#include "PyErrorStatusException.h"
#include "PyDb2dPolyline.h"
#include "PyDb2dVertex.h"

PyWrapper_Cpp(Db2dPolyline)
AcString PyDb2dPolyline::toString() const { return AcString(std::format("AcDb2dPolyline[wrapper]").c_str(), AcString::Utf8); }
bool PyDb2dPolyline::isClosed() const { return impObj()->isClosed() == Adesk::kTrue; }

bool  PyDb2dPolyline::setClosed(bool c) { return PyWarnIfError(impObj()->setClosed(c ? Adesk::kTrue : Adesk::kFalse), L"AcDb2dPolyline::setClosed"); }

double PyDb2dPolyline::elevation() const { return impObj()->elevation(); }

bool  PyDb2dPolyline::setElevation(double e) { return PyWarnIfError(impObj()->setElevation(e), L"AcDb2dPolyline::setElevation"); }

unsigned int PyDb2dPolyline::numVerts() const { AcDbObjectIterator* it = impObj()->vertexIterator(); unsigned int n = 0; if (it) { for (; !it->done(); it->step()) ++n; delete it; } return n; }

int PyDb2dPolyline::polyType() const { return static_cast<int>(impObj()->polyType()); }
bool  PyDb2dPolyline::setPolyType(int type) { return PyWarnIfError(impObj()->setPolyType(static_cast<AcDb::Poly2dType>(type)), L"AcDb2dPolyline::setPolyType"); }
bool  PyDb2dPolyline::convertToPolyType(int newVal) { return PyWarnIfError(impObj()->convertToPolyType(static_cast<AcDb::Poly2dType>(newVal)), L"AcDb2dPolyline::convertToPolyType"); }
bool  PyDb2dPolyline::makeClosed() { return PyWarnIfError(impObj()->makeClosed(), L"AcDb2dPolyline::makeClosed"); }
bool  PyDb2dPolyline::makeOpen() { return PyWarnIfError(impObj()->makeOpen(), L"AcDb2dPolyline::makeOpen"); }
double PyDb2dPolyline::constantWidth() const { double v = 0.0; if(!PyWarnIfError(impObj()->constantWidth(v), L"AcDb2dPolyline::constantWidth")) return 0.0; return v; }
bool  PyDb2dPolyline::setConstantWidth(double w) { return PyWarnIfError(impObj()->setConstantWidth(w), L"AcDb2dPolyline::setConstantWidth"); }
double PyDb2dPolyline::length() const { double v = 0.0; if(!PyWarnIfError(impObj()->length(v), L"AcDb2dPolyline::length")) return 0.0; return v; }
double PyDb2dPolyline::defaultStartWidth() const { return impObj()->defaultStartWidth(); }
bool  PyDb2dPolyline::setDefaultStartWidth(double w) { return PyWarnIfError(impObj()->setDefaultStartWidth(w), L"AcDb2dPolyline::setDefaultStartWidth"); }
double PyDb2dPolyline::defaultEndWidth() const { return impObj()->defaultEndWidth(); }
bool  PyDb2dPolyline::setDefaultEndWidth(double w) { return PyWarnIfError(impObj()->setDefaultEndWidth(w), L"AcDb2dPolyline::setDefaultEndWidth"); }
double PyDb2dPolyline::thickness() const { return impObj()->thickness(); }
bool  PyDb2dPolyline::setThickness(double t) { return PyWarnIfError(impObj()->setThickness(t), L"AcDb2dPolyline::setThickness"); }
AcGeVector3d PyDb2dPolyline::normal() const { return impObj()->normal(); }
bool  PyDb2dPolyline::setNormal(const AcGeVector3d& n) { return PyWarnIfError(impObj()->setNormal(n), L"AcDb2dPolyline::setNormal"); }
bool PyDb2dPolyline::isLinetypeGenerationOn() const { return impObj()->isLinetypeGenerationOn() != Adesk::kFalse; }
bool  PyDb2dPolyline::setLinetypeGenerationOn() { return PyWarnIfError(impObj()->setLinetypeGenerationOn(), L"AcDb2dPolyline::setLinetypeGenerationOn"); }
bool  PyDb2dPolyline::setLinetypeGenerationOff() { return PyWarnIfError(impObj()->setLinetypeGenerationOff(), L"AcDb2dPolyline::setLinetypeGenerationOff"); }
bool  PyDb2dPolyline::setLinetypeGenerationOn(bool on) { return PyWarnIfError(impObj()->setLinetypeGenerationOn(on ? Adesk::kTrue : Adesk::kFalse), L"AcDb2dPolyline::setLinetypeGenerationOn"); }
bool  PyDb2dPolyline::straighten() { return PyWarnIfError(impObj()->straighten(), L"AcDb2dPolyline::straighten"); }
bool  PyDb2dPolyline::splineFit() { return PyWarnIfError(impObj()->splineFit(), L"AcDb2dPolyline::splineFit"); }
bool  PyDb2dPolyline::splineFit(int splineType, int splineSegs) { return PyWarnIfError(impObj()->splineFit(static_cast<AcDb::Poly2dType>(splineType), static_cast<Adesk::Int16>(splineSegs)), L"AcDb2dPolyline::splineFit"); }
bool  PyDb2dPolyline::curveFit() { return PyWarnIfError(impObj()->curveFit(), L"AcDb2dPolyline::curveFit"); }
bool  PyDb2dPolyline::makeClosedIfStartAndEndVertexCoincide(double distTol) {
	return PyWarnIfError(impObj()->makeClosedIfStartAndEndVertexCoincide(distTol), L"AcDb2dPolyline::makeClosedIfStartAndEndVertexCoincide");
}

// 追加顶点：SDK 接管顶点对象指针，调用方包装不可再析构删除该顶点
bool PyDb2dPolyline::appendVertex(PyDb2dVertex& vertex) {
	return PyWarnIfError(impObj()->appendVertex(vertex.impObj()), L"AcDb2dPolyline::appendVertex");
}
uint64_t PyDb2dPolyline::appendVertexId(PyDb2dVertex& vertex) {
	AcDbObjectId outId;
	if (!PyWarnIfError(impObj()->appendVertex(outId, vertex.impObj()), L"AcDb2dPolyline::appendVertexId")) return 0ULL;
	return outId.isNull() ? 0ULL : static_cast<uint64_t>(outId.asOldId());
}
bool PyDb2dPolyline::insertVertexAt(const PyDb2dVertex& indexVertex, PyDb2dVertex& newVertex) {
	return PyWarnIfError(impObj()->insertVertexAt(indexVertex.impObj(), newVertex.impObj()), L"AcDb2dPolyline::insertVertexAt");
}
uint64_t PyDb2dPolyline::insertVertexAtById(uint64_t indexVertexId, PyDb2dVertex& newVertex) {
	AcDbObjectId indexId;
	indexId.setFromOldId(static_cast<Adesk::IntDbId>(indexVertexId));
	AcDbObjectId newId;
	if (!PyWarnIfError(impObj()->insertVertexAt(newId, indexId, newVertex.impObj()), L"AcDb2dPolyline::insertVertexAtById")) return 0ULL;
	return newId.isNull() ? 0ULL : static_cast<uint64_t>(newId.asOldId());
}

void bind_PyDb2dPolyline(pym& m) {
	py::class_<PyDb2dPolyline, PyDbCurve>(m, "PyDbPolyline2d")
		.def(py::init<>())
		.def(py::init<AcDb2dPolyline*>(), py::arg("object"))
		.def("numVerts", &PyDb2dPolyline::numVerts)
		.def("isClosed", &PyDb2dPolyline::isClosed)
		.def("setClosed", &PyDb2dPolyline::setClosed, py::arg("closed"))
		.def("elevation", &PyDb2dPolyline::elevation)
		.def("setElevation", &PyDb2dPolyline::setElevation, py::arg("e"))
		.def("polyType", &PyDb2dPolyline::polyType)
		.def("setPolyType", &PyDb2dPolyline::setPolyType, py::arg("type"))
		.def("convertToPolyType", &PyDb2dPolyline::convertToPolyType, py::arg("newVal"))
		.def("makeClosed", &PyDb2dPolyline::makeClosed)
		.def("makeOpen", &PyDb2dPolyline::makeOpen)
		.def("constantWidth", &PyDb2dPolyline::constantWidth)
		.def("setConstantWidth", &PyDb2dPolyline::setConstantWidth, py::arg("w"))
		.def("length", &PyDb2dPolyline::length)
		.def("defaultStartWidth", &PyDb2dPolyline::defaultStartWidth)
		.def("setDefaultStartWidth", &PyDb2dPolyline::setDefaultStartWidth, py::arg("w"))
		.def("defaultEndWidth", &PyDb2dPolyline::defaultEndWidth)
		.def("setDefaultEndWidth", &PyDb2dPolyline::setDefaultEndWidth, py::arg("w"))
		.def("thickness", &PyDb2dPolyline::thickness)
		.def("setThickness", &PyDb2dPolyline::setThickness, py::arg("t"))
		.def("normal", &PyDb2dPolyline::normal)
		.def("setNormal", &PyDb2dPolyline::setNormal, py::arg("n"))
		.def("isLinetypeGenerationOn", &PyDb2dPolyline::isLinetypeGenerationOn)
		.def("setLinetypeGenerationOn", py::overload_cast<>(&PyDb2dPolyline::setLinetypeGenerationOn))
		.def("setLinetypeGenerationOff", &PyDb2dPolyline::setLinetypeGenerationOff)
		.def("setLinetypeGenerationOn", py::overload_cast<bool>(&PyDb2dPolyline::setLinetypeGenerationOn), py::arg("on"))
		.def("straighten", &PyDb2dPolyline::straighten)
		.def("splineFit", py::overload_cast<>(&PyDb2dPolyline::splineFit))
		.def("splineFit", py::overload_cast<int, int>(&PyDb2dPolyline::splineFit), py::arg("splineType"), py::arg("splineSegs"))
		.def("curveFit", &PyDb2dPolyline::curveFit)
		.def("makeClosedIfStartAndEndVertexCoincide", &PyDb2dPolyline::makeClosedIfStartAndEndVertexCoincide, py::arg("distTol"),
			"若首尾顶点距离小于 distTol 则将多段线闭合")
		.def("appendVertex", &PyDb2dPolyline::appendVertex, py::arg("vertex"),
			"追加顶点（顶点交由多段线接管，包装不可再删除该顶点）")
		.def("appendVertexId", &PyDb2dPolyline::appendVertexId, py::arg("vertex"),
			"追加顶点并返回新顶点 ObjectId(oldId)，失败返回 0")
		.def("insertVertexAt", &PyDb2dPolyline::insertVertexAt, py::arg("indexVertex"), py::arg("newVertex"),
			"在 indexVertex 之前插入 newVertex")
		.def("insertVertexAtById", &PyDb2dPolyline::insertVertexAtById, py::arg("indexVertexId"), py::arg("newVertex"),
			"按 indexVertexId(oldId) 在其前插入 newVertex，返回新顶点 ObjectId(oldId)，失败返回 0")
		PyWrapper_BIND(Db2dPolyline)
		.def("__repr__", &PyDb2dPolyline::toString)
		;
}

