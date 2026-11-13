// PyMfcCore.xaml —— 把 PyMfcXaml 的加载器（XamlRuntime + XamlElement 树）暴露给 Python。
//
// 只暴露"加载 / 解析 / 遍历 / 布局 / 序列化"这一层，不做控件实例化：
// 实例化需要宿主 IXamlControl 工厂与 HWND，仍由 PyMfcCore 的 Window/Panel + guiload 负责。
// 这样设计器、测试脚本、外部工具都能在纯 Python 里校验 XAML，不必拉窗口。
//
// 编码：本文件 UTF-8 with BOM。

#include "XamlBindings.h"

#include "XamlCore.h"
#include "XamlPowerPlan.h"
#include "XamlSvgScene.h"

#include <pybind11/stl.h>

#include <string>
#include <vector>

namespace py = pybind11;

// 由 PyMfcXaml.cpp 提供：配电规划图 Direct2D 离屏渲染 → PNG
bool RenderPowerPlanPng(const std::wstring& path, const xamlplan::PowerPlan& plan, int scale);
bool RenderSvgPng(const std::wstring& svgPath, const std::wstring& pngPath, int scale,
                  COLORREF background, int& outW, int& outH);

namespace
{
    std::string utf8Of(const std::wstring& s)
    {
        if (s.empty())
            return {};
        const int n = ::WideCharToMultiByte(CP_UTF8, 0, s.c_str(), static_cast<int>(s.size()),
                                            nullptr, 0, nullptr, nullptr);
        std::string out(static_cast<size_t>(n), '\0');
        ::WideCharToMultiByte(CP_UTF8, 0, s.c_str(), static_cast<int>(s.size()),
                              out.data(), n, nullptr, nullptr);
        return out;
    }

    std::wstring wideOf(const std::string& s)
    {
        if (s.empty())
            return {};
        const int n = ::MultiByteToWideChar(CP_UTF8, 0, s.c_str(), static_cast<int>(s.size()),
                                            nullptr, 0);
        std::wstring out(static_cast<size_t>(n), L'\0');
        ::MultiByteToWideChar(CP_UTF8, 0, s.c_str(), static_cast<int>(s.size()),
                              out.data(), n);
        return out;
    }

    // 由 PyMfcXaml.cpp 提供：配电规划图 Direct2D 离屏渲染 → PNG（声明在文件作用域）

    std::vector<xamlplan::PowerNode> toPowerNodes(py::list nodes)
    {
        std::vector<xamlplan::PowerNode> in;
        for (auto item : nodes)
        {
            py::dict d = py::cast<py::dict>(item);
            xamlplan::PowerNode n;
            if (d.contains("code")) n.code = wideOf(py::cast<std::string>(d["code"]));
            if (d.contains("name")) n.name = wideOf(py::cast<std::string>(d["name"]));
            if (d.contains("upstream")) n.upstream = wideOf(py::cast<std::string>(d["upstream"]));
            if (d.contains("cable")) n.cable = wideOf(py::cast<std::string>(d["cable"]));
            if (d.contains("loadKw")) n.loadKw = py::cast<double>(d["loadKw"]);
            if (d.contains("voltage")) n.voltage = wideOf(py::cast<std::string>(d["voltage"]));
            if (d.contains("length")) n.length = py::cast<double>(d["length"]);
            if (d.contains("collapsedCount")) n.collapsedCount = py::cast<int>(d["collapsedCount"]);
            in.push_back(n);
        }
        return in;
    }
}

// 文档节点视图：不拥有 XamlElement；owner 持有 Document 的 Python 对象，
// 保证节点存活期间解析树不会释放（比 keep_alive 更稳，列表里也能用）
struct PyXamlNode
{
    XamlElement* el = nullptr;
    py::object owner;   // 所属 Document 的 Python 对象
};

// 文档：持有 XamlRuntime（解析结果 + 样式/模板/绑定表）
struct PyXamlDocument
{
    XamlRuntime rt;
    bool loaded = false;
};

