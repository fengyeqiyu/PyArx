# -*- coding: utf-8 -*-
"""把 XML 标签的本意名对齐 QML 命名（X 前缀保持不变）。

映射（只改 QML 侧确实有同名控件、且语义一致的）：
    TextBox        -> TextField        （QML TextField）
    TextBlock      -> Label            （QML Label）
    RichText       -> TextArea         （QML TextArea）
    StaticBitmap   -> Image            （QML Image）
    TreeCtrl       -> TreeView         （QML TreeView）
    ScrolledWindow -> ScrollView       （QML ScrollView）
    Splitter       -> SplitView        （QML SplitView）
    ToolStrip      -> ToolBar          （QML ToolBar）
    MenuStrip      -> MenuBar          （QML MenuBar）

保留不动的（QML 无同名类型，硬套会造出假名）：
    ListBox / ListCtrl / Choice / TreeTable / CalendarCtrl / Hyperlink /
    CodeEditor / StaticLine / DatePicker / BitmapButton / VirtualList /
    TabControl / Panel / DwgCanvas / QCanvas / Arc

用法：
    python tools\\migrate_tags_qml_names.py --dry    只统计不写
    python tools\\migrate_tags_qml_names.py          执行替换
"""
import io
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

MAP = [
    ('TextField', 'TextField'),
    ('Label', 'Label'),
    ('TextArea', 'TextArea'),
    ('Image', 'Image'),
    ('TreeView', 'TreeView'),
    ('ScrollView', 'ScrollView'),
    ('SplitView', 'SplitView'),
    ('ToolBar', 'ToolBar'),
    ('MenuBar', 'MenuBar'),
]

EXTS = ('.xml', '.xml', '.h', '.cpp', '.py', '.md', '.ps1', '.json')
SKIP_DIRS = {'.git', 'X64', 'third_party', 'build', 'log', '__pycache__'}


def read_text(path):
    raw = io.open(path, 'rb').read()
    bom = raw.startswith(b'\xef\xbb\xbf')
    text = raw.decode('utf-8-sig')
    return text, bom


def write_text(path, text, bom):
    data = text.encode('utf-8')
    if bom:
        data = b'\xef\xbb\xbf' + data
    io.open(path, 'wb').write(data)


def migrate(text):
    """X 形式全局替换；裸名只替换"整串字面量"，避免误伤普通词汇。"""
    for old, new in MAP:
        text = re.sub(r'\bX' + old + r'\b', 'X' + new, text)
        for q in ('"', "'", '`'):
            text = text.replace(q + old + q, q + new + q)
            text = text.replace('L' + q + old + q, 'L' + q + new + q)
    return text


def main():
    dry = '--dry' in sys.argv
    changed = []
    total = 0
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
    print('---- files=%d  tag_hits=%d  %s' % (len(changed), total,
                                              'DRY' if dry else 'WRITTEN'))


if __name__ == '__main__':
    main()
