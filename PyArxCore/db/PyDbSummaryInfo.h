#pragma once
#include "stdafx.h"
#include <summinfo.h>

// AcDbDatabaseSummaryInfo : AcRxObject (abstract; obtained via acdbGetSummaryInfo,
// engine-owned, NOT auto-deleted here). Plain pointer wrapper; no PyWrapper_H macro.
class PyDbDatabaseSummaryInfo {
	AcDbDatabaseSummaryInfo* m_ptr = nullptr;
public:
	explicit PyDbDatabaseSummaryInfo(AcDbDatabaseSummaryInfo* p) : m_ptr(p) {}
	AcDbDatabaseSummaryInfo* impObj() const { return m_ptr; }

	AcString title() const;
	bool setTitle(const AcString& v);
	AcString subject() const;
	bool setSubject(const AcString& v);
	AcString author() const;
	bool setAuthor(const AcString& v);
	AcString keywords() const;
	bool setKeywords(const AcString& v);
	AcString comments() const;
	bool setComments(const AcString& v);
	AcString lastSavedBy() const;
	bool setLastSavedBy(const AcString& v);
	AcString revisionNumber() const;
	bool setRevisionNumber(const AcString& v);
	AcString hyperlinkBase() const;
	bool setHyperlinkBase(const AcString& v);

	int numCustomInfo() const;

	// ---- 自定义摘要信息 ----
	bool addCustomInfo(const AcString& key, const AcString& value);
	bool deleteCustomInfoAt(int index);
	bool deleteCustomInfoByKey(const AcString& key);
	py::tuple getCustomInfoAt(int index) const;
	bool setCustomInfoAt(int index, const AcString& key, const AcString& value);
	AcString getCustomInfoByKey(const AcString& key) const;
	bool setCustomInfoByKey(const AcString& key, const AcString& value);

	AcString toString() const;
};

void bind_PyDbDatabaseSummaryInfo(pym& m);
