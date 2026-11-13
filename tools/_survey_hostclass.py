import io, re
d = r'G:\PyArx\PyMfcXaml\parts'
host = ''
for f in ('02a-host-core.inc','02b-host-widgets.inc','02c-host-fill.inc','02d-host-export.inc',
          '02e-host-canvas.inc','02f-host-anim.inc','02g-host-props.inc'):
    t = io.open(d + '\\' + f, encoding='utf-8').read().split('\n')
    # 去掉分片首行标题注释
    host += '\n'.join(t[1:]) + '\n'
lines = host.split('\n')
print('类体合计 %d 行' % len(lines))

# 顶层（4 空格）结构统计
stats = {'access':0,'nested':0,'member':0,'method_same_line':0,'method_next_line':0,'other':0}
samples = {'method_next_line':[], 'nested':[]}
for i, l in enumerate(lines):
    if not l.strip():
        continue
    cls = l.startswith('    ') and not l.startswith('     ')
    if not cls:
        continue
    s = l.strip()
    if s in ('public:','private:','protected:'):
        stats['access'] += 1
    elif re.match(r'^(struct|class|enum|union|using|typedef|friend|template)\b', s):
        stats['nested'] += 1
        if len(samples['nested']) < 15: samples['nested'].append((i+1, s[:80]))
    elif s.endswith('{') and '(' in s:
        stats['method_same_line'] += 1
    elif s.endswith(';'):
        stats['member'] += 1
    elif '(' in s and not s.endswith('{'):
        # 可能是多行签名
        stats['method_next_line'] += 1
        if len(samples['method_next_line']) < 25: samples['method_next_line'].append((i+1, s[:80]))
    else:
        stats['other'] += 1
print(stats)
print('\n多行签名样本：')
for x in samples['method_next_line']: print('  %6d %s' % x)
print('\n嵌套类型样本：')
for x in samples['nested']: print('  %6d %s' % x)
