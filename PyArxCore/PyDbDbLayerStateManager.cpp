#include "stdafx.h"
#include "PyErrorStatusException.h"
#include "PyDbDbLayerStateManager.h"
#include "PyDbDatabase.h"

uint64_t PyDbDbLayerStateManager::layerStatesDictionaryId(bool createIfNotPresent) const {
	if (!m_ptr) return 0ULL;
	AcDbObjectId id = m_ptr->layerStatesDictionaryId(createIfNotPresent);
	return id.isNull() ? 0ULL : static_cast<uint64_t>(id.asOldId());
}

bool PyDbDbLayerStateManager::hasLayerState(const AcString& name) const {
	return m_ptr && m_ptr->hasLayerState(name.kwszPtr());
}

bool PyDbDbLayerStateManager::saveLayerState(const AcString& name, int mask) {
	if (!m_ptr) return false;
	return PyWarnIfError(m_ptr->saveLayerState(name.kwszPtr(),
		static_cast<AcDbLayerStateManager::LayerStateMask>(mask)), L"LayerStateManager.saveLayerState");
}

bool PyDbDbLayerStateManager::restoreLayerState(const AcString& name) {
	if (!m_ptr) return false;
	return PyWarnIfError(m_ptr->restoreLayerState(name.kwszPtr()), L"LayerStateManager.restoreLayerState");
}

bool PyDbDbLayerStateManager::deleteLayerState(const AcString& name) {
	if (!m_ptr) return false;
	return PyWarnIfError(m_ptr->deleteLayerState(name.kwszPtr()), L"LayerStateManager.deleteLayerState");
}

bool PyDbDbLayerStateManager::renameLayerState(const AcString& oldName, const AcString& newName) {
	if (!m_ptr) return false;
	return PyWarnIfError(m_ptr->renameLayerState(oldName.kwszPtr(), newName.kwszPtr()),
		L"LayerStateManager.renameLayerState");
}

std::vector<AcString> PyDbDbLayerStateManager::getLayerStateNames(bool includeHidden, bool includeXref) const {
	std::vector<AcString> out;
	if (!m_ptr) return out;
	AcStringArray arr;
	if (!PyWarnIfError(m_ptr->getLayerStateNames(arr, includeHidden, includeXref),
		L"LayerStateManager.getLayerStateNames")) return out;
	for (int i = 0; i < arr.length(); ++i) out.push_back(arr[i]);
	return out;
}

PyDbDatabase PyDbDbLayerStateManager::getDatabase() const {
	return PyDbDatabase(m_ptr ? m_ptr->getDatabase() : nullptr, false);
}

AcString PyDbDbLayerStateManager::toString() const {
	return AcString(std::format("PyDbDbLayerStateManager[{}]", m_ptr ? "valid" : "null").c_str(), AcString::Utf8);
}

void bind_PyDbDbLayerStateManager(pym& m) {
	auto c = py::class_<PyDbDbLayerStateManager>(m, "PyDbDbLayerStateManager")
		.def(py::init<AcDbLayerStateManager*>(), py::arg("object") = nullptr)
		.def("isNull", &PyDbDbLayerStateManager::isNull)
		.def("layerStatesDictionaryId", &PyDbDbLayerStateManager::layerStatesDictionaryId,
			py::arg("createIfNotPresent") = false)
		.def("hasLayerState", &PyDbDbLayerStateManager::hasLayerState, py::arg("name"))
		.def("saveLayerState", &PyDbDbLayerStateManager::saveLayerState,
			py::arg("name"), py::arg("mask"))
		.def("restoreLayerState", &PyDbDbLayerStateManager::restoreLayerState, py::arg("name"))
		.def("deleteLayerState", &PyDbDbLayerStateManager::deleteLayerState, py::arg("name"))
		.def("renameLayerState", &PyDbDbLayerStateManager::renameLayerState,
			py::arg("oldName"), py::arg("newName"))
		.def("getLayerStateNames", &PyDbDbLayerStateManager::getLayerStateNames,
			py::arg("includeHidden") = true, py::arg("includeXref") = true)
		.def("getDatabase", &PyDbDbLayerStateManager::getDatabase)
		.def("__repr__", &PyDbDbLayerStateManager::toString);

	// LayerStateMask bit constants (dblstate.h).
	c.attr("kNone") = (int)AcDbLayerStateManager::kNone;
	c.attr("kOn") = (int)AcDbLayerStateManager::kOn;
	c.attr("kFrozen") = (int)AcDbLayerStateManager::kFrozen;
	c.attr("kLocked") = (int)AcDbLayerStateManager::kLocked;
	c.attr("kPlot") = (int)AcDbLayerStateManager::kPlot;
	c.attr("kNewViewport") = (int)AcDbLayerStateManager::kNewViewport;
	c.attr("kColor") = (int)AcDbLayerStateManager::kColor;
	c.attr("kLineType") = (int)AcDbLayerStateManager::kLineType;
	c.attr("kLineWeight") = (int)AcDbLayerStateManager::kLineWeight;
	c.attr("kPlotStyle") = (int)AcDbLayerStateManager::kPlotStyle;
	c.attr("kCurrentViewport") = (int)AcDbLayerStateManager::kCurrentViewport;
	c.attr("kTransparency") = (int)AcDbLayerStateManager::kTransparency;
	c.attr("kAll") = (int)AcDbLayerStateManager::kAll;
	c.attr("kStateIsHidden") = (int)AcDbLayerStateManager::kStateIsHidden;
	c.attr("kLastRestored") = (int)AcDbLayerStateManager::kLastRestored;
	c.attr("kDecomposition") = (int)AcDbLayerStateManager::kDecomposition;
}
