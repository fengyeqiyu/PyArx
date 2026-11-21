#include "dwg_reader/db/entity/entities.h"

#include "dwg_reader/db/entity/encode.h"
#include "dwg_reader/io/codec/bitstream.h"
#include "dwg_reader/io/codec/bitwriter.h"
#include <cstdio>
#include <cstdlib>

namespace dwr {

namespace {
// R13/R14 的 BT/BE 无快捷位（libredwg bits.c：version < R2000 时 bit_write_BT 直接写 BD、
// bit_write_BE 直接写 3BD），R2000+ 才有 1 位标志 + 可选 BD。
inline void readThickness(BitStream& b, DwgVersion v, double& t) {
    t = isR13R14(v) ? b.read_bd() : b.read_bt();
}
inline void writeThickness(BitWriter& w, DwgVersion v, double t) {
    if (isR13R14(v)) w.bd(t); else w.bt(t);
}
inline void readExtrusion(BitStream& b, DwgVersion v, Vector3d& e) {
    if (isR13R14(v)) {
        e.x = b.read_bd(); e.y = b.read_bd(); e.z = b.read_bd();
    } else {
        b.read_be(e.x, e.y, e.z);
    }
}
inline void writeExtrusion(BitWriter& w, DwgVersion v, const Vector3d& e) {
    if (isR13R14(v)) {
        w.bd(e.x); w.bd(e.y); w.bd(e.z);
    } else {
        w.be(e.x, e.y, e.z);
    }
}
}  // namespace

// ---- LINE ----
// R13/R14: 3BD start + 3BD end；R2000+: B z_is_zero + RD/DD 差分编码。
// 两段之后恒有 BT0 thickness + BE extrusion（dwg.spec: DWG_ENTITY (LINE)）。
void LineEntity::readGeom(BitStream& b, BitStream& h, BitStream* s) {
    (void)h;
    (void)s;
    if (isR13R14(version)) {
        b.read_3bd(startPoint.x, startPoint.y, startPoint.z);
        b.read_3bd(endPoint.x, endPoint.y, endPoint.z);
    } else {
        const bool flat = b.read_b() != 0;
        startPoint.x = b.read_rd();
        endPoint.x = b.read_dd(startPoint.x);
        startPoint.y = b.read_rd();
        endPoint.y = b.read_dd(startPoint.y);
        if (flat) {
            startPoint.z = 0.0;
            endPoint.z = 0.0;
        } else {
            startPoint.z = b.read_rd();
            endPoint.z = b.read_dd(startPoint.z);
        }
    }
    readThickness(b, version, thickness);
    readExtrusion(b, version, extrusion);
}

void LineEntity::writeGeom(BitWriter& w) const {
    const char* tr = getenv("DWR_TRACE");
    auto mark = [&](const char* tag) {
        if (tr && handle >= 4990 && handle <= 5020)
            fprintf(stderr, "TRG h=%llu %-10s %zu\n", (unsigned long long)handle, tag, w.nbits);
    };
    if (isR13R14(version)) {
        w.bd(startPoint.x); w.bd(startPoint.y); w.bd(startPoint.z);
        mark("sx,sy,sz");
        w.bd(endPoint.x); w.bd(endPoint.y); w.bd(endPoint.z);
        mark("ex,ey,ez");
    } else {
        const bool flat = (startPoint.z == 0.0 && endPoint.z == 0.0);
        w.b(flat ? 1 : 0);
        w.rd(startPoint.x);
        w.dd(endPoint.x, startPoint.x);
        w.rd(startPoint.y);
        w.dd(endPoint.y, startPoint.y);
        if (!flat) {
            w.rd(startPoint.z);
            w.dd(endPoint.z, startPoint.z);
        }
    }
    writeThickness(w, version, thickness);
    mark("thickness");
    writeExtrusion(w, version, extrusion);
    mark("extrusion");
}

// ---- CIRCLE ----
void CircleEntity::readGeom(BitStream& b, BitStream& h, BitStream* s) {
    (void)h;
    (void)s;
    b.read_3bd(center.x, center.y, center.z);
    radius = b.read_bd();
    readThickness(b, version, thickness);
    readExtrusion(b, version, normal);
}

void CircleEntity::writeGeom(BitWriter& w) const {
    w.bd(center.x); w.bd(center.y); w.bd(center.z);
    w.bd(radius);
    writeThickness(w, version, thickness);
    writeExtrusion(w, version, normal);
}

// ---- ARC ----
void ArcEntity::readGeom(BitStream& b, BitStream& h, BitStream* s) {
    (void)h;
    (void)s;
    b.read_3bd(center.x, center.y, center.z);
    radius = b.read_bd();
    readThickness(b, version, thickness);
    readExtrusion(b, version, normal);
    startAngle = b.read_bd();
    endAngle = b.read_bd();
}

void ArcEntity::writeGeom(BitWriter& w) const {
    w.bd(center.x); w.bd(center.y); w.bd(center.z);
    w.bd(radius);
    writeThickness(w, version, thickness);
    writeExtrusion(w, version, normal);
    w.bd(startAngle);
    w.bd(endAngle);
}

// ---- ELLIPSE ----
// dwg.spec 2444-2457：center(3BD) → sm_axis(3BD) → extrusion(3BD/BE) →
// axis_ratio(BD) → start_angle(BD) → end_angle(BD)。与 CIRCLE/ARC 不同，
// AcDbEllipse 无 thickness 字段（spec 块内无 FIELD_BT0/BT）。
void EllipseEntity::readGeom(BitStream& b, BitStream& h, BitStream* s) {
    (void)h;
    (void)s;
    b.read_3bd(center.x, center.y, center.z);
    b.read_3bd(smAxis.x, smAxis.y, smAxis.z);
    // R13/R14 的 extrusion 为 3BD；R2000+ 按 BE（1 位快捷 + 3BD），与 CIRCLE/ARC 一致。
    if (isR13R14(version)) {
        normal.x = b.read_bd(); normal.y = b.read_bd(); normal.z = b.read_bd();
    } else {
        b.read_be(normal.x, normal.y, normal.z);
    }
    axisRatio = b.read_bd();
    startAngle = b.read_bd();
    endAngle = b.read_bd();
}

void EllipseEntity::writeGeom(BitWriter& w) const {
    w.bd(center.x); w.bd(center.y); w.bd(center.z);
    w.bd(smAxis.x); w.bd(smAxis.y); w.bd(smAxis.z);
    if (isR13R14(version)) {
        w.bd(normal.x); w.bd(normal.y); w.bd(normal.z);
    } else {
        w.be(normal.x, normal.y, normal.z);
    }
    w.bd(axisRatio);
    w.bd(startAngle);
    w.bd(endAngle);
}

// ---- POINT ----
void PointEntity::readGeom(BitStream& b, BitStream& h, BitStream* s) {
    (void)h;
    (void)s;
    position.x = b.read_bd();
    position.y = b.read_bd();
    position.z = b.read_bd();
    readThickness(b, version, thickness);
    readExtrusion(b, version, normal);
    ecsRotation = b.read_bd();
}

void PointEntity::writeGeom(BitWriter& w) const {
    w.bd(position.x); w.bd(position.y); w.bd(position.z);
    writeThickness(w, version, thickness);
    writeExtrusion(w, version, normal);
    w.bd(ecsRotation);
}

// ---- BLOCK ----
// R13..R2000: 数据段只有 BLOCK_NAME（T 型串 = BS 长度 + RC 字节，libredwg
// bit_read_T -> bit_read_TV）；R2007+ 的 T 是 UCS-2，映射键另存于字符串流，
// 本轮只在 R13..R2000 落地，R2004+ 先留空以免误读。
void BlockEntity::readGeom(BitStream& b, BitStream& h, BitStream* s) {
    (void)h;
    (void)s;
    if (version == DwgVersion::kAc1012 || version == DwgVersion::kAc1014 ||
        version == DwgVersion::kAc1015) name = b.read_tv();
}

void BlockEntity::writeGeom(BitWriter& w) const {
    if (version == DwgVersion::kAc1012 || version == DwgVersion::kAc1014 ||
        version == DwgVersion::kAc1015) w.tv(name);
}

// ---- INSERT ----
namespace {
HandleRef readHandleRef(BitStream& h) {
    HandleRef r;
    const int c = h.read_rc();
    r.code = (c >> 4) & 0x0F;
    const int sz = c & 0x0F;
    uint64_t v = 0;
    for (int i = 0; i < sz; ++i) v = (v << 8) | static_cast<uint64_t>(h.read_rc());
    r.value = v;
    return r;
}
}  // namespace

void InsertEntity::readGeom(BitStream& b, BitStream& h, BitStream* s) {
    (void)h;
    (void)s;
    if (isR13R14(version)) {
        b.read_3bd(insertionPoint.x, insertionPoint.y, insertionPoint.z);
        b.read_3bd(scale.x, scale.y, scale.z);
        rotation = b.read_bd();
        b.read_3bd(blockExtrusion.x, blockExtrusion.y, blockExtrusion.z);
        hasAttribs = b.read_b();
    } else if (version == DwgVersion::kAc1015) {
        b.read_3bd(insertionPoint.x, insertionPoint.y, insertionPoint.z);
        const int flag = b.read_bb();  // R2000+ 的 scale_flag 快捷编码
        if (flag == 3) {
            scale.x = scale.y = scale.z = 1.0;
        } else if (flag == 2) {
            const double v = b.read_rd();
            scale.x = scale.y = scale.z = v;
        } else if (flag == 1) {
            scale.x = 1.0;
            scale.y = b.read_dd(1.0);
            scale.z = b.read_dd(1.0);
        } else {
            scale.x = b.read_rd();
            scale.y = b.read_dd(scale.x);
            scale.z = b.read_dd(scale.x);
        }
        rotation = b.read_bd();
        b.read_3bd(blockExtrusion.x, blockExtrusion.y, blockExtrusion.z);
        hasAttribs = b.read_b();
    }
}

void InsertEntity::writeGeom(BitWriter& w) const {
    if (isR13R14(version)) {
        w.bd(insertionPoint.x); w.bd(insertionPoint.y); w.bd(insertionPoint.z);
        w.bd(scale.x); w.bd(scale.y); w.bd(scale.z);
        w.bd(rotation);
        w.bd(blockExtrusion.x); w.bd(blockExtrusion.y); w.bd(blockExtrusion.z);
        w.b(hasAttribs);
    } else if (version == DwgVersion::kAc1015) {
        w.bd(insertionPoint.x); w.bd(insertionPoint.y); w.bd(insertionPoint.z);
        if (scale.x == 1.0 && scale.y == 1.0 && scale.z == 1.0) {
            w.bb(3);
        } else if (scale.x == scale.y && scale.x == scale.z) {
            w.bb(2);
            w.rd(scale.x);
        } else if (scale.x == 1.0) {
            w.bb(1);
            w.dd(scale.y, 1.0);
            w.dd(scale.z, 1.0);
        } else {
            w.bb(0);
            w.rd(scale.x);
            w.dd(scale.y, scale.x);
            w.dd(scale.z, scale.x);
        }
        w.bd(rotation);
        w.bd(blockExtrusion.x); w.bd(blockExtrusion.y); w.bd(blockExtrusion.z);
        w.b(hasAttribs);
    }
}

void InsertEntity::readExtraHandles(BitStream& h) {
    if (version != DwgVersion::kAc1012 && version != DwgVersion::kAc1014 &&
        version != DwgVersion::kAc1015)
        return;
    blockHeader = readHandleRef(h);
    if (hasAttribs) {
        firstAttrib = readHandleRef(h);
        lastAttrib = readHandleRef(h);
        seqend = readHandleRef(h);
    }
}

void InsertEntity::writeExtraHandles(BitWriter& w) const {
    if (version != DwgVersion::kAc1012 && version != DwgVersion::kAc1014 &&
        version != DwgVersion::kAc1015)
        return;
    writeHandleRef(w, blockHeader.code, blockHeader.value);
    if (hasAttribs) {
        writeHandleRef(w, firstAttrib.code, firstAttrib.value);
        writeHandleRef(w, lastAttrib.code, lastAttrib.value);
        writeHandleRef(w, seqend.code, seqend.value);
    }
}

// ---- TEXT ----
// R13/R14 数据段顺序（dwg.spec TEXT 的非 DXF 分支）：elevation BD、ins_pt 2RD、
// alignment_pt 2RD、extrusion 3BD、thickness/oblique/rotation BD0、height BD、
// width_factor BD、text_value TV、generation/horiz/vert BS。
void TextEntity::readGeom(BitStream& b, BitStream& h, BitStream* s) {
    (void)h;
    (void)s;
    if (version == DwgVersion::kAc1015) {
        // R2000：RC dataflags（bit=1 表示省略该字段，见 dwg.spec SINCE(R_2000b) 块）
        dataFlags = b.read_rc();
        if (!(dataFlags & 0x01)) elevation = b.read_rd();
        insertionPoint.x = b.read_rd();
        insertionPoint.y = b.read_rd();
        if (!(dataFlags & 0x02)) {
            alignmentX = b.read_dd(insertionPoint.x);
            alignmentY = b.read_dd(insertionPoint.y);
        } else {
            alignmentX = insertionPoint.x;
            alignmentY = insertionPoint.y;
        }
        b.read_be(textExtrusion.x, textExtrusion.y, textExtrusion.z);
        thicknessValue = b.read_bt();
        if (!(dataFlags & 0x04)) obliqueAngle = b.read_rd();
        if (!(dataFlags & 0x08)) rotation = b.read_rd();
        height = b.read_rd();
        if (!(dataFlags & 0x10))
            widthFactor = b.read_rd();
        else
            widthFactor = 1.0;
        textValue = b.read_tv();
        if (!(dataFlags & 0x20)) generation = b.read_bs();
        if (!(dataFlags & 0x40)) horizAlignment = b.read_bs();
        if (!(dataFlags & 0x80)) vertAlignment = b.read_bs();
        return;
    }
    if (!isR13R14(version)) return;
    elevation = b.read_bd();
    b.read_2rd(insertionPoint.x, insertionPoint.y);
    insertionPoint.z = 0.0;
    b.read_2rd(alignmentX, alignmentY);
    b.read_3bd(textExtrusion.x, textExtrusion.y, textExtrusion.z);
    thicknessValue = b.read_bd();
    obliqueAngle = b.read_bd();
    rotation = b.read_bd();
    height = b.read_bd();
    widthFactor = b.read_bd();
    textValue = b.read_tv();
    generation = b.read_bs();
    horizAlignment = b.read_bs();
    vertAlignment = b.read_bs();
}

void TextEntity::writeGeom(BitWriter& w) const {
    if (version == DwgVersion::kAc1015) {
        w.rc(dataFlags);
        if (!(dataFlags & 0x01)) w.rd(elevation);
        w.rd(insertionPoint.x); w.rd(insertionPoint.y);
        if (!(dataFlags & 0x02)) {
            w.dd(alignmentX, insertionPoint.x);
            w.dd(alignmentY, insertionPoint.y);
        }
        w.be(textExtrusion.x, textExtrusion.y, textExtrusion.z);
        w.bt(thicknessValue);
        if (!(dataFlags & 0x04)) w.rd(obliqueAngle);
        if (!(dataFlags & 0x08)) w.rd(rotation);
        w.rd(height);
        if (!(dataFlags & 0x10)) w.rd(widthFactor);
        w.tv(textValue);
        if (!(dataFlags & 0x20)) w.bs(generation);
        if (!(dataFlags & 0x40)) w.bs(horizAlignment);
        if (!(dataFlags & 0x80)) w.bs(vertAlignment);
        return;
    }
    if (!isR13R14(version)) return;
    w.bd(elevation);
    w.rd(insertionPoint.x); w.rd(insertionPoint.y);
    w.rd(alignmentX); w.rd(alignmentY);
    w.bd(textExtrusion.x); w.bd(textExtrusion.y); w.bd(textExtrusion.z);
    w.bd(thicknessValue);
    w.bd(obliqueAngle);
    w.bd(rotation);
    w.bd(height);
    w.bd(widthFactor);
    w.tv(textValue);
    w.bs(generation);
    w.bs(horizAlignment);
    w.bs(vertAlignment);
}

void TextEntity::readExtraHandles(BitStream& h) {
    if (!isR13R14(version) && version != DwgVersion::kAc1015) return;
    style = readHandleRef(h);
}

void TextEntity::writeExtraHandles(BitWriter& w) const {
    if (!isR13R14(version) && version != DwgVersion::kAc1015) return;
    writeHandleRef(w, style.code, style.value);
}

// ---- ATTRIB ----
// AcDbText 全部字段之后追加（dwg.spec ATTRIB 的 SINCE (R_13b1) 块）：
// T tag、BS0 field_length、RC flags；R2007+ 还有 1 位 lock_position_flag。
void AttribEntity::readGeom(BitStream& b, BitStream& h, BitStream* s) {
    TextEntity::readGeom(b, h, s);
    if (!isR13R14(version) && version != DwgVersion::kAc1015) return;
    tag = b.read_tv();
    fieldLength = b.read_bs();
    flags = b.read_rc();
}

void AttribEntity::writeGeom(BitWriter& w) const {
    TextEntity::writeGeom(w);
    if (!isR13R14(version) && version != DwgVersion::kAc1015) return;
    w.tv(tag);
    w.bs(fieldLength);
    w.rc(flags);
}

// ---- VERTEX_2D ----
// 宽度压缩：起止宽度相等且非 0 时只写一个负的 start_width（libredwg VERTEX_2D ENCODER）。
void Vertex2dEntity::readGeom(BitStream& b, BitStream& h, BitStream* s) {
    (void)h;
    (void)s;
    if (!(isR13R14(version) || version == DwgVersion::kAc1015)) return;
    flag = b.read_rc();
    b.read_3bd(point.x, point.y, point.z);
    startWidth = b.read_bd();
    if (startWidth < 0.0) {
        startWidth = -startWidth;
        endWidth = startWidth;
    } else {
        endWidth = b.read_bd();
    }
    bulge = b.read_bd();
    tangentDir = b.read_bd();
}

void Vertex2dEntity::writeGeom(BitWriter& w) const {
    if (!(isR13R14(version) || version == DwgVersion::kAc1015)) return;
    w.rc(flag);
    w.bd(point.x); w.bd(point.y); w.bd(point.z);
    if (startWidth != 0.0 && startWidth == endWidth) {
        w.bd(-startWidth);
    } else {
        w.bd(startWidth);
        w.bd(endWidth);
    }
    w.bd(bulge);
    w.bd(tangentDir);
}

// ---- ATTDEF ----
// AcDbText 字段之后（dwg.spec ATTDEF 的 SINCE (R_13b1) 块）：
// T tag、BS field_length、RC flags、T prompt（顺序与 ATTRIB 不同：prompt 在最后）。
void AttdefEntity::readGeom(BitStream& b, BitStream& h, BitStream* s) {
    TextEntity::readGeom(b, h, s);
    if (!isR13R14(version) && version != DwgVersion::kAc1015) return;
    tag = b.read_tv();
    fieldLength = b.read_bs();
    flags = b.read_rc();
    prompt = b.read_tv();
}

void AttdefEntity::writeGeom(BitWriter& w) const {
    TextEntity::writeGeom(w);
    if (!isR13R14(version) && version != DwgVersion::kAc1015) return;
    w.tv(tag);
    w.bs(fieldLength);
    w.rc(flags);
    w.tv(prompt);
}

// ---- DICTIONARY（非实体对象，AcDbDictionary）----
// dwg.spec 2644-2724 行（非 DXF 分支，R13b1 起）：
//   FIELD_BL numitems → SINCE(R_2000b) FIELD_BS cloning → FIELD_RC is_hardowner
//   → FIELD_VECTOR_T(texts, numitems) → 句柄流 HANDLE_VECTOR_N(itemhandles, numitems)。
void DictionaryObject::readGeom(BitStream& b, BitStream& h, BitStream* s) {
    (void)h;
    (void)s;
    if (version != DwgVersion::kAc1015) return;
    numItems = static_cast<int32_t>(b.read_bl());
    if (numItems < 0 || numItems > 10000) numItems = 0;
    cloning = b.read_bs();
    isHardOwner = b.read_rc();
    texts.clear();
    texts.resize(static_cast<size_t>(numItems));
    for (int32_t i = 0; i < numItems; ++i) texts[static_cast<size_t>(i)] = b.read_tv();
}

void DictionaryObject::writeGeom(BitWriter& w) const {
    if (version != DwgVersion::kAc1015) return;
    w.bl(static_cast<uint32_t>(numItems));
    w.bs(cloning);
    w.rc(isHardOwner);
    for (int32_t i = 0; i < numItems; ++i)
        w.tv(i < static_cast<int32_t>(texts.size()) ? texts[static_cast<size_t>(i)]
                                                    : std::string());
}

void DictionaryObject::readExtraHandles(BitStream& h) {
    if (version != DwgVersion::kAc1015) return;
    itemHandles.clear();
    itemHandles.reserve(static_cast<size_t>(numItems));
    for (int32_t i = 0; i < numItems; ++i) itemHandles.push_back(readHandleRef(h));
}

void DictionaryObject::writeExtraHandles(BitWriter& w) const {
    if (version != DwgVersion::kAc1015) return;
    for (int32_t i = 0; i < numItems; ++i) {
        const HandleRef r = (static_cast<size_t>(i) < itemHandles.size())
                                ? itemHandles[static_cast<size_t>(i)]
                                : HandleRef{};
        int sz = 0;
        for (uint64_t t = r.value; t; t >>= 8) ++sz;
        w.rc(((r.code & 0x0F) << 4) | (sz & 0x0F));
        for (int k = sz - 1; k >= 0; --k)
            w.rc(static_cast<int>((r.value >> (8 * k)) & 0xFF));
    }
}

// ---- LAYER（表对象，AcDbLayerTableRecord，R2000 口径）----
// dwg.spec 3250-3402 行：COMMON_TABLE_FLAGS 的数据段 name(T)+is_xref_ref(B)
// +is_xref_resolved(BS)+is_xref_dep(B)+xref(H，数据段内)；随后 flag0(BSx)、color(CMC 索引)；
// START_OBJECT_HANDLE_STREAM 之后追加 plotstyle、ltype。
void LayerObject::readGeom(BitStream& b, BitStream& h, BitStream* s) {
    (void)h;
    (void)s;
    if (version != DwgVersion::kAc1015) return;
    // 字段口径未定（name/xref/flag0/color 的位序与 R2000 实测不符），先整段保留数据段位串，
    // 由 writeGeom 逐位回放，保证 bitsize 与字节一致；句柄流仍按 owner→reactors→xdic→plotstyle→ltype。
    const uint64_t start = b.tell();
    if (dataSectionEndBit > start) {
        rawDataBits = static_cast<uint32_t>(dataSectionEndBit - start);
        rawData.assign((rawDataBits + 7) / 8, 0);
        for (uint32_t i = 0; i < rawDataBits; ++i) {
            const uint64_t g = start + i;
            const uint8_t bit = (b.data_[g / 8] >> (7 - (g % 8))) & 1;
            rawData[i / 8] |= static_cast<uint8_t>(bit << (7 - (i % 8)));
        }
    }
}

void LayerObject::writeGeom(BitWriter& w) const {
    if (version != DwgVersion::kAc1015) return;
    for (uint32_t i = 0; i < rawDataBits; ++i) {
        const uint8_t bit = (i / 8 < rawData.size())
                                ? ((rawData[i / 8] >> (7 - (i % 8))) & 1)
                                : 0;
        w.b(bit);
    }
}

void LayerObject::readExtraHandles(BitStream& h) {
    if (version != DwgVersion::kAc1015) return;
    plotstyle = readHandleRef(h);
    ltype = readHandleRef(h);
}

void LayerObject::writeExtraHandles(BitWriter& w) const {
    if (version != DwgVersion::kAc1015) return;
    for (const HandleRef& r : {plotstyle, ltype}) {
        int sz = 0;
        for (uint64_t t = r.value; t; t >>= 8) ++sz;
        w.rc(((r.code & 0x0F) << 4) | (sz & 0x0F));
        for (int k = sz - 1; k >= 0; --k) w.rc(static_cast<int>((r.value >> (8 * k)) & 0xFF));
    }
}

// ---- BLOCK_HEADER（表对象，AcDbBlockTableRecord，R2000 口径）----
// dwg.spec 3098-3217 行：COMMON_TABLE_FLAGS 数据段 + anonymous/hasattrs/blkisxref/
// xrefoverlaid/xref_loaded + base_pt(3BD) + xref_pname(T) + num_inserts(RL) +
// description(T) + preview_size(BL)+preview；句柄流 block_entity(3)/first_entity(4)/last_entity(4)。
// 数据段先整段位串回放（与 LAYER 同法），句柄流按规范建模。
void BlockHeaderObject::readGeom(BitStream& b, BitStream& h, BitStream* s) {
    (void)h;
    (void)s;
    if (version != DwgVersion::kAc1015) return;
    const uint64_t start = b.tell();
    if (dataSectionEndBit > start) {
        rawDataBits = static_cast<uint32_t>(dataSectionEndBit - start);
        rawData.assign((rawDataBits + 7) / 8, 0);
        for (uint32_t i = 0; i < rawDataBits; ++i) {
            const uint64_t g = start + i;
            const uint8_t bit = (b.data_[g / 8] >> (7 - (g % 8))) & 1;
            rawData[i / 8] |= static_cast<uint8_t>(bit << (7 - (i % 8)));
        }
    }
}

void BlockHeaderObject::writeGeom(BitWriter& w) const {
    if (version != DwgVersion::kAc1015) return;
    for (uint32_t i = 0; i < rawDataBits; ++i) {
        const uint8_t bit = (i / 8 < rawData.size())
                                ? ((rawData[i / 8] >> (7 - (i % 8))) & 1)
                                : 0;
        w.b(bit);
    }
}

static void writeHref(BitWriter& w, const HandleRef& r) {
    int sz = 0;
    for (uint64_t t = r.value; t; t >>= 8) ++sz;
    w.rc(((r.code & 0x0F) << 4) | (sz & 0x0F));
    for (int k = sz - 1; k >= 0; --k) w.rc(static_cast<int>((r.value >> (8 * k)) & 0xFF));
}

void BlockHeaderObject::readExtraHandles(BitStream& h) {
    if (version != DwgVersion::kAc1015) return;
    blockEntity = readHandleRef(h);
    firstEntity = readHandleRef(h);
    lastEntity = readHandleRef(h);
}

void BlockHeaderObject::writeExtraHandles(BitWriter& w) const {
    if (version != DwgVersion::kAc1015) return;
    writeHref(w, blockEntity);
    writeHref(w, firstEntity);
    writeHref(w, lastEntity);
}

// ---- LWPOLYLINE（77）----
void LwPolylineObject::readGeom(BitStream& b, BitStream& h, BitStream* s) {
    (void)h;
    (void)s;
    flag = b.read_bs();
    if (flag & 4) constWidth = b.read_bd();
    if (flag & 8) elevation = b.read_bd();
    if (flag & 2) thickness = b.read_bd();
    if (flag & 1) b.read_3bd(ex, ey, ez);
    numPoints = static_cast<int>(b.read_bl());
    numBulges = (flag & 16) ? static_cast<int>(b.read_bl()) : 0;
    numWidths = (flag & 32) ? static_cast<int>(b.read_bl()) : 0;
    points.clear();
    if (numPoints > 0 && numPoints <= 20000) {
        Pt p;
        b.read_2rd(p.x, p.y);  // 首点：两个原始 double
        points.push_back(p);
        for (int i = 1; i < numPoints; ++i) {
            const Pt prev = points.back();
            Pt q;
            q.x = b.read_dd(prev.x);  // 其余点：相对前一点的 DD 增量编码
            q.y = b.read_dd(prev.y);
            points.push_back(q);
        }
    }
    bulges.clear();
    for (int i = 0; i < numBulges; ++i) bulges.push_back(b.read_bd());
    widths.clear();
    for (int i = 0; i < numWidths; ++i) {
        Pt w;
        w.x = b.read_bd();
        w.y = b.read_bd();
        widths.push_back(w);
    }
}

void LwPolylineObject::writeGeom(BitWriter& w) const {
    w.bs(flag);
    if (flag & 4) w.bd(constWidth);
    if (flag & 8) w.bd(elevation);
    if (flag & 2) w.bd(thickness);
    if (flag & 1) {
        w.bd(ex);
        w.bd(ey);
        w.bd(ez);
    }
    w.bl(static_cast<uint32_t>(numPoints));
    if (flag & 16) w.bl(static_cast<uint32_t>(numBulges));
    if (flag & 32) w.bl(static_cast<uint32_t>(numWidths));
    if (!points.empty()) {
        w.rd(points[0].x);
        w.rd(points[0].y);
        for (size_t i = 1; i < points.size(); ++i) {
            w.dd(points[i].x, points[i - 1].x);
            w.dd(points[i].y, points[i - 1].y);
        }
    }
    for (size_t i = 0; i < bulges.size(); ++i) w.bd(bulges[i]);
    for (size_t i = 0; i < widths.size(); ++i) {
        w.bd(widths[i].x);
        w.bd(widths[i].y);
    }
}

// ---- AcDbSymbolTableRecord（COMMON_TABLE_FLAGS）----
void SymbolTableRecord::readGeom(BitStream& b, BitStream& h, BitStream* s) {
    (void)h;
    (void)s;
    name = b.read_tv();
    isXrefRef = b.read_b();
    isXrefResolved = b.read_bs();
    isXrefDep = b.read_b();
    xref = b.read_handle();
}

void SymbolTableRecord::writeGeom(BitWriter& w) const {
    w.tv(name);
    w.b(isXrefRef ? 1 : 0);
    w.bs(isXrefResolved);
    w.b(isXrefDep ? 1 : 0);
    w.handle(xref);
}

// ---- AcDbTextStyleTableRecord（STYLE）----
void TextStyleTableRecord::readGeom(BitStream& b, BitStream& h, BitStream* s) {
    SymbolTableRecord::readGeom(b, h, s);
    isShape = b.read_b();
    isVertical = b.read_b();
    textSize = b.read_bd();
    widthFactor = b.read_bd();
    obliqueAngle = b.read_bd();
    generation = b.read_rc();
    lastHeight = b.read_bd();
    fontFile = b.read_tv();
    bigFontFile = b.read_tv();
}

void TextStyleTableRecord::writeGeom(BitWriter& w) const {
    SymbolTableRecord::writeGeom(w);
    w.b(isShape ? 1 : 0);
    w.b(isVertical ? 1 : 0);
    w.bd(textSize);
    w.bd(widthFactor);
    w.bd(obliqueAngle);
    w.rc(generation);
    w.bd(lastHeight);
    w.tv(fontFile);
    w.tv(bigFontFile);
}

// ---- 通用保真类（GenericObject）----
// 数据段：从头部之后到句柄流起点整段位串回放；句柄流：由 decode 侧存入 rawHandles 后整段回放。
void GenericObject::readGeom(BitStream& b, BitStream& h, BitStream* s) {
    (void)h;
    (void)s;
    const uint64_t start = b.tell();
    if (dataSectionEndBit > start) {
        rawDataBits = static_cast<uint32_t>(dataSectionEndBit - start);
        rawData.assign((rawDataBits + 7) / 8, 0);
        for (uint32_t i = 0; i < rawDataBits; ++i) {
            const uint64_t g = start + i;
            const uint8_t bit = (b.data_[g / 8] >> (7 - (g % 8))) & 1;
            rawData[i / 8] |= static_cast<uint8_t>(bit << (7 - (i % 8)));
        }
    }
}

void GenericObject::writeGeom(BitWriter& w) const {
    for (uint32_t i = 0; i < rawDataBits; ++i) {
        const uint8_t bit = (i / 8 < rawData.size())
                                ? ((rawData[i / 8] >> (7 - (i % 8))) & 1)
                                : 0;
        w.b(bit);
    }
}

}  // namespace dwr
