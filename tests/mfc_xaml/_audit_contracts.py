# -*- coding: utf-8 -*-
"""审计：控件短名 vs XamlEvents 事件契约表 vs 注册标签 的覆盖差。"""
import os
import re

CTRL = r'G:\PyArx\MfcXamlCore\controls'
EV = os.path.join(CTRL, 'XamlEvents.h')
REG = os.path.join(CTRL, 'RegisterPyMfcControls.cpp')


def read(p):
    for enc in ('utf-8-sig', 'utf-8', 'gbk'):
        try:
            return open(p, encoding=enc).read()
        except UnicodeDecodeError:
            continue
    return open(p, encoding='utf-8', errors='replace').read()


def table_keys():
    txt = read(EV)
    body = txt.split('kTable = {', 1)[1].split('\n    };', 1)[0]
    return set(re.findall(r'\{L"([a-z0-9_]+)"', body))


def ctrl_shortnames():
    out = {}
    for fn in os.listdir(CTRL):
        if not (fn.startswith('Xaml') and fn.endswith('.h')):
            continue
        txt = read(os.path.join(CTRL, fn))
        m = re.search(r'XamlMfcControlBase\(L"([a-z0-9_]+)"\)', txt)
        if m:
            out.setdefault(m.group(1), []).append(fn)
    return out


def registered_tags():
    txt = read(REG)
    tags = re.findall(r'L"([A-Za-z][A-Za-z0-9_ ]*)"', txt)
    return tags


def main():
    keys = table_keys()
    shorts = ctrl_shortnames()
    tags = registered_tags()
    print('契约表 key 数:', len(keys))
    print('控件的 type() 短名数:', len(shorts))
    missing = sorted(k for k in shorts if k not in keys)
    print('没有事件契约的控件短名 (%d):' % len(missing))
    for k in missing:
        print('  %-14s <- %s' % (k, ','.join(shorts[k])))
    extra = sorted(keys - set(shorts))
    print('契约表里没有对应控件类 (%d): %s' % (len(extra), ', '.join(extra)))
    lower = {t.lower() for t in tags}
    untagged = sorted(k for k in shorts if k not in lower)
    print('短名在注册标签里找不到的 (%d): %s' % (len(untagged), ', '.join(untagged)))


if __name__ == '__main__':
    main()
