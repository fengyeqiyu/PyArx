# -*- coding: utf-8 -*-
"""重新拆分扩展控件：源文本从 XamlContentParse.h 里抢救出来（它误存了完整原文）。

与原脚本相比修了两个 bug：
  1) .h 里除生成声明外，还要**丢掉**原来那段多行签名（之前它是被"保留"了）；
  2) .cpp 的函数定义签名要去掉夹在里面的 // 注释（之前整行被压成一行，注释把代码吃掉了）。
"""
import os
import re

ROOT = r'G:\PyArx\MfcXamlCore\controls'
PARSE = os.path.join(ROOT, 'XamlContentParse.h')
ORIG = os.path.join(ROOT, 'XamlExtraControls.orig.h')

SRC_HEADER = '''#pragma once
// XamlExtraControls —— 扩展 XAML 控件集（对齐 wxFormBuilder 常见控件）。
// 每个类继承 XamlMfcControlBase，只实现 createNative/properties/setProperty。
// 本文件 UTF-8 with BOM。

#include "XamlMfcControlBase.h"

'''

SIG_RE = re.compile(r'^(?:virtual\s+)?[\w:<>,\*&\s]+?\b(\w+)\s*\(.*\)\s*(?:const\s*)?(?:override\s*)?$', re.S)


def read(p):
    with open(p, 'r', encoding='utf-8-sig') as f:
        return f.read()


def write(p, t):
    with open(p, 'w', encoding='utf-8-sig', newline='\r\n') as f:
        f.write(t)


def brace_body(text, start):
    depth = 0
    i = start
    n = len(text)
    while i < n:
        ch = text[i]
        if ch == '"' or ch == "'":
            q = ch
            i += 1
            while i < n:
                if text[i] == '\\':
                    i += 2
                    continue
                if text[i] == q:
                    break
                i += 1
        elif ch == '/' and i + 1 < n and text[i + 1] == '/':
            while i < n and text[i] != '\n':
                i += 1
            continue
        elif ch == '/' and i + 1 < n and text[i + 1] == '*':
            j = text.find('*/', i + 2)
            if j < 0:
                return None, -1
            i = j + 2
            continue
        elif ch == '{':
            depth += 1
        elif ch == '}':
            depth -= 1
            if depth == 0:
                return text[start + 1:i], i + 1
        i += 1
    return None, -1


def strip_comments(s):
    s = re.sub(r'//[^\n]*', '', s)
    return re.sub(r'/\*.*?\*/', '', s, flags=re.S)


def tail_sig(seg):
    """seg 末尾那个签名（含注释），去掉 public:/protected: 标签与前面残留语句。"""
    seg = re.sub(r'\n\s*(?:public|protected|private)\s*:\s*', '\n', seg)
    chunks = [c for c in re.split(r';', seg) if c.strip()]
    return chunks[-1].strip() if chunks else ''


def split_members(body, cls_name):
    """返回 (新声明体, [(定义签名, 函数体内文, 声明单行)])"""
    out = []
    moves = []
    i = 0
    n = len(body)
    last = 0
    while i < n:
        ch = body[i]
        if ch == '"' or ch == "'":
            q = ch
            i += 1
            while i < n:
                if body[i] == '\\':
                    i += 2
                    continue
                if body[i] == q:
                    break
                i += 1
            i += 1
            continue
        if ch == '/' and i + 1 < n and body[i + 1] == '/':
            while i < n and body[i] != '\n':
                i += 1
            continue
        if ch == '/' and i + 1 < n and body[i + 1] == '*':
            j = body.find('*/', i + 2)
            i = (j + 2) if j >= 0 else n
            continue
        if ch == '{':
            inner, end = brace_body(body, i)
            if inner is None:
                raise RuntimeError('大括号不配对: %s' % cls_name)
            seg = body[last:i]
            sig_raw = tail_sig(seg)
            sig_one = re.sub(r'\s+', ' ', strip_comments(sig_raw)).strip()
            m = SIG_RE.match(sig_one)
            if m and m.group(1) != cls_name and 'override' in sig_one:
                # 丢掉原文里的签名（多行），换成一行声明
                pos = seg.rfind(sig_raw)
                if pos < 0:
                    raise RuntimeError('签名定位失败: %s / %s' % (cls_name, sig_one[:60]))
                out.append(body[last:last + pos])
                out.append('    ' + sig_one + ';\n')
                # 定义签名：保留 /* */ 内联注释，去掉 // 行注释
                def_sig = '\n'.join(l for l in sig_raw.split('\n')
                                    if not l.strip().startswith('//')).strip()
                def_sig = re.sub(r'\s+', ' ', def_sig)
                moves.append((def_sig, inner, sig_one))
            else:
                out.append(body[last:end])
            last = end
            i = end
            continue
        i += 1
    out.append(body[last:])
    return ''.join(out), moves


