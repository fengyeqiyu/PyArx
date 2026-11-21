#pragma once
#include "stdafx.h"
#include <achapi.h>

// AcDbHyperlink (plain class, NOT AcRxObject; engine-owned). Plain pointer wrapper.
class PyDbHyperlink {
	AcDbHyperlink* m_ptr = nullptr;
public:
	explicit PyDbHyperlink(AcDbHyperlink* p) : m_ptr(p) {}
	AcDbHyperlink* impObj() const { return m_ptr; }

	AcString name() const;
	bool setName(const AcString& v);
	AcString description() const;
	bool setDescription(const AcString& v);
	AcString subLocation() const;
	bool setSubLocation(const AcString& v);
	AcString displayString() const;
	bool isOutermostContainer() const;
	int nestedLevel() const;
	int flags() const;
	bool setFlags(int f);

	AcString toString() const;
};

void bind_PyDbHyperlink(pym& m);
