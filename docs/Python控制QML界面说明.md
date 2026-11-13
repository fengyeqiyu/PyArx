# Python 控制 QML 界面：接口与寻址

## 结论先说

**不需要给每个控件单独封装 pybind11 接口。** Qt 的元对象系统（`QMetaObject` + `QQmlProperty`）本身就是通用通道：拿一个 `QObject*`，靠属性名和方法名就能读写一切，属性类型统一走 `QVariant`。

真正需要 C++ 手写的只有三类：
1. 你自己的 **C++ 类**要出现在 QML 里（`qmlRegisterType`，比如 `PyNode {}`）；
2. 自定义 `QAbstractItemModel`、`QQuickImageProvider` 这类"有虚函数"的基类；
3. 需要 C++ 侧性能/线程的计算。

普通的改文本、改颜色、改可见性、调 QML 函数，不用封装。

## 已经实现的接口

| Python 调用 | 作用 |
|---|---|
| `PyArxQml.set_prop(name, value)` | 写 qml **根对象**声明的属性 |
| `PyArxQml.get_prop(name)` | 读根对象属性 |
| `PyArxQml.set_named(objectName, prop, value)` | 按 `objectName` 写子控件 |
| `PyArxQml.get_named(objectName, prop)` | 按 `objectName` 读子控件 |
| `PyArxQml.tree(max_depth=99, max_nodes=500)` | **导出整棵对象树**（path/type/objectName/text/几何/可见性） |
| `PyArxQml.set_path(path, prop, value)` | 按树路径写，如 `"0/5/0/1"` |
| `PyArxQml.get_path(path, prop)` | 按树路径读 |
| `PyArxQml.call(method, *args)` | 调根对象上的 JS 函数，取返回值 |
| `PyArxQml.call_path(path, method, *args)` | 按路径调控件上的函数 |
| `PyArxQml.has_item(objectName)` | objectName 是否存在 |
| `PyArxQml.push(text)` / `query(prop)` | 便捷写/读 `backend.status`、`backend.result` |
| `PyArxQml.set_callback(fn)` | 注册 QML→Python 回调（`fn(op, a, b)`） |

`pyarx_qml.py` 里的 `QmlPanel` 已经全部代理，写起来更短：

```python
panel.set('pyResult', 6912)                    # 根属性
panel.find(type='Text', text='等待 Python')     # 按内容找控件，不用名字
panel.set_path(node['path'], 'text', '6912')    # 按路径改
panel.call('flash', '来自 Python')              # 调 QML 函数
```

## 控件寻址的三条路（"名字不好找"的解法）

### 1. 树路径（推荐，零成本）

`tree()` 直接给出每个节点的路径，路径就是"第几层第几个子元素"：

```
0            Item_QML_0
0/0          Rectangle
0/1          Text  text='PyArxQml 面板'
0/5/0        Column
0/5/0/1      Text  objectName=resultText  text='等待 Python'
0/7/0/1      Text  objectName=pyText  text='（等待 Python 写入）'
```

只要 QML 结构不变，路径就稳定。**不用在 QML 里写任何 objectName。**

### 2. 按内容反查（最省事）

QML 里的 `id` 不进元对象系统，`findChild` 找不到 —— 但 `text`、`type`、几何都在树里，可以直接按内容找：

```python
node = next(n for n in panel.tree()
            if n.get('type') == 'Text' and '等待 Python' in n.get('text', ''))
panel.set_path(node['path'], 'text', '6912')
```

实测：这样拿到了 `0/5/0/1`，改完 `get_path` 回读 `'Path 定位写入：6912'`，截图确认变色生效。

**注意**：如果同一个 Text 的内容也会被界面自己改，按内容找就不稳；这种情况下才是给关键控件加 `objectName` 的场合（一次性成本，一行 QML）。

### 3. objectName（只给关键控件写）

```qml
Text { objectName: "pyText" }
```

然后 `panel.set_named('pyText', 'text', 'x')`。只给需要长期寻址的控件写，不用全树都写。

## QML 侧要配合的两件事

### 值必须写成声明的 property

