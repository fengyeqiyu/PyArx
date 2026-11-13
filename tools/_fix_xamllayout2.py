import io
P = r'G:\PyArx\PyMfcXaml\PyMfcXaml.cpp'
lines = io.open(P, encoding='utf-8-sig').read().split('\n')

# 定位块：从 '// XAML 布局计算' 上面那条分隔线，到 m.def("xamlLayout" 的参数结束行
i_def = next(i for i, l in enumerate(lines) if 'py::dict XamlLayoutRects' in l)
i_start = i_def
while i_start > 0 and lines[i_start - 1].strip().startswith('// ----'):
    i_start -= 1
i_start -= 0
# 再往上多吃一行注释标题
if i_start > 0 and lines[i_start - 1].strip().startswith('// XAML 布局计算'):
    i_start -= 1
i_mdef = next(i for i, l in enumerate(lines) if 'm.def("xamlLayout"' in l)
i_end = i_mdef
while ');' not in lines[i_end]:
    i_end += 1
print('删除行 %d..%d' % (i_start, i_end))
block = '\n'.join(lines[i_start:i_end + 1])
lines = lines[:i_start] + lines[i_end + 1:]

# 把函数体（不含 m.def）与 m.def 分开
i_m = block.index('    m.def("xamlLayout"')
func_part = block[:i_m].rstrip() + '\n\n'
def_part = block[i_m:]

# 修 docstring：把跨行的字符串接回一行
def_part = def_part.replace('；\n          "', '；').replace('\\n"\n', '"')
def_part = def_part.replace('py::arg("overrides") = py::none(),"读', 'py::arg("overrides") = py::none(), "读')

text = '\n'.join(lines)
anchor = "    py::object Guiload(const std::string& path, py::object target, py::object context,"
assert text.count(anchor) == 1
text = text.replace(anchor, func_part + anchor, 1)
anchor2 = '    m.def("register_control", &RegisterPythonControl,'
assert text.count(anchor2) == 1
text = text.replace(anchor2, def_part + '\n' + anchor2, 1)
io.open(P, 'w', encoding='utf-8-sig', newline='').write(text)
print('已重排：函数体移到 Guiload 之前，def 留在模块内')
print('--- def 片段 ---')
print(def_part)