namespace
{
    py::dict nodeToDict(const XamlElement& el)
    {
        py::dict d;
        d["tag"] = utf8Of(el.tag);
        if (!el.name.empty())
            d["name"] = utf8Of(el.name);
        if (!el.text.empty())
            d["text"] = utf8Of(el.text);
        if (!el.icon.empty())
            d["icon"] = utf8Of(el.icon);

        py::dict attrs;
        for (const auto& kv : el.attrs)
            attrs[py::str(utf8Of(kv.first))] = utf8Of(kv.second);
        d["attrs"] = attrs;

        py::list kids;
        for (const auto& child : el.children)
            kids.append(nodeToDict(child));
        d["children"] = kids;
        return d;
    }

    XamlElement* findByPath(XamlElement* root, const std::wstring& name)
    {
        if (root == nullptr)
            return nullptr;
        if (root->name == name)
            return root;
        for (auto& child : root->children)
        {
            if (XamlElement* hit = findByPath(&child, name))
                return hit;
        }
        return nullptr;
    }

    void collectByTag(XamlElement* root, const std::wstring& tag, std::vector<XamlElement*>& out)
    {
        if (root == nullptr)
            return;
        if (root->tag == tag)
            out.push_back(root);
        for (auto& child : root->children)
            collectByTag(&child, tag, out);
    }

    void collectNames(XamlElement* root, py::dict& out)
    {
        if (root == nullptr)
            return;
        if (!root->name.empty())
            out[py::str(utf8Of(root->name))] = utf8Of(root->tag);
        for (auto& child : root->children)
            collectNames(&child, out);
    }
}

