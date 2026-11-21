#include "stdafx.h"
#include "PyErrorStatusException.h"
#include "PyDbFaceRecord.h"

PyWrapper_Cpp(DbFaceRecord);
AcString PyDbFaceRecord::toString() const { return AcString(std::format("AcDbFaceRecord[wrapper]").c_str(), AcString::Utf8); }
int PyDbFaceRecord::getVertexAt(int faceIdx) const {
	if (!impObj()) return 0;
	Adesk::Int16 vtxIdx = 0;
	if (!PyWarnIfError(impObj()->getVertexAt((Adesk::UInt16)faceIdx, vtxIdx), L"AcDbFaceRecord::getVertexAt")) return 0;
	return vtxIdx;
}

bool PyDbFaceRecord::setVertexAt(int faceIdx, int vtxIdx) {
	if (!impObj()) return false;
	return PyWarnIfError(impObj()->setVertexAt((Adesk::UInt16)faceIdx, (Adesk::Int16)vtxIdx), L"AcDbFaceRecord::setVertexAt");
}

bool PyDbFaceRecord::isEdgeVisibleAt(int faceIndex) const {
	if (!impObj()) return false;
	Adesk::Boolean visible = Adesk::kFalse;
	if (!PyWarnIfError(impObj()->isEdgeVisibleAt((Adesk::UInt16)faceIndex, visible), L"AcDbFaceRecord::isEdgeVisibleAt")) return false;
	return visible != Adesk::kFalse;
}

bool PyDbFaceRecord::makeEdgeVisibleAt(int faceIndex) {
	if (!impObj()) return false;
	return PyWarnIfError(impObj()->makeEdgeVisibleAt((Adesk::UInt16)faceIndex), L"AcDbFaceRecord::makeEdgeVisibleAt");
}

bool PyDbFaceRecord::makeEdgeInvisibleAt(int faceIndex) {
	if (!impObj()) return false;
	return PyWarnIfError(impObj()->makeEdgeInvisibleAt((Adesk::UInt16)faceIndex), L"AcDbFaceRecord::makeEdgeInvisibleAt");
}

void bind_PyDbFaceRecord(pym& m) {
	py::class_<PyDbFaceRecord, PyDbVertex>(m, "PyDbFaceRecord")
		.def(py::init<>())
		.def(py::init<int, int, int, int>(), py::arg("v0"), py::arg("v1"), py::arg("v2"), py::arg("v3"))
		.def(py::init<AcDbFaceRecord*>(), py::arg("object"))
		.def("getVertexAt", &PyDbFaceRecord::getVertexAt, py::arg("faceIdx"))
		.def("setVertexAt", &PyDbFaceRecord::setVertexAt, py::arg("faceIdx"), py::arg("vtxIdx"))
		.def("isEdgeVisibleAt", &PyDbFaceRecord::isEdgeVisibleAt, py::arg("faceIndex"))
		.def("makeEdgeVisibleAt", &PyDbFaceRecord::makeEdgeVisibleAt, py::arg("faceIndex"))
		.def("makeEdgeInvisibleAt", &PyDbFaceRecord::makeEdgeInvisibleAt, py::arg("faceIndex"))
		PyWrapper_BIND(DbFaceRecord)
		.def("__repr__", &PyDbFaceRecord::toString)
		;
}