```qml
Item {
    id: root
    property string pyResult: "（等待）"
    Text { text: root.pyResult }        // 绑定 → Python 一写就刷新
}
```

Python 写 `pyResult`，绑定自动求值重绘。
**动态属性不行**——`setProperty` 一个 QML 里没声明的名字，不参与绑定，界面不会变。

### 写完要 pump 一次

界面重绘发生在事件循环里。`attach` 之后用 `pump()` 推进；脚本里 `time.sleep` 会饿死泵，一帧都不出（这是之前 3.7 秒"假慢"的真正原因）。

```python
for _ in range(N):
    PyArxQml.pump()
```

## 实测记录

| 项 | 结果 |
|---|---|
| 对象树节点数（panel_demo.qml） | 20 |
| 按文本找到 Text | `0/5/0/1`，`objectName=resultText` |
| `set_path(path,'text',...)` | True，`get_path` 回读一致 |
| `set_path(path,'color','#ffd166')` | True，截图确认变黄 |
| CAD 进程内 `set_prop`/`call` | 全部 ok，`call('flash',...)` 返回 QML 的返回值 |

截图：`G:\PyArx\docs\_pyarx_qml_tree.png`（Text 已被改值与改色）

## 还剩什么没做

- `qmlRegisterType`：让 C++ 类变成 QML 类型（这是唯一需要写 C++ 的部分，但不是"每个控件"，是"每个自定义类"）。
- 树节点回调传参上限 4 个（`invokeMethod` 的可变参限制，超了打日志忽略）。
- `tree()` 默认上限 500 节点，复杂面板需调大 `max_nodes`。

## 相关文件

| 文件 | 内容 |
|---|---|
| `G:\PyArx\PyArxQml\PyArxQmlModule.cpp` | Python 绑定层：`FromPy`/`ToPy`、`tree`/`set_path`/`call` 等 |
| `G:\PyArx\PyArxQml\QmlBackend.{h,cpp}` | 注入的 `backend` 对象（QML→Python 回调） |
| `G:\PyArx\PyArxQml\pyarx_qml.py` | `QmlPanel` 便捷封装：`set`/`get`/`find`/`set_path`/`call` |
| `G:\PyArx\docs\_qt_baseline\qml_tree_test.py` | 对象树定位验证脚本 |

---

## 附：names() / eval() / diag()（对齐 PyMfcCore.xaml 与 WebView2）

| 接口 | 作用 | 返回 |
|---|---|---|
| `PyArxQml.names(max_depth=99, max_nodes=2000)` | 遍历对象树，导出 `{objectName: 类型}`，等价于 `PyMfcCore` 的 `Document.names()` | dict（空名字的对象不导出） |
| `PyArxQml.eval(expr, obj_name=None)` | 用 `QQmlExpression` 在根对象（或指定 objectName 的对象）作用域里求值 | 求值结果；表达式报错返回 `None` 并记日志 |
| `PyArxQml.diag()` | 环境诊断：Qt 版本、平台、字体数/枚举耗时、屏幕数 | dict |
| `PyArxQml.ensure_core()` | 只建 `QCoreApplication`（二分探针用），**用完必须退出进程** | bool |

`QmlPanel` 上也有同名代理：`panel.names()` / `panel.eval(expr, obj_name=None)`。

实测（CAD 2024 进程内，13:12:57）：
- `names()` → `{'pyText': 'Text', 'resultText': 'Text', 'statusText': 'Text'}`
- `eval("pyResult")` → `'CAD 里 Python 调 QML 函数'`
- `eval("2 + 3 * 4")` → `14`（int）
- `eval('[1,2,3].map(function(v){return v*v})')` → `[1, 4, 9]`
- `eval('pyColor = "#FF8800"')` 后 `get_prop("pyColor")` 回读 `'#ff8800'`

已知限制：`eval("resultText.text", "resultText")` 返回 `None`（`ReferenceError`）——`objectName` 不在 `QQmlExpression` 的作用域链里，第二参数只用于"在该对象上运行的表达式"，不能按名字引用兄弟对象。
