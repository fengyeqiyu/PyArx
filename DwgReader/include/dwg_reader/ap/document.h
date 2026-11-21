#pragma once

#include <string>

namespace dwr {

class Database;

/// 文档预留（蓝本 AcApDocument）：一份打开的 DWG 关联其 Database。
class Document {
public:
    const std::string& fileName() const { return m_file; }
    Database* database() const { return m_db; }

private:
    std::string m_file;
    Database* m_db = nullptr;
};

}  // namespace dwr
