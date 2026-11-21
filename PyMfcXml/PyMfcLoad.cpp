// PyMfcLoad.cpp —— guiload：XML 文件 → 控件树（布局运行时 + 控件工厂）。
// 原名 parts\05-load.inc；三个对外入口（Guiload / XmlLayoutRects / RegisterPythonControl）
// 已移出匿名 namespace 成为外部链接函数，声明见 PyMfcParts.h。

#include "PyMfcCommon.h"
#include "PyMfcWrap.h"
#include "PyMfcParts.h"
#include "PyMfcPerf.h"

// 图片自然尺寸（定义在 PyMfcHost.cpp）：HTML 的 <img> 不写宽高时按原图尺寸占位
bool pyMfcImagePixelSize(const std::wstring& path, int& outW, int& outH);

// guiload：XML → 控件树（XML 布局运行时）  （从 PyMfcXml.cpp 按类/职责切出，行 9505-11089）
// ---------------------------------------------------------------------------
// 标准 MFC 控件工厂：把 XML 控件名映射到 PyMfcHost 的 Win32 控件实现。
// 实现 IXmlControl 接口，注册进 XmlControlRegistry —— 这样 guiload 与设计器
// 都通过统一运行时 + 注册表创建控件，不再各自硬编码 switch。
//
// 生命周期与 PyMfc 原则一致：宿主（PyMfcHost）持有子控件 HWND；本对象只持
// 有 host 指针与 id，不拥有/不删除原生控件。create/destroy 只做窗口创建/销毁。
// ---------------------------------------------------------------------------
namespace
{
    // P0 真实父子层级开关：**默认开**（这是 HTML 语义的正确形态——子控件就该挂在容器里，
    // 裁剪/层叠/子项自排才成立）。只有显式 PYARX_FLEX_PARENT=0 才退回老的扁平兄弟结构，
    // 用于排查历史界面的层级相关差异。
    bool RealParentEnabled()
    {
        static const bool on = [] {
            wchar_t buf[8] = {0};
            const DWORD n = ::GetEnvironmentVariableW(L"PYARX_FLEX_PARENT", buf, 8);
            return !(n > 0 && (buf[0] == L'0' || buf[0] == L'n' || buf[0] == L'N'));
        }();
        return on;
    }

