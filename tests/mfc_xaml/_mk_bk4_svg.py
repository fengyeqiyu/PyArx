# -*- coding: utf-8 -*-
cur = open(r'G:\PyArx\MfcXamlCore\XamlPowerPlan.cpp', encoding='utf-8-sig').read()

new_block = (
'            // 与屏上绘制一致：4 个拐点连成正交折线（L 命令），不再输出三次曲线 C。\r\n'
'            ss << L"<path d=\\"M " << l.bez[0] << L" " << l.bez[1];\r\n'
'            for (int q = 1; q < 4; ++q)\r\n'
'            {\r\n'
'                const double qx = l.bez[q * 2], qy = l.bez[q * 2 + 1];\r\n'
'                const double px = l.bez[(q - 1) * 2], py = l.bez[(q - 1) * 2 + 1];\r\n'
'                if (qx != px || qy != py)\r\n'
'                    ss << L" L " << qx << L" " << qy;\r\n'
'            }\r\n'
'            ss << L"\\" fill=\\"none\\" stroke=\\"#2F6FBA\\" stroke-width=\\"2\\"/>";'
)

old_block = (
'            ss << L"<path d=\\"M " << l.bez[0] << L" " << l.bez[1]\r\n'
'               << L" C " << l.bez[2] << L" " << l.bez[3]\r\n'
'               << L" " << l.bez[4] << L" " << l.bez[5]\r\n'
'               << L" " << l.bez[6] << L" " << l.bez[7]\r\n'
'               << L"\\" fill=\\"none\\" stroke=\\"#2F6FBA\\" stroke-width=\\"2\\"/>";'
)

# 兼容 LF 行尾
if new_block.replace('\r\n', '\n') in cur:
    new_block = new_block.replace('\r\n', '\n')
    old_block = old_block.replace('\r\n', '\n')

print('matched:', new_block in cur)
orig = cur.replace(new_block, old_block)
open(r'G:\PyArx\docs\_bk4_XamlPowerPlan.cpp', 'w', encoding='utf-8-sig', newline='').write(orig)
print('C cmd in backup:', orig.count(' L" C '))
