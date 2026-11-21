# -*- coding: utf-8 -*-
"""R2007 (AC1021) 容器层：系统页 RS 解交织+解压、页地图、节地图解析。

对照 decode_r2007.c：read_system_page / read_pages_map / read_sections_map。
位读复用 dwg1018/bitio.py 的 BitStream（RLL 为通用 DWG 位码，两版本同口径）。
"""
from __future__ import annotations

import struct
import sys
from pathlib import Path
from dataclasses import dataclass, field
from typing import List, Optional

from compress1021 import decode_rs, decompress_r2007, DecompressError

# 复用 1018 位流原语（RC/RS/RL/RLL）
_BT = Path(__file__).resolve().parent.parent / "dwg1018"
if str(_BT) not in sys.path:
    sys.path.insert(0, str(_BT))
from bitio import BitStream  # noqa: E402

RS_K = 239          # 系统页 RS 数据字节/块
RS_K_DATA = 251     # 数据页 RS 数据字节/块（0xFB）
RS_N = 255
HEADER_FIXED = 0x3D8
OVERREAD = 0x28
PAGES_MAP_BASE = 0x480  # read_pages_map 内 page->offset 累计基准


@dataclass
class R2007Page:
    size: int
    page_id: int
    offset: int


@dataclass
class R2007SectionPage:
    offset: int
    size: int
    page_id: int
    uncomp_size: int
    comp_size: int
    checksum: int
    crc: int


@dataclass
class R2007Section:
    data_size: int
    max_size: int
    encrypted: int
    hashcode: int
    name_length: int
    unknown: int
    encoded: int
    num_pages: int
    name: str
    pages: List[R2007SectionPage] = field(default_factory=list)


def read_system_page(data: bytes, addr: int, size_comp: int, size_uncomp: int,
                      correction: int) -> bytes:
    """read_system_page：RS 解交织 → 解压（或原样拷贝），返回解压后节缓冲。"""
    pesize = ((size_comp + 7) & ~7) * correction
    block_count = (pesize + RS_K - 1) // RS_K
    if block_count <= 0:
        raise DecompressError("invalid system page block_count")
    page_size = (block_count * RS_N + 7) & ~7
    if addr + page_size > len(data):
        raise DecompressError(f"system page overrun @0x{addr:X} page_size={page_size}")
    rsdata = data[addr:addr + page_size]
    pedata = decode_rs(rsdata, block_count, RS_K)
    pedata_size = block_count * RS_K
    if size_comp < size_uncomp:
        return bytes(decompress_r2007(size_uncomp, pedata, min(pedata_size, size_comp)))
    return bytes(pedata[:size_uncomp])


def parse_pages_map(buf: bytes, max_count: int = 0) -> List[R2007Page]:
    """read_pages_map：逐条 RLL(size, id)，offset 从 0x480 累计。max_count=pages_amount。"""
    bs = BitStream(buf)
    pages: List[R2007Page] = []
    offset = PAGES_MAP_BASE
    while bs.byte < len(buf):
        if max_count and len(pages) >= max_count:
            break
        size = bs.read_rll()
        pid = bs.read_rll()
        pages.append(R2007Page(size=size, page_id=pid, offset=offset))
        offset += size
    return pages


def _read_name(bs: BitStream, name_length: int) -> str:
    if name_length & 1:
        name_length += 1
    raw = bytes(bs.read_rc() for _ in range(name_length))
    return raw.decode("utf-16-le", errors="replace").strip("\x00")


def parse_sections_map(buf: bytes) -> List[R2007Section]:
    """read_sections_map：节记录 + 每节页列表，字段均 RLL。"""
    bs = BitStream(buf)
    sections: List[R2007Section] = []
    while bs.byte < len(buf):
        sec = R2007Section(
            data_size=bs.read_rll(), max_size=bs.read_rll(), encrypted=bs.read_rll(),
            hashcode=bs.read_rll(), name_length=bs.read_rll(), unknown=bs.read_rll(),
            encoded=bs.read_rll(), num_pages=bs.read_rll(), name="")
        sec.name = _read_name(bs, sec.name_length)
        for _ in range(sec.num_pages):
            sec.pages.append(R2007SectionPage(
                offset=bs.read_rll(), size=bs.read_rll(), page_id=bs.read_rll(),
                uncomp_size=bs.read_rll(), comp_size=bs.read_rll(),
                checksum=bs.read_rll(), crc=bs.read_rll()))
        sections.append(sec)
    return sections


def _page_size_if_rs_coded(size_comp: int) -> int:
    """page_size_if_rs_coded：页磁盘大小恰为 RS 编码形式时才走 RS 路径。"""
    pesize = (size_comp + 7) & ~7
    block_count = (pesize + RS_K_DATA - 1) // RS_K_DATA
    return (block_count * RS_N + 31) & ~31


def _read_data_page(data: bytes, page: R2007Page, comp_size: int,
                    uncomp_size: int) -> bytes:
    """read_data_page：RS 解交织(k=251) → 解压（或原样），返回单页解压字节。"""
    pesize = (comp_size + 7) & ~7
    block_count = (pesize + RS_K_DATA - 1) // RS_K_DATA
    pedata_size = block_count * RS_K_DATA
    rsdata = data[page.offset:page.offset + page.size]
    pedata = decode_rs(rsdata, block_count, RS_K_DATA)
    if comp_size < uncomp_size:
        return bytes(decompress_r2007(uncomp_size, pedata, min(pedata_size, comp_size)))
    return bytes(pedata[:uncomp_size])


def read_section(data: bytes, section: R2007Section,
                 pages: List[R2007Page]) -> bytes:
    """read_data_section：逐页解压/原样拷贝，按 section_page.offset 拼到全局缓冲。"""
    buf = bytearray(section.data_size)
    for sp in section.pages:
        page = find_page(pages, sp.page_id)
        if not page:
            raise DecompressError(f"page {sp.page_id} not in pages map")
        if sp.comp_size != sp.uncomp_size or page.size == _page_size_if_rs_coded(sp.comp_size):
            page_bytes = _read_data_page(data, page, sp.comp_size, sp.uncomp_size)
        else:
            page_bytes = data[page.offset:page.offset + sp.uncomp_size]
        buf[sp.offset:sp.offset + sp.uncomp_size] = page_bytes
    return bytes(buf)


def find_page(pages: List[R2007Page], page_id: int) -> Optional[R2007Page]:
    for p in pages:
        if p.page_id == page_id:
            return p
    return None
