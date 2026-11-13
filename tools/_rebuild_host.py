import io, os
B = r'G:\PyArx\docs\_backup_PyMfcXaml.cpp'
blines = io.open(B, encoding='utf-8-sig').read().split('\n')
host = blines[443:6262]          # 备份 444..6262 = class PyMfcHost 全段
print('重建 02-host 段 %d 行' % len(host))

# 边界自检（重建后的 1-based 行号）
for n in (1165, 1166, 2608, 2609, 3442, 3443, 3778, 3779, 4609, 4610, 5155, 5156):
    print('%6d |%s' % (n, host[n - 1]))
