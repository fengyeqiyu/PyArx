#pragma once

namespace dwr {

/// DWG 对象 type code（十进制口径，与离线 Python 基线一致）。
/// 枚举成员统一 k 前缀。
enum class TypeCode : int {
    kText = 1,
    kAttrib = 2,
    kAttdef = 3,
    kBlock = 4,
    kEndblk = 5,
    kSeqend = 6,
    kInsert = 7,
    kVertex = 10,
    kArc = 17,
    kCircle = 18,
    kLine = 19,
    kDimLinear = 20,
    kDimAligned = 21,
    kDimAng2Ln = 22,
    kDimDiametric = 23,
    kDimRadius = 24,
    kDimAng3Pt = 25,
    kDimOrdinate = 26,
    kPoint = 27,
    kDictionary = 42,
    kBlockHeader = 49,
    kLayer = 51,
    kEllipse = 35,
    kSpline = 36,
    kMText = 44,
    kLwPolyline = 77,
    kHatch = 78,
    kTable = 816,
};

}  // namespace dwr
