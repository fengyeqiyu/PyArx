#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>

namespace dwr {

/// 类型注册表（蓝本 AcRxClass / RTTI）。
/// 仅维护 type code ↔ 类名映射；实体子类构造工厂归 db 层，rx 不反向依赖 db。
class RxRegistry {
public:
    static RxRegistry& instance();

    void registerName(int typeCode, std::string dxfName);
    const std::string* nameOf(int typeCode) const;
    bool isRegistered(int typeCode) const;

private:
    RxRegistry() = default;
    std::unordered_map<int, std::string> m_names;
};

}  // namespace dwr
