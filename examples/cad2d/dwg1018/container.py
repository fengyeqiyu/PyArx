# -*- coding: utf-8 -*-
"""R2004 容器层：文件头解密、系统/数据页定位与解压（ODA 规范 §4.1–4.6）。"""
from __future__ import annotations

import struct
from dataclasses import dataclass, field
from typing import Dict, List, Optional, Tuple

try:
    from .compress import decompress, DecompressError
except ImportError:  # 直接以脚本目录运行
    from compress import decompress, DecompressError

# 系统页类型签名（小端）
SIG_PAGE_MAP = 0x41630E3B
SIG_SECTION_MAP = 0x4163003B
# 数据页类型签名（掩码后恒为 0x4163043B）
SIG_DATA_PAGE = 0x4163043B
SEC_MASK = 0x4164536B

PAGE_BOUNDARY = 0x20
FIRST_SECTION_ADDR = 0x100


def _magic(n: int = 0x6C) -> bytes:
    seed = 1
    out = bytearray()
    for _ in range(n):
        seed = (seed * 0x343FD + 0x269EC3) & 0xFFFFFFFF
        out.append((seed >> 16) & 0xFF)
    return bytes(out)


# 规范 §4.1 硬编码的 92 字节表（前 92 字节），用于交叉校验生成式。
_MAGIC_TABLE = bytes([
    0x29, 0x23, 0xBE, 0x84, 0xE1, 0x6C, 0xD6, 0xAE, 0x52, 0x90, 0x49, 0xF1, 0xF1, 0xBB, 0xE9, 0xEB,
    0xB3, 0xA6, 0xDB, 0x3C, 0x87, 0x0C, 0x3E, 0x99, 0x24, 0x5E, 0x0D, 0x1C, 0x06, 0xB7, 0x47, 0xDE,
    0xB3, 0x12, 0x4D, 0xC8, 0x43, 0xBB, 0x8B, 0xA6, 0x1F, 0x03, 0x5A, 0x7D, 0x09, 0x38, 0x25, 0x1F,
    0x5D, 0xD4, 0xCB, 0xFC, 0x96, 0xF5, 0x45, 0x3B, 0x13, 0x0D, 0x89, 0x0A, 0x1C, 0xDB, 0xAE, 0x32,
    0x20, 0x9A, 0x50, 0xEE, 0x40, 0x78, 0x36, 0xFD, 0x12, 0x49, 0x32, 0xF6, 0x9E, 0x7D, 0x49, 0xDC,
    0xAD, 0x4F, 0x14, 0xF2, 0x44, 0x40, 0x66, 0xD0, 0x6B, 0xC4, 0x30, 0xB7,
])


MAGIC = _magic()


@dataclass
class PageEntry:
    """SectionPageMap 中的一页：页号 → 文件内槽位大小。"""
    page_id: int
    section_size: int
    parent: int = 0
    left: int = 0
    right: int = 0
    is_gap: bool = False


@dataclass
class SectionDesc:
    size: int
    page_count: int
    max_page_size: int
    unknown: int
    compressed: int
    section_id: int
    encrypted: int
    name: str
    pages: List[dict] = field(default_factory=list)


@dataclass
class Container:
    path: str
    version: str
    maintenance: int
    codepage: int
    security: int
    header: Dict[str, int]
    page_map: Dict[int, PageEntry]
    sections: Dict[str, SectionDesc]
    data: bytes

    def _page_addr(self, page_id: int) -> int:
        """按页号在 SectionPageMap 中累计求绝对地址（§4.4）。"""
        addr = FIRST_SECTION_ADDR
        for pid in sorted(self.page_map):
            entry = self.page_map[pid]
            if pid == page_id:
                return addr
            addr += entry.section_size
        raise KeyError(f"page id {page_id} not in page map")

    def read_system_page(self, sig: int, trace: bool = False) -> bytes:
        """定位并解压一个系统页（页地图 / 节地图）。"""
        data = self.data
        pat = struct.pack("<I", sig)
        hit = data.find(pat)
        if hit < 0:
            raise KeyError(f"system page signature 0x{sig:08X} not found")
        # 系统页头从签名处开始：type(4) decomp(4) comp(4) ctype(4) checksum(4)
        ptype, dsize, csize, ctype, _ = struct.unpack_from("<LLLLL", data, hit)
        payload = data[hit + 0x14: hit + 0x14 + csize]
        if ctype == 2 and csize < dsize:
            return decompress(payload, dsize, hit + 0x14, trace=trace)
        return payload[:dsize]

    def read_data_page(self, page_id: int) -> tuple[bytes, dict]:
        """读一个数据页：解掩码页头 → 取 payload（不解压），返回 payload 与页头。"""
        data = self.data
        addr = self._page_addr(page_id)
        hdr_raw = struct.unpack_from("<8I", data, addr)
        mask = SEC_MASK ^ addr
        hdr = [v ^ mask for v in hdr_raw]
        ptype, sec_no, data_size, page_size, start_off, _, _, _ = hdr
        if ptype != SIG_DATA_PAGE:
            raise DecompressError(f"data page {page_id} bad type 0x{ptype:08X} @0x{addr:X}")
        payload_addr = addr + 0x20
        payload = data[payload_addr: payload_addr + data_size]
        return payload, {"page_size": page_size, "data_size": data_size,
                          "section_type": sec_no, "start_offset": start_off,
                          "compressed": data_size < page_size}

    def read_section(self, name: str) -> bytes:
        """把一节所有页解压后按 start_offset 拼到全局缓冲（§4.6 + 源码 2132）。

        全局缓冲大小为 max_decomp_size*page_count；逐页解压，输出直接写入对应
        start_offset，保留前页内容供跨页 LZ77 回引；页尾填充保持零。
        """
        s = self.sections[name]
        buf = bytearray(s.max_page_size * s.page_count)
        limit = len(buf)
        for pg in s.pages:
            payload, hdr = self.read_data_page(pg["page_id"])
            off = pg["start_offset"]
            if hdr["compressed"]:
                decompress(payload, limit, 0, out=buf, out_pos=off)
            else:
                buf[off:off + hdr["page_size"]] = payload[:hdr["page_size"]]
        return bytes(buf)