    // 标准控件标签 -> 短名（与 guiload 的 TagKind 一致）
    std::string StdTagKind(const std::string& raw)
    {
        std::string tag = raw;
        const size_t brace = tag.rfind('}');
        if (brace != std::string::npos)
            tag = tag.substr(brace + 1);
        // 统一命名：<XWindow> / <XButton> 去掉前缀后按本意名分派
        if (tag.size() > 1 && (tag[0] == 'X' || tag[0] == 'x') && isupper((unsigned char)tag[1]))
            tag = tag.substr(1);
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
    XmlElement* FindXmlByName(XmlElement* node, const std::wstring& name)
    {
        if (node == nullptr)
            return nullptr;
        if (node->name == name)
            return node;
        for (auto& c : node->children)
        {
            if (XmlElement* hit = FindXmlByName(&c, name))
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

    // 标签只接受 XML 控件名（大小写不敏感，允许带命名空间前缀），映射到内部短名。
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
        // 统一命名：<XStackPanel> 去掉前缀后按本意名分派
        if (tag.size() > 1 && tag[0] == 'x' && isupper((unsigned char)tag[1]))
            tag = tag.substr(1);
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
                       const XmlEventInfo& info, py::object cb)
    {
        auto apply = [&](auto* target) {
            switch (info.kind)
            {
            case XmlEventKind::Click:            target->onClick(id, std::move(cb)); break;
            case XmlEventKind::DoubleClick:      target->onDoubleClick(id, std::move(cb)); break;
            case XmlEventKind::TextChanged:      target->onChange(id, std::move(cb)); break;
            case XmlEventKind::SelectionChanged: target->onSelect(id, std::move(cb)); break;
            case XmlEventKind::ValueChanged:     target->onValue(id, std::move(cb)); break;
            case XmlEventKind::CheckedChanged:   target->onChecked(id, std::move(cb)); break;
            case XmlEventKind::DropDown:         target->onDropDown(id, std::move(cb)); break;
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
            for (const XmlEventInfo& info : xmlEventInfos())
                v.push_back(Utf8Of(info.property));
            return v;
        }();
        return kNames;
    }


    // ---- 统一运行时路径：遍历 XmlElement 树，控件经注册表创建 ----
    // 每个节点都已被 XmlRuntime::Layout 算出 previewX/Y/W/H，容器/Grid/GroupBox
    // 的递归定位已在布局阶段完成，这里只负责"创建控件 + 属性 + 事件 + 映射"。
    // 事件回调名到 context 全局函数的解析逻辑与原 LoadControlsInto 完全一致。

    // 元素的有效外观（本地声明优先，其余取样式/绑定注入的有效层）。
    // 容器不建原生控件，只能靠这份外观由宿主铺底。
    xmlapp::XmlAppearance AppearanceOf(const XmlElement& el)
    {
        PyMfcPerfScope _p(&pyMfcPerf().apOfMs, &pyMfcPerf().nApOf);
        xmlapp::XmlAppearance ap;
        for (const auto& kv : el.attrs)
            xmlapp::SetProperty(ap, kv.first, kv.second);
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
                xmlapp::SetProperty(ap, kv.first, kv.second);
        }
        return ap;
    }

    int AttrIntOf(const XmlElement& el, const wchar_t* key, int fallback)
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

    std::wstring AttrWOf(const XmlElement& el, const wchar_t* key)
    {
        return el.get(key);
    }

    void LoadControlsFromElement(XmlElement& node, PyMfcWindow* window, PyMfcPanel* page,
                                 const py::dict& context, int* nextId, py::dict* names,
                                 const std::wstring& inheritAlign = std::wstring())
    {
        // CSS 继承：text-align 是可继承属性（HTML 里写在父容器的 style 上，子元素自动跟随）。
        // XML 早先必须给每个 <label> 各写一遍 —— 这里把父容器声明的对齐方式往下发一层。
        const std::wstring nodeAlign = [&]() -> std::wstring {
            for (const auto& kv : node.attrs)
                if (_wcsicmp(kv.first.c_str(), L"TextAlign") == 0)
                    return kv.second;
            for (const auto& kv : node.styledAttrs)
                if (_wcsicmp(kv.first.c_str(), L"textalign") == 0 ||
                    _wcsicmp(kv.first.c_str(), L"text-align") == 0)
                    return kv.second;
            return inheritAlign;
        }();
        // P0 真实父子层级：本元素若是 flex 容器（XmlFlexPanel），它的子控件要挂到它窗口上，
        // 子窗口坐标改成"容器内相对坐标"，之后容器 reflowItems 自己排、自己挪。
        // 扁平兄弟结构下容器的裁剪/层叠/缩放跟排永远做不到，这一步是前提。
        HWND nodeFlexHwnd = nullptr;
        bool nodeClipsOverflow = false;
        if (node.control != nullptr)
        {
            auto* nc = dynamic_cast<XmlMfcControlBase*>(static_cast<IXmlControl*>(node.control));
            if (nc != nullptr)
            {
                if (nc->isFlexContainer())
                    nodeFlexHwnd = static_cast<HWND>(nc->nativeHandle());
                else if (nc->nativeHandle() != nullptr)
                {
                    // CSS overflow:hidden/auto/scroll 的普通容器：子项必须挂到它窗口上，
                    // 系统的"子窗口裁到父客户区"才会生效 —— 扁平兄弟结构下 overflow 永远裁不住。
                    // 只在显式声明裁剪时启用，避免影响未使用 overflow 的老界面。
                    const xmlapp::XmlAppearance cap = AppearanceOf(node);
                    if (::_wcsicmp(cap.overflow.c_str(), L"hidden") == 0 ||
                        ::_wcsicmp(cap.overflow.c_str(), L"auto") == 0 ||
                        ::_wcsicmp(cap.overflow.c_str(), L"scroll") == 0)
                    {
                        nodeFlexHwnd = static_cast<HWND>(nc->nativeHandle());
                        nodeClipsOverflow = true;
                    }
                }
            }
        }

        for (auto& child : node.children)
        {
            PyMfcPerfScope _elem(&pyMfcPerf().elemMs, &pyMfcPerf().nElem);
            // 父容器的 text-align 下发（CSS 继承）；子元素自己写了就不覆盖。
            // 写进 styledAttrs 而不是 attrs：样式注入不能污染 XML 源码写回。
            if (!nodeAlign.empty())
            {
                bool childHasAlign = false;
                for (const auto& kv : child.attrs)
                    if (_wcsicmp(kv.first.c_str(), L"TextAlign") == 0) { childHasAlign = true; break; }
                if (!childHasAlign)
                    for (const auto& kv : child.styledAttrs)
                        if (_wcsicmp(kv.first.c_str(), L"textalign") == 0 ||
                            _wcsicmp(kv.first.c_str(), L"text-align") == 0) { childHasAlign = true; break; }
                if (!childHasAlign)
                    child.styledAttrs.emplace_back(L"TextAlign", nodeAlign);
            }

            const std::string rawTag = Utf8Of(child.tag);
            const std::string tag = StdTagKind(rawTag);
            if (tag == "page")
                continue;  // <page> 由 palette 分支处理
            if (tag == "rowdefinitions" || tag == "columndefinitions" ||
                tag == "resources" || tag == "style")
                continue;  // 布局/资源定义，不是控件
            // Ribbon 内部结构节点（Tab/Group/Button/MenuItem/Backstage）由 ribbonParse
            // 自行遍历子树，运行时加载层不为它们创建原生控件
            if (tag == "ribbontab" || tag == "ribbongroup" || tag == "ribbonbutton" ||
                tag == "ribbonmenuitem" || tag == "ribbonbackstage" ||
                tag == "ribbonbackstageitem")
                continue;
            if (tag == "filetabitem" || tag == "xfiletabitem")
                continue;

            // 容器：不创建原生控件，递归子树（坐标已在布局阶段算好）
            // 注意 <ToolBar> 是历史布局容器名，而 <XToolBar> 是自绘工具条控件——
            // 两者归一到同一个 tag，必须靠原始标签区分，否则 XToolBar 会落进这里
            // 被当成容器吃掉（只递归子节点，自己永远不创建）。
            const bool legacyToolbar = (tag == "toolbar") &&
                ::_stricmp(rawTag.c_str(), "ToolBar") == 0;
            if (tag == "stackpanel" || tag == "grid" || tag == "dockpanel" ||
                tag == "wrappanel" || tag == "canvas" || legacyToolbar ||
                tag == "menubar" || tag == "statusbar")
            {
                // 容器的 Background 没有原生控件可承载，交宿主按矩形铺底
                IXmlMfcHost* host = window != nullptr
                    ? static_cast<IXmlMfcHost*>(window->Host())
                    : static_cast<IXmlMfcHost*>(page->Host());
                const xmlapp::XmlAppearance ap = AppearanceOf(child);
                if (host != nullptr && !ap.empty())
                    host->addContainerBackdrop(child.previewX, child.previewY,
                                               child.previewW, child.previewH, ap);
                LoadControlsFromElement(child, window, page, context, nextId, names, nodeAlign);
                continue;
            }

            // 标准控件的 ID 由 guiload 自动分配；Name 可写可不写（HTML 式写法）
            // 例外：ControlTemplate 实例化出来的内部节点（templateChild）属于模板实现细节，
            // 不登记到 names（否则多处复用会互相覆盖）。
            if (nextId == nullptr)
                throw std::runtime_error("guiload: 缺少自动 ID 计数器");
            std::string name = Utf8Of(child.name);
            const bool anonymous = name.empty();
            if (anonymous && !child.templateChild)
            {
                // 不写 Name：按标签 + 序号自动命名，仍登记进 names（Python 侧可回查）
                name = tag + "_" + std::to_string(*nextId);
                child.name = Utf8ToWide(name);
            }
            // HTML 里 <div>/<section>/<form>/<fieldset> 这类是"块级容器"，本身不是控件：
            // 匿名容器再登记进 names，Python 侧 `names` 就平白多出一个空文本节点，
            // 与 HTML 语义不符（浏览器里 layout 容器也没有 textContent 可读）。
            // 自动名仍写进 child.name（宿主建控件与 x:Name 查找依赖它），只是不对外登记。
            const auto containerish = [](const std::string& t) {
                return t == "div" || t == "section" || t == "form" || t == "fieldset" ||
                       t == "row" || t == "column" || t == "xflexpanel" || t == "flexpanel" ||
                       t == "panel" || t == "stackpanel" || t == "grid" || t == "dockpanel" ||
                       t == "wrappanel" || t == "canvas" || t == "page";
            };
            const bool skipRegister = anonymous && containerish(tag);
            const int id = (*nextId)++;
            if (names != nullptr && !name.empty() && !skipRegister)
                (*names)[py::str(name)] = id;

            if (!StdIsKnown(tag))
            {
                // 允许自定义控件：查注册表
                if (!XmlControlRegistry::instance().isKnown(child.tag.c_str()))
                    throw std::runtime_error("guiload: 未知控件 <" + rawTag + ">");
            }

            // 注册表创建控件（标准控件返回 XmlMfcControlBase 派生，自定义控件返回其 IXmlControl）
            IXmlControl* ctrl = XmlControlRegistry::instance().create(child.tag.c_str());
            if (ctrl == nullptr)
                throw std::runtime_error("guiload: 无法创建控件 <" + rawTag + ">");

            // 布局结果（运行时已算好）
            const int x = child.previewX;
            const int y = child.previewY;
            const int w = child.previewW;
            const int h = child.previewH;
            const int minValue = AttrIntOf(child, L"Minimum", 0);
            const int maxValue = AttrIntOf(child, L"Maximum", 100);
            debugLogf(L"[attach] tag=%ls id=%d box=%d,%d,%d,%d",
                      child.tag.c_str(), id, x, y, w, h);

            // 标准控件：Attach 真正创建原生子控件并 setBounds
            if (auto* mfc = dynamic_cast<XmlMfcControlBase*>(ctrl))
            {
                // 回填元素->控件：控件层 flex 容器要靠它按 id 认领子项
                child.control = ctrl;
                IXmlMfcHost* host = window != nullptr
                    ? static_cast<IXmlMfcHost*>(window->Host())
                    : static_cast<IXmlMfcHost*>(page->Host());
                mfc->attach(host, id, minValue, maxValue, x, y, w, h);
                mfc->setBounds(x, y, w, h);
                // CSS position:sticky：交给宿主登记吸顶距离（控件本身只会跟着滚）
                if (child.sticky && host != nullptr)
                    host->registerStickyChild(id, child.stickyTop);
                // CSS border-radius 落到容器上：把原生窗口裁成圆角区域，圆角外的像素露出父窗口底色。
                // 此前 <div style="border-radius:12"> 画出来仍是直角块（背板由 CTLCOLOR 纯色刷子出图，
                // 圆角路径走不到），与浏览器差一个观感维度。
                if (tag == "xflexpanel" || tag == "flexpanel" || tag == "panel")
                {
                    const xmlapp::XmlAppearance cap = AppearanceOf(child);
                    HWND hw = static_cast<HWND>(mfc->nativeHandle());
                    RECT rc = { 0, 0, w, h };
                    // 半径判据：必须看 XML 有没有显式声明。AppearanceOf 会给未声明的容器垫一个
                    // 默认半径，只看 wantRound/ResolveRadius 会把每个容器都当成圆角窗口。
                    auto declared = [&](std::initializer_list<const wchar_t*> keys) -> bool
                    {
                        for (const auto& kv : child.attrs)
                            for (const wchar_t* k : keys)
                                if (::_wcsicmp(kv.first.c_str(), k) == 0)
                                    return true;
                        for (const auto& kv : child.styledAttrs)
                            for (const wchar_t* k : keys)
                                if (::_wcsicmp(kv.first.c_str(), k) == 0)
                                    return true;
                        return false;
                    };
                    // div 糖化时会把"没写背景"补成 Background="transparent"（XmlCore 的 HTML
                    // 对齐），所以判"有没有背板"必须看**属性值**，不能只看键在不在。
                    auto bgValue = [&]() -> std::wstring
                    {
                        auto pick = [](const std::vector<std::pair<std::wstring, std::wstring>>& lst)
                            -> std::wstring
                        {
                            for (const auto& kv : lst)
                                if (::_wcsicmp(kv.first.c_str(), L"Background") == 0 ||
                                    ::_wcsicmp(kv.first.c_str(), L"BackColor") == 0 ||
                                    ::_wcsicmp(kv.first.c_str(), L"BgColor") == 0 ||
                                    ::_wcsicmp(kv.first.c_str(), L"BackgroundColor") == 0)
                                    return kv.second;
                            return std::wstring();
                        };
                        std::wstring v = pick(child.attrs);
                        if (v.empty())
                            v = pick(child.styledAttrs);
                        return v;
                    };
                    const std::wstring bgVal = bgValue();
                    const bool hasPlate = !bgVal.empty() &&
                        ::_wcsicmp(bgVal.c_str(), L"transparent") != 0;
                    const bool hasRadiusDecl = declared({ L"BorderRadius", L"Radius", L"Shape" });
                    // ResolveRadius 在"没写半径"时会回退默认 8，所以它只能用来算**已经确认要圆角**
                    // 的容器；先按 radius/radiusPct/Capsule 判实值，再取半径，才不会把 border-radius:0
                    // 也裁成圆角。
                    const bool realRadius = cap.radius > 0 || cap.radiusPct >= 0 ||
                        ::_wcsicmp(cap.shape.c_str(), L"Capsule") == 0;
                    const int r = (hw != nullptr && hasRadiusDecl && realRadius)
                        ? xmlapp::ResolveRadius(rc, cap) : 0;
                    // CSS overflow:hidden/auto/scroll：把容器窗口收成自身区域，挂在它上面的
                    // 子窗口被系统统一裁到父客户区 —— 这就是 HTML「溢出被裁」的等价做法。
                    // 未声明 / visible 时保持老行为（不裁，与 HTML 默认 visible 一致）。
                    const bool clipOverflow = ::_wcsicmp(cap.overflow.c_str(), L"hidden") == 0 ||
                        ::_wcsicmp(cap.overflow.c_str(), L"auto") == 0 ||
                        ::_wcsicmp(cap.overflow.c_str(), L"scroll") == 0;
                    if (hw != nullptr && clipOverflow)
                    {
                        HRGN rgn = (r > 0)
                            ? ::CreateRoundRectRgn(0, 0, w + 1, h + 1, r * 2, r * 2)
                            : ::CreateRectRgn(0, 0, w + 1, h + 1);
                        if (rgn != nullptr)
                        {
                            ::SetWindowRgn(hw, rgn, TRUE);
                            debugLogf(L"[overflow] clip hw=%p w=%d h=%d r=%d", static_cast<void*>(hw), w, h, r);
                        }
                        // CSS overflow:scroll/auto —— 光裁不够，内容还要能滚。
                        // 把容器登记成"可滚容器"，滚轮到达时按偏移整体平移子窗口。
                        const bool scrollable = ::_wcsicmp(cap.overflow.c_str(), L"scroll") == 0 ||
                            ::_wcsicmp(cap.overflow.c_str(), L"auto") == 0;
                        if (scrollable && host != nullptr)
                            host->registerScrollContainer(id);
                    }
                    else if (hw != nullptr && r > 0)
                    {
                        HRGN rgn = ::CreateRoundRectRgn(0, 0, w + 1, h + 1, r * 2, r * 2);
                        if (rgn != nullptr)
                            ::SetWindowRgn(hw, rgn, TRUE);
                    }
                    else if (hw != nullptr && !child.children.empty() && !hasPlate)
                    {
                        // 真父子模式下**不能**把这层做成空 region：窗口区域是"裁剪区"，
                        // 父容器一旦空壳，挂在它上面的子窗口会被一起裁掉 —— 表现为
                        // 层级、坐标全对，但整块卡片只剩页面底色。此时靠 CTLCOLOR 刷子
                        // 返回父表面色来"透明"，而不是靠 region 挖空。
                        static const bool kRealParentRegion = RealParentEnabled();
                        if (!kRealParentRegion)
                        {
                        // HTML 对齐：没写 background 的 div 只是排版容器，不该占住一块不透明窗口 ——
                        // 它的窗口矩形会把同层子项的背板/文字整块盖掉（实测 flex 里的卡片只会
                        // 采到页面底色，把容器 region 清空后立刻恢复）。给空 region 让它退化成
                        // "纯排版壳"，绘制全部交给子控件自己。
                        // 写了 background 的容器必须保留自己的窗口：宿主背板层在 WS_CLIPCHILDREN
                        // 下画不到子窗口覆盖区，真正出颜色的是控件自己的 WM_CTLCOLORSTATIC 刷子，
                        // 清空 region 反而会把已能显示的底色涂丢。
                        HRGN rgn = ::CreateRectRgn(0, 0, 0, 0);
                        if (rgn != nullptr)
                            ::SetWindowRgn(hw, rgn, TRUE);
                        }
                    }
                }
                // CSS linear-gradient 背板【暂缓】—— 试过"登记宿主背板 + CTLCOLOR 返回 HOLLOW"，
                // 实测无效：控件窗口的底色仍由 WM_CTLCOLORSTATIC 纯色刷子决定，透明只会露出窗口底。
                // 渐变要真落地，必须先统一"背板由谁画"（当前分裂为容器背板层与控件刷子两套）。
                // 契约层（BgImage 属性解析/回读）已就位，等背板归属收敛后直接接画即可。
                // 父是 flex 容器：改成真子窗口（裁剪/层叠/焦点链都正确）。
                // 开关 PYARX_FLEX_PARENT：**默认开**；显式 =0/n/N 才退回老的扁平兄弟结构
                // （仅用于排查历史界面的层级相关差异）。SetParent 只改父、不改 hwnd，挂在
                // hwnd 上的事件/皮肤 prop 不丢；换父后由 WM_SIZE -> reflowItems 按容器
                // 客户区重排，宿主 moveControl 负责把宿主坐标映射到容器父坐标系。
                static const bool kRealParent = RealParentEnabled();
                if (kRealParent)
                {
                    debugLogf(L"[flex-parent] child=%ls node=%ls nodeTag=%ls flexHwnd=%p self=%p",
                              child.name.c_str(), node.name.c_str(), node.tag.c_str(),
                              nodeFlexHwnd, mfc->nativeHandle());
                }
                if (kRealParent && nodeFlexHwnd != nullptr && mfc->nativeHandle() != nullptr)
                {
                    ::SetParent(static_cast<HWND>(mfc->nativeHandle()), nodeFlexHwnd);
                    if (nodeClipsOverflow)
                    {
                        // 普通 div 没有 reflowItems：直接把子项换算成"容器内相对坐标"再摆位，
                        // 超出容器的部分由系统按父客户区裁掉。
                        ::SetWindowPos(static_cast<HWND>(mfc->nativeHandle()), nullptr,
                                       x - node.previewX, y - node.previewY, w, h,
                                       SWP_NOZORDER | SWP_NOACTIVATE);
                    }
                    else
                    {
                    // SetParent 只改父、不改坐标数值：控件原本的 x/y 是按"宿主客户区"算的，
                    // 换父之后同一数值会被解释成"相对容器"，位置就跑到容器外被裁掉。
                    // 挂在容器上以后必须让容器按自己的客户区重排一次（WM_SIZE -> reflowItems），
                    // 子项才会落回容器内的正确位置。
                    RECT frc = {};
                    ::GetClientRect(nodeFlexHwnd, &frc);
                    ::SendMessageW(nodeFlexHwnd, WM_SIZE, SIZE_RESTORED,
                                   MAKELPARAM(static_cast<WORD>(frc.right), static_cast<WORD>(frc.bottom)));
                    }
                }
                // 记下创建序：Tab 序在 TabIndex 相同时按它兜底
                if (window != nullptr)
                    window->Host()->noteCreated(id);
                else if (page != nullptr)
                    page->Host()->noteCreated(id);

                // Ribbon：把整棵 Tab/Group/Button 子树交给宿主 ribbonParse 建模型
                if (tag == "ribbon" && host != nullptr)
                    host->setRibbonElement(id, &child);
                // FileTab：把整棵 FileTabItem 子树交给宿主 filetabParse 建模型
                if (tag == "filetab" && host != nullptr)
                    host->setFileTabElement(id, &child);

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
                          xmlapp::IsAppearanceProperty(kv.first.c_str()) ? "外观" : "其他");
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

            // 叠放次序（CSS z-index）：样式里的 z-index 由 XmlCore 翻成 ZIndex 属性，
            // 老写法 Level="N" 一并在首次出现处落到宿主。控件层的 setProperty 是否接收
            // ZIndex 取决于控件实现，宿主这张表才是叠放的唯一真相，所以在这里直接下发。
            {
                IXmlMfcHost* host = window != nullptr
                    ? static_cast<IXmlMfcHost*>(window->Host())
                    : static_cast<IXmlMfcHost*>(page->Host());
                int z = 0;
                bool hasZ = false;
                auto pickZ = [&](const std::vector<std::pair<std::wstring, std::wstring>>& lst) {
                    for (const auto& kv : lst)
                        if (::_wcsicmp(kv.first.c_str(), L"ZIndex") == 0 ||
                            ::_wcsicmp(kv.first.c_str(), L"Level") == 0 ||
                            ::_wcsicmp(kv.first.c_str(), L"zindex") == 0)
                        {
                            z = _wtoi(kv.second.c_str());
                            hasZ = true;
                        }
                };
                pickZ(child.attrs);
                pickZ(child.styledAttrs);
                if (host != nullptr && hasZ)
                    host->setControlLevel(id, z);

                // CSS cursor（XmlCore 已把 style 的 cursor 翻成 Cursor 属性）
                std::wstring cur;
                auto pickCur = [&](const std::vector<std::pair<std::wstring, std::wstring>>& lst) {
                    for (const auto& kv : lst)
                        if (::_wcsicmp(kv.first.c_str(), L"Cursor") == 0)
                            cur = kv.second;
                };
                pickCur(child.attrs);
                if (cur.empty())
                    pickCur(child.styledAttrs);
                if (host != nullptr && !cur.empty())
                    host->setControlCursor(id, cur);

                // CSS pointer-events（style 里写的 none → 点击穿透；auto 回正常）
                std::wstring pe;
                auto pickPe = [&](const std::vector<std::pair<std::wstring, std::wstring>>& lst) {
                    for (const auto& kv : lst)
                        if (::_wcsicmp(kv.first.c_str(), L"PointerEvents") == 0)
                            pe = kv.second;
                };
                pickPe(child.attrs);
                if (pe.empty())
                    pickPe(child.styledAttrs);
                if (host != nullptr && !pe.empty())
                    host->setControlPointerEvents(id, pe);

                // 可见性：HTML 的 `visibility:hidden` / `display:none` / `hidden` 属性与 XML
                // 的 `Visible="False"` 都是"建出来但不显示"。此前这些键只被"认名字"，从来
                // 没有消费者 —— 写了 Visible="False" 控件照样在屏幕上，某些形状下还挡住兄弟。
                {
                    auto pickVis = [&](const std::vector<std::pair<std::wstring, std::wstring>>& lst,
                                       bool& has, bool& vis) {
                        for (const auto& kv : lst)
                        {
                            if (::_wcsicmp(kv.first.c_str(), L"Visible") == 0 ||
                                ::_wcsicmp(kv.first.c_str(), L"IsVisible") == 0)
                            {
                                const std::wstring& s = kv.second;
                                vis = !(::_wcsicmp(s.c_str(), L"false") == 0 || s == L"0" ||
                                        ::_wcsicmp(s.c_str(), L"hidden") == 0 ||
                                        ::_wcsicmp(s.c_str(), L"none") == 0);
                                has = true;
                            }
                            else if (::_wcsicmp(kv.first.c_str(), L"Display") == 0)
                            {
                                if (::_wcsicmp(kv.second.c_str(), L"none") == 0)
                                {
                                    vis = false;
                                    has = true;
                                }
                            }
                        }
                    };
                    bool hasVis = false, visVal = true;
                    pickVis(child.attrs, hasVis, visVal);
                    pickVis(child.styledAttrs, hasVis, visVal);
                    // CSS visibility:hidden —— 和 display:none 是两件事：保留占位、只是不画。
                    // 布局阶段已经按几何摆好，所以这里只把窗口藏起来，兄弟位置保持不动。
                    bool hasCssVis = false, cssVisVal = true;
                    auto pickCssVis = [&](const std::vector<std::pair<std::wstring, std::wstring>>& lst) {
                        for (const auto& kv : lst)
                        {
                            if (::_wcsicmp(kv.first.c_str(), L"Visibility") != 0)
                                continue;
                            const std::wstring& s = kv.second;
                            cssVisVal = !(::_wcsicmp(s.c_str(), L"hidden") == 0 ||
                                          ::_wcsicmp(s.c_str(), L"collapse") == 0 ||
                                          ::_wcsicmp(s.c_str(), L"false") == 0);
                            hasCssVis = true;
                        }
                    };
                    pickCssVis(child.attrs);
                    pickCssVis(child.styledAttrs);
                    if (!hasVis && hasCssVis)
                    {
                        hasVis = true;
                        visVal = cssVisVal;
                    }
                    if (host != nullptr && hasVis)
                        host->setVisible(id, visVal);
                    // HTML 的 `disabled` / XML 的 `Enabled="False"`：必须真的把窗口禁用（WS_DISABLED）。
                    // 早先这两个键只被 CSS 选择器认名字、没有任何消费者，于是 `<button disabled>`
                    // 照样能点、也拿不到禁用态的绘制分支 —— 与 HTML 语义不符。
                    if (host != nullptr)
                    {
                        bool hasEn = false, en = true;
                        auto pickEn = [&](const std::vector<std::pair<std::wstring, std::wstring>>& lst) {
                            for (const auto& kv : lst)
                            {
                                const bool isEn = ::_wcsicmp(kv.first.c_str(), L"Enabled") == 0 ||
                                                  ::_wcsicmp(kv.first.c_str(), L"IsEnabled") == 0;
                                const bool isDis = ::_wcsicmp(kv.first.c_str(), L"Disabled") == 0;
                                if (!isEn && !isDis)
                                    continue;
                                const std::wstring& s = kv.second;
                                // 真假判定：宽/详两套拼写都在一处收敛（`enabled="1"` 与
                                // `disabled="1"` 语义相反，不能只看字符串本身）
                                const bool truthy = !(::_wcsicmp(s.c_str(), L"false") == 0 ||
                                                      s == L"0" || ::_wcsicmp(s.c_str(), L"no") == 0 ||
                                                      ::_wcsicmp(s.c_str(), L"off") == 0);
                                en = isDis ? !truthy : truthy;
                                hasEn = true;
                            }
                        };
                        pickEn(child.attrs);
                        pickEn(child.styledAttrs);
                        if (hasEn)
                            host->setEnabled(id, en);
                        // 只读同样两套拼写收敛到一处：XML 的 ReadOnly/IsReadOnly 与
                        // HTML 的 readonly 都下发到同一个 setReadOnly（否则 `readonly="1"`
                        // 在 XML 控件上是"写了等于没写"，得改用内部名才生效）
                        bool hasRo = false, ro = false;
                        auto pickRo = [&](const std::vector<std::pair<std::wstring, std::wstring>>& lst) {
                            for (const auto& kv : lst)
                            {
                                if (::_wcsicmp(kv.first.c_str(), L"ReadOnly") != 0 &&
                                    ::_wcsicmp(kv.first.c_str(), L"IsReadOnly") != 0 &&
                                    ::_wcsicmp(kv.first.c_str(), L"readonly") != 0)
                                    continue;
                                const std::wstring& s = kv.second;
                                ro = !(::_wcsicmp(s.c_str(), L"false") == 0 || s == L"0" ||
                                       ::_wcsicmp(s.c_str(), L"no") == 0);
                                hasRo = true;
                            }
                        };
                        pickRo(child.attrs);
                        pickRo(child.styledAttrs);
                        if (hasRo)
                            host->setReadOnly(id, ro);
                    }
                    // CSS position:fixed —— 在 overflow 容器里不随内容滚动。
                    // 几何仍由 left/top 决定，这里只把"钉住"的控件登记给宿主。
                    if (host != nullptr)
                    {
                        bool hasPos = false, isFixed = false;
                        auto pickPos = [&](const std::vector<std::pair<std::wstring, std::wstring>>& lst) {
                            for (const auto& kv : lst)
                            {
                                if (::_wcsicmp(kv.first.c_str(), L"Position") != 0)
                                    continue;
                                isFixed = (::_wcsicmp(kv.second.c_str(), L"fixed") == 0);
                                hasPos = true;
                            }
                        };
                        pickPos(child.attrs);
                        pickPos(child.styledAttrs);
                        if (hasPos && isFixed)
                            host->markFixedChild(id);
                    }
                    // CSS animation：Keyframes 给帧表，animation 简写给时长/次数。
                    // HTML 的关键帧写在 @keyframes 块，XML 用属性一行替代，少一层嵌套。
                    if (host != nullptr)
                    {
                        std::wstring kf, anim, durStr, iterStr;
                        auto pickAnim = [&](const std::vector<std::pair<std::wstring, std::wstring>>& lst) {
                            for (const auto& kv : lst)
                            {
                                if (::_wcsicmp(kv.first.c_str(), L"Keyframes") == 0) kf = kv.second;
                                else if (::_wcsicmp(kv.first.c_str(), L"Animation") == 0) anim = kv.second;
                                else if (::_wcsicmp(kv.first.c_str(), L"AnimationDuration") == 0) durStr = kv.second;
                                else if (::_wcsicmp(kv.first.c_str(), L"AnimationIteration") == 0) iterStr = kv.second;
                            }
                        };
                        pickAnim(child.attrs);
                        pickAnim(child.styledAttrs);
                        if (!kf.empty())
                        {
                            int dur = 300;
                            auto readDur = [](const std::wstring& s) -> int {
                                const size_t ms = s.find(L"ms");
                                const size_t sec = s.find(L's');
                                if (ms != std::wstring::npos)
                                    return static_cast<int>(_wtof(s.c_str()));
                                if (sec != std::wstring::npos)
                                    return static_cast<int>(_wtof(s.c_str()) * 1000.0);
                                return 0;
                            };
                            if (!durStr.empty())
                            {
                                int d = readDur(durStr);
                                if (d > 0) dur = d;
                            }
                            else
                            {
                                // 从简写里挑时间 token（`pulse 900ms linear infinite`）
                                std::wstring cur;
                                for (wchar_t c : anim)
                                {
                                    if (iswspace(c))
                                    {
                                        if (!cur.empty())
                                        {
                                            int d = readDur(cur);
                                            if (d > 0) dur = d;
                                            cur.clear();
                                        }
                                    }
                                    else cur.push_back(c);
                                }
                                if (!cur.empty())
                                {
                                    int d = readDur(cur);
                                    if (d > 0) dur = d;
                                }
                            }
                            bool loop = true;
                            if (!iterStr.empty() && iterStr != L"infinite")
                                loop = _wtoi(iterStr.c_str()) > 1;
                            else if (anim.find(L"infinite") == std::wstring::npos && !iterStr.empty())
                                loop = false;
                            host->startCssKeyframes(id, kf, dur, loop);
                        }
                    }
                }
            }

            // 事件绑定（S2）：按契约表统一绑定，不再写死三种。
            // 回调名解析不到就报错（绝不静默忽略）；宿主还不能派发的事件告警后跳过。
            const std::string idText = std::to_string(id);
            for (const XmlEventInfo& info : xmlEventInfos())
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
                // 控件类型本身不产生该事件：绑定即使成功也永远不触发。
                // 缺省只告警（存量 XML 不因此崩），PYARX_STRICT_XML=1 时升格为加载期错误。
                if (!xmlTypeSupportsEvent(ctrl->type(), info.kind))
                {
                    const std::string why = std::string("<") + rawTag + "> id=" + idText +
                        " 声明了 " + Utf8Of(info.property) + "，但控件类型 " +
                        Utf8Of(std::wstring(ctrl->type())) + " 不会产生该事件";
                    xmlprop2::WarnUnsupportedEvent(std::wstring(ctrl->type()), info.key, cbName);
                    if (xmlprop2::StrictXmlEnabled())
                        throw std::runtime_error("guiload: " + why);
                    debugLogf(L"[event] %ls", Utf8ToWide(why).c_str());
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
            // 容器就只剩一个空矩形。XML 阶段坐标已算成绝对坐标，直接在同一宿主上继续建。
            // Ribbon 子节点（Tab/Group/Button）由 ribbonParse 解析，不再逐个当独立控件建
            // FileTab 子节点（FileTabItem）由 filetabParse 解析，同上
            if (!child.children.empty() && tag != "ribbon" && tag != "filetab")
                LoadControlsFromElement(child, window, page, context, nextId, names, nodeAlign);

            // 子树建完 → 控件层 flex 容器认领自己的子项（子项 id 这时才齐），
            // 之后容器自己响应尺寸变化重排，宿主不再需要整树 reflowXml
            if (child.control != nullptr)
            {
                std::shared_ptr<XmlRuntime> rt = window != nullptr ? window->Host()->xmlRuntime()
                                                                  : (page != nullptr ? page->Host()->xmlRuntime() : nullptr);
                if (rt)
                    rt->attachFlexContainer(child);
            }
        }
    }

    // ItemView 的 model 改变：按新行数重建行控件。
    // 行控件的生命周期不能交给 runtime（它只认元素树，不知道 HWND），所以三步走：
    //   1) 收旧行全部 x:Name→id，销毁 HWND 并从名字表摘掉（同名残留会挡住新 id）
    //   2) 让 runtime 按新 Model 重新展开行元素
    //   3) 走与加载时同一条 LoadControlsFromElement 通路重建行内控件
    bool RebuildItemViewRows(PyMfcHost* host, const std::wstring& name)
    {
        if (host == nullptr)
            return false;
        std::shared_ptr<XmlRuntime> rt = host->xmlRuntime();
        if (!rt)
            return false;
        XmlElement* root = rt->elementByName(name);
        if (root == nullptr)
            return false;

        auto* ownerWindow = static_cast<PyMfcWindow*>(host->xmlOwnerWindow());
        auto* ownerPage = static_cast<PyMfcPanel*>(host->xmlOwnerPage());
        if (ownerWindow == nullptr && ownerPage == nullptr)
            return false;

        py::dict& names = host->xmlNames();

        std::vector<std::wstring> oldNames;
        std::function<void(XmlElement&)> collect = [&](XmlElement& el) {
            if (!el.name.empty())
                oldNames.push_back(el.name);
            for (auto& c : el.children)
                collect(c);
        };
        for (auto& c : root->children)
            collect(c);

        for (const std::wstring& n : oldNames)
        {
            const std::string key = Utf8Of(n);
            if (!names.contains(key.c_str()))
                continue;
            int id = 0;
            try
            {
                id = names[key.c_str()].cast<int>();
            }
            catch (...)
            {
                PyErr_Clear();
            }
            if (id != 0)
                host->removeControlById(id);
            names.attr("pop")(py::str(key), py::none());
        }

        if (!rt->refreshItemView(name))
            return false;

        // 先重排：行元素的新坐标要算出来，后面建控件才知道自己该在哪一行
        host->reflowXml();

        int nextId = host->xmlNextId();
        LoadControlsFromElement(*root, ownerWindow, ownerPage, host->xmlContext(),
                                &nextId, &names);
        host->xmlNextId() = nextId;
        host->applyLevelsAndTabs();
        if (HWND h = host->GetSafeHwnd())
            ::InvalidateRect(h, nullptr, TRUE);
        return true;
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
            // Ribbon 内部结构节点（Tab/Group/Button/MenuItem/Backstage）由 ribbonParse
            // 自行遍历子树，运行时加载层不为它们创建原生控件
            if (tag == "ribbontab" || tag == "ribbongroup" || tag == "ribbonbutton" ||
                tag == "ribbonmenuitem" || tag == "ribbonbackstage" ||
                tag == "ribbonbackstageitem")
                continue;
            if (tag == "filetabitem" || tag == "xfiletabitem")
                continue;
            const py::dict attrs = child.attr("attrib").cast<py::dict>();

            const std::wstring text = AttrWStringL(attrs, {"Text", "Content", "Header"});
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
            // ElementTree 会把 x:Name 展开成 {xml-uri}Name，本地名就是 Name，故两种写法都认
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
                const bool readOnly = AttrIntL(attrs, {"ReadOnly", "IsReadOnly", "readonly"}, 0) != 0;
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
            else if (tag == "toolbar" || tag == "xtoolbar" || tag == "toolstrip")
            {
                // 自绘工具条按钮条（kind 11 = XmlSkinKind::ToolBar）：
                // 宿主负责画按钮格子、选中态和悬停，条目由 Items 用 | 分隔。
                // 以前这个标签没有创建分支，整条 ToolBar 会被静默丢弃。
                if (window) window->addSkinBox(id, 11, x, y, w, h);
                else page->addSkinBox(id, 11, x, y, w, h);

                const std::wstring items = AttrWStringL(attrs, {"Items"});
                std::wstring buffer;
                for (wchar_t c : items)
                {
                    if (c == L'|' || c == L'\n' || c == L'\r')
                    {
                        if (!buffer.empty())
                        {
                            if (window) window->addToolBarItem(id, buffer);
                            else page->addToolBarItem(id, buffer);
                            buffer.clear();
                        }
                    }
                    else
                    {
                        buffer.push_back(c);
                    }
                }
                if (!buffer.empty())
                {
                    if (window) window->addToolBarItem(id, buffer);
                    else page->addToolBarItem(id, buffer);
                }

                if (AttrHas(attrs, {"SelectedIndex"}))
                {
                    const int sel = AttrIntL(attrs, {"SelectedIndex"}, -1);
                    if (window) window->setToolBarSelected(id, sel);
                    else page->setToolBarSelected(id, sel);
                }
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

            // 外观：XML 上声明的 Background / Foreground / Radius / TextAlign / FontSize…
            // （style 里的 CSS 键已在 XmlCore 侧翻成这些属性）必须推给宿主。
            // 此前控件级 setControlAppearance 从未被调用，owner-draw 的 findAppearance 恒空，
            // 等于 <button style="background:#2563eb;border-radius:8px;color:#fff"> 全部被丢掉。
            {
                IXmlMfcHost* host = window != nullptr
                    ? static_cast<IXmlMfcHost*>(window->Host())
                    : static_cast<IXmlMfcHost*>(page->Host());
                if (host != nullptr)
                {
                    xmlapp::XmlAppearance ap;
                    for (auto kv : attrs)
                    {
                        const std::string k = py::str(kv.first).cast<std::string>();
                        const std::string v = py::isinstance<py::str>(kv.second)
                                                  ? kv.second.cast<std::string>()
                                                  : py::str(kv.second).cast<std::string>();
                        xmlapp::SetProperty(ap, Utf8ToWide(k), Utf8ToWide(v));
                    }
                    if (!ap.empty())
                    {
                        host->setControlAppearance(id, ap);
                        // 原生输入框（EDIT/RICHEDIT）的文字由系统按 WM_SETFONT 那把 HFONT 画，
                        // 外观表的字号/字体名此前只影响自绘件 → 输入框恒 10pt。
                        // 这里把 XML 声明的 font-family/font-size/font-weight 一并下发，
                        // 保证"不聚焦"和"点进去"用同一把字体，不再点一下就换字形。
                        if (ap.fontSize > 0 || !ap.fontFamily.empty())
                        {
                            bool bold = false;
                            if (!ap.fontWeight.empty())
                            {
                                if (_wcsicmp(ap.fontWeight.c_str(), L"bold") == 0 ||
                                    _wcsicmp(ap.fontWeight.c_str(), L"bolder") == 0)
                                    bold = true;
                                else if (_wtoi(ap.fontWeight.c_str()) >= 600)
                                    bold = true;
                            }
                            host->setControlFont(
                                id, ap.fontFamily.empty() ? L"Segoe UI" : ap.fontFamily,
                                ap.fontSize > 0 ? ap.fontSize : 10, bold);
                        }
                    }
                }
            }

            // 叠放次序（CSS z-index）：样式里的 z-index 由 XmlCore 翻成 ZIndex 属性，
            // 老写法 Level="N" 也在这里统一落到宿主。此前这条属性只被"认名字"、从未下发，
            // 自绘件于是永远按注册次序画 —— HTML 里能靠 z-index 翻层的效果 XML 做不到。
            {
                IXmlMfcHost* host = window != nullptr
                    ? static_cast<IXmlMfcHost*>(window->Host())
                    : static_cast<IXmlMfcHost*>(page->Host());
                if (host != nullptr && AttrHas(attrs, {"ZIndex", "Level", "zindex"}))
                    host->setControlLevel(id, AttrIntL(attrs, {"ZIndex", "Level", "zindex"}, 0));
            }

            // 事件绑定（S2）：与 Element 路径同一张契约表
            const std::string idText = std::to_string(id);
            for (size_t ei = 0; ei < xmlEventInfos().size(); ++ei)
            {
                const XmlEventInfo& info = xmlEventInfos()[ei];
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

    // ---- 自定义控件（Python 侧实现，C++ 提供 IXmlControl 统一接口）----
    // register_control(tag, factory)：factory 是 Python 可调用，返回自定义控件实例。
    // 实例约定实现（duck-typing）：
    //   create(hwnd) -> bool          创建原生控件（hwnd 为宿主 HWND）
    //   destroy()                     销毁
    //   set_property(name:str, value:str) -> bool
    //   get_property(name:str) -> str
    //   native_handle() -> int        返回控件 HWND
    //   type() -> str                 可选，默认返回注册标签
    // C++ 侧用 PyXmlControlAdapter 桥接到 IXmlControl，guiload 走注册表统一创建。
    class PyXmlControlAdapter : public IXmlControl
    {
    public:
        explicit PyXmlControlAdapter(std::shared_ptr<py::object> obj, std::wstring type)
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
            if (name != nullptr && xmlapp::SetProperty(m_ap, name, value))
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
            if (name != nullptr && xmlapp::IsAppearanceProperty(name))
            {
                std::wstring ap = xmlapp::GetProperty(m_ap, name);
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

        std::vector<XmlProperty> properties() const override
        {
            // 自定义控件与标准控件共用同一套外观契约
            return xmlapp::Properties();
        }

        bool bindEvent(const wchar_t* kind, void* callback) override
        {
            (void)kind;
            (void)callback;
            return true;
        }

        const xmlapp::XmlAppearance& appearance() const { return m_ap; }

    private:
        std::shared_ptr<py::object> m_obj;
        std::wstring m_type;
        mutable std::wstring m_typeCache;
        xmlapp::XmlAppearance m_ap;
    };

    // register_control(tag, factory)：把 Python 工厂注册进统一注册表



}

// 供 PyMfcHost 在 ItemView 的 model 改变时调用（同一 DLL，不能留在匿名命名空间里）
bool PyMfcRebuildItemView(PyMfcHost* host, const std::wstring& name)
{
    return RebuildItemViewRows(host, name);
}

    py::dict XmlLayoutRects(const std::string& path, int width, int height,
                             py::object overrides)
    {
        XmlRuntime rt;
        std::wstring loadError;
        if (!rt.loadFile(Utf8ToWide(path).c_str(), loadError))
            throw std::runtime_error("xmlLayout: " + Utf8Of(loadError));

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
                XmlElement* el = FindXmlByName(rt.root(), Utf8ToWide(parts[0]));
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
        std::function<void(XmlElement&)> walk = [&](XmlElement& el) {
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
        auto& reg = XmlControlRegistry::instance();
        bool ok = reg.registerFactory(tagW.c_str(), [holder, tagW]() -> IXmlControl* {
            try
            {
                py::object obj = (*holder)();
                return new PyXmlControlAdapter(
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

    // 片段根没写 Height：按内容算（量法和布局同源，都在 XmlCore 的 xmlNaturalHeight）
    // 片段根自己常常只是 <html>/<body> 这层壳，宽高写在里层（style="width:420"）。
    // 沿单链往下找第一个显式宽度，找不到再退回默认。
    static int ExplicitWidthDeep(const XmlElement& e, int depth)
    {
        if (e.hasW && e.w > 0)
            return e.w;
        if (depth <= 0)
            return 0;
        for (const auto& c : e.children)
        {
            const int w = ExplicitWidthDeep(c, depth - 1);
            if (w > 0)
                return w;
        }
        return 0;
    }

    static int FragmentRootHeight(XmlRuntime&, XmlElement& root, int width)
    {
        (void)width;
        return xmlNaturalHeight(root);
    }

    // path 为空、xmlText 非空 = 从字符串建界面（HTML 式一行出控件用这条路）
    static py::object GuiloadImpl(const std::string& path, const std::string& xmlText,
                                  py::object target, py::object context,
                                  py::object names, int firstId, py::object palette)
    {
        // 统一运行时：XmlRuntime 解析 + 布局，控件经注册表创建。
        // 用 shared_ptr 而不是栈上对象：加载完要交给宿主持管，
        // 否则改一个 Width 也没人能表达式重算、把控件挪位（runtime 一析构就全丢）。
        auto runtimeOwned = std::make_shared<XmlRuntime>();
        XmlRuntime& runtime = *runtimeOwned;
        std::wstring loadError;
        {
        PyMfcPerfScope _p(&pyMfcPerf().parseMs, &pyMfcPerf().nParse);
        if (!xmlText.empty())
        {
            // 字符串入口没有文件基准目录：ResourceDictionary 的 Source 相对路径不可用，
            // 需要跨文件样式时仍走 guiload(path)。
            if (!runtime.loadString(Utf8ToWide(xmlText).c_str(), loadError))
                throw std::runtime_error("guiload_text: " + Utf8Of(loadError));
        }
        else
        {
            std::wstring widePath = Utf8ToWide(path);
            if (!runtime.loadFile(widePath.c_str(), loadError))
                throw std::runtime_error("guiload: " + Utf8Of(loadError));
        }
        }
        XmlElement* root = runtime.root();
        if (root == nullptr)
            throw std::runtime_error("guiload: XML 根节点无效");

        // 声明层三件套必须先落地：样式 / 控件模板 / 属性绑定（含 Trigger）。
        // 少了这一步，运行时不认 Style、Template、{Binding}，只有设计器和预览器好看。
        {
            PyMfcPerfScope _p(&pyMfcPerf().declMs, &pyMfcPerf().nDecl);
            runtime.applyDeclarations();
        }

        // HTML 的 <img> 不写 width/height 时按原图自然尺寸占位（intrinsic size）。
        // 必须在布局之前把尺寸写进 Width/Height：布局阶段就会按它算盒子，
        // 事后再 setBounds 会被随后的重排（show/onResize）用布局值覆盖回去。
        {
            std::function<void(XmlElement&)> natural = [&](XmlElement& el) {
                if (::_wcsicmp(el.tag.c_str(), L"XImage") == 0 && el.get(L"Width").empty() &&
                    el.get(L"Height").empty() && !el.get(L"Image").empty())
                {
                    int nw = 0, nh = 0;
                    if (pyMfcImagePixelSize(el.get(L"Image"), nw, nh))
                    {
                        el.set(L"Width", std::to_wstring(nw));
                        el.set(L"Height", std::to_wstring(nh));
                        // 记下原图尺寸：布局被 max-width:100% 夹窄时要按这个比例缩高
                        el.set(L"NaturalWidth", std::to_wstring(nw));
                        el.set(L"NaturalHeight", std::to_wstring(nh));
                    }
                }
                for (auto& c : el.children)
                    natural(c);
            };
            natural(*root);
        }

        const std::wstring rootRawTag = root->tag;
        const std::string rootKind = StdTagKind(Utf8Of(rootRawTag));

        // 根节点决定容器类型；除此之外一律按"片段根"处理（HTML 式写法）：
        // 直接写 <XStackPanel>… 或一个控件，都自动包一层 Window，高度按内容量。
        const std::string type = rootKind;
        const bool fragmentRoot = (type != "window" && type != "palette");

        // 片段根开箱即用的呼吸感：没写 Spacing/Padding 就按 8 / 12 排
        // （只作用于片段根，既有 <XWindow> 的 XML 布局一个像素都不动）
        if (fragmentRoot && !root->children.empty())
        {
            if (root->get(L"Spacing").empty())
                root->spacing = 8;
            if (root->get(L"Padding").empty() && root->padL == 0 && root->padT == 0)
                root->padL = root->padR = root->padT = root->padB = 12;
        }

        py::dict ctx = py::isinstance<py::dict>(context) ? context.cast<py::dict>() : CallerGlobals();
        py::dict nameMap = py::isinstance<py::dict>(names) ? names.cast<py::dict>() : py::dict();
        int nextId = firstId < 1000 ? 1000 : firstId;

        if (type == "window" || fragmentRoot)
        {
            int rootWidth = AttrIntOf(*root, L"Width", fragmentRoot ? 320 : 420);
            int rootHeight = fragmentRoot
                ? FragmentRootHeight(runtime, *root, rootWidth)
                : AttrIntOf(*root, L"Height", 320);
            // 片段根的 Width/Height 说的是"内容区"尺寸：先留一份原值，
            // 建完窗口后按真实非客户区再校准，避免"内容高被裁掉 1~2px"。
            const int contentW = rootWidth, contentH = rootHeight;
            if (fragmentRoot)
            {
                // create 用的是外框尺寸，这里把边框与标题栏折进去，
                // 免得自动高度被裁掉最后一行。
                RECT adj = {0, 0, rootWidth, rootHeight};
                ::AdjustWindowRectEx(&adj,
                                     WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
                                     FALSE, WS_EX_APPWINDOW | WS_EX_COMPOSITED);
                rootWidth = adj.right - adj.left;
                rootHeight = adj.bottom - adj.top;
            }
            bool createdByUs = false;
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
                createdByUs = true;
                const std::wstring theme = AttrWOf(*root, L"Theme");
                window->setTheme(!(theme == L"light"));
                window->markSilentBuild();   // 建完控件树再由 endSilentBuild() 显示
            }

            // 片段根：上面的 AdjustWindowRectEx 用的是"我们假设的样式"，
            // 而窗口类真实的非客户区（标题栏/边框）可能差 1~2px，
            // 结果内容区高度比算出来的少 2px（t_xml_ergonomics 里 120→118、32→30）。
            // 建完窗口后按真实 style/exstyle 再校准一次：客户区精确等于内容尺寸。
            if (fragmentRoot && createdByUs)
            {
                HWND hw = window->Host()->GetSafeHwnd();
                if (hw != nullptr)
                {
                    RECT want = {0, 0, contentW, contentH};
                    ::AdjustWindowRectEx(&want,
                                         static_cast<DWORD>(::GetWindowLongPtrW(hw, GWL_STYLE)),
                                         FALSE,
                                         static_cast<DWORD>(::GetWindowLongPtrW(hw, GWL_EXSTYLE)));
                    const int ww = want.right - want.left;
                    const int wh = want.bottom - want.top;
                    RECT cur = {};
                    ::GetWindowRect(hw, &cur);
                    if (cur.right - cur.left != ww || cur.bottom - cur.top != wh)
                        ::SetWindowPos(hw, nullptr, 0, 0, ww, wh,
                                       SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
                }
            }

            try
            {
                // 布局：以窗口客户区为根矩形
                CRect rc;
                HWND hostHwnd = window->Host()->GetSafeHwnd();
                ::GetClientRect(hostHwnd, &rc);
                {
                    PyMfcPerfScope _p(&pyMfcPerf().layoutMs, &pyMfcPerf().nLayout);
                    runtime.layout(0, 0, rc.Width(), rc.Height());
                }

                // 容器铺底：先清上一轮，再把根节点自己声明的 Background 铺满客户区
                window->Host()->clearContainerBackdrops();
                const xmlapp::XmlAppearance rootAp = AppearanceOf(*root);
                if (!rootAp.empty())
                    window->Host()->addContainerBackdrop(0, 0, rc.Width(), rc.Height(), rootAp);
                COLORREF pageBg = 0;
                if (!rootAp.backColor.empty() && xmlapp::ParseColor(rootAp.backColor, pageBg))
                {
                    window->Host()->setPageBackdrop(pageBg, true);
                    window->setBackground(GetRValue(pageBg), GetGValue(pageBg), GetBValue(pageBg));
                }

                {
                PyMfcPerfScope _p(&pyMfcPerf().loadMs, &pyMfcPerf().nLoad);
                if (fragmentRoot)
                {
                    // 片段根：根本身可能就是一个控件，也可能是个容器。
                    // 容器直接按"子元素"加载——必须用真实树上的节点，
                    // 否则自动分配的 Name 落在副本上，运行时重排/改属性都找不到控件。
                    const std::wstring kind = Utf8ToWide(StdTagKind(Utf8Of(root->tag)));
                    const bool rootIsContainer =
                        kind == L"stackpanel" || kind == L"grid" || kind == L"dockpanel" ||
                        kind == L"wrappanel" || kind == L"canvas" || kind == L"menubar" ||
                        kind == L"statusbar";
                    if (rootIsContainer)
                    {
                        LoadControlsFromElement(*root, window, nullptr, ctx, &nextId, &nameMap);
                    }
                    else
                    {
                        // 根是一个控件：包一层假容器走同一条加载路径，再把自动名字写回真节点
                        XmlElement holder;
                        holder.spacing = 0;
                        holder.padL = holder.padT = holder.padR = holder.padB = 0;
                        holder.children.push_back(*root);
                        LoadControlsFromElement(holder, window, nullptr, ctx, &nextId, &nameMap);
                        if (!holder.children.empty() && root->name.empty())
                            root->name = holder.children.front().name;
                    }
                }
                else
                {
                    LoadControlsFromElement(*root, window, nullptr, ctx, &nextId, &nameMap);
                }

                // 整棵控件树建完 → 控件层 flex 容器认领自己的子项（子项 id 这时才齐）。
                // 之后容器自己响应尺寸变化重排，宿主不需要整树 reflowXml。
                {
                    std::function<void(XmlElement&)> attachAll = [&](XmlElement& el) {
                        for (auto& c : el.children)
                            attachAll(c);
                        runtime.attachFlexContainer(el);
                    };
                    attachAll(*root);
                }

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
                window->Host()->attachXml(runtimeOwned, ids, nameMap, ctx, nextId,
                                           static_cast<void*>(window), nullptr);
                // 层次与 Tab 序：等整棵树建完再统一排一次，避免边建边动 z 序
                window->Host()->applyLevelsAndTabs();
                // 控件树齐了再一次性亮相（Embedded 窗口创建时是隐藏的）
                window->endSilentBuild();
                }
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
                    xmlLayoutSubtree(child, 0, 0, contentWidth, height);

                    panel->Host()->clearContainerBackdrops();
                    const xmlapp::XmlAppearance pageAp = AppearanceOf(child);
                    if (!pageAp.empty())
                        panel->Host()->addContainerBackdrop(0, 0, contentWidth, height, pageAp);
                    COLORREF pageBg = 0;
                    if (!pageAp.backColor.empty() && xmlapp::ParseColor(pageAp.backColor, pageBg))
                        panel->Host()->setPageBackdrop(pageBg, true);

                    LoadControlsFromElement(child, nullptr, panel, ctx, &nextId, &nameMap);
                }

                // 根节点下直接写控件也支持：自动建一页
                if (!hasPage)
                {
                    PyMfcPanel* panel = openPage(L"page");
                    xmlLayoutSubtree(*root, 0, 0, width - kFlowMargin * 2, height);
                    panel->Host()->clearContainerBackdrops();
                    const xmlapp::XmlAppearance pageAp = AppearanceOf(*root);
                    if (!pageAp.empty())
                        panel->Host()->addContainerBackdrop(0, 0, width - kFlowMargin * 2, height, pageAp);
                    COLORREF pageBg = 0;
                    if (!pageAp.backColor.empty() && xmlapp::ParseColor(pageAp.backColor, pageBg))
                        panel->Host()->setPageBackdrop(pageBg, true);
                    LoadControlsFromElement(*root, nullptr, panel, ctx, &nextId, &nameMap);
                }
            }

            palette.attr("show")();
            return palette;
        }


        throw std::runtime_error("guiload: 不支持的根节点 <" + Utf8Of(rootRawTag) + ">");
    }

    // 文件入口（原 guiload）
    py::object Guiload(const std::string& path, py::object target, py::object context,
                       py::object names, int firstId, py::object palette)
    {
        return GuiloadImpl(path, std::string(), target, context, names, firstId, palette);
    }

    // 字符串入口（guiload_text）：同一套解析/布局/注册表，只是不落盘
    py::object GuiloadText(const std::string& xmlText, py::object target, py::object context,
                           py::object names, int firstId, py::object palette)
    {
        if (xmlText.empty())
            throw std::runtime_error("guiload_text: XML 字符串为空");
        return GuiloadImpl(std::string(), xmlText, target, context, names, firstId, palette);
    }

