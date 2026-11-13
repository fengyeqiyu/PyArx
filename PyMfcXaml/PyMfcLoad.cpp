// PyMfcLoad.cpp —— guiload：XAML 文件 → 控件树（布局运行时 + 控件工厂）。
// 原名 parts\05-load.inc；三个对外入口（Guiload / XamlLayoutRects / RegisterPythonControl）
// 已移出匿名 namespace 成为外部链接函数，声明见 PyMfcParts.h。

#include "PyMfcCommon.h"
#include "PyMfcWrap.h"
#include "PyMfcParts.h"

// guiload：XML → 控件树（XAML 布局运行时）  （从 PyMfcXaml.cpp 按类/职责切出，行 9505-11089）
// ---------------------------------------------------------------------------
// 标准 MFC 控件工厂：把 XAML 控件名映射到 PyMfcHost 的 Win32 控件实现。
// 实现 IXamlControl 接口，注册进 XamlControlRegistry —— 这样 guiload 与设计器
// 都通过统一运行时 + 注册表创建控件，不再各自硬编码 switch。
//
// 生命周期与 PyMfc 原则一致：宿主（PyMfcHost）持有子控件 HWND；本对象只持
// 有 host 指针与 id，不拥有/不删除原生控件。create/destroy 只做窗口创建/销毁。
// ---------------------------------------------------------------------------
namespace
{
    // 标准控件标签 -> 短名（与 guiload 的 TagKind 一致）
    std::string StdTagKind(const std::string& raw)
    {
        std::string tag = raw;
        const size_t brace = tag.rfind('}');
        if (brace != std::string::npos)
            tag = tag.substr(brace + 1);
        for (char& c : tag)
            c = (char)tolower((unsigned char)c);
        const size_t dot = tag.rfind('.');
        if (dot != std::string::npos)
            tag = tag.substr(dot + 1);

        if (tag == "textblock") return "label";
        if (tag == "textbox") return "edit";
        if (tag == "radiobutton") return "radio";
        if (tag == "groupbox") return "group";
        if (tag == "paletteset") return "palette";   // 侧栏根节点
        if (tag == "tabitem") return "page";         // <TabItem> 等同 <page>
        // 布局正名：Layout / AbsolutePanel（Canvas 只是它的历史别名）
        if (tag == "layout" || tag == "layoutpanel" || tag == "absolutepanel" ||
            tag == "abspanel" || tag == "canvaspanel" || tag == "canvaslayout")
            return "canvas";
        return tag;  // button/checkbox/combo/listbox/progress/slider 原样
    }

    bool StdIsKnown(const std::string& tag)
    {
        return tag == "label" || tag == "button" || tag == "edit" || tag == "listbox" ||
               tag == "checkbox" || tag == "radio" || tag == "group" || tag == "combo" ||
               tag == "progress" || tag == "slider";
    }

    // 字符串大小写不敏感比较（窄字符）
    bool StrEq(const std::string& a, const std::string& b)
    {
        if (a.size() != b.size())
            return false;
        for (size_t i = 0; i < a.size(); ++i)
        {
            if (tolower((unsigned char)a[i]) != tolower((unsigned char)b[i]))
                return false;
        }
        return true;
    }

    std::string Utf8Of(const std::wstring& w)
    {
        const int len = ::WideCharToMultiByte(CP_UTF8, 0, w.c_str(), -1, nullptr, 0, nullptr, nullptr);
        if (len <= 0)
            return {};
        std::string a(static_cast<size_t>(len), '\0');
        ::WideCharToMultiByte(CP_UTF8, 0, w.c_str(), -1, a.data(), len, nullptr, nullptr);
        if (!a.empty() && a.back() == '\0')
            a.pop_back();
        return a;
    }

    std::wstring Utf8ToWide(const std::string& s)
    {
        const int len = ::MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, nullptr, 0);
        if (len <= 0)
            return {};
        std::wstring w(static_cast<size_t>(len), L'\0');
        ::MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, w.data(), len);
        if (!w.empty() && w.back() == L'\0')
            w.pop_back();
        return w;
    }

    std::wstring Utf8ToWideSafe(const std::string& s)
    {
        return Utf8ToWide(s);
    }
}


// ---------------------------------------------------------------------------
// XML 界面加载：PyMfcCore.guiload(path, target=None, context=None, palette=...)
//
// XML 由 Python 标准库 xml.etree.ElementTree 解析（不引入额外 C++ XML 依赖），
// 控件创建全部复用上面已验证的原生接口，因此生命周期规则完全一致：
// 对象归 PyMfcStore、Python 析构不 delete、ID 仍走 1000..65000 校验。
//
// 事件：节点上写 on_click / on_change / on_select = "函数名"，
// 加载时到 context（缺省为调用方 globals）里按名字取函数，取不到直接报错。
// ---------------------------------------------------------------------------
namespace
{
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


    constexpr int kFlowMargin = 12;
    constexpr int kFlowSpacing = 8;

    int AttrInt(const py::dict& attrs, const char* key, int fallback)
    {
        if (!attrs.contains(key))
            return fallback;
        try
        {
            // XML 属性值一律是字符串，pybind11 的 cast<int> 不做隐式转换，必须先转 string
            py::object value = attrs[key];
            if (py::isinstance<py::str>(value))
                return std::stoi(value.cast<std::string>());
            return value.cast<int>();
        }
        catch (...)
        {
            PyErr_Clear();
            return fallback;
        }
    }

    std::wstring AttrWString(const py::dict& attrs, const char* key)
    {
        if (!attrs.contains(key))
            return {};
        try
        {
            return attrs[key].cast<std::wstring>();
        }
        catch (...)
        {
            PyErr_Clear();
            return {};
        }
    }

    std::string AttrString(const py::dict& attrs, const char* key)
    {
        if (!attrs.contains(key))
            return {};
        try
        {
            return attrs[key].cast<std::string>();
        }
        catch (...)
        {
            PyErr_Clear();
            return {};
        }
    }

    // guiload 没显式传 context 时用调用方的模块 globals
    // （C++ 函数不产生 Python 帧，_getframe(0) 就是调用 guiload 的那一帧）
    py::dict CallerGlobals()
    {
        try
        {
            py::object sys = py::module_::import("sys");
            py::object frame = sys.attr("_getframe")(0);
            py::object globals = frame.attr("f_globals");
            if (py::isinstance<py::dict>(globals))
                return globals.cast<py::dict>();
        }
        catch (...)
        {
            PyErr_Clear();
        }
        try
        {
            return py::module_::import("__main__").attr("__dict__").cast<py::dict>();
        }
        catch (...)
        {
            PyErr_Clear();
            return py::dict();
        }
    }

    // ---- 属性名匹配：去掉 XML 命名空间前缀 + 大小写不敏感 ----
    // 这样 x:Name（{uri}Name）、Name、name 都能命中同一条别名规则。
    std::string LocalName(const std::string& key)
    {
        const size_t pos = key.rfind('}');
        return pos == std::string::npos ? key : key.substr(pos + 1);
    }

    bool NameMatch(const std::string& a, const std::string& b)
    {
        if (a.size() != b.size())
            return false;
        for (size_t i = 0; i < a.size(); ++i)
        {
            if (tolower((unsigned char)a[i]) != tolower((unsigned char)b[i]))
                return false;
        }
        return true;
    }