def _cstr(raw: bytes) -> str:
    return raw.split(b"\x00", 1)[0].decode("gbk", errors="replace").strip()


def open_dwg(path: str) -> Container:
    data = open(path, "rb").read()
    if data[:6] != b"AC1018":
        raise ValueError(f"unsupported version {data[:6]!r}; this decoder is AC1018 only")

    maintenance = data[0x0B]
    codepage = struct.unpack_from("<H", data, 0x13)[0]
    security = struct.unpack_from("<L", data, 0x18)[0]

    enc = data[0x80:0x80 + 0x6C]
    hdr = bytes(b ^ m for b, m in zip(enc, MAGIC))
    if hdr[0:11] != b"AcFssFcAJMB":
        raise ValueError("file header magic string mismatch; decrypt failed")

    def q(off, fmt):
        return struct.unpack_from(fmt, hdr, off)[0]

    header = {
        "last_page_id": q(0x28, "<I"),
        "last_end": q(0x2C, "<Q"),
        "second_hdr": q(0x34, "<Q"),
        "gap_amount": q(0x3C, "<I"),
        "page_amount": q(0x40, "<I"),
        "page_map_id": q(0x50, "<I"),
        "page_map_addr": q(0x54, "<Q"),
        "section_map_id": q(0x5C, "<I"),
        "page_array_size": q(0x60, "<I"),
        "gap_array_size": q(0x64, "<I"),
        "crc32": q(0x68, "<I"),
    }

    c = Container(path=path, version="AC1018", maintenance=maintenance,
                  codepage=codepage, security=security, header=header,
                  page_map={}, sections={}, data=data)

    # --- 4.4 Section Page Map ---（压缩层迭代中，失败不阻断容器信息）
    try:
        pm = c.read_system_page(SIG_PAGE_MAP, trace=False)
        pos = 0
        while pos + 8 <= len(pm):
            page_id, section_size = struct.unpack_from("<ii", pm, pos)
            pos += 8
            if page_id < 0:  # gap 节点
                parent, left, right, zero = struct.unpack_from("<iiii", pm, pos)
                pos += 16
                c.page_map[-page_id] = PageEntry(-page_id, section_size, parent, left, right, True)
            else:
                c.page_map[page_id] = PageEntry(page_id, section_size)
    except Exception as e:
        c.header["pm_decompress_error"] = f"{type(e).__name__}: {e}"

    # --- 4.5 Section Map ---
    try:
        sm = c.read_system_page(SIG_SECTION_MAP)
    except Exception as e:
        c.header["sm_decompress_error"] = f"{type(e).__name__}: {e}"
        sm = b""
    num = struct.unpack_from("<I", sm, 0x00)[0] if len(sm) >= 4 else 0
    pos = 0x14
    for _ in range(num):
        size, page_count, max_page, unknown, compressed, section_id, encrypted = \
            struct.unpack_from("<QIIIIII", sm, pos)
        pos += 0x20
        name = _cstr(sm[pos:pos + 0x40])
        pos += 0x40
        desc = SectionDesc(size, page_count, max_page, unknown, compressed,
                           section_id, encrypted, name)
        for _p in range(page_count):
            page_no, data_size, start_off = struct.unpack_from("<IIQ", sm, pos)
            pos += 0x10
            desc.pages.append({"page_id": page_no, "data_size": data_size,
                               "start_offset": start_off})
        c.sections[name] = desc

    return c
