#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace dwr {

/// 符号表记录基类预留（蓝本 AcDbSymbolTableRecord）。
/// 派生：Layer / LType / TextStyle / Block / DimStyle / Ucs / VPort / View / AppId。
class SymbolTableRecord {
public:
    virtual ~SymbolTableRecord() = default;

    uint64_t handle = 0;
    std::string name;
};

/// 符号表预留（蓝本 AcDbSymbolTable）：按名/句柄索引一批记录。
class SymbolTable {
public:
    virtual ~SymbolTable() = default;

    size_t size() const { return m_records.size(); }

protected:
    std::vector<SymbolTableRecord> m_records;
};

}  // namespace dwr
