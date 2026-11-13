import io
P = r'G:\PyArx\docs\mfcxaml-powerplan-app.md'
t = io.open(P, encoding='utf-8-sig').read()
add = """
## 6. 闪烁 / 拖影：根因与修复（实测定位，非推测）

用户原话「界面闪烁厉害，找出原因」「拖影太严重了」。逐条查下来是四个独立原因叠在一起：

| # | 位置 | 原因 | 修法 |
|---|---|---|---|
| 1 | `moveMany` | 每个控件 `InvalidateRect(c, nullptr, TRUE)` 先把控件擦成白底再重画，一帧里白→内容 → 肉眼"闪一下" | erase 参数改 `FALSE`（控件自己首行 `fillRect` 铺底，本来就不需要预先擦） |
| 2 | `setPowerCanvasView` | 同上，缩放/平移每一帧都擦白一次 | 同样 `TRUE` → `FALSE` |
| 3 | 主窗 `CreateWindowExW` | 没有 `WS_EX_COMPOSITED`，子控件各自往屏幕画，重排时能看到中间态 | 加 `WS_EX_COMPOSITED`（整窗离屏合成，重排只在最后合成一次） |
| 4 | `moveMany` 改控件位置 | 控件挪走后，旧位置属于**父容器**；父容器没标脏，主窗又带 `WS_CLIPCHILDREN` 把这块裁掉 → 旧内容原地不动 = 拖影（用户截图里表格区出现两行重复） | 对 `::GetParent(c) != m_hWnd` 的父容器补 `InvalidateRect(p, nullptr, FALSE)` |

修复后整窗截图 `docs/_pp_live.png`：表格 39 行无重复残影、右侧无灰带、画布与状态栏对齐。

## 7. 控件级导出 PNG（基类能力）

`PyMfcHost::saveControlPng(id, path)` —— 任意控件（含 D2D 自绘画布、原生 ListView）都能按当前外观导成 PNG：

```python
w.saveControlPng(CV,  r'G:\\PyArx\\docs\\_ctl_canvas.png')   # 33638 B
w.saveControlPng(TBL, r'G:\\PyArx\\docs\\_ctl_table.png')    # 60081 B
```

做法：`PrintWindow(c, memDC, PW_RENDERFULLCONTENT=2)` 画进 32bpp 自顶向下 DIB → `GdiFlush` → `wicWritePng`。
自绘控件之所以能抓到而不是黑图，是因为 `PyMfcOwnerDrawProc` 新增了 `WM_PRINTCLIENT` 分支：拿到外部 HDC 后按
和 `WM_DRAWITEM` 完全同一条路径重画一遍。Python 侧 `PyMfcWindow` / `PyMfcPanel` 都转发了这个方法。

## 8. 事件驱动验证（不需要真人操作）

`tests/mfc_xaml/pp_events.py` 把模拟输入封成函数，全部走 `user32.SendMessageW` 直接投递到控件过程：

```python
cv  = E.find_control(app.win.hwnd, pp_app.CV)   # 递归找句柄（GetDlgItem 取不到自绘控件）
E.click(cv, 140, 389)                           # 单击
E.double_click(tbl, 60, 60)                     # 双击
E.drag(cv, 400, 300, 560, 400)                  # 按住拖动
E.wheel(cv, 120, 300, 300)                      # 滚轮（lParam 用屏幕坐标）
E.key(tbl, 0x71)                                # F2 打开行编辑条（VK_F2=0x71，不是 0x72）
```

`pp_event_test.py` 8 项全 PASS：找到画布/表格句柄、单击选中卡片、滚轮 `0.2000→0.2240` 并反向缩回、
拖动视口 `(36,334)→(196,434)`、F2 展开行编辑条、取消收起。

## 9. 「选不中画布里的节点与线」的真因

不是命中测试的问题，是**单击被丢掉了**：

```
WM_LBUTTONUP:  if (GetCapture()==hwnd) ReleaseCapture();   // ← 同步发出 WM_CAPTURECHANGED
               const bool wasDrag = st.drag;              // ← 已经被那个分支清成 false
```

`ReleaseCapture` 会**同步**回调本窗口的 `WM_CAPTURECHANGED`，而那个分支里 `st.drag = false`；
所以 `wasDrag` 永远读到 false，单击分支根本不执行 → `planHit` 从来没被调用过 → 选不中。
（`hitPlan()` 直接调 API 是好的，这个反差就是定位线索。）

修法：**先把状态取下来再释放捕获**（`wasDrag = st.drag; st.drag = false; if (GetCapture()) ReleaseCapture();`）。
同时补了两件：`planHit` 在盒命中失败后做贝塞尔折线最近距离（24 段、容差 `8.0f/zoom`，命中返回 `l.to`）；
Python 侧补上 `w.onClick(CV, self.on_canvas_click)` —— 之前只绑了 `onDoubleClick`，`on_canvas_click` 存在但从没接上。

`pp_select_test.py` 7/7 PASS：单击卡片 → `getPlanSelection` 得 `AT1-1` + 详情浮层打开、点空白清空、
点连线中点 (242,378) → 选中下游 `AT1-1`、两个控件级 PNG 落盘。

## 10. 回归基线

`pp_grid_test.py` **18/18**；`pp_test_all.py` **27/27**；`pp_select_test.py` **7/7**；`pp_event_test.py` **8/8**。
"""
t = t.rstrip() + "\n" + add
io.open(P, 'w', encoding='utf-8-sig', newline='').write(t)
print('文档已追加 6~10 节')
