#include "dwg_reader/io/container/classic_writer.h"

#include <algorithm>
#include <fstream>
#include <stdexcept>

#include "dwg_reader/io/filer/dwg_out_filer.h"

namespace dwr {

namespace {

uint32_t rd32(const std::vector<uint8_t>& d, size_t p) {
    return static_cast<uint32_t>(d[p]) | (static_cast<uint32_t>(d[p + 1]) << 8) |
           (static_cast<uint32_t>(d[p + 2]) << 16) | (static_cast<uint32_t>(d[p + 3]) << 24);
}

size_t alignUp(size_t v, size_t a) { return a ? ((v + a - 1) / a) * a : v; }

/// DWG 经典布局的 CRC16：poly 0xA001（反射 CRC-16/IBM），种子 0xC0C1。
/// 依据 libredwg `bits.c: bit_calc_CRC` + `decode.c: bit_check_CRC(…, 0xC0C1)`，已在 R14 真样本上逐页验证。
uint16_t crc16(const uint8_t* p, size_t n, uint16_t seed = 0xC0C1) {
    uint16_t c = seed;
    for (size_t i = 0; i < n; ++i) {
        c ^= p[i];
        for (int b = 0; b < 8; ++b) c = (c & 1) ? static_cast<uint16_t>((c >> 1) ^ 0xA001)
                                               : static_cast<uint16_t>(c >> 1);
    }
    return c;
}

/// 句柄表：逐页 RS_BE 页长 + {UMC handle 增量, MC 地址增量} + 页尾 2 字节 CRC。
/// 口径与读侧 `openClassicDwg` 严格互逆：handle 跨页连续递增，地址每页从 0 起算。
void buildHandles(MemoryOutFiler& f, const std::vector<uint64_t>& handles,
                  const std::vector<uint64_t>& newAddrs, uint32_t pagePayload,
                  size_t* pageCount, size_t* written,
                  const std::vector<uint32_t>* pageCounts = nullptr) {
    size_t i = 0;
    uint64_t lastHandle = 0;
    const size_t n = handles.size();
    while (i < n) {
        MemoryOutFiler page;
        page.u16be(static_cast<uint16_t>(0));  // 页长占位
        const size_t start = i;
        const uint64_t startHandle = lastHandle;
        uint64_t prevAddr = 0;
        std::vector<size_t> entEnd;  // 每条目结束位置（相对 page.bytes）
        // 保持源分页：页号 = 已写页数，条目数取源文件该页的条数。
        const uint32_t fixedCount =
            (pageCounts && *pageCount < pageCounts->size()) ? pageCounts->at(*pageCount) : 0;
        while (i < n) {
            if (fixedCount && entEnd.size() >= fixedCount) break;
            MemoryOutFiler ent;
            ent.writeUmc(handles[i] - lastHandle);
            const int64_t delta = (i == start)
                                      ? static_cast<int64_t>(newAddrs[i])
                                      : static_cast<int64_t>(newAddrs[i]) -
                                            static_cast<int64_t>(prevAddr);
            ent.writeMc(delta);
            // 读侧按 `p + 2 < pageEnd` 收尾：页末余量 <3 字节就停，
            // 因此最后一条至少 3 字节，且页必须正好在条目边界结束。
            if (page.size() + ent.size() > pagePayload) break;
            page.writeBytes(ent.bytes.data(), ent.size());
            entEnd.push_back(page.size());
            lastHandle = handles[i];
            prevAddr = newAddrs[i];
            ++i;
        }
        if (i == start) {  // 单条超长：强制写一条，避免死循环
            page.writeUmc(handles[i] - lastHandle);
            page.writeMc(static_cast<int64_t>(newAddrs[i]));
            entEnd.push_back(page.size());
            lastHandle = handles[i];
            ++i;
        }
        // 页末若以 2 字节短条目结尾，读侧会跳过它：把它留给下一页（保持源分页时不调整）。
        if (!fixedCount && i < n && entEnd.size() >= 2) {
            const size_t lastLen = entEnd.back() - entEnd[entEnd.size() - 2];
            if (lastLen == 2) {
                page.bytes.resize(entEnd[entEnd.size() - 2]);
                entEnd.pop_back();
                --i;
                lastHandle = (entEnd.size() >= 2)
                                 ? handles[start + entEnd.size() - 1]
                                 : startHandle;
            }
        }
        const uint16_t pageSize = static_cast<uint16_t>(page.size());
        page.bytes[0] = static_cast<uint8_t>(pageSize >> 8);
        page.bytes[1] = static_cast<uint8_t>(pageSize & 0xFF);
        f.writeBytes(page.bytes.data(), page.size());
        f.u16be(crc16(page.bytes.data(), page.size()));  // 页尾 CRC，种子 0xC0C1
        ++*pageCount;
    }
    // 收尾空页：真样本为 `RS_BE 0x0002` + 2 字节 CRC（如 000201d0），少写 CRC 会被 AutoCAD 判坏。
    {
        const uint8_t tail[2] = {0x00, 0x02};
        f.writeBytes(tail, 2);
        f.u16be(crc16(tail, 2));
    }
    *written = i;
}

}  // namespace

ClassicWriteResult writeClassicImage(const ClassicContainer& src,
                                     const ClassicWriteOptions& opt) {
    ClassicWriteResult r;
    if (src.data.empty()) throw std::runtime_error("empty classic container");
    if (src.sections.empty()) throw std::runtime_error("no sections");

    // 就地位模式：只在原字节上重写 Handles 节，其余（含 R13/R14 第二头等未知区）保持原样。
    // 用于把「容器结构错误」和「未知区缺失」区分开的诊断路径。
    if (opt.preserveLayout) {
        const ClassicSection* hdl = src.section("AcDb:Handles");
        if (!hdl) throw std::runtime_error("AcDb:Handles not found");
        const size_t nsec = src.sections.size();
        const size_t hdrB = static_cast<size_t>(src.headerBase);
        const size_t locStart = hdrB + 6;

        MemoryOutFiler f;
        f.writeBytes(src.data.data(), static_cast<size_t>(hdl->address));
        const uint32_t hdlAddr = static_cast<uint32_t>(f.size());
        std::vector<uint64_t> newAddrs(src.objectAddresses.begin(), src.objectAddresses.end());
        buildHandles(f, src.objectHandles, newAddrs, opt.handlePagePayload, &r.handlePages,
                     &r.objectCount,
                     opt.preservePagination ? &src.handlePageEntries : nullptr);
        const uint32_t hdlSize = static_cast<uint32_t>(f.size() - hdlAddr);
        r.objectBase = src.objectBase;
        r.diag = "inplace handles@" + std::to_string(hdlAddr) + "+" + std::to_string(hdlSize) + "; ";

        // Handles 之后的节（通常是 Template 4 字节哨兵）顺次后移，更新定位器。
        std::vector<size_t> locOff(nsec);
        for (size_t i = 0; i < nsec; ++i)
            locOff[i] = locStart + i * 9;
        for (size_t i = 0; i < nsec; ++i) {
            const ClassicSection& s = src.sections[i];
            if (s.name == "AcDb:Handles" || s.size == 0) continue;
            if (s.address < hdl->address) continue;
            f.padTo(alignUp(f.size(), opt.align), 0);
            const uint32_t addr = static_cast<uint32_t>(f.size());
            f.writeBytes(src.data.data() + s.address, s.size);
            f.patchU32(locOff[i] + 1, addr);
            f.patchU32(locOff[i] + 5, s.size);
        }
        // 回填 Handles 定位器
        for (size_t i = 0; i < nsec; ++i) {
            if (src.sections[i].name == "AcDb:Handles") {
                f.patchU32(locOff[i] + 1, hdlAddr);
                f.patchU32(locOff[i] + 5, hdlSize);
            }
        }
        r.bytes = std::move(f.bytes);
        r.diag += "total=" + std::to_string(r.bytes.size());
        return r;
    }

    const size_t hdrBase = static_cast<size_t>(src.headerBase);
    const size_t n = src.sections.size();
    const size_t locStart = hdrBase + 6;
    const size_t locEnd = locStart + n * 9;

    MemoryOutFiler f;
    if (opt.copyHeaderPrefix) f.writeBytes(src.data.data(), hdrBase);  // 0x00..版本码
    else {
        f.writeBytes(src.data.data(), 6);  // 版本码
        f.padTo(hdrBase, 0);
    }
    // codepage RS + 节数 RL
    f.u16le(static_cast<uint16_t>(src.codepage));
    f.u32le(static_cast<uint32_t>(n));
    // 定位器占位：{RC number, RL address, RL size}
    std::vector<size_t> locOff(n);
    for (size_t i = 0; i < n; ++i) {
        locOff[i] = f.size();
        f.u8(static_cast<uint8_t>(src.sections[i].number));
        f.u32le(0);
        f.u32le(src.sections[i].size);
    }
    f.padTo(locEnd, 0);

    // 定位器表与首个节之间的「未知区」（R13/R14 第二头等，内容为加密/不透明数据）：
    // 原样搬回原相对位置，否则 AutoCAD 拒开（实测 ErrorStatus=434）。
    {
        size_t firstSec = 0;
        for (const auto& s : src.sections)
            if (s.size && s.address >= locEnd && (firstSec == 0 || s.address < firstSec))
                firstSec = static_cast<size_t>(s.address);
        if (firstSec > f.size() && firstSec <= src.data.size())
            f.writeBytes(src.data.data() + f.size(), firstSec - f.size());
    }

    // 头部各节之间的空隙（第二头/未知区）原样搬运：AutoCAD 会校验这些不透明数据。
    auto fillTo = [&](size_t target) {
        if (target <= f.size()) return;
        if (target <= src.data.size())
            f.writeBytes(src.data.data() + f.size(), target - f.size());
        else
            f.padTo(target, 0);
    };

    auto place = [&](size_t i, const uint8_t* p, size_t len, const std::string& tag,
                     bool exact = false) {
        const size_t target = (exact || opt.preserveLayout)
                                  ? static_cast<size_t>(src.sections[i].address)
                                  : alignUp(f.size(), opt.align);
        fillTo(target);
        const uint32_t addr = static_cast<uint32_t>(f.size());
        f.writeBytes(p, len);
        f.patchU32(locOff[i] + 1, addr);
        f.patchU32(locOff[i] + 5, static_cast<uint32_t>(len));
        r.diag += tag + "@" + std::to_string(addr) + "+" + std::to_string(len) + "; ";
    };

    auto find = [&](const char* name) -> size_t {
        for (size_t i = 0; i < n; ++i)
            if (src.sections[i].name == name) return i;
        return static_cast<size_t>(-1);
    };

    // 1) AuxHeader（R2000 起有，写在最前）
    const size_t iAux = find("AcDb:AuxHeader");
    if (iAux != static_cast<size_t>(-1)) {
        const ClassicSection& s = src.sections[iAux];
        if (s.size) place(iAux, src.data.data() + s.address, s.size, "aux", true);
    }
    // 2) Header / Classes
    for (const char* nm : {"AcDb:Header", "AcDb:Classes"}) {
        const size_t i = find(nm);
        if (i == static_cast<size_t>(-1)) continue;
        const ClassicSection& s = src.sections[i];
        place(i, src.data.data() + s.address, s.size, nm + 6, true);
    }
    // 3) 对象区（整体搬运，相对偏移不变）：起始地址保持源值，R2000 的 aux/第二头里
    //    记录了对象区起点，改动 1 字节都会被 AutoCAD 判为坏文件。
    if (opt.preserveLayout) f.padTo(static_cast<size_t>(src.objectBase), 0);
    else fillTo(static_cast<size_t>(src.objectBase));
    const uint64_t objBase = f.size();
    r.objectBase = objBase;
    uint64_t objEnd = objBase;
    if (src.objectEnd > src.objectBase) {
        const size_t len = static_cast<size_t>(src.objectEnd - src.objectBase);
        f.writeBytes(src.data.data() + src.objectBase, len);
        objEnd = objBase + len;
    }
    r.objectCount = src.objectHandles.size();
    r.diag += "objects@" + std::to_string(objBase) + "+" +
              std::to_string(objEnd - objBase) + "; ";

    // 4) Handles 重建（地址按新对象基址平移）
    std::vector<uint64_t> newAddrs;
    newAddrs.reserve(src.objectAddresses.size());
    const int64_t shift = static_cast<int64_t>(objBase) - static_cast<int64_t>(src.objectBase);
    for (uint64_t a : src.objectAddresses)
        newAddrs.push_back(static_cast<uint64_t>(static_cast<int64_t>(a) + shift));

    const size_t iHdl = find("AcDb:Handles");
    if (iHdl != static_cast<size_t>(-1)) {
        if (opt.preserveLayout) f.padTo(static_cast<size_t>(src.sections[iHdl].address), 0);
        else fillTo(static_cast<size_t>(src.sections[iHdl].address));
        const uint32_t addr = static_cast<uint32_t>(f.size());
        if (opt.copyHandles) {  // 诊断：原样复制句柄节
            f.writeBytes(src.data.data() + src.sections[iHdl].address, src.sections[iHdl].size);
            r.handlePages = 0;
        } else
            buildHandles(f, src.objectHandles, newAddrs, opt.handlePagePayload, &r.handlePages, &r.objectCount,
                     opt.preservePagination ? &src.handlePageEntries : nullptr);
        // R2000 的 aux/second header 里记录了句柄节尺寸，长度变化会被判坏文件（ErrorStatus=53）：
        // 不足原长度时补零对齐。
        const ClassicSection* hs = src.section("AcDb:Handles");
        if (hs && f.size() - addr < hs->size) f.padTo(addr + hs->size, 0);
        f.patchU32(locOff[iHdl] + 1, addr);
        f.patchU32(locOff[iHdl] + 5, static_cast<uint32_t>(f.size() - addr));
        r.diag += "handles@" + std::to_string(addr) + "+" +
                  std::to_string(f.size() - addr) + "; ";
    }
    // 5) Template（4 字节哨兵）
    const size_t iTpl = find("AcDb:Template");
    if (iTpl != static_cast<size_t>(-1) && src.sections[iTpl].size) {
        const ClassicSection& s = src.sections[iTpl];
        place(iTpl, src.data.data() + s.address, s.size, "tpl", true);
    }
    // 6) 其余节（含空节：置 {0,0}）
    for (size_t i = 0; i < n; ++i) {
        const std::string& nm = src.sections[i].name;
        if (nm == "AcDb:Header" || nm == "AcDb:Classes" || nm == "AcDb:Handles" ||
            nm == "AcDb:Template" || nm == "AcDb:AuxHeader")
            continue;
        if (src.sections[i].size == 0) {
            f.patchU32(locOff[i] + 1, 0);
            f.patchU32(locOff[i] + 5, 0);
            continue;
        }
        place(i, src.data.data() + src.sections[i].address, src.sections[i].size,
              "sec" + std::to_string(src.sections[i].number));
    }

    r.bytes = std::move(f.bytes);
    r.diag += "total=" + std::to_string(r.bytes.size());
    return r;
}

ClassicWriteResult writeClassicDwg(const ClassicContainer& src, const std::string& outPath,
                                   const ClassicWriteOptions& opt) {
    ClassicWriteResult r = writeClassicImage(src, opt);
    std::ofstream out(outPath, std::ios::binary | std::ios::trunc);
    if (!out) throw std::runtime_error("cannot write: " + outPath);
    out.write(reinterpret_cast<const char*>(r.bytes.data()),
              static_cast<std::streamsize>(r.bytes.size()));
    if (!out) throw std::runtime_error("write failed: " + outPath);
    return r;
}

}  // namespace dwr
