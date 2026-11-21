#include "dwg_reader/db/entity/encode.h"

#include "dwg_reader/io/codec/bitwriter.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace dwr {

void copyBits(BitWriter& w, const uint8_t* data, size_t len, uint64_t fromBit, uint64_t toBit) {
    for (uint64_t p = fromBit; p < toBit; ++p) {
        const size_t byte = static_cast<size_t>(p / 8);
        int v = 0;
        if (byte < len) v = (data[byte] >> (7 - (p % 8))) & 1;
        w.putBit(v);
    }
}

namespace {

uint16_t crc16Ibm(const uint8_t* p, size_t n, uint16_t seed = 0xC0C1) {    uint16_t crc = seed;
    for (size_t i = 0; i < n; ++i) {
        crc ^= p[i];
        for (int b = 0; b < 8; ++b) crc = (crc & 1) ? static_cast<uint16_t>((crc >> 1) ^ 0xA001)
                                                     : static_cast<uint16_t>(crc >> 1);
    }
    return crc;
}

uint64_t g_traceHandle = 0;

void writeCmc(BitWriter& w, const CommonData& c) {
    // R13/R14：只有索引；rgb/flag 属于 R2004+ 口径。
    w.bs(c.color);
    (void)c;
}

}  // namespace

void writeCommonR13R14(BitWriter& w, const CommonData& c, uint32_t bitsize) {
    const char* tr = (g_traceHandle != 0) ? "1" : nullptr;
    auto mark = [&](const char* tag) {
        if (tr) fprintf(stderr, "TR %-10s %zu\n", tag, w.nbits);
    };
    w.b(0);        // 预览图存在位：写侧不生成预览
    mark("preview");
    w.rl(bitsize); // 实体数据位长（句柄流起点 = 记录起点 + bitsize）
    mark("bitsize");
    w.bb(c.entmode);
    mark("entmode");
    w.bl(static_cast<uint32_t>(c.numReactors));
    mark("nreactors");
    w.b(c.isbylayerlt ? 1 : 0);
    mark("bylayerlt");
    w.b(c.nolinks ? 1 : 0);  // R13..R2002 恒有该位
    mark("nolinks");
    writeCmc(w, c);  // R13/R14 只写索引
    mark("color");
    w.bd(c.ltypeScale);
    mark("ltscale");
    w.bs(c.invisible);
    mark("invisible");
}

void writeHandleRef(BitWriter& w, int code, uint64_t v) {
    int sz = 0;
    for (uint64_t t = v; t; t >>= 8) ++sz;
    w.rc((code << 4) | (sz & 0x0F));
    for (int i = sz - 1; i >= 0; --i) w.rc(static_cast<int>((v >> (8 * i)) & 0xFF));
}

void writeHandlesR13R14(BitWriter& w, const CommonData& c) {
    const CommonHandles& h = c.handles;
    if (c.entmode == 0) writeHandleRef(w, h.ownerCode, h.owner);
    for (size_t i = 0; i < h.reactors.size(); ++i) {
        const int code = (i < h.reactorCodes.size()) ? h.reactorCodes[i] : 0;
        writeHandleRef(w, code, h.reactors[i]);
    }
    writeHandleRef(w, h.xdicCode, h.xdic);
    writeHandleRef(w, h.layerCode, h.layer);
    if (!c.isbylayerlt) writeHandleRef(w, h.ltypeCode, h.ltype);
    if (!c.nolinks) {
        writeHandleRef(w, h.prevCode, h.prevEntity);
        writeHandleRef(w, h.nextCode, h.nextEntity);
    }
}

// ---- R2000（AC1015）写侧口径 ----
// 对象记录：MS size → BS type → H handle → [EED] → 公共段 → 几何段 → 句柄流 → 补位 → CRC16。
// 与 R13/R14 的差别：公共段无 `RL bitsize` 字段（bitsize 只在 R13b1..R14 存在），
// 且自 R2000 起公共段多 ltype_flags/plotstyle_flags 两个 BB 与 lineweight RC，
// BT/BE 恢复 1 位快捷位（由 entities.cpp 的版本分支处理）。
void writeCommonR2000(BitWriter& w, const CommonData& c) {
    w.b(0);  // 预览图存在位（不生成预览）
    w.bb(c.entmode);
    w.bs(static_cast<uint32_t>(c.numReactors));  // R2000 实测为 BS
    w.b(c.nolinks ? 1 : 0);  // R13..R2002 恒有该位
    w.bs(c.color);           // R2000 的 CMC 仍只有索引
    w.bd(c.ltypeScale);
    w.bb(c.ltypeFlags);
    w.bb(c.plotstyleFlags);
    w.bs(c.invisible);
    w.rc(c.linewt);
}

