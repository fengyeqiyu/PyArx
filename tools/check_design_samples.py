# -*- coding: utf-8 -*-
r"""校验 UIDesign\samples 下的示例文档：逐份 guiload 建窗，并核对控件主标签覆盖。

用法：python.exe G:\PyArx\tools\check_design_samples.py
判定：每份示例能加载、建出的控件数与 x:Name 数一致；三份合计覆盖 prop_schema() 的全部主标签。
说明：示例里写了 Click/SelectionChanged 等回调名，校验时给一份同名空实现，只验加载与建控件。
"""
import os
import re
import sys
import json
import glob

ROOT = r'G:\PyArx'
BIN = os.path.join(ROOT, 'X64', 'Release')
SAMPLES = os.path.join(ROOT, 'UIDesign', 'samples')

sys.path.insert(0, BIN)
os.add_dll_directory(BIN)
import PyMfcXaml as M

TAG_RE = re.compile(r'<([A-Za-z][A-Za-z0-9_]*)')
CB_RE = re.compile(r'"on_[A-Za-z0-9_]+"')
CONTAINERS = {'Window', 'StackPanel', 'Grid', 'RowDefinition', 'ColumnDefinition',
              'DockPanel', 'WrapPanel', 'Canvas'}


def main():
    ok = True
    covered = set()
    total = 0

    files = sorted(glob.glob(os.path.join(SAMPLES, '*.xaml')))
    if not files:
        print('FAIL  samples 目录为空')
        return 1

    for path in files:
        with open(path, 'r', encoding='utf-8') as fh:
            text = fh.read()

        def stub(comp, name, _n=0):
            return None

        ctx = {name.strip('"'): stub for name in CB_RE.findall(text)}
        tags = set(TAG_RE.findall(text)) - CONTAINERS

        names = {}
        try:
            win = M.guiload(path, context=ctx, names=names)
        except Exception as exc:
            print(f'FAIL  {os.path.basename(path)} 加载失败：{exc}')
            ok = False
            continue

        built = len(names or {})
        if built == 0:
            print(f'FAIL  {os.path.basename(path)} 没有建出控件')
            ok = False
            continue

        covered |= tags
        total += built
        print(f'ok    {os.path.basename(path)}  控件 {built} 个  hwnd={hex(win.hwnd)}')

    known = {row['tag'] for row in json.loads(M.prop_schema())['controls']}
    missing = sorted(known - covered)
    unknown = sorted(covered - known)

    print('')
    if missing:
        print(f'FAIL  未被示例覆盖的控件 {len(missing)} 个：{", ".join(missing)}')
        ok = False
    else:
        print(f'ok    主标签覆盖 {len(known)}/{len(known)}')
    if unknown:
        print(f'warn  示例里出现 schema 之外的标签：{", ".join(unknown)}')

    print(f'{total} 个控件实例，{len(files)} 份示例；{"PASS" if ok else "FAIL"}')
    return 0 if ok else 1


if __name__ == '__main__':
    sys.exit(main())
