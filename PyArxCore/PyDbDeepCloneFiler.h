#pragma once
#include "stdafx.h"
#include "PyRxObject.h"
#include "dbcfilrs.h"

// AcDbDeepCloneFiler : public AcDbDwgFiler (dbcfilrs.h:30)
// AcDbDwgFiler : public AcRxObject (dbfiler.h:82)
// 工程无 PyDbDwgFiler 包装，故 PyDbDeepCloneFiler 直接继承 PyRxObject，
// 不使用 PyWrapper_H/Cpp/BIND 宏（独立绑定）。
class PyDbDeepCloneFiler : public PyRxObject {
public:
	PyDbDeepCloneFiler(AcDbDeepCloneFiler* p, bool autoDelete = true)
		: PyRxObject(p, autoDelete, false) {}
	PyDbDeepCloneFiler() : PyDbDeepCloneFiler(new AcDbDeepCloneFiler()) {}
	AcDbDeepCloneFiler* impObj() { return static_cast<AcDbDeepCloneFiler*>(m_ptr); }
	AcDbDeepCloneFiler* impObj() const { return static_cast<AcDbDeepCloneFiler*>(m_ptr); }
};

void bind_PyDbDeepCloneFiler(pym& m);
