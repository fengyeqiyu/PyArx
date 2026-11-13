# -*- coding: utf-8 -*-
"""侧栏按钮 py.run 的示例脚本 / 用户可直接改这个文件。

约定：
- 页面按 {cmd:'py.run', file:'G:\\PyArx\\tests\\cad\\palette_script.py', args:{...}} 触发；
  路径既可以写正斜杠，也可以写反斜杠（JSON 里双写即可，桥已换真解析器，会还原成单反斜杠）；
- 页面传的 args 会原样变成脚本里的变量 args（嵌套对象→dict、数组→list、数字→float）；
- 脚本执行完，若命名空间里有变量 result，它的字符串形式会回送到页面结果框；
- 脚本自己用 PyDbTrans() 开事务，构造时就加了文档写锁，不需要外面再锁。

下面演示：读 args、往模型空间加一条直线、把回执交给页面。
"""
import sys

import PyArxCore
from PyArxCore.Db import PyDbLine, PyDbTrans
from PyArxCore.Ge import AcGePoint3d

LOG = r'G:\PyArx\docs\_palette_py.log'


def log(msg):
    with open(LOG, 'a', encoding='utf-8') as f:
        f.write(str(msg) + '\n')


log('--- palette_script.py 开始 ---')
log('python=' + sys.version.split()[0])
log('PyArxCore=' + str(getattr(PyArxCore, '__file__', '?')))
log('args=' + repr(args))

# print 的内容会被桥接管，原样送到 CAD 命令行和页面结果框
print('palette_script.py 开始执行')
print('Python 版本：' + sys.version.split()[0])
print('args = ' + repr(args))

try:
    # 页面没传 args 就用默认值；传了就按页面给的走（含嵌套字段）
    length = 150.0
    if isinstance(args, dict):
        length = float(args.get('length', length))
    sys._palette_py_round = getattr(sys, '_palette_py_round', 0) + 1

    with PyDbTrans() as trans:
        trans.addEntity(PyDbLine(AcGePoint3d(0, 0, 0), AcGePoint3d(length, length * 0.6, 0)))
        trans.commit()

    tag = ''
    if isinstance(args, dict):
        tag = '，args=' + repr(args)
    result = (f'Python 脚本已在 CAD 里执行：新建直线 (0,0)→({int(length)},{int(length * 0.6)})，'
              f'第 {sys._palette_py_round} 次{tag}')
    log(result)
except Exception as exc:  # noqa: BLE001 - 例子里把异常也回送页面
    result = f'脚本出错：{type(exc).__name__}: {exc}'
    log(result)
