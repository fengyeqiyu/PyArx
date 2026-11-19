#include "stdafx.h"
#include "PyErrorStatusException.h"
#include "PyDbLayerTable.h"

PyWrapper_Cpp(DbLayerTable);
bool PyDbLayerTable::has(const AcString& name) const {
	if (!impObj()) return false;
	return impObj()->has(name.kwszPtr());
}
PyDbLayerTableRecord* PyDbLayerTable::getAt(const AcString& name, int mode) const {
	if (!impObj()) return nullptr;
	// 走 acdbOpenObject：事务内打开会被登记为「事务驻留」，set 属性才安全。
	// 此前直接调 AcDbLayerTable::getAt 打开，对象不在事务里，改属性直接杀进程（无头实测）。
	AcDbObjectId id;
	if (impObj()->getIdAt(name.kwszPtr(), id) != Acad::eOk || id.isNull())
		return nullptr;
	AcDbLayerTableRecord* pRec = nullptr;
	if (!PyWarnIfError(acdbOpenObject(pRec, id, static_cast<AcDb::OpenMode>(mode)), L"LayerTable.getAt"))
		return nullptr;
	return new PyDbLayerTableRecord(pRec);
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
		.def("getAt", &PyDbLayerTable::getAt, py::arg("name"), py::arg("mode") = 1, "按名称打开图层记录；mode 默认 1=写，纯查询传 0=读")
		.def("hasUnreconciledLayers", &PyDbLayerTable::hasUnreconciledLayers)
		.def("generateUsageData", &PyDbLayerTable::generateUsageData)
		.def("getUnreconciledLayers", &PyDbLayerTable::getUnreconciledLayers,
			"返回未协调层的 ObjectId 列表（旧 Id），失败返回空列表");
}
