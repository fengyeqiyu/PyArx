# -*- coding: utf-8 -*-
"""把 RegisterPyMfcControls.cpp 的标签改成统一 X 前缀、每控件只留一个规范名。

读一遍现有 alias 列表 -> 第一个元素是主名 -> 断言主名唯一 -> 重写为 {L"X<主名>"}。
"""
import io
import os
import re

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SRC = os.path.join(ROOT, 'MfcXamlCore', 'controls', 'RegisterPyMfcControls.cpp')

CALL_RE = re.compile(r'(RegisterOne\(reg, \[\]\(\) -> IXamlControl\* \{ return new [A-Za-z]+\(\); \},\s*)'
                     r'\{([^}]*)\}\);', re.S)
MAIN_RE = re.compile(r'L"([A-Za-z]+)"')


def main():
    text = io.open(SRC, encoding='utf-8').read()
    mains = []
    pairs = []

    def repl(m):
        head, body = m.group(1), m.group(2)
        names = MAIN_RE.findall(body)
        if not names:
            raise SystemExit('未解析到标签: %s' % body)
        main_name = names[0]
        mains.append(main_name)
        pairs.append((main_name, names))
        return '%s{L"%s"});' % (head, 'X' + main_name)

    new = CALL_RE.sub(repl, text)

    dup = [n for n in set(mains) if mains.count(n) > 1]
    if dup:
        raise SystemExit('主名重复（同一标签注册了两份实现）: %s' % dup)

    io.open(SRC, 'w', encoding='utf-8', newline='\n').write(new)
    print('控件 %d 个 -> 统一 X 前缀，删除别名 %d 条'
          % (len(mains), sum(len(p[1]) - 1 for p in pairs)))
    for main_name, names in pairs:
        if len(names) > 1:
            print('  X%-14s 丢弃别名 %s' % (main_name, ', '.join(names[1:])))


if __name__ == '__main__':
    main()
