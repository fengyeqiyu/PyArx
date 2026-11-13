import io, re
load = io.open('G:/PyArx/PyMfcXaml/parts/05-load.inc', encoding='utf-8-sig').read()
bind = io.open('G:/PyArx/PyMfcXaml/parts/06-bind.inc', encoding='utf-8-sig').read()
# 05 里定义的函数名：形如 标识符( 且前一行不是 return/if 等
names = set(re.findall(r'^\s{4}([\w:<>\*&, ]+?)\b(\w+)\s*\([^;]*\)\s*(?:const)?\s*$', load, re.M)[0:0])
defs = re.findall(r'^    ([A-Za-z_][\w:<>\*&, ]*?)\b([A-Za-z_]\w*)\s*\(([^;{}]*)\)\s*$', load, re.M)
fns = sorted({d[1] for d in defs})
print('05 中形似函数定义的候选名 %d 个' % len(fns))
used = [f for f in fns if re.search(r'\b' + f + r'\b', bind)]
print('06 里引用到的：')
for u in used: print('   ', u)
