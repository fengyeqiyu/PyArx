# -*- coding: utf-8 -*-
"""从 Chromium 的 resources.pak 里把所有小型 PNG 抽出来，供挑选内置光标图。"""
import os
import struct
import sys

OUTDIR = r'G:\PyArx\docs\_pak_cursors'
SIG = b'\x89PNG\r\n\x1a\n'


def parse(path):
    with open(path, 'rb') as f:
        data = f.read()
    version = struct.unpack('<I', data[:4])[0]
    pos = 4
    resources = []
    if version == 4:
        count = struct.unpack('<I', data[pos:pos + 4])[0]
        pos += 4
        for _ in range(count):
            rid, off = struct.unpack('<HI', data[pos:pos + 6])
            pos += 6
            resources.append((rid, off))
    elif version == 5:
        # 实测 Edge/Chromium 的 v5 头是 32 位布局：
        # version(u32) encoding(u32) resource_count(u32) alias_count(u32)，
        # 随后每条 (id u32, offset u32)。
        encoding, count, alias_count = struct.unpack('<III', data[4:16])
        pos = 16
        for _ in range(count):
            rid, off = struct.unpack('<II', data[pos:pos + 8])
            pos += 8
            resources.append((rid, off))
        pos += 8 * alias_count
    else:
        raise SystemExit('pak version %d unsupported' % version)
    out = []
    for i, (rid, off) in enumerate(resources):
        end = resources[i + 1][1] if i + 1 < len(resources) else len(data)
        out.append((rid, data[off:end]))
    return version, out


def png_size(b):
    if len(b) < 24 or not b.startswith(SIG):
        return None
    w, h = struct.unpack('>II', b[16:24])
    return w, h


def main():
    path = sys.argv[1]
    os.makedirs(OUTDIR, exist_ok=True)
    ver, res = parse(path)
    print('version', ver, 'resources', len(res))
    hit = 0
    for rid, payload in res:
        sz = png_size(payload)
        if not sz:
            continue
        w, h = sz
        if w > 64 or h > 64 or w < 8:
            continue
        with open(os.path.join(OUTDIR, 'r%05d_%dx%d.png' % (rid, w, h)), 'wb') as f:
            f.write(payload)
        hit += 1
    print('png dumped', hit, '->', OUTDIR)


if __name__ == '__main__':
    main()
