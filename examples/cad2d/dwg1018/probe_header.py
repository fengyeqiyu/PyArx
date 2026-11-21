# -*- coding: utf-8 -*-
"""R2004 (AC1018) 容器层探针：解密 0x80 处 0x6C 字节文件头数据，核对魔数串与节地图定位。

参考：ODA Specification §4.1 R2004 File Header。
"""
import struct
import sys

MAGIC = bytes([
    0x29, 0x23, 0xBE, 0x84, 0xE1, 0x6C, 0xD6, 0xAE, 0x52, 0x90, 0x49, 0xF1, 0xF1, 0xBB, 0xE9, 0xEB,
    0xB3, 0xA6, 0xDB, 0x3C, 0x87, 0x0C, 0x3E, 0x99, 0x24, 0x5E, 0x0D, 0x1C, 0x06, 0xB7, 0x47, 0xDE,
    0xB3, 0x12, 0x4D, 0xC8, 0x43, 0xBB, 0x8B, 0xA6, 0x1F, 0x03, 0x5A, 0x7D, 0x09, 0x38, 0x25, 0x1F,
    0x5D, 0xD4, 0xCB, 0xFC, 0x96, 0xF5, 0x45, 0x3B, 0x13, 0x0D, 0x89, 0x0A, 0x1C, 0xDB, 0xAE, 0x32,
    0x20, 0x9A, 0x50, 0xEE, 0x40, 0x78, 0x36, 0xFD, 0x12, 0x49, 0x32, 0xF6, 0x9E, 0x7D, 0x49, 0xDC,
    0xAD, 0x4F, 0x14, 0xF2, 0x44, 0x40, 0x66, 0xD0, 0x6B, 0xC4, 0x30, 0xB7,
])


def gen_magic(n=0x6C):
    """按规范伪代码生成 LCG 魔数串（校验上面的硬编码表）。

    规范伪代码：``randseed *= 0x343fd; randseed += 0x269ec3; *p++ = randseed >> 0x10``，
    即取 32 位 LCG 的最高字节（bits 24..31），不是高 16 位的低字节。
    """
    seed = 1
    out = bytearray()
    for _ in range(n):
        seed = (seed * 0x343FD + 0x269EC3) & 0xFFFFFFFF
        out.append((seed >> 24) & 0xFF)
    return bytes(out)


def main(path):
    data = open(path, "rb").read()
    print("file len", len(data), "version", data[:6])
    magic = gen_magic()
    print("magic matches spec table:", magic == MAGIC)

    # 固定字段
    print("0x0B maintenance =", data[0x0B])
    print("0x0C byte =", data[0x0C])
    preview_addr = struct.unpack_from("<L", data, 0x0D)[0]
    print("0x0D preview addr =", hex(preview_addr))
    print("0x11 app ver =", data[0x11], "0x12 app maint =", data[0x12])
    codepage = struct.unpack_from("<H", data, 0x13)[0]
    print("0x13 codepage =", codepage)
    security = struct.unpack_from("<L", data, 0x18)[0]
    print("0x18 security flags =", hex(security))
    summary = struct.unpack_from("<L", data, 0x20)[0]
    print("0x20 summary addr =", hex(summary))

    # 0x80 起 0x6C 字节异或解密
    enc = data[0x80:0x80 + 0x6C]
    hdr = bytes(b ^ m for b, m in zip(enc, magic))
    print("--- decrypted 2004 header data (0x6C) ---")
    for i in range(0, len(hdr), 16):
        hx = " ".join(f"{b:02X}" for b in hdr[i:i + 16])
        asc = "".join(chr(b) if 32 <= b < 127 else "." for b in hdr[i:i + 16])
        print(f"{i:04X}  {hx:<47} {asc}")

    file_id = hdr[0:12]
    print("file id string:", file_id)
    fields = {
        0x0C: ("root tree node gap?", "<I"),
        0x10: ("size 0x6c?", "<I"),
        0x14: ("root tree node gap", "<I"),
        0x1C: ("low left gap", "<I"),
        0x20: ("low right gap", "<I"),
        0x24: ("unknown (ODA=1)", "<I"),
        0x28: ("last section page Id", "<I"),
        0x2C: ("last section page end addr", "<Q"),
        0x34: ("second header data addr", "<Q"),
        0x3C: ("gap amount", "<I"),
        0x40: ("section page amount", "<I"),
        0x44: ("const 0x40", "<I"),
        0x48: ("const 0x80", "<I"),
        0x4C: ("const 0x40", "<I"),
        0x50: ("section page map Id", "<I"),
        0x54: ("section page map addr", "<Q"),
        0x5C: ("section map Id", "<I"),
        0x60: ("section page array size", "<I"),
        0x64: ("gap array size", "<I"),
        0x68: ("crc32", "<I"),
    }
    for off, (name, fmt) in fields.items():
        v = struct.unpack_from(fmt, hdr, off)[0]
        print(f"  hdr+0x{off:02X} {name}: {v}" + (f" (0x{v:X})" if v > 9 else ""))


if __name__ == "__main__":
    main(sys.argv[1])
