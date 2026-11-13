# -*- coding: utf-8 -*-
"""停掉 Qt 的泵定时器（用于对照测量 CPU 开销）。"""
import sys
sys.path.insert(0, r'G:\PyArx\tests\qt_spike')
import QtSpike
QtSpike.stop()
print('[qt_spike] pump stopped')
