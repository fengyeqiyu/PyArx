
AutoCAD 二次开发的老问题是：业务逻辑用 C++/ObjectArx 写，界面要么堆 MFC 对话框资源，要么用一堆 `CreateWindow` 手搓，改一个控件位置就要重编译。PyArx 试着把这件事拆开——**逻辑交给 Python，界面交给声明式 XML，C++ 只负责底层的绑定与绘制**。

- GitHub：https://github.com/fengyeqiyu/PyArx
- Gitee（国内镜像）：https://gitee.com/fengyeqiyu/PyArx

## 一、项目做什么

用一句话说：PyArx 把 ObjectArx 的类库和一套 MFC 自绘控件库，用 pybind11 暴露成 Python 模块，界面用 XML 标签声明，事件直接回调到 Python 函数。

具体三块能力：

1. **Python 调 CAD**：pybind11 把 ObjectArx 2024 的数据库、实体、事务、选择集等类库暴露成 Python 模块，写业务逻辑不用碰 C++。
2. **XML 写界面**：界面用声明式标签描述（`<XButton>`、`<XGrid>`、`<XStackPanel>` 等），自带 50+ 自绘控件，事件用 `on_click="fn"` 绑到 Python 函数上。
3. **可视化设计器**：`UIDesign.exe` 提供控件箱、设计画布、对象检查器、属性编辑器和 XML 源码同步，拖控件改属性即可，改完直接跑。

界面长这样：

![PyArx 可视化设计器](https://aka.doubaocdn.com/s/mISGVjCdb3)

## 二、上手是什么感觉

XML 侧只描述结构：

```xml
<XWindow Title="示例窗口" Width="704" Height="404">
  <XGrid>
    <XLabel Name="title" Text="图纸处理流程" FontSize="18"/>
    <XButton Name="ok" Text="确定" on_click="on_ok"/>
    <XButton Name="cancel" Text="取消" on_click="on_cancel"/>
  </XGrid>
</XWindow>
```

Python 侧拿到窗口对象，注册回调，开消息泵：

```python
import PyMfcXml as M

names = {}
win = M.guiload("ui.xml", context=globals(), names=names)
win.show()
M.pump()

def on_ok():
    print("确定被点击", names["title"].Text)

M.pump(400)
```

控件名直接当属性用（`win.ok`、`names["title"]`），不用 `FindWindow`、不用句柄。

## 三、几个设计取舍

**回调统一走 Python。** XML 里写 `on_click="fn"`，或者加载后在 Python 里 `child.click = fn` 赋值，两条路都支持。事件契约在 C++ 侧是一张表（属性名 / 键 / 载荷 / 分派方式），而不是散落的 `if` 链，加控件时只填表不改逻辑。

**XML 与 HTML/CSS 双写法。** 同一套布局既能用 XML 标签写，也能用 `flex`、`grid`、`margin`、`border-radius` 这类 CSS 语义写，两条路径走同一批 C++ 绘制函数，所以渲染结果一致——设计器画布、独立预览器、运行时看到的是同一个画面，不会出现"设计器好看、跑起来不一样"。

**C++ 只提供基础控件，复合控件放插件目录。** 通用组合再下沉回 C++，避免把业务界面硬编码进控件库。

**free-threaded Python。** 绑定层按 `Py_GIL_DISABLED` 自动选择 `python315t.lib` / `python315.lib`，Python 3.15 的 free-threaded 版本和 GIL 版本都能编。

## 四、仓库结构

| 目录 | 说明 |
|---|---|
| `PyArxCore` | ObjectArx 核心封装：模块入口、数据库、实体、事务 |
| `PyArxEngine` | AutoCAD `.arx` 入口，在 CAD 进程内加载 Python 运行时并注册命令 |
| `MfcXmlCore` | XML 解析引擎、控件库、属性表、事件契约 |
| `PyMfcXml` | MFC 宿主：窗口、消息泵、皮肤自绘、事件派发 |
| `MfcXmlRuntime` | 复合控件运行时（Python） |
| `UIDesign` | 可视化 UI 设计器 |
| `Viewer` | 独立预览器 |
| `plugins` | 用 XML 组合出的复合控件 |

## 五、构建与环境

- Windows 64 位 + Visual Studio / MSBuild
- ObjectArx SDK 2024
- Python 3.15（free-threaded 或标准版均可）

```bash
MSBuild PyArx.sln /t:PyMfcXml /p:Configuration=Release /p:Platform=x64
```

`third_party`（pybind11 / zlib / python315t）和构建产物不入库，需要自行准备。

## 六、现状与限制

- 只面向 Windows + AutoCAD，跨平台不在计划内
- 控件是自绘的，风格偏工业软件，不是原生控件外观
- 文档还在补，控件清单与属性表以运行时导出的 schema 为准

## 七、链接

- GitHub：https://github.com/fengyeqiyu/PyArx
- Gitee：https://gitee.com/fengyeqiyu/PyArx

如果你也在做 CAD 插件、又不想为了一个对话框写几百行 MFC，可以看看这个仓库，欢迎提 issue 交流。
