#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "dwg_reader/export.h"
#include "dwg_reader/value/version.h"

namespace dwr {

/// 经典布局（R13/R14/R2000）的一节：section locator 记录。
struct ClassicSection {
    int number = 0;
    uint64_t address = 0;
    uint32_t size = 0;
    std::string name;
};

/// 经典布局（R13/R14/R2000/R2002）容器：文件头 + 节定位器 + Handles 对象映射。
/// 与 R2004+ 的页容器不同：这里是「绝对偏移 + 顺序节」，无需解压。
class DWREADER_API ClassicContainer {
public:
    std::string version;
    DwgVersion ver = DwgVersion::kUnknown;
    int codepage = 0;
    int declaredSections = 0;
    uint32_t thumbnailAddress = 0;
    /// codepage 字段所在偏移（0x12 = libredwg 系写入器，0x13 = 真 AutoCAD）。
    int headerBase = 0x13;
    std::vector<ClassicSection> sections;
    std::vector<uint8_t> data;
    /// 对象地址表：由 Handles 节逐页解析的 {handle 增量, 地址增量} 归约得到。
    std::vector<uint64_t> objectAddresses;
    /// 与 `objectAddresses` 一一对应的 handle（写侧重建句柄表所需）。
    std::vector<uint64_t> objectHandles;
    /// 源文件句柄节每页的条目数（写侧「保持原分页」重写时用；R2000 只认原分页）。
    std::vector<uint32_t> handlePageEntries;
    /// 对象数据区连续块 [objectBase, objectEnd)，写侧整体搬运用。
    uint64_t objectBase = 0;
    uint64_t objectEnd = 0;
    mutable std::string diag;

    const ClassicSection* section(const std::string& name) const;
};

/// 打开经典布局 DWG；失败抛 std::runtime_error。
DWREADER_API ClassicContainer openClassicDwg(const std::string& path);

}  // namespace dwr
