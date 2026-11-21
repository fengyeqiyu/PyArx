#pragma once

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

#include "dwg_reader/db/entity/entity.h"
#include "dwg_reader/db/index/handle_map.h"
#include "dwg_reader/db/index/object_index.h"
#include "dwg_reader/export.h"
#include "dwg_reader/io/container/classic_container.h"
#include "dwg_reader/io/container/container.h"
#include "dwg_reader/io/container/container1021.h"
#include "dwg_reader/value/version.h"

namespace dwr {

/// DWG 数据库门面：聚合容器、对象流缓冲、句柄/类型索引与实体表。
/// 读：io 层解出对象流缓冲 → 索引/实体在 db 层装配；改/写经 setter 与 io 写侧落盘。
class DWREADER_API Database {
public:
    Database();
    ~Database();

    Database(const Database&) = delete;
    Database& operator=(const Database&) = delete;

    /// 打开并解码 DWG；成功返回 true。
    bool open(const std::string& path);
    void close();

    bool isOpen() const { return m_open; }
    int lastError() const { return m_err; }
    /// 打开失败的具体原因（版本不支持 / 容器解密失败 / 节缺失等）。
    const std::string& lastErrorText() const { return m_errText; }
    DwgVersion version() const { return m_version; }

    /// 解压后的对象流缓冲（AcDb:AcDbObjects 节）。
    const std::vector<uint8_t>& objectsBuffer() const { return m_objects; }

    /// 解压后的句柄流缓冲（AcDb:Handles 节）。
    const std::vector<uint8_t>& handlesBuffer() const { return m_handlesBuf; }

    /// 容器元数据（节清单、页地图等）。
    const Container& container() const { return m_container; }
    const Container1021& container1021() const { return m_container1021; }
    /// 经典布局（R13/R14/R2000）容器。
    const ClassicContainer& classicContainer() const { return m_classic; }

    /// 句柄→地址目录、addr↔handle↔type 索引、实体表（装配随 #21/#20 填充）。
    HandleMap& handleMap() { return m_handles; }
    ObjectIndex& objectIndex() { return m_index; }
    std::vector<std::unique_ptr<Entity>>& entities() { return m_entities; }

private:
    Container m_container;
    Container1021 m_container1021;
    ClassicContainer m_classic;
    std::vector<uint8_t> m_objects;
    std::vector<uint8_t> m_handlesBuf;
    HandleMap m_handles;
    ObjectIndex m_index;
    std::vector<std::unique_ptr<Entity>> m_entities;
    DwgVersion m_version = DwgVersion::kUnknown;
    bool m_open = false;
    int m_err = 0;
    std::string m_errText;
};

}  // namespace dwr
