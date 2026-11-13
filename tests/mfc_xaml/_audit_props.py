# -*- coding: utf-8 -*-
"""审计：ownProperties 声明了 Name/别名，但 setOwnProperty 没有对应分支的控件。"""
import os
import re

CTRL = r'G:\PyArx\MfcXamlCore\controls'


def read(p):
    for enc in ('utf-8-sig', 'utf-8', 'gbk'):
        try:
            return open(p, encoding=enc).read()
        except UnicodeDecodeError:
            continue
    return open(p, encoding='utf-8', errors='replace').read()


def main():
    decl_only = []
    alias_only = []
    for fn in sorted(os.listdir(CTRL)):
        if not (fn.startswith('Xaml') and fn.endswith('.cpp')):
            continue
        txt = read(os.path.join(CTRL, fn))
        if 'ownProperties' not in txt:
            continue
        # 表里声明的属性名
        seg = txt.split('ownProperties', 1)[1]
        seg = seg.split('setOwnProperty', 1)[0]
        declared = set(re.findall(r'\{L"([A-Za-z][A-Za-z0-9_.]*)"', seg))
        setseg = txt.split('setOwnProperty', 1)[1] if 'setOwnProperty' in txt else ''
        setseg = setseg.split('getOwnProperty', 1)[0]
        handled = set(m.lower() for m in re.findall(r'StrEq\(key,\s*"([a-z0-9_.]+)"', setseg))
        miss = sorted(d.lower() for d in declared if d.lower() not in handled and d.lower() != 'name')
        if miss:
            decl_only.append((fn, miss))
        # 实现里有别名，但表里没声明
        gotseg = txt.split('getOwnProperty', 1)[1] if 'getOwnProperty' in txt else ''
        got = set(m.lower() for m in re.findall(r'StrEq\(key,\s*"([a-z0-9_.]+)"', gotseg))
        extra = sorted(g for g in got if g not in set(d.lower() for d in declared))
        if extra:
            alias_only.append((fn, extra))
    print('== 表里声明、set 里没分支（写了返回 false，静默丢） ==')
    for fn, m in decl_only:
        print('  %-26s %s' % (fn, ', '.join(m)))
    print('== 实现认别名、表里没声明（诊断/回读看不到这个名字） ==')
    for fn, m in alias_only:
        print('  %-26s %s' % (fn, ', '.join(m)))


if __name__ == '__main__':
    main()
