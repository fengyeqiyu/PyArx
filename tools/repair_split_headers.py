# -*- coding: utf-8 -*-
"""修正 split_extra_controls 生成结果的两种残留：

1) .h 里"原始多行签名"没被删掉（它后面紧跟着同名声明、以 override; 结尾）；
2) .cpp 里签名行把前置的 // 注释一起压成一行了。

修复依据：.h 里那条正确声明（以 override; 结尾）。.h 声明顺序 == .cpp 函数块顺序，
所以按序号一一替换 .cpp 的函数头即可。
"""
import os
import re

ROOT = r'G:\PyArx\MfcXamlCore\controls'
DECL_RE = re.compile(r'^    (.*?\b\w+\s*\(.*\)\s*(?:const\s*)?override);\s*$')


def read_text(p):
    with open(p, 'r', encoding='utf-8-sig') as f:
        return f.read()


def write_text(p, t):
    with open(p, 'w', encoding='utf-8-sig', newline='\r\n') as f:
        f.write(t)


def header_stop_line(ln):
    """回溯终止条件：这一行是完整语句/标签/注释，不是签名碎片。"""
    s = ln.strip()
    if not s:
        return True
    if s.startswith('//') or s.startswith('/*'):
        return True
    return s.endswith((';', '{', '}', ':')) or s.endswith('{}')


def clean_header(text):
    lines = text.split('\n')
    out = []
    decls = []
    for ln in lines:
        m = DECL_RE.match(ln)
        if m:
            # 删掉紧邻在它前面的原始签名碎片
            while out and not header_stop_line(out[-1]):
                out.pop()
            decls.append(m.group(1).strip())
        out.append(ln)
    return '\n'.join(out), decls


def make_def(decl, cls):
    one = re.sub(r'\s*\boverride\b', '', decl).strip()
    one = re.sub(r'\s*\bvirtual\b', '', one)
    lp = one.rfind('(')
    head = one[:lp]
    m = re.match(r'^(.*?)(\w+)\s*$', head, re.S)
    prefix, name = m.group(1).strip(), m.group(2)
    return prefix + ' ' + cls + '::' + name + one[lp:]


def fix_cpp(text, cls, decls):
    lines = text.split('\n')
    out = []
    idx = 0
    for i, ln in enumerate(lines):
        nxt = lines[i + 1] if i + 1 < len(lines) else ''
        if nxt.lstrip().startswith('{') and idx < len(decls):
            out.append(make_def(decls[idx], cls))
            idx += 1
            continue
        out.append(ln)
    return '\n'.join(out), idx


def main():
    pairs = sorted(f[:-2] for f in os.listdir(ROOT)
                   if f.endswith('.h') and f.startswith('Xaml')
                   and os.path.exists(os.path.join(ROOT, f[:-2] + '.cpp')))
    print('待修: %d 组' % len(pairs))
    for cls in pairs:
        hp = os.path.join(ROOT, cls + '.h')
        cp = os.path.join(ROOT, cls + '.cpp')
        h = read_text(hp)
        newh, decls = clean_header(h)
        changed_h = newh != h
        if changed_h:
            write_text(hp, newh)
        c = read_text(cp)
        newc, n = fix_cpp(c, cls, decls)
        changed_c = newc != c
        if changed_c:
            write_text(cp, newc)
        flag = []
        if changed_h:
            flag.append('h')
        if changed_c:
            flag.append('cpp')
        if n != len(decls):
            flag.append('数量不符 decls=%d cpp=%d' % (len(decls), n))
        print('  %-22s 声明 %2d  修正 %s' % (cls, len(decls), ','.join(flag) if flag else '无'))


if __name__ == '__main__':
    main()
