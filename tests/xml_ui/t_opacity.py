# -*- coding: utf-8 -*-
"""CSS opacity 语义：opacity:0.5 必须变成"半透明"(128)，不是 0（全透明）。

真 bug：style="opacity:0.5" 经 _wtoi 读成 0，控件底色被完全抹掉（看不见）。
HTML 里 opacity 是 0~1 的浮点，XML 老写法是 0~255 整数，两套语义必须都认。
判据：宿主外观日志（PYARX_MFC_DEBUG=1）里的 [ap] ... op=<0~255>。
"""
import os
import sys

ROOT = os.path.abspath(os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..'))
BIN = os.environ.get('PYARX_BIN') or os.path.join(ROOT, 'X64', 'Release')
sys.path.insert(0, BIN)
os.add_dll_directory(BIN)
os.environ.setdefault('PYARX_D2D_RT', 'software')
os.environ['PYARX_MFC_DEBUG'] = '1'

import PyMfcXml as M  # noqa: E402

FAILS = []
CHECKS = [0]
LOG = os.path.join(BIN, 'log', 'pymfc_shutdown.log')


def check(cond, what, got=None):
    CHECKS[0] += 1
    print(('PASS ' if cond else 'FAIL ') + what + ('' if got is None else '  got=%r' % (got,)))
    if not cond:
        FAILS.append(what)


def log_size():
    try:
        return os.path.getsize(LOG)
    except OSError:
        return 0


def log_since(off):
    try:
        with open(LOG, 'rb') as f:
            f.seek(off)
            return f.read().decode('utf-8', 'replace')
    except OSError:
        return ''


def main():
    xml = """<XWindow Name="root" Width="360" Height="200">
  <button Name="full" Text="F" style="background:#ff0000;color:#ff0000"/>
  <button Name="half" Text="H" style="background:#ff0000;color:#ff0000;opacity:0.5"/>
  <button Name="legacy" Text="L" style="background:#ff0000;opacity:128"/>
</XWindow>"""
    mark = log_size()
    names = {}
    win = M.guiload_text(xml, names=names)
    M.pump(200)
    try:
        win.close()
    except Exception:
        pass
    text = log_since(mark)
    full, half, legacy = names['full'], names['half'], names['legacy']

    check("[ap] id=%d" % half in text, 'opacity:0.5 的按钮外观已下发')
    check("id=%d bg='#ff0000'" % half in text, 'background 与 opacity 同一条 style 都能解析')
    check("op=128" in text, 'CSS 的 0.5 换算成 128（不是被 _wtoi 读成 0）')
    check("id=%d bg='#ff0000' fg='' op=0" % legacy not in text.split("id=%d" % legacy)[-1][:60],
          '老写法 opacity:128 仍是 128（两种语义并存）')

    print('\nt_opacity: PASS=%d FAIL=%d' % (CHECKS[0] - len(FAILS), len(FAILS)))
    if FAILS:
        for f in FAILS:
            print('  - ' + f)
        print('--- log slice ---')
        for ln in text.splitlines():
            if '[ap]' in ln:
                print(ln)
    return 1 if FAILS else 0


if __name__ == '__main__':
    sys.exit(main())