    // 按别名列表取属性值；找不到返回空对象
    py::object AttrOf(const py::dict& attrs, std::initializer_list<const char*> aliases)
    {
        for (const char* alias : aliases)
        {
            if (attrs.contains(alias))
                return attrs[alias];
        }
        // pybind11 的 dict 迭代器产出 std::pair<key, value>
        for (const auto& item : attrs)
        {
            py::handle key = item.first;
            if (!py::isinstance<py::str>(key))
                continue;
            const std::string name = LocalName(key.cast<std::string>());
            for (const char* alias : aliases)
            {
                if (NameMatch(name, alias))
                    return py::reinterpret_borrow<py::object>(item.second);
            }
        }
        return py::object();
    }

    bool AttrHas(const py::dict& attrs, std::initializer_list<const char*> aliases)
    {
        return static_cast<bool>(AttrOf(attrs, aliases));
    }

    int AttrIntL(const py::dict& attrs, std::initializer_list<const char*> aliases, int fallback)
    {
        py::object value = AttrOf(attrs, aliases);
        if (!value)
            return fallback;
        try
        {
            // XML 属性值一律是字符串，pybind11 的 cast<int> 不做隐式转换，必须先转 string
            if (py::isinstance<py::str>(value))
            {
                const std::string text = value.cast<std::string>();
                if (text.empty())
                    return fallback;
                return std::stoi(text);
            }
            return value.cast<int>();
        }
        catch (...)
        {
            PyErr_Clear();
            return fallback;
        }
    }

    std::wstring AttrWStringL(const py::dict& attrs, std::initializer_list<const char*> aliases)
    {
        py::object value = AttrOf(attrs, aliases);
        if (!value)
            return {};
        try
        {
            return value.cast<std::wstring>();
        }
        catch (...)
        {
            PyErr_Clear();
            return {};
        }
    }

    std::string AttrStringL(const py::dict& attrs, std::initializer_list<const char*> aliases,
                            const std::string& fallback = {})
    {
        py::object value = AttrOf(attrs, aliases);
        if (!value)
            return fallback;
        try
        {
            return value.cast<std::string>();
        }
        catch (...)
        {
            PyErr_Clear();
            return fallback;
        }
    }

    // 标签只接受 XAML 控件名（大小写不敏感，允许带命名空间前缀），映射到内部短名。
    // 未列出的标签一律视为未知控件并报错，避免拼错名字后被静默忽略。
    std::string TagKind(const std::string& rawTag)
    {
        std::string tag = LocalName(rawTag);
        for (char& c : tag)
            c = (char)tolower((unsigned char)c);

        // <Grid.RowDefinitions> 这类带点前缀的标签，取最后一段
        const size_t dot = tag.rfind('.');
        if (dot != std::string::npos)
            tag = tag.substr(dot + 1);

        if (tag == "textblock") return "label";
        if (tag == "button") return "button";
        if (tag == "textbox") return "edit";
        if (tag == "listbox") return "listbox";
        if (tag == "checkbox") return "checkbox";
        if (tag == "radiobutton") return "radio";
        if (tag == "groupbox") return "group";
        if (tag == "combobox") return "combo";
        if (tag == "progressbar") return "progress";
        if (tag == "slider") return "slider";
        if (tag == "stackpanel") return "stackpanel";
        if (tag == "grid") return "grid";
        if (tag == "dockpanel") return "dockpanel";
        if (tag == "wrappanel") return "wrappanel";
        if (tag == "canvas") return "canvas";
        // 布局正名：Layout / AbsolutePanel（Canvas 只是兼容别名）
        if (tag == "layout" || tag == "layoutpanel" || tag == "absolutepanel" ||
            tag == "abspanel" || tag == "canvaspanel" ||
            tag == "canvaslayout") return "canvas";
        if (tag == "window") return "window";
        if (tag == "paletteset") return "palette";
        if (tag == "tabitem") return "page";
        return tag;  // 未知，调用方报错
    }

    int DefaultHeight(const std::string& tag)
    {
        if (tag == "label") return 24;
        if (tag == "button") return 32;
        if (tag == "edit") return 28;
        if (tag == "checkbox" || tag == "radio") return 24;
        if (tag == "group") return 80;
        if (tag == "listbox") return 120;
        if (tag == "combo") return 120;
        if (tag == "progress") return 20;
        if (tag == "slider") return 32;
        return 24;
    }

    bool IsKnownControl(const std::string& tag)
    {
        return tag == "label" || tag == "button" || tag == "edit" || tag == "listbox" ||
               tag == "checkbox" || tag == "radio" || tag == "group" || tag == "combo" ||
               tag == "progress" || tag == "slider";
    }

    struct GuiFlow
    {
        int margin = kFlowMargin;
        int spacing = kFlowSpacing;
        int cursorY = kFlowMargin;  // 相对 offsetY 的游标
        int cursorX = kFlowMargin;  // 横向排列时的游标
        int width = 240;            // 容器内容宽度，用于子控件默认 w
        int offsetX = 0;            // 容器内区左上角（嵌套时由父容器累加）
        int offsetY = 0;
        bool horizontal = false;    // StackPanel Orientation="Horizontal"
        int* nextId = nullptr;      // x:Name 自动分配 ID 的计数器（跨容器共享）
        py::dict* names = nullptr;  // x:Name -> id 映射（可选，回传给 Python）
        int height = 0;             // 容器可用高度（Grid/StackPanel 用）
        int padding = 0;            // 容器内边距（Grid/StackPanel）
        bool grid = false;          // 当前控件集合位于 Grid 单元格内
        std::vector<int> gridRows;  // 行高（已解析）
        std::vector<int> gridCols;  // 列宽（已解析）
    };

    // ---- WPF 风格 Grid 支持 ------------------------------------------------
    struct GridTrack
    {
        int kind = 2;   // 0=fixed, 1=auto, 2=star
        int value = 1;  // fixed 像素 / star 权重
    };

    std::string LocalLower(py::handle element)
    {
        std::string tag = element.attr("tag").cast<std::string>();
        tag = LocalName(tag);
        for (char& c : tag)
            c = (char)tolower((unsigned char)c);
        return tag;
    }

    bool IsContainerTag(const std::string& tag)
    {
        return tag == "stackpanel" || tag == "grid" || tag == "dockpanel" ||
               tag == "wrappanel" || tag == "canvas";
    }

    std::vector<GridTrack> ParseGridDefs(py::handle node, bool rows)
    {
        std::vector<GridTrack> result;
        const std::string defsTag = rows ? "rowdefinitions" : "columndefinitions";
        const std::string itemTag = rows ? "rowdefinition" : "columndefinition";
        const char* sizeAttr = rows ? "Height" : "Width";

        for (py::handle child : py::reinterpret_borrow<py::iterable>(node))
        {
            if (!py::isinstance<py::str>(child.attr("tag")))
                continue;
            if (LocalLower(child) != defsTag)
                continue;

            for (py::handle def : py::reinterpret_borrow<py::iterable>(child))
            {
                if (!py::isinstance<py::str>(def.attr("tag")))
                    continue;
                if (LocalLower(def) != itemTag)
                    continue;

                const py::dict attrs = def.attr("attrib").cast<py::dict>();
                std::string text = AttrStringL(attrs, {sizeAttr}, "Auto");
                for (char& c : text)
                    c = (char)tolower((unsigned char)c);

                GridTrack track;
                if (text == "auto")
                {
                    track.kind = 1;
                    track.value = 0;
                }
                else if (!text.empty() && text.back() == '*')
                {
                    track.kind = 2;
                    std::string weight = text.substr(0, text.size() - 1);
                    track.value = weight.empty() ? 1 : (std::max)(1, std::atoi(weight.c_str()));
                }
                else
                {
                    track.kind = 0;
                    track.value = (std::max)(0, std::atoi(text.c_str()));
                }
                result.push_back(track);
            }
            break;
        }
        return result;
    }

