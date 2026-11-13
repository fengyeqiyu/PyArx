import io
P = r'G:\PyArx\PyMfcXaml\PyMfcXaml.cpp'
t = io.open(P, encoding='utf-8-sig').read()

anchor = """    m.def("guiload", &Guiload,"""
code = """    // -----------------------------------------------------------------------
    // XAML 布局计算：只读 XAML，按给定客户区尺寸算出每个 x:Name 的矩形。
    // 用途：界面布局/尺寸写在 .xaml 里，Python 拿结果 moveMany，不再手算坐标；
    //       控件仍由现有 addXxx 创建（画布/原生表格等 guiload 不支持的类型也能用）。
    // 返回 {name: (x, y, w, h)}，坐标相对客户区。
    // -----------------------------------------------------------------------
    py::dict XamlLayoutRects(const std::string& path, int width, int height)
    {
        XamlRuntime rt;
        std::wstring loadError;
        if (!rt.loadFile(Utf8ToWide(path).c_str(), loadError))
            throw std::runtime_error("xamlLayout: " + Utf8Of(loadError));
        rt.applyDeclarations();
        rt.layout(0, 0, width, height);

        py::dict out;
        std::function<void(XamlElement&)> walk = [&](XamlElement& el) {
            if (!el.name.empty())
            {
                out[py::str(Utf8Of(el.name))] =
                    py::make_tuple(el.previewX, el.previewY, el.previewW, el.previewH);
            }
            for (auto& c : el.children)
                walk(c);
        };
        if (rt.root() != nullptr)
            walk(*rt.root());
        return out;
    }

    m.def("guiload", &Guiload,"""
assert t.count(anchor) == 1
t = t.replace(anchor, code, 1)

anchor2 = """    m.def("register_control", &RegisterPythonControl,"""
code2 = """    m.def("xamlLayout", &XamlLayoutRects, py::arg("path"), py::arg("width"), py::arg("height"),
          "读 XAML 按客户区尺寸算布局，返回 {x:Name: (x, y, w, h)}");

    m.def("register_control", &RegisterPythonControl,"""
assert t.count(anchor2) == 1
t = t.replace(anchor2, code2, 1)

io.open(P, 'w', encoding='utf-8-sig', newline='').write(t)
print('C++: xamlLayout 已加')
