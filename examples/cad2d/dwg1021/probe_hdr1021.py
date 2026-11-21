# -*- coding: utf-8 -*-
"""R2007 (AC1021) 文件头探针：RS 解交织 -> decompress_r2007 -> 解析头字段。

对照 docs/libredwg_src/src/decode_r2007.c read_file_header + dwg.h Dwg_R2007_Header。
"""
from __future__ import annotations

import struct
import sys
from pathlib import Path

try:
    from compress1021 import decode_rs, decompress_r2007, DecompressError
except ImportError:
    sys.path.insert(0, str(Path(__file__).resolve().parent))
    from compress1021 import decode_rs, decompress_r2007, DecompressError

HERE = Path(__file__).resolve().parent
DEFAULT = HERE / "测试图-V2007.dwg"

HDR_FIXED = 0x3D8
RS_K = 239

# Dwg_R2007_Header 字段顺序（dwg.h 11217+，全部 LE64）
HDR_FIELDS = [
    "header_size", "file_size", "pages_map_crc_compressed", "pages_map_correction",
    "pages_map_crc_seed", "pages_map2_offset", "pages_map2_id", "pages_map_offset",
    "pages_map_id", "header2_offset", "pages_map_size_comp", "pages_map_size_uncomp",
    "pages_amount", "pages_maxid", "unknown1", "unknown2", "pages_map_crc_uncomp",
    "unknown3", "unknown4", "unknown5", "num_sections", "sections_map_crc_uncomp",
    "sections_map_size_comp", "sections_map2_id", "sections_map_id",
    "sections_map_size_uncomp", "sections_map_crc_comp", "sections_map_correction",
    "sections_map_crc_seed", "stream_version", "crc_seed", "crc_seed_encoded",
    "random_seed", "header_crc",
]


def main() -> int:
    path = Path(sys.argv[1]) if len(sys.argv) > 1 else DEFAULT
    data = path.read_bytes()
    print(f"file={path.name} size={len(data)}")
    if data[:6] != b"AC1021":
        print(f"!! version {data[:6]!r} not AC1021")
        return 1
    print(f"maintenance=0x{data[0x0B]:02X} codepage=0x{struct.unpack_from('<H', data, 0x13)[0]:04X}")

    enc = data[0x80:0x80 + HDR_FIXED]
    pedata = decode_rs(enc, 3, RS_K)
    seq_crc, seq_key, compr_crc = struct.unpack_from("<QQQ", pedata, 0)
    compr_len, len2 = struct.unpack_from("<ii", pedata, 24)
    print(f"compr_len={compr_len} len2={len2}")

    out_size = len(HDR_FIELDS) * 8
    if compr_len > 0:
        hdr = decompress_r2007(out_size, pedata[32:], min(len(pedata) - 32, compr_len))
    else:
        hdr = bytearray(pedata[32:32 + out_size])

    vals = struct.unpack_from(f"<{len(HDR_FIELDS)}Q", hdr, 0)
    for name, v in zip(HDR_FIELDS, vals):
        print(f"  {name:28s}=0x{v:016X} ({v})")

    file_size = vals[HDR_FIELDS.index("file_size")]
    stream_ver = vals[HDR_FIELDS.index("stream_version")]
    print(f"check file_size={file_size} expect={len(data)} -> {'OK' if file_size == len(data) else 'MISMATCH'}")
    print(f"check stream_version=0x{stream_ver:X} expect=0x60100 -> {'OK' if stream_ver == 0x60100 else '??'}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