    int GridChildIndex(const py::dict& attrs, bool rows)
    {
        return AttrIntL(attrs, rows ? std::initializer_list<const char*>{"Grid.Row", "Row"}
                                    : std::initializer_list<const char*>{"Grid.Column", "Column"},
                        0);
    }

    int GridChildSpan(const py::dict& attrs, bool rows)
    {
        return (std::max)(1, AttrIntL(attrs,
            rows ? std::initializer_list<const char*>{"Grid.RowSpan", "RowSpan"}
                 : std::initializer_list<const char*>{"Grid.ColumnSpan", "ColumnSpan"},
            1));
    }

    std::vector<int> ResolveGridTracks(py::handle node, bool rows, int available)
    {
        std::vector<GridTrack> tracks = ParseGridDefs(node, rows);
        if (tracks.empty())
            tracks.push_back(GridTrack{2, 1});

        int fixedSum = 0;
        int autoSum = 0;
        int starWeight = 0;

        for (size_t i = 0; i < tracks.size(); ++i)
        {
            GridTrack& t = tracks[i];
            if (t.kind == 0)
            {
                fixedSum += t.value;
            }
            else if (t.kind == 1)
            {
                int need = rows ? 24 : 100;
                for (py::handle child : py::reinterpret_borrow<py::iterable>(node))
                {
                    if (!py::isinstance<py::str>(child.attr("tag")))
                        continue;
                    const std::string tag = TagKind(child.attr("tag").cast<std::string>());
                    if (!IsKnownControl(tag) && !IsContainerTag(tag))
                        continue;
                    const py::dict attrs = child.attr("attrib").cast<py::dict>();
                    if (GridChildIndex(attrs, rows) != (int)i)
                        continue;
                    const int desired = AttrIntL(attrs,
                        rows ? std::initializer_list<const char*>{"Height"}
                             : std::initializer_list<const char*>{"Width"},
                        rows ? DefaultHeight(tag) : 100);
                    need = (std::max)(need, desired);
                }
                t.value = need;
                autoSum += need;
            }
            else
            {
                starWeight += (std::max)(1, t.value);
            }
        }

        const int remaining = (std::max)(0, available - fixedSum - autoSum);
        int starUnit = starWeight > 0 ? remaining / starWeight : 0;
        if (starUnit <= 0)
            starUnit = 30;

        std::vector<int> sizes;
        sizes.reserve(tracks.size());
        for (const GridTrack& t : tracks)
        {
            if (t.kind == 0)
                sizes.push_back(t.value);
            else if (t.kind == 1)
                sizes.push_back(t.value);
            else
                sizes.push_back((std::max)(1, starUnit * (std::max)(1, t.value)));
        }
        return sizes;
    }

    int SumTracks(const std::vector<int>& tracks, int start, int count)
    {
        int total = 0;
        for (int i = start; i < start + count && i < (int)tracks.size(); ++i)
            total += tracks[i];
        return total;
    }

    void ParseThickness(const std::string& text, int& left, int& top, int& right, int& bottom)
    {
        left = top = right = bottom = 0;
        if (text.empty())
            return;

        std::vector<int> values;
        std::string current;
        for (char c : text)
        {
            if (c == ',' || c == ' ')
            {
                if (!current.empty())
                {
                    values.push_back(std::atoi(current.c_str()));
                    current.clear();
                }
            }
            else
            {
                current.push_back(c);
            }
        }
        if (!current.empty())
            values.push_back(std::atoi(current.c_str()));

        if (values.size() == 1)
            left = top = right = bottom = values[0];
        else if (values.size() == 2)
        {
            left = right = values[0];
            top = bottom = values[1];
        }
        else if (values.size() >= 4)
        {
            left = values[0];
            top = values[1];
            right = values[2];
            bottom = values[3];
        }
    }


    // 回调名 -> 函数对象；找不到就报错，绝不静默忽略
    py::object ResolveCallback(const py::dict& context, const py::dict& attrs,
                               std::initializer_list<const char*> aliases, const char* idText)
    {
        const std::string name = AttrStringL(attrs, aliases);
        if (name.empty())
            return py::object();
        if (!context.contains(name.c_str()))
        {
            throw std::runtime_error(std::string("guiload: 控件 id=") + idText +
                                     " 引用的回调 \"" + name + "\" 不存在（未在 context 中找到）");
        }
        return context[name.c_str()];
    }

    // ---- 事件绑定（S2）：一张表统一分派，加载器不再只认 Click/TextChanged/SelectionChanged
    // 有专门通道的事件走对应注册口，其余（鼠标/键盘/焦点）走 onControlEvent；
    // hostDispatch=false（当前只有 Scroll）直接告警，不再静默失效。
    void BindHostEvent(PyMfcWindow* window, PyMfcPanel* page, int id,
                       const XamlEventInfo& info, py::object cb)
    {
        auto apply = [&](auto* target) {
            switch (info.kind)
            {
            case XamlEventKind::Click:            target->onClick(id, std::move(cb)); break;
            case XamlEventKind::DoubleClick:      target->onDoubleClick(id, std::move(cb)); break;
            case XamlEventKind::TextChanged:      target->onChange(id, std::move(cb)); break;
            case XamlEventKind::SelectionChanged: target->onSelect(id, std::move(cb)); break;
            case XamlEventKind::ValueChanged:     target->onValue(id, std::move(cb)); break;
            case XamlEventKind::CheckedChanged:   target->onChecked(id, std::move(cb)); break;
            case XamlEventKind::DropDown:         target->onDropDown(id, std::move(cb)); break;
            default:
                target->onControlEvent(id, std::wstring(info.key), std::move(cb));
                break;
            }
        };
        if (window != nullptr)
            apply(window);
        else
            apply(page);
    }

    // 事件属性名的窄字符版本（XML 路径按 py::dict 键查值，键是 std::string）
    const std::vector<std::string>& NarrowEventProps()
    {
        static const std::vector<std::string> kNames = []() {
            std::vector<std::string> v;
            for (const XamlEventInfo& info : xamlEventInfos())
                v.push_back(Utf8Of(info.property));
            return v;
        }();
        return kNames;
    }


    // ---- 统一运行时路径：遍历 XamlElement 树，控件经注册表创建 ----
    // 每个节点都已被 XamlRuntime::Layout 算出 previewX/Y/W/H，容器/Grid/GroupBox
    // 的递归定位已在布局阶段完成，这里只负责"创建控件 + 属性 + 事件 + 映射"。
    // 事件回调名到 context 全局函数的解析逻辑与原 LoadControlsInto 完全一致。

