import io
P = r'G:\PyArx\PyMfcXaml\PyMfcXaml.cpp'
head = '''// PyMfcXaml.cpp —— 只做"装配"：源码按类/职责分片放在 parts\\ 下，这里负责按顺序聚合。
// 改代码请直接改对应分片；本文件只维护顺序，不再放任何实现。
//
// ── 公共区 ──────────────────────────────────────────────────────────────
//   parts\\01-pre.inc            公共头 / 宏 / 辅助函数 / 前向声明
//
// ── class PyMfcHost（宿主：控件库 + 皮肤 + 事件 + XAML 桥）──────────────
//   parts\\02a-host-core.inc      类声明 / 构造 / 窗口与子控件管理 / 选项卡页 / 自绘过程 / 图片 / Tab 焦点
//   parts\\02b-host-widgets.inc   PyMfcSkinBox 自绘小控件（Switch/RangeSlider/SpinBox/Tumbler/ScrollBar/Dial）
//   parts\\02c-host-fill.inc      内容填充（Tab/Tree/ListCtrl/Grid/ToolBar）
//   parts\\02d-host-export.inc    控件级导出（PrintWindow/WIC PNG）与 Z 序 Level
//   parts\\02e-host-canvas.inc    画布图元解析/选中/删除、透明度、页导航
//   parts\\02f-host-anim.inc      属性动画引擎
//   parts\\02g-host-props.inc     通用属性推送 / 滚轮路由 / 活动单元格 / 富文本
//
// ── 窗口过程与离屏渲染 ────────────────────────────────────────────────
//   parts\\03a-procs-skin.inc     全控件自绘 + 皮肤绘制
//   parts\\03b-procs-canvas.inc   配电画布鼠标状态机 + 可编辑表格 Grid 过程
//   parts\\03c-procs-off.inc      离屏渲染（PNG / D2D / nanosvg）
//
// ── 包装类与装载 ──────────────────────────────────────────────────────
//   parts\\04-wrap.inc            PyMfcTopWnd / PyMfcWindow / PyMfcPanel / PyMfcStore
//   parts\\05-load.inc            guiload（XML → 控件树）
//   parts\\06-bind.inc            PYBIND11_MODULE 绑定区

#include "parts\\01-pre.inc"

#include "parts\\02a-host-core.inc"
#include "parts\\02b-host-widgets.inc"
#include "parts\\02c-host-fill.inc"
#include "parts\\02d-host-export.inc"
#include "parts\\02e-host-canvas.inc"
#include "parts\\02f-host-anim.inc"
#include "parts\\02g-host-props.inc"

#include "parts\\03a-procs-skin.inc"
#include "parts\\03b-procs-canvas.inc"
#include "parts\\03c-procs-off.inc"

#include "parts\\04-wrap.inc"
#include "parts\\05-load.inc"
#include "parts\\06-bind.inc"
'''
io.open(P, 'w', encoding='utf-8-sig', newline='').write(head)
print('装配文件已更新')
