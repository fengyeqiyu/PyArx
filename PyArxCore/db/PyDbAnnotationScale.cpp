#include "stdafx.h"
#include "PyErrorStatusException.h"
#include "PyDbAnnotationScale.h"

AcString PyDbAnnotationScale::name() const {
	if (!m_ptr) return AcString();
	AcString s;
	if (!PyWarnIfError(m_ptr->getName(s), L"AnnotationScale.getName")) return AcString();
	return s;
}

bool PyDbAnnotationScale::setName(const AcString& v) {
	return m_ptr ? PyWarnIfError(m_ptr->setName(v.kwszPtr()), L"AnnotationScale.setName") : false;
}

intptr_t PyDbAnnotationScale::uniqueIdentifier() const {
	return m_ptr ? static_cast<intptr_t>(m_ptr->uniqueIdentifier()) : 0;
}

AcString PyDbAnnotationScale::collectionName() const {
	return m_ptr ? m_ptr->collectionName() : AcString();
}

double PyDbAnnotationScale::paperUnits() const {
	if (!m_ptr) return 0.0;
	double v = 0.0;
	PyWarnIfError(m_ptr->getPaperUnits(v), L"AnnotationScale.getPaperUnits");
	return v;
}

bool PyDbAnnotationScale::setPaperUnits(double v) {
	return m_ptr ? PyWarnIfError(m_ptr->setPaperUnits(v), L"AnnotationScale.setPaperUnits") : false;
}

double PyDbAnnotationScale::drawingUnits() const {
	if (!m_ptr) return 0.0;
	double v = 0.0;
	PyWarnIfError(m_ptr->getDrawingUnits(v), L"AnnotationScale.getDrawingUnits");
	return v;
}

bool PyDbAnnotationScale::setDrawingUnits(double v) {
	return m_ptr ? PyWarnIfError(m_ptr->setDrawingUnits(v), L"AnnotationScale.setDrawingUnits") : false;
}

double PyDbAnnotationScale::scale() const {
	if (!m_ptr) return 0.0;
	double v = 0.0;
	PyWarnIfError(m_ptr->getScale(v), L"AnnotationScale.getScale");
	return v;
}

bool PyDbAnnotationScale::isTemporaryScale() const {
	if (!m_ptr) return false;
	bool b = false;
	PyWarnIfError(m_ptr->getIsTemporaryScale(b), L"AnnotationScale.getIsTemporaryScale");
	return b;
}

bool PyDbAnnotationScale::matchScaleId(intptr_t id) const {
	return m_ptr && m_ptr->matchScaleId(static_cast<Adesk::LongPtr>(id));
}

AcString PyDbAnnotationScale::toString() const {
	return AcString(std::format("PyDbAnnotationScale[id={}]", static_cast<long long>(uniqueIdentifier())).c_str(), AcString::Utf8);
}

void bind_PyDbAnnotationScale(pym& m) {
	py::class_<PyDbAnnotationScale>(m, "PyDbAnnotationScale")
		.def(py::init<AcDbAnnotationScale*>(), py::arg("object") = nullptr)
		.def("isNull", &PyDbAnnotationScale::isNull)
		.def("name", &PyDbAnnotationScale::name)
		.def("setName", &PyDbAnnotationScale::setName, py::arg("v"))
		.def("uniqueIdentifier", &PyDbAnnotationScale::uniqueIdentifier)
		.def("collectionName", &PyDbAnnotationScale::collectionName)
		.def("paperUnits", &PyDbAnnotationScale::paperUnits)
		.def("setPaperUnits", &PyDbAnnotationScale::setPaperUnits, py::arg("v"))
		.def("drawingUnits", &PyDbAnnotationScale::drawingUnits)
		.def("setDrawingUnits", &PyDbAnnotationScale::setDrawingUnits, py::arg("v"))
		.def("scale", &PyDbAnnotationScale::scale)
		.def("isTemporaryScale", &PyDbAnnotationScale::isTemporaryScale)
		.def("matchScaleId", &PyDbAnnotationScale::matchScaleId, py::arg("id"))
		.def("__repr__", &PyDbAnnotationScale::toString);
}
