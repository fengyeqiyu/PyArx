import io
p = 'G:/PyArx/docs/mfcxaml-composite-controls.md'
s = io.open(p, encoding='utf-8').read()
old = "| `fill_table()`（清空 + 39 行 `addTableRow`） | **22.5 ms** | 每行 0.58 ms，跨 pybind 39 次 + `LVM_INSERTITEM` 39 次 |"
new = "| `fill_table()` 旧（清空 + 39 行 `addTableRow`） | **22.5 ms** | 每行 0.58 ms，跨 pybind 39 次 + `LVM_INSERTITEM` 39 次 |\n| `fill_table()` 新（一次 `setTableRows`） | **15.6 ms**（最快 9.7） | 39 行整表一条消息，`WM_SETREDRAW(FALSE)` 包住插入，只失效一次 |"
assert s.count(old) == 1
s = s.replace(old, new)
s = s.replace("- `G:\\PyArx\\tests\\mfc_xaml\\_perf_ctl.py` —— 逐控件重绘耗时",
 "- `G:\\PyArx\\tests\\mfc_xaml\\_perf_ctl.py` —— 逐控件重绘耗时\n\n## 七、已落地的第一刀（表格灌数据）\n\n`PyMfcHost::setTableRows(id, blob)`（行间 `\\n`、格间 `\\t`，`PyMfcWindow`/`PyMfcPanel` 均已绑定）替代\nPython 侧“清空 + 逐行 `addTableRow`”。原来每插一行都在控件重绘开启状态下失效一次，39 行就是 39 次重绘；\n现在 `WM_SETREDRAW(FALSE)` 包住全部插入、末尾只失效一次。\n\n实测：`fill_table()` 22.5 ms → **15.6 ms**（最快 9.7 ms）；回归 `pp_test_all` 27/27、`pp_split_test` 13/13。\n\n下一步仍是复合控件本身（`TableView` 内建列头/行区/单元格编辑/排序/滚动）与整窗按失效矩形裁剪——\n15.6 ms 里剩下的部分是 39×8 条 `LVM_SETITEMTEXTW`，只有改成 owner-data 虚拟列表（数据在控件里、\n按可见行画）才能再降一个量级。")
io.open(p, 'w', encoding='utf-8', newline='').write(s)
print('doc 已更新')