def find_class_blocks(text):
    lines = text.split('\n')
    starts = [(i, re.match(r'^class\s+(\w+)\s*:\s*public\s+(\w+)', ln))
              for i, ln in enumerate(lines)]
    starts = [(i, m.group(1), m.group(2)) for i, m in starts if m]
    res = []
    for idx, (i, name, parent) in enumerate(starts):
        c0 = i
        while c0 - 1 >= 0 and lines[c0 - 1].lstrip().startswith('//'):
            c0 -= 1
        end = starts[idx + 1][0] if idx + 1 < len(starts) else len(lines)
        j = end
        while j - 1 > i and lines[j - 1].lstrip().startswith('//'):
            j -= 1
        res.append((name, parent, '\n'.join(lines[c0:i]).rstrip(), '\n'.join(lines[i:j]).rstrip()))
    return res


def qualify(sig, cls):
    one = re.sub(r'\s+', ' ', sig).strip()
    one = re.sub(r'\s*\boverride\b', '', one)
    one = re.sub(r'\s*\bvirtual\b', '', one)
    m = re.match(r'^(.*?\b)(\w+)(\s*\(.*)$', one)
    if not m:
        raise RuntimeError('签名无法限定: %s' % one)
    return m.group(1).strip() + ' ' + cls + '::' + m.group(2) + m.group(3)


def main():
    # ---- 1) 抢救原文 ----
    parse = read(PARSE)
    k = parse.find('namespace xamlextra')
    if k < 0:
        print('XamlContentParse.h 里没有 namespace xamlextra，无法恢复')
        return 1
    orig = SRC_HEADER + parse[k:]
    if not orig.rstrip().endswith('};'):
        # 抢救时按 "\n}" 截断，最后那个类的 "};" 掉了
        orig = orig.rstrip() + '\n};\n'
    write(ORIG, orig)
    print('原文已还原: %s（%d 行）' % (ORIG, len(orig.split('\n'))))
    print('  override 次数: %d' % len(re.findall(r'\boverride\b', strip_comments(orig))))

    blocks = find_class_blocks(orig)
    print('  类数: %d' % len(blocks))

    # ---- 2) 逐个类拆 ----
    total = 0
    includes = []
    for name, parent, comment, block in blocks:
        lb = block.find('{')
        body, _ = brace_body(block, lb)
        decl, moves = split_members(body, name)
        total += len(moves)
        head = comment if comment else '// ' + name
        header = ('#pragma once\n'
                  '#include "XamlMfcControlBase.h"\n'
                  '#include "XamlContentParse.h"\n\n'
                  + head + '\n'
                  'class ' + name + ' : public ' + parent + '\n{\n' + decl.rstrip() + '\n};\n')
        write(os.path.join(ROOT, name + '.h'), header)

        cpp = ['// %s.cpp —— 实现（拆分自 XamlExtraControls.h，UTF-8 with BOM）' % name,
               '#include "%s.h"' % name, '']
        for def_sig, inner, _ in moves:
            cpp.append(qualify(def_sig, name))
            cpp.append('{' + inner + '}')
            cpp.append('')
        write(os.path.join(ROOT, name + '.cpp'), '\n'.join(cpp))
        includes.append('#include "%s.h"' % name)
        print('  %-22s <- %-18s 函数 %d' % (name, parent, len(moves)))

    print('函数总数: %d' % total)

    # ---- 3) XamlContentParse.h 只留 namespace xamlextra ----
    ns = orig[orig.find('namespace xamlextra'):]
    ns_body, ns_end = brace_body(ns, ns.find('{'))
    ns_text = 'namespace xamlextra\n{' + ns_body + '}'
    parse_h = ('#pragma once\n'
               '// XamlContentParse —— 内容类属性（Pages/Columns/Rows/Nodes）的公共解析。\n'
               '// 声明了属性却不实现，XAML 里的内容会被静默丢弃，控件就只剩空壳，\n'
               '// 所以这几个解析函数是"控件能用"和"只有样子"的分界线。\n'
               '// 本文件 UTF-8 with BOM。\n\n'
               '#include "XamlMfcControlBase.h"\n\n' + ns_text + '\n')
    write(PARSE, parse_h)

    # ---- 4) 聚合头 ----
    agg = ('#pragma once\n'
           '// XamlExtraControls —— 扩展控件聚合头。\n'
           '// 拆分后每个控件各有一份 XamlXxx.h / XamlXxx.cpp，本文件只做统一 include，\n'
           '// 原来的 #include "XamlExtraControls.h" 继续可用。\n'
           '// 本文件 UTF-8 with BOM。\n\n'
           + '\n'.join(includes) + '\n')
    write(os.path.join(ROOT, 'XamlExtraControls.h'), agg)
    print('完成。')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
