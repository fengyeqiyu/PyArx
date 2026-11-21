// 写侧实体级验证（R13/R14）：解码每个对象 → 用 writeGeom/writeCommon 重编码 →
// 与源记录逐字节比对。用于证明实体序列化口径与 AutoCAD 写入器一致。
#include <cstdio>
#include <cstring>
#include <fstream>
#include <map>
#include <string>
#include <vector>

#include "dwg_reader/db/entity/decode.h"
#include "dwg_reader/db/entity/encode.h"
#include "dwg_reader/db/entity/entities.h"
#include "dwg_reader/db/index/object_index.h"
#include "dwg_reader/io/codec/bitstream.h"
#include "dwg_reader/io/container/classic_container.h"

using namespace dwr;
static int dbg19 = 0;

namespace {

struct Probe {
    int typeCode = 0;
    uint64_t handle = 0;
    uint32_t msSize = 0;
    uint32_t bitsize = 0;
    uint32_t recLen = 0;
    uint32_t entmode = 0;
    uint64_t afterHeader = 0;
    uint64_t afterEed = 0;
};

bool probe(const std::vector<uint8_t>& obj, uint64_t recBit, Probe& p,
           DwgVersion ver = DwgVersion::kAc1014) {
    BitStream b(obj.data(), obj.size());
    b.byte_ = static_cast<size_t>(recBit / 8);
    b.bit_ = static_cast<int>(recBit % 8);
    p.msSize = static_cast<uint32_t>(b.read_ms());
    p.typeCode = b.read_bs();
    const bool r2000hdr = (ver >= DwgVersion::kAc1015 && ver < DwgVersion::kAc1018);
    // 非图元对象（字典 42 / 符号表记录与表控制 48..69）本轮只支持 R2000
    // （R13/R14 的非图元公共段口径未建模，跳过以免污染统计）。
    if (!r2000hdr && (p.typeCode == 42 || (p.typeCode >= 48 && p.typeCode <= 69)))
        return false;
    if (r2000hdr) p.bitsize = b.read_rl();  // R2000/R2002：type 之后是 RL bitsize
    p.handle = b.read_handle();
    p.afterHeader = b.byte_ * 8 + b.bit_;
    decodeEed(b, obj.data(), obj.size());
    p.afterEed = b.byte_ * 8 + b.bit_;
    // 公共段：preview 位 + bitsize(RL) + entmode(BB)
    BitStream q(obj.data(), obj.size());
    q.byte_ = b.byte_;
    q.bit_ = b.bit_;
    if (q.read_b()) {
        uint32_t pvsz = static_cast<uint32_t>(q.read_rl());
        q.read_tf(static_cast<int>(pvsz));
    }
    if (isR13R14(ver)) {
        p.bitsize = q.read_rl();
        if (p.bitsize == 0) p.bitsize = 1;
        p.entmode = q.read_bb();
        return p.bitsize != 0 && p.bitsize < obj.size() * 8;
    }
    // R2000：公共段首部同样有 preview 位，但没有 bitsize(RL)——bitsize 已在对象头里读过。
    p.entmode = q.read_bb();
    return p.bitsize != 0 && p.bitsize < obj.size() * 8;
}

}  // namespace

