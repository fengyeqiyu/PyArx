#pragma once

#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

namespace dwr {

/// 位写入器：与 BitStream 的读口径严格互逆（R13/R14 位码优先，R2000+ 亦兼容 BS/RL 原码）。
class BitWriter {
public:
    std::vector<uint8_t> bytes;
    size_t nbits = 0;

    void putBit(int v) {
        if ((nbits & 7) == 0) bytes.push_back(0);
        if (v & 1) bytes.back() = static_cast<uint8_t>(bytes.back() | (0x80 >> (nbits & 7)));
        ++nbits;
    }

    void putBits(int n, uint64_t v) {
        for (int i = n - 1; i >= 0; --i) putBit(static_cast<int>((v >> i) & 1));
    }

    void b(int v) { putBit(v != 0); }
    void bb(int v) { putBits(2, static_cast<uint64_t>(v & 3)); }
    void rc(int v) { putBits(8, static_cast<uint64_t>(v & 0xFF)); }
    void rs(int v) { rc(v & 0xFF); rc((v >> 8) & 0xFF); }
    void rl(uint32_t v) { rs(static_cast<int>(v & 0xFFFF)); rs(static_cast<int>(v >> 16)); }

    void rd(double d) {
        uint64_t u;
        std::memcpy(&u, &d, sizeof(u));
        rl(static_cast<uint32_t>(u & 0xFFFFFFFFu));
        rl(static_cast<uint32_t>(u >> 32));
    }

    /// BB 2 位编码：0=全精度、1=1.0、2=0.0。
    void bd(double d) {
        if (d == 1.0) {
            bb(1);
        } else if (d == 0.0) {
            bb(2);
        } else {
            bb(0);
            rd(d);
        }
    }

    /// dd：2 位模式码 —— 0=沿用默认值、1=高 4 字节同（写低 4 字节）、
    /// 2=高 2 字节同（写 4/5/0/1/2/3 六个字节）、3=全 8 字节（libredwg bit_write_DD 口径）。
    void dd(double d, double def) {
        if (d == def) {
            bb(0);
            return;
        }
        uint64_t uv, dv;
        std::memcpy(&uv, &d, sizeof(uv));
        std::memcpy(&dv, &def, sizeof(dv));
        const uint8_t* pu = reinterpret_cast<const uint8_t*>(&uv);
        const uint8_t* pd = reinterpret_cast<const uint8_t*>(&dv);
        if (pu[4] == pd[4] && pu[5] == pd[5] && pu[6] == pd[6] && pu[7] == pd[7]) {
            bb(1);
            for (int i = 0; i < 4; ++i) rc(pu[i]);
        } else if (pu[6] == pd[6] && pu[7] == pd[7]) {
            bb(2);
            rc(pu[4]); rc(pu[5]);
            for (int i = 0; i < 4; ++i) rc(pu[i]);
        } else {
            bb(3);
            rd(d);
        }
    }

    /// bt 厚度：1 位 1 表示 0.0。
    void bt(double t) {
        if (t == 0.0) {
            b(1);
        } else {
            b(0);
            bd(t);
        }
    }

    /// be 法向：1 位 1 表示 (0,0,1)。
    void be(double x, double y, double z) {
        if (x == 0.0 && y == 0.0 && z == 1.0) {
            b(1);
        } else {
            b(0);
            bd(x);
            bd(y);
            bd(z);
        }
    }

    /// BS：0=RS 全值、1=RC、2=0、3=256。
    void bs(int v) {
        if (v == 0) {
            bb(2);
        } else if (v == 256) {
            bb(3);
        } else if (v >= 0 && v < 256) {
            bb(1);
            rc(v);
        } else {
            bb(0);
            rs(v & 0xFFFF);
        }
    }

    /// BL：0=RL 全值、1=RC、2=0、3=256。
    void bl(uint32_t v) {
        if (v == 0) {
            bb(2);
        } else if (v == 256) {
            bb(3);
        } else if (v < 256) {
            bb(1);
            rc(static_cast<int>(v));
        } else {
            bb(0);
            rl(v);
        }
    }

    /// 句柄：低 4 位为字节数，随后按大端写句柄字节。
    void handle(uint64_t h) {
        int sz = 0;
        for (uint64_t t = h; t; t >>= 8) ++sz;
        rc(sz & 0x0F);
        for (int i = sz - 1; i >= 0; --i) rc(static_cast<int>((h >> (8 * i)) & 0xFF));
    }

    /// TV 变长串（R13..R2000）：BS 长度 + 逐字节 RC（libredwg bits.c bit_write_TV）。
    void tv(const std::string& s) {
        bs(static_cast<int>(s.size()));
        for (char c : s) rc(static_cast<int>(static_cast<unsigned char>(c)));
    }

    /// MS 模块化短整：低段 15 位，最高位为「续读」标志。
    void ms(uint32_t v) {
        if (v < 0x8000u) {
            rs(static_cast<int>(v));
        } else {
            rs(static_cast<int>((v & 0x7FFFu) | 0x8000u));
            rs(static_cast<int>(v >> 15));
        }
    }

    size_t byteSize() const { return bytes.size(); }
};

}  // namespace dwr
