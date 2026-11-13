# -*- coding: utf-8 -*-
"""取 Qt Quick 侧栏的 stats 并抓图。"""
import sys
sys.path.insert(0, r'G:\PyArx\tests\qt_spike')
import QtQuickSpike

print('[qtquick stats]', QtQuickSpike.stats())
print('[qtquick grab]', QtQuickSpike.grab())