int main(int argc, char** argv) {
    if (argc < 2) {
        printf("usage: verify_entity_write <r14.dwg> [dump.dwg]\n");
        return 2;
    }
    try {
        {
            std::ifstream f(argv[1], std::ios::binary);
            char ver[7] = {0};
            f.read(ver, 6);
            std::string v(ver);
            if (v != "AC1014" && v != "AC1015") {
                printf("skip: 仅支持 AC1014(R14)/AC1015(R2000)；%s 的实体写侧编码器待实现\n",
                       v.c_str());
                return 0;
            }
        }
        ClassicContainer src = openClassicDwg(argv[1]);
        if (src.ver > DwgVersion::kAc1015) {
            // R2000+ 的对象头是 MS+UMC+bitsize（无 preview 位/RL），编码器尚未实现。
            printf("skip: R2000 及以上实体写侧编码器待实现（本工具仅覆盖 R13/R14）\n");
            return 0;
        }
        const bool r2000 = (src.ver == DwgVersion::kAc1015);
        if (r2000) printf("mode: R2000(AC1015) 实体写侧口径 base=%llu end=%llu\n",
                          (unsigned long long)src.objectBase,
                          (unsigned long long)src.objectEnd);
        const size_t base = static_cast<size_t>(src.objectBase);
        std::vector<uint8_t> obj(src.data.begin() + base, src.data.end());

        std::map<int, size_t> total, matched;
        std::map<int, size_t> shown;
        size_t mismatched = 0, encoded = 0, dataOkCount = 0, dataTotal = 0;
        for (size_t i = 0; i < src.objectAddresses.size(); ++i) {
            const uint64_t addr = src.objectAddresses[i];
            if (addr < src.objectBase || addr >= src.objectEnd) continue;
            Probe p;
            if (!probe(obj, (addr - src.objectBase) * 8, p, src.ver)) {
                if (i < 6) printf("  probe-fail #%zu type=%d bitsize=%u\n", i, p.typeCode, p.bitsize);
                continue;
            }
            ObjectRef ref;
            ref.address = addr - src.objectBase;
            ref.handle = p.handle;
            ref.typeCode = p.typeCode;
            ref.bitsize = p.bitsize;
            auto ent = decodeObject(obj.data(), obj.size(), ref, src.ver);
            if (!ent) {
                if (p.typeCode == 19) printf("  ent19 null #%zu h=%llu bitsize=%u\n", i,
                                             (unsigned long long)p.handle, p.bitsize);
                continue;
            }
            if (p.typeCode == 19) printf("  ent19 ok #%zu h=%llu deg=%d\n", i,
                                         (unsigned long long)p.handle, 0);
            if (p.typeCode == 19) {
                auto* ln = dynamic_cast<LineEntity*>(ent.get());
                if (ln && dbg19 < 3) {
                    ++dbg19;
                    printf("  dbg19 ms=%u used=%u tail=%u h=%llu th=%g ext=(%g,%g,%g) col=%u lt=%g inv=%u em=%u nl=%u bylt=%u prev=%llu next=%llu xd=%llu lay=%llu\n",
                           (unsigned)ent->msSize, (unsigned)ent->decBits, (unsigned)ent->rawTailBits, (unsigned long long)p.handle, ln->thickness, ln->extrusion.x,
                           ln->extrusion.y, ln->extrusion.z, (unsigned)ent->common.color,
                           ent->common.ltypeScale, (unsigned)ent->common.invisible,
                           (unsigned)ent->common.entmode, (unsigned)ent->common.nolinks,
                           (unsigned)ent->common.isbylayerlt,
                           (unsigned long long)ent->common.handles.prevEntity,
                           (unsigned long long)ent->common.handles.nextEntity,
                           (unsigned long long)ent->common.handles.xdic,
                           (unsigned long long)ent->common.handles.layer);
                }
            }
            ++total[p.typeCode];

            std::vector<uint8_t> out;
            uint32_t bitsize = 0, payload = 0;
            if (r2000) {
                if (!encodeObjectR2000(*ent, obj.data(), obj.size(), ref.address * 8,
                                        p.afterHeader, p.afterEed, out, &payload))
                    continue;
                bitsize = static_cast<uint32_t>(out.size()) * 8;  // R2000 无 bitsize 字段
            } else if (!encodeObjectR13R14(*ent, obj.data(), obj.size(),
                                           ref.address * 8, p.afterHeader, p.afterEed, out,
                                           &bitsize, &payload)) {
                continue;
            }
            ++encoded;
            if (encoded <= 6 && i + 1 <= src.objectAddresses.size()) {
                const size_t headLen =
                    r2000 ? (size_t)((p.msSize < 0x8000 ? 2u : 4u) + p.msSize + 2u) : 0u;
                printf("  #%zu type=%d h=%llu ms=%u recLen=%zu bitsize src=%u our=%u/%zu\n",
                       i, p.typeCode, (unsigned long long)p.handle, p.msSize, headLen,
                       p.bitsize, bitsize, out.size());
            }
            const uint64_t nextAddr = (i + 1 < src.objectAddresses.size())
                                          ? src.objectAddresses[i + 1]
                                          : src.objectEnd;
            // R2000 记录长度 = MS 宽度 + 记录体 + 2 字节 CRC；nextAddr 不保证升序（块区记录在文件尾部回绕），
            // 故 R2000 分支改用 MS 自算。
            const size_t srcLen =
                r2000 ? (size_t)((p.msSize < 0x8000 ? 2u : 4u) + p.msSize + 2u)
                      : static_cast<size_t>(nextAddr - addr);
            const size_t n = out.size();
            const bool ok = (ref.address + n <= obj.size()) &&
                            std::memcmp(out.data(), obj.data() + ref.address, n) == 0 &&
                            (r2000 ? (n == srcLen) : (bitsize == p.bitsize));
            // 数据段（记录除 CRC 外）是否逐字节全等：把「字段建模」与「句柄/尾巴」分开计分。
            const size_t cmpBytes = (r2000 ? n : (bitsize + 7) / 8) - (r2000 ? 2 : 1);
            const bool dataOk = cmpBytes >= 2 &&
                                std::memcmp(out.data() + 2, obj.data() + ref.address + 2,
                                            cmpBytes) == 0 &&
                                (r2000 ? (n == srcLen) : (bitsize == p.bitsize));
            ++dataTotal;
            if (dataOk) ++dataOkCount;
            if (ok) {
                ++matched[p.typeCode];
            } else {
                ++mismatched;
                if (shown[p.typeCode] < 3) {
                    ++shown[p.typeCode];
                    const uint64_t nextA = (i + 1 < src.objectAddresses.size())
                                               ? src.objectAddresses[i + 1]
                                               : src.objectEnd;
                    printf("  mismatch type=%d handle=%llu addr=%llu bitsize %u/%u msSrc=%u recLen src/our=%llu/%zu\n",
                           p.typeCode, (unsigned long long)p.handle, (unsigned long long)addr,
                           p.bitsize, bitsize, p.msSize,
                           (unsigned long long)(nextA - addr), n);
                    const size_t cmp = (n < (size_t)(nextA - addr)) ? n : (size_t)(nextA - addr);
                    size_t diffAt = 0;
                    while (diffAt < cmp && out[diffAt] == obj[ref.address + diffAt]) ++diffAt;
                    printf("    firstDiff=%zu cmp=%zu\n", diffAt, cmp);
                    printf("    c: col=%d ls=%g inv=%d em=%d nre=%d\n", (int)ent->common.color,
                           ent->common.ltypeScale, (int)ent->common.invisible,
                           (int)ent->common.entmode, (int)ent->common.numReactors);
                    {
                        size_t nd = 0;
                        printf("    diffs@");
                        for (size_t k = 0; k < cmp && nd < 10; ++k)
                            if (out[k] != obj[ref.address + k]) { printf(" %zu", k); ++nd; }
                        size_t tot = 0;
                        for (size_t k = 0; k < cmp; ++k)
                            if (out[k] != obj[ref.address + k]) ++tot;
                        printf(" (%zu bytes)\n", tot);
                    }
                    printf("    h: own=%llu(%d) xd=%llu(%d) lay=%llu(%d) lt=%llu(%d) prev=%llu(%d) nxt=%llu(%d) nre=%zu tail=%u bylt=%d nl=%d\n",
                           (unsigned long long)ent->common.handles.owner, ent->common.handles.ownerCode,
                           (unsigned long long)ent->common.handles.xdic, ent->common.handles.xdicCode,
                           (unsigned long long)ent->common.handles.layer, ent->common.handles.layerCode,
                           (unsigned long long)ent->common.handles.ltype, ent->common.handles.ltypeCode,
                           (unsigned long long)ent->common.handles.prevEntity, ent->common.handles.prevCode,
                           (unsigned long long)ent->common.handles.nextEntity, ent->common.handles.nextCode,
                           ent->common.handles.reactors.size(), (unsigned)ent->rawTailBits,
                           (int)ent->common.isbylayerlt, (int)ent->common.nolinks);
                    const size_t b0 = (diffAt > 3) ? diffAt - 3 : 0;
                    const size_t b1 = (b0 + 10 < cmp) ? b0 + 10 : cmp;
                    printf("    bits src:");
                    for (size_t k = b0; k < b1; ++k)
                        for (int q = 7; q >= 0; --q) printf("%d", (obj[ref.address + k] >> q) & 1);
                    printf("\n    bits our:");
                    for (size_t k = b0; k < b1; ++k)
                        for (int q = 7; q >= 0; --q) printf("%d", (out[k] >> q) & 1);
                    printf("\n");
                    const size_t e0 = (cmp > 16) ? cmp - 16 : 0;                    printf("    tail src[%zu..]:", e0);
                    for (size_t k = e0; k < cmp; ++k) printf(" %02X", obj[ref.address + k]);
                    printf("\n    tail our[%zu..]:", e0);
                    for (size_t k = e0; k < cmp; ++k) printf(" %02X", out[k]);
                    printf("\n");
                }
            }
        }
        printf("objects=%zu encoded=%zu mismatched=%zu dataOk=%zu/%zu\n", total.size(), encoded, mismatched, dataOkCount, dataTotal);
        {  // type19dump
            size_t shown19 = 0;
            for (size_t i = 0; i < src.objectAddresses.size() && shown19 < 3; ++i) {
                const uint64_t addr = src.objectAddresses[i];
                if (addr < src.objectBase || addr >= src.objectEnd) continue;
                Probe p;
                if (!probe(obj, (addr - src.objectBase) * 8, p)) continue;
                if (p.typeCode != 19) continue;
                ++shown19;
                printf("  t19 addr=%llu ms=%u h=%llu bitsize=%u bytes:",
                       (unsigned long long)addr, p.msSize, (unsigned long long)p.handle, p.bitsize);

                for (size_t k = 0; k < 24; ++k) printf(" %02X", obj[addr - src.objectBase + k]);
                printf("\n");
            }
        }
        {
            std::map<int, size_t> hist;
            for (size_t i = 0; i < src.objectAddresses.size(); ++i) {
                const uint64_t addr = src.objectAddresses[i];
                if (addr < src.objectBase || addr >= src.objectEnd) continue;
                Probe p;
                if (!probe(obj, (addr - src.objectBase) * 8, p)) continue;
                if (p.typeCode > 0) ++hist[p.typeCode];
            }
            printf("type histogram:");
            size_t shownN = 0;
            for (auto& kv : hist) {
                if (kv.second < 50) continue;
                printf(" %d=%zu", kv.first, kv.second);
                if (++shownN >= 14) break;
            }
            printf("\n");
        }
        for (auto& kv : total) {
            printf("  type=%d total=%zu exact=%zu\n", kv.first, kv.second, matched[kv.first]);
        }
        return mismatched ? 1 : 0;
    } catch (const std::exception& e) {
        printf("exception: %s\n", e.what());
        return 1;
    }
}
