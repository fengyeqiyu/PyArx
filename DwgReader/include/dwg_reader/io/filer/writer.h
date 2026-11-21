#pragma once

#include <string>

namespace dwr {

class Database;

/// DWG/DXF 写侧预留（蓝本 AcDbDwgFiler 写回 + 导出）。
/// 对象树 → 位流 → 压缩 → 页节打包；DXF 先行，DWG 二进制写回最后（M5+）。
/// 本轮仅落接口骨架，不实现编码。
class DwgWriter {
public:
    explicit DwgWriter(const Database& db) : m_db(db) {}

    /// 导出 DXF（里程碑 M5 先做）。
    bool saveDxf(const std::string& path);

    /// 打包写回 DWG（二进制压缩 + 页节，里程碑 M6 后）。
    bool saveDwg(const std::string& path);

private:
    const Database& m_db;
};

}  // namespace dwr
