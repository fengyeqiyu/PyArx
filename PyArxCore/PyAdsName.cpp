#include "stdafx.h"
#include "PyAdsName.h"
#include "PyErrorStatusException.h"
#include "stdafx.h"
#include "PyEdSelectionSets.h"
// ---------- 构造 / 析构 ----------

PyAdsName::PyAdsName()
{
    clear();
}

PyAdsName::PyAdsName(const PyAdsName& other)
{
    std::memcpy(m_data, other.m_data, sizeof(m_data));
}

PyAdsName::PyAdsName(const ads_name& other)
{
    std::memcpy(m_data, other, sizeof(m_data));
}

PyAdsName& PyAdsName::operator=(const PyAdsName& other)
{
    if (this != &other)
        std::memcpy(m_data, other.m_data, sizeof(m_data));
    return *this;
}

PyAdsName::~PyAdsName() = default;

PyAdsName::PyAdsName(const AcDbObjectId& objId)
{
    clear();
    acdbGetAdsName(m_data, objId);   // 失败时保持全 0
}

// ---------- 静态工厂 ----------

bool PyAdsName::fromObjectId(const AcDbObjectId& objId, PyAdsName& out)
{
    out.clear();
    return acdbGetAdsName(out.m_data, objId) == eOk;
}

// ---------- 扩展 API ----------

bool PyAdsName::isEmpty() const
{
    return m_data[0] == 0 && m_data[1] == 0;
}

void PyAdsName::clear()
{
    m_data[0] = 0;
    m_data[1] = 0;
}

AcString PyAdsName::toString() const
{
    AcString acs;
    acs.format(L"<AdsName (%08X,%08X)>",
        static_cast<unsigned>(m_data[1]),
        static_cast<unsigned>(m_data[0]));
    return acs;
}

AcDbObjectId PyAdsName::toObjectId() const
{
    AcDbObjectId id;
    Acad::ErrorStatus es = acdbGetObjectId(id, m_data);
    PyWarnIfError(es, L"PyAdsName::toObjectId");
    return id;
}

bool PyAdsName::isValid() const
{
    if (isEmpty())
        return false;

    AcDbObjectId id;
    return acdbGetObjectId(id, const_cast<ads_name&>(m_data)) == eOk
        && !id.isNull();
}
// ---------- 比较运算符 ----------

bool PyAdsName::operator==(const PyAdsName& other) const
{
    return m_data[0] == other.m_data[0]
        && m_data[1] == other.m_data[1];
}

bool PyAdsName::operator!=(const PyAdsName& other) const
{
    return !(*this == other);
}

bool PyAdsName::operator<(const PyAdsName& other) const
{
    if (m_data[1] != other.m_data[1])
        return m_data[1] < other.m_data[1];
    return m_data[0] < other.m_data[0];
}

