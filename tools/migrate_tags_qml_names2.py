# -*- coding: utf-8 -*-
"""第二批：把剩余"QML 有对应类型、且改名后不撞车"的标签对齐 QML 命名。

    GridCtrl      -> GridLayout   （QML GridLayout）
    Panel         -> Pane         （QML Pane，Qt Quick Controls 的容器）
    StaticLine    -> Divider      （QML Divider / HorizontalDivider）
    TabControl    -> TabBar       （QML TabBar）
    BitmapButton  -> IconButton   （QML 生态惯用的 IconButton）
    CalendarCtrl  -> MonthGrid    （QML MonthGrid）

仍然保留（会与既有标签撞名，或 QML 没有该类型）：
    ListBox / ListCtrl  -> 都落 ListView，撞名且语义不同（单列列表 vs 多列列表）
    Choice              -> 落 ComboBox，与 XComboBox 撞名
    CodeEditor          -> 落 TextArea，与 XTextArea 撞名
    TreeTable           -> 落 TreeView，与 XTreeView 撞名
    VirtualList         -> 落 ListView，撞名
    DatePicker / Hyperlink / Arc / PowerCanvas / QCanvas
                        -> QML 无对应类型；QCanvas 还要避开设计器画布宿主 XCanvas

用法：python tools\\migrate_tags_qml_names2.py  [--dry]
"""
import io
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

MAP = [
    ('GridLayout', 'GridLayout'),
    ('Pane', 'Pane'),
    ('Divider', 'Divider'),
    ('TabBar', 'TabBar'),
    ('IconButton', 'IconButton'),
    ('MonthGrid', 'MonthGrid'),
]

EXTS = ('.xaml', '.xml', '.h', '.cpp', '.py', '.md', '.ps1', '.json')
SKIP_DIRS = {'.git', 'X64', 'third_party', 'build', 'log', '__pycache__'}


def read_text(path):
    raw = io.open(path, 'rb').read()
    return raw.decode('utf-8-sig'), raw.startswith(b'\xef\xbb\xbf')


def write_text(path, text, bom):
    data = text.encode('utf-8')
    if bom:
        data = b'\xef\xbb\xbf' + data
    io.open(path, 'wb').write(data)


def migrate(text):
    for old, new in MAP:
        text = re.sub(r'\bX' + old + r'\b', 'X' + new, text)
        for q in ('"', "'", '`'):
            text = text.replace('L' + q + old + q, 'L' + q + new + q)
            text = text.replace(q + old + q, q + new + q)
    return text


def main():
    dry = '--dry' in sys.argv
    changed, total = [], 0
    for dp, dn, fn in os.walk(ROOT):
        dn[:] = [d for d in dn if d not in SKIP_DIRS]
        for f in fn:
            if os.path.splitext(f)[1].lower() not in EXTS:
                continue
            p = os.path.join(dp, f)
            try:
                text, bom = read_text(p)
            except Exception:
                continue
            new = migrate(text)
            if new != text:
                n = sum(1 for o, _ in MAP for _ in re.finditer(r'\bX' + o + r'\b', text))
                total += n
                changed.append((os.path.relpath(p, ROOT), n))
                if not dry:
                    write_text(p, new, bom)
    changed.sort(key=lambda kv: -kv[1])
    for rel, n in changed:
        print('%4d  %s' % (n, rel))
    print('---- files=%d  tag_hits=%d  %s' % (len(changed), total, 'DRY' if dry else 'WRITTEN'))


if __name__ == '__main__':
    main()
