#pragma once
#include "stdafx.h"
#include "PyDbDeepCloneFiler.h"
#include "dbcfilrs.h"

// AcDbWblockCloneFiler : public AcDbDeepCloneFiler (dbcfilrs.h:142)
class PyDbWblockCloneFiler : public PyDbDeepCloneFiler {
public:
	PyDbWblockCloneFiler(AcDbWblockCloneFiler* p = nullptr, bool autoDelete = true)
		: PyDbDeepCloneFiler(p, autoDelete) {}
	AcDbWblockCloneFiler* impObj() { return static_cast<AcDbWblockCloneFiler*>(m_ptr); }
	AcDbWblockCloneFiler* impObj() const { return static_cast<AcDbWblockCloneFiler*>(m_ptr); }
};

void bind_PyDbWblockCloneFiler(pym& m);
