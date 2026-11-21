# -*- coding: utf-8 -*-
"""R2004 (AC1018) 18-bit LZ77 变体解压，严格按 ODA 规范 §4.7（p32-34）。

分支：
  0x10  Long Offset 长度 +9；Two Byte Offset +0x3FFF
  0x11  流结束
  0x12–0x1F  compressedBytes=(op&0xF)+2；2bo +0x3FFF
  0x20  Long Offset 长度 +0x21；2bo 无 plus
  0x21–0x3F  compressedBytes=op-0x1E；2bo 无 plus
  0x40–0xFF  compressedBytes=((op&0xF0)>>4)-1；op2 拼偏移；litCount=op&3
Two Byte Offset：offset=(first>>2)|(second<<6)，litCount=first&3。
"""
from __future__ import annotations


class DecompressError(Exception):
    pass


class _State:
    __slots__ = ("src", "pos", "end", "out", "limit", "base", "two_offset", "out_pos")

    def __init__(self, src: bytes, max_out: int, base: int, out: bytearray | None = None,
                 out_pos: int = 0):
        self.src = src
        self.pos = 0
        self.end = len(src)
        self.out = out if out is not None else bytearray(max_out)
        self.limit = max_out
        self.base = base
        self.two_offset = 0
        self.out_pos = out_pos

    def next(self) -> int:
        if self.pos >= self.end:
            raise DecompressError(f"input overrun @0x{self.base + self.pos:X}")
        b = self.src[self.pos]
        self.pos += 1
        return b

    def copy_bytes(self, n: int) -> int:
        """拷贝 n 个字面字节，再读一字节返回作为下一 opcode1。"""
        if n == 0:
            return self.next()
        end = self.pos + n
        if end > self.end:
            raise DecompressError("literal run past input")
        if self.out_pos + n > self.limit:
            raise DecompressError("literal run past output")
        for k in range(n):
            self.out[self.out_pos + k] = self.src[self.pos + k]
        self.out_pos += n
        self.pos = end
        return self.next()

    def emit_match(self, length: int, offset: int) -> None:
        if offset == 0:
            if self.out_pos == 0:
                raise DecompressError("offset 0 match on empty output")
            last = self.out[self.out_pos - 1]
            if self.out_pos + length > self.limit:
                raise DecompressError("match past output")
            self.out[self.out_pos:self.out_pos + length] = bytes([last]) * length
            self.out_pos += length
            return
        start = self.out_pos - offset
        if start < 0:
            raise DecompressError(f"back-reference before start: off={offset} out={self.out_pos}")
        if self.out_pos + length > self.limit:
            raise DecompressError("match past output")
        for _ in range(length):
            self.out[self.out_pos] = self.out[start]
            start += 1
            self.out_pos += 1


def _read_literal_length(st: "_State", opcode: int) -> int:
    """read_literal_length（源码 1160）。"""
    low = opcode & 0x0F
    if low == 0:
        last = 0
        while True:
            last = st.next()
            if last == 0 and st.pos < st.end:
                low += 0xFF
            else:
                break
        low += 0xF + last
    return low + 3


def _compressed_bytes(st: "_State", opcode: int, bits: int) -> int:
    """read_compressed_bytes（源码 1180）。"""
    n = opcode & bits
    if n == 0:
        last = 0
        while True:
            last = st.next()
            if last == 0 and st.pos < st.end:
                n += 0xFF
            else:
                break
        n += last + bits
    return n + 2


def _long_offset(st: "_State") -> int:
    """Long Compression Offset（规范 p34）。"""
    op = st.next()
    if op == 0x00:
        total = 0xFF
        while True:
            b = st.next()
            if b == 0x00:
                total += 0xFF
            else:
                total += b
                break
        return total
    return op


def _two_byte_offset(st: "_State", plus: int, high: int = 0) -> tuple[int, int]:
    """Two Byte Offset（规范 p34；源码 1201，high 先并入偏移，最后再加 plus）。"""
    first = st.next()
    second = st.next()
    offset = high | (first >> 2) | (second << 6)
    offset += plus
    return offset, first & 0x03



def decompress(src: bytes, max_out: int, base: int = 0, trace: bool = False,
               out: bytearray | None = None, out_pos: int = 0):
    st = _State(src, max_out, base, out=out, out_pos=out_pos)
    if st.end == 0:
        return bytes(st.out)

    opcode1 = st.next()
    if (opcode1 & 0xF0) == 0:
        lit_length = _read_literal_length(st, opcode1)
        opcode1 = st.copy_bytes(lit_length)
        if trace:
            print(f"  [lit start] n={lit_length} pos={st.pos} out={st.out_pos}")
    else:
        raise DecompressError(f"first byte 0x{opcode1:02X} is not a literal length")

    while st.pos < st.end and st.out_pos < st.limit and opcode1 != 0x11:
        if trace:
            print(f"  [op1] 0x{opcode1:02X} pos={st.pos} out={st.out_pos}")

        comp_bytes = 0
        comp_offset = 0
        lit_count = 0

        if opcode1 == 0x11:
            break
        elif 0x10 <= opcode1 <= 0x1F:  # C decode.c 1269：0x10 同 0x12-0x1F
            comp_bytes = _compressed_bytes(st, opcode1, 7)
            high = (opcode1 & 8) << 11
            comp_offset, lit_count = _two_byte_offset(st, 0x4000, high)
        elif 0x20 <= opcode1 <= 0x3F:  # 源码 1277
            comp_bytes = _compressed_bytes(st, opcode1, 0x1F)
            comp_offset, lit_count = _two_byte_offset(st, 1)
        else:  # 0x40–0xFF（规范 p33）
            comp_bytes = (opcode1 >> 4) - 1
            opcode2 = st.next()
            comp_offset = (((opcode1 >> 2) & 3) | (opcode2 << 2)) + 1
            lit_count = opcode1 & 0x03

        if trace:
            print(f"      match len={comp_bytes} off={comp_offset} lit={lit_count}")
        if trace:
            print(f"  [PY] op=0x{opcode1:02X} src={st.pos} dec={st.out_pos} cb={comp_bytes} off={comp_offset} lit={lit_count}")

        st.emit_match(comp_bytes, comp_offset)

        if lit_count == 0:
            opcode1 = st.next()
            if (opcode1 & 0xF0) == 0:
                lit_count = _read_literal_length(st, opcode1)
        if lit_count:
            if st.out_pos + lit_count <= st.limit:
                opcode1 = st.copy_bytes(lit_count)
            else:
                break
        # 否则 opcode1 已是下一压缩 opcode

    return st.out
