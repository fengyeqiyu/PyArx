#pragma once
#include "stdafx.h"
#include <achapi.h>
#include "PyDbHyperlink.h"

// AcDbHyperlinkCollection (plain class, engine-owned). Plain pointer wrapper.
// item() returns a non-owning PyDbHyperlink view over the raw pointer.
class PyDbHyperlinkCollection {
	AcDbHyperlinkCollection* m_ptr = nullptr;
public:
	explicit PyDbHyperlinkCollection(AcDbHyperlinkCollection* p) : m_ptr(p) {}
	AcDbHyperlinkCollection* impObj() const { return m_ptr; }

	int count() const;
	bool addHead(const AcString& name, const AcString& desc, const AcString& subLoc = L"");
	bool addTail(const AcString& name, const AcString& desc, const AcString& subLoc = L"");
	bool addAt(int index, const AcString& name, const AcString& desc, const AcString& subLoc = L"");
	bool removeHead();
	bool removeTail();
	bool removeAt(int index);
	PyDbHyperlink item(int index) const;

	AcString toString() const;
};

void bind_PyDbHyperlinkCollection(pym& m);
