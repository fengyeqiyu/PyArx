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
#include "PyDbOpenCache.h"   // 析构时摘除打开缓存，避免缓存留下已关闭的悬空指针


PyWrapper_Cpp(RxObject)

PyRxObject::~PyRxObject() {
	if (m_ptr == nullptr) return;
	if (m_isDbObject) {
		AcDbObject* pDbo = static_cast<AcDbObject*>(m_ptr);
		if (!m_autoDelete) {
			// 非拥有视图（拷贝出来的临时包装）：既不再 close 也不 delete，
			// 否则会把调用方正在使用的对象关掉。
			m_ptr = nullptr;
			return;
		}
		PyDbOpenCache::Close(pDbo);   // 先摘缓存，后续重新打开不会拿到已关闭的指针
		if (!pDbo->objectId().isNull()) {
			// 入库对象：包装只负责关闭，所有权归数据库。
			// 注意：不能拿 isReallyClosing() 当「是否已关闭」判据 —— 无头实测它对
			// 正常打开的对象恒返回 true，跳过 close 会让对象永远保持打开态，
			// 后续再以 kForWrite 打开就永远 eWasOpenForRead。
			// 已关闭时 close() 返回 eNotOpenForWrite，无副作用。
			pDbo->close();
		}
		else if (m_autoDelete) {
			// 未入库对象（clone 副本 / new 出来的图元）：包装持有所有权。
			// 先 close 再 delete —— 直接 delete 打开态 AcDbObject 会造成 ARX 堆损坏
			// （无头实测 0xC0000374）；已关闭时 close 返回 eNotOpenForWrite，无副作用。
			pDbo->close();
			delete pDbo;
		}
		m_ptr = nullptr;
		return;
	}
	if (m_autoDelete) {
		delete m_ptr;
		m_ptr = nullptr;
	}
}



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
	AcRxObject* pNew = impObj()->clone();
	if (!pNew) throw std::runtime_error("clone: 该对象不支持克隆");
	// 克隆出的 AcDbObject 处于「未入库但仍打开」状态；若直接交给自动 delete 的包装，
	// 析构时会 delete 打开态对象 → ARX 堆损坏（无头实测 0xC0000374）。先关闭再交付。
	if (pNew->isKindOf(AcDbObject::desc())) {
		AcDbObject* pDbo = static_cast<AcDbObject*>(pNew);
		if (pDbo->objectId().isNull()) pDbo->close();
	}
	return PyRxObject(pNew, true, false);
}

Acad::ErrorStatus PyRxObject::copyFrom(const PyRxObject& other)
{
	return this->impObj()->copyFrom(other.impObj());
}