#include "stdafx.h"
#include "pyedSelectionSets.h"
#include <acedsubsel.h>
#include "PyResbuf.h"
#include "PyAdsName.h"

int PyEdSelectionSets::clear()
{
	int rc = RTNORM;
	if (m_owned) rc = acedSSFree(m_data);
	m_data[0] = 0;
	m_data[1] = 0;
	m_owned = false;
	return rc;
}

bool PyEdSelectionSets::isEmpty() const
{
	return count() == 0;
}

size_t PyEdSelectionSets::count() const {
	Adesk::Int32 sslen = 0;
	if (acedSSLength(m_data, &sslen) != RTNORM)
		return 0;
	return sslen;
}

PyAdsName PyEdSelectionSets::getEntNameAt(size_t index) const
{
	PyAdsName ename;
	acedSSName(m_data, index, ename.data());
	return ename;
}

PyResbuf PyEdSelectionSets::getResbufAt(size_t index)const
{
	PyAdsName ename;
	acedSSName(m_data, index, ename.data());
	resbuf* pRb = acdbEntGet(ename.data());
	return PyResbuf(pRb);
}

AcDbObjectId PyEdSelectionSets::getObjectIdAt(size_t index) const
{
	AcDbObjectId result;
	PyAdsName ename;
	acedSSName(m_data, index, ename.data());
	acdbGetObjectId(result, ename.data());
	return result;
}

size_t PyEdSelectionSets::subentCount(size_t index) const
{
	Adesk::Int32 sslen = 0;
	if (acedSSLength(m_data, &sslen) != RTNORM)
		return 0;
	return sslen;
}


void PyEdSelectionSets::add(const AcDbObjectId& objId)
{
	PyAdsName entname;
	auto es = acdbGetAdsName(entname.data(), objId);
	if (es != eOk) return;
	if (isEmpty()) {
		auto rc = acedSSAdd(entname.data(), NULL, m_data);
		if (rc != RTNORM) {
			acutPrintf(L"添加objid 到选择集失败！\n");
		} else {
			m_owned = true; // 首次建集成功，由本对象析构释放
		}
	}
	else {
		auto rc = acedSSAdd(entname.data(), m_data, m_data);
		if (rc != RTNORM) {
			acutPrintf(L"添加objid 到选择集失败！\n");

		}
	}
}

void PyEdSelectionSets::remove(const AcDbObjectId& objId)
{
	PyAdsName ent;
	auto es = acdbGetAdsName(ent.data(), objId);
	if (es != eOk) return;
	auto rc = acedSSDel(ent.data(), m_data);
	if (rc != RTNORM) return;
}

bool PyEdSelectionSets::hasMember(const AcDbObjectId& objId) const
{
	PyAdsName ent;
	auto es = acdbGetAdsName(ent.data(), objId);
	if (es != eOk) return false;
	return acedSSMemb(ent.data(), m_data) == RTNORM;
}


AcDbFullSubentPath PyEdSelectionSets::subentName(size_t entIndex, size_t subentIndex) const
{
	AcDbFullSubentPath path;
	acedSSSubentName(m_data, entIndex, subentIndex, path);
	return path;
}

TVList PyEdSelectionSets::subentNameX(size_t entIndex, size_t subentIndex, int flags) const
{
	resbuf* pRb;
	acedSSSubentNameX(&pRb, m_data, entIndex, subentIndex, flags);
	PyResbuf buff(pRb);
	return buff.toList();
}

bool PyEdSelectionSets::ssSetFirst() const
{
	PyAdsName dummy;
	return acedSSSetFirst(m_data, dummy.data()) == RTNORM;
}

Acad::PromptStatus PyEdSelectionSets::ssXform(const AcGeMatrix3d& xform) const
{
	ads_matrix adsXform;
	memcpy(adsXform, xform.entry, sizeof(ads_matrix));
	return static_cast<Acad::PromptStatus>(acedXformSS(m_data, adsXform));
}

TVList PyEdSelectionSets::ssNameX(int idx) const
{
	resbuf* rb = nullptr;
	acedSSNameX(&rb, m_data, idx);
	PyResbuf buff(rb);
	auto tvs = buff.toList();
	return tvs;
}

std::vector<AcDbObjectId> PyEdSelectionSets::objectIds() const
{
	std::vector<AcDbObjectId> result; // 不预分配，避免默认构造无效对象
	result.reserve(count()); // 预分配内存提升性能

	PyAdsName ename;
	AcDbObjectId objId;

	for (int i = 0; i < count(); i++)
	{
		if (acedSSName(m_data, i, ename.data()) == RTNORM &&
			acdbGetObjectId(objId, ename.data()) == eOk)
		{
			result.emplace_back(objId); // 直接构造，避免拷贝 
		}
	}
	return result;
}


