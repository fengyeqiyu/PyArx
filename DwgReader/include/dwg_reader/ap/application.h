#pragma once

#include <vector>

#include "dwg_reader/ap/document.h"

namespace dwr {

/// 应用入口预留（蓝本 AcApApplication / DocumentManager）：文档列表、当前文档。
class Application {
public:
    static Application& instance();

    Document* currentDocument() {
        return m_docs.empty() ? nullptr : &m_docs.back();
    }

    size_t documentCount() const { return m_docs.size(); }

private:
    Application() = default;
    std::vector<Document> m_docs;
};

}  // namespace dwr
