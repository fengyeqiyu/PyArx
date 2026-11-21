#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "dwg_reader/export.h"
#include "dwg_reader/io/container/classic_container.h"

namespace dwr {

struct ClassicWriteOptions {
    /// 节按 4 字节对齐（ARX/AutoCAD 的经典布局约定）。
    uint32_t align = 4;
    /// 句柄表每页载荷上限（真 AutoCAD R14 实测约 2032）。
    uint32_t handlePagePayload = 2030;
    /// 复制文件头 [0, headerBase) 的前缀字节（版本码 + 缩略图地址等）。
    bool copyHeaderPrefix = true;
    /// 保持源文件的节地址不变（用于定位「与 AutoCAD 的字节差异」；默认紧凑重排）。
    bool preserveLayout = false;
    /// 诊断用：句柄节原样复制（不重建），用于区分「表内容错误」与「容器错误」。
    bool copyHandles = false;
    /// 保持源文件句柄节的分页（每页条目数），R2000 重写必须开启才能被 AutoCAD 接受。
    bool preservePagination = false;
};

struct ClassicWriteResult {
    std::vector<uint8_t> bytes;
    size_t objectCount = 0;
    size_t handlePages = 0;
    uint64_t objectBase = 0;
    std::string diag;
};

/// 重建经典布局 DWG 容器（R13/R14/R2000/R2002）：
///   文件头 + 节定位器 + [AuxHeader, Header, Classes, 对象区, Handles, Template]
/// 对象区按原样搬运（相对偏移不变），Handles 按新的对象基址重算增量并重新分页。
DWREADER_API ClassicWriteResult writeClassicImage(const ClassicContainer& src,
                                                 const ClassicWriteOptions& opt = {});

/// 同上并落盘。
DWREADER_API ClassicWriteResult writeClassicDwg(const ClassicContainer& src,
                                               const std::string& outPath,
                                               const ClassicWriteOptions& opt = {});

}  // namespace dwr
