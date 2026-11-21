#include "dwg_reader/db/entity/decode.h"

#include <cmath>
#include <cstdio>

#include "dwg_reader/db/entity/registry.h"
#include "dwg_reader/io/codec/bitstream.h"

namespace dwr {

namespace {

BitStream atBit(const uint8_t* data, size_t len, uint64_t absBit) {
    BitStream b(data, len);
    if (absBit / 8 <= len) {
        b.byte_ = static_cast<size_t>(absBit / 8);
        b.bit_ = static_cast<int>(absBit % 8);
    } else {
        b.byte_ = len;
        b.bit_ = 0;
    }
    return b;
}

/// 记录尾部的字符串流（R2007+）。口径对齐 decode_r2007.c:obj_string_stream：
/// 最后一位是 has_strings 位，其前 16 位是字符串流位长（bit15 置位时再向前 16 位取高段）。
bool stringStreamRange(const uint8_t* data, size_t len, const ObjectRef& ref,
                       uint64_t& startBit, uint32_t& sizeBits, uint32_t bitsize) {
    if (bitsize < 40) return false;
    const uint64_t recBit = ref.address * 8;
    const uint64_t lastBit = recBit + bitsize - 1;
    BitStream b = atBit(data, len, lastBit);
    if (b.read_b() != 1) return false;  // has_strings

    uint64_t pos = lastBit;      // has_strings 之前
    pos -= 16;                   // 字符串流位长（RS）
    BitStream q = atBit(data, len, pos);
    uint32_t dsize = static_cast<uint32_t>(q.read_rs());
    if (dsize & 0x8000) {
        pos -= 32;               // 高段在前 2 字节
        BitStream hi = atBit(data, len, pos);
        uint32_t hiSize = static_cast<uint32_t>(hi.read_rs());
        dsize = (dsize & 0x7FFF) | (hiSize << 15);
    }
    pos -= 16;                   // 尾部另有 2 字节
    if (dsize == 0 || dsize > bitsize || pos < recBit + 32) return false;
    startBit = pos - dsize;      // 字符串流从 dsize 位之前开始
    if (startBit < recBit) return false;
    sizeBits = dsize;
    return true;
}

}  // namespace

bool locateStringStream(const uint8_t* objects, size_t len, const ObjectRef& ref,
                        uint64_t& startBit, uint32_t& sizeBits) {
    return stringStreamRange(objects, len, ref, startBit, sizeBits, ref.bitsize);
}

void decodeEed(BitStream& b, const uint8_t* data, size_t len) {
    while (true) {
        int size = b.read_bs();
        if (size == 0 || size > 1000) break;

        b.read_handle();  // EED 所属句柄
        size_t end = b.byte_ + static_cast<size_t>(size);

        BitStream q(data, len);
        q.byte_ = b.byte_;
        q.bit_ = b.bit_;

        while (q.byte_ < end) {
            int code = q.read_rc();
            if (code == 70) {
                q.read_rs();
            } else if (code == 71) {
                q.read_rl();
            } else if (code == 3 || code == 5) {
                q.read_rll_be();
            } else if (code == 1) {
                q.read_rs();
            } else if (code == 2) {
                q.read_rc();
            } else if (code == 0) {
                int l = q.read_rc();
                q.read_rs_be();
                q.read_tf(l);
            } else if (code == 4) {
                int l = q.read_rc();
                q.read_tf(l);
            } else if (code >= 10 && code <= 15) {
                for (int k = 0; k < 3; ++k) q.read_rd();
            } else if (code == 40 || code == 41 || code == 42) {
                q.read_rd();
            } else {
                q.byte_ = end;
                q.bit_ = 0;
                break;
            }
        }
        b.byte_ = end;
        b.bit_ = q.bit_;
    }
}

CommonData readCommon(BitStream& b, DwgVersion ver) {
    CommonData c;
    c.ltypeScale = 1.0;

    // 预览图（R13+）：B 存在位 + 长度 + 二进制
    int preview = b.read_b();
    if (preview) {
        uint32_t pvsz = hasHandleStreamSize(ver) ? static_cast<uint32_t>(b.read_bl())
                                                 : b.read_rl();
        b.read_tf(static_cast<int>(pvsz));
    }

    // R13/R14：实体自己的 bitsize 先于公共段
    if (isR13R14(ver)) b.read_rl();

    c.entmode = b.read_bb();
    // R2000/R2002 实测 num_reactors 为 BS（spec 标注 BL 是 ODA bug 的另一面，样本已是 BS）
    if (ver >= DwgVersion::kAc1015 && ver < DwgVersion::kAc1018)
        c.numReactors = b.read_bs();
    else
        c.numReactors = static_cast<int>(b.read_bl());

    if (isR13R14(ver)) {
        c.isbylayerlt = b.read_b() != 0;
        c.ltypeFlags = c.isbylayerlt ? 0 : 3;
    }
    if (ver >= DwgVersion::kAc1018) c.xdicMissing = b.read_b() != 0;   // R2004+
    if (ver <= DwgVersion::kAc1015) c.nolinks = b.read_b() != 0;  // R13..R2002
    if (ver >= DwgVersion::kAc1027) c.hasDsData = b.read_b() != 0;     // R2013+

    if (hasEncColor(ver)) {
        // R2004+：ENC 颜色编码（BS 原始值 + 可选 rgb/alpha/名称）
        int raw = b.read_bs();
        c.raw = raw;
        int flag = raw >> 8;
        c.colorFlags = flag;
        if (flag & 0x40) c.handles.colorHandle = b.read_handle();  // DBCOLOR
        if (flag & 0x80) c.rgb = b.read_bl();
        if (flag & 0x20) c.alpha = b.read_bl();
        if ((flag & 0x41) == 0x41) b.read_tv();
        if ((flag & 0x42) == 0x42) b.read_tv();
        c.color = raw & 0x1ff;
    } else if (ver >= DwgVersion::kAc1018) {
        b.read_cmc(c.color, c.rgb, c.colorFlags);
    } else {
        // R13..R2002：CMC 只有索引 BS，rgb/flag 属于 R2004+（libredwg bit_read_CMC 同口径）
        c.color = b.read_bs();
        c.rgb = 0;
        c.colorFlags = 0;
    }

    c.ltypeScale = b.read_bd();

    if (ver >= DwgVersion::kAc1015) {  // R2000+
        c.ltypeFlags = b.read_bb();
        if (!hasStringStream(ver)) c.plotstyleFlags = b.read_bb();  // R2000..R2004
    }
    if (hasStringStream(ver)) {
        c.materialFlags = b.read_bb();
        c.plotstyleFlags = b.read_bb();
        c.shadowFlags = b.read_rc();
    }
    if (hasHandleStreamSize(ver)) {
        c.hasFullVisualStyle = b.read_b() != 0;
        c.hasFaceVisualStyle = b.read_b() != 0;
        c.hasEdgeVisualStyle = b.read_b() != 0;
    }

    c.invisible = b.read_bs();
    if (ver >= DwgVersion::kAc1015) c.linewt = b.read_rc();
    return c;
}

struct HRef {
    uint64_t value = 0;
    int code = 0;
};

/// 句柄引用：首字节 = (code<<4)|字节数，其后为大端字节值。
HRef readHRef(BitStream& h) {
    const int c = h.read_rc();
    HRef r;
    r.code = c >> 4;
    const int n = c & 0x0F;
    for (int i = 0; i < n && i < 8; ++i) {
        r.value = (r.value << 8) | static_cast<uint64_t>(h.read_rc());
    }
    return r;
}

void readHandles(BitStream& h, CommonData& c, DwgVersion ver) {
    if (c.numReactors < 0 || c.numReactors > 100000) c.numReactors = 0;
    if (c.entmode == 0) {
        const HRef r = readHRef(h);
        c.handles.owner = r.value;
        c.handles.ownerCode = r.code;
    }
    for (int i = 0; i < c.numReactors; ++i) {
        const HRef r = readHRef(h);
        c.handles.reactors.push_back(r.value);
        c.handles.reactorCodes.push_back(r.code);
    }
    if (ver >= DwgVersion::kAc1018) {
        if (!c.xdicMissing) {
            const HRef r = readHRef(h);
            c.handles.xdic = r.value;
            c.handles.xdicCode = r.code;
        }
    } else {
        const HRef r = readHRef(h);  // R13..R2000 恒有 xdic 句柄
        c.handles.xdic = r.value;
        c.handles.xdicCode = r.code;
    }
    const bool r2000 = (ver >= DwgVersion::kAc1015 && ver < DwgVersion::kAc1018);
    if (r2000) {
        // R2000：prev/next 在 xdic 之后、layer 之前（common_entity_handle_data.spec 107-114 段）
        if (!c.nolinks) {
            const HRef p = readHRef(h);
            c.handles.prevEntity = p.value;
            c.handles.prevCode = p.code;
            const HRef n = readHRef(h);
            c.handles.nextEntity = n.value;
            c.handles.nextCode = n.code;
        }
    }
    {
        const HRef r = readHRef(h);
        c.handles.layer = r.value;
        c.handles.layerCode = r.code;
    }
    if (isR13R14(ver)) {
        if (!c.isbylayerlt) {
            const HRef r = readHRef(h);
            c.handles.ltype = r.value;
            c.handles.ltypeCode = r.code;
        }
    }
    if (ver <= DwgVersion::kAc1015 && !r2000) {
        // R13..R2000：layer/ltype 之后才是 prev/next（libredwg common_entity_handle_data.spec）
        if (!c.nolinks) {
            const HRef p = readHRef(h);
            c.handles.prevEntity = p.value;
            c.handles.prevCode = p.code;
            const HRef n = readHRef(h);
            c.handles.nextEntity = n.value;
            c.handles.nextCode = n.code;
        }
    }
    if (isR13R14(ver)) return;
    if (c.ltypeFlags == 3) {
        const HRef r = readHRef(h);
        c.handles.ltype = r.value;
        c.handles.ltypeCode = r.code;
    }
    if (hasStringStream(ver)) {
        if (c.materialFlags == 3) c.handles.material = h.read_handle();
        if (c.shadowFlags == 3) c.handles.shadow = h.read_handle();
    }
    if (c.plotstyleFlags == 3) {
        const HRef r = readHRef(h);
        c.handles.plotstyle = r.value;
        c.handles.plotstyleCode = r.code;
    }
    if (hasHandleStreamSize(ver)) {
        if (c.hasFullVisualStyle) c.handles.fullVisualStyle = h.read_handle();
        if (c.hasFaceVisualStyle) c.handles.faceVisualStyle = h.read_handle();
        if (c.hasEdgeVisualStyle) c.handles.edgeVisualStyle = h.read_handle();
    }
}

/// 非实体对象（DICTIONARY 等）的句柄流：ownerhandle → reactors → xdic
/// （common_object_handle_data.spec 51/55/56 行；控制类无 owner，本轮暂按有 owner 处理）。
void readObjectHandlesR2000(BitStream& h, CommonData& c) {
    if (c.numReactors < 0 || c.numReactors > 100000) c.numReactors = 0;
    const HRef o = readHRef(h);
    c.handles.owner = o.value;
    c.handles.ownerCode = o.code;
    for (int i = 0; i < c.numReactors; ++i) {
        const HRef r = readHRef(h);
        c.handles.reactors.push_back(r.value);
        c.handles.reactorCodes.push_back(r.code);
    }
    const HRef x = readHRef(h);
    c.handles.xdic = x.value;
    c.handles.xdicCode = x.code;
}

std::unique_ptr<Entity> decodeObject(const uint8_t* objects, size_t len,
                                     const ObjectRef& ref, DwgVersion ver) {
    if (!objects || ref.address >= len) return nullptr;
    try {
        BitStream b(objects, len);
        b.byte_ = static_cast<size_t>(ref.address);
        b.bit_ = 0;

        int typeCode = ref.typeCode;
        uint32_t bitsize = ref.bitsize;
        uint64_t handle = ref.handle;
        uint32_t msSize = 0;      // R13..R2000：MS 值（记录体字节数，不含 MS 与尾部 CRC）
        size_t msLenBits = 0;     // MS 自身位长

        // 重读对象头（拿到位游标位置与按版本口径的长度字段）
        if (hasHandleStreamSize(ver)) {
            uint32_t size = b.read_ms();
            uint32_t hdlBits = static_cast<uint32_t>(b.read_umc());
            typeCode = b.read_bot();
            handle = b.read_handle();
            bitsize = size * 8 - hdlBits;
        } else if (isR13R14(ver)) {
            // R13/R14：MS size → BS type → H handle（bitsize 在公共段里，稍后读）
            msSize = static_cast<uint32_t>(b.read_ms());
            msLenBits = b.tell() - static_cast<size_t>(ref.address) * 8;
            typeCode = b.read_bs();
            handle = b.read_handle();
        }

        // R2000/R2002 头部另有 RL bitsize（基准 = MS 之后的记录体起点）。
        if (ver >= DwgVersion::kAc1015 && ver < DwgVersion::kAc1018) {
            msSize = static_cast<uint32_t>(b.read_ms());
            msLenBits = b.tell() - static_cast<size_t>(ref.address) * 8;
            typeCode = b.read_bs();
            bitsize = b.read_rl();
            handle = b.read_handle();
        }

        decodeEed(b, objects, len);

        // 非实体对象（DICTIONARY/BLOCK_HEADER/LAYER）：公共段只有 BL num_reactors，
        // 无 preview/entmode/color 等实体字段（common_object_handle_data.spec 23 行）。
        // ARX 口径：符号表记录（48..69 段）与 42 字典都属「非图元对象」，
        // 公共段只有 BL num_reactors + 句柄流 owner/reactors/xdic。
        const bool tableObj =
            (typeCode == 42 || (typeCode >= 48 && typeCode <= 69));
        const bool nonEntity = tableObj;
        // 通用保真类：尚未语义建模的表对象（数据段与句柄流整段位串回放）。
        const bool genericObj =
            tableObj && typeCode != 42 && typeCode != 49 && typeCode != 51 && typeCode != 53;
        const bool r2000 = (ver >= DwgVersion::kAc1015 && ver < DwgVersion::kAc1018);

        // R13/R14：公共段首字节为 preview 位，随后 RL 即实体数据位长（句柄流起点）。
        if (isR13R14(ver) && !nonEntity) {
            BitStream q = b;
            if (q.read_b()) {  // preview_exists：先跳过预览图数据再读 bitsize
                uint32_t pvsz = static_cast<uint32_t>(q.read_rl());
                q.read_tf(static_cast<int>(pvsz));
            }
            bitsize = q.read_rl();
        }
        CommonData common;
        if (genericObj && r2000) {
            // 通用保真类：数据段整段交给 rawData，此处不消费任何位。
            common.entmode = 0;
        } else if (nonEntity && r2000) {
            // 仅 DICTIONARY(42) 的数据段以 BL num_reactors 起头；LAYER(51)/BLOCK_HEADER(49)
            // 的数据段整段按 rawData 携带，故此处不消费 num_reactors。
            if (typeCode == 42) common.numReactors = static_cast<int>(b.read_bl());
            common.entmode = 0;  // 非实体恒有 ownerhandle
        } else {
            common = readCommon(b, ver);
        }

        // 句柄流：R13/R14 由公共段里的 bitsize 定位（基准 = MS 之后）；
        // R2000/R2002 由头部 RL bitsize 定位，基准同为「MS 之后」（含 type/RL/H 自身位，
        // 实测 130938 源 bitsize=92 → 句柄流 @16+92=108，而非 type 之后 82+92）。
        const uint64_t recDataBit = (ver >= DwgVersion::kAc1015 && ver < DwgVersion::kAc1018)
                                        ? static_cast<uint64_t>(ref.address) * 8 + msLenBits
                                        : ref.address * 8 + 16;
        BitStream h = atBit(objects, len, recDataBit + bitsize);
        if (genericObj && r2000)
            ;  // 句柄流整段保真（下面抓 rawHandles），不做语义解析
        else if (nonEntity && r2000)
            readObjectHandlesR2000(h, common);
        else
            readHandles(h, common, ver);

        auto ent = makeEntity(typeCode, handle);
        if (!ent) return nullptr;

        // R2007+ 字符串流（实体字符串字段从这里取）
        uint64_t strStart = 0;
        uint32_t strSize = 0;
        BitStream s = atBit(objects, len, 0);
        bool hasStr = stringStreamRange(objects, len, ref, strStart, strSize, bitsize);
        if (hasStr) s = atBit(objects, len, strStart);

        ent->version = ver;
        ent->common = std::move(common);
        ent->typeCode = static_cast<TypeCode>(typeCode);
        ent->handle = handle;
        ent->dataSectionEndBit = recDataBit + bitsize;
        ent->readGeom(b, h, hasStr ? &s : nullptr);
        ent->readExtraHandles(h);

        // 通用保真类：句柄流（bitsize 终点 → 记录体末尾，含补位）整段带出。
        const uint64_t bodyEndBitProbe = static_cast<uint64_t>(ref.address) * 8 + msLenBits +
                                         static_cast<uint64_t>(msSize) * 8;
        ent->recordBodyEndBit = bodyEndBitProbe;
        if (genericObj && r2000) {
            const uint64_t hs = recDataBit + bitsize;
            if (bodyEndBitProbe > hs && bodyEndBitProbe <= len * 8) {
                ent->rawHandlesBits = static_cast<uint32_t>(bodyEndBitProbe - hs);
                ent->rawHandles.assign((ent->rawHandlesBits + 7) / 8, 0);
                for (uint32_t i = 0; i < ent->rawHandlesBits; ++i) {
                    const uint64_t g = hs + i;
                    const uint8_t bit = (objects[g / 8] >> (7 - (g % 8))) & 1;
                    ent->rawHandles[i / 8] |= static_cast<uint8_t>(bit << (7 - (i % 8)));
                }
            }
        }

        // 记录体尾部：句柄流结束后到「MS 记录体末尾」（R14 实测每条 3 位补位），原样携带；
        // 记录体末尾之后还有 2 字节 CRC16-IBM（写侧重算）。
        ent->msSize = msSize;
        const size_t hTailStart = h.tell();
        const size_t bodyEndBit = static_cast<size_t>(ref.address) * 8 + msLenBits +
                                  static_cast<size_t>(msSize) * 8;
        ent->recBodyBits = static_cast<uint32_t>(bodyEndBit - recDataBit);
        if (bodyEndBit > hTailStart && bodyEndBit <= len * 8) {
            ent->rawTailBits = static_cast<uint32_t>(bodyEndBit - hTailStart);
            ent->rawTail.assign((ent->rawTailBits + 7) / 8, 0);
            for (uint32_t i = 0; i < ent->rawTailBits; ++i) {
                const size_t g = hTailStart + i;
                const uint8_t bit = (objects[g / 8] >> (7 - (g % 8))) & 1;
                ent->rawTail[i / 8] |= static_cast<uint8_t>(bit << (7 - (i % 8)));
            }
        }
        // R2000 超长记录（块区/未建模子结构，>512 字节）：留源字节做无损透传。
        if (ver >= DwgVersion::kAc1015 && ver < DwgVersion::kAc1018 && msSize > 512) {
            const size_t recBytes = msLenBits / 8 + msSize + 2;
            if (ref.address + recBytes <= len)
                ent->rawRecord.assign(reinterpret_cast<const char*>(objects + ref.address), recBytes);
        }
        return ent;
    } catch (...) {
        return nullptr;  // 单条解码失败隔离，不影响整库装配
    }
}

}  // namespace dwr
