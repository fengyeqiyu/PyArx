import io
p = 'G:/PyArx/docs/session-2026-11-11-mfcxaml-digest.md'
s = io.open(p, encoding='utf-8').read()
old = "## 五、回归状态"
new = """## 四·补、画布光标（已修，规格已定）

规格：**画布内始终手形（悬浮与拖动都是），离开画布即复位成普通箭头**。
实现：`PyMfcProcs.cpp` 画布窗口过程 `WM_SETCURSOR` 一律 `IDC_HAND`；`WM_MOUSEMOVE` 里
`TrackMouseEvent(TME_LEAVE)` 登记，`WM_MOUSELEAVE` 里 `SetCursor(IDC_ARROW)` 复位。
坑：宿主不会替子窗口复位光标，只设不复位就会"手形挂满全窗"。
用例同步改成真鼠标驱动（`SetCursorPos` + `GetCursor`）：模拟 `WM_MOUSEMOVE` 不触发 `WM_SETCURSOR`，
读到的是残值——这就是旧用例 2/4 假失败的成因。

## 五、回归状态"""
assert s.count(old) == 1
s = s.replace(old, new)
s = s.replace("- `pp_test_all` **27/27**、`pp_split_test` **13/13**、`pp_event_test` **8/8**、`pp_plugin_test` **5/5**\n- `pp_cell_test` 单独跑 **15/15**；跟别的 GUI 测试紧邻跑会 1/2（残留窗口抢鼠标/焦点，不是代码 bug）。",
"- 全量自动测试 **113 项全绿**：`pp_test_all` 27、`pp_split_test` 13、`pp_cell_test` 15、`pp_event_test` 8、\n  `pp_grid_test` 18、`pp_select_test` 7、`pp_real_test` 6、`pp_cursor_test` 5、`pp_runtime_test` 9、`pp_plugin_test` 5。\n- 批次脚本：每跑一个 GUI 用例前杀 python 并 sleep 1.4 s（残留窗口会抢鼠标/焦点造成假失败）。")
io.open(p, 'w', encoding='utf-8', newline='').write(s)
print('digest 已更新光标规格与 113 项回归')
