#include "stdafx.h"
#include "PyErrorStatusException.h"
#include "PyDbLeader.h"
#include "PyDbDimStyleTableRecord.h"

PyWrapper_Cpp(DbLeader)
AcString PyDbLeader::toString() const { return AcString(std::format("AcDbLeader[wrapper]").c_str(), AcString::Utf8); }
int PyDbLeader::numVertices() const { return impObj()->numVertices(); }

bool PyDbLeader::appendVertex(const AcGePoint3d& p) { return impObj()->appendVertex(p) == Adesk::kTrue; }

AcGePoint3d PyDbLeader::firstVertex() const { return impObj()->firstVertex(); }

AcGePoint3d PyDbLeader::lastVertex() const { return impObj()->lastVertex(); }

AcGePoint3d PyDbLeader::vertexAt(int i) const { return impObj()->vertexAt(i); }

bool PyDbLeader::setVertexAt(int i, const AcGePoint3d& p) { return impObj()->setVertexAt(i, p) == Adesk::kTrue; }

void PyDbLeader::removeLastVertex() { impObj()->removeLastVertex(); }
bool PyDbLeader::hasArrowHead() const { return impObj()->hasArrowHead() != Adesk::kFalse; }
void PyDbLeader::enableArrowHead() { impObj()->enableArrowHead(); }
void PyDbLeader::disableArrowHead() { impObj()->disableArrowHead(); }
void PyDbLeader::setHasArrowHead(bool enable) { impObj()->setHasArrowHead(enable ? Adesk::kTrue : Adesk::kFalse); }
bool PyDbLeader::hasHookLine() const { return impObj()->hasHookLine() != Adesk::kFalse; }
void PyDbLeader::setToSplineLeader() { impObj()->setToSplineLeader(); }
void PyDbLeader::setToStraightLeader() { impObj()->setToStraightLeader(); }
bool PyDbLeader::isSplined() const { return impObj()->isSplined() != Adesk::kFalse; }
void PyDbLeader::setSplined(bool b) { impObj()->setSplined(b ? Adesk::kTrue : Adesk::kFalse); }
uint64_t PyDbLeader::annotationObjId() const { return static_cast<uint64_t>(impObj()->annotationObjId().asOldId()); }
bool  PyDbLeader::detachAnnotation() { return PyWarnIfError(impObj()->detachAnnotation(), L"AcDbLeader::detachAnnotation"); }
bool  PyDbLeader::evaluateLeader() { return PyWarnIfError(impObj()->evaluateLeader(), L"AcDbLeader::evaluateLeader"); }
AcGeVector3d PyDbLeader::annotationOffset() const { return impObj()->annotationOffset(); }
bool  PyDbLeader::setAnnotationOffset(const AcGeVector3d& o) { return PyWarnIfError(impObj()->setAnnotationOffset(o), L"AcDbLeader::setAnnotationOffset"); }
int PyDbLeader::annoType() const { return static_cast<int>(impObj()->annoType()); }
double PyDbLeader::annoHeight() const { return impObj()->annoHeight(); }
double PyDbLeader::annoWidth() const { return impObj()->annoWidth(); }
double PyDbLeader::dimasz() const { return impObj()->dimasz(); }
bool  PyDbLeader::setDimasz(double v) { return PyWarnIfError(impObj()->setDimasz(v), L"AcDbLeader::setDimasz"); }
double PyDbLeader::dimgap() const { return impObj()->dimgap(); }
bool  PyDbLeader::setDimgap(double v) { return PyWarnIfError(impObj()->setDimgap(v), L"AcDbLeader::setDimgap"); }
double PyDbLeader::dimscale() const { return impObj()->dimscale(); }
bool  PyDbLeader::setDimscale(double v) { return PyWarnIfError(impObj()->setDimscale(v), L"AcDbLeader::setDimscale"); }
double PyDbLeader::dimtxt() const { return impObj()->dimtxt(); }
bool  PyDbLeader::setDimtxt(double v) { return PyWarnIfError(impObj()->setDimtxt(v), L"AcDbLeader::setDimtxt"); }
int PyDbLeader::dimtad() const { return impObj()->dimtad(); }
bool  PyDbLeader::setDimtad(int v) { return PyWarnIfError(impObj()->setDimtad(v), L"AcDbLeader::setDimtad"); }
bool PyDbLeader::dimsah() const { return impObj()->dimsah(); }
bool  PyDbLeader::setDimsah(bool v) { return PyWarnIfError(impObj()->setDimsah(v), L"AcDbLeader::setDimsah"); }

