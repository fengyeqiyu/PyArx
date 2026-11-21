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

	// 拷贝 = 非拥有视图（不 close/不 delete），仅 pybind 内部需要时使用；
	// 真正的所有权只跟随「原始包装」或「移动构造」。
	PyRxObject(const PyRxObject& other)
		:m_ptr(other.m_ptr), m_autoDelete(false), m_isDbObject(other.m_isDbObject) {
	}

	// 移动即转移所有权：按值返回（return PyDbEntity(p)）时 pybind 会移动临时对象，
	// 若不做转移，临时对象析构会把「刚打开的对象」close 掉甚至 delete，
	// 调用方拿到已关闭/悬空指针（无头实测表现为 eNotOpenForWrite 致命错误 / 0xC0000374）。
	PyRxObject(PyRxObject&& other) noexcept
		:m_ptr(other.m_ptr), m_autoDelete(other.m_autoDelete), m_isDbObject(other.m_isDbObject) {
		other.m_ptr = nullptr;
		other.m_autoDelete = false;
	}

	PyRxObject& operator=(PyRxObject&& other) noexcept {
		if (this != &other) {
			m_ptr = other.m_ptr;
			m_autoDelete = other.m_autoDelete;
			m_isDbObject = other.m_isDbObject;
			other.m_ptr = nullptr;
			other.m_autoDelete = false;
		}
		return *this;
	}

	// 析构实现在 PyRxObject.cpp：需要在关闭时同步摘除打开缓存（PyDbOpenCache），
	// 这里不内联以免头文件互相包含。
	virtual ~PyRxObject();

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
