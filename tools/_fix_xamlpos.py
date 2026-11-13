import io
P = r'G:\PyArx\PyMfcXaml\PyMfcXaml.cpp'
lines = io.open(P, encoding='utf-8-sig').read().split('\n')

def block_range(sig):
    """从包含 sig 的行开始，到花括号平衡结束（含）。返回 (start, end)。"""
    s = next(i for i, l in enumerate(lines) if sig in l)
    depth = 0
    started = False
    for i in range(s, len(lines)):
        depth += lines[i].count('{') - lines[i].count('}')
        if '{' in lines[i]:
            started = True
        if started and depth == 0:
            return s, i
    raise SystemExit('未找到结束: ' + sig)

r1 = block_range('XamlElement* FindXamlByName')
r2 = block_range('py::dict XamlLayoutRects')
b1 = '\n'.join(lines[r1[0]:r1[1] + 1])
b2 = '\n'.join(lines[r2[0]:r2[1] + 1])
print('FindXamlByName %d..%d, XamlLayoutRects %d..%d' % (r1[0], r1[1], r2[0], r2[1]))

# 从后往前删
for r in sorted([r1, r2], reverse=True):
    del lines[r[0]:r[1] + 1]

# 缩进规整成 4 空格基础
funcs = []
for b in (b1, b2):
    out = []
    for l in b.split('\n'):
        out.append(('    ' + l) if l.strip() and not l.startswith('    ') else l)
    funcs.append('\n'.join(out))

anchor = '    constexpr int kFlowMargin = 12;'
i_anchor = next(i for i, l in enumerate(lines) if anchor in l)
lines[i_anchor:i_anchor] = funcs[0].split('\n') + [''] + funcs[1].split('\n') + ['']

io.open(P, 'w', encoding='utf-8-sig', newline='').write('\n'.join(lines))
print('两个函数已移到匿名 namespace 开头')
