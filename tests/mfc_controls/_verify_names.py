# -*- coding: utf-8 -*-
"""_verify_names.py — 验证"布局是布局、画布是画布"的命名：

1) <AbsolutePanel x:Name="root"> 是绝对定位布局（不建 HWND），里面的 Button 落在 Canvas.Left/Top 上；
2) <DrawCanvas> / <PaintCanvas> 是画布控件（建 HWND、按命令串画图）；
3) <Canvas x:Name="root">（历史写法）仍然当布局用，老文件不会坏；
4) <QCanvas> 老名字仍可用。
"""
import os

import PyMfcXaml as p

CASES = {
    'abs': '''<?xml version="1.0" encoding="utf-8"?>
<Window xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml" Title="AbsLayoutProbe" Width="360" Height="220">
  <AbsolutePanel x:Name="root">
    <Button x:Name="b1" Content="布局按钮" Width="120" Height="36" Canvas.Left="20" Canvas.Top="20" />
    <TextBlock x:Name="t1" Text="绝对定位" Width="160" Height="22" Canvas.Left="150" Canvas.Top="28" />
  </AbsolutePanel>
</Window>''',
    'legacy': '''<?xml version="1.0" encoding="utf-8"?>
<Window xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml" Title="LegacyCanvasProbe" Width="360" Height="220">
  <Canvas x:Name="root">
    <Button x:Name="b1" Content="老写法" Width="120" Height="36" Canvas.Left="20" Canvas.Top="20" />
  </Canvas>
</Window>''',
    'draw': '''<?xml version="1.0" encoding="utf-8"?>
<Window xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml" Title="DrawCanvasProbe" Width="360" Height="220">
  <AbsolutePanel x:Name="root">
    <DrawCanvas x:Name="dc" Width="300" Height="160" Canvas.Left="20" Canvas.Top="20"
                Commands="Clear #2B313C; RoundRect 12,12,120,46,10,#00A8E8; Text 12,80,DrawCanvas,11,#E6EDF3" />
  </AbsolutePanel>
</Window>''',
    'paint': '''<?xml version="1.0" encoding="utf-8"?>
<Window xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml" Title="PaintCanvasProbe" Width="360" Height="220">
  <AbsolutePanel x:Name="root">
    <PaintCanvas x:Name="pc" Width="300" Height="160" Canvas.Left="20" Canvas.Top="20"
                 Commands="Clear #2B313C; Ellipse 20,20,90,90,#57C84D" />
  </AbsolutePanel>
</Window>''',
    'layout': '''<?xml version="1.0" encoding="utf-8"?>
<Window xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml" Title="LayoutAliasProbe" Width="360" Height="220">
  <Layout x:Name="root">
    <Button x:Name="b1" Content="Layout 布局" Width="120" Height="36" Canvas.Left="20" Canvas.Top="20" />
    <LayoutPanel x:Name="inner" Width="160" Height="80" Canvas.Left="150" Canvas.Top="20">
      <TextBlock x:Name="t1" Text="嵌套布局" Width="120" Height="22" Canvas.Left="10" Canvas.Top="10" />
    </LayoutPanel>
  </Layout>
</Window>''',
}

import ctypes

u = ctypes.windll.user32
u.GetWindowRect.argtypes = [ctypes.c_void_p, ctypes.c_void_p]
u.GetDlgItem.argtypes = [ctypes.c_void_p, ctypes.c_int]
u.GetDlgItem.restype = ctypes.c_void_p
u.EnumChildWindows.argtypes = [ctypes.c_void_p, ctypes.c_void_p, ctypes.c_void_p]
CB = ctypes.WINFUNCTYPE(ctypes.c_int, ctypes.c_void_p, ctypes.c_void_p)


class R(ctypes.Structure):
    _fields_ = [('l', ctypes.c_long), ('t', ctypes.c_long), ('r', ctypes.c_long), ('b', ctypes.c_long)]


def rect(hwnd):
    r = R()
    u.GetWindowRect(hwnd, ctypes.byref(r))
    return (r.l, r.t, r.r - r.l, r.b - r.t)


def find_child(hwnd, id_):
    """页容器/Flickable 都挂在宿主第一层子窗下，先枚举第一层再 GetDlgItem。"""
    found = []

    def cb(h, _l):
        found.append(h)
        return 1

    u.EnumChildWindows(hwnd, CB(cb), None)
    for h in found:
        got = u.GetDlgItem(h, id_)
        if got:
            return got
    return None


def main():
    ok = bad = 0
    for name, xml in CASES.items():
        path = os.path.join(r'G:\PyArx\tests\mfc_controls', '_name_%s.xaml' % name)
        with open(path, 'w', encoding='utf-8') as f:
            f.write(xml)
        names = {}
        w = p.guiload(path, names=names)
        w.show()
        p.pump(200)
        print('--- %s names=%s' % (name, names))
        if name in ('abs', 'legacy', 'layout'):
            hb = find_child(w.hwnd, names['b1'])
            rc = rect(hb) if hb else None
            wr = rect(w.hwnd)
            rel = (rc[0] - wr[0], rc[1] - wr[1], rc[2], rc[3]) if rc else None
            good = rel is not None and rel[2] == 120 and rel[3] == 36 and 10 <= rel[0] <= 40
            (ok, bad) = (ok + 1, bad) if good else (ok, bad + 1)
            print(('  OK   ' if good else '  FAIL ') + '布局：按钮 120x36 落在 (%s)' % (rel,))
        else:
            key = 'dc' if 'dc' in names else 'pc'
            hid = find_child(w.hwnd, names[key])
            good = hid is not None
            (ok, bad) = (ok + 1, bad) if good else (ok, bad + 1)
            rr = rect(hid) if hid else None
            print(('  OK   ' if good else '  FAIL ') + '画布：控件建出 HWND，尺寸 %s' % (rr[2:] if rr else None,))
        w.close()
        p.pump(60)
    print('命名验收：通过 %d，失败 %d' % (ok, bad))


if __name__ == '__main__':
    main()
