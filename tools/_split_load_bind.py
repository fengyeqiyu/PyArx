import io, re, os
import declsplit as ds

LOAD = 'G:/PyArx/PyMfcXaml/parts/05-load.inc'
BIND = 'G:/PyArx/PyMfcXaml/parts/06-bind.inc'
OUT_LOAD = 'G:/PyArx/PyMfcXaml/PyMfcLoad.cpp'
OUT_PARTS = 'G:/PyArx/PyMfcXaml/PyMfcParts.h'
OUT_BIND = 'G:/PyArx/PyMfcXaml/PyMfcBind.cpp'

lines = io.open(LOAD, encoding='utf-8-sig').read().split('\n')
sc = ds.scan(lines)

EXPORT = ['RegisterPythonControl', 'Guiload', 'XamlLayoutRects']


def chunk_at(start):
    """从 start 行开始，取到花括号闭合（含签名可能跨行）"""
    i = start
    dep = 0
    opened = False
    while i < len(lines):
        dep += sc[i][0]
        opened = opened or sc[i][1]
        i += 1
        if opened and dep == 0:
            break
    return start, i


chunks = []
for name in EXPORT:
    idx = None
    for j, l in enumerate(lines):
        if not re.search(r'\b' + name + r'\s*\(', l):
            continue
        if l.strip().startswith(('//', 'm.def', '.def', 'return')):
            continue
        if ds.endsemi(l):
            continue
        # 签名后面紧跟 '{' 行（签名可能跨行，最多看 8 行；中间不能出现 ';'）
        k = j + 1
        ok = True
        while k < len(lines) and k <= j + 8:
            st = lines[k].strip()
            if st == '{' or st.endswith('{'):
                break
            if ';' in st:
                ok = False
                break
            k += 1
        if ok and k < len(lines) and (lines[k].strip() == '{' or lines[k].strip().endswith('{')):
            idx = j
            break
    if idx is None:
        raise SystemExit('找不到 %s 的定义' % name)
    # 签名可能从上一行开始（返回类型单独一行）
    s, e = chunk_at(idx)
    chunks.append((name, s, e))

# 从后往前删，避免行号错位
moved = []
newlines = list(lines)
for name, s, e in sorted(chunks, key=lambda x: -x[1]):
    moved.append((name, '\n'.join(lines[s:e])))
    del newlines[s:e]

tail = moved[::-1]

body = '\n'.join(newlines).rstrip('\n')
out = ('// PyMfcLoad.cpp —— guiload：XAML 文件 → 控件树（布局运行时 + 控件工厂）。\n'
       '// 原名 parts\\05-load.inc；三个对外入口（Guiload / XamlLayoutRects / RegisterPythonControl）\n'
       '// 已移出匿名 namespace 成为外部链接函数，声明见 PyMfcParts.h。\n\n'
       '#include "PyMfcCommon.h"\n'
       '#include "PyMfcWrap.h"\n'
       '#include "PyMfcParts.h"\n\n'
       + body + '\n\n'
       + '\n\n'.join(t[1] for t in tail) + '\n')
io.open(OUT_LOAD, 'w', encoding='utf-8-sig', newline='').write(out)

parts = ('// PyMfcParts.h —— 各编译单元之间共享的内部入口（不属于 Python 绑定接口）。\n'
         '// 放在这里而不是某个 .cpp 的匿名 namespace 里，才能跨 TU 使用。\n\n'
         '#pragma once\n\n'
         '#include "PyMfcCommon.h"\n'
         '#include "PyMfcWrap.h"\n\n'
         '// 把 Python 工厂注册进统一控件注册表（register_control）\n'
         'void RegisterPythonControl(const std::string& tag, py::object factory);\n\n'
         '// XAML 文件 → 控件树（guiload）\n'
         'py::object Guiload(const std::string& path, py::object target, py::object context,\n'
         '                   py::object names, int firstId, py::object palette);\n\n'
         '// 只跑布局、返回各 x:Name 矩形（xamlLayout）\n'
         'py::dict XamlLayoutRects(const std::string& path, int width, int height,\n'
         '                         py::object overrides);\n')
io.open(OUT_PARTS, 'w', encoding='utf-8', newline='').write(parts)

bind = io.open(BIND, encoding='utf-8-sig').read()
# 去掉切分来源注释行
bind = re.sub(r'^// PYBIND11_MODULE.*\n', '', bind)
out_bind = ('// PyMfcBind.cpp —— PYBIND11_MODULE(PyMfcXaml)：全部 Python 接口定义。\n'
            '// 原名 parts\\06-bind.inc。整个工程只有这一处模块定义。\n\n'
            '#include "PyMfcCommon.h"\n'
            '#include "PyMfcWrap.h"\n'
            '#include "PyMfcParts.h"\n\n'
            + bind.lstrip('\n'))
io.open(OUT_BIND, 'w', encoding='utf-8-sig', newline='').write(out_bind)

print('PyMfcLoad.cpp %d 行；PyMfcBind.cpp %d 行；PyMfcParts.h %d 行'
      % (len(out.split('\n')), len(out_bind.split('\n')), len(parts.split('\n'))))
print('移出的三个入口：', ', '.join(t[0] for t in tail))
