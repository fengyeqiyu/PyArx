import io
# 1) runtime README：两条已知问题已闭环
p = 'G:/PyArx/MfcXamlRuntime/README.md'
s = io.open(p, encoding='utf-8').read()
old = """## 已知问题

- XamlCore 星号列取整：640 宽 `12 + 3*star + 12` 得 639（每列 floor）。应把余数补给最后一列星号列。
- 真实鼠标点击 → XAML 声明回调"被调到"这条链路尚未端到端验通（只验到回调登记与 `child.click` 绑定）。"""
new = """## 已知问题

- ~~XamlCore 星号列取整丢 1px~~ **已修**：`ResolveTracks` 把取整余数补给最后一个 star 轨道，
  640 宽 `12 + 3*star + 12` 现在正好 640（三段 205/205/206）。
- ~~点击 → XAML 声明回调未验通~~ **已验证**（`pp_click_test` 5/5）：真实点击能同时调到
  "XAML 里声明的 `on_click`" 和"属性赋值的 `child.click`"。
- **坑（已修）**：XAML 声明的名字在当前作用域找不到时，原实现抛 `NameError`，会把同一控件上
  属性赋值的回调一起挡掉。现在解析失败只 `stderr` 提示一次并跳过，其余回调照常执行；
  作用域也改成取调用方模块的 `globals`（脚本以 `__main__` 跑也能解析到）。"""
assert s.count(old) == 1
s = s.replace(old, new)
io.open(p, 'w', encoding='utf-8', newline='').write(s)

# 2) digest：回归口径 118 项 + 新增点击用例
p = 'G:/PyArx/docs/session-2026-11-11-mfcxaml-digest.md'
s = io.open(p, encoding='utf-8').read()
old2 = "- 全量自动测试 **113 项全绿**"
new2 = """- 全量自动测试 **118 项**（新增 `pp_click_test` 5 项）：`pp_test_all` 27、`pp_split_test` 13、`pp_cell_test` 15、
  `pp_event_test` 8、`pp_grid_test` 18、`pp_select_test` 7、`pp_real_test` 6、`pp_cursor_test` 5、
  `pp_click_test` 5、`pp_runtime_test` 9、`pp_plugin_test` 5。
- 已闭环两件事：星号列取整（`ResolveTracks` 余数补最后一列，640 宽现在正好 640）；
  点击 → XAML 声明回调（`pp_click_test` 5/5，同时验到属性赋值回调）。
- 新坑：XAML 声明名解析失败会抛 `NameError` 把属性赋值回调一起挡掉 → 改成"提示一次并跳过"，
  作用域改为取调用方模块 globals。
- 旧的 113 项全绿"""
assert s.count(old2) == 1
s = s.replace(old2, new2)
io.open(p, 'w', encoding='utf-8', newline='').write(s)
print('docs 已更新')
