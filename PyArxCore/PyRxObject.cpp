#include "stdafx.h"
#include "PyRxObject.h"
#include "stdafx.h"
#include "PyRxClass.h"
#include "stdafx.h"
#include <acadstrc.h>
#include "stdafx.h"
#include <rxclass.h>
#include "stdafx.h"
#include <rxdefs.h>
#include "stdafx.h"
#include <rxobject.h>
#include "stdafx.h"
#include <string>
#include "stdafx.h"
#include "PyDbObject.h"


PyWrapper_Cpp(RxObject)



bool PyRxObject::isDerivedFrom(const PyRxClass& base) const
{
	return AcRxClass::isDerivedFrom(impObj()->isA(), base.impObj());
}

bool PyRxObject::isKindOf(const PyRxClass& aClass) const
{
	return impObj()->isKindOf(aClass.impObj());
}

bool PyRxObject::isNull() const
{
	return impObj() == nullptr;
}

// 关闭已入库的 AcDbObject 并置空内部指针（防悬空）。ObjectARX 规范：
// addNewlyCreatedDBRObject 后必须 close()，否则事务提交不会将实体写入数据库。
void PyRxObject::detachDbObject()
{
	if (!m_ptr || !m_isDbObject) return;
	AcDbObject* pDbo = static_cast<AcDbObject*>(m_ptr);
	// 脱钩前缓存所属数据库：addEntity 后包装被掏空，database() 需回退到该库
	//（GUI 当前文档场景实测：不缓存则 l.database() 空指针解引用崩溃）
	if (!m_pOwningDb) m_pOwningDb = pDbo->database();
	if (!pDbo->objectId().isNull() && !pDbo->isReallyClosing()) {
		pDbo->close();
	}
	m_ptr = nullptr;
}

bool PyRxObject::isEqualTo(const PyRxObject& other) const
{
	return impObj()->isEqualTo(other.impObj());
}

INT_PTR PyRxObject::intPtr() const
{
	return INT_PTR(impObj());
}

size_t PyRxObject::hash() const
{
	return std::hash<AcRxObject*>{}(m_ptr);
}


PyRxObject PyRxObject::queryX(const PyRxClass& protocolClass) const
{
	return impObj()->queryX(protocolClass.impObj());
}

AcRx::Ordering PyRxObject::comparedTo(const PyRxObject& other) const
{
	return impObj()->comparedTo(other.impObj());
}

bool PyRxObject::operator==(const PyRxObject& other) const
{
	return impObj() == other.impObj();
}

bool PyRxObject::operator!=(const PyRxObject& other) const
{
	return impObj() != other.impObj();
}

PyRxObject PyRxObject::clone() const
{
	return impObj()->clone();
}

Acad::ErrorStatus PyRxObject::copyFrom(const PyRxObject& other)
{
	return this->impObj()->copyFrom(other.impObj());
}