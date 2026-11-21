#pragma once

#include <cstddef>
#include <cstdint>
#include <map>
#include <string>
#include <vector>

#include "dwg_reader/export.h"

namespace dwr {

struct PageRef {
    int pageId = 0;
    uint32_t dataSize = 0;
    uint64_t startOffset = 0;
};

struct PageEntry {
    int pageId = 0;
    int sectionSize = 0;
    int parent = 0;
    int left = 0;
    int right = 0;
    bool isGap = false;
    /// 该页在文件中的绝对起始地址（按系统页地图顺序累加得到）。
    uint64_t address = 0;
};

struct SectionDesc {
    uint64_t size = 0;
    uint32_t pageCount = 0;
    uint32_t maxPageSize = 0;
    uint32_t unknown = 0;
    uint32_t compressed = 0;
    uint32_t sectionId = 0;
    uint32_t encrypted = 0;
    std::string name;
    std::vector<PageRef> pages;
};

struct DataPageHdr {
    uint32_t pageSize = 0;
    uint32_t dataSize = 0;
    uint32_t sectionType = 0;
    uint64_t startOffset = 0;
    bool compressed = false;
};

/// R2004 容器：文件头解密、系统/数据页定位与节解压（移植自 container.py）。
class DWREADER_API Container {
public:
    std::string version;
    int maintenance = 0;
    int codepage = 0;
    uint32_t security = 0;
    std::map<int, PageEntry> pageMap;
    /// 系统页地图里的 gap 项（负页号，占据地址空间但无页头）。
    std::vector<PageEntry> gaps;
    /// 最后一页结束后的地址（= 文件内节区末端，可与 last_section_address 校验）。
    uint64_t lastPageAddress = 0;
    std::map<std::string, SectionDesc> sections;
    std::vector<uint8_t> data;
    /// 解析过程中的诊断信息（页地图/节地图解压失败等），供 verify 打印。
    mutable std::string diag;

    /// 按页号在 SectionPageMap 中累计求绝对地址。
    uint64_t pageAddr(int pageId) const;

    /// 定位并解压一个系统页（页地图 / 节地图）。
    std::vector<uint8_t> readSystemPage(uint32_t sig) const;

    /// 读一个数据页：解掩码页头，返回 payload 与页头（不解压）。
    std::vector<uint8_t> readDataPage(int pageId, DataPageHdr& hdr) const;

    /// 把一节所有页解压后按 start_offset 拼到全局缓冲。
    std::vector<uint8_t> readSection(const std::string& name) const;
};

/// 打开并解析 DWG 文件头与页/节地图；失败抛 std::runtime_error。
DWREADER_API Container openDwg(const std::string& path);

}  // namespace dwr
