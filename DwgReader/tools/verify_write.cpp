// 经典布局写侧验证：读 → 重建容器 → 重新打开 → 句柄/地址一致性。
#include <cstdio>
#include <string>
#include <vector>

#include "dwg_reader/io/container/classic_container.h"
#include "dwg_reader/io/container/classic_writer.h"

int main(int argc, char** argv) {
    if (argc < 3) {
        printf("usage: verify_write <src.dwg> <out.dwg>\n");
        return 2;
    }
    try {
        dwr::ClassicContainer src = dwr::openClassicDwg(argv[1]);
        printf("src  %s objects=%zu objRange=%llu..%llu sections=%zu\n", argv[1],
               src.objectHandles.size(), (unsigned long long)src.objectBase,
               (unsigned long long)src.objectEnd, src.sections.size());

        dwr::ClassicWriteOptions opt;
        if (argc >= 4 && std::string(argv[3]) == "keep") { opt.preserveLayout = true; opt.preservePagination = true; }
        if (argc >= 4 && std::string(argv[3]) == "copyhandles") opt.copyHandles = true;
        dwr::ClassicWriteResult w = dwr::writeClassicDwg(src, argv[2], opt);
        printf("out  %s bytes=%zu objects=%zu pages=%zu objBase=%llu\n", argv[2],
               w.bytes.size(), w.objectCount, w.handlePages,
               (unsigned long long)w.objectBase);
        printf("diag %s\n", w.diag.c_str());

        dwr::ClassicContainer re = dwr::openClassicDwg(argv[2]);
        printf("reopen objects=%zu sections=%zu\n", re.objectHandles.size(),
               re.sections.size());
        if (re.objectHandles.size() != src.objectHandles.size()) {
            printf("FAIL count %zu != %zu\n", re.objectHandles.size(), src.objectHandles.size());
            return 1;
        }
        size_t bad = 0, shown = 0;
        for (size_t i = 0; i < src.objectHandles.size(); ++i) {
            const bool hOk = re.objectHandles[i] == src.objectHandles[i];
            const bool aOk = (re.objectAddresses[i] - re.objectBase) ==
                             (src.objectAddresses[i] - src.objectBase);
            if (!hOk || !aOk) {
                if (shown < 6) {
                    printf("  diff #%zu handle %llu/%llu addr %llu/%llu\n", i,
                           (unsigned long long)src.objectHandles[i],
                           (unsigned long long)re.objectHandles[i],
                           (unsigned long long)src.objectAddresses[i],
                           (unsigned long long)re.objectAddresses[i]);
                    ++shown;
                }
                ++bad;
            }
        }
        printf("%s handle+addr mismatch=%zu/%zu\n", bad ? "FAIL" : "PASS", bad,
               src.objectHandles.size());
        return bad ? 1 : 0;
    } catch (const std::exception& e) {
        printf("exception: %s\n", e.what());
        return 1;
    }
}
