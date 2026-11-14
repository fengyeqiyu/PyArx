# 配电规划画布迁到控件库（新底层逻辑）

以前配电图（power-plan）只能在 Python 宿主里跑：解析、排版、绘制、命中、缩放全写在
`PyMfcXml/PyMfcHost.cpp` + `PyMfcCommon.h` 里，设计器和 Viewer 一行都取不到，
所以"设计器看到的、预览器看到的、跑起来的"三张脸。

本轮按 `UIdesign 只是调用者 / 控件库要有 frame 框架 / 画布调用统一渲染器` 重做：

| 层 | 文件 | 职责 |
| --- | --- | --- |
| 模型 | `MfcXmlCore/XmlPowerPlan.h/.cpp` | 节点/回路/布局（`buildPlan`）、JSON/CSV/SVG/XML 出入口 |
| 绘制 | `MfcXmlCore/XmlPowerPlanDraw.h` | `xmlplan::drawPlan(cv, plan, k, ox, oy, sel, showCable, clip)` —— 唯一一份画法 |
| 控件 | `MfcXmlCore/XmlPowerCanvasView.h` | `xmlctl::PowerCanvasView`：解析 `Data`、fit、绘制、命中、缩放、平移 |
| 外壳 | `examples/ui/power_plan_shell.xml` | Ribbon + 画布 + 右侧信息板 + 状态行，全部 XML 声明 |
| 三端 | `Viewer/XmlViewer.cpp`、`UIDesign/DesignCanvas.h`、`PyMfcXml/PyMfcCommon.h` | 只是把矩形和数据喂给 `PowerCanvasView` |

## 用法（XML）

```xml
<PowerCanvas Name="Plan" Width="978" Height="660" ShowCable="1"
             Data="{&quot;dir&quot;:&quot;h&quot;,&quot;nodes&quot;:[…]}"/>
```

`Data` 以 `{` 开头：先按"已算好坐标的场景"（`parseScene`）读，失败再按 `nodes` 由
`buildPlan` 排版。控件不要求调用方写坐标。

## 三层各做什么

- **控件库**：解析 / 排版 / 绘制 / 命中 / 缩放平移，全部在库里；换外观只改 XML。
- **设计器**：`DesignCanvas::drawNode` 里一个 `PowerCanvas` 分支，传文档坐标矩形；
  视图状态按 `XmlElement*` 缓存，不参与排版。
- **预览器**：`XmlViewer::drawElement` 里同构分支，传屏幕矩形。
- **运行时（Python）**：`drawPowerPlanContent` 变成一行转发，行为不变。

## 顺带修掉的两个根因

1. `D2DDcCanvas` 维护自己的矩阵栈，但设计器画布原来直接对 render target
   `SetTransform`。任何 `pushRotate/popTransform`（配电图的竖排电缆标注就要用）
   `pop` 回来是 Identity，外层缩放平移被抹掉 —— 卡片整体跑位。
   现在矩阵归后端（`XmlCanvas::setViewTransform`），设计器改调它。
2. 设计器文档画布里 `XRibbon` / `PowerCanvas` 以前是"占位框"，
   现在与预览器同一份 `ribbonDraw` / `PowerCanvasView`。

## 验收

- `docs/_pp_shell_viewer2.png`：Viewer 打开 `power_plan_shell.xml`，12 台设备、11 条回路完整。
- `docs/_pp_shell_designer7.png`：同一份 XML 在设计器里，画布观感与 Viewer 一致。
- `docs/_pp_zoom_in.png`：设计器滚轮 48% → 73%，整窗外层缩放与画布内 fit 互不干扰。

## 还差的（明确没做）

- 运行时按钮命令（适应窗口 / 电缆标注开关 / 导出）绑定，外壳里按钮已就位但命令未接。
- 小地图（`Mini`）、详情浮层、侧板表格编辑仍是旧 Python 实现，未搬进库。
- `XPowerCanvas` 在 `MfcXmlFrame` 的 `IXmlMfcHost` 侧未实现（该框架暂无此控件）。
