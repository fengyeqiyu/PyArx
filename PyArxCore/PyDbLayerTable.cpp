#include "stdafx.h"
#include "PyErrorStatusException.h"
#include "PyDbLayerTable.h"

PyWrapper_Cpp(DbLayerTable);
bool PyDbLayerTable::has(const AcString& name) const {
	if (!impObj()) return false;
	return impObj()->has(name.kwszPtr());
}
PyDbLayerTableRecord PyDbLayerTable::getAt(const AcString& name) const {
	if (!impObj()) return PyDbLayerTableRecord(nullptr);
	AcDbLayerTable::RecordType* rec = nullptr;
	if(!PyWarnIfError(impObj()->getAt(name.kwszPtr(), rec, AcDb::kForRead), L"getAt")) return PyDbLayerTableRecord(nullptr);
	return PyDbLayerTableRecord(rec);
}

bool PyDbLayerTable::hasUnreconciledLayers() const {
	if (!impObj()) return false;
	return impObj()->hasUnreconciledLayers();
}

void PyDbLayerTable::generateUsageData() {
	if (!impObj()) return;
	impObj()->generateUsageData();
}

AcDbObjectIdArray PyDbLayerTable::getUnreconciledLayers() const {
	if (!impObj()) return AcDbObjectIdArray();
	AcDbObjectIdArray ids;
	if (!PyWarnIfError(impObj()->getUnreconciledLayers(ids), L"LayerTable.getUnreconciledLayers"))
		return AcDbObjectIdArray();
	return ids;
}

void bind_PyDbLayerTable(pym& m) {
	py::class_<PyDbLayerTable, PyDbSymbolTable>(m, "PyDbLayerTable")
		.def(py::init<AcDbLayerTable*>(), py::arg("p"))
		PyWrapper_BIND(DbLayerTable)
		.def("has", &PyDbLayerTable::has, py::arg("name"))
		.def("getAt", &PyDbLayerTable::getAt, py::arg("name"))
		.def("hasUnreconciledLayers", &PyDbLayerTable::hasUnreconciledLayers)
		.def("generateUsageData", &PyDbLayerTable::generateUsageData)
		.def("getUnreconciledLayers", &PyDbLayerTable::getUnreconciledLayers,
			"返回未协调层的 ObjectId 列表（旧 Id），失败返回空列表");
}
