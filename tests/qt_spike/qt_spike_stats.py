# -*- coding: utf-8 -*-
"""取泵统计 + 抓图 + 记录 Qt DLL 实际路径（跨版本验证），结果写日志文件。"""
import sys
sys.path.insert(0, r'G:\PyArx\tests\qt_spike')
import QtSpike

LOG = r'G:\PyArx\docs\_qt_in_cad.log'
stats = QtSpike.stats()
grab = QtSpike.grab()
lines = ['[stats] %s' % stats, '[grab] %s' % grab]
with open(LOG, 'a', encoding='utf-8') as f:
    f.write('\n'.join(lines) + '\n')
print('\n'.join(lines))