uint64_t PyDbLeader::dimensionStyle() const { auto id = impObj()->dimensionStyle(); return id.isNull() ? 0ULL : static_cast<uint64_t>(id.asOldId()); }
uint64_t PyDbLeader::dimldrblk() const { auto id = impObj()->dimldrblk(); return id.isNull() ? 0ULL : static_cast<uint64_t>(id.asOldId()); }
int PyDbLeader::dimlwd() const { return static_cast<int>(impObj()->dimlwd()); }
uint64_t PyDbLeader::dimtxsty() const { auto id = impObj()->dimtxsty(); return id.isNull() ? 0ULL : static_cast<uint64_t>(id.asOldId()); }
AcGeVector3d PyDbLeader::normal() const { return impObj()->normal(); }
bool  PyDbLeader::setAnnotationObjId(uint64_t annoId) { AcDbObjectId id; id.setFromOldId(static_cast<Adesk::UIntPtr>(annoId)); return PyWarnIfError(impObj()->setAnnotationObjId(id), L"AcDbLeader::setAnnotationObjId"); }
void PyDbLeader::setDimVars() { impObj()->setDimVars(); }
void PyDbLeader::setDimensionStyle(uint64_t styleId) { AcDbObjectId oid; oid.setFromOldId(static_cast<Adesk::UIntPtr>(styleId)); impObj()->setDimensionStyle(AcDbHardPointerId(oid)); }
bool  PyDbLeader::setDimldrblk(uint64_t blkId) { AcDbObjectId id; id.setFromOldId(static_cast<Adesk::UIntPtr>(blkId)); return PyWarnIfError(impObj()->setDimldrblk(id), L"AcDbLeader::setDimldrblk"); }
bool  PyDbLeader::setDimlwd(int lw) { return PyWarnIfError(impObj()->setDimlwd(static_cast<AcDb::LineWeight>(lw)), L"AcDbLeader::setDimlwd"); }
bool  PyDbLeader::setDimstyleData(uint64_t styleId) { AcDbObjectId id; id.setFromOldId(static_cast<Adesk::UIntPtr>(styleId)); return PyWarnIfError(impObj()->setDimstyleData(id), L"AcDbLeader::setDimstyleData"); }
bool  PyDbLeader::setDimtxsty(uint64_t txstyId) { AcDbObjectId id; id.setFromOldId(static_cast<Adesk::UIntPtr>(txstyId)); return PyWarnIfError(impObj()->setDimtxsty(id), L"AcDbLeader::setDimtxsty"); }
void PyDbLeader::setPlane(const AcGePlane& plane) { impObj()->setPlane(plane); }
PyDbDimStyleTableRecord* PyDbLeader::getDimstyleData() const {
	AcDbDimStyleTableRecord* pRec = nullptr;
	if (!PyWarnIfError(impObj()->getDimstyleData(pRec), L"AcDbLeader::getDimstyleData")) return nullptr;
	return new PyDbDimStyleTableRecord(pRec);
}

