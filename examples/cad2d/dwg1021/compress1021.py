# -*- coding: utf-8 -*-
"""R2007 (AC1021) 解压：RS(255,239) 列解交织 + decompress_r2007。

对照 docs/libredwg_src/src/decode_r2007.c（只读移植，不链 libredwg）。
RS 纠错本身在参照源码中 disabled，这里只做列解交织取前 239 字节/块。
"""
from __future__ import annotations


class DecompressError(Exception):
    pass


def decode_rs(src: bytes, block_count: int, data_size: int) -> bytearray:
    """decode_rs：列优先转置，输出 block_count*data_size 字节。"""
    if block_count * data_size > len(src):
        raise DecompressError(f"decode_rs overflow: {block_count*data_size} > {len(src)}")
    dst = bytearray(block_count * data_size)
    for i in range(block_count):
        for j in range(data_size):
            dst[i * data_size + j] = src[j * block_count + i]
    return dst


# ---- literal 字节拷贝（copy_compressed_bytes 各宽度端序规则，位置写入）----

def _put_at(dst: bytearray, dp: int, src: bytes, sp: int, offset: int, width: int) -> int:
    if width == 1:
        dst[dp] = src[sp + offset]
        return dp + 1
    if width == 2:
        dst[dp] = src[sp + offset + 1]
        dst[dp + 1] = src[sp + offset]
        return dp + 2
    if width == 3:
        dst[dp] = src[sp + offset + 2]
        dst[dp + 1] = src[sp + offset + 1]
        dst[dp + 2] = src[sp + offset]
        return dp + 3
    if width == 4:
        dst[dp:dp + 4] = src[sp + offset:sp + offset + 4]
        return dp + 4
    if width == 8:
        dst[dp:dp + 8] = src[sp + offset:sp + offset + 8]
        return dp + 8
    if width == 16:
        dst[dp:dp + 8] = src[sp + offset + 8:sp + offset + 16]
        dst[dp + 8:dp + 16] = src[sp + offset:sp + offset + 8]
        return dp + 16
    raise DecompressError(f"copy width {width}")


