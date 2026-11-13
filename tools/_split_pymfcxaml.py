import io, os
P = r'G:\PyArx\PyMfcXaml\PyMfcXaml.cpp'
lines = io.open(P, encoding='utf-8-sig').read().split('\n')
outdir = r'G:\PyArx\PyMfcXaml\parts'
os.makedirs(outdir, exist_ok=True)

parts = [
    ('01-pre.inc',     1,   443,  '公共头/宏/辅助函数/前向声明'),
    ('02-host.inc',    444, 6262, 'class PyMfcHost（宿主：控件库/皮肤/事件/XAML 桥）'),
    ('03-procs.inc',   6263, 8587, '窗口过程与离屏渲染等自由函数（skin / wheel / grid / D2D）'),
    ('04-wrap.inc',    8588, 9503, 'PyMfcTopWnd / PyMfcWindow / PyMfcPanel / PyMfcStore'),
    ('05-load.inc',    9505, 11089, 'guiload：XML → 控件树（XAML 布局运行时）'),
    ('06-bind.inc',    11091, len(lines), 'PYBIND11_MODULE：全部 .def 绑定'),
]

for name, a, b, desc in parts:
    body = '\n'.join(lines[a - 1:b])
    path = os.path.join(outdir, name)
    io.open(path, 'w', encoding='utf-8', newline='').write(
        '// %s  （从 PyMfcXaml.cpp 按类/职责切出，行 %d-%d）\n%s\n' % (desc, a, b, body))
    print('%-14s %5d 行  %s' % (name, b - a + 1, desc))

head = '''// PyMfcXaml.cpp —— 只做"装配"：按类/职责分片后由这里聚合编译（单编译单元）。
// 分片都在 parts\\ 下，改代码请直接改对应分片，不要再往本文件塞实现。
//
//   parts\\01-pre.inc      公共头/宏/辅助函数/前向声明
//   parts\\02-host.inc     class PyMfcHost（宿主：控件库/皮肤/事件/XAML 桥）
//   parts\\03-procs.inc    窗口过程与离屏渲染等自由函数
//   parts\\04-wrap.inc     PyMfcTopWnd / PyMfcWindow / PyMfcPanel / PyMfcStore
//   parts\\05-load.inc     guiload（XML → 控件树）
//   parts\\06-bind.inc     PYBIND11_MODULE 绑定区
//
// 下一步：把这些分片提升为真正的 .h/.cpp（PyMfcHost 声明与实现分离后，
// 各窗口过程即可独立成 .cpp 并只依赖 PyMfcHost.h）。

#include "parts\\01-pre.inc"
#include "parts\\02-host.inc"
#include "parts\\03-procs.inc"
#include "parts\\04-wrap.inc"
#include "parts\\05-load.inc"
#include "parts\\06-bind.inc"
'''
io.open(P, 'w', encoding='utf-8-sig', newline='').write(head)
print('主文件已改为装配文件')
