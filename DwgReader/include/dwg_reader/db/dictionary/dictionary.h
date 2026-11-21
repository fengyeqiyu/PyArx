#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>

namespace dwr {

/// 字典预留（蓝本 AcDbDictionary）：名称 → 对象句柄。
class Dictionary {
public:
    const std::unordered_map<std::string, uint64_t>& entries() const { return m_map; }

    bool has(const std::string& name) const { return m_map.count(name) > 0; }
    uint64_t operator[](const std::string& name) const {
        auto it = m_map.find(name);
        return it == m_map.end() ? 0 : it->second;
    }

private:
    std::unordered_map<std::string, uint64_t> m_map;
};

}  // namespace dwr
