import io
P = r'G:\PyArx\PyMfcXaml\PyMfcXaml.cpp'
t = io.open(P, encoding='utf-8-sig').read()
old = '''void PyMfcHost::OnListChange(UINT id)
{
    auto it = m_select.find(id);
    if (it != m_select.end())
        invokePython(it->second);
}'''
new = '''void PyMfcHost::OnListChange(UINT id)
{
    // 列表 / 下拉框的"选中项变了"。这里既要满足 onSelect（XamlEvents 的 SelectionChanged），
    // 也要满足 onChange —— 原先只派发 onSelect，用 onChange 绑下拉框的（例如布局方向切换）
    // 永远收不到回调，表现为"选了没反应"。
    auto it = m_select.find(id);
    if (it != m_select.end())
        invokePython(it->second);
    auto ic = m_change.find(id);
    if (ic != m_change.end())
        invokePython(ic->second);
}'''
assert t.count(old) == 1, 'pattern'
t = t.replace(old, new)
io.open(P, 'w', encoding='utf-8-sig', newline='').write(t)
print('OnListChange 已派发 m_change')