    // 元素的有效外观（本地声明优先，其余取样式/绑定注入的有效层）。
    // 容器不建原生控件，只能靠这份外观由宿主铺底。
    xamlapp::XamlAppearance AppearanceOf(const XamlElement& el)
    {
        xamlapp::XamlAppearance ap;
        for (const auto& kv : el.attrs)
            xamlapp::SetProperty(ap, kv.first, kv.second);
        for (const auto& kv : el.styledAttrs)
        {
            bool local = false;
            for (const auto& a : el.attrs)
            {
                if (_wcsicmp(a.first.c_str(), kv.first.c_str()) == 0)
                {
                    local = true;
                    break;
                }
            }
            if (!local)
                xamlapp::SetProperty(ap, kv.first, kv.second);
        }
        return ap;
    }

    int AttrIntOf(const XamlElement& el, const wchar_t* key, int fallback)
    {
        const std::wstring v = el.get(key);
        if (v.empty())
            return fallback;
        try
        {
            return std::stoi(v);
        }
        catch (...)
        {
            return fallback;
        }
    }

    std::wstring AttrWOf(const XamlElement& el, const wchar_t* key)
    {
        return el.get(key);
    }

    void LoadControlsFromElement(XamlElement& node, PyMfcWindow* window, PyMfcPanel* page,
                                 const py::dict& context, int* nextId, py::dict* names)
    {
        for (auto& child : node.children)
        {
            const std::string rawTag = Utf8Of(child.tag);
            const std::string tag = StdTagKind(rawTag);
            if (tag == "page")
                continue;  // <page> 由 palette 分支处理
            if (tag == "rowdefinitions" || tag == "columndefinitions" ||
                tag == "resources" || tag == "style")
                continue;  // 布局/资源定义，不是控件

            // 容器：不创建原生控件，递归子树（坐标已在布局阶段算好）
            if (tag == "stackpanel" || tag == "grid" || tag == "dockpanel" ||
                tag == "wrappanel" || tag == "canvas" || tag == "toolbar" ||
                tag == "menubar" || tag == "statusbar")
            {
                // 容器的 Background 没有原生控件可承载，交宿主按矩形铺底
                IXamlMfcHost* host = window != nullptr
                    ? static_cast<IXamlMfcHost*>(window->Host())
                    : static_cast<IXamlMfcHost*>(page->Host());
                const xamlapp::XamlAppearance ap = AppearanceOf(child);
                if (host != nullptr && !ap.empty())
                    host->addContainerBackdrop(child.previewX, child.previewY,
                                               child.previewW, child.previewH, ap);
                LoadControlsFromElement(child, window, page, context, nextId, names);
                continue;
            }

            // 标准控件一律用 x:Name 命名，ID 由 guiload 自动分配
            // 例外：ControlTemplate 实例化出来的内部节点（templateChild）属于模板实现细节，
            // 不要求用户命名，也不登记到 names（否则多处复用会互相覆盖）。
            const std::string name = Utf8Of(child.name);
            if (name.empty() && !child.templateChild)
                throw std::runtime_error("guiload: <" + rawTag + "> 需要 x:Name 属性");
            if (nextId == nullptr)
                throw std::runtime_error("guiload: 缺少自动 ID 计数器");
            const int id = (*nextId)++;
            if (names != nullptr && !name.empty())
                (*names)[py::str(name)] = id;

            if (!StdIsKnown(tag))
            {
                // 允许自定义控件：查注册表
                if (!XamlControlRegistry::instance().isKnown(child.tag.c_str()))
                    throw std::runtime_error("guiload: 未知控件 <" + rawTag + ">");
            }

            // 注册表创建控件（标准控件返回 XamlMfcControlBase 派生，自定义控件返回其 IXamlControl）
            IXamlControl* ctrl = XamlControlRegistry::instance().create(child.tag.c_str());
            if (ctrl == nullptr)
                throw std::runtime_error("guiload: 无法创建控件 <" + rawTag + ">");

            // 布局结果（运行时已算好）
            const int x = child.previewX;
            const int y = child.previewY;
            const int w = child.previewW;
            const int h = child.previewH;
            const int minValue = AttrIntOf(child, L"Minimum", 0);
            const int maxValue = AttrIntOf(child, L"Maximum", 100);

            // 标准控件：Attach 真正创建原生子控件并 setBounds
            if (auto* mfc = dynamic_cast<XamlMfcControlBase*>(ctrl))
            {
                IXamlMfcHost* host = window != nullptr
                    ? static_cast<IXamlMfcHost*>(window->Host())
                    : static_cast<IXamlMfcHost*>(page->Host());
                mfc->attach(host, id, minValue, maxValue, x, y, w, h);
                mfc->setBounds(x, y, w, h);
                // 记下创建序：Tab 序在 TabIndex 相同时按它兜底
                if (window != nullptr)
                    window->Host()->noteCreated(id);
                else if (page != nullptr)
                    page->Host()->noteCreated(id);

                // Icon="res/xxx.png" 相对 exe 目录：解析成绝对路径推给宿主，
                // 由宿主在 owner-draw 时画，与设计器/预览器走同一条 drawImage 通路
                if (host != nullptr && !child.icon.empty())
                {
                    std::wstring ip = child.icon;
                    const bool isDir = (ip.back() == L'\\' || ip.back() == L'/');
                    const bool isAbs = (ip.size() > 2 && ip[1] == L':') || ip[0] == L'\\' || ip[0] == L'/';
                    if (!isDir)
                    {
                        if (!isAbs)
                            ip = pymfcDir() + L"\\" + ip;
                        host->setControlIcon(id, ip);
                    }
                }
            }
            else if (!ctrl->create(reinterpret_cast<void*>(window != nullptr
                                                              ? window->Host()->GetSafeHwnd()
                                                              : page->Host()->GetSafeHwnd())))
            {
                delete ctrl;
                throw std::runtime_error("guiload: 创建自定义控件失败 <" + rawTag + ">");
            }

            // 属性（注册表控件已实现的属性系统）。
            // 先下发本地声明（值取 get()，绑定表达式会解析成有效值），
            // 再下发样式/Trigger/绑定注入的有效层——否则运行时看不到 Style 与 {Binding}
            for (const auto& kv : child.attrs)
            {
                const std::string key = Utf8Of(kv.first);
                if (StrEq(key, "x:name") || StrEq(key, "name"))
                    continue;  // 布局/命名类属性，不是控件值属性
                const std::wstring val = child.get(kv.first.c_str());
                ctrl->setProperty(kv.first.c_str(), val);
                debugLogf(L"[prop] id=%d %ls=%ls -> %s", id, kv.first.c_str(), val.c_str(),
                          xamlapp::IsAppearanceProperty(kv.first.c_str()) ? "外观" : "其他");
            }
            for (const auto& kv : child.styledAttrs)
            {
                // 只看 attrs 判断是否本地声明：has() 会同时查 styledAttrs，
                // 用它判断会把有效层里的每个键都当成“本地已有”而全部跳过
                bool local = false;
                for (const auto& a : child.attrs)
                {
                    if (_wcsicmp(a.first.c_str(), kv.first.c_str()) == 0)
                    {
                        local = true;
                        break;
                    }
                }
                if (local)
                    continue;
                ctrl->setProperty(kv.first.c_str(), kv.second);
            }

            // 事件绑定（S2）：按契约表统一绑定，不再写死三种。
            // 回调名解析不到就报错（绝不静默忽略）；宿主还不能派发的事件告警后跳过。
            const std::string idText = std::to_string(id);
            for (const XamlEventInfo& info : xamlEventInfos())
            {
                const std::wstring cbName = AttrWOf(child, info.property);
                if (cbName.empty())
                    continue;
                if (!info.hostDispatch)
                {
                    debugLogf(L"[event] id=%d %ls 的 %ls 事件宿主暂不支持派发，已忽略（回调 %ls）",
                              id, ctrl->type(), info.property, cbName.c_str());
                    continue;
                }
                const std::string cbNameA = Utf8Of(cbName);
                if (!context.contains(cbNameA.c_str()))
                    throw std::runtime_error(std::string("guiload: 控件 id=") + idText +
                                             " 引用的回调 \"" + cbNameA + "\" 不存在（未在 context 中找到）");
                BindHostEvent(window, page, id, info, context[cbNameA.c_str()]);
            }

            // 控件内部还嵌着子控件（<Panel>、<GridCtrl> 的 <children> 等）：
            // 以前这里只遍历 node.children 一层，Panel 里的按钮/标签一个都不会创建，
            // 容器就只剩一个空矩形。XAML 阶段坐标已算成绝对坐标，直接在同一宿主上继续建。
            if (!child.children.empty())
                LoadControlsFromElement(child, window, page, context, nextId, names);
        }
    }

