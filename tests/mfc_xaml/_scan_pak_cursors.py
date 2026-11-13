# -*- coding: utf-8 -*-
"""在 Chromium 的 .pak 里按光标二进制签名扫描（ICONDIR: 00 00 02 00 + 条目数），
把命中的条目原样存成 .cur，再用 LoadImageW 验证能不能当光标加载。"""
import ctypes
import os
import struct
import sys

OUT = r'G:\PyArx\docs\_pak_cur'
SIG = b'\x00\x00\x02\x00'
LOAD_LIBRARY_AS_DATAFILE = 0


def parse_pak(path):
    with open(path, 'rb') as f:
        data = f.read()
    version = struct.unpack('<I', data[:4])[0]
    if version != 5:
        return []
    encoding, count, alias = struct.unpack('<III', data[4:16])
    pos = 16
    ents = []
    for _ in range(count):
        rid, off = struct.unpack('<II', data[pos:pos + 8])
        pos += 8
        ents.append((rid, off))
    out = []
    for i, (rid, off) in enumerate(ents):
        end = ents[i + 1][1] if i + 1 < len(ents) else len(data)
        out.append((rid, data[off:end]))
    return out


def looks_like_cur(b):
    if len(b) < 22 or b[:4] != SIG:
        return False
    n = struct.unpack('<H', b[4:6])[0]
    if not (1 <= n <= 16):
        return False
    if 6 + 16 * n > len(b):
        return False
    ok = 0
    for i in range(n):
        w, h, c, r, planes, bits, size, off = struct.unpack('<BBBBHHII', b[6 + 16 * i:6 + 22 + 16 * i])
        if 8 <= (w or 256) <= 256 and (h or 256) <= 256 and bits in (1, 4, 8, 24, 32) and off + size <= len(b):
            ok += 1
    return ok == n


def verify(path):
    u = ctypes.windll.user32
    u.LoadImageW.restype = ctypes.c_void_p
    u.LoadImageW.argtypes = [ctypes.c_void_p, ctypes.c_wchar_p, ctypes.c_uint,
                             ctypes.c_int, ctypes.c_int, ctypes.c_uint]
    return bool(u.LoadImageW(None, path, 2, 0, 0, 0x10))


def main():
    os.makedirs(OUT, exist_ok=True)
    found = 0
    for p in sys.argv[1:]:
        if not os.path.exists(p):
            print('missing', p)
            continue
        ents = parse_pak(p)
        hits = 0
        for rid, payload in ents:
            if looks_like_cur(payload):
                name = '%s_r%d.cur' % (os.path.basename(p).split('.')[0], rid)
                dst = os.path.join(OUT, name)
                with open(dst, 'wb') as f:
                    f.write(payload)
                good = verify(dst)
                hits += 1
                found += 1
                print('  hit id=%d size=%d load=%s' % (rid, len(payload), good))
        print(os.path.basename(p), 'entries=%d cur-hits=%d' % (len(ents), hits))
    print('total', found, '->', OUT)


if __name__ == '__main__':
    main()
