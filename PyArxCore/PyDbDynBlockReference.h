#pragma once
#include "stdafx.h"
#include <dbdynblk.h>

class PyDbDynBlockReferenceProperty;

// AcDbDynBlockReference : public AcHeapOperators（dbdynblk.h 行274）
// 非 AcDbObject 派生的工具类，按指针持有内部实现对象。
class PyDbDynBlockReference {
public:
	// 从块参照 ObjectId（oldId uint64）构造
	explicit PyDbDynBlockReference(uint64_t blockRefOldId);
	~PyDbDynBlockReference();

	// 不可拷贝（持有实现指针）
	PyDbDynBlockReference(const PyDbDynBlockReference&) = delete;
	PyDbDynBlockReference& operator=(const PyDbDynBlockReference&) = delete;

	AcDbDynBlockReference* impObj() { return m_pObj; }
	const AcDbDynBlockReference* impObj() const { return m_pObj; }
	// 无效对象（空 ObjectId 构造，m_pObj==nullptr）：SDK 方法不可调，返回默认值/false
	bool isValid() const { return m_pObj != nullptr; }

private:
	AcDbDynBlockReference* m_pObj = nullptr;
};

void bind_PyDbDynBlockReference(pym& m);
