#pragma once
#include "stdafx.h"
#include "dbobjcontext.h"
#include "PyRxObject.h"

// AcDbObjectContext : public AcRxObject  (dbobjcontext.h:30，抽象基类)
class PyDbObjectContext : public PyRxObject {
	PyWrapper_H(DbObjectContext)
public:
	// 上下文对象通常由其 collection 持有，包装默认按借用（不 delete、不 close）。
	PyDbObjectContext(AcDbObjectContext* p = nullptr, bool autoDelete = false)
		: PyRxObject(p, autoDelete, false) {}
};

void bind_PyDbObjectContext(pym& m);
