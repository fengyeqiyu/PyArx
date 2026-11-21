# -*- coding: utf-8 -*-
"""窗口缩放测试：主窗体尺寸变化后，画布/侧栏/顶栏/状态栏是否跟着重排。
用户最在意的回归点：画布必须随窗口一起缩放。"""
import os, sys

BIN = os.environ.get('PYARX_BIN') or r'G:\PyArx\X64\Release'
sys.path.insert(0, BIN)
os.add_dll_directory(BIN)
os.environ.setdefault('PYARX_D2D_RT', 'software')
import PyMfcXml as M

SHELL = """<XWindow Title="resize probe" Width="1280" Height="866" Theme="dark" Background="#FF14181B">
  <XStackPanel Name="page" Display="flex" Flex="column" Gap="0" Width="100%" Height="100%">
    <XStackPanel Name="top" Display="flex" Flex="row" Align="center" Gap="10" Height="40"
                 Padding="12 0" Background="#FF22262A">
      <XLabel Name="lbTitle" Text="title" Width="360" Height="24"/>
      <XLabel Name="lbInfo" Text="info" Grow="1" Height="24"/>
    </XStackPanel>
    <XStackPanel Name="mid" Display="flex" Flex="row" Gap="0" Grow="1">
      <XStackPanel Name="side" Display="flex" Flex="column" Gap="6" Width="232" Padding="8"
                   Background="#FF1A1E22">
        <XTableView Name="tblLayer" Columns="a&#9;b&#9;c" Rows="1&#9;2&#9;3" Grow="1"/>
      </XStackPanel>
      <XDwgCanvas Name="cvMain" Grow="1" Background="#FF14181B"/>
    </XStackPanel>
    <XStackPanel Name="sta" Display="flex" Flex="row" Align="center" Height="26" Padding="12 0"
                 Background="#FF22262A">
      <XLabel Name="stView" Text="status" Width="470" Height="20"/>
    </XStackPanel>
  </XStackPanel>
</XWindow>"""

FAILS = []
COUNT = [0]


def check(ok, label, got=None):
    COUNT[0] += 1
    if ok:
        print('PASS', label)
    else:
        FAILS.append(label)
        print('FAIL', label, '' if got is None else 'got=%r' % (got,))


def dump(win, names, tag):
    cw, ch = win.client_size()
    print('   [%s] client=%r' % (tag, (cw, ch)))
    for k in sorted(names):
        print('   %-10s %r' % (k, tuple(win.get_rect(names[k]))))
    return cw, ch


def main():
    import ctypes
    names = {}
    win = M.guiload_text(SHELL, context={}, names=names)
    M.pump(200)
    cw, ch = dump(win, names, 'init')
    cv = win.get_rect(names['cvMain'])
    tbl = win.get_rect(names['tblLayer'])
    lb = win.get_rect(names['lbTitle'])
    stv = win.get_rect(names['stView'])
    check(cv[2] == cw - 232, '初始画布宽 = 客户区宽-侧栏', (cv, cw))
    check(cv[3] == ch - 40 - 26, '初始画布高 = 客户区高-顶栏-状态栏', (cv[3], ch))
    check(lb[1] == 8, '顶栏文字垂直居中（y=8）', lb[1])
    check(stv[1] == ch - 26 + 3, '状态栏文字贴底', stv[1])

    # 真实窗口缩放（客户区 +320 × +34）
    hwnd = win.hwnd
    ctypes.windll.user32.SetWindowPos(hwnd, 0, 0, 0, cw + 320, ch + 34, 0x0004 | 0x0002)
    M.pump(300)
    cw2, ch2 = dump(win, names, 'big')
    cv2 = win.get_rect(names['cvMain'])
    tbl2 = win.get_rect(names['tblLayer'])
    stv2 = win.get_rect(names['stView'])
    check(cw2 > cw, '窗口确实变宽了', (cw, cw2))
    check(cv2[2] == cw2 - 232, '放大后画布宽跟着变', cv2)
    check(cv2[3] == ch2 - 40 - 26, '放大后画布高跟着变', (cv2[3], ch2))
    check(tbl2[3] == ch2 - 40 - 26 - 16, '放大后侧栏表格高 = 中间区-上下内边距', (tbl2[3], ch2))
    check(tbl2[3] == 745 + (ch2 - ch), '放大后侧栏表格高按客户区增量变化', (tbl[3], tbl2[3], ch2 - ch))
    check(stv2[1] == ch2 - 26 + 3, '放大后状态栏仍贴底', stv2[1])

    # 缩小回来
    ctypes.windll.user32.SetWindowPos(hwnd, 0, 0, 0, cw - 180, ch - 160, 0x0004 | 0x0002)
    M.pump(300)
    cw3, ch3 = win.client_size()
    cv3 = win.get_rect(names['cvMain'])
    check(cv3[2] == cw3 - 232, '缩小后画布宽跟着变', (cv3, cw3))
    check(cv3[3] == ch3 - 40 - 26, '缩小后画布高跟着变', (cv3[3], ch3))

    try:
        win.save_png(r'G:\PyArx\log\after\resize_probe.png')
    except Exception:
        pass
    win.close()

    print('\nt_resize: PASS=%d FAIL=%d' % (COUNT[0] - len(FAILS), len(FAILS)))
    if FAILS:
        for f in FAILS:
            print('  -', f)
        return 1
    return 0


if __name__ == '__main__':
    sys.exit(main())