def _copy_compressed_at(dst: bytearray, dp: int, src: bytes, sp: int, length: int) -> tuple[int, int]:
    """literal 段拷贝，返回 (新 dp, 新 sp)。对应 copy_compressed_bytes。"""
    while length >= 32:
        dp = _put_at(dst, dp, src, sp, 16, 16)
        dp = _put_at(dst, dp, src, sp, 0, 16)
        sp += 32
        length -= 32
    if length == 0:
        return dp, sp
    if length == 1:
        dp = _put_at(dst, dp, src, sp, 0, 1)
    elif length == 2:
        dp = _put_at(dst, dp, src, sp, 0, 2)
    elif length == 3:
        dp = _put_at(dst, dp, src, sp, 0, 3)
    elif length == 4:
        dp = _put_at(dst, dp, src, sp, 0, 4)
    elif length == 5:
        dp = _put_at(dst, dp, src, sp, 4, 1); dp = _put_at(dst, dp, src, sp, 0, 4)
    elif length == 6:
        dp = _put_at(dst, dp, src, sp, 5, 1); dp = _put_at(dst, dp, src, sp, 1, 4); dp = _put_at(dst, dp, src, sp, 0, 1)
    elif length == 7:
        dp = _put_at(dst, dp, src, sp, 5, 2); dp = _put_at(dst, dp, src, sp, 1, 4); dp = _put_at(dst, dp, src, sp, 0, 1)
    elif length == 8:
        dp = _put_at(dst, dp, src, sp, 0, 8)
    elif length == 9:
        dp = _put_at(dst, dp, src, sp, 8, 1); dp = _put_at(dst, dp, src, sp, 0, 8)
    elif length == 10:
        dp = _put_at(dst, dp, src, sp, 9, 1); dp = _put_at(dst, dp, src, sp, 1, 8); dp = _put_at(dst, dp, src, sp, 0, 1)
    elif length == 11:
        dp = _put_at(dst, dp, src, sp, 9, 2); dp = _put_at(dst, dp, src, sp, 1, 8); dp = _put_at(dst, dp, src, sp, 0, 1)
    elif length == 12:
        dp = _put_at(dst, dp, src, sp, 8, 4); dp = _put_at(dst, dp, src, sp, 0, 8)
    elif length == 13:
        dp = _put_at(dst, dp, src, sp, 12, 1); dp = _put_at(dst, dp, src, sp, 8, 4); dp = _put_at(dst, dp, src, sp, 0, 8)
    elif length == 14:
        dp = _put_at(dst, dp, src, sp, 13, 1); dp = _put_at(dst, dp, src, sp, 9, 4); dp = _put_at(dst, dp, src, sp, 1, 8); dp = _put_at(dst, dp, src, sp, 0, 1)
    elif length == 15:
        dp = _put_at(dst, dp, src, sp, 13, 2); dp = _put_at(dst, dp, src, sp, 9, 4); dp = _put_at(dst, dp, src, sp, 1, 8); dp = _put_at(dst, dp, src, sp, 0, 1)
    elif length == 16:
        dp = _put_at(dst, dp, src, sp, 0, 16)
    elif length == 17:
        dp = _put_at(dst, dp, src, sp, 9, 8); dp = _put_at(dst, dp, src, sp, 8, 1); dp = _put_at(dst, dp, src, sp, 0, 8)
    elif length == 18:
        dp = _put_at(dst, dp, src, sp, 17, 1); dp = _put_at(dst, dp, src, sp, 1, 16); dp = _put_at(dst, dp, src, sp, 0, 1)
    elif length == 19:
        dp = _put_at(dst, dp, src, sp, 16, 3); dp = _put_at(dst, dp, src, sp, 0, 16)
    elif length == 20:
        dp = _put_at(dst, dp, src, sp, 16, 4); dp = _put_at(dst, dp, src, sp, 0, 16)
    elif length == 21:
        dp = _put_at(dst, dp, src, sp, 20, 1); dp = _put_at(dst, dp, src, sp, 16, 4); dp = _put_at(dst, dp, src, sp, 0, 16)
    elif length == 22:
        dp = _put_at(dst, dp, src, sp, 20, 2); dp = _put_at(dst, dp, src, sp, 16, 4); dp = _put_at(dst, dp, src, sp, 0, 16)
    elif length == 23:
        dp = _put_at(dst, dp, src, sp, 20, 3); dp = _put_at(dst, dp, src, sp, 16, 4); dp = _put_at(dst, dp, src, sp, 0, 16)
    elif length == 24:
        dp = _put_at(dst, dp, src, sp, 16, 8); dp = _put_at(dst, dp, src, sp, 0, 16)
    elif length == 25:
        dp = _put_at(dst, dp, src, sp, 17, 8); dp = _put_at(dst, dp, src, sp, 16, 1); dp = _put_at(dst, dp, src, sp, 0, 16)
    elif length == 26:
        dp = _put_at(dst, dp, src, sp, 25, 1); dp = _put_at(dst, dp, src, sp, 17, 8); dp = _put_at(dst, dp, src, sp, 16, 1); dp = _put_at(dst, dp, src, sp, 0, 16)
    elif length == 27:
        dp = _put_at(dst, dp, src, sp, 25, 2); dp = _put_at(dst, dp, src, sp, 17, 8); dp = _put_at(dst, dp, src, sp, 16, 1); dp = _put_at(dst, dp, src, sp, 0, 16)
    elif length == 28:
        dp = _put_at(dst, dp, src, sp, 24, 4); dp = _put_at(dst, dp, src, sp, 16, 8); dp = _put_at(dst, dp, src, sp, 0, 16)
    elif length == 29:
        dp = _put_at(dst, dp, src, sp, 28, 1); dp = _put_at(dst, dp, src, sp, 24, 4); dp = _put_at(dst, dp, src, sp, 16, 8); dp = _put_at(dst, dp, src, sp, 0, 16)
    elif length == 30:
        dp = _put_at(dst, dp, src, sp, 28, 2); dp = _put_at(dst, dp, src, sp, 24, 4); dp = _put_at(dst, dp, src, sp, 16, 8); dp = _put_at(dst, dp, src, sp, 0, 16)
    elif length == 31:
        dp = _put_at(dst, dp, src, sp, 30, 1); dp = _put_at(dst, dp, src, sp, 26, 4); dp = _put_at(dst, dp, src, sp, 18, 8); dp = _put_at(dst, dp, src, sp, 2, 16); dp = _put_at(dst, dp, src, sp, 0, 2)
    else:
        raise DecompressError(f"wrong literal length {length}")
    return dp, sp + length


