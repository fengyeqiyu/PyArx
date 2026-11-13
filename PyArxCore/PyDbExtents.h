#pragma once
#include "stdafx.h"
class PyDbExtents2d : public AcHeapOperators
{
public:
	PyDbExtents2d() : m_Extents() {};
	PyDbExtents2d(const PyDbExtents2d& src) : m_Extents(src.m_Extents) {};
	PyDbExtents2d(const AcGePoint2d& min, const AcGePoint2d& max) : m_Extents(min, max) {};

	AcDbExtents2d& impObj() {
		return m_Extents;
	}
	AcDbExtents2d impObj() const {
		return m_Extents;
	}
	AcGePoint2d minPoint() const { return m_Extents.minPoint(); }
	AcGePoint2d maxPoint() const { return m_Extents.maxPoint(); }
	void set(const AcGePoint2d& min, const AcGePoint2d& max) {
		m_Extents.set(
			{ std::min(min.x, max.x), std::min(min.y, max.y) },
			{ std::max(min.x, max.x), std::max(min.y, max.y) }
		);
	}

	void addPoint(const AcGePoint2d& pt) {
		m_Extents.addPoint(pt);
	}
	void addExt(const PyDbExtents2d& src) {
		m_Extents.addExt(src.m_Extents);
	}
	void expandBy(const AcGeVector2d& vec) {
		m_Extents.expandBy(vec);
	}
	void transformBy(const AcGeMatrix2d& mat) {
		m_Extents.transformBy(mat);
	}
	bool operator==(const PyDbExtents2d& other) {
		return m_Extents == other.m_Extents;
	}
private:
	AcDbExtents2d m_Extents;
	bool m_isEmpty = false;
};


//AcDbExtents 不带空检查，所以我加了一个bool 值

class PyDbExtents3d
{
public:
	// 构造函数组 
	PyDbExtents3d() : m_Extents(), m_isEmpty(true) {};
	PyDbExtents3d(const AcDbExtents& src, bool isEmpty = false) : m_Extents(src), m_isEmpty(isEmpty) {};
	PyDbExtents3d(const AcGePoint3d& min, const AcGePoint3d& max)
		: m_Extents(min, max), m_isEmpty(min == max) {
	};

	~PyDbExtents3d() {
		m_isEmpty = true;
	};
	// 对象访问接口 
	AcDbExtents& impObj() { return m_Extents; }
	AcDbExtents impObj() const { return m_Extents; }
	bool isEmpty() const { return m_isEmpty; }

	// 几何属性访问 
	AcGePoint3d minPoint() const { return m_isEmpty ? AcGePoint3d::kOrigin : m_Extents.minPoint(); }
	AcGePoint3d maxPoint() const { return m_isEmpty ? AcGePoint3d::kOrigin : m_Extents.maxPoint(); }

	// 范围操作 
	void set(const AcGePoint3d& min, const AcGePoint3d& max) {
		m_Extents.set(
			{ std::min(min.x, max.x), std::min(min.y, max.y), std::min(min.z, max.z) },
			{ std::max(min.x, max.x), std::max(min.y, max.y), std::max(min.z, max.z) }
		);
		m_isEmpty = (min == max);
	}

	void addPoint(const AcGePoint3d& pt) {
		if (m_isEmpty) {
			m_Extents.set(pt, pt);
			m_isEmpty = false;
		}
		else {
			m_Extents.addPoint(pt);
		}
	}

	void addExt(const PyDbExtents3d& src) {
		if (src.m_isEmpty) return;
		if (m_isEmpty) {
			m_Extents = src.m_Extents;
			m_isEmpty = false;
		}
		else {
			m_Extents.addExt(src.m_Extents);
		}
	}

	// 块范围操作 
	Acad::ErrorStatus addBlockExt(AcDbBlockTableRecord* pBTR) {
		auto ret = m_Extents.addBlockExt(pBTR);
		if (ret == Acad::eOk) m_isEmpty = false;
		return ret;
	}

	// 几何变换
	void expandBy(const AcGeVector3d& vec) {
		if (!m_isEmpty) m_Extents.expandBy(vec);
	}

	void transformBy(const AcGeMatrix3d& mat) {
		if (!m_isEmpty) m_Extents.transformBy(mat);
	}

	// 运算符重载 
	bool operator==(const PyDbExtents3d& other) const {
		return (m_isEmpty && other.m_isEmpty) ||
			(!m_isEmpty && !other.m_isEmpty && m_Extents == other.impObj());
	}

private:
	AcDbExtents m_Extents;
	bool m_isEmpty = true;  // 默认初始化为true 
};
