# -*- coding: utf-8 -*-
"""步骤 3：再跑一次普通 PYRUNFILE，证明命令通道没被打坏。"""
LOG = r'G:\PyArx\docs\_step.log'

with open(LOG, 'a', encoding='utf-8') as f:
    f.write('--- 步骤 3：命令通道仍然可用（这条能写进来就是好的） ---\n')
