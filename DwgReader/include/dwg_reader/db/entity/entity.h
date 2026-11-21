#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "dwg_reader/rx/object.h"
#include "dwg_reader/value/types.h"
#include "dwg_reader/value/vec3.h"
#include "dwg_reader/ge/vector3d.h"
#include "dwg_reader/value/version.h"

namespace dwr {

class BitStream;
class BitWriter;

/// 通用句柄引用：值 + 事务码（DWG 里每个引用首字节 = (code<<4)|字节数）。
/// 读侧原样保留 code，写侧按 `code<<4|size` 回写才能逐字节一致。
struct HandleRef {
    uint64_t value = 0;
    int code = 0;
};

/// 句柄流引用（owner / reactors / xdic / layer / ltype / plotstyle /
/// material / shadow / visualstyle）。
struct CommonHandles {
    /// 句柄引用的事务码（DWG 里每个引用首字节 = (code<<4)|字节数）：
    /// 2..5 = 绝对句柄，6/8/10/12 = 相对对象句柄的偏移，0 = 空。
    /// 读侧原样保留 code，写侧按 `code<<4|size` 回写，才能与源逐字节一致。
    int ownerCode = 0;
    int xdicCode = 0;
    int prevCode = 0;
    int nextCode = 0;
    int layerCode = 0;
    int ltypeCode = 0;
    int plotstyleCode = 0;
    std::vector<int> reactorCodes;
    uint64_t owner = 0;
    std::vector<uint64_t> reactors;
    uint64_t xdic = 0;
    uint64_t prevEntity = 0;
    uint64_t nextEntity = 0;
    uint64_t layer = 0;
    uint64_t ltype = 0;
    uint64_t plotstyle = 0;
    uint64_t material = 0;
    uint64_t shadow = 0;
    uint64_t fullVisualStyle = 0;
    uint64_t faceVisualStyle = 0;
    uint64_t edgeVisualStyle = 0;
    uint64_t colorHandle = 0;
};

/// 对象公共 dat 段（EED 之后、类型几何之前）。字段随版本增删，见 readCommon。
struct CommonData {
    int color = 256;
    int raw = 0;
    int colorFlags = 0;
    uint32_t rgb = 0;
    uint32_t alpha = 0;
    int entmode = 0;
    int numReactors = 0;
    bool xdicMissing = false;
    bool nolinks = false;
    int ltypeFlags = 0;
    int plotstyleFlags = 0;
    int materialFlags = 0;
    int shadowFlags = 0;
    bool isbylayerlt = false;
    double ltypeScale = 1.0;
    int invisible = 0;
    int linewt = 29;
    bool hasDsData = false;
    bool hasFullVisualStyle = false;
    bool hasFaceVisualStyle = false;
    bool hasEdgeVisualStyle = false;
    CommonHandles handles;
};

/// 实体基类（蓝本 PyDbEntity / AcDbEntity），继承 rx::RxObject。
/// 几何段由子类 readGeom 自读主流(b)与句柄流(h)。
class Entity : public RxObject {
public:
    CommonData common;

/// 解码时注入的版本（readGeom / writeGeom 的位码分支依据）。
    DwgVersion version = DwgVersion::kUnknown;
    /// BT0 厚度（0 表示「按位写 1」）。
    double thickness = 0.0;
    /// BE 挤出方向，默认 (0,0,1) 表示「按位写 1」。
    Vector3d extrusion{0.0, 0.0, 1.0};
    /// 数据段尾部未建模位（bitsize 与已解字段之差）：原样携带、写侧逐位回放。
    std::vector<uint8_t> rawTail;
    uint32_t rawTailBits = 0;
    /// R2000 超长记录（>512 字节）的源字节（含尾部 CRC）：未建模子结构无法逐位复现时原样透传。
    std::string rawRecord;
    /// 字段未建模的类（如 LAYER R2000）把「数据段（头之后到句柄流起点）」整段位串原样携带，
    /// 写侧逐位回放，保证 bitsize 与字节一致；后续可逐字段替换为真实解析。
    std::vector<uint8_t> rawData;
    uint32_t rawDataBits = 0;
    /// 解码期上下文：数据段终点（绝对位），仅用于取 rawData。
    uint64_t dataSectionEndBit = 0;
    /// 记录体终点（绝对位，= 句柄流起点 + bitsize + MS 体），用于取 rawHandles。
    uint64_t recordBodyEndBit = 0;
    /// 未建模句柄序列的半成品类：把「句柄流起点 → 记录体末尾」的整段位串原样携带并回放，
    /// 不做 owner/reactors/xdic/类追加的语义解析（后续可逐类替换）。
    std::vector<uint8_t> rawHandles;
    uint32_t rawHandlesBits = 0;
    /// 记录级：MS 值（记录体字节数，不含 MS 与尾部 CRC）与记录体位长。
    uint32_t msSize = 0;
    uint32_t recBodyBits = 0;
    uint32_t decBits = 0;  // 调试：解码实际用掉的位数（相对数据段起点）

    Entity() = default;
    Entity(uint64_t h, TypeCode tc) : RxObject(h, tc) {}
    ~Entity() override = default;

    /// 子类自写几何段（writeGeom），与 readGeom 严格互逆。
    virtual void writeGeom(BitWriter& w) const { (void)w; }

    /// 子类自读几何段：b=主流位链，h=句柄流位链，s=R2007+ 字符串流（可空）。
    virtual void readGeom(BitStream& b, BitStream& h, BitStream* s) {
        (void)b;
        (void)h;
        (void)s;
    }

    /// 公共句柄流之后、本类型专有的句柄（如 INSERT 的 block_header/attribs/seqend）。
    /// 默认无；子类覆盖后 readGeom 侧同实现。
    virtual void readExtraHandles(BitStream& h) { (void)h; }
    virtual void writeExtraHandles(BitWriter& w) const { (void)w; }

    // ---- 公共属性 getter（对齐 SDK 风格）----
    int colorIndex() const { return common.color; }
    uint64_t ownerId() const { return common.handles.owner; }
    uint64_t layerId() const { return common.handles.layer; }
};

}  // namespace dwr
