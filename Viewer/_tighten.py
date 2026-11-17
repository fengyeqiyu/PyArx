# -*- coding: utf-8 -*-
"""重读后的收尾：把 #1（ZAL1）的框收紧到柜体 y<=772435。"""
import json

CA = r'G:\PyArx\viewer\export\cabinets_all.json'
C = json.load(open(CA, encoding='utf-8'))
for o in C['cabinets']:
    if o.get('name') == '101-1-1ZAL1':
        if o['box'][3] > 772435:
            o['box'][3] = 772435
            o['w'] = ((o.get('w') or '') + ' 框收紧到柜体').strip()
json.dump(C, open(CA, 'w', encoding='utf-8'), ensure_ascii=False)
print('已收紧 ZAL1 框')
