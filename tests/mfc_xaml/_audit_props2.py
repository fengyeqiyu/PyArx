# -*- coding: utf-8 -*-
"""审计：逐个控件列出"声明了哪些属性"与"实现认哪些名字"，指出该补的别名。"""
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


def dump():
    for fn in sorted(os.listdir(CTRL)):
        if not (fn.startswith('Xaml') and fn.endswith('.cpp')):
            continue
        txt = read(os.path.join(CTRL, fn))
        if 'ownProperties' not in txt:
            continue
        seg = txt.split('ownProperties', 1)[1].split('setOwnProperty', 1)[0]
        declared = re.findall(r'\{L"([^"]+)"(?:,\s*(XamlPropType::[A-Za-z]+))?', seg)
        setseg = txt.split('setOwnProperty', 1)[1] if 'setOwnProperty' in txt else ''
        setseg = setseg.split('getOwnProperty', 1)[0]
        handled = re.findall(r'StrEq\(key,\s*"([a-z0-9_.]+)"', setseg)
        if not declared:
            continue
        print(fn)
        print('  declared:', ', '.join('%s:%s' % (n, t or '?') for n, t in declared))
        print('  handled :', ', '.join(sorted(set(handled))) or '-')
        dec_low = {n.lower() for n, _ in declared}
        miss = [h for h in sorted(set(handled)) if h not in dec_low]
        print('  extra   :', ', '.join(miss) or '-')


if __name__ == '__main__':
    dump()
