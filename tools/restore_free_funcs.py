# -*- coding: utf-8 -*-
"""把类外的两个自由函数（xamlExtraNum / xamlSplitItems）补回 XamlContentParse.h。

它们夹在类与类之间，拆分按"类体"切块时自然被丢掉了 —— 几十个 .cpp 都要用。
"""
import os
import re

ORIG = r'G:\PyArx\MfcXamlCore\controls\XamlExtraControls.orig.h'
PARSE = r'G:\PyArx\MfcXamlCore\controls\XamlContentParse.h'


def read(p):
    with open(p, 'r', encoding='utf-8-sig') as f:
        return f.read()


def write(p, t):
    with open(p, 'w', encoding='utf-8-sig', newline='\r\n') as f:
        f.write(t)


def brace_body(text, start):
    depth = 0
    i = start
    while i < len(text):
        ch = text[i]
        if ch == '"' or ch == "'":
            q = ch
            i += 1
            while i < len(text):
                if text[i] == '\\':
                    i += 2
                    continue
                if text[i] == q:
                    break
                i += 1
        elif ch == '{':
            depth += 1
        elif ch == '}':
            depth -= 1
            if depth == 0:
                return text[start + 1:i], i + 1
        i += 1
    return None, -1


orig = read(ORIG)
parse = read(PARSE)
if 'xamlExtraNum' in parse:
    print('已经补过了，跳过')
    raise SystemExit(0)

wanted = ['inline int xamlExtraNum(', 'inline std::vector<std::wstring> xamlSplitItems(']
chunks = []
for w in wanted:
    k = orig.find(w)
    if k < 0:
        print('找不到: %s' % w)
        continue
    # 往上带一句注释
    line_start = orig.rfind('\n', 0, k) + 1
    prev = orig.rfind('\n', 0, line_start - 1) + 1
    if orig[prev:line_start].strip().startswith('//'):
        k = prev
    lb = orig.find('{', k)
    body, end = brace_body(orig, lb)
    chunks.append(orig[k:end].rstrip())

extra = '\n\n'.join(chunks)
# 放到 namespace 之外：现有文件以 "namespace xamlextra { ... }" 结尾
parse = parse.rstrip() + '\n\n// ---- 类外公共小工具（原 XamlExtraControls.h 里的自由函数） ----\n' + extra + '\n'
write(PARSE, parse)
print('已补 %d 个函数，XamlContentParse.h 现有 %d 行' % (len(chunks), len(parse.split('\n'))))