void writeHandlesR2000(BitWriter& w, const CommonData& c) {
    const CommonHandles& h = c.handles;
    if (c.entmode == 0) writeHandleRef(w, h.ownerCode, h.owner);
    for (size_t i = 0; i < h.reactors.size(); ++i) {
        const int code = (i < h.reactorCodes.size()) ? h.reactorCodes[i] : 0;
        writeHandleRef(w, code, h.reactors[i]);
    }
    writeHandleRef(w, h.xdicCode, h.xdic);   // R2000 恒有 xdic 句柄
    // R2000 的 prev/next 在 xdic 之后、layer 之前（common_entity_handle_data.spec 107-114 段）
    if (!c.nolinks) {
        writeHandleRef(w, h.prevCode, h.prevEntity);
        writeHandleRef(w, h.nextCode, h.nextEntity);
    }
    writeHandleRef(w, h.layerCode, h.layer);
    if (c.ltypeFlags == 3) writeHandleRef(w, h.ltypeCode, h.ltype);
    if (c.plotstyleFlags == 3) writeHandleRef(w, h.plotstyleCode, h.plotstyle);
}

bool encodeObjectR2000(const Entity& e, const uint8_t* data, size_t len,
                       uint64_t recBit, uint64_t eedStartBit, uint64_t eedEndBit,
                       std::vector<uint8_t>& out, uint32_t* outDataBits) {
    (void)recBit;
    // 非实体对象（DICTIONARY/BLOCK_HEADER/LAYER）：公共段只有 BL num_reactors，
    // 句柄流 owner→reactors→xdic→（本类追加），与实体口径不同。
    // ARX 口径：符号表记录/表控制（48..69 段）与 42 字典 = 非图元对象，
    // 数据段不进 common（由 writeGeom 写），句柄流为 owner/reactors/xdic。
    const int tc = static_cast<int>(e.typeCode);
    const bool nonEntity = (tc == 42 || (tc >= 48 && tc <= 69));
    // 通用保真类：尚未语义建模者，整段位串回放。
    const bool genericObj = nonEntity && tc != 42 && tc != 49 && tc != 51 && tc != 53;
    // 两遍：bitsize 相对「type 之后」且 4 字节定宽，只受自身影响，可先量后贴。
    auto writeRest = [&](BitWriter& w, uint32_t bitsize) {
        w.bs(static_cast<int>(e.typeCode));
        w.rl(bitsize);
        w.handle(e.handle);
        const size_t afterHdr = w.nbits;
        copyBits(w, data, len, eedStartBit, eedEndBit);
        const size_t afterEed = w.nbits;
        if (nonEntity) {
            // 只有 DICTIONARY(42) 的数据段以 BL num_reactors 起头；其余非实体类
            // 的数据段由 writeGeom 以 rawData 整段回放。
            if (static_cast<int>(e.typeCode) == 42)
                w.bl(static_cast<uint32_t>(e.common.numReactors));
        } else {
            writeCommonR2000(w, e.common);
        }
        const size_t afterCommon = w.nbits;
        e.writeGeom(w);
        if (getenv("DWR_TRACE2"))
            fprintf(stderr, "[R2K] h=%llu hdr=%zu eed=%zu common=%zu geom=%zu total=%zu\n",
                    (unsigned long long)e.handle, afterHdr, afterEed - afterHdr,
                    afterCommon - afterEed, w.nbits - afterCommon, w.nbits);
    };
    uint32_t bitsize = 0;
    for (int it = 0; it < 3; ++it) {
        BitWriter probe;
        writeRest(probe, bitsize);
        // bitsize 基准 = MS 之后的记录体起点（type 开始处），即 probe 的位数本身。
        const uint32_t now = static_cast<uint32_t>(probe.nbits);
        if (now == bitsize) break;
        bitsize = now;
    }

    BitWriter body;
    writeRest(body, bitsize);
    const uint32_t dataBits = static_cast<uint32_t>(body.nbits);

    if (!e.rawHandles.empty()) {
        // 通用保真类：句柄流（含补位）整段回放。
        for (uint32_t i = 0; i < e.rawHandlesBits; ++i) {
            const uint8_t bit = (i / 8 < e.rawHandles.size())
                                    ? ((e.rawHandles[i / 8] >> (7 - (i % 8))) & 1)
                                    : 0;
            body.b(bit);
        }
    } else {
        if (nonEntity) {
            const CommonHandles& ch = e.common.handles;
            writeHandleRef(body, ch.ownerCode, ch.owner);
            for (size_t i = 0; i < ch.reactors.size(); ++i) {
                const int code = (i < ch.reactorCodes.size()) ? ch.reactorCodes[i] : 0;
                writeHandleRef(body, code, ch.reactors[i]);
            }
            writeHandleRef(body, ch.xdicCode, ch.xdic);
        } else {
            writeHandlesR2000(body, e.common);
        }
        e.writeExtraHandles(body);
        for (uint32_t i = 0; i < e.rawTailBits; ++i) {
            const uint8_t bit = (i / 8 < e.rawTail.size())
                                    ? ((e.rawTail[i / 8] >> (7 - (i % 8))) & 1)
                                    : 0;
            body.b(bit);
        }
    }
    while (body.nbits & 7) body.b(0);  // 记录体按字节对齐

    BitWriter w;
    w.ms(static_cast<uint32_t>(body.bytes.size()));
    while (w.nbits & 7) w.b(0);
    for (uint8_t v : body.bytes) w.rc(v);
    out = w.bytes;
    const uint16_t crc = crc16Ibm(out.data(), out.size());
    out.push_back(static_cast<uint8_t>(crc & 0xFF));
    out.push_back(static_cast<uint8_t>((crc >> 8) & 0xFF));
    // 超长记录（块区/未建模子结构）编码长度明显不及源时，整条原样透传（无损）。
    if (!e.rawRecord.empty() && e.rawRecord.size() > out.size() + 128) {
        out.assign(e.rawRecord.begin(), e.rawRecord.end());
        if (outDataBits) *outDataBits = static_cast<uint32_t>(e.rawRecord.size() * 8);
        return true;
    }
    if (outDataBits) *outDataBits = dataBits;
    return true;
}