def _read_literal_length(src: bytes, sp: int, src_end: int, opcode: int) -> tuple[int, int]:
    """read_literal_length，返回 (length, 新 sp)。"""
    length = opcode + 8
    if length == 0x17:
        if sp >= src_end:
            raise DecompressError("literal length: read past end")
        n = src[sp]; sp += 1
        length += n
        if n == 0xFF:
            while True:
                if src_end - sp < 2:
                    raise DecompressError("literal length: read past end (16)")
                n = src[sp] | (src[sp + 1] << 8); sp += 2
                length += n
                if n != 0xFFFF:
                    break
    return length, sp


def _read_instructions(src: bytes, sp: int, src_end: int, opcode: int):
    """read_instructions，返回 (opcode, offset, length, 新 sp)。"""
    def need(n: int):
        if src_end - sp < n:
            raise DecompressError("instructions: read past end")

    hi = opcode >> 4
    if hi == 0:
        length = (opcode & 0xF) + 0x13
        need(2)
        offset = src[sp]; sp += 1
        opcode = src[sp]; sp += 1
        length = ((opcode >> 3) & 0x10) + length
        offset = ((opcode & 0x78) << 5) + 1 + offset
    elif hi == 1:
        length = (opcode & 0xF) + 3
        need(2)
        offset = src[sp]; sp += 1
        opcode = src[sp]; sp += 1
        offset = ((opcode & 0xF8) << 5) + 1 + offset
    elif hi == 2:
        need(2)
        offset = src[sp]; sp += 1
        offset = ((src[sp] << 8) & 0xFF00) | offset; sp += 1
        length = opcode & 7
        if (opcode & 8) == 0:
            need(1)
            opcode = src[sp]; sp += 1
            length = (opcode & 0xF8) + length
        else:
            offset += 1
            need(1)
            length = (src[sp] << 3) + length; sp += 1
            need(1)
            opcode = src[sp]; sp += 1
            length = (((opcode & 0xF8) << 8) + length) + 0x100
    else:
        length = opcode >> 4
        offset = opcode & 15
        need(1)
        opcode = src[sp]; sp += 1
        offset = (((opcode & 0xF8) << 1) + offset) + 1
    return opcode, offset, length, sp


def decompress_r2007(dst_size: int, src: bytes, src_size: int | None = None,
                     trace: bool = False) -> bytearray:
    """decompress_r2007：解压到 dst_size 字节；返回解压缓冲。"""
    if src_size is None:
        src_size = len(src)
    if dst_size == 0 or src_size < 2:
        raise DecompressError("invalid argument to decompress_r2007")

    dst = bytearray(dst_size)
    dp = 0
    src_end = src_size
    sp = 0
    length = 0
    offset = 0

    opcode = src[sp]; sp += 1
    if (opcode & 0xF0) == 0x20:
        sp += 2
        length = src[sp] & 0x07; sp += 1
        if length == 0:
            raise DecompressError("zero length")

    while sp < src_end:
        if length == 0:
            length, sp = _read_literal_length(src, sp, src_end, opcode)
        if trace:
            print(f"  [lit] len={length} sp={sp} dp={dp}")
        if dp + length > dst_size:
            raise DecompressError("literal length overflow (dst)")
        if sp + length > src_end:
            raise DecompressError("literal length overflow (src)")
        dp, sp = _copy_compressed_at(dst, dp, src, sp, length)

        length = 0
        if sp >= src_end:
            break
        opcode = src[sp]; sp += 1
        opcode, offset, length, sp = _read_instructions(src, sp, src_end, opcode)
        if trace:
            print(f"  [match] op=0x{opcode:02X} off={offset} len={length} dp={dp} sp={sp}")
        while True:
            if offset > dp:
                raise DecompressError(f"offset underflow off={offset} dp={dp}")
            for _ in range(length):
                dst[dp] = dst[dp - offset]
                dp += 1
            length = opcode & 7
            if length != 0 or sp >= src_end:
                break
            opcode = src[sp]; sp += 1
            if (opcode >> 4) == 0:
                break
            if (opcode >> 4) == 0x0F:
                opcode &= 0xF
            opcode, offset, length, sp = _read_instructions(src, sp, src_end, opcode)
            if trace:
                print(f"  [match+] op=0x{opcode:02X} off={offset} len={length} dp={dp}")
    return dst
