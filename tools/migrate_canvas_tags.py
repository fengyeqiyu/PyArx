# -*- coding: utf-8 -*-
"""把 DesignCanvas.h 里的标签直比改成 tagIs（tagIs 内部已去 X 前缀）。"""
import io
import os
import re

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SRC = os.path.join(ROOT, 'UIDesign', 'DesignCanvas.h')

EQ = re.compile(r'(\b(?:node->tag|child\.tag|m_selPath->tag|n->tag))\s*==\s*L"([A-Za-z]+)"')
NE = re.compile(r'(\b(?:node->tag|child\.tag|n->tag))\s*!=\s*L"([A-Za-z]+)"')


def main():
    text = io.open(SRC, encoding='utf-8').read()
    # 只需要 node / child / n 这三种上下文；m_selPath 单独保留原样（它是根判定，稍后手改）
    sub_eq = lambda m: 'tagIs(%s, L"%s")' % (
        'node' if m.group(1).startswith('node') else ('child' if m.group(1).startswith('child') else 'n'),
        m.group(2))
    sub_ne = lambda m: '!tagIs(%s, L"%s")' % (
        'node' if m.group(1).startswith('node') else ('child' if m.group(1).startswith('child') else 'n'),
        m.group(2))

    text, n1 = EQ.subn(sub_eq, text)
    text, n2 = NE.subn(sub_ne, text)
    io.open(SRC, 'w', encoding='utf-8', newline='\n').write(text)
    print('改写 == %d 处，!= %d 处' % (n1, n2))


if __name__ == '__main__':
    main()
