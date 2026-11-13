import io
P = r'G:\PyArx\PyMfcXaml\PyMfcXaml.cpp'
t = io.open(P, encoding='utf-8-sig').read()

# 1) 取出误插在模块函数里的整块（从注释分隔线到 "m.def("guiload", &Guiload,"）
start = t.index("    // -----------------------------------------------------------------------\n    // XAML 布局计算：只读 XAML")
end = t.index('    m.def("guiload", &Guiload,')
block = t[start:end]
t = t[:start] + t[end:]

# 2) 修正 docstring 里的真实换行
block = block.replace('"读 XAML 按客户区尺寸算布局，返回 {x:Name: (x, y, w, h)}；\\n"', '@@SPLIT@@')
# 兜底：把 block 里 "；\n  " 那行的真实换行修掉
block = block.replace('py::arg("overrides") = py::none(),\n          "读 XAML 按客户区尺寸算布局', 'py::arg("overrides") = py::none(), "读 XAML 按客户区尺寸算布局')
block = block.replace('返回 {x:Name: (x, y, w, h)}；\n          "  overrides 可运行时改行列尺寸，键 \'Root/cols/3\'、值 \'640\'");',
                      '返回 {x:Name: (x, y, w, h)}；overrides 可运行时改行列尺寸，键 Root/cols/3、值 640");')

# 3) 函数体（去掉 m.def 部分）插到 Guiload 定义之前
defpart = block[:block.index('    m.def(')]
anchor = "    py::object Guiload(const std::string& path, py::object target, py::object context,"
assert t.count(anchor) == 1
t = t.replace(anchor, defpart + anchor, 1)

# 4) m.def 单独留在 register_control 前
defblock = block[block.index('    m.def('):]
anchor2 = '    m.def("register_control", &RegisterPythonControl,'
assert t.count(anchor2) == 1
t = t.replace(anchor2, defblock + anchor2, 1)

io.open(P, 'w', encoding='utf-8-sig', newline='').write(t)
print('位置修正完成')
