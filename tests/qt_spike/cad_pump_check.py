# -*- coding: utf-8 -*-
"""只读：报告当前泵的帧数（用于验证脚本返回后定时器是否自驱）"""
import os
import sys
sys.path.insert(0, r'G:\PyArx\PyArxQml')
os.add_dll_directory(r'G:\PyArx\X64\Release')
import PyArxQml
with open(r'G:\PyArx\_pump_stats2.txt', 'w', encoding='utf-8') as f:
    f.write(str(PyArxQml.stats()))