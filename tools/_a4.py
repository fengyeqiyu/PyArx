import io
p = 'G:/PyArx/docs/mfcxaml-render-refactor-plan.md'
s = io.open(p, encoding='utf-8').read()
s += """

---

## 追加：最根本的原因（2026-11-11）

分项重绘数字本身就是证据链：

| 控件 | 重绘耗时 | 该控件真正要画的量 |
|---|---|---|
| 侧板白卡 ED_PANEL | 16.6 ms | 一块纯色矩形（fillRect） |
| 表格 TBL | 8.7 ms | 一屏 30 行的文本 |
| 画布 CV | 6.0 ms | 39 卡片 + 8 条曲线 |
| 状态栏 | 1.7 ms | 一行文本 |
| 小地图 | 0.0 ms | 几乎不画 |

**一块纯色矩形要 16.6 ms** —— 纯 `FillSolidRect` 在软件渲染下是 0.1 ms 级。
这说明耗时不在"画了什么"，而在**每次 WM_PAINT 都要重新建一遍 D2D 环境**：
设备/工厂/渲染目标/缓冲 surface 每次 `BeginPaint` 重建，软件光栅下单次创建就是 5~16 ms，
再乘以"一次布局动到的子窗口数"，就是 217 ms/步。

因此三条现象是同一个病根的三个面：

1. 拖动卡 → N 个子窗口各自重建一次 D2D 环境；
2. 点节点慢 → 详情浮层显隐又带起一次多窗口重绘；
3. 分割条看不见/被盖 → 每窗口各画各的，没有统一表面，z 序与裁剪不可控。

**修法优先级（改这里，收益最大）**
- A（最根本）：**把 D2D 环境按 HWND 缓存**，只在尺寸/DPI/渲染后端变化时重建，`WM_PAINT` 里只做
  `BeginDraw / 画 / EndDraw`。单点改动，能同时把它三件事一起解决。
- B：父表面一次绘制（省掉 N 次 BeginPaint，也顺手解决 z 序）。
- C：画布离屏位图缓存；D：表格 owner-data。

A 与 B 不冲突，A 先做（改动面更小、可测量），B 用来彻底抹掉"每控件一次 paint"。
"""
io.open(p, 'w', encoding='utf-8', newline='').write(s)
print('plan updated')