bool encodeObjectR13R14(const Entity& e, const uint8_t* data, size_t len,
                        uint64_t recBit, uint64_t eedStartBit, uint64_t eedEndBit,
                        std::vector<uint8_t>& out, uint32_t* outBitsize,
                        uint32_t* outPayloadBits) {
    // bitsize = MS 之后的位数；MS 宽度取决于总字节长，故先量后写（两遍）。
    auto writeRest = [&](BitWriter& w, uint32_t bitsize) {
        w.bs(static_cast<int>(e.typeCode));
        w.handle(e.handle);
        copyBits(w, data, len, eedStartBit, eedEndBit);
        writeCommonR13R14(w, e.common, bitsize);
        e.writeGeom(w);
    };

    // 记录体尾部未建模位（句柄流之后，到 MS 记录体末尾），原样回放。
    auto writeTailBits = [&](BitWriter& w) {
        for (uint32_t i = 0; i < e.rawTailBits; ++i) {
            const uint8_t bit = (i / 8 < e.rawTail.size())
                                    ? ((e.rawTail[i / 8] >> (7 - (i % 8))) & 1)
                                    : 0;
            w.b(bit);
        }
    };

    {
        const char* tr = getenv("DWR_TRACE");
        g_traceHandle = tr ? std::strtoull(tr, nullptr, 10) : 0;
    }
    // bitsize = MS 之后的位数；RL 自身宽度随值变化 → 迭代到不动点。
    uint32_t bitsize = 0;
    for (int it = 0; it < 6; ++it) {
        BitWriter probe;
        writeRest(probe, bitsize);
        const uint32_t got = static_cast<uint32_t>(probe.nbits);
        if (got == bitsize) break;
        bitsize = got;
    }

    // MS = 记录体字节数（源值优先；新建对象按实写位数推算）。
    BitWriter probe2;
    writeRest(probe2, bitsize);
    writeHandlesR13R14(probe2, e.common);
    e.writeExtraHandles(probe2);
    writeTailBits(probe2);
    const uint32_t bodyBytes = e.msSize ? e.msSize
                                        : static_cast<uint32_t>(probe2.bytes.size());

    BitWriter w;
    w.ms(bodyBytes);
    writeRest(w, bitsize);
    writeHandlesR13R14(w, e.common);
    e.writeExtraHandles(w);
    writeTailBits(w);
    // 记录体按 MS 对齐（不足补 0、超出则截断到 MS 长度），随后附加 2 字节 CRC16-IBM。
    BitWriter m;
    m.ms(0);
    const uint32_t msLen = static_cast<uint32_t>(m.bytes.size());
    const uint32_t totalBytes = msLen + bodyBytes;
    if (static_cast<uint32_t>(w.bytes.size()) > totalBytes) w.bytes.resize(totalBytes);
    while (static_cast<uint32_t>(w.bytes.size()) < totalBytes) w.bytes.push_back(0);
    w.nbits = static_cast<size_t>(totalBytes) * 8;
    out = w.bytes;
    const uint16_t crc = crc16Ibm(out.data(), out.size());
    out.push_back(static_cast<uint8_t>(crc & 0xFF));
    out.push_back(static_cast<uint8_t>((crc >> 8) & 0xFF));
    if (outBitsize) *outBitsize = bitsize;
    if (outPayloadBits) *outPayloadBits = static_cast<uint32_t>(w.nbits);
    (void)recBit;
    return true;
}

}  // namespace dwr
