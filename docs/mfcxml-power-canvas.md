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
<PowerCanvas Name="Plan" Width="978" Height="660" ShowCable="1" Grid="1" Ruler="1"
             Data="{&quot;dir&quot;:&quot;h&quot;,&quot;nodes&quot;:[…]}"/>
```

`Data` 以 `{` 开头：先按"已算好坐标的场景"（`parseScene`）读，失败再按 `nodes` 由
`buildPlan` 排版。控件不要求调用方写坐标。`Grid` = 纸面网格（每 100 模型单位，
缩放后每格小于 8px 就自动不画），`Ruler` = 左/上 18px 刻度带（每 100 一小格、每 500 标数字）。

### 小地图

小地图是同一控件的另一种用法，不写 `Data`：

```xml
<PowerCanvas Name="Plan" Width="978" Height="660" ShowCable="1" Grid="1" Ruler="1" Data="{…}"/>
<PowerCanvas Name="Mini" Width="316" Height="160" Mini="1" MiniOf="Plan"/>
```

- 迷你画布整图铺满自己的矩形（不画电缆标注、网格、标尺），并把主画布的可见
  范围换算成模型坐标，在缩略图上画一个蓝色视口框；
- 点缩略图任意一处 → 换算成模型坐标 → 主画布把该点居中（`centerOn`）；
- `Mini` / `MiniOf` 是纯外观声明，三端读同一份属性，画出来必然一致；
  预览器 Ribbon「小地图」只是把这块整体显示/隐藏。

### 尺寸变化不重置视图

客户区尺寸变化（拉伸窗口、侧板宽度变化）时保持当前缩放和视图中心，只补上差额
的一半；只有「适应窗口」才真正 re-fit。以前"尺寸一变就 fit"会把刚放好的视图抹掉。

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

## 交互（预览器）

预览器只做"上一帧记下矩形和模型，下一帧按同一份命中表反查"，业务全在库里：

| 操作 | 落到哪 |
| --- | --- |
| 画布滚轮 | `PowerCanvasView::wheelAt`（锚点锁光标） |
| 画布中键拖动 | `PowerCanvasView::panBy` |
| 画布左键 | `PowerCanvasView::hit` → `sel`（卡片优先，其次回路折线容差 8/zoom） |
| Ribbon 点选项卡 | `ribbonHitTest` → 覆盖 XML 的 `SelectedTab`（否则重绘就被打回原页） |
| 适应窗口 / 放大 / 缩小 / 100% / 电缆标注 | 按钮 `Name` 直接当命令名，转成视图状态 |
| 网格 / 标尺 | 同上，翻转 `Grid` / `Ruler` 的开关态（XML 里写的是初值） |
| 小地图 | 显示/隐藏整块迷你画布；在缩略图上按下并拖动，主画布连续跟随居中 |

选中结果不是"预览器自己再画一块信息板"，而是写回外壳 XML 里那几个 `XLabel` 的
`Content`（注意解析期已把 Content 收进 `XmlElement::text`，两处都要改），所以右侧
信息板和状态行的样子仍然完全由 XML 决定。

## 验收

- `docs/_pp_shell_viewer2.png`：Viewer 打开 `power_plan_shell.xml`，12 台设备、11 条回路完整。
- `docs/_pp_shell_designer7.png`：同一份 XML 在设计器里，画布观感与 Viewer 一致。
- `docs/_pp_zoom_in.png`：设计器滚轮 48% → 73%，整窗外层缩放与画布内 fit 互不干扰。
- `docs/_pp_viselect.png`：点树顶卡片 → 红框 + 右板 P0/10kV 总进线/10kV/YJV22-3×70。
- `docs/_pp_vizoom.png`：滚轮放大，内容严格裁在画布矩形内（不再压到右板）。
- `docs/_pp_vitabview.png` / `_pp_vifit.png`：切「视图」页 → 点「适应窗口」复位，选中保持。

## 还差的（明确没做）

- 导入/导出/打印/撤销重做等按钮在预览器里是 no-op（留给运行时宿主接 Python 回调）。
- 小地图（`Mini`）、详情浮层、侧板表格编辑仍是旧 Python 实现，未搬进库。
- `XPowerCanvas` 在 `MfcXmlFrame` 的 `IXmlMfcHost` 侧未实现（该框架暂无此控件）。
- 设计器里点文档内的 Ribbon 按钮不触发命令（设计器是编辑器，点中即选中控件）。
