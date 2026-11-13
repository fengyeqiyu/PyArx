import io, os
d = r'G:\PyArx\PyMfcXaml'
parts = os.path.join(d, 'parts')

host_files = ['02a-host-core.inc','02b-host-widgets.inc','02c-host-fill.inc','02d-host-export.inc',
              '02e-host-canvas.inc','02f-host-anim.inc','02g-host-props.inc']
body = []
for f in host_files:
    t = io.open(os.path.join(parts, f), encoding='utf-8').read().split('\n')
    body.append('\n'.join(t[1:]))   # 去掉分片标题行
host = '\n'.join(body)

hdr = ['// PyMfcHost.h —— 宿主类 PyMfcHost 的完整定义（控件库 / 皮肤 / 事件 / XAML 桥）。',
       '// 由 02a..02g 七个域分片合并而成；改代码请改 parts\\02x-*.inc 后重新生成本文件，',
       '// 或在 parts 里继续细分。声明与实现分离（.h/.cpp）是下一步。',
       '#pragma once',
       '',
       '#include "PyMfcCommon.h"',
       ''] + host.split('\n')
io.open(os.path.join(d, 'PyMfcHost.h'), 'w', encoding='utf-8', newline='').write('\n'.join(hdr))
print('PyMfcHost.h 生成，%d 行' % len(hdr))

# 窗口过程独立编译单元
procs = []
for f in ('03a-procs-skin.inc', '03b-procs-canvas.inc'):
    t = io.open(os.path.join(parts, f), encoding='utf-8').read().split('\n')
    procs.append('\n'.join(t[1:]))
txt = ['// PyMfcProcs.cpp —— 窗口过程：全控件自绘/皮肤绘制、配电画布鼠标状态机、可编辑表格 Grid。',
       '// 独立编译单元：依赖 PyMfcHost.h（类内静态过程与宿主方法均已在头里）。',
       '',
       '#include "PyMfcHost.h"',
       ''] + procs
io.open(os.path.join(d, 'PyMfcProcs.cpp'), 'w', encoding='utf-8-sig', newline='').write('\n'.join(txt))
print('PyMfcProcs.cpp 生成')