    void LoadControlsInto(py::handle node, PyMfcWindow* window, PyMfcPanel* page,
                         const py::dict& context, GuiFlow& flow)
    {
        // ElementTree 的 Element 是任意可迭代对象，不是 list，必须用 iterable 包装
        const py::iterable children = py::reinterpret_borrow<py::iterable>(node);
        for (py::handle child : children)
        {
            if (!py::isinstance<py::str>(child.attr("tag")))
                continue;  // 注释 / 处理指令
            const std::string rawTag = child.attr("tag").cast<std::string>();
            const std::string tag = TagKind(rawTag);
            if (tag == "page")
                continue;  // <page> 由 palette 分支处理
            if (tag == "rowdefinitions" || tag == "columndefinitions" ||
                tag == "resources" || tag == "style")
                continue;  // 布局/资源定义，不是控件
            const py::dict attrs = child.attr("attrib").cast<py::dict>();

            const std::wstring text = AttrWStringL(attrs, {"Content", "Header"});
            const int defH = IsContainerTag(tag)
                ? (flow.height > 0 ? flow.height : 160)
                : DefaultHeight(tag);
            int h = AttrIntL(attrs, {"Height"}, defH);
            int w = AttrIntL(attrs, {"Width"},
                             (std::max)(1, flow.width - flow.margin * 2 - flow.padding * 2));
            const int minValue = AttrIntL(attrs, {"Minimum"}, 0);
            const int maxValue = AttrIntL(attrs, {"Maximum"}, 100);

            // 没写 Top（或 Left）就按流式布局，写全了就按绝对坐标。
            // 坐标一律先按"相对容器内区"计算，最后加上 flow.offsetX/offsetY。
            const bool hasY = AttrHas(attrs, {"Top"});
            const bool hasX = AttrHas(attrs, {"Left"});
            int localX = 0;
            int localY = 0;
            if (flow.horizontal)
            {
                localX = hasX ? AttrIntL(attrs, {"Left"}, flow.cursorX) : flow.cursorX;
                localY = hasY ? AttrIntL(attrs, {"Top"}, flow.cursorY) : flow.cursorY;
                flow.cursorX = (std::max)(flow.cursorX, localX + w + flow.spacing);
            }
            else
            {
                localX = hasX ? AttrIntL(attrs, {"Left"}, flow.margin) : flow.margin;
                localY = hasY ? AttrIntL(attrs, {"Top"}, flow.cursorY) : flow.cursorY;
                if (hasY)
                    flow.cursorY = (std::max)(flow.cursorY, localY + h + flow.spacing);
                else
                    flow.cursorY += h + flow.spacing;
            }
            // Grid 单元格：位置和大小由行列定义决定
            if (!flow.gridRows.empty() && !flow.gridCols.empty())
            {
                int row = GridChildIndex(attrs, true);
                int col = GridChildIndex(attrs, false);
                int rowSpan = GridChildSpan(attrs, true);
                int colSpan = GridChildSpan(attrs, false);
                row = (std::max)(0, (std::min)(row, (int)flow.gridRows.size() - 1));
                col = (std::max)(0, (std::min)(col, (int)flow.gridCols.size() - 1));
                rowSpan = (std::max)(1, (std::min)(rowSpan, (int)flow.gridRows.size() - row));
                colSpan = (std::max)(1, (std::min)(colSpan, (int)flow.gridCols.size() - col));

                localX = flow.padding + SumTracks(flow.gridCols, 0, col);
                localY = flow.padding + SumTracks(flow.gridRows, 0, row);
                const int cellW = SumTracks(flow.gridCols, col, colSpan);
                const int cellH = SumTracks(flow.gridRows, row, rowSpan);
                if (AttrHas(attrs, {"Width"}))
                    w = (std::min)(w, cellW);
                else
                    w = cellW;
                if (AttrHas(attrs, {"Height"}))
                    h = (std::min)(h, cellH);
                else
                    h = cellH;
            }

            // Margin：WPF 的一/二/四值写法
            int marginLeft = 0, marginTop = 0, marginRight = 0, marginBottom = 0;
            ParseThickness(AttrStringL(attrs, {"Margin"}), marginLeft, marginTop,
                           marginRight, marginBottom);
            localX += marginLeft;
            localY += marginTop;
            w = (std::max)(1, w - marginLeft - marginRight);
            h = (std::max)(1, h - marginTop - marginBottom);

            const int x = flow.offsetX + localX;
            const int y = flow.offsetY + localY;

            // StackPanel：不创建原生控件，只提供一块布局区域（可横向 / 纵向）
            if (tag == "stackpanel")
            {
                const std::string orientation = AttrStringL(attrs, {"Orientation"});
                GuiFlow inner;
                inner.margin = 0;
                inner.spacing = kFlowSpacing;
                inner.width = w;
                inner.offsetX = x;
                inner.offsetY = y;
                inner.cursorX = 0;
                inner.cursorY = 0;
                inner.horizontal = NameMatch(orientation, "horizontal");
                inner.nextId = flow.nextId;
                inner.names = flow.names;
                LoadControlsInto(child, window, page, context, inner);

                const int consumed = inner.horizontal ? h : inner.cursorY;
                if (flow.horizontal)
                    flow.cursorX = (std::max)(flow.cursorX, localX + w + flow.spacing);
                else
                    flow.cursorY = (std::max)(flow.cursorY, localY + consumed + flow.spacing);
                continue;
            }

            // Grid：解析 RowDefinitions/ColumnDefinitions，子控件按 Grid.Row/Column 定位
            if (tag == "grid")
            {
                const int padding = AttrIntL(attrs, {"Padding"}, 0);
                GuiFlow inner;
                inner.margin = 0;
                inner.spacing = 0;
                inner.padding = padding;
                inner.width = w;
                inner.height = h;
                inner.offsetX = x;
                inner.offsetY = y;
                inner.gridRows = ResolveGridTracks(child, true,
                                                   (std::max)(1, h - padding * 2));
                inner.gridCols = ResolveGridTracks(child, false,
                                                   (std::max)(1, w - padding * 2));
                inner.nextId = flow.nextId;
                inner.names = flow.names;
                LoadControlsInto(child, window, page, context, inner);
                continue;
            }

            // DockPanel/WrapPanel/Canvas：先做兼容布局，后续再加强
            if (tag == "dockpanel" || tag == "wrappanel" || tag == "canvas")
            {
                GuiFlow inner;
                inner.margin = 0;
                inner.spacing = kFlowSpacing;
                inner.width = w;
                inner.height = h;
                inner.offsetX = x;
                inner.offsetY = y;
                inner.cursorX = 0;
                inner.cursorY = 0;
                inner.horizontal = (tag == "wrappanel");
                inner.nextId = flow.nextId;
                inner.names = flow.names;
                LoadControlsInto(child, window, page, context, inner);
                continue;
            }

            // 控件一律用 x:Name 命名，ID 由 guiload 自动分配（起点 first_id）
            // ElementTree 会把 x:Name 展开成 {xaml-uri}Name，本地名就是 Name，故两种写法都认
            const std::string name = AttrStringL(attrs, {"x:Name", "Name"});
            if (name.empty())
                throw std::runtime_error("guiload: <" + rawTag + "> 需要 x:Name 属性");
            if (flow.nextId == nullptr)
                throw std::runtime_error("guiload: 缺少自动 ID 计数器");
            const int id = (*flow.nextId)++;
            if (flow.names != nullptr)
                (*flow.names)[py::str(name)] = id;

            if (!IsKnownControl(tag))
                throw std::runtime_error("guiload: 未知控件 <" + rawTag + ">");

            if (tag == "label")
            {
                if (window) window->addLabel(id, text, x, y, w, h);
                else page->addLabel(id, text, x, y, w, h);
            }
            else if (tag == "button")
            {
                if (window) window->addButton(id, text, x, y, w, h);
                else page->addButton(id, text, x, y, w, h);
            }
            else if (tag == "edit")
            {
                // 文本框的基础功能全部由属性驱动：多行、只读、输入限长、空内容占位提示。
                // 旧写法 IsReadOnly 仍兼容（早期 catalog 用的是这个名字）。
                const bool multiline = AttrIntL(attrs, {"Multiline"}, 0) != 0;
                const bool readOnly = AttrIntL(attrs, {"ReadOnly", "IsReadOnly"}, 0) != 0;
                const int maxLength = AttrIntL(attrs, {"MaxLength"}, 0);
                const std::wstring placeholder = AttrWStringL(attrs, {"Placeholder"});
                if (window)
                    window->addEditAdvanced(id, text, multiline, readOnly, maxLength, placeholder, x, y, w, h);
                else
                    page->addEditAdvanced(id, text, multiline, readOnly, maxLength, placeholder, x, y, w, h);
            }
            else if (tag == "listbox")
            {
                const std::wstring mode = AttrWStringL(attrs, {"SelectionMode"});
                const bool multi = (mode == L"Multiple" || mode == L"Extended");
                if (window) window->addListBoxAdvanced(id, multi, x, y, w, h);
                else page->addListBoxAdvanced(id, multi, x, y, w, h);
            }
            else if (tag == "checkbox")
            {
                if (window) window->addCheckBox(id, text, x, y, w, h);
                else page->addCheckBox(id, text, x, y, w, h);
                if (AttrIntL(attrs, {"Checked", "IsChecked"}, 0) != 0)
                {
                    if (window) window->setCheck(id, true);
                    else page->setCheck(id, true);
                }
            }
            else if (tag == "radio")
            {
                if (window) window->addRadio(id, text, x, y, w, h);
                else page->addRadio(id, text, x, y, w, h);
                if (AttrIntL(attrs, {"Checked", "IsChecked"}, 0) != 0)
                {
                    if (window) window->setCheck(id, true);
                    else page->setCheck(id, true);
                }
            }
            else if (tag == "group")
            {
                if (window) window->addGroup(id, text, x, y, w, h);
                else page->addGroup(id, text, x, y, w, h);

                // group 可以当布局容器：内部控件坐标相对 group 内区
                // （左边距 kFlowMargin、标题下方 24px），并且 group 内有独立的流式游标
                GuiFlow inner;
                inner.margin = kFlowMargin;
                inner.spacing = kFlowSpacing;
                inner.cursorY = kFlowMargin;
                inner.width = (std::max)(w - 24, 40);
                inner.offsetX = x + kFlowMargin;
                inner.offsetY = y + 24;
                inner.nextId = flow.nextId;
                inner.names = flow.names;
                LoadControlsInto(child, window, page, context, inner);
            }
            else if (tag == "combo")
            {
                // DropDownStyle：DropDownList = 只能从下拉项里选（默认），
                // DropDown = 下拉框本身也能手打文字，系统原生支持，不需要另写输入框
                const std::wstring ddStyle = AttrWStringL(attrs, {"DropDownStyle"});
                const bool dropDownList = (ddStyle.empty() || ddStyle == L"DropDownList");
                if (window) window->addComboAdvanced(id, dropDownList, x, y, w, h);
                else page->addComboAdvanced(id, dropDownList, x, y, w, h);
            }
            else if (tag == "progress")
            {
                if (window) window->addProgress(id, x, y, w, h);
                else page->addProgress(id, x, y, w, h);
                if (window) window->setRange(id, minValue, maxValue);
                else page->setRange(id, minValue, maxValue);
            }
            else if (tag == "slider")
            {
                if (window) window->addSlider(id, minValue, maxValue, x, y, w, h);
                else page->addSlider(id, minValue, maxValue, x, y, w, h);
            }

            // 控件初始值：Value 对 ProgressBar/Slider 是位置
            if (AttrHas(attrs, {"Value"}) && (tag == "progress" || tag == "slider"))
            {
                const int pos = AttrIntL(attrs, {"Value"}, 0);
                if (window) window->setPos(id, pos);
                else page->setPos(id, pos);
            }

            // <ListBox>/<ComboBox> 的条目用 Items="a|b|c"
            const std::wstring items = AttrWStringL(attrs, {"Items"});
            if (!items.empty() && (tag == "listbox" || tag == "combo"))
            {
                std::wstring buffer;
                for (wchar_t c : items)
                {
                    if (c == L'|')
                    {
                        if (window) window->addString(id, buffer);
                        else page->addString(id, buffer);
                        buffer.clear();
                    }
                    else
                    {
                        buffer.push_back(c);
                    }
                }
                if (!buffer.empty())
                {
                    if (window) window->addString(id, buffer);
                    else page->addString(id, buffer);
                }
            }

            // 初始选中项必须在条目填完之后设置：空列表上 CB_SETCURSEL / LB_SETCURSEL
            // 会静默失败，选不上任何项
            if ((tag == "listbox" || tag == "combo") && AttrHas(attrs, {"SelectedIndex"}))
            {
                const int initSel = AttrIntL(attrs, {"SelectedIndex"}, -1);
                if (initSel >= 0)
                {
                    if (window) window->setCurSel(id, initSel);
                    else page->setCurSel(id, initSel);
                }
            }

            // 事件绑定（S2）：与 Element 路径同一张契约表
            const std::string idText = std::to_string(id);
            for (size_t ei = 0; ei < xamlEventInfos().size(); ++ei)
            {
                const XamlEventInfo& info = xamlEventInfos()[ei];
                const std::string cbName = AttrStringL(attrs, {NarrowEventProps()[ei].c_str()});
                if (cbName.empty())
                    continue;
                if (!info.hostDispatch)
                {
                    debugLogf(L"[event] id=%d %ls 事件宿主暂不支持派发，已忽略（回调 %ls）",
                              id, info.property, cbName.c_str());
                    continue;
                }
                if (!context.contains(cbName.c_str()))
                    throw std::runtime_error(std::string("guiload: 控件 id=") + idText +
                                             " 引用的回调 \"" + cbName + "\" 不存在（未在 context 中找到）");
                BindHostEvent(window, page, id, info, context[cbName.c_str()]);
            }

            // 控件里还嵌着子控件（<Panel> 内的按钮等）：继续建出来，
            // 否则容器型控件在 XML 路径下也是一个空框
            if (py::isinstance<py::iterable>(child) && py::len(child) > 0)
            {
                GuiFlow inner = flow;
                inner.nextId = flow.nextId;
                inner.width = (std::max)(w - 24, 40);
                inner.offsetX = x + 12;
                inner.offsetY = y + 12;
                inner.cursorX = kFlowMargin;
                inner.cursorY = kFlowMargin;
                LoadControlsInto(child, window, page, context, inner);
            }
        }
    }

