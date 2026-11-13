import io
p = 'G:/PyArx/PyMfcXaml/PyMfcHost.h'
s = io.open(p, encoding='utf-8').read()
old = """// PyMfcHost.h —— 宿主类 PyMfcHost 的完整定义（控件库 / 皮肤 / 事件 / XAML 桥）。
// 由 02a..02g 七个域分片合并而成；改代码请改 parts\\02x-*.inc 后重新生成本文件，
// 或在 parts 里继续细分。声明与实现分离（.h/.cpp）是下一步。"""
new = """// PyMfcHost.h —— 宿主类 PyMfcHost 的接口（控件库 / 皮肤 / 事件 / XAML 桥）。
// 这里只放声明、嵌套类型、数据成员与类内宏；实现全部在 PyMfcHost.cpp（312 个成员）。
// 改接口改这个文件，改实现改 PyMfcHost.cpp。"""
assert old in s, '旧注释没找到'
s = s.replace(old, new)
io.open(p, 'w', encoding='utf-8', newline='').write(s)
print('PyMfcHost.h 头部注释已更新')
