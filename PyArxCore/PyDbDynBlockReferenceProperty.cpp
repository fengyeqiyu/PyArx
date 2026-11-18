#include "stdafx.h"
#include "PyDbDynBlockReferenceProperty.h"
#include "PyErrorStatusException.h"

PyDbEvalVariant PyDbDynBlockReferenceProperty::value() const {
	if (m_prop.propertyName().isEmpty()) return PyDbEvalVariant();
	try {
		return PyDbEvalVariant(m_prop.value());
	}
	catch (const std::exception& e) {
		acutPrintf(L"\n[DynBlockReferenceProperty] value: %hs\n", e.what());
		return PyDbEvalVariant();
	}
	catch (...) {
		acutPrintf(L"\n[DynBlockReferenceProperty] value: unknown SDK exception\n");
		return PyDbEvalVariant();
	}
}

bool PyDbDynBlockReferenceProperty::setValue(const PyDbEvalVariant& v) {
	if (m_prop.propertyName().isEmpty()) return false;
	return PyWarnIfError(m_prop.setValue(v.impObj()), L"AcDbDynBlockReferenceProperty::setValue");
}

std::vector<PyDbEvalVariant> PyDbDynBlockReferenceProperty::allowedValues() {
	std::vector<PyDbEvalVariant> out;
	if (m_prop.propertyName().isEmpty()) return out;
	AcDbEvalVariantArray arr;
	if (!PyWarnIfError(m_prop.getAllowedValues(arr), L"AcDbDynBlockReferenceProperty::getAllowedValues"))
		return out;
	out.reserve(arr.length());
	for (int i = 0; i < arr.length(); ++i) out.emplace_back(arr[i]);
	return out;
}

void bind_PyDbDynBlockReferenceProperty(pym& m) {
	py::class_<PyDbDynBlockReferenceProperty>(m, "DynBlockReferenceProperty")
		.def(py::init<>())
		// blockId -> uint64_t（oldId 约定，0 表示 null）
		.def("blockId", [](const PyDbDynBlockReferenceProperty& p) -> uint64_t {
			auto id = p.impObj().blockId();
			return id.isNull() ? 0ULL : static_cast<uint64_t>(id.asOldId());
		})
		// propertyName
		.def("propertyName", [](const PyDbDynBlockReferenceProperty& p) -> AcString {
			return p.impObj().propertyName();
		})
		// propertyType（AcDb::DwgDataType 枚举 -> int）
		.def("propertyType", [](const PyDbDynBlockReferenceProperty& p) -> int {
			return static_cast<int>(p.impObj().propertyType());
		})
		.def("readOnly", [](const PyDbDynBlockReferenceProperty& p) { return p.impObj().readOnly(); })
		.def("show", [](const PyDbDynBlockReferenceProperty& p) { return p.impObj().show(); })
		.def("visibleInCurrentVisibilityState", [](const PyDbDynBlockReferenceProperty& p) {
			return p.impObj().visibleInCurrentVisibilityState();
		})
		// description
		.def("description", [](const PyDbDynBlockReferenceProperty& p) -> AcString {
			return p.impObj().description();
		})
		// unitsType（UnitsType 枚举 -> int）
		.def("unitsType", [](const PyDbDynBlockReferenceProperty& p) -> int {
			return static_cast<int>(p.impObj().unitsType());
		})
		// ---- 值读写（AcDbEvalVariant 已封装，补上原先挂起的三项）----
		.def("isValid", &PyDbDynBlockReferenceProperty::isValid, "属性是否有效（空对象返回 false）")
		.def("value", &PyDbDynBlockReferenceProperty::value,
			"当前值（PyDbEvalVariant，用 getDouble/getString/getType 取具体内容）")
		.def("setValue", &PyDbDynBlockReferenceProperty::setValue, py::arg("value"),
			"设置当前值（传 PyDbEvalVariant），返回 bool")
		.def("allowedValues", &PyDbDynBlockReferenceProperty::allowedValues,
			"允许值列表（PyDbEvalVariant 列表；非枚举属性返回空 list）")
		;
}
