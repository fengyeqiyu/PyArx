# -*- coding: utf-8 -*-
"""style 外观通路：CSS 的 background / color / border-radius 必须真正进到 owner-draw。

此前的真缺口：控件级 setControlAppearance 从未被调用，findAppearance 恒空，
于是 <button style="background:#2563eb;color:#fff;border-radius:8px"> 这些声明
全部被丢掉 —— HTML 一行就能定制的按钮，在 XML 里写了等于没写。

判据来自宿主自己的调试日志（PYARX_MFC_DEBUG=1）：
  [ap] id=<id> bg='#2563eb' fg='#ffffff' empty=0     → 外观真的推给了宿主
  [drawitem] id=<id> styled=1                        → 绘制端认到"有样式"，不再走主题默认
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
    return bool(cond)


def log_mark():
    try:
        return os.path.getsize(LOG)
    except OSError:
        return 0


def log_since(offset):
    try:
        with open(LOG, 'rb') as f:
            f.seek(offset)
            return f.read().decode('utf-8', 'replace')
    except OSError:
        return ''


def main():
    xml = """<XWindow Name="root" Width="360" Height="140">
  <button Name="b1" Text="Blue" style="background:#2563eb;color:#ffffff;border-radius:8px"/>
  <button Name="b2" Text="Plain"/>
</XWindow>"""
    mark = log_mark()
    names = {}
    win = M.guiload_text(xml, context=globals(), names=names)
    b1 = names['b1']
    b2 = names['b2']
    check(set(names) == {'b1', 'b2'}, '两个按钮都建出来了', list(names))
    win.pump(200) if hasattr(win, 'pump') else None
    text = log_since(mark)
    try:
        win.close()
    except Exception:
        pass

    check("[ap] id=%d bg='#2563eb'" % b1 in text, 'background 推到宿主（[ap] bg 非空）')
    check("[ap] id=%d bg='#2563eb' fg='#ffffff'" % b1 in text,
          'CSS color 走 Foreground 而不是 FrontColor（fg 非空）')
    check('[drawitem] id=%d styled=1' % b1 in text, '绘制端认到 styled=1（自己的外观）')
    # 第35轮起 HTML 原生 <button> 默认带 Radius=6（浏览器观感），所以 b2 不再是空外观；
    # 判据改成"有默认外观、但没有写 style 的属性"。
    check('[drawitem] id=%d styled=1' % b2 in text,
          '没写 style 的按钮仍走 HTML 默认外观（默认圆角）')
    check("[ap] id=%d bg=''" % b2 in text or ("[ap] id=%d bg=\"\"" % b2) in text,
          '默认按钮没有显式底色（仍用主题底）')
    check('Radius=8px' in text or 'Radius=8' in text, 'border-radius 已下发（Radius）')

    print('\nt_style_appearance: PASS=%d FAIL=%d' % (CHECKS[0] - len(FAILS), len(FAILS)))
    if FAILS:
        for f in FAILS:
            print('  - ' + f)
        print('--- log slice ---')
        print(text[-1500:])
    return 1 if FAILS else 0


if __name__ == '__main__':
    sys.exit(main())
