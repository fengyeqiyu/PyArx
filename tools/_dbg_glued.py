import io
lines = io.open(r'G:\PyArx\docs\_backup_PyMfcHost.h', encoding='utf-8').read().split('\n')
import re
hits = [(i + 1, l) for i, l in enumerate(lines) if re.match(r'\s*\}\s{2,}[^\s/]', l)]
print('形如 "}  代码" 的行：%d' % len(hits))
for h in hits: print('  %5d |%s' % h)
