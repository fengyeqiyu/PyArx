# 渲染核心：Qt 机制对齐程度与验收标准

本文件记录设计器渲染核心里“哪些 Qt 机制已经搬过来、怎么量的、还差什么”。
所有数据来自副屏 (2720,420) 1600×1000 窗口、`PYARX_DESIGN_DEBUG=1` 的
`log/paint_diag.log` 与 `log/canvas_diag.log`，输入全部是模拟消息，不动真实鼠标。

## 一、机制对照

| Qt / Qt Quick 机制 | 本仓实现 | 状态 |
|---|---|---|
| 窗口 resize 期间由平台合成旧帧，应用不重画场景 | `WM_ENTERSIZEMOVE` 挂起整窗重画，`OnPaint` 直接 `StretchBlt` 上一帧位图；`WM_EXITSIZEMOVE` 作废缓存并全量重画一次 | 已完成 |
| `QQuickItem::layer.enabled`（图层缓存） | 面板常驻内存 DC/位图（仅客户区尺寸变化时重建），替代每帧 `CreateCompatibleBitmap` | 已完成（面板层） |
| 画刷图案填充（`QBrush` + `Qt::TexturePattern`） | `XmlCanvas::dotPattern()`：D2D 后端用 8×8 图块 + `ID2D1BitmapBrush`（WRAP），一次 `FillRectangle` | 已完成 |
| 视图变换与内容分离（`transform`） | 画布 `Scale` × `Translation` 统一作用于文档树与装饰层，`docPt` 反变换一致 | 已完成 |
| `QQuickItem::clip` | 画布区域 `clipPush(bg)`，文档不溢出到相邻面板 | 已完成 |
| `update()` 合并重绘（脏区） | 画布失效收窄为 `InvalidateRect(画布矩形)`；整窗仍全量重画 | 部分 |
| `QQuickWindow` 场景图 + 视口裁剪（`clip/cull`） | 未实现，`drawNode` 仍遍历全树 | 未完成 |
| 文档图层缓存（缩小/平移只贴位图） | 未实现，缩放/平移仍重画整棵文档树 | 未完成 |
| 帧同步/垂直同步节流 | 未实现，仍是 `WM_PAINT` 驱动 | 未完成 |

## 二、实测基线

| 指标 | 改造前 | 改造后 |
|---|---|---|
| 连续缩放单步（8 步合计） | 368 ms（46~68 ms/步） | **103 ms（13 ms/步）** |
| 缩放期间全量重画次数 | 每步 1 次（31 ms 级） | **0**（日志只有 `[sizeMove] begin/end`） |
| 画布预览 `paintInto` 耗时（981×623） | 15~47 ms（约 1.2 万次 `fillRect`） | **0~1 ms**（图案填充 1 次） |
| 外壳 `paintSurface` 整窗 | 31~78 ms | 16~31 ms |

## 三、验收标准（可复现的量法）

1. **拖动/缩放跟手**：模拟 `WM_ENTERSIZEMOVE` 后连续 8 次 `SetWindowPos`，
   合计 ≤ 120 ms（单步 ≤ 15 ms）；期间 `paint_diag.log` 不出现 `[paint]` 行。
2. **松手精确**：`WM_EXITSIZEMOVE` 后 1 s 内出现一次 `[paint]`，截图与松手前同尺寸内容一致，边缘无旧像素。
3. **滚轮锚点缩放**：光标处文档点在缩放前后屏幕坐标漂移 ≤ 1 px；状态栏百分比随之更新。
4. **中键平移**：鼠标位移 (dx,dy) 与文档位移一致（误差 ≤ 1 px），缩放值不变。
5. **网格开销**：画布预览 `[canvas]` 行 ≤ 1 ms（图案填充替代逐点绘制）。
6. **整窗重绘**：`[paint]` 的 shell ≤ 33 ms（30 fps 门槛），复杂文档（≥1000 节点）≤ 16 ms —— 后者依赖文档图层缓存与视口裁剪，尚未达标。

## 四、下一步（未完成项）

- 文档图层缓存：文档变更时把文档树渲染进一张按文档尺寸的离屏位图，平移/缩放只做位图变换；
- 视口裁剪：`drawNode` 传入视口矩形，整棵子树在外时直接跳过；
- 重绘节流：用 `SetTimer` 合并同一帧内的多次失效，避免一次输入触发多次整窗绘制。
