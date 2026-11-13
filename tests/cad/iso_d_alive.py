# -*- coding: utf-8 -*-
"""隔离测试阶段 D：普通脚本再跑一次，命令通道还在就写这行。"""
import time

with open(r'G:\PyArx\docs\_isolate.log', 'a', encoding='utf-8') as f:
    f.write('%s 阶段 D：命令通道仍然可用\n' % time.strftime('%H:%M:%S'))
