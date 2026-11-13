#include "stdafx.h"
#include "PyDbDynBlockReferenceProperty.h"
#include "PyErrorStatusException.h"

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
		// ---- 以下方法依赖 AcDbEvalVariant，待 P2 封装后补绑 ----
		// Acad::ErrorStatus getAllowedValues(AcDbEvalVariantArray& values);
		// AcDbEvalVariant value() const;
		// Acad::ErrorStatus setValue(const AcDbEvalVariant& value);
		;
}
