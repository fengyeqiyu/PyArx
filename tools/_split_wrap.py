import io, re, os
import declsplit as ds

src = 'G:/PyArx/PyMfcXaml/parts/04-wrap.inc'
lines = io.open(src, encoding='utf-8-sig').read().split('\n')
sc = ds.scan(lines)

CLASSES = ('PyMfcTopWnd', 'PyMfcWindow', 'PyMfcPanel', 'PyMfcStore')

hdr_parts = []   # 头文件内容块
cpp_parts = []   # cpp 内容块
member_defs = []
fails = 0

i = 0
while i < len(lines):
    line = lines[i]
    s = line.strip()

    # 开头那一行切分来源注释丢掉
    if s.startswith('// PyMfcTopWnd / PyMfcWindow'):
        i += 1
        continue

    # 匿名 namespace：内容进头（PyMfcNoDelete / PyMfcHandle 两个 TU 都要用同一份类型）
    if s == 'namespace':
        start = i + 2                     # 跳过 'namespace' 和 '{'
        dep = 1
        i += 2
        inner = []
        while i < len(lines) and dep > 0:
            if lines[i].strip() == '}' and dep == 1:
                break
            inner.append(lines[i])
            dep += sc[i][0]
            i += 1
        i += 1                            # 吃掉 '}'
        hdr_parts.append('\n'.join(inner).strip('\n'))
        continue

    m = re.match(r'(class|struct)\s+(PyMfc\w+)\b', s)
    if m and m.group(2) in CLASSES:
        name = m.group(2)
        # 收整块
        start = i
        dep = 0
        opened = False
        while i < len(lines):
            dep += sc[i][0]
            opened = opened or sc[i][1]
            i += 1
            if opened and dep == 0:
                break
        block = lines[start:i]
        if name == 'PyMfcStore':
            hdr_parts.append('\n'.join(block))
            continue
        # 类头 + 类体
        open_idx = next(j for j, l in enumerate(block) if l.strip() == '{')
        class_head = block[:open_idx + 1]
        body = block[open_idx + 1:-1]     # 去掉最后的 '};'
        tail_line = block[-1]
        h, d, f = ds.split_class(body, name)
        fails += f
        hdr_parts.append('\n'.join(class_head) + '\n' + h + '\n' + tail_line)
        member_defs.extend(d)
        continue

    # 其余顶层内容（BEGIN_MESSAGE_MAP / 类外定义）
    cpp_parts.append(line)
    i += 1

hdr = ('// PyMfcWrap.h —— Python 侧的窗口/面板包装（PyMfcWindow / PyMfcPanel / PyMfcStore）。\n'
       '// 类体内只留声明，实现在 PyMfcWrap.cpp。\n\n'
       '#pragma once\n\n'
       '#include "PyMfcCommon.h"\n'
       '#include "PyMfcHost.h"\n\n'
       + '\n\n'.join(x for x in hdr_parts if x.strip()) + '\n')

cpp = ('// PyMfcWrap.cpp —— PyMfcTopWnd / PyMfcWindow / PyMfcPanel 的实现。\n\n'
       '#include "PyMfcWrap.h"\n\n'
       + '\n'.join(cpp_parts).strip('\n') + '\n\n'
       + '\n\n'.join(member_defs) + '\n')

io.open('G:/PyArx/PyMfcXaml/PyMfcWrap.h', 'w', encoding='utf-8', newline='').write(hdr)
io.open('G:/PyArx/PyMfcXaml/PyMfcWrap.cpp', 'w', encoding='utf-8-sig', newline='').write(cpp)
print('PyMfcWrap.h %d 行；PyMfcWrap.cpp %d 行；失败 %d 段；成员定义 %d 个'
      % (len(hdr.split('\n')), len(cpp.split('\n')), fails, len(member_defs)))