    // ---- 自定义控件（Python 侧实现，C++ 提供 IXamlControl 统一接口）----
    // register_control(tag, factory)：factory 是 Python 可调用，返回自定义控件实例。
    // 实例约定实现（duck-typing）：
    //   create(hwnd) -> bool          创建原生控件（hwnd 为宿主 HWND）
    //   destroy()                     销毁
    //   set_property(name:str, value:str) -> bool
    //   get_property(name:str) -> str
    //   native_handle() -> int        返回控件 HWND
    //   type() -> str                 可选，默认返回注册标签
    // C++ 侧用 PyXamlControlAdapter 桥接到 IXamlControl，guiload 走注册表统一创建。
    class PyXamlControlAdapter : public IXamlControl
    {
    public:
        explicit PyXamlControlAdapter(std::shared_ptr<py::object> obj, std::wstring type)
            : m_obj(std::move(obj)), m_type(std::move(type)) {}

        const wchar_t* type() const override
        {
            m_typeCache = m_type;
            return m_typeCache.c_str();
        }

        bool create(void* parentHandle) override
        {
            try
            {
                py::object f = m_obj->attr("create");
                return f(reinterpret_cast<long long>(parentHandle)).cast<bool>();
            }
            catch (const std::exception&)
            {
                return false;
            }
        }

