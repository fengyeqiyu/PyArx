#pragma once

namespace dwr {

/// DWG 文件格式版本（枚举成员统一 k 前缀）。
/// 数值顺序 = 文件格式代际，便于用比较做版本分支。
enum class DwgVersion : int {
    kUnknown = 0,
    kAc1009,  ///< R11/R12（无对象句柄流）
    kAc1012,  ///< R13
    kAc1014,  ///< R14
    kAc1015,  ///< R2000/R2001/R2002（经典节定位器 + 第二头）
    kAc1018,  ///< R2004/R2005/R2006（0x2000 页 + 掩码数据页）
    kAc1021,  ///< R2007/R2008/R2009（RS 编码文件头 + 数据页 RS 校验）
    kAc1024,  ///< R2010/R2011/R2012（对象头改 BOT + 句柄流长度 UMC）
    kAc1027,  ///< R2013/R2014/R2015/R2016/R2017
    kAc1032,  ///< R2018/R2019+
};

/// R2004 及以后（页式容器 + Handles 分页）。
inline bool isPageContainer(DwgVersion v) {
    return v >= DwgVersion::kAc1018;
}

/// R2007 及以后（独立字符串流；实体公共段带 material/shadow 标志）。
inline bool hasStringStream(DwgVersion v) {
    return v >= DwgVersion::kAc1021;
}

/// R2010 及以后（对象记录 = MS size + UMC 句柄流长度；类型改 BOT；句柄 8 字节可选）。
inline bool hasHandleStreamSize(DwgVersion v) {
    return v >= DwgVersion::kAc1024;
}

/// R2004 及以后（公共实体段用 ENC 颜色编码，带 is_xdic_missing 位）。
inline bool hasEncColor(DwgVersion v) {
    return v >= DwgVersion::kAc1018;
}

/// R13-R14（实体公共段自身带 bitsize、isbylayerlt 位、无 lineweight/flags）。
inline bool isR13R14(DwgVersion v) {
    return v == DwgVersion::kAc1012 || v == DwgVersion::kAc1014;
}

/// 从 6 字节版本串（"AC1018"）解析版本枚举；不认识返回 kUnknown。
inline DwgVersion versionFromCode(const char* code6) {
    if (!code6 || code6[0] != 'A' || code6[1] != 'C') return DwgVersion::kUnknown;
    int n = 0;
    for (int i = 2; i < 6; ++i) {
        if (code6[i] < '0' || code6[i] > '9') return DwgVersion::kUnknown;
        n = n * 10 + (code6[i] - '0');
    }
    switch (n) {
        case 1009: return DwgVersion::kAc1009;
        case 1012: return DwgVersion::kAc1012;
        case 1014: return DwgVersion::kAc1014;
        case 1015: return DwgVersion::kAc1015;
        case 1018: return DwgVersion::kAc1018;
        case 1021: return DwgVersion::kAc1021;
        case 1024: return DwgVersion::kAc1024;
        case 1027: return DwgVersion::kAc1027;
        case 1032: return DwgVersion::kAc1032;
        default: return DwgVersion::kUnknown;
    }
}

/// 版本串（"AC1021"）；未知返回 "?"。
inline const char* versionCode(DwgVersion v) {
    switch (v) {
        case DwgVersion::kAc1009: return "AC1009";
        case DwgVersion::kAc1012: return "AC1012";
        case DwgVersion::kAc1014: return "AC1014";
        case DwgVersion::kAc1015: return "AC1015";
        case DwgVersion::kAc1018: return "AC1018";
        case DwgVersion::kAc1021: return "AC1021";
        case DwgVersion::kAc1024: return "AC1024";
        case DwgVersion::kAc1027: return "AC1027";
        case DwgVersion::kAc1032: return "AC1032";
        default: return "?";
    }
}

/// 中文版本名（R2004 之类）。
inline const char* versionLabel(DwgVersion v) {
    switch (v) {
        case DwgVersion::kAc1009: return "R11/R12";
        case DwgVersion::kAc1012: return "R13";
        case DwgVersion::kAc1014: return "R14";
        case DwgVersion::kAc1015: return "R2000";
        case DwgVersion::kAc1018: return "R2004";
        case DwgVersion::kAc1021: return "R2007";
        case DwgVersion::kAc1024: return "R2010";
        case DwgVersion::kAc1027: return "R2013";
        case DwgVersion::kAc1032: return "R2018";
        default: return "unknown";
    }
}

}  // namespace dwr
