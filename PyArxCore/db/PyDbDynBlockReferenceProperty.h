#pragma once
#include "stdafx.h"
#include <dbdynblk.h>
#include "PyDbEvalVariant.h"

// AcDbDynBlockReferenceProperty : public AcHeapOperators（dbdynblk.h 行45）
// 轻量属性引用对象，按值持有（非 AcDbObject 派生，不使用 PyWrapper 宏）。
class PyDbDynBlockReferenceProperty {
public:
	PyDbDynBlockReferenceProperty() {}
	PyDbDynBlockReferenceProperty(const AcDbDynBlockReferenceProperty& p) : m_prop(p) {}

	AcDbDynBlockReferenceProperty& impObj() { return m_prop; }
	const AcDbDynBlockReferenceProperty& impObj() const { return m_prop; }

	// 默认构造/空属性（propertyName 为空）时 SDK 方法不可调，值为无效
	bool isValid() const { return !m_prop.propertyName().isEmpty(); }

	// 当前值 / 设置值 / 可选值列表
	PyDbEvalVariant value() const;
	bool setValue(const PyDbEvalVariant& v);
	std::vector<PyDbEvalVariant> allowedValues();

private:
	AcDbDynBlockReferenceProperty m_prop;
};

void bind_PyDbDynBlockReferenceProperty(pym& m);
