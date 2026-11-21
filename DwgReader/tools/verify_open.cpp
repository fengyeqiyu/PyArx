#include <dwg_reader/db/database.h>

#include <cstdio>
#include <fstream>
#include <map>
#include <vector>

static void dump(const char* path, const std::vector<uint8_t>& buf) {
    std::ofstream out(path, std::ios::binary);
    out.write(reinterpret_cast<const char*>(buf.data()), buf.size());
}

int main(int argc, char** argv) {
    if (argc < 2) {
        printf("usage: verify_open <dwg>\n");
        return 2;
    }
    dwr::Database db;
    bool ok = db.open(argv[1]);
    if (!ok) {
        printf("open fail err=%d msg=%s\n", db.lastError(), db.lastErrorText().c_str());
        if (!db.container().diag.empty())
            printf("container diag: %s\n", db.container().diag.c_str());
        for (const auto& kv : db.container().sections) {
            printf("  sec %s pages=%u size=%llu maxPage=%u\n", kv.first.c_str(),
                   kv.second.pageCount,
                   static_cast<unsigned long long>(kv.second.size),
                   kv.second.maxPageSize);
        }
        return 1;
    }
    const auto& buf = db.objectsBuffer();
    printf("open ok version=%s (%s) objects=%zu\n", dwr::versionCode(db.version()),
           dwr::versionLabel(db.version()), buf.size());
    dump("G:\\PyArx\\DwgReader\\build\\objects_cpp.bin", buf);

    std::vector<uint8_t> hdl = db.handlesBuffer();
    size_t nsec = (db.version() == dwr::DwgVersion::kAc1021)
                      ? db.container1021().sections.size()
                      : db.container().sections.size();
    printf("sections=%zu hdl=%zu\n", nsec, hdl.size());
    if (db.version() != dwr::DwgVersion::kAc1021 && !db.container().diag.empty())
        printf("container diag: %s\n", db.container().diag.c_str());
    if (!hdl.empty()) dump("G:\\PyArx\\DwgReader\\build\\hdl_cpp.bin", hdl);

    const auto& idx = db.objectIndex();
    printf("index objects=%zu handleMap=%zu realigned=%u mismatched=%u\n", idx.size(),
           db.handleMap().size(), idx.realigned(), idx.mismatched());
    {
        size_t bad = 0;
        for (const auto& r : idx.objects()) {
            uint64_t a = 0;
            if (!db.handleMap().lookup(r.handle, a) || a != r.address) ++bad;
        }
        printf("handleMap lookup mismatch=%zu\n", bad);
    }
    {
        std::ofstream ix("G:\\PyArx\\DwgReader\\build\\index_cpp.tsv");
        for (const auto& r : idx.objects()) {
            ix << r.handle << '\t' << r.address << '\t' << r.typeCode << '\t'
               << r.bitsize << '\n';
        }
    }
    for (size_t i = 0; i < idx.objects().size() && i < 5; ++i) {
        const auto& r = idx.objects()[i];
        printf("  #%zu handle=%llu addr=%llu type=%d bitsize=%u recSize=%u hdlstream=%u\n", i,
               (unsigned long long)r.handle, (unsigned long long)r.address, r.typeCode,
               r.bitsize, r.recordSize, r.handlestream);
    }

    // 对象类型分布（前 8）
    {
        std::map<int, size_t> byType;
        for (const auto& r : idx.objects()) ++byType[r.typeCode];
        size_t shown = 0;
        printf("obj types:");
        for (const auto& kv : byType) {
            if (shown++ >= 8) break;
            printf(" %d:%zu", kv.first, kv.second);
        }
        printf("\n");
    }

    const auto& ents = db.entities();
    printf("decoded entities=%zu\n", ents.size());
    std::map<int, size_t> byType;
    for (const auto& e : ents) ++byType[static_cast<int>(e->typeCode)];
    for (const auto& kv : byType) printf("  type=%d count=%zu\n", kv.first, kv.second);
    return 0;
}
