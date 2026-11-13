#pragma once
#pragma pack(push, 1)  // 1字节对齐
#include "stdafx.h"

class PyRxClass;
class AcDbDatabase;

class PyRxObject {
protected:
	union {
		struct {
			bool m_autoDelete : 1;
			bool m_isDbObject : 1;
		};
		UCHAR flags;
	};
	AcRxObject* m_ptr;
	// 脱钩时缓存的所属数据库，供 database() 空指针回退
	AcDbDatabase* m_pOwningDb = nullptr;

public:
	virtual AcRxObject* impObj();
	virtual AcRxObject* impObj() const; 
	PyRxClass getRxClass() const;
	static PyRxClass desc();
	static PyRxObject cast(const PyRxObject& rxobj);
	static const AcString className();
	static const AcString appName();
	static const AcString dxfName();
	static const AcString name();

public:
	PyRxObject(AcRxObject* ptr = nullptr, bool isAutoDelete = true, bool isDbObject = false)
		:m_ptr(ptr), m_autoDelete(isAutoDelete), m_isDbObject(isDbObject) {
	};

	virtual ~PyRxObject() {
		if (m_ptr == nullptr) return;
		if (m_isDbObject) {
			AcDbObject* pDbo = static_cast<AcDbObject*>(m_ptr);
			// 已在关闭流程中的对象（CAD 事务/数据库清理）不再重复 close，交由 CAD 清理，
			// 避免静默环境中「入库实体回滚/提交后析构重复 close」导致的关闭挂起。
			if (!pDbo->objectId().isNull() && !pDbo->isReallyClosing()) {
				if (const auto es = pDbo->close(); es != Acad::eOk) {
					acutPrintf(L"\nStatus = %ls in %ls: ", acadErrorStatusText(es));
				}
			}
			m_ptr = nullptr;
			return;
		}
		if (m_autoDelete) {
			delete m_ptr;
			m_ptr = nullptr;
		}
	}

	virtual PyRxObject			clone() const;
	void detachDbObject();
	virtual Acad::ErrorStatus	copyFrom(const PyRxObject& other);
	PyRxObject					queryX(const PyRxClass& protocolClass) const;
	virtual AcRx::Ordering		comparedTo(const PyRxObject& other) const;
	bool						operator == (const PyRxObject& other) const;
	bool						operator!=(const PyRxObject& other) const;
	bool						isDerivedFrom(const PyRxClass& other) const;
	bool						isKindOf(const PyRxClass& aClass) const;
	bool						isNull() const;
	virtual bool				isEqualTo(const PyRxObject& other) const;
	intptr_t intPtr() const;
	size_t hash() const;
};
// 【ABI 修复 2026-10-31】本文件顶部 #pragma pack(push, 1) 原先无配对 pop，
// 经 stdafx.h 预编译头泄漏成整个工程的 pack(1)（/Yu PCH 消费端全部中招）：
// wxFrame 等类被 1 字节打包（sizeof 842 vs 正常 944），成员偏移错位导致
// GetHWND 垃圾值 + 建窗静默失败 + 0xC0000374 堆损坏。此处补配对 pop。
#pragma pack(pop)
