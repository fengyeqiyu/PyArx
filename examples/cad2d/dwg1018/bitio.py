# -*- coding: utf-8 -*-
"""R2004 位链读取（对照 bits_ref.c）。

对象/句柄流为位流：bit 可不在字节边界。RC/RS/MS 等底层读取均经 bit 推进，
高层 B/BB/BS/BOT/MC/UMC 在其上组合。
"""
from __future__ import annotations


class BitStream:
    def __init__(self, data: bytes):
        self.data = data
        self.byte = 0
        self.bit = 0

    # ---- 底层位 / 字节 ----
    def read_b(self) -> int:
        """bit_read_B（bits_ref.c 172）：读 1 位，MSB 在前。"""
        byte = self.data[self.byte]
        result = (byte & (0x80 >> self.bit)) >> (7 - self.bit)
        self._advance(1)
        return result

    def read_bb(self) -> int:
        """bit_read_BB（bits_ref.c 206）：读 2 位。"""
        byte = self.data[self.byte]
        if self.bit < 7:
            result = (byte & (0xC0 >> self.bit)) >> (6 - self.bit)
        else:
            result = (byte & 0x01) << 1
            if self.byte < len(self.data) - 1:
                byte = self.data[self.byte + 1]
                result |= (byte & 0x80) >> 7
        self._advance(2)
        return result

    def read_rc(self) -> int:
        """bit_read_RC（bits_ref.c 288）：读 1 字节，跨位边界。"""
        byte = self.data[self.byte]
        if self.bit == 0:
            result = byte
            self.byte += 1
        else:
            result = byte << self.bit
            nxt = self.data[self.byte + 1]
            result |= nxt >> (8 - self.bit)
            self._advance(8)
        return result & 0xFF

    def read_rs(self) -> int:
        """bit_read_RS（bits_ref.c 350）：2 字节小端。"""
        lo = self.read_rc()
        hi = self.read_rc()
        return (hi << 8) | lo

    def read_rl(self) -> int:
        """bit_read_RL（bits_ref.c 398）：两个 RS，低字在前。"""
        w1 = self.read_rs()
        w2 = self.read_rs()
        return ((w2 << 16) | w1) & 0xFFFFFFFF

    def read_rll(self) -> int:
        """bit_read_RLL（bits_ref.c 448）：两个 RL，低字在前。"""
        lo = self.read_rl()
        hi = self.read_rl()
        return ((hi << 32) | lo) & 0xFFFFFFFFFFFFFFFF

    def read_rll_be(self) -> int:
        """bit_read_RLL_BE：8 字节大端（EED code 3/5 句柄）。"""
        v = 0
        for _ in range(8):
            v = (v << 8) | self.read_rc()
        return v & 0xFFFFFFFFFFFFFFFF

    def read_rd(self) -> float:
        """bit_read_RD（bits_ref.c 528）：8 字节 IEEE double 小端（经 RLL 跨位读）。"""
        import struct
        u = self.read_rll()
        return struct.unpack("<d", struct.pack("<Q", u))[0]

    def read_bd(self) -> float:
        """bit_read_BD（bits_ref.c 912）：紧凑 double。BB=0→RD；1→1.0；2→0.0。"""
        code = self.read_bb()
        if code == 0:
            return self.read_rd()
        if code == 1:
            return 1.0
        return 0.0

    def read_bl(self) -> int:
        """bit_read_BL（bits_ref.c 613）：紧凑 long。BB=0→RL；1→RC；2→0；3→256。"""
        code = self.read_bb()
        if code == 0:
            return self.read_rl()
        if code == 1:
            return self.read_rc() & 0xFF
        if code == 2:
            return 0
        return 256

    def read_dd(self, default_value: float) -> float:
        """bit_read_DD（bits_ref.c 1227）：BB=0→默认；1→低4字节；2→低6字节；3→全 RD。"""
        import struct
        code = self.read_bb()
        if code == 0:
            return default_value
        if code == 3:
            return self.read_rd()
        u = struct.unpack("<Q", struct.pack("<d", default_value))[0]
        if code == 2:
            # 低 6 字节（小端：byte4,5,0,1,2,3），保留高 2 字节
            b4 = self.read_rc(); b5 = self.read_rc()
            b0 = self.read_rc(); b1 = self.read_rc(); b2 = self.read_rc(); b3 = self.read_rc()
            arr = [b0, b1, b2, b3, b4, b5, (u >> 48) & 0xFF, (u >> 56) & 0xFF]
            u = int.from_bytes(bytes(arr), "little")
        else:  # code == 1：低 4 字节，保留高 4 字节
            b0 = self.read_rc(); b1 = self.read_rc(); b2 = self.read_rc(); b3 = self.read_rc()
            arr = [b0, b1, b2, b3, (u >> 32) & 0xFF, (u >> 40) & 0xFF,
                   (u >> 48) & 0xFF, (u >> 56) & 0xFF]
            u = int.from_bytes(bytes(arr), "little")
        return struct.unpack("<d", struct.pack("<Q", u))[0]

    def read_2rd(self):
        """FIELD_2RD：x,y 各 RD（原始 double）。"""
        return self.read_rd(), self.read_rd()

    def read_2bd(self):
        """2BD：x,y 各 BD。"""
        return self.read_bd(), self.read_bd()

    def read_3bd(self):
        """FIELD_3BD：x,y,z 各 BD。"""
        return self.read_bd(), self.read_bd(), self.read_bd()

    def read_be(self):
        """FIELD_BE 挤出方向(法线)：B=1→默认(0,0,1)；0→3BD。"""
        if self.read_b():
            return 0.0, 0.0, 1.0
        return self.read_3bd()

    def read_bt(self):
        """FIELD_BT 厚度：B=1→0.0；0→BD。"""
        if self.read_b():
            return 0.0
        return self.read_bd()

    def read_tf(self, n: int) -> bytes:
        """bit_read_TF：读 n 个原始字节（RC），返回 bytes。"""
        return bytes(self.read_rc() for _ in range(n))

    def read_tv(self) -> bytes:
        """bit_read_TV（bits.c 1857）：BS 长度前缀 + n 个 RC 字节。"""
        n = self.read_bs()
        return bytes(self.read_rc() for _ in range(n))

    def read_rs_be(self) -> int:
        """bit_read_RS_BE（bits_ref.c 374）：2 字节大端。"""
        hi = self.read_rc()
        lo = self.read_rc()
        return (hi << 8) | lo

    # ---- 高层紧凑类型 ----
    def read_ms(self) -> int:
        """bit_read_MS（bits_ref.c 1133）：15 位分组，最多 2 个 RS；
        首读字放低位（j=0），次读字放高位（j=15）。"""
        result = 0
        j = 0
        for _ in range(2):
            word = self.read_rs()
            if not (word & 0x8000):
                result |= word << j
                return result
            result |= (word & 0x7FFF) << j
            j += 15
        return 0

    def read_bs(self) -> int:
        """bit_read_BS（bits_ref.c 572）：紧凑位短。"""
        code = self.read_bb()
        if code == 0:
            return self.read_rs()
        if code == 1:
            return self.read_rc() & 0xFF
        if code == 2:
            return 0
        return 256

    def read_bot(self) -> int:
        """bit_read_BOT（bits_ref.c 678）：对象类型（2010+）。"""
        code = self.read_bb()
        if code == 0:
            return self.read_rc()
        if code == 1:
            return self.read_rc() + 0x1F0
        return self.read_rs()

    def read_umc(self) -> int:
        """bit_read_UMC（bits_ref.c 1066）：最多 8 字节无符号变长，先读落低位。"""
        result = 0
        j = 0
        for i in range(7, -1, -1):
            b = self.read_rc()
            if not (b & 0x80):
                result |= b << j
                return result
            result |= (b & 0x7F) << j
            j += 7
        return 0

    def read_mc(self) -> int:
        """bit_read_MC（bits.c 985）：最多 5 字节有符号变长，先读落高位。"""
        negative = 0
        result = 0
        for i, j in zip(range(4, -1, -1), range(0, 35, 7)):
            b = self.read_rc()
            if not (b & 0x80):
                if b & 0x40:
                    negative = 1
                    b &= 0xBF
                result |= b << j
                return -result if negative else result
            b &= 0x7F
            result |= b << j
        return 0

    def read_cmc(self):
        """bit_read_CMC（R2004）：index BS, rgb BL, flag RC；flag&1/2 可选 T 名（str_dat，离线占位 TV）。"""
        index = self.read_bs()
        rgb = self.read_bl()
        flag = self.read_rc()
        if flag < 4:
            if flag & 1: self.read_tv()
            if flag & 2: self.read_tv()
        return index, rgb, flag

    # ---- 位置 ----
    def _advance(self, bits: int) -> None:
        total = self.byte * 8 + self.bit + bits
        self.byte = total >> 3
        self.bit = total & 7

    def tell(self) -> int:
        return self.byte * 8 + self.bit