        void destroy() override
        {
            try
            {
                m_obj->attr("destroy")();
            }
            catch (...)
            {
            }
        }

        void* nativeHandle() const override
        {
            try
            {
                py::object h = m_obj->attr("native_handle")();
                return reinterpret_cast<void*>(h.cast<long long>());
            }
            catch (...)
            {
                return nullptr;
            }
        }

        void setBounds(int x, int y, int w, int h) override
        {
            try
            {
                m_obj->attr("set_bounds")(x, y, w, h);
            }
            catch (...)
            {
            }
        }

        void getBounds(int& x, int& y, int& w, int& h) const override
        {
            x = y = w = h = 0;
            try
            {
                py::tuple t = m_obj->attr("get_bounds")().cast<py::tuple>();
                if (t.size() >= 4)
                {
                    x = t[0].cast<int>();
                    y = t[1].cast<int>();
                    w = t[2].cast<int>();
                    h = t[3].cast<int>();
                }
            }
            catch (...)
            {
            }
        }

        bool setProperty(const wchar_t* name, const std::wstring& value) override
        {
            // 自定义控件同样遵守统一外观契约（形状/颜色/风格/前置背景/后置背景）
            if (name != nullptr && xamlapp::SetProperty(m_ap, name, value))
            {
                // 外观同时对 Python 侧可见，便于自定义控件自绘时读取
                try
                {
                    m_obj->attr("set_property")(name, value);
                }
                catch (...)
                {
                    PyErr_Clear();
                }
                return true;
            }
            try
            {
                py::object f = m_obj->attr("set_property");
                return f(name != nullptr ? name : L"", value).cast<bool>();
            }
            catch (const std::exception&)
            {
                return false;
            }
        }

        std::wstring getProperty(const wchar_t* name) const override
        {
            if (name != nullptr && xamlapp::IsAppearanceProperty(name))
            {
                std::wstring ap = xamlapp::GetProperty(m_ap, name);
                if (!ap.empty())
                    return ap;
            }
            try
            {
                py::object f = m_obj->attr("get_property");
                return f(name != nullptr ? name : L"").cast<std::wstring>();
            }
            catch (const std::exception&)
            {
                return {};
            }
        }

        std::vector<XamlProperty> properties() const override
        {
            // 自定义控件与标准控件共用同一套外观契约
            return xamlapp::Properties();
        }

        bool bindEvent(const wchar_t* kind, void* callback) override
        {
            (void)kind;
            (void)callback;
            return true;
        }

        const xamlapp::XamlAppearance& appearance() const { return m_ap; }

    private:
        std::shared_ptr<py::object> m_obj;
        std::wstring m_type;
        mutable std::wstring m_typeCache;
        xamlapp::XamlAppearance m_ap;
    };

    // register_control(tag, factory)：把 Python 工厂注册进统一注册表



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

    void RegisterPythonControl(const std::string& tag, py::object factory)
    {
        if (tag.empty())
            throw std::runtime_error("register_control: 控件标签不能为空");
        std::wstring tagW = Utf8ToWide(tag);
        auto holder = std::make_shared<py::object>(factory);
        auto& reg = XamlControlRegistry::instance();
        bool ok = reg.registerFactory(tagW.c_str(), [holder, tagW]() -> IXamlControl* {
            try
            {
                py::object obj = (*holder)();
                return new PyXamlControlAdapter(
                    std::make_shared<py::object>(obj), tagW);
            }
            catch (...)
            {
                return nullptr;
            }
        });
        if (!ok)
            throw std::runtime_error("register_control: 标签已注册或注册失败 <" + tag + ">");
    }

