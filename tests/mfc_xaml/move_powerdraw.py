# -*- coding: utf-8 -*-
"""把 drawPowerPlanContent 从 PyMfcHost 所在命名空间挪到全局命名空间。

原因：它原先插在 PyMfcHost 之前，但那段代码处在一个 namespace 内部，
而 RenderPowerPlanPng（离屏渲染）在全局命名空间，于是报 C3861。
"""
import io

SRC = r'G:\PyArx\PyMfcXaml\PyMfcXaml.cpp'
with io.open(SRC, 'r', encoding='utf-8-sig', newline='') as f:
    text = f.read()

BEGIN = '// -----------------------------------------------------------------------------\r\n// 配电图绘制：屏幕自绘画布与离屏出图共用同一套画法。'
if text.find(BEGIN) < 0:
    BEGIN = BEGIN.replace('\r\n', '\n')
END = '// 重画这个控件自己（自绘控件在自己的 WM_PAINT 里画，所以只失效自己）'

i = text.find(BEGIN)
if i < 0:
    raise SystemExit('找不到绘制函数起点')
j = text.find(END, i)
if j < 0:
    raise SystemExit('找不到绘制函数终点')
block = text[i:j]
text = text[:i] + text[j:]

# 插到第一个 namespace 之前（全局作用域）
k = text.find('namespace')
while k > 0 and text[k - 1] == ':':   # 跳过 "using namespace" 之类
    k = text.find('namespace', k + 1)
if k < 0:
    raise SystemExit('找不到可插入的位置')
text = text[:k] + block + text[k:]

with io.open(SRC, 'w', encoding='utf-8-sig', newline='') as f:
    f.write(text)
print('移动完成，块长度 %d' % len(block))
