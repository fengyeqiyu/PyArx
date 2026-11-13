#pragma once
#include "stdafx.h"
#include <dbdynblk.h>

// AcDbDynBlockTableRecord : public AcHeapOperators（dbdynblk.h 行453）
// 动态块定义工具类，非 AcDbObject 派生，按指针持有。
class PyDbDynBlockTableRecord {
public:
	explicit PyDbDynBlockTableRecord(uint64_t btrOldId);
	~PyDbDynBlockTableRecord();

	PyDbDynBlockTableRecord(const PyDbDynBlockTableRecord&) = delete;
	PyDbDynBlockTableRecord& operator=(const PyDbDynBlockTableRecord&) = delete;

	AcDbDynBlockTableRecord* impObj() { return m_pObj; }
	const AcDbDynBlockTableRecord* impObj() const { return m_pObj; }

private:
	AcDbDynBlockTableRecord* m_pObj = nullptr;
};

void bind_PyDbDynBlockTableRecord(pym& m);
