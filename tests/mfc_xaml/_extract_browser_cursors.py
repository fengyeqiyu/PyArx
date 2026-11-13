# -*- coding: utf-8 -*-
"""直接解析 PE 资源表，把 exe/dll 里的 RT_CURSOR(1) + RT_GROUP_CURSOR(12) 全部还原成 .cur。

不调用 EnumResourceNames（Python 3.14 的 ctypes 回调会崩），纯字节解析。
"""
import os
import struct
import sys

OUTDIR = r'G:\PyArx\docs\_edge_cursors'
RT_CURSOR = 1
RT_GROUP_CURSOR = 12


def sections(data):
    pe = struct.unpack('<I', data[0x3C:0x40])[0]
    assert data[pe:pe + 4] == b'PE\0\0'
    coff = pe + 4
    nsec = struct.unpack('<H', data[coff + 2:coff + 4])[0]
    opt_size = struct.unpack('<H', data[coff + 16:coff + 18])[0]
    opt = coff + 20
    magic = struct.unpack('<H', data[opt:opt + 2])[0]
    dd = opt + (112 if magic == 0x20B else 96)
    res_rva, res_size = struct.unpack('<II', data[dd + 16:dd + 24])
    sec = opt + opt_size
    out = []
    for i in range(nsec):
        s = sec + 40 * i
        vsize, vaddr, rsize, roff = struct.unpack('<IIII', data[s + 8:s + 24])
        out.append((vaddr, max(vsize, rsize), roff))
    return res_rva, res_size, out


def rva2off(rva, secs):
    for vaddr, vsize, roff in secs:
        if vaddr <= rva < vaddr + vsize:
            return roff + (rva - vaddr)
    return None


def walk(data, res_rva, secs):
    """返回 {(type, name): (rva, size)}，type/name 可能是 int。"""
    base = rva2off(res_rva, secs)
    result = {}

    def read_dir(off):
        named, ids = struct.unpack('<HH', data[off + 12:off + 16])
        for i in range(named + ids):
            e = off + 16 + 8 * i
            nid, target = struct.unpack('<II', data[e:e + 8])
            name = nid if (nid & 0x80000000) else None
            if name is None:
                # 命名项：指向 UTF-16 字符串，这里用偏移当标识
                str_off = base + (nid & 0x7FFFFFFF)
                ln = struct.unpack('<H', data[str_off:str_off + 2])[0]
                name = data[str_off + 2:str_off + 2 + ln * 2].decode('utf-16-le', 'ignore')
            yield name, target
        return

    def is_dir(target):
        return bool(target & 0x80000000)

    def dir_off(target):
        return base + (target & 0x7FFFFFFF)

    for tname, ttarget in read_dir(base):
        if not is_dir(ttarget):
            continue
        for nname, ntarget in read_dir(dir_off(ttarget)):
            if not is_dir(ntarget):
                continue
            for lname, ltarget in read_dir(dir_off(ntarget)):
                if is_dir(ltarget):
                    continue
                off = dir_off(ltarget)
                drva, dsize = struct.unpack('<II', data[off:off + 8])
                result[(tname, nname)] = (drva, dsize)
    return result


def bytes_at(data, rva, size, secs):
    off = rva2off(rva, secs)
    if off is None:
        return None
    return data[off:off + size]


def build_cur(group, images):
    count = struct.unpack('<H', group[4:6])[0]
    entries = []
    pos = 6
    for _ in range(count):
        if pos + 14 > len(group):
            break
        w, h, colors, res, planes, bits, size, cid = struct.unpack('<BBBBHHIH', group[pos:pos + 14])
        pos += 14
        img = images.get(cid)
        if img is None:
            continue
        entries.append((w, h, colors, res, planes, bits, img))
    if not entries:
        return None
    hdr = 6 + 16 * len(entries)
    off = hdr
    dirs = b''
    body = b''
    for w, h, colors, res, planes, bits, img in entries:
        dirs += struct.pack('<BBBBHHII', w, h, colors, res, planes, bits, len(img), off)
        off += len(img)
        body += img
    return struct.pack('<HHH', 0, 2, len(entries)) + dirs + body


def process(path):
    with open(path, 'rb') as f:
        data = f.read()
    res_rva, res_size, secs = sections(data)
    if res_rva == 0:
        print(os.path.basename(path), 'no resource section')
        return 0
    items = walk(data, res_rva, secs)
    images = {}
    for (t, n), (rva, size) in items.items():
        if t == RT_CURSOR and isinstance(n, int):
            b = bytes_at(data, rva, size, secs)
            if b:
                images[n] = b
    groups = [(n, rva, size) for (t, n), (rva, size) in items.items()
              if t == RT_GROUP_CURSOR and isinstance(n, int)]
    base = os.path.basename(path).split('.')[0].lower()
    print('%s: RT_CURSOR=%d RT_GROUP_CURSOR=%d' % (base, len(images), len(groups)))
    n_ok = 0
    for gid, rva, size in groups:
        g = bytes_at(data, rva, size, secs)
        if not g:
            continue
        blob = build_cur(g, images)
        if not blob:
            continue
        with open(os.path.join(OUTDIR, '%s_g%d.cur' % (base, gid)), 'wb') as f:
            f.write(blob)
        n_ok += 1
    return n_ok


def main():
    os.makedirs(OUTDIR, exist_ok=True)
    total = 0
    for p in sys.argv[1:]:
        if os.path.exists(p):
            total += process(p)
        else:
            print('missing', p)
    print('cursors written', total, '->', OUTDIR)


if __name__ == '__main__':
    main()
