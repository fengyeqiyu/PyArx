#pragma once

#include <cstddef>
#include <cstdint>
#include <map>
#include <string>
#include <vector>

#include "dwg_reader/export.h"

namespace dwr {

/// R2007 (AC1021) 页地图条目。
struct Page1021 {
    uint32_t size = 0;
    int pageId = 0;
    uint64_t offset = 0;
};

/// 节内单页引用（节地图记录）。
struct SectionPage1021 {
    uint64_t offset = 0;
    uint32_t size = 0;
    int pageId = 0;
    uint32_t uncompSize = 0;
    uint32_t compSize = 0;
    uint32_t checksum = 0;
    uint32_t crc = 0;
};

/// 节地图记录。
struct Section1021 {
    uint64_t dataSize = 0;
    uint64_t maxSize = 0;
    uint32_t encrypted = 0;
    uint64_t hashcode = 0;
    uint32_t nameLength = 0;
    uint32_t unknown = 0;
    uint32_t encoded = 0;
    uint32_t numPages = 0;
    std::string name;
    std::vector<SectionPage1021> pages;
};

/// R2007 容器：文件头 RS 解交织+解压、页地图、节地图、系统/数据页读取。
/// 移植自离线蓝本 container1021.py，对照 decode_r2007.c。
class DWREADER_API Container1021 {
public:
    std::vector<uint8_t> data;
    std::map<int, Page1021> pages;
    std::map<std::string, Section1021> sections;

    const Page1021* findPage(int pageId) const;

    /// 定位并解压一个系统页（页地图 / 节地图），RS k=239。
    std::vector<uint8_t> readSystemPage(uint64_t addr, uint32_t compSize,
                                        uint32_t uncompSize,
                                        uint32_t correction) const;

    /// 把一节所有页解压后按页内 offset 拼到全局缓冲。
    std::vector<uint8_t> readSection(const std::string& name) const;
};

/// 打开并解析 R2007 文件头与页/节地图；失败抛 std::runtime_error。
DWREADER_API Container1021 openDwg1021(const std::string& path);

}  // namespace dwr
