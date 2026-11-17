import io, sys
p = r'G:\PyArx\PyMfcXml\PyMfcCommon.h'
src = io.open(p, encoding='utf-8').read()
lines = src.split('\n')
start = None
for i, ln in enumerate(lines):
    if ln.startswith('static void drawPowerPlanContent'):
        start = i
        break
if start is None:
    sys.exit('start not found')
# 先在签名之后找到函数体的第一个 '{' 所在行，再按花括号深度找匹配的 '}'
open_ln = None
for i in range(start, len(lines)):
    if lines[i].strip().endswith('{') or lines[i].strip() == '{':
        open_ln = i
        break
depth = 0
end = None
for i in range(open_ln, len(lines)):
    depth += lines[i].count('{') - lines[i].count('}')
    if depth <= 0 and i > open_ln:
        end = i
        break
print('start=%d open=%d end=%d' % (start, open_ln, end))
print('open line:', lines[open_ln][:70])
print('end line:', lines[end][:70])
assert lines[end].strip() == '}', lines[end]
new = [
    u'// 画法本体已下沉到控件库（MfcXmlCore\\XmlPowerPlanDraw.h）：',
    u'// 屏幕自绘画布 / Viewer 预览 / 设计器表面 / 离屏出图共用同一份实现，三端不会再各画一套。',
    u'static void drawPowerPlanContent(XmlCanvas& cv, const xmlplan::PowerPlan& plan,',
    u'                                 float k, int ox, int oy, const std::wstring& sel,',
    u'                                 bool showCable = true, const RECT* clip = nullptr)',
    u'{',
    u'    xmlplan::drawPlan(cv, plan, k, ox, oy, sel, showCable, clip);',
    u'}',
]
lines[start:end+1] = new
for i, ln in enumerate(lines):
    if ln.strip() == '#include "XmlPowerPlan.h"':
        lines.insert(i+1, '#include "XmlPowerPlanDraw.h"  // 配电规划图绘制层（三端共用）')
        break
io.open(p, 'w', encoding='utf-8', newline='').write('\n'.join(lines))
print('done')
