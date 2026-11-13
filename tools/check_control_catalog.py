# -*- coding: utf-8 -*-
"""核对 control_catalog.xml：XML 合法、与 prop_schema() 的主标签对齐、并列出目录里多出来的标签。"""
import json
import os
import sys
import xml.etree.ElementTree as ET

ROOT = r'G:\PyArx'
BIN = os.path.join(ROOT, 'X64', 'Release')
CATALOG = os.path.join(ROOT, 'UIDesign', 'control_catalog.xml')

sys.path.insert(0, BIN)
os.add_dll_directory(BIN)
import PyMfcXaml as M


def main():
    tree = ET.parse(CATALOG)
    tags = [o.get('tag') for o in tree.getroot().findall('objectinfo')]
    schema = [c['tag'] for c in json.loads(M.prop_schema())['controls']]

    ok = True
    if len(tags) != len(set(tags)):
        print('FAIL  目录里有重复 tag')
        ok = False
    missing = [t for t in schema if t not in tags]
    if missing:
        print(f'FAIL  schema 里有、目录没有：{", ".join(missing)}')
        ok = False
    extra = [t for t in tags if t not in schema]
    if extra:
        print(f'note  目录多出（schema 外，别名或历史条目）：{", ".join(extra)}')

    print(f'目录 {len(tags)} 条，schema {len(schema)} 条；{"PASS" if ok else "FAIL"}')
    return 0 if ok else 1


if __name__ == '__main__':
    sys.exit(main())
