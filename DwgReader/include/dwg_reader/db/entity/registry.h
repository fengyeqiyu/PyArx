#pragma once

#include <cstdint>
#include <memory>

namespace dwr {

class Entity;

/// 按对象 type code 构造对应实体子类；未注册类型返回 nullptr。
/// 对齐离线 acdb.BY_CODE 注册表（#20 填充各子类）。
std::unique_ptr<Entity> makeEntity(int typeCode, uint64_t handle);

}  // namespace dwr
