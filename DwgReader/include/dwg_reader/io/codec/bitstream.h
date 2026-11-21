#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

#include "dwg_reader/value/vec3.h"

namespace dwr {

/// R2004 位链读取（移植自 bitio.py）。bit 可不在字节边界，MSB 在前。
class BitStream {
public:
    BitStream(const uint8_t* data, size_t len) : data_(data), len_(len) {}

    void advance(int bits);
    size_t tell() const { return byte_ * 8 + bit_; }

    int read_b();
    int read_bb();
    int read_rc();
    int read_rs();
    uint32_t read_rl();
    uint64_t read_rll();
    uint64_t read_rll_be();
    double read_rd();
    double read_bd();
    uint32_t read_bl();
    double read_dd(double default_value);
    void read_2rd(double& x, double& y);
    void read_2bd(double& x, double& y);
    void read_3bd(double& x, double& y, double& z);
    void read_be(double& x, double& y, double& z);
    double read_bt();
    std::string read_tf(int n);
    std::string read_tv();
    int read_rs_be();
    int read_ms();
    int read_bs();
    int read_bot();
    uint64_t read_umc();
    int read_mc();
    /// 句柄读取（read_h）：首字节低 4 位为后续字节数，大端拼句柄。
    uint64_t read_handle();
    // CMC: index BS, rgb BL, flag RC；flag<4 时可选两个 TV 名（离线占位跳过）。
    void read_cmc(int& index, uint32_t& rgb, int& flag);

    const uint8_t* data_;
    size_t len_;
    size_t byte_ = 0;
    int bit_ = 0;
};

}  // namespace dwr