void RegisterXamlBindings(py::module_& m)
{
    py::module_ xaml = m.def_submodule(
        "xaml",
        "PyMfcXaml 加载器接口：解析 XAML 文件/字符串，遍历节点树，算布局，序列化回文本。\n"
        "  不创建任何窗口或控件（建界面请用 PyMfcCore.guiload）。");

    py::class_<PyXamlNode>(xaml, "Node", "XAML 文档节点（只读视图，生命周期跟随 Document）")
        .def_property_readonly("tag", [](const PyXamlNode& n) { return utf8Of(n.el->tag); })
        .def_property_readonly("name", [](const PyXamlNode& n) { return utf8Of(n.el->name); })
        .def_property_readonly("text", [](const PyXamlNode& n) { return utf8Of(n.el->text); })
        .def_property_readonly("icon", [](const PyXamlNode& n) { return utf8Of(n.el->icon); })
        .def_property_readonly("attributes", [](const PyXamlNode& n) {
            py::dict d;
            for (const auto& kv : n.el->attrs)
                d[py::str(utf8Of(kv.first))] = utf8Of(kv.second);
            return d;
        }, "全部 XAML 属性（有序：写什么就按什么顺序出现）")
        .def("get", [](const PyXamlNode& n, const std::string& key) {
            return utf8Of(n.el->get(wideOf(key).c_str()));
        }, py::arg("key"), "取属性值，缺省返回空串（含 Style/绑定注入的有效值）")
        .def("has", [](const PyXamlNode& n, const std::string& key) {
            return n.el->has(wideOf(key).c_str());
        }, py::arg("key"))
        .def_property_readonly("children", [](py::object self) {
            const PyXamlNode& n = self.cast<const PyXamlNode&>();
            py::list out;
            for (auto& child : n.el->children)
                out.append(py::cast(new PyXamlNode{ &child, n.owner },
                                    py::return_value_policy::take_ownership));
            return out;
        }, "子节点列表（每个节点各自持有 Document 引用）")
        .def("find", [](const PyXamlNode& n, const std::string& name) -> py::object {
            if (XamlElement* hit = findByPath(n.el, wideOf(name)))
            {
                return py::cast(new PyXamlNode{ hit, n.owner },
                                py::return_value_policy::take_ownership);
            }
            return py::none();
        }, py::arg("name"), "按 x:Name/Name 找后代节点，找不到返回 None")
        .def("findTag", [](const PyXamlNode& n, const std::string& tag) {
            std::vector<XamlElement*> hits;
            collectByTag(n.el, wideOf(tag), hits);
            py::list out;
            for (XamlElement* e : hits)
                out.append(py::cast(new PyXamlNode{ e, n.owner },
                                    py::return_value_policy::take_ownership));
            return out;
        }, py::arg("tag"), "按标签名收集本子树内全部节点")
        .def_property_readonly("bounds", [](const PyXamlNode& n) {
            return py::make_tuple(n.el->previewX, n.el->previewY, n.el->previewW, n.el->previewH);
        }, "布局后的 (x, y, w, h)；未调用 Document.layout 时全为 0")
        .def_property_readonly("declared", [](const PyXamlNode& n) {
            return py::make_tuple(n.el->x, n.el->y, n.el->w, n.el->h);
        }, "XAML 里声明的 (x, y, w, h)")
        .def_property_readonly("gridPos", [](const PyXamlNode& n) {
            return py::make_tuple(n.el->row, n.el->col, n.el->rowSpan, n.el->colSpan);
        }, "(Grid.Row, Grid.Column, RowSpan, ColumnSpan)")
        .def_property_readonly("isTemplate", [](const PyXamlNode& n) { return n.el->templateChild; })
        .def("toDict", [](const PyXamlNode& n) { return nodeToDict(*n.el); },
             "整棵子树转成 dict（tag/name/text/attrs/children），便于 JSON 比对")
        .def("__repr__", [](const PyXamlNode& n) {
            std::string s = "<Node " + utf8Of(n.el->tag);
            if (!n.el->name.empty())
                s += " Name=" + utf8Of(n.el->name);
            s += " children=" + std::to_string(n.el->children.size()) + ">";
            return s;
        });

    py::class_<PyXamlDocument, std::unique_ptr<PyXamlDocument>>(xaml, "Document",
        "解析结果。文档对象存活期间其 Node 才有效（绑定用 keep_alive 保证）。")
        .def_property_readonly("root", [](py::object self) {
            const PyXamlDocument& d = self.cast<const PyXamlDocument&>();
            if (!d.loaded || d.rt.root() == nullptr)
                throw std::runtime_error("xaml: 文档未加载");
            return new PyXamlNode{ const_cast<XamlElement*>(d.rt.root()), self };
        }, py::return_value_policy::take_ownership)
        .def("layout", [](PyXamlDocument& d, int x, int y, int w, int h) {
            d.rt.layout(x, y, w, h);
        }, py::arg("x"), py::arg("y"), py::arg("width"), py::arg("height"),
           "按给定根矩形计算所有节点的 bounds")
        .def("apply", [](PyXamlDocument& d) { d.rt.applyDeclarations(); },
             "应用样式/模板/绑定（load 时已自动应用一次，改值后可重跑）")
        .def("setProperty", [](PyXamlDocument& d, const std::string& name,
                               const std::string& prop, const std::string& value) {
            return d.rt.setProperty(wideOf(name), wideOf(prop), wideOf(value));
        }, py::arg("name"), py::arg("prop"), py::arg("value"),
           "按节点 name 改属性并刷新绑定；name 传空串表示根节点")
        .def("toXml", [](PyXamlDocument& d) { return utf8Of(d.rt.toXml()); },
             "当前文档的 XAML 文本")
        .def("save", [](PyXamlDocument& d, const std::string& path) {
            std::wstring err;
            if (!d.rt.save(wideOf(path).c_str(), err))
                throw std::runtime_error("xaml.save: " + utf8Of(err));
        }, py::arg("path"))
        .def("names", [](PyXamlDocument& d) {
            py::dict out;
            collectNames(d.rt.root(), out);
            return out;
        }, "全文档 name -> tag 映射")
        .def("styles", [](PyXamlDocument& d) {
            py::list out;
            for (const auto& style : d.rt.styles())
            {
                py::dict item;
                item["key"] = utf8Of(style.key);
                item["targetType"] = utf8Of(style.targetType);
                item["basedOn"] = utf8Of(style.basedOn);
                py::list setters;
                for (const auto& s : style.setters)
                    setters.append(py::make_tuple(utf8Of(s.prop), utf8Of(s.value)));
                item["setters"] = setters;
                item["triggers"] = static_cast<int>(style.triggers.size());
                out.append(item);
            }
            return out;
        }, "加载时采集到的样式表")
        .def("bindings", [](PyXamlDocument& d) {
            py::list out;
            for (const auto& b : d.rt.bindings())
            {
                py::dict item;
                item["target"] = utf8Of(b.targetName);
                item["prop"] = utf8Of(b.targetProp);
                item["source"] = utf8Of(b.sourceName);
                item["sourceProp"] = utf8Of(b.sourceProp);
                item["twoWay"] = b.twoWay;
                out.append(item);
            }
            return out;
        }, "加载时采集到的绑定表")
        .def("templateKeys", [](PyXamlDocument& d) {
            std::vector<std::wstring> keys = d.rt.templateKeys();
            py::list out;
            for (const auto& k : keys)
                out.append(utf8Of(k));
            return out;
        }, "控件模板键（含 ResourceDictionary 跨文件合并进来的）");

    xaml.def("load", [](const std::string& path) {
        auto doc = std::make_unique<PyXamlDocument>();
        std::wstring err;
        if (!doc->rt.loadFile(wideOf(path).c_str(), err))
            throw std::runtime_error("xaml.load: " + utf8Of(err));
        doc->loaded = true;
        return doc;
    }, py::arg("path"), "解析 XAML 文件（UTF-8），返回 Document；失败抛 RuntimeError（含解析器原始原因）");

    xaml.def("loadString", [](const std::string& text) {
        auto doc = std::make_unique<PyXamlDocument>();
        std::wstring err;
        if (!doc->rt.loadString(wideOf(text).c_str(), err))
            throw std::runtime_error("xaml.loadString: " + utf8Of(err));
        doc->loaded = true;
        return doc;
    }, py::arg("text"), "解析内存中的 XAML 文本");

    xaml.def("tags", []() {
        std::vector<std::wstring> tags = XamlControlRegistry::instance().tags();
        py::list out;
        for (const auto& t : tags)
            out.append(utf8Of(t));
        return out;
    }, "当前已注册的控件标签（PyMfcCore 导入后为全部标准 MFC 控件）");

    xaml.def("defaultSize", [](const std::string& tag) -> py::object {
        int w = 0, h = 0;
        if (!xamlGetDefaultSize(wideOf(tag).c_str(), w, h))
            return py::none();
        return py::make_tuple(w, h);
    }, py::arg("tag"), "控件缺省尺寸 (w, h)，来自 control_catalog.xml；无则 None");

    xaml.def("parseColor", [](const std::string& text) -> py::object {
        COLORREF c = 0;
        if (!xamlParseColor(wideOf(text), c))
            return py::none();
        return py::int_(static_cast<unsigned long>(c));
    }, py::arg("text"), "#RRGGBB 或 RGB(r,g,b) → COLORREF 整数；失败返回 None");

    xaml.def("colorToString", [](unsigned long colorRef) {
        return utf8Of(xamlColorToString(static_cast<COLORREF>(colorRef)));
    }, py::arg("colorref"), "COLORREF → '#RRGGBB'");

    // ---------------- 配电规划图（XamlPowerPlan） ----------------
    py::module_ pp = m.def_submodule(
        "powerplan",
        "配电规划图：建树 + 自动分层布局 + XAML/JSON/CSV 出口。纯数据层，不出窗口。");

    py::class_<xamlplan::PowerPlanStyle>(pp, "Style", "布局尺寸参数")
        .def(py::init<>())
        .def_readwrite("boxW", &xamlplan::PowerPlanStyle::boxW)
        .def_readwrite("boxH", &xamlplan::PowerPlanStyle::boxH)
        .def_readwrite("termW", &xamlplan::PowerPlanStyle::termW)
        .def_readwrite("termH", &xamlplan::PowerPlanStyle::termH)
        .def_readwrite("nameH", &xamlplan::PowerPlanStyle::nameH)
        .def_readwrite("gapX", &xamlplan::PowerPlanStyle::gapX)
        .def_readwrite("gapY", &xamlplan::PowerPlanStyle::gapY)
        .def_readwrite("margin", &xamlplan::PowerPlanStyle::margin)
        .def_readwrite("rootGap", &xamlplan::PowerPlanStyle::rootGap)
        .def_readwrite("colGap", &xamlplan::PowerPlanStyle::colGap);

    pp.def("build", [](py::list nodes, const xamlplan::PowerPlanStyle& style,
                       const std::string& title, bool horizontal) {
        std::vector<xamlplan::PowerNode> in = toPowerNodes(nodes);

        xamlplan::PowerPlan plan;
        std::wstring err;
        if (!xamlplan::buildPlan(in, plan, style, err, horizontal))
            throw std::runtime_error("powerplan.build: " + utf8Of(err));

        py::dict out;
        out["xaml"] = utf8Of(xamlplan::toXaml(plan, wideOf(title)));
        out["json"] = utf8Of(xamlplan::toJson(plan));
        out["csv"] = utf8Of(xamlplan::toCsv(in));
        out["svg"] = utf8Of(xamlplan::toSvg(plan, wideOf(title), true));
        out["width"] = plan.width;
        out["height"] = plan.height;
        out["dir"] = utf8Of(plan.dir);
        out["crossings"] = xamlplan::countCrossings(plan);
        py::list warn;
        for (const auto& w : plan.warnings) warn.append(utf8Of(w));
        out["warnings"] = warn;
        py::list items;
        for (const auto& n : plan.nodes)
        {
            py::dict item;
            item["code"] = utf8Of(n.code);
            item["name"] = utf8Of(n.name);
            item["upstream"] = utf8Of(n.upstream);
            item["cable"] = utf8Of(n.cable);
            item["loadKw"] = n.loadKw;
            item["voltage"] = utf8Of(n.voltage);
            item["length"] = n.length;
            item["role"] = utf8Of(n.role);
            item["collapsedCount"] = n.collapsedCount;
            items.append(item);
        }
        out["nodes"] = items;
        py::list boxes;
        for (const auto& b : plan.boxes)
        {
            py::dict item;
            item["code"] = utf8Of(b.code);
            item["x"] = b.x; item["y"] = b.y; item["w"] = b.w; item["h"] = b.h;
            item["depth"] = b.depth;
            boxes.append(item);
        }
        out["boxes"] = boxes;
        py::list links;
        for (const auto& l : plan.links)
        {
            py::dict item;
            item["from"] = utf8Of(l.from); item["to"] = utf8Of(l.to);
            item["cable"] = utf8Of(l.cable);
            item["x1"] = l.x1; item["y1"] = l.y1; item["x2"] = l.x2; item["y2"] = l.y2;
            links.append(item);
        }
        out["links"] = links;
        return out;
    }, py::arg("nodes"), py::arg("style") = xamlplan::PowerPlanStyle(),
       py::arg("title") = std::string(), py::arg("horizontal") = false,
       "nodes=[{code,name,upstream,cable,loadKw,voltage,length,collapsedCount}] → "
       "{xaml,json,csv,svg,width,height,dir,crossings,warnings,nodes,boxes,links}");

    // 导出走"场景"而不是"重排"：屏幕坐标由 Python 引擎（engine.layout）算好放进 scene，
    // 导出的 JSON/CSV/SVG/PNG 必须与屏上完全一致，所以这里直接吃 scene，不再 buildPlan。
    pp.def("fromScene", [](const std::string& scene, const std::string& title, bool withTitleBlock) {
        xamlplan::PowerPlan plan;
        std::wstring err;
        if (!xamlplan::parseScene(wideOf(scene), plan, err))
            throw std::runtime_error("powerplan.fromScene: " + utf8Of(err));
        py::dict out;
        out["json"] = utf8Of(xamlplan::toJson(plan));
        out["csv"] = utf8Of(xamlplan::toCsv(plan.nodes));
        out["svg"] = utf8Of(xamlplan::toSvg(plan, wideOf(title), withTitleBlock));
        out["width"] = plan.width;
        out["height"] = plan.height;
        out["dir"] = utf8Of(plan.dir);
        out["crossings"] = xamlplan::countCrossings(plan);
        return out;
    }, py::arg("scene"), py::arg("title") = std::string(), py::arg("withTitleBlock") = false,
       "按场景 JSON（engine.scene）导出 json/csv/svg：坐标与屏上一致，不重排");

    pp.def("renderScenePng", [](const std::string& path, const std::string& scene, int scale) {
        xamlplan::PowerPlan plan;
        std::wstring err;
        if (!xamlplan::parseScene(wideOf(scene), plan, err))
            throw std::runtime_error("powerplan.renderScenePng: " + utf8Of(err));
        const int s = scale < 1 ? 1 : scale;
        if (!RenderPowerPlanPng(wideOf(path), plan, s))
            throw std::runtime_error("powerplan.renderScenePng: 写 PNG 失败");
        return py::make_tuple(plan.width * s, plan.height * s);
    }, py::arg("path"), py::arg("scene"), py::arg("scale") = 2,
       "按场景 JSON 离屏渲染 PNG（只按坐标画，不重排）；返回 (宽, 高)");

    pp.def("toSvg", [](py::list nodes, const xamlplan::PowerPlanStyle& style,
                       const std::string& title, bool horizontal, bool withTitleBlock) {
        std::vector<xamlplan::PowerNode> in = toPowerNodes(nodes);
        xamlplan::PowerPlan plan;
        std::wstring err;
        if (!xamlplan::buildPlan(in, plan, style, err, horizontal))
            throw std::runtime_error("powerplan.toSvg: " + utf8Of(err));
        return utf8Of(xamlplan::toSvg(plan, wideOf(title), withTitleBlock));
    }, py::arg("nodes"), py::arg("style") = xamlplan::PowerPlanStyle(),
       py::arg("title") = std::string(), py::arg("horizontal") = false,
       py::arg("withTitleBlock") = true, "布局并返回 SVG 文本（图框标题栏可选）");

    pp.def("parseCsv", [](const std::string& text) {
        std::vector<xamlplan::PowerNode> nodes;
        std::wstring title, err;
        if (!xamlplan::parseCsv(wideOf(text), nodes, title, err))
            throw std::runtime_error("powerplan.parseCsv: " + utf8Of(err));
        py::dict out;
        py::list items;
        for (const auto& n : nodes)
        {
            py::dict item;
            item["code"] = utf8Of(n.code);
            item["name"] = utf8Of(n.name);
            item["upstream"] = utf8Of(n.upstream);
            item["cable"] = utf8Of(n.cable);
            item["loadKw"] = n.loadKw;
            item["voltage"] = utf8Of(n.voltage);
            item["length"] = n.length;
            items.append(item);
        }
        out["nodes"] = items;
        out["title"] = utf8Of(title);
        return out;
    }, py::arg("text"), "CSV 文本 → {nodes:[...], title}");

    pp.def("parseJson", [](const std::string& text) {
        std::vector<xamlplan::PowerNode> nodes;
        std::wstring title, err;
        if (!xamlplan::parseJson(wideOf(text), nodes, title, err))
            throw std::runtime_error("powerplan.parseJson: " + utf8Of(err));
        py::dict out;
        py::list items;
        for (const auto& n : nodes)
        {
            py::dict item;
            item["code"] = utf8Of(n.code);
            item["name"] = utf8Of(n.name);
            item["upstream"] = utf8Of(n.upstream);
            item["cable"] = utf8Of(n.cable);
            item["loadKw"] = n.loadKw;
            item["voltage"] = utf8Of(n.voltage);
            item["length"] = n.length;
            items.append(item);
        }
        out["nodes"] = items;
        out["title"] = utf8Of(title);
        return out;
    }, py::arg("text"), "JSON 文本 → {nodes:[...], title}");

    pp.def("renderPng", [](const std::string& path, py::list nodes,
                          const xamlplan::PowerPlanStyle& style, int scale, bool horizontal) {
        std::vector<xamlplan::PowerNode> in = toPowerNodes(nodes);
        xamlplan::PowerPlan plan;
        std::wstring err;
        if (!xamlplan::buildPlan(in, plan, style, err, horizontal))
            throw std::runtime_error("powerplan.renderPng: " + utf8Of(err));
        if (!RenderPowerPlanPng(wideOf(path), plan, scale))
            throw std::runtime_error("powerplan.renderPng: 写 PNG 失败");
        return py::make_tuple(plan.width * (scale < 1 ? 1 : scale),
                              plan.height * (scale < 1 ? 1 : scale));
    }, py::arg("path"), py::arg("nodes"), py::arg("style") = xamlplan::PowerPlanStyle(),
       py::arg("scale") = 1, py::arg("horizontal") = false,
       "用 Direct2D + DirectWrite 把配电图离屏渲染成 PNG（不建窗口），返回 (宽, 高)");

    // SVG 矢量图（nanosvg 解析，本栈 D2D 画）
    py::module_ sv = m.def_submodule("svg", "SVG 矢量图导入：nanosvg 解析几何 + 本栈 D2D 绘制");

    sv.def("parse", [](const std::string& path) {
        xamlsvg::SvgScene scene;
        std::wstring err;
        if (!xamlsvg::loadSvgFile(wideOf(path), scene, err))
            throw std::runtime_error("svg.parse: " + utf8Of(err));
        py::dict out;
        out["width"] = scene.width;
        out["height"] = scene.height;
        out["shapes"] = static_cast<int>(scene.shapes.size());
        out["paths"] = scene.pathCount;
        py::list warns;
        for (const std::wstring& w : scene.warnings)
            warns.append(utf8Of(w));
        out["warnings"] = warns;
        return out;
    }, py::arg("path"), "解析 SVG 文件 → {width, height, shapes, paths, warnings}");

    sv.def("renderPng", [](const std::string& svgPath, const std::string& pngPath,
                           int scale, const std::string& bg) {
        // 背景色：#RRGGBB / #RGB / 不带 #；空串表示透明（目前按白底处理，PNG 是 32bpp）
        int r = 0xFF, g = 0xFF, b = 0xFF;
        std::string hex = bg;
        if (!hex.empty() && hex[0] == '#') hex.erase(0, 1);
        if (hex.size() == 6)
        {
            r = std::stoi(hex.substr(0, 2), nullptr, 16);
            g = std::stoi(hex.substr(2, 2), nullptr, 16);
            b = std::stoi(hex.substr(4, 2), nullptr, 16);
        }
        int w = 0, h = 0;
        if (!RenderSvgPng(wideOf(svgPath), wideOf(pngPath), scale, RGB(r, g, b), w, h))
            throw std::runtime_error("svg.renderPng: 渲染失败（SVG 解析或写 PNG 失败）");
        return py::make_tuple(w, h);
    }, py::arg("svg"), py::arg("png"), py::arg("scale") = 1, py::arg("bg") = "#FFFFFF",
       "把 SVG 渲染成 PNG（不建窗口），返回实际像素 (宽, 高)");
}
