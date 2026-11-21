#pragma once

#include <cstdint>
#include <vector>

namespace dwr {

/// 交互入口预留（蓝本 AcEdEditor）。
/// 能力：选择集 / 点拾取 / 夹点 / 拖拽 Jig / 命令提示 Prompt / UCS 与视口交互。
/// 依赖 db + gi + ge；本轮仅落空骨架，M6 实现。
class Editor {
public:
    virtual ~Editor() = default;

    /// 当前选择集句柄列表（预留）。
    const std::vector<uint64_t>& selectionSet() const { return m_selection; }

private:
    std::vector<uint64_t> m_selection;
};

}  // namespace dwr
