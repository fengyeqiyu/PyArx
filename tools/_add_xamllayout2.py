import io
P = r'G:\PyArx\PyMfcXaml\PyMfcXaml.cpp'
t = io.open(P, encoding='utf-8-sig').read()

old = """    py::dict XamlLayoutRects(const std::string& path, int width, int height)
    {
        XamlRuntime rt;
        std::wstring loadError;
        if (!rt.loadFile(Utf8ToWide(path).c_str(), loadError))
            throw std::runtime_error("xamlLayout: " + Utf8Of(loadError));
        rt.applyDeclarations();
        rt.layout(0, 0, width, height);"""
new = """    // 找名为 name 的元素（深度优先）
    XamlElement* FindXamlByName(XamlElement* node, const std::wstring& name)
    {
        if (node == nullptr)
            return nullptr;
        if (node->name == name)
            return node;
        for (auto& c : node->children)
        {
            if (XamlElement* hit = FindXamlByName(&c, name))
                return hit;
        }
        return nullptr;
    }

    py::dict XamlLayoutRects(const std::string& path, int width, int height,
                             py::object overrides)
    {
        XamlRuntime rt;
        std::wstring loadError;
        if (!rt.loadFile(Utf8ToWide(path).c_str(), loadError))
            throw std::runtime_error("xamlLayout: " + Utf8Of(loadError));

        // 运行时覆盖行列尺寸：键形如 "Root/cols/3"（容器名/cols|rows/索引），值为 "640"、"*"
        if (!overrides.is_none() && py::isinstance<py::dict>(overrides))
        {
            for (auto item : overrides.cast<py::dict>())
            {
                const std::string key = item.first.cast<std::string>();
                const std::string val = item.second.cast<std::string>();
                std::vector<std::string> parts;
                std::string cur;
                for (char ch : key)
                {
                    if (ch == '/')
                    {
                        parts.push_back(cur);
                        cur.clear();
                    }
                    else
                        cur.push_back(ch);
                }
                parts.push_back(cur);
                if (parts.size() != 3 || rt.root() == nullptr)
                    continue;
                XamlElement* el = FindXamlByName(rt.root(), Utf8ToWide(parts[0]));
                if (el == nullptr)
                    continue;
                const size_t idx = static_cast<size_t>(std::stoi(parts[2]));
                auto& defs = (parts[1] == "rows") ? el->gridRows : el->gridCols;
                if (idx >= defs.size())
                    continue;
                defs[idx].second = Utf8ToWide(val);
            }
        }

        rt.applyDeclarations();
        rt.layout(0, 0, width, height);"""
assert t.count(old) == 1
t = t.replace(old, new)

old = """    m.def("xamlLayout", &XamlLayoutRects, py::arg("path"), py::arg("width"), py::arg("height"),
          "读 XAML 按客户区尺寸算布局，返回 {x:Name: (x, y, w, h)}");"""
new = """    m.def("xamlLayout", &XamlLayoutRects, py::arg("path"), py::arg("width"), py::arg("height"),
          py::arg("overrides") = py::none(),
          "读 XAML 按客户区尺寸算布局，返回 {x:Name: (x, y, w, h)}；\n"
          "  overrides 可运行时改行列尺寸，键 'Root/cols/3'、值 '640'");"""
assert t.count(old) == 1
t = t.replace(old, new)
io.open(P, 'w', encoding='utf-8-sig', newline='').write(t)
print('xamlLayout 支持 overrides')
