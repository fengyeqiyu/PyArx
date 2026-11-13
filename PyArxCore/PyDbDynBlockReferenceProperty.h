#pragma once
#include "stdafx.h"
#include <dbdynblk.h>

// AcDbDynBlockReferenceProperty : public AcHeapOperators（dbdynblk.h 行45）
// 轻量属性引用对象，按值持有（非 AcDbObject 派生，不使用 PyWrapper 宏）。
// AcDbEvalVariant 相关方法（value/setValue/getAllowedValues）待 P2 封装后补绑。
class PyDbDynBlockReferenceProperty {
public:
	PyDbDynBlockReferenceProperty() {}
	PyDbDynBlockReferenceProperty(const AcDbDynBlockReferenceProperty& p) : m_prop(p) {}

	AcDbDynBlockReferenceProperty& impObj() { return m_prop; }
	const AcDbDynBlockReferenceProperty& impObj() const { return m_prop; }

private:
	AcDbDynBlockReferenceProperty m_prop;
};

void bind_PyDbDynBlockReferenceProperty(pym& m);
