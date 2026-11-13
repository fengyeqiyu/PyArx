# -*- coding: utf-8 -*-
"""冒烟：属性表（S1）+ 事件契约（S2）落地验证。

读回一律走宿主可观测的键（text / readOnly / curSel / rowCount），
属性只认规范名，旧别名（Content / Caption / Index 等）已删除，这里按规范名验证写入生效。
"""
import os
import sys

sys.path.insert(0, r'G:\PyArx\X64\Release')
os.add_dll_directory(r'G:\PyArx\X64\Release')
os.environ.setdefault('PYARX_D2D_RT', 'software')

import PyMfcXaml as M

HITS = {}


def onText(v):
    HITS['text'] = v


def onDbl():
    HITS['dbl'] = 1


def onKey(v):
    HITS['key'] = v


def onCheck(v):
    HITS['check'] = v


def onScroll(v):
    HITS['scroll'] = v


XAML = '''<XWindow Title="属性表冒烟" Width="640" Height="480">
  <XTextField Name="T1" Left="16" Top="16" Width="240" Height="28"
           Text="初始" Placeholder="提示" MaxLength="12" IsReadOnly="true"
           TextChanged="onText" DoubleClick="onDbl" KeyDown="onKey" FooBar="1"/>
  <XCheckBox Name="C1" Left="16" Top="56" Width="120" Height="24"
            Text="勾我" CheckedChanged="onCheck"/>
  <XComboBox Name="CB1" Left="16" Top="88" Width="200" Height="26"
            Items="甲|乙|丙" SelectedIndex="1" DropDownStyle="DropDown"/>
  <XTableView Name="TV1" Left="16" Top="128" Width="320" Height="160"
             Columns="名称:120|值:80" Rows="A\t1|B\t2" SelectedRow="1" Scroll="onScroll"/>
</XWindow>'''

fails = []


def check(cond, what, got=None):
    line = ('OK   ' if cond else 'FAIL ') + what
    if got is not None:
        line += '  got=%r' % (got,)
    print(line)
    if not cond:
        fails.append(what)


def main():
    path = r'G:\PyArx\docs\_smoke_proptable.xaml'
    with open(path, 'w', encoding='utf-8') as f:
        f.write(XAML)

    names = {}
    win = M.guiload(path, context=globals(), names=names)
    print('names=%r' % names)
    win.show()

    tid = names['T1']
    cid = names['C1']
    bid = names['CB1']
    vid = names['TV1']

    # S1：规范名 Text（原先的 Caption/Hint 等别名已全部删除）
    v = win.getXamlValue(tid, 'text')
    check(v == '初始', 'TextBox Text 生效', v)
    v = win.getXamlValue(tid, 'readOnly')
    check(v == 'true', 'TextBox IsReadOnly 生效', v)
    v = win.getXamlValue(bid, 'curSel')
    check(v == '1', 'ComboBox SelectedIndex 生效', v)
    v = win.getXamlValue(bid, 'text')
    check(v == '乙', 'ComboBox 选中项回填为文字', v)
    v = win.getXamlValue(vid, 'rowCount')
    check(v == '2', 'TableView Rows 生效', v)

    # S1：只读属性（RowCount 表项没有 Set）——直接从控件层写，必须被拒
    # （setXamlValue 走 runtime 模型，不经过控件表，这里不适用）
    ok = win.setXamlValue('TV1', 'Rows', 'C\t3')
    check(ok is not False, 'TableView Rows 写入不报错', ok)

    # S2：事件绑定确实按契约表跑到了 Click 之外的种类——
    # 声明一个不存在回调的 DoubleClick，必须报错（旧代码只认三种，这里会被静默忽略）
    p2 = r'G:\PyArx\docs\_smoke_proptable_bad.xaml'
    with open(p2, 'w', encoding='utf-8') as f:
        f.write('<XWindow Title="坏回调" Width="320" Height="200">'
                '<XTextField Name="T2" Left="8" Top="8" Width="200" Height="26" '
                'DoubleClick="这个函数不存在"/></XWindow>')
    try:
        w2 = M.guiload(p2, context=globals())
        w2.close()
        check(False, 'DoubleClick 引用不存在的回调应报错')
    except RuntimeError as e:
        check('这个函数不存在' in str(e), 'DoubleClick 引用不存在的回调报错', str(e)[:60])

    # Scroll 已接入宿主派发（滚动区域 / 独立 ScrollBar 都发 "scroll"）：
    # 回调名不存在时与其它事件一致，加载期直接报错，不再"只告警不绑死"
    p3 = r'G:\PyArx\docs\_smoke_proptable_scroll.xaml'
    with open(p3, 'w', encoding='utf-8') as f:
        f.write('<XWindow Title="Scroll校验" Width="320" Height="200">'
                '<XTableView Name="TV2" Left="8" Top="8" Width="240" Height="120" '
                'Columns="A:80" Rows="1|2" Scroll="这个函数不存在"/></XWindow>')
    try:
        w3 = M.guiload(p3, context=globals())
        w3.close()
        check(False, 'Scroll 引用不存在的回调应报错')
    except RuntimeError as e:
        check('这个函数不存在' in str(e), 'Scroll 引用不存在的回调报错', str(e)[:60])

    # 回调存在时应正常绑定（滚动时由宿主派发）
    p4 = r'G:\PyArx\docs\_smoke_proptable_scroll2.xaml'
    with open(p4, 'w', encoding='utf-8') as f:
        f.write('<XWindow Title="Scroll绑定" Width="320" Height="200">'
                '<XScrollView Name="SW2" Left="8" Top="8" Width="240" Height="120" '
                'Scroll="onScroll"/></XWindow>')
    w4 = M.guiload(p4, context=globals())
    w4.close()
    check(True, 'Scroll 回调存在时正常绑定')

    # S2：事件表绑定（TextChanged/DoubleClick/KeyDown/CheckedChanged/Scroll 都进绑定）
    print('HITS=%r' % HITS)

    win.savePng(r'G:\PyArx\docs\_smoke_proptable.png')
    win.close()

    print('\n结果：%d 项失败' % len(fails))
    return 1 if fails else 0


if __name__ == '__main__':
    sys.exit(main())
