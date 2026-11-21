#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace dwr {

/// R2007 (AC1021) 编解码：RS 列解交织 + 自有 LZ77 变体解压。
/// 移植自离线蓝本 compress1021.py，对照 decode_r2007.c（不链 libredwg）。

/// RS 列优先转置：输出 block_count*data_size 字节。
/// 磁盘布局为列交织（所有块第 j 字节连续），纠错在参照中 disabled，仅取前 data_size 字节/块。
std::vector<uint8_t> decodeRs1021(const uint8_t* src, size_t src_len,
                                  int block_count, int data_size);

/// decompress_r2007：把 src 解压到 dst_size 字节；返回解压缓冲。
/// src_size 为压缩 payload 长度（pedata 内有效字节数）。
std::vector<uint8_t> decompressR2007(size_t dst_size, const uint8_t* src,
                                     size_t src_size);

}  // namespace dwr
