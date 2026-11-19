#include "stdafx.h"
#include "PyErrorStatusException.h"
#include "PyDbLinetypeTableRecord.h"

PyWrapper_Cpp(DbLinetypeTableRecord);

void bind_PyDbLinetypeTableRecord(pym& m) {
	py::class_<PyDbLinetypeTableRecord, PyDbSymbolTableRecord>(m, "PyDbLinetypeTableRecord")
		.def(py::init<>())
		.def(py::init<AcDbLinetypeTableRecord*>(), py::arg("p"))
		PyWrapper_BIND(DbLinetypeTableRecord)
		// ---- comments / asciiDescription（asciiDescription 为 comments 的内联别名） ----
		.def("comments", [](PyDbLinetypeTableRecord& s) -> AcString {
			AcString sOut;
			if (!PyWarnIfError(s.impObj()->comments(sOut), L"AcDbLinetypeTableRecord::comments")) return AcString();
			return sOut;
		})
		.def("setComments", [](PyDbLinetypeTableRecord& s, const AcString& v) {
			return PyWarnIfError(s.impObj()->setComments(v.kwszPtr()), L"AcDbLinetypeTableRecord::setComments");
		}, py::arg("v"))
		.def("asciiDescription", [](PyDbLinetypeTableRecord& s) -> AcString {
			AcString sOut;
			if (!PyWarnIfError(s.impObj()->comments(sOut), L"AcDbLinetypeTableRecord::asciiDescription")) return AcString();
			return sOut;
		})
		.def("setAsciiDescription", [](PyDbLinetypeTableRecord& s, const AcString& v) {
			return PyWarnIfError(s.impObj()->setComments(v.kwszPtr()), L"AcDbLinetypeTableRecord::setAsciiDescription");
		}, py::arg("v"))
		// ---- patternLength ----
		.def("patternLength", [](PyDbLinetypeTableRecord& s) {
			if (s.isNull()) return 0.0;
			return s.impObj()->patternLength();
		})
		.def("setPatternLength", [](PyDbLinetypeTableRecord& s, double v) {
			return PyWarnIfError(s.impObj()->setPatternLength(v), L"AcDbLinetypeTableRecord::setPatternLength");
		}, py::arg("v"))
		// ---- numDashes ----
		.def("numDashes", [](PyDbLinetypeTableRecord& s) {
			if (s.isNull()) return 0;
			return s.impObj()->numDashes();
		})
		.def("setNumDashes", [](PyDbLinetypeTableRecord& s, int v) {
			return PyWarnIfError(s.impObj()->setNumDashes(v), L"AcDbLinetypeTableRecord::setNumDashes");
		}, py::arg("v"))
		// ---- dashLengthAt ----
		.def("dashLengthAt", [](PyDbLinetypeTableRecord& s, int index) { return s.impObj()->dashLengthAt(index); }, py::arg("index"))
		.def("setDashLengthAt", [](PyDbLinetypeTableRecord& s, int index, double v) {
			return PyWarnIfError(s.impObj()->setDashLengthAt(index, v), L"AcDbLinetypeTableRecord::setDashLengthAt");
		}, py::arg("index"), py::arg("v"))
		// ---- shapeStyleAt（ObjectId 透出 uint64） ----
		.def("shapeStyleAt", [](PyDbLinetypeTableRecord& s, int index) -> uint64_t {
			auto id = s.impObj()->shapeStyleAt(index);
			return id.isNull() ? 0ULL : static_cast<uint64_t>(id.asOldId());
		}, py::arg("index"))
		.def("setShapeStyleAt", [](PyDbLinetypeTableRecord& s, int index, uint64_t idv) {
			AcDbObjectId id; id.setFromOldId(static_cast<Adesk::UIntPtr>(idv));
			return PyWarnIfError(s.impObj()->setShapeStyleAt(index, id), L"AcDbLinetypeTableRecord::setShapeStyleAt");
		}, py::arg("index"), py::arg("id"))
		// ---- shapeNumberAt ----
		.def("shapeNumberAt", [](PyDbLinetypeTableRecord& s, int index) { return s.impObj()->shapeNumberAt(index); }, py::arg("index"))
		.def("setShapeNumberAt", [](PyDbLinetypeTableRecord& s, int index, int v) {
			return PyWarnIfError(s.impObj()->setShapeNumberAt(index, v), L"AcDbLinetypeTableRecord::setShapeNumberAt");
		}, py::arg("index"), py::arg("v"))
		// ---- shapeOffsetAt（AcGeVector2d） ----
		.def("shapeOffsetAt", [](PyDbLinetypeTableRecord& s, int index) { return s.impObj()->shapeOffsetAt(index); }, py::arg("index"))
		.def("setShapeOffsetAt", [](PyDbLinetypeTableRecord& s, int index, const AcGeVector2d& v) {
			return PyWarnIfError(s.impObj()->setShapeOffsetAt(index, v), L"AcDbLinetypeTableRecord::setShapeOffsetAt");
		}, py::arg("index"), py::arg("v"))
		// ---- shapeScaleAt ----
		.def("shapeScaleAt", [](PyDbLinetypeTableRecord& s, int index) { return s.impObj()->shapeScaleAt(index); }, py::arg("index"))
		.def("setShapeScaleAt", [](PyDbLinetypeTableRecord& s, int index, double v) {
			return PyWarnIfError(s.impObj()->setShapeScaleAt(index, v), L"AcDbLinetypeTableRecord::setShapeScaleAt");
		}, py::arg("index"), py::arg("v"))
		// ---- isScaledToFit ----
		.def("isScaledToFit", [](PyDbLinetypeTableRecord& s) {
			if (s.isNull()) return false;
			return s.impObj()->isScaledToFit();
		})
		.def("setIsScaledToFit", [](PyDbLinetypeTableRecord& s, bool v) {
			s.impObj()->setIsScaledToFit(v); return true;
		}, py::arg("v"))
		// ---- shapeIsUcsOrientedAt ----
		.def("shapeIsUcsOrientedAt", [](PyDbLinetypeTableRecord& s, int index) { return s.impObj()->shapeIsUcsOrientedAt(index); }, py::arg("index"))
		.def("setShapeIsUcsOrientedAt", [](PyDbLinetypeTableRecord& s, int index, bool v) {
			return PyWarnIfError(s.impObj()->setShapeIsUcsOrientedAt(index, v), L"AcDbLinetypeTableRecord::setShapeIsUcsOrientedAt");
		}, py::arg("index"), py::arg("v"))
		// ---- shapeRotationAt ----
		.def("shapeRotationAt", [](PyDbLinetypeTableRecord& s, int index) { return s.impObj()->shapeRotationAt(index); }, py::arg("index"))
		.def("setShapeRotationAt", [](PyDbLinetypeTableRecord& s, int index, double v) {
			return PyWarnIfError(s.impObj()->setShapeRotationAt(index, v), L"AcDbLinetypeTableRecord::setShapeRotationAt");
		}, py::arg("index"), py::arg("v"))
		// ---- textAt ----
		.def("textAt", [](PyDbLinetypeTableRecord& s, int index) -> AcString {
			AcString sOut;
			if (!PyWarnIfError(s.impObj()->textAt(index, sOut), L"AcDbLinetypeTableRecord::textAt")) return AcString();
			return sOut;
		}, py::arg("index"))
		.def("setTextAt", [](PyDbLinetypeTableRecord& s, int index, const AcString& v) {
			return PyWarnIfError(s.impObj()->setTextAt(index, v.kwszPtr()), L"AcDbLinetypeTableRecord::setTextAt");
		}, py::arg("index"), py::arg("v"))
		;
}