void bind_PyDbLeader(pym& m) {
	py::class_<PyDbLeader, PyDbCurve>(m, "PyDbLeader")
		.def(py::init<>())
		.def(py::init<AcDbLeader*>(), py::arg("object"))
		.def("numVertices", &PyDbLeader::numVertices)
		.def("appendVertex", &PyDbLeader::appendVertex, py::arg("pt"))
		.def("firstVertex", &PyDbLeader::firstVertex)
		.def("lastVertex", &PyDbLeader::lastVertex)
		.def("vertexAt", &PyDbLeader::vertexAt, py::arg("i"))
		.def("setVertexAt", &PyDbLeader::setVertexAt, py::arg("i"), py::arg("pt"))
		.def("removeLastVertex", &PyDbLeader::removeLastVertex)
		.def("hasArrowHead", &PyDbLeader::hasArrowHead)
		.def("enableArrowHead", &PyDbLeader::enableArrowHead)
		.def("disableArrowHead", &PyDbLeader::disableArrowHead)
		.def("setHasArrowHead", &PyDbLeader::setHasArrowHead, py::arg("enable"))
		.def("hasHookLine", &PyDbLeader::hasHookLine)
		.def("setToSplineLeader", &PyDbLeader::setToSplineLeader)
		.def("setToStraightLeader", &PyDbLeader::setToStraightLeader)
		.def("isSplined", &PyDbLeader::isSplined)
		.def("setSplined", &PyDbLeader::setSplined, py::arg("b"))
		.def("annotationObjId", &PyDbLeader::annotationObjId)
		.def("attachAnnotation", [](PyDbLeader& self, uint64_t annoId) {
			AcDbObjectId id;
			id.setFromOldId(static_cast<Adesk::UIntPtr>(annoId));
			return PyWarnIfError(self.impObj()->attachAnnotation(id), L"attachAnnotation");
		}, py::arg("anno_id"), "把注解实体 id 关联到引线")
		.def("detachAnnotation", &PyDbLeader::detachAnnotation)
		.def("evaluateLeader", &PyDbLeader::evaluateLeader)
		.def("annotationOffset", &PyDbLeader::annotationOffset)
		.def("setAnnotationOffset", &PyDbLeader::setAnnotationOffset, py::arg("offset"))
		.def("annoType", &PyDbLeader::annoType)
		.def("annoHeight", &PyDbLeader::annoHeight)
		.def("annoWidth", &PyDbLeader::annoWidth)
		.def("dimasz", &PyDbLeader::dimasz)
		.def("setDimasz", &PyDbLeader::setDimasz, py::arg("v"))
		.def("dimgap", &PyDbLeader::dimgap)
		.def("setDimgap", &PyDbLeader::setDimgap, py::arg("v"))
		.def("dimscale", &PyDbLeader::dimscale)
		.def("setDimscale", &PyDbLeader::setDimscale, py::arg("v"))
		.def("dimtxt", &PyDbLeader::dimtxt)
		.def("setDimtxt", &PyDbLeader::setDimtxt, py::arg("v"))
		.def("dimtad", &PyDbLeader::dimtad)
		.def("setDimtad", &PyDbLeader::setDimtad, py::arg("v"))
		.def("dimsah", &PyDbLeader::dimsah)
		.def("setDimsah", &PyDbLeader::setDimsah, py::arg("v"))
		.def("dimensionStyle", &PyDbLeader::dimensionStyle)
		.def("dimldrblk", &PyDbLeader::dimldrblk)
		.def("dimlwd", &PyDbLeader::dimlwd)
		.def("dimtxsty", &PyDbLeader::dimtxsty)
		.def("normal", &PyDbLeader::normal)
		.def("setAnnotationObjId", &PyDbLeader::setAnnotationObjId, py::arg("anno_id"))
		.def("setDimVars", &PyDbLeader::setDimVars)
		.def("setDimensionStyle", &PyDbLeader::setDimensionStyle, py::arg("style_id"))
		.def("setDimldrblk", &PyDbLeader::setDimldrblk, py::arg("blk_id"))
		.def("setDimlwd", &PyDbLeader::setDimlwd, py::arg("lw"))
		.def("setDimstyleData", &PyDbLeader::setDimstyleData, py::arg("style_id"))
		.def("setDimtxsty", &PyDbLeader::setDimtxsty, py::arg("txsty_id"))
		.def("setPlane", &PyDbLeader::setPlane, py::arg("plane"))
		.def("getDimstyleData", &PyDbLeader::getDimstyleData, py::return_value_policy::take_ownership, "取出副本样式记录（包装类接管，自动释放）")
		PyWrapper_BIND(DbLeader)
		.def("__repr__", &PyDbLeader::toString)
		;
}

