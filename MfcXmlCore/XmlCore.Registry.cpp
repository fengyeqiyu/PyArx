// XmlCore.Registry.cpp —— 拆自 XmlCore.cpp 第 5918-5966 行。
#include "XmlCoreCommon.h"
#include "XmlCoreInternal.h"

// ===========================================================================
// XmlControlRegistry
// ===========================================================================
XmlControlRegistry& XmlControlRegistry::instance()
{
    static XmlControlRegistry s_inst;
    return s_inst;
}

bool XmlControlRegistry::registerFactory(const wchar_t* tag, Factory factory)
{
    if (tag == nullptr || !factory)
        return false;
    for (auto& kv : m_factories)
        if (_wcsicmp(kv.first.c_str(), tag) == 0)
        {
            kv.second = std::move(factory);
            return true;
        }
    m_factories.emplace_back(tag, std::move(factory));
    return true;
}

bool XmlControlRegistry::isKnown(const wchar_t* tag) const
{
    if (tag == nullptr) return false;
    for (auto& kv : m_factories)
        if (_wcsicmp(kv.first.c_str(), tag) == 0)
            return true;
    return false;
}

IXmlControl* XmlControlRegistry::create(const wchar_t* tag) const
{
    if (tag == nullptr) return nullptr;
    for (auto& kv : m_factories)
        if (_wcsicmp(kv.first.c_str(), tag) == 0)
            return kv.second ? kv.second() : nullptr;
    return nullptr;
}

std::vector<std::wstring> XmlControlRegistry::tags() const
{
    std::vector<std::wstring> out;
    for (auto& kv : m_factories)
        out.push_back(kv.first);
    return out;
}
