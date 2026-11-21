#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace dwr {

class DecompressError {
public:
    explicit DecompressError(std::string msg) : msg_(std::move(msg)) {}
    const std::string& what() const { return msg_; }

private:
    std::string msg_;
};

/// R2004 (AC1018) 18-bit LZ77 变体解压（移植自 compress.py，ODA 规范 §4.7）。
/// 将 src 解压写入 out（预分配 limit 字节，从 out_pos 起）；返回写后的 out_pos。
size_t decompress(const uint8_t* src, size_t src_len, size_t limit,
                  uint8_t* out, size_t out_pos = 0, size_t base = 0);

/// 解压到内部新建缓冲（max_out 为解压后上限），返回缓冲。
std::vector<uint8_t> decompress_to_vec(const uint8_t* src, size_t src_len,
                                       size_t max_out, size_t base = 0);

}  // namespace dwr