    py::object Guiload(const std::string& path, py::object target, py::object context,
                       py::object names, int firstId, py::object palette)
    {
        // 统一运行时：XamlRuntime 解析 + 布局，控件经注册表创建。
        // 用 shared_ptr 而不是栈上对象：加载完要交给宿主持管，
        // 否则改一个 Width 也没人能把表达式重算、把控件挪位（runtime 一析构就全丢）。
        auto runtimeOwned = std::make_shared<XamlRuntime>();
        XamlRuntime& runtime = *runtimeOwned;
        std::wstring loadError;
        std::wstring widePath = Utf8ToWide(path);
        if (!runtime.loadFile(widePath.c_str(), loadError))
            throw std::runtime_error("guiload: " + Utf8Of(loadError));
        XamlElement* root = runtime.root();
        if (root == nullptr)
            throw std::runtime_error("guiload: XML 根节点无效");

        // 声明层三件套必须先落地：样式 / 控件模板 / 属性绑定（含 Trigger）。
        // 少了这一步，运行时不认 Style、Template、{Binding}，只有设计器和预览器好看。
        runtime.applyDeclarations();

        const std::wstring rootRawTag = root->tag;
        const std::string rootKind = StdTagKind(Utf8Of(rootRawTag));

        // 根节点只用标签决定类型：<Window> 或 <PaletteSet>
        const std::string type = rootKind;
        if (type != "window" && type != "palette")
            throw std::runtime_error("guiload: 根节点必须是 <Window> 或 <PaletteSet>，收到 <" +
                                     Utf8Of(rootRawTag) + ">");

        py::dict ctx = py::isinstance<py::dict>(context) ? context.cast<py::dict>() : CallerGlobals();
        py::dict nameMap = py::isinstance<py::dict>(names) ? names.cast<py::dict>() : py::dict();
        int nextId = firstId < 1000 ? 1000 : firstId;

        if (type == "window")
        {
            const int rootWidth = AttrIntOf(*root, L"Width", 420);
            const int rootHeight = AttrIntOf(*root, L"Height", 320);
            PyMfcWindow* window = nullptr;
            if (!target.is_none())
            {
                try
                {
                    window = target.cast<PyMfcWindow*>();
                }
                catch (...)
                {
                    PyErr_Clear();
                }
                if (window == nullptr)
                    throw std::runtime_error("guiload: target 不是 PyMfc.Window");
            }
            else
            {
                std::wstring title = AttrWOf(*root, L"title");
                if (title.empty())
                    title = L"PyMfc";
                window = PyMfcStore::Get().NewWindow();
                // Window.Icon → 标题栏左上角图标（相对 exe 目录，与控件 Icon 同一套规则）
                if (!root->icon.empty())
                {
                    std::wstring ip = root->icon;
                    const bool isDir = (ip.back() == L'\\' || ip.back() == L'/');
                    if (!isDir)
                    {
                        const bool isAbs = (ip.size() > 2 && ip[1] == L':') ||
                                           ip[0] == L'\\' || ip[0] == L'/';
                        if (!isAbs)
                            ip = pymfcDir() + L"\\" + ip;
                        window->setWindowIcon(ip);
                    }
                }
                if (!window->create(title, rootWidth, rootHeight))
                    throw std::runtime_error("guiload: 创建 Window 失败");
                const std::wstring theme = AttrWOf(*root, L"Theme");
                window->setTheme(!(theme == L"light"));
                window->markSilentBuild();   // 建完控件树再由 endSilentBuild() 显示
            }

            try
            {
                // 布局：以窗口客户区为根矩形
                CRect rc;
                HWND hostHwnd = window->Host()->GetSafeHwnd();
                ::GetClientRect(hostHwnd, &rc);
                runtime.layout(0, 0, rc.Width(), rc.Height());

                // 容器铺底：先清上一轮，再把根节点自己声明的 Background 铺满客户区
                window->Host()->clearContainerBackdrops();
                const xamlapp::XamlAppearance rootAp = AppearanceOf(*root);
                if (!rootAp.empty())
                    window->Host()->addContainerBackdrop(0, 0, rc.Width(), rc.Height(), rootAp);
                COLORREF pageBg = 0;
                if (!rootAp.backColor.empty() && xamlapp::ParseColor(rootAp.backColor, pageBg))
                    window->Host()->setPageBackdrop(pageBg, true);

                LoadControlsFromElement(*root, window, nullptr, ctx, &nextId, &nameMap);

                // 运行时交接：宿主拿住 runtime + name→id，之后改属性就能真切重排
                std::map<std::wstring, int> ids;
                for (auto item : nameMap)
                {
                    try
                    {
                        ids[item.first.cast<std::wstring>()] = item.second.cast<int>();
                    }
                    catch (...)
                    {
                        PyErr_Clear();
                    }
                }
                window->Host()->attachXaml(runtimeOwned, ids);
                // 层次与 Tab 序：等整棵树建完再统一排一次，避免边建边动 z 序
                window->Host()->applyLevelsAndTabs();
                // 控件树齐了再一次性亮相（Embedded 窗口创建时是隐藏的）
                window->endSilentBuild();
            }
            catch (...)
            {
                // 加载失败不留半成品窗口在屏幕上
                window->hide();
                window->closeAsync();
                throw;
            }
            return py::cast(window, py::return_value_policy::reference);
        }

        if (type == "palette")
        {
            // 侧栏容器（CAdUiPaletteSet）由 CAD 侧创建：调用方传入 PyArxCore.Ap 的
            // 侧栏对象，本库只按 <page> 向它要页窗口、在页里铺控件 —— 对 CAD 零依赖。
            if (palette.is_none())
                throw std::runtime_error(
                    "guiload: 根节点 <PaletteSet> 需要 palette 参数（PyArxCore.Ap.getPalette(...) 的返回值）");

            const int width = AttrIntOf(*root, L"Width", 280);
            const int height = AttrIntOf(*root, L"Height", 600);

            // 向 CAD 侧要一页，并把页 HWND 交回本库建控件宿主
            auto openPage = [&](const std::wstring& title) -> PyMfcPanel*
            {
                py::object page = palette.attr("addPage")(py::cast(title));
                if (page.is_none())
                    throw std::runtime_error("guiload: palette.addPage 返回空页");
                const long long pageHwnd = page.attr("hwnd").cast<long long>();
                PyMfcPanel* panel = PyMfcStore::Get().NewPanel(
                    reinterpret_cast<HWND>(static_cast<std::intptr_t>(pageHwnd)));
                if (panel == nullptr || !panel->valid())
                    throw std::runtime_error("guiload: 在侧栏页上创建控件宿主失败");
                page.attr("setHost")(static_cast<long long>(
                    reinterpret_cast<std::intptr_t>(panel->hwnd())));
                return panel;
            };

            bool hasPage = false;
            {
                for (auto& child : root->children)
                {
                    if (StdTagKind(Utf8Of(child.tag)) != "page")
                        continue;
                    hasPage = true;
                    std::wstring title = AttrWOf(child, L"Header");
                    if (title.empty())
                        title = L"page";
                    const int contentWidth = AttrIntOf(child, L"Width", width - kFlowMargin * 2);
                    PyMfcPanel* panel = openPage(title);
                    xamlLayoutSubtree(child, 0, 0, contentWidth, height);

                    panel->Host()->clearContainerBackdrops();
                    const xamlapp::XamlAppearance pageAp = AppearanceOf(child);
                    if (!pageAp.empty())
                        panel->Host()->addContainerBackdrop(0, 0, contentWidth, height, pageAp);
                    COLORREF pageBg = 0;
                    if (!pageAp.backColor.empty() && xamlapp::ParseColor(pageAp.backColor, pageBg))
                        panel->Host()->setPageBackdrop(pageBg, true);

                    LoadControlsFromElement(child, nullptr, panel, ctx, &nextId, &nameMap);
                }

                // 根节点下直接写控件也支持：自动建一页
                if (!hasPage)
                {
                    PyMfcPanel* panel = openPage(L"page");
                    xamlLayoutSubtree(*root, 0, 0, width - kFlowMargin * 2, height);
                    panel->Host()->clearContainerBackdrops();
                    const xamlapp::XamlAppearance pageAp = AppearanceOf(*root);
                    if (!pageAp.empty())
                        panel->Host()->addContainerBackdrop(0, 0, width - kFlowMargin * 2, height, pageAp);
                    COLORREF pageBg = 0;
                    if (!pageAp.backColor.empty() && xamlapp::ParseColor(pageAp.backColor, pageBg))
                        panel->Host()->setPageBackdrop(pageBg, true);
                    LoadControlsFromElement(*root, nullptr, panel, ctx, &nextId, &nameMap);
                }
            }

            palette.attr("show")();
            return palette;
        }


        throw std::runtime_error("guiload: 不支持的根节点 <" + Utf8Of(rootRawTag) + ">");
    }
