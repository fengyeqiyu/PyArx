import io, re, os
d = r'G:\PyArx\PyMfcXaml\parts'
all_parts = {}
for f in sorted(os.listdir(d)):
    all_parts[f] = io.open(os.path.join(d, f), encoding='utf-8').read()

txt = all_parts['02-host.inc']
lines = txt.split('\n')
print('02-host.inc 行数 %d' % len(lines))

# 扫描类体内的方法定义（顶格 4 空格的方法签名行，后跟 { 或同行 {）
meth = re.compile(r'^    (?:static\s+|virtual\s+|inline\s+)*([\w:<>,\*&\s]+?)\s+(\w+)\s*\([^;]*\)\s*(?:const)?\s*\{?\s*$')
defs = []
for i, l in enumerate(lines):
    m = re.match(r'^    (?:static\s+|virtual\s+|inline\s+)*(?:[\w:<>]+[\s\*&]+)+(\w+)\s*\(', l)
    if m and not l.strip().startswith(('//', 'if', 'for', 'while', 'return', 'switch', 'case')):
        name = m.group(1)
        # 排除声明（以分号结尾）
        decl_tail = l.rstrip()
        if decl_tail.endswith(';'):
            continue
        defs.append((i + 1, name, l.strip()[:90]))

print('疑似方法定义 %d 个' % len(defs))
# 统计每个方法名在其他位置出现次数
names = {}
for ln, name, sig in defs:
    names.setdefault(name, []).append(ln)
total_ref = {}
for name in names:
    total_ref[name] = sum(p.count(name) for p in all_parts.values())
dead = [(names[n], n, total_ref[n]) for n in names if total_ref[n] <= 1]
print('\n仅定义一次、无任何引用的方法（疑似死代码）：%d' % len(dead))
for lns, n, c in sorted(dead)[:60]:
    print('   行 %-6s %-34s 引用 %d' % (','.join(map(str, lns)), n, c))
