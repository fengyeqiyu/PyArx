# PyMfcXml 源码结构

原来的单文件 `PyMfcXml.cpp`（11683 行）已**全部**拆成按类/职责的编译单元与头文件，不再有任何 `.inc` 分片、也没有"先 include 分片再编译"的装配文件。

## 编译单元（8 个，全部在 vcxproj 里独立编译）

| 文件 | 行数 | 职责 |
|---|---|---|
| `PyMfcHost.cpp` | 5456 | `class PyMfcHost` 全部成员函数实现（312 个）：控件创建 / 皮肤 / 事件 / 画布 / XML 桥 |
| `PyMfcProcs.cpp` | 2137 | 窗口过程：全控件自绘、配电画布鼠标状态机、可编辑表格 Grid、滚轮安装 |
| `PyMfcWrap.cpp` | 1969 | `PyMfcTopWnd` / `PyMfcWindow` / `PyMfcPanel` 的实现（423 个成员函数） |
| `PyMfcLoad.cpp` | 1598 | `guiload`：XML 文件 → 控件树（布局运行时 + 控件工厂） |
| `PyMfcBind.cpp` | 601 | `PYBIND11_MODULE(PyMfcXml)`：全部 Python 接口。**全工程只此一处** |
| `PyMfcOffscreen.cpp` | 212 | 离屏渲染：BGRA→PNG(WIC)、D2D 配电图、nanosvg SVG |
| `XmlBindings.cpp` | 560 | XmlRuntime / XmlElement 的 Python 绑定（原有） |
| `dllmain.cpp` | 29 | MFC 模块态 DLL 入口 |

## 头文件

| 文件 | 行数 | 内容 |
|---|---|---|
| `PyMfcCommon.h` | 451 | 公共头 / 宏 / 辅助函数 / 前向声明；跨 TU 导出声明（`wicWritePng`、`installWheelHost`、`PyMfcGridProc`、`PyMfcPlanCanvasMouse`） |
| `PyMfcHost.h` | 1066 | `class PyMfcHost` 只留声明、嵌套类型（`Item` / `PlanCanvasState` / `AnimTrack`）、数据成员、类内 `static constexpr` 与宏 |
| `PyMfcWrap.h` | 571 | `PyMfcNoDelete` / `PyMfcHandle` / `PyMfcTopWnd` / `PyMfcWindow` / `PyMfcPanel` / `PyMfcStore` 的声明与 `PyMfcStore` 内联单例 |
| `PyMfcParts.h` | 19 | 跨 TU 内部入口：`Guiload`、`XmlLayoutRects`、`RegisterPythonControl` |
| `XmlBindings.h` | — | `RegisterXmlBindings(py::module&)` |

## 跨 TU 的三种写法（都已在代码里落实）

1. **头文件只放声明**：`PyMfcHost.h` 5833 → 1066；`PyMfcWrap.h` 571。改实现只重编一个 TU。
2. **匿名 namespace 里的入口要显式导出**：`PyMfcLoad.cpp` 内部全是匿名 namespace 的辅助函数，
   只有 `Guiload` / `XmlLayoutRects` / `RegisterPythonControl` 三个是外部链接，声明在 `PyMfcParts.h`；
   做法是把这三个函数的定义**移出匿名 namespace、放到文件末尾**（在所有辅助函数之后），
   这样既不用给每个辅助函数加 `static`，也不会污染全局符号。
3. **类内 static 过程去 `static`**：`PyMfcGridProc` / `PyMfcPlanCanvasMouse` 声明与定义一起去掉 `static`，
   否则别的 TU 报 `C2129 已声明但未定义`。

## 构建与验证

```powershell
$env:ComSpec='C:\Windows\SysWOW64\cmd.exe'
& 'C:\Program Files\Microsoft Visual Studio\18\Community\MSBuild\Current\Bin\MSBuild.exe' `
  'G:\PyArx\PyArx.sln' /t:MfcXmlCore /t:PyMfcXml /p:Configuration=Release /p:Platform=x64 /m /nologo /v:m
```

回归（`G:\PyArx\tests\mfc_xml`，`$env:PYARX_D2D_RT='software'`）：
`pp_test_all.py` 27/27、`pp_split_test.py` 13/13（连跑三次稳定）、`pp_cell_test.py` 15/15、`_probe_xmllayout.py` 两组矩形不变。

> 拖动回调的横坐标约定：`fireDrag` 传的是**屏幕 x**（`PyMfcHost.cpp` 里的 `DragScreenX`）。
> 分割条在拖动中每帧都会移位，用客户坐标的话零点跟着走，同一屏幕点在不同帧映射出不同的 x，
> 按差分算宽度就会漂（实测出现 5/10、8/10 这种间歇失败）。屏幕坐标是拖动全程不变的参照系。
> Python 侧按**绝对目标宽度**算（`base + (x0 - x)`），不做逐帧增量累加；尺寸边界由版式推导
> （`_side_bounds`：上限 = min(900, 客户宽 − 边距 − 把手宽 − 画布最小宽 320)），
> 并且把「用户偏好宽度 `_ed_w`」与「当前生效宽度 `_side_width()`」分开：
> 窗口临时变窄只收窄生效值，不动偏好，窗口恢复后回到用户拖出来的宽度。

## 拆分工具（一次性脚本，可复用）

- `G:\PyArx\tools\declsplit.py` —— 通用"类体内联 → 声明头 + 实现 cpp"切分器：
  按**花括号深度**遍历（不按缩进，源码里窗口过程签名写在第 0 列）、剥行尾 `//` 再判 `;`、
  预处理 `}   下一个签名` 这类粘连行、签名上方注释留在头里、`static/explicit/constexpr` 等前缀只在头里保留。
- `G:\PyArx\tools\_split_host_decl.py`、`_split_wrap.py`、`_split_load_bind.py` —— 三次拆分的驱动脚本。
- 原文件备份：`G:\PyArx\docs\_backup_PyMfcHost.h`（5833 行）、`G:\PyArx\docs\_backup_PyMfcXml.cpp`（11683 行）、
  `G:\PyArx\docs\_backup_parts\`（04/05/06 三个 .inc + 旧装配文件）。
