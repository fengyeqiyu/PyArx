// XmlViewer —— XML 预览与校验器（控制台 exe）
//
// 用法：
//   XmlViewer.exe <file.xml>            解析 + 校验 + 打开窗口渲染
//   XmlViewer.exe --check <file.xml>    只校验，打印错误位置（行号 + 节点路径）
//   XmlViewer.exe --catalog <path> ...   指定 control_catalog.xml
//
// 错误定位来源：
//   * MSXML 解析错误 -> 行号 / 列号 / 原因
//   * 语义校验（未知标签、枚举值非法、数值/颜色/布尔格式错误、Name 重复）-> 行号 + 节点路径
//   * 布局后的越界警告
//
// 布局与渲染严格按 XML 的 Left/Top/Width/Height/Margin/Padding/Spacing 与 Grid.Row/Column，
// 本程序不做任何位置推算。

#include <winsock2.h>   // 必须先于 windows.h：MFC 的 afxwin.h 要求
#include <windows.h>
#include <shellapi.h>
#pragma comment(lib, "shell32.lib")
#include <atlbase.h>
#include <msxml6.h>
#include <comdef.h>
#include <cwctype>
#include <cstdarg>
#include <cwchar>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <algorithm>
#include <cstdio>
#include <cstdlib>

#include "XmlCore.h"
#include "XmlAppearance.h"
#include "../MfcXmlCore/controls/XmlMfcTags.h"   // 标签规范名/别名的唯一来源

#pragma comment(lib, "msxml6.lib")

// 输出：控制台走 WriteConsoleW（中文正确），重定向走 UTF-8
static void outImpl(const wchar_t* fmt, ...)
{
    wchar_t buf[4096];
    va_list ap;
    va_start(ap, fmt);
    _vsnwprintf_s(buf, _TRUNCATE, fmt, ap);
    va_end(ap);

    HANDLE h = ::GetStdHandle(STD_OUTPUT_HANDLE);
    if (h == nullptr || h == INVALID_HANDLE_VALUE)
        return;      // 无控制台（GUI 方式启动）：静默丢弃
    DWORD mode = 0;
    if (::GetConsoleMode(h, &mode))
    {
        ::WriteConsoleW(h, buf, (DWORD)wcslen(buf), nullptr, nullptr);
        return;
    }
    int len = ::WideCharToMultiByte(CP_UTF8, 0, buf, -1, nullptr, 0, nullptr, nullptr);
    if (len <= 1) return;
    std::string s((size_t)len - 1, '\0');
    ::WideCharToMultiByte(CP_UTF8, 0, buf, -1, s.data(), len, nullptr, nullptr);
    ::WriteFile(h, s.c_str(), (DWORD)s.size(), nullptr, nullptr);
}
#define wprintf outImpl

// ---------------------------------------------------------------------------
// 基础工具
// ---------------------------------------------------------------------------
static std::wstring readAllText(const std::wstring& path)
{
    FILE* f = nullptr;
    if (_wfopen_s(&f, path.c_str(), L"rb") != 0 || f == nullptr)
        return std::wstring();

    std::string raw;
    char buf[8192];
    size_t n = 0;
    while ((n = fread(buf, 1, sizeof(buf), f)) > 0)
        raw.append(buf, n);
    fclose(f);

    if (raw.size() >= 2 && raw[0] == '\xFF' && raw[1] == '\xFE')
    {
        const wchar_t* p = reinterpret_cast<const wchar_t*>(raw.data() + 2);
        return std::wstring(p, (raw.size() - 2) / 2);
    }
    std::string body = raw;
    if (raw.size() >= 3 && raw.compare(0, 3, "\xEF\xBB\xBF") == 0)
        body = raw.substr(3);

    int len = ::MultiByteToWideChar(CP_UTF8, 0, body.c_str(), (int)body.size(), nullptr, 0);
    std::wstring out(len > 0 ? len : 0, L'\0');
    if (len > 0)
        ::MultiByteToWideChar(CP_UTF8, 0, body.c_str(), (int)body.size(), out.data(), len);
    return out;
}

static std::wstring exeDir()
{
    wchar_t buf[MAX_PATH] = {0};
    ::GetModuleFileNameW(nullptr, buf, MAX_PATH);
    std::wstring p = buf;
    size_t pos = p.find_last_of(L"\\");
    return (pos == std::wstring::npos) ? std::wstring(L".") : p.substr(0, pos);
}

// Icon 属性是相对 exe 目录的路径；空串或目录（res/）不画
static std::wstring resolveIconPath(const std::wstring& p)
{
    if (p.empty()) return std::wstring();
    if (p.back() == L'\\' || p.back() == L'/') return std::wstring();
    const bool abs = (p.size() > 2 && p[1] == L':') || p[0] == L'\\' || p[0] == L'/';
    if (abs) return p;
    return exeDir() + L"\\" + p;
}

static bool isInt(const std::wstring& s)
{
    if (s.empty()) return false;
    size_t i = 0;
    if (s[0] == L'+' || s[0] == L'-') i = 1;
    if (i >= s.size()) return false;
    for (; i < s.size(); ++i)
        if (!iswdigit(s[i])) return false;
    return true;
}

static bool isBool(const std::wstring& s)
{
    if (s == L"1" || s == L"0") return true;
    std::wstring t = s;
    for (auto& c : t) c = (wchar_t)towlower(c);
    return t == L"true" || t == L"false";
}

static bool isColor(const std::wstring& s)
{
    if (!s.empty() && s[0] == L'#')
    {
        std::wstring h = s.substr(1);
        if (h.size() != 6 && h.size() != 8) return false;
        for (wchar_t c : h)
            if (!iswxdigit(c)) return false;
        return true;
    }
    std::wstring t = s;
    for (auto& c : t) c = (wchar_t)towlower(c);
    static const std::set<std::wstring> names = {
        L"white", L"black", L"red", L"green", L"blue", L"gray", L"grey",
        L"lightgray", L"orange", L"yellow", L"transparent"
    };
    return names.count(t) > 0;
}

// ---------------------------------------------------------------------------
// 行号扫描：按文本顺序记下每个元素标签出现的行号
// ---------------------------------------------------------------------------
struct TagSite
{
    std::wstring tag;
    int line = 0;
};

static std::vector<TagSite> scanTagSites(const std::wstring& text)
{
    std::vector<TagSite> out;
    int line = 1;
    for (size_t i = 0; i < text.size(); ++i)
    {
        wchar_t c = text[i];
        if (c == L'\n') { ++line; continue; }
        if (c != L'<') continue;

        size_t j = i + 1;
        if (j >= text.size()) break;
        if (text[j] == L'/' || text[j] == L'?' || text[j] == L'!') continue;
        if (!(iswalpha(text[j]) || text[j] == L'_')) continue;

        std::wstring tag;
        while (j < text.size() &&
               (iswalnum(text[j]) || text[j] == L'_' || text[j] == L':' ||
                text[j] == L'.' || text[j] == L'-'))
        {
            tag.push_back(text[j]);
            ++j;
        }
        if (!tag.empty())
        {
            TagSite site;
            site.tag = tag;
            site.line = line;
            out.push_back(site);
        }
    }
    return out;
}

// ---------------------------------------------------------------------------
// 控件目录（tag -> 属性声明）
// ---------------------------------------------------------------------------
struct PropSpec
{
    std::wstring name;
    std::wstring type;
    std::wstring options;
};

static std::map<std::wstring, std::vector<PropSpec>> loadCatalog(const std::wstring& path)
{
    std::map<std::wstring, std::vector<PropSpec>> out;
    CComPtr<IXMLDOMDocument2> doc;
    if (FAILED(::CoCreateInstance(CLSID_DOMDocument60, nullptr, CLSCTX_INPROC_SERVER,
                                  IID_IXMLDOMDocument2, reinterpret_cast<void**>(&doc))))
        return out;
    doc->put_async(VARIANT_FALSE);
    doc->put_validateOnParse(VARIANT_FALSE);
    doc->put_resolveExternals(VARIANT_FALSE);

    VARIANT_BOOL ok = VARIANT_FALSE;
    if (FAILED(doc->load(_variant_t(path.c_str()), &ok)) || ok != VARIANT_TRUE)
        return out;

    CComPtr<IXMLDOMElement> root;
    if (FAILED(doc->get_documentElement(&root)) || root == nullptr) return out;

    CComPtr<IXMLDOMNodeList> list;
    if (FAILED(root->getElementsByTagName(_bstr_t(L"objectinfo"), &list)) || list == nullptr)
        return out;

    long n = 0;
    list->get_length(&n);
    for (long i = 0; i < n; ++i)
    {
        CComPtr<IXMLDOMNode> node;
        list->get_item(i, &node);
        if (node == nullptr) continue;
        CComPtr<IXMLDOMElement> el;
        node->QueryInterface(IID_IXMLDOMElement, reinterpret_cast<void**>(&el));
        if (el == nullptr) continue;

        std::wstring tag;
        {
            CComVariant v;
            if (SUCCEEDED(el->getAttribute(_bstr_t(L"tag"), &v)) && v.vt == VT_BSTR && v.bstrVal != nullptr)
                tag = static_cast<const wchar_t*>(v.bstrVal);
        }
        if (tag.empty()) continue;

        // 缺省尺寸注入布局引擎：控件尺寸由 catalog 管控，C++ 不再硬编码
        {
            CComVariant wv, hv;
            long dw = 0, dh = 0;
            if (SUCCEEDED(el->getAttribute(_bstr_t(L"w"), &wv)) && wv.vt == VT_BSTR)
                dw = _wtoi(wv.bstrVal);
            if (SUCCEEDED(el->getAttribute(_bstr_t(L"h"), &hv)) && hv.vt == VT_BSTR)
                dh = _wtoi(hv.bstrVal);
            if (dw > 0 && dh > 0)
                xmlSetDefaultSize(tag.c_str(), (int)dw, (int)dh);
        }

        std::vector<PropSpec> props;
        CComPtr<IXMLDOMNodeList> plist;
        if (SUCCEEDED(el->getElementsByTagName(_bstr_t(L"property"), &plist)) && plist != nullptr)
        {
            long pn = 0;
            plist->get_length(&pn);
            for (long p = 0; p < pn; ++p)
            {
                CComPtr<IXMLDOMNode> pnode;
                plist->get_item(p, &pnode);
                if (pnode == nullptr) continue;
                CComPtr<IXMLDOMElement> pel;
                pnode->QueryInterface(IID_IXMLDOMElement, reinterpret_cast<void**>(&pel));
                if (pel == nullptr) continue;

                PropSpec ps;
                CComVariant nv, tv, ov;
                if (SUCCEEDED(pel->getAttribute(_bstr_t(L"name"), &nv)) && nv.vt == VT_BSTR)
                    ps.name = nv.bstrVal;
                if (SUCCEEDED(pel->getAttribute(_bstr_t(L"type"), &tv)) && tv.vt == VT_BSTR)
                    ps.type = tv.bstrVal;
                if (SUCCEEDED(pel->getAttribute(_bstr_t(L"options"), &ov)) && ov.vt == VT_BSTR)
                    ps.options = ov.bstrVal;
                if (ps.type.empty()) ps.type = L"text";
                if (!ps.name.empty()) props.push_back(ps);
            }
        }
        out[std::wstring(tag)] = props;
    }
    return out;
}

// ---------------------------------------------------------------------------
// 校验
// ---------------------------------------------------------------------------
struct Issue
{
    bool isError = true;
    int line = 0;
    std::wstring path;
    std::wstring message;
};

static std::vector<Issue> g_issues;
static std::map<std::wstring, std::vector<PropSpec>> g_catalog;
// 模板键表：由 XmlRuntime 加载一次后填充（已包含 ResourceDictionary Source 合并进来的键）
static std::set<std::wstring> g_templateKeys;
static bool g_templateKeysReady = false;
// 当前文档目录：资源字典 Source 的相对路径基准
static std::wstring g_docDir;

static std::wstring trimWs(const std::wstring& s)
{
    size_t b = s.find_first_not_of(L" \t\r\n");
    if (b == std::wstring::npos)
        return std::wstring();
    size_t e = s.find_last_not_of(L" \t\r\n");
    return s.substr(b, e - b + 1);
}

// 是否资源容器标签（其上的 Source 表示"合并另一个资源字典文件"）
static bool isResContainerTag(const std::wstring& tag)
{
    return tag == L"ResourceDictionary" || tag == L"MergedDictionaries" ||
           tag.find(L"Resources") != std::wstring::npos;
}

static const std::set<std::wstring>& containerTags()
{
    static const std::set<std::wstring> kSet = {
        L"Window", L"Page", L"Grid", L"StackPanel", L"DockPanel", L"WrapPanel",
        L"Canvas", L"Pane", L"Frame", L"GroupBox", L"TabBar", L"TabPage",
        L"SplitView", L"ScrollViewer", L"ScrollView", L"MenuBar", L"MenuItem",
        L"Separator", L"ToolBar", L"StatusBar", L"RowDefinition", L"ColumnDefinition",
        L"Grid.RowDefinitions", L"Grid.ColumnDefinitions", L"Grid.Resources",
        L"TreeView", L"ListView", L"PropertyGrid", L"Border", L"ScrollBar",
        // 声明层资源（样式 / 模板 / 绑定），不参与布局但属于合法 XML
        L"Style", L"Setter", L"ControlTemplate", L"DataTemplate", L"Binding",
        L"Trigger", L"Style.Triggers", L"DataTrigger",
        L"Window.Resources", L"Resources", L"UserControl.Resources", L"Grid.Resources",
        // 跨文件资源合并：<ResourceDictionary Source="x.xml"/> + MergedDictionaries
        L"ResourceDictionary", L"MergedDictionaries", L"ResourceDictionary.MergedDictionaries",
        // 侧栏：根 <PaletteSet>，每页 <page>/<TabItem>（PyMfc 运行时同一套标签）
        L"PaletteSet", L"PaletteSet.Resources", L"PalettePage", L"page", L"TabItem",
        // 设计器外壳构件（PanelHeader / TabStrip / ChipBar / Stage 与它们的子项）
        L"PanelHeader", L"TabStrip", L"Tab", L"ChipBar", L"Chip", L"Stage", L"Page"
    };
    return kSet;
}

static bool isKnownTag(const std::wstring& tag)
{
    if (g_catalog.find(tag) != g_catalog.end()) return true;
    // 容器名/别名与控件别名一律以核心库（PyMfcCore/controls/XmlMfcTags.h）为准，
    // 不再在这里另抄一张表 —— 抄一份就会漂移，出现"Layout/Edit 未知标签"。
    if (xmlmfc::isContainerTag(tag)) return true;
    if (!xmlmfc::leafCanonical(tag).empty()) return true;
    return containerTags().count(xmlmfc::BareTag(tag)) > 0;
}

static const PropSpec* findProp(const std::wstring& tag, const std::wstring& name)
{
    auto it = g_catalog.find(tag);
    if (it == g_catalog.end()) return nullptr;
    for (auto& p : it->second)
        if (_wcsicmp(p.name.c_str(), name.c_str()) == 0) return &p;
    return nullptr;
}

static std::wstring nodeNameOf(IXMLDOMElement* el)
{
    CComVariant v;
    if (SUCCEEDED(el->getAttribute(_bstr_t(L"Name"), &v)) && v.vt == VT_BSTR && v.bstrVal)
        return v.bstrVal;
    if (SUCCEEDED(el->getAttribute(_bstr_t(L"x:Name"), &v)) && v.vt == VT_BSTR && v.bstrVal)
        return v.bstrVal;
    return std::wstring();
}

static void validateElement(IXMLDOMElement* el,
                            const std::wstring& parentPath,
                            const std::vector<TagSite>& sites,
                            size_t& cursor,
                            std::set<std::wstring>& seenNames)
{
    CComBSTR tagB;
    el->get_nodeName(&tagB);
    std::wstring tag = tagB ? std::wstring(tagB) : std::wstring();

    // 行号：按文档顺序消费扫描结果
    int line = 0;
    for (size_t i = cursor; i < sites.size(); ++i)
    {
        if (sites[i].tag == tag)
        {
            line = sites[i].line;
            cursor = i + 1;
            break;
        }
    }

    std::wstring name = nodeNameOf(el);
    std::wstring path = parentPath + L"/" + tag;
    if (!name.empty()) path += L"[" + name + L"]";

    if (!isKnownTag(tag))
    {
        Issue is;
        is.line = line;
        is.path = path;
        is.message = L"未知标签 \"" + tag + L"\"（不在 control_catalog.xml，也不是内置容器标签）";
        g_issues.push_back(is);
    }

    // Ribbon 的子结构允许重名：同一个命令（如"左对齐"）可以同时挂在多个选项卡下，
    // 名字就是命令名，跨选项卡复用是 Office 的常规做法，不是错误。
    const bool ribbonChild = _wcsicmp(tag.c_str(), L"XRibbonButton") == 0 ||
                             _wcsicmp(tag.c_str(), L"XRibbonTab") == 0 ||
                             _wcsicmp(tag.c_str(), L"XRibbonGroup") == 0 ||
                             _wcsicmp(tag.c_str(), L"XRibbonBackstageItem") == 0;
    if (!name.empty() && !ribbonChild && seenNames.count(name) > 0)
    {
        Issue is;
        is.line = line;
        is.path = path;
        is.message = L"Name 重复 \"" + name + L"\"";
        g_issues.push_back(is);
    }
    if (!name.empty()) seenNames.insert(name);

    // 属性校验
    CComPtr<IXMLDOMNamedNodeMap> attrs;
    if (SUCCEEDED(el->get_attributes(&attrs)) && attrs != nullptr)
    {
        long an = 0;
        attrs->get_length(&an);
        for (long a = 0; a < an; ++a)
        {
            CComPtr<IXMLDOMNode> anode;
            attrs->get_item(a, &anode);
            if (anode == nullptr) continue;

            CComBSTR attrB;
            anode->get_nodeName(&attrB);
            CComBSTR valB;
            anode->get_text(&valB);
            std::wstring attr = attrB ? std::wstring(attrB) : std::wstring();
            std::wstring value = valB ? std::wstring(valB) : std::wstring();

            // Template="键"：必须在合并后的模板表里存在，否则运行时会静默画空白
            if (_wcsicmp(attr.c_str(), L"Template") == 0 && g_templateKeysReady)
            {
                std::wstring key;
                if (!xmlIsResourceRef(value, key))
                    key = trimWs(value);
                if (!key.empty() && g_templateKeys.find(key) == g_templateKeys.end())
                {
                    Issue is;
                    is.line = line;
                    is.path = path;
                    is.message = L"Template 引用的模板 \"" + key +
                                 L"\" 未定义（本文件及其合并的资源字典里都没有）";
                    g_issues.push_back(is);
                }
            }

            // 资源容器的 Source="x.xml"：目标文件必须存在（相对当前文档目录）
            if (_wcsicmp(attr.c_str(), L"Source") == 0 && isResContainerTag(tag))
            {
                std::wstring abs = trimWs(value);
                for (wchar_t& c : abs)
                    if (c == L'/')
                        c = L'\\';
                const bool isAbs = (abs.size() >= 2 && abs[1] == L':') ||
                                   (!abs.empty() && abs[0] == L'\\');
                if (!isAbs && !g_docDir.empty())
                    abs = g_docDir + L"\\" + abs;
                if (abs.empty() || ::GetFileAttributesW(abs.c_str()) == INVALID_FILE_ATTRIBUTES)
                {
                    Issue is;
                    is.line = line;
                    is.path = path;
                    is.message = L"资源字典 Source 文件不存在: " + abs;
                    g_issues.push_back(is);
                }
            }

            const PropSpec* spec = findProp(tag, attr);
            if (spec == nullptr) continue;

            if (_wcsicmp(spec->type.c_str(), L"enum") == 0 && !spec->options.empty())
            {
                bool hit = false;
                std::wstring rest = spec->options;
                while (!rest.empty())
                {
                    size_t p = rest.find(L'|');
                    std::wstring opt = (p == std::wstring::npos) ? rest : rest.substr(0, p);
                    if (_wcsicmp(opt.c_str(), value.c_str()) == 0) { hit = true; break; }
                    if (p == std::wstring::npos) break;
                    rest = rest.substr(p + 1);
                }
                if (!hit)
                {
                    Issue is;
                    is.line = line;
                    is.path = path;
                    is.message = L"属性 " + attr + L" 的值 \"" + value + L"\" 不合法，可选：" + spec->options;
                    g_issues.push_back(is);
                }
            }
            else if (_wcsicmp(spec->type.c_str(), L"int") == 0)
            {
                if (!isInt(value))
                {
                    Issue is;
                    is.line = line;
                    is.path = path;
                    is.message = L"属性 " + attr + L" 需要整数，实际 \"" + value + L"\"";
                    g_issues.push_back(is);
                }
            }
            else if (_wcsicmp(spec->type.c_str(), L"bool") == 0)
            {
                if (!isBool(value))
                {
                    Issue is;
                    is.line = line;
                    is.path = path;
                    is.message = L"属性 " + attr + L" 需要 1/0/true/false，实际 \"" + value + L"\"";
                    g_issues.push_back(is);
                }
            }
            else if (_wcsicmp(spec->type.c_str(), L"color") == 0)
            {
                if (!isColor(value))
                {
                    Issue is;
                    is.line = line;
                    is.path = path;
                    is.message = L"属性 " + attr + L" 颜色格式非法 \"" + value + L"\"（用 #RRGGBB 或 white/black/red/green/blue 等）";
                    g_issues.push_back(is);
                }
            }
        }
    }

    // 递归子元素
    CComPtr<IXMLDOMNodeList> children;
    if (SUCCEEDED(el->get_childNodes(&children)) && children != nullptr)
    {
        long cn = 0;
        children->get_length(&cn);
        for (long c = 0; c < cn; ++c)
        {
            CComPtr<IXMLDOMNode> cnode;
            children->get_item(c, &cnode);
            if (cnode == nullptr) continue;
            DOMNodeType type = NODE_INVALID;
            cnode->get_nodeType(&type);
            if (type != NODE_ELEMENT) continue;
            CComPtr<IXMLDOMElement> cel;
            cnode->QueryInterface(IID_IXMLDOMElement, reinterpret_cast<void**>(&cel));
            if (cel == nullptr) continue;
            validateElement(cel, path, sites, cursor, seenNames);
        }
    }
}

// ---------------------------------------------------------------------------
// 渲染（GDI，按 XML 布局结果绘制）
// ---------------------------------------------------------------------------
static XmlElement* g_root = nullptr;
// 布局只在"脏"时重排：外观属性被改（如侧板宽度）就置脏，下一帧 layout 一次，
// 绘制统一读 preview 矩形 —— 这样 XML 属性与画面永远是同一份数据。
static XmlRuntime* g_runtime = nullptr;
static bool g_layoutDirty = true;

static bool isContainerTag(const std::wstring& tag)
{
    static const std::set<std::wstring> kSet = {
        L"Window", L"Page", L"Grid", L"StackPanel", L"DockPanel", L"WrapPanel",
        L"Canvas", L"Layout", L"LayoutPanel", L"AbsolutePanel", L"AbsPanel", L"CanvasPanel",
        L"Pane", L"Border", L"Frame", L"GroupBox", L"TabBar",
        L"TabPage", L"ScrollView", L"ScrollViewer", L"MenuBar", L"ToolBar", L"StatusBar",
        L"PaletteSet", L"PalettePage", L"page", L"TabItem",
        L"PanelHeader", L"TabStrip", L"ChipBar", L"Stage"
    };
    // 统一命名：比较前一律去 X 前缀（<XWindow> / <XStackPanel> 与旧写法同一语义）
    return kSet.count(xmlmfc::BareTag(tag)) > 0;
}

#include <memory>
#include <psapi.h>
#pragma comment(lib, "psapi.lib")
#include "XmlDraw.h"
#include "XmlD2D.h"
#include "XmlControls.h"
#include "../MfcXmlCore/XmlRibbon.h"      // 控件库 Ribbon：预览器与设计器画同一份
#include "../MfcXmlCore/XmlPowerCanvasView.h"  // 控件库配电画布：解析/布局/绘制/命中
#include "../MfcXmlCore/XmlTableDraw.h"        // 控件库表格画法：与设计器同一份

// MFC 控件封装 + 实体化层：控件是真控件（能打字/下拉/勾选），外观仍由 D2D 画。
// 注意：afxwin.h 必须放在 windows.h 之后；本项目已用 (std::min) 形式避开 MFC 的 min/max 宏。
#include <afxwin.h>
#include "XmlTheme.h"
#include "XmlLeafPaint.h"
#include "XmlViewerMfcHost.h"
#include "ViewerControls.h"
#include "../MfcXmlCore/XFrameWindow.h"   // 无框外壳规则（边命中/系统循环/DWM 剥离）

// ---------------------------------------------------------------------------
// 渲染（后端无关）：画什么由这里决定，用什么画交给 XmlCanvas
//   语义与 PyMfc 运行时一致（同一份 XmlAppearance）；GDI 与 Direct2D
//   两个后端产出同一套几何，保证"设计器 / 预览器 / CAD 运行时"观感一致。
// ---------------------------------------------------------------------------
// 配色/圆角统一到 MfcXmlCore/XmlTheme.h（与设计器画布同一份）：
// 这里只留名字，值全来自主题，观感不再两边各画各的。
static const COLORREF kBtnFace  = xmlmfc::theme::button;
static const COLORREF kLine     = xmlmfc::theme::border;
static const COLORREF kEditEdge = xmlmfc::theme::border;
static const COLORREF kInk      = xmlmfc::theme::ink;
static const COLORREF kMuted    = xmlmfc::theme::muted;
static const COLORREF kTrack    = xmlmfc::theme::track;
static const COLORREF kProgress = xmlmfc::theme::accent;
static const COLORREF kPageBack = xmlmfc::theme::surface;
static const COLORREF kFieldCv  = xmlmfc::theme::field;

static const wchar_t* g_backend = L"未初始化";

// ---------------------------------------------------------------------------
// 交互状态（预览器最小可用：配电画布 + Ribbon 按钮）
// 只记录"上一帧画到哪、模型是什么"，命中与业务全部交给控件库：
// 画布 = xmlctl::PowerCanvasView，Ribbon = ribbonHitTest + 按钮 name 命令表。
// ---------------------------------------------------------------------------
struct PowerSlot
{
    xmlctl::PowerCanvasView view;
    RECT rc = { 0, 0, 0, 0 };          // 最后一帧的客户区矩形
    std::wstring name;                  // XML Name
};
static std::map<XmlElement*, PowerSlot> g_powerSlots;   // 一张文档可有多张（主 + 小地图）
static std::map<std::wstring, XmlElement*> g_powerByName;
static XmlElement* g_powerMainEl = nullptr;             // 第一张非 mini 画布 = 主画布
static bool   g_powerShowCable = true;
static bool   g_powerGridFlip = false;    // Ribbon「网格」按过一次就翻转一次
static bool   g_powerRulerFlip = false;
static bool   g_powerMiniHidden = false; // Ribbon「小地图」：整张迷你画布显示/隐藏
static bool   g_panning = false;
static POINT  g_panLast = { 0, 0 };
static XmlElement* g_panEl = nullptr;   // 正在平移的画布（多画布时不能只认全局那一张）
static XmlElement* g_miniDragEl = nullptr;  // 正在小地图上拖动视口的迷你画布

// 节点编辑侧板：XML 里是一块 Width=0 的 XStackPanel，展开时把主画布同步收窄
static bool         g_editOpen = false;
static std::wstring g_editCode;             // 正在编辑的节点编号
static double       g_editLoad = 0.0;       // 待应用的负载（kW）
// 每帧重建的命中表：自绘 Button 的矩形、表格布局与选中行
static std::map<XmlElement*, RECT> g_buttonHits;
static std::map<XmlElement*, xmltable::Table> g_tableHits;
static std::map<XmlElement*, int> g_tableSel;

static xmlctl::PowerCanvasView* mainPowerView()
{
    auto it = g_powerSlots.find(g_powerMainEl);
    return (it == g_powerSlots.end()) ? nullptr : &it->second.view;
}

static bool inRectPt(const RECT& r, POINT p)
{
    return r.right > r.left && r.bottom > r.top &&
           p.x >= r.left && p.x < r.right && p.y >= r.top && p.y < r.bottom;
}

// 哪张画布吃掉了这个点（小地图排在主画布之后，命中时按矩形判定）
static PowerSlot* powerSlotAt(POINT p)
{
    for (auto& kv : g_powerSlots)
        if (inRectPt(kv.second.rc, p)) return &kv.second;
    return nullptr;
}

// 反查槽表里的元素指针（拖动时要用元素做键）
static XmlElement* powerElOf(PowerSlot* slot)
{
    for (auto& kv : g_powerSlots)
        if (&kv.second == slot) return kv.first;
    return nullptr;
}

// XmlElement 没有 parent 指针，只能从根按 Name 递归找（Name 解析进的是 name 成员）
static XmlElement* findByName(XmlElement* root, const std::wstring& name)
{
    if (root == nullptr) return nullptr;
    if (root->name == name) return root;
    for (auto& c : root->children)
        if (XmlElement* r = findByName(&c, name)) return r;
    return nullptr;
}

// 下面两个助手定义在后面（选中回写 / 表单回写），这里先声明
static bool setLabelByName(XmlElement& el, const std::wstring& name, const std::wstring& text);
static void viewerSyncSelection();

// 把编辑态写到侧板那几个 XLabel 上（选中节点 → 表单外观）
static void editorSyncLabels()
{
    const xmlplan::PowerNode* node = nullptr;
    if (xmlctl::PowerCanvasView* pv = mainPowerView())
        for (auto& n : pv->plan.nodes)
            if (n.code == g_editCode) { node = &n; break; }

    auto put = [](const wchar_t* name, const std::wstring& value) {
        if (g_root != nullptr) setLabelByName(*g_root, name, value);
    };
    if (node == nullptr)
    {
        put(L"lblEdCode", L"编号　—");
        put(L"lblEdName", L"名称　—");
        put(L"lblEdUp", L"上游　—");
        put(L"lblEdCable", L"电缆　—");
        put(L"lblEdVolt", L"电压　—");
        put(L"lblEdLoad", L"负载　— kW");
        put(L"lblEdLen", L"长度　— m");
        return;
    }
    put(L"lblEdCode", L"编号　" + node->code);
    put(L"lblEdName", L"名称　" + node->name);
    put(L"lblEdUp", L"上游　" + (node->upstream.empty() ? std::wstring(L"—") : node->upstream));
    put(L"lblEdCable", L"电缆　" + (node->cable.empty() ? std::wstring(L"—") : node->cable));
    put(L"lblEdVolt", L"电压　" + (node->voltage.empty() ? std::wstring(L"—") : node->voltage));
    wchar_t buf[64] = {};
    swprintf_s(buf, L"%.2f", g_editLoad);
    std::wstring loadText(buf);
    if (loadText.find(L'.') != std::wstring::npos)
    {
        while (!loadText.empty() && loadText.back() == L'0') loadText.pop_back();
        if (!loadText.empty() && loadText.back() == L'.') loadText.pop_back();
    }
    put(L"lblEdLoad", L"负载　" + loadText + L" kW");
    put(L"lblEdLen", L"长度　" + (node->length > 0 ? std::to_wstring((int)node->length) + L" m" : std::wstring(L"—")));
}

// 选中一个节点：表格行、画布、表单三处同步
static void editorSelectNode(const std::wstring& code)
{
    g_editCode = code;
    if (xmlctl::PowerCanvasView* pv = mainPowerView())
    {
        pv->sel = code;
        for (auto& n : pv->plan.nodes)
            if (n.code == code) { g_editLoad = n.loadKw; break; }
    }
    editorSyncLabels();
    viewerSyncSelection();
}

static void editorShowButtons(bool show);   // 定义在控件建好之后（要用 g_host / g_bindings）

// 侧板展开/收起：改三处 Width（解析期已把 Width 收进 w 成员，两处都要写）
// 收起时 Width=0 会被"内容自适应"重新撑开，所以同时把 Left 移出可视区 ——
// 面板是绝对锚定的，退到窗口外就不参与任何排版，也不必靠额外的显隐开关。
static void editorToggle(HWND hwnd, bool open)
{
    g_editOpen = open;
    const int planW    = open ? 560 : 978;
    const int rightW   = open ? 0   : 336;
    const int editorW  = open ? 632 : 0;
    const int editorL  = open ? 668 : 2400;
    auto apply = [&](const wchar_t* name, int w) {
        XmlElement* el = findByName(g_root, name);
        if (el == nullptr) return;
        el->w = w;
        el->set(L"Width", std::to_wstring(w));
    };
    auto moveTo = [&](const wchar_t* name, int left) {
        if (XmlElement* el = findByName(g_root, name))
        {
            // Left 与 Width 一样在解析期收进成员（x / hasX），只 set 属性不会重排
            el->x = left;
            el->hasX = true;
            el->set(L"Left", std::to_wstring(left));
        }
    };
    apply(L"Plan", planW);
    apply(L"RightPanel", rightW);
    apply(L"NodeEditor", editorW);
    moveTo(L"NodeEditor", editorL);
    // 侧板里的按钮是 MFC 真控件（子 HWND），收起来必须一起藏掉，
    // 否则它们不会被父面板的裁剪管住，留在窗口底部当幽灵按钮。
    editorShowButtons(open);
    g_layoutDirty = true;
    editorSyncLabels();
    ::InvalidateRect(hwnd, nullptr, FALSE);
}

// 侧板表格从当前 Data 重建一次 Rows：应用回写后表格里的数是新的，
// 不让"表单说 47、表格还写 45"。列与外壳 XML 的 Columns 一一对应。
static void editorRefreshTable()
{
    XmlElement* tbl = findByName(g_root, L"tblNodes");
    XmlElement* pe = g_powerMainEl;
    if (tbl == nullptr || pe == nullptr) return;

    std::vector<xmlplan::PowerNode> nodes;
    std::wstring title, err;
    bool horizontal = false;
    if (!xmlplan::parseJson(pe->get(L"Data"), nodes, title, err, &horizontal)) return;

    auto num = [](double v) {
        std::wstring s = std::to_wstring(v);
        const size_t dot = s.find(L'.');
        if (dot != std::wstring::npos)
        {
            while (!s.empty() && s.back() == L'0') s.pop_back();
            if (!s.empty() && s.back() == L'.') s.pop_back();
        }
        return s;
    };
    std::wstring rows;
    for (size_t i = 0; i < nodes.size(); ++i)
    {
        const auto& n = nodes[i];
        if (i) rows += L"|";
        rows += n.code + L"\t" + n.name + L"\t"
             + (n.upstream.empty() ? std::wstring(L"—") : n.upstream) + L"\t"
             + (n.cable.empty() ? std::wstring(L"—") : n.cable) + L"\t"
             + (n.voltage.empty() ? std::wstring(L"—") : n.voltage) + L"\t"
             + num(n.loadKw);
    }
    tbl->set(L"Rows", rows);
}

// 应用：改负载 → 重排版 → 写回 XML 的 Data（外壳里的数据就是这一份 JSON）
static void editorApply()
{
    XmlElement* pe = g_powerMainEl;
    if (pe == nullptr || g_editCode.empty()) return;

    std::vector<xmlplan::PowerNode> nodes;
    std::wstring title, err;
    bool horizontal = false;
    if (!xmlplan::parseJson(pe->get(L"Data"), nodes, title, err, &horizontal)) return;

    bool touched = false;
    for (auto& n : nodes)
        if (n.code == g_editCode) { n.loadKw = g_editLoad; touched = true; }
    if (!touched) return;

    xmlplan::PowerPlan plan;
    xmlplan::PowerPlanStyle style;
    if (!xmlplan::buildPlan(nodes, plan, style, err, horizontal)) return;

    pe->set(L"Data", xmlplan::toJson(plan));
    if (xmlctl::PowerCanvasView* pv = mainPowerView()) pv->sel = g_editCode;
    editorRefreshTable();
    editorSyncLabels();
}

static xmlctl::XRibbonModel  g_ribbonModel;
static xmlctl::XRibbonLayout g_ribbonLayout;
static bool   g_ribbonValid = false;
static int    g_ribbonSel = -1;     // 用户在预览器里切换的选项卡（-1 = 用 XML 的 SelectedTab）



// 名字比对要同时认 "GroupBox" 与 "XGroupBox"：XML 里统一是 X 前缀，
// 绘制分支按本意名分派，两边都要命中（否则预览器会静默漏画一类控件）。
static bool isAnyTag(const std::wstring& tag, std::initializer_list<const wchar_t*> names)
{
    for (const wchar_t* n : names)
    if (!tag.empty() && (tag[0] == L'X' || tag[0] == L'x'))
    {
        const std::wstring bare = tag.substr(1);
        for (const wchar_t* n : names)
            if (_wcsicmp(bare.c_str(), n) == 0) return true;
    }
    return false;
}

static bool truthy(const std::wstring& v)
{
    return _wcsicmp(v.c_str(), L"true") == 0 || _wcsicmp(v.c_str(), L"yes") == 0 || v == L"1";
}

// 叶子控件：按类型复刻运行时外观（有 XML 外观声明时优先用 XmlAppearance 背板）
// 叶子控件统一由 MfcXmlCore/XmlLeafPaint.h 绘制：
// 设计器画布、预览器调的是同一份实现，圆角/边框/配色不可能再分叉。
static void drawLeaf(XmlCanvas& cv, XmlElement* el, const RECT& rc,
                     const xmlapp::XmlAppearance& ap, bool styled, COLORREF parentBack)
{
    xmlleaf::drawLeaf(cv, el, rc, ap, styled, parentBack, exeDir());
}

static COLORREF drawContainer(XmlCanvas& cv, XmlElement* el, const RECT& rc,
                              const xmlapp::XmlAppearance& ap, bool styled, COLORREF parentBack)
{
    COLORREF back = parentBack;
    COLORREF bg = 0;
    if (xmlParseColor(el->get(L"Background"), bg)) back = bg;

    // 设计器外壳构件：标题条 / 文档标签条 / 画布舞台（与设计器共用同一批绘制函数）
    if (_wcsicmp(el->tag.c_str(), L"PanelHeader") == 0)
    {
        xmlctl::xmlDrawPanelHeader(cv, rc, el->text, el->get(L"Hint"), 10);
        return xmlctl::shell().header;
    }
    if (isAnyTag(el->tag, {L"Stage"}))
    {
        xmlctl::xmlDrawStage(cv, rc);
        return xmlctl::shell().stage;
    }
    if (isAnyTag(el->tag, {L"TabStrip"}))
    {
        xmlctl::xmlDrawTabBar(cv, rc);
        return xmlctl::shell().header;
    }

    const bool group = isAnyTag(el->tag, {L"GroupBox"});
    const bool bar = isAnyTag(el->tag, {L"MenuBar", L"ToolBar"});
    const bool root = (el == g_root);

    if (styled)
        xmlDrawBackplate(cv, rc, ap, parentBack, kLine);
    else if (group || bar || root || el->has(L"Background"))
        cv.fillRect(rc, back);

    if (bar && !styled)
        cv.strokeLine({rc.left, rc.bottom - 1}, {rc.right - 1, rc.bottom - 1}, kLine, 1);

    if (group)
    {
        // 与 MFC GroupBox 一致：细边框 + 左上标题压在边线上
        const std::wstring& header = el->text;
        const int headerH = 16;
        const int indent = 8;
        RECT fr = rc;
        fr.top += headerH / 2;
        cv.strokeRect(fr, kLine, 1);

        if (!header.empty())
        {
            // 标题区先用底色挖空，再写字
            const int sz = ::MulDiv(headerH, 3, 4) * static_cast<int>(header.length()) / 2;
            RECT hole = {rc.left + indent - 2, rc.top, rc.left + indent + sz + 2, rc.top + headerH};
            cv.fillRect(hole, back);
            RECT tr = {rc.left + indent, rc.top, rc.right - 2, rc.top + headerH};
            xmlDrawText(cv, tr, header, ap, styled, kInk, DT_LEFT, 0);
        }
    }

    return back;
}

static void drawElement(XmlCanvas& cv, XmlElement* el, int ox, int oy, COLORREF parentBack)
{
    // 编辑侧板收起时（Width=0）整棵子树不画：宽 0 的容器会被内容"撑开"，
    // 否则它的按钮/表格会留在窗口里。判据只看宽度，与设计器画布一致。
    if (el != nullptr && el->name == L"NodeEditor" && el->w <= 0) return;
    RECT rc;
    rc.left = ox + el->previewX;
    rc.top = oy + el->previewY;
    rc.right = rc.left + el->previewW;
    rc.bottom = rc.top + el->previewH;
    if (rc.right <= rc.left || rc.bottom <= rc.top) return;

    // 统一外观契约（与设计器画布、PyMfc 运行时同一份语义）
    xmlapp::XmlAppearance ap;
    for (const auto& kv : el->attrs)
        xmlapp::SetProperty(ap, kv.first, kv.second);
    // 样式/绑定注入的有效属性：本地已声明的优先
    for (const auto& kv : el->styledAttrs)
    {
        bool local = false;
        for (const auto& lv : el->attrs)
            if (_wcsicmp(lv.first.c_str(), kv.first.c_str()) == 0)
            {
                local = true;
                break;
            }
        if (!local)
            xmlapp::SetProperty(ap, kv.first, kv.second);
    }
    const bool styled = !ap.empty();

    // 外壳构件：菜单行 / 状态栏 / Draw 条 / 胶囊条 / 文档标签条 —— 与设计器同一批绘制函数，
    // 子节点（标题、菜单项、窗口按钮、状态格、形状按钮）已由这些函数整体画出，不再单独绘制
    {
        const std::wstring shellTag = xmlmfc::BareTag(el->tag);
        if (_wcsicmp(shellTag.c_str(), L"MenuBar") == 0)
        {
            xmlctl::xmlDrawShellMenuBar(cv, rc, *el);
            return;
        }
        if (_wcsicmp(shellTag.c_str(), L"StatusBar") == 0)
        {
            xmlctl::xmlDrawShellStatusBar(cv, rc, *el);
            return;
        }
        if (_wcsicmp(shellTag.c_str(), L"DrawBar") == 0)
        {
            xmlctl::xmlDrawShellDrawBar(cv, rc, *el);
            return;
        }
        if (_wcsicmp(shellTag.c_str(), L"ChipBar") == 0)
        {
            xmlctl::xmlDrawShellChipBar(cv, rc, *el);
            return;
        }
        if (_wcsicmp(shellTag.c_str(), L"PanelHeader") == 0)
        {
            xmlctl::xmlDrawShellPanelHeader(cv, rc, *el);
            return;
        }
        if (_wcsicmp(shellTag.c_str(), L"Stage") == 0)
        {
            xmlctl::xmlDrawShellStage(cv, rc, *el);
            // 舞台自身按 XML 底色铺满，子节点（画布纸）继续绘制
            COLORREF back = parentBack;
            { COLORREF c; if (xmlapp::ParseColor(el->get(L"Background"), c)) back = c; }
            for (auto& c : el->children) drawElement(cv, &c, ox, oy, back);
            return;
        }
        if (_wcsicmp(shellTag.c_str(), L"TabStrip") == 0)
        {
            xmlctl::xmlDrawShellTabStrip(cv, rc, *el);
            return;
        }
        if (_wcsicmp(shellTag.c_str(), L"Ribbon") == 0)
        {
            // 与设计器同一份绘制：控件库按 XML 解析、布局、画（含宽度不足时的分组折叠）
            if (xmlctl::ribbonIconBase().empty())
                xmlctl::ribbonIconBase() = exeDir();
            xmlctl::XRibbonModel model;
            xmlctl::ribbonParse(*el, model);
            // Collapsed="true"：只显示选项卡条（与设计器点右端三角收起后的观感一致）
            model.bodyCollapsed = _wcsicmp(el->get(L"Collapsed").c_str(), L"true") == 0;
            // 预览器里点过的选项卡优先于 XML 的 SelectedTab（否则一重绘就被打回原页）
            if (g_ribbonSel >= 0 && g_ribbonSel < (int)model.tabs.size())
                model.selected = g_ribbonSel;
            const int tabH = 34;
            RECT tabRc  = { rc.left, rc.top, rc.right, rc.top + tabH };
            RECT bodyRc = { rc.left, rc.top + tabH, rc.right,
                            model.bodyCollapsed ? rc.top + tabH : rc.bottom };
            xmlctl::XRibbonMetrics mx;
            mx.bodyH = bodyRc.bottom - bodyRc.top;
            mx.titleTop = (std::max)(36, mx.bodyH - 24);
            xmlctl::XRibbonLayout lay;
            xmlctl::ribbonLayout(model, mx, tabRc, bodyRc, lay);
            xmlctl::ribbonDraw(cv, tabRc, bodyRc, model, mx, lay, -1, std::wstring(), std::wstring(), 10, 9);
            // 命中表留在全局：点击选项卡 / 按钮时由窗口过程按同一份 layout 反查
            g_ribbonModel = model;
            g_ribbonLayout = lay;
            g_ribbonValid = true;
            return;
        }
        if (_wcsicmp(shellTag.c_str(), L"TableView") == 0 ||
            _wcsicmp(shellTag.c_str(), L"DataGrid") == 0 ||
            _wcsicmp(shellTag.c_str(), L"ListCtrl") == 0)
        {
            // 表格：与设计器画布同一份实现（MfcXmlCore/XmlTableDraw.h）
            xmltable::Style st;
            st.ink = kInk;
            st.headBg = RGB(0xF3, 0xF5, 0xF8);
            st.grid = RGB(0xDC, 0xE2, 0xEA);
            st.border = kLine;
            cv.fillRect(rc, RGB(0xFF, 0xFF, 0xFF));
            xmltable::Table layout;
            if (xmltable::buildTable(rc, el->get(L"Columns"), el->get(L"Rows"),
                                     el->get(L"Sort"), st.pt, layout))
            {
                // 布局留一份：点击时按同一份几何反查行号
                g_tableHits[el] = layout;
                int selRow = -1;
                auto it = g_tableSel.find(el);
                if (it != g_tableSel.end()) selRow = it->second;
                xmltable::drawTable(cv, rc, layout, st, selRow);
                cv.strokeRect(rc, kLine, 1);
                return;
            }
        }
        if (_wcsicmp(shellTag.c_str(), L"PowerCanvas") == 0)
        {
            // 配电规划画布：解析/布局/绘制全在控件库（xmlctl::PowerCanvasView），
            // 这里只按 XML 属性喂数据。一个文档可以有多张（主画布 + 小地图），
            // 视图状态按元素指针各存一份，互不影响。
            PowerSlot& slot = g_powerSlots[el];
            slot.name = el->get(L"Name");
            xmlctl::PowerCanvasView& v = slot.view;
            const bool isMini = el->get(L"Mini") == L"1";
            v.mini = isMini;
            if (isMini && g_powerMiniHidden)
            {
                slot.rc = { 0, 0, 0, 0 };   // 隐藏时清命中矩形，否则看不见的地方还能点
                return;
            }
            if (isMini)
            {
                // 小地图不写 Data：跟随 MiniOf 指定的主画布（缺省 = 文档里第一张）
                std::wstring target = el->get(L"MiniOf");
                XmlElement* hostEl = nullptr;
                if (!target.empty())
                {
                    auto it = g_powerByName.find(target);
                    if (it != g_powerByName.end()) hostEl = it->second;
                }
                else if (g_powerMainEl != nullptr)
                {
                    hostEl = g_powerMainEl;
                }
                if (hostEl != nullptr)
                {
                    v.viewRef = &g_powerSlots[hostEl].view;
                    v.loaded = true;
                    slot.rc = rc;
                    v.paint(cv, rc, xmlctl::miniBack());
                }
                return;
            }

            if (!slot.name.empty()) g_powerByName[slot.name] = el;
            if (g_powerMainEl == nullptr) g_powerMainEl = el;

            const std::wstring data = el->get(L"Data");
            const bool showCable = _wcsicmp(el->get(L"ShowCable").c_str(), L"0") != 0;
            // 只在 Data 换版本时重建视图状态（重绘不清用户的选择）
            if (data != v.data)
            {
                v.load(data);
                // 数据变版本（应用编辑）后仍保留正在编辑的选中，否则一应用选择就丢
                v.sel = g_editCode;
            }
            else if (!v.loaded)
            {
                v.load(data);
            }
            v.viewRef = nullptr;
            v.showCable = showCable && g_powerShowCable;
            v.showGrid = (el->get(L"Grid") == L"1") != g_powerGridFlip;
            v.showRuler = (el->get(L"Ruler") == L"1") != g_powerRulerFlip;
            slot.rc = rc;
            v.paint(cv, rc, xmlmfc::theme::track);
            return;
        }
    }

    if (isContainerTag(el->tag))
    {
        const COLORREF back = drawContainer(cv, el, rc, ap, styled, parentBack);
        // 子节点裁剪到容器内：运行时 MFC 子窗口同样被父窗口裁剪
        cv.clipPush(rc);
        for (auto& child : el->children)
            drawElement(cv, &child, ox, oy, back);
        cv.clipPop();
    }
    else
    {
        // 普通按钮也要能点：几何就在这里，顺手记一份命中矩形
        if (_wcsicmp(el->tag.c_str(), L"Button") == 0) g_buttonHits[el] = rc;
        drawLeaf(cv, el, rc, ap, styled, parentBack);
    }
}

static LRESULT CALLBACK viewerProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);

// 预览器把选中结果写回外壳 XML 的对应标签：右侧信息板是普通 XLabel，
// 名称由 XML 决定，内容由控件层汇报 —— 不在预览器里画一块"专用的"信息板。
static bool setLabelByName(XmlElement& el, const std::wstring& name, const std::wstring& text)
{
    if (el.name == name)
    {
        el.set(L"Content", text);
        el.text = text;          // 解析期已把 Content 收进 text，绘制读的就是它
        return true;
    }
    for (auto& child : el.children)
        if (setLabelByName(child, name, text))
            return true;
    return false;
}

static void viewerSyncSelection()
{
    if (g_root == nullptr) return;
    static const xmlplan::PowerPlan kEmptyPlan;
    xmlctl::PowerCanvasView* pv = mainPowerView();
    const xmlplan::PowerPlan& plan = pv ? pv->plan : kEmptyPlan;
    const xmlplan::PowerNode* node = nullptr;
    const std::wstring sel = pv ? pv->sel : std::wstring();
    for (const xmlplan::PowerNode& n : plan.nodes)
    {
        if (n.code == sel)
        {
            node = &n;
            break;
        }
    }
    const int total = (int)plan.boxes.size();
    const int links = (int)plan.links.size();
    wchar_t buf[256];
    if (node == nullptr)
    {
        setLabelByName(*g_root, L"lblSelCode", L"（未选中）");
        setLabelByName(*g_root, L"lblSelName", L"名称");
        setLabelByName(*g_root, L"lblSelVolt", L"电压等级");
        setLabelByName(*g_root, L"lblSelLoad", L"负荷");
        setLabelByName(*g_root, L"lblSelCable", L"电缆");
        setLabelByName(*g_root, L"lblSelLen", L"长度");
        swprintf_s(buf, L"就绪 · %d 台设备 · %d 条回路", total, links);
        setLabelByName(*g_root, L"lblState", buf);
        return;
    }
    setLabelByName(*g_root, L"lblSelCode", node->code);
    setLabelByName(*g_root, L"lblSelName",
                   node->name.empty() ? std::wstring(L"名称  —")
                                      : (std::wstring(L"名称  ") + node->name));
    setLabelByName(*g_root, L"lblSelVolt", std::wstring(L"电压等级  ") + node->voltage);
    if (node->loadKw > 0.0)
    {
        swprintf_s(buf, L"负荷  %.1f kW", node->loadKw);
        setLabelByName(*g_root, L"lblSelLoad", buf);
    }
    else setLabelByName(*g_root, L"lblSelLoad", L"负荷  —");
    setLabelByName(*g_root, L"lblSelCable",
                   node->cable.empty() ? std::wstring(L"电缆  —")
                                       : (std::wstring(L"电缆  ") + node->cable));
    if (node->length > 0.0)
    {
        swprintf_s(buf, L"长度  %.0f m", node->length);
        setLabelByName(*g_root, L"lblSelLen", buf);
    }
    else setLabelByName(*g_root, L"lblSelLen", L"长度  —");
    swprintf_s(buf, L"就绪 · %d 台设备 · %d 条回路 · 选中 %s", total, links,
               node->name.empty() ? node->code.c_str() : node->name.c_str());
    setLabelByName(*g_root, L"lblState", buf);
}

// 按钮命令：外壳 XML 里 <XRibbonButton Name="btnZoomFit"/> 的 Name 就是命令名。
// 全部转成 PowerCanvasView 的一个动作，预览器不自己算任何几何。
static void viewerCommand(HWND hwnd, const std::wstring& name)
{
    if (name.empty()) return;

    // 侧板命令不依赖画布，先处理（画布不存在时也要能开关）
    if (name == L"btnNodeEdit")
    {
        editorToggle(hwnd, !g_editOpen);
        return;
    }
    if (name == L"btnClose")
    {
        editorToggle(hwnd, false);
        return;
    }
    if (name == L"btnLoadUp" || name == L"btnLoadDown")
    {
        g_editLoad += (name == L"btnLoadUp") ? 1.0 : -1.0;
        if (g_editLoad < 0.0) g_editLoad = 0.0;
        editorSyncLabels();
        ::InvalidateRect(hwnd, nullptr, FALSE);
        return;
    }
    if (name == L"btnApply")
    {
        editorApply();
        ::InvalidateRect(hwnd, nullptr, FALSE);
        return;
    }

    xmlctl::PowerCanvasView* pv = mainPowerView();
    if (pv == nullptr) return;
    RECT prc = { 0, 0, 0, 0 };
    if (auto it = g_powerSlots.find(g_powerMainEl); it != g_powerSlots.end()) prc = it->second.rc;
    const int cx = prc.left + (prc.right - prc.left) / 2;
    const int cy = prc.top + (prc.bottom - prc.top) / 2;
    bool changed = true;
    if (name == L"btnZoomFit")
    {
        pv->fitPending = true;
    }
    else if (name == L"btnZoomIn")
    {
        pv->wheelAt(cx, cy, 120);
    }
    else if (name == L"btnZoomOut")
    {
        pv->wheelAt(cx, cy, -120);
    }
    else if (name == L"btnZoomReset")
    {
        pv->setZoomAbout(cx, cy, 1.0f);
    }
    else if (name == L"btnCable")
    {
        g_powerShowCable = !g_powerShowCable;
    }
    else if (name == L"btnGrid")
    {
        g_powerGridFlip = !g_powerGridFlip;
    }
    else if (name == L"btnRuler")
    {
        g_powerRulerFlip = !g_powerRulerFlip;
    }
    else if (name == L"btnMini")
    {
        g_powerMiniHidden = !g_powerMiniHidden;   // 显示/隐藏小地图整块
    }
    else if (name == L"btnDevice")
    {
        editorToggle(hwnd, true);                 // 数据页「设备表」= 打开节点编辑侧板
        return;
    }
    else if (name == L"btnRelayout")
    {
        pv->fitPending = true;                    // 重新排版 = 按当前 XML 重算并适应窗口
    }
    else
    {
        changed = false;   // 设备表 / 导出 / 打印等留给运行时宿主，预览器不假装实现
    }
    if (changed) ::InvalidateRect(hwnd, nullptr, FALSE);
}
static bool g_inSizeMove = false;   // 系统拖动/缩放循环中：只擦底色，松手后一次重画

// 实体化状态：控件 id -> 元素/矩形/外观；宿主负责建窗与 owner-draw 绘制
static ViewerMfcHost* g_host = nullptr;
static std::map<int, ViewerLeafBinding> g_bindings;
static bool g_controlsBuilt = false;

// owner-draw 回调：控件外观仍交给 drawLeaf（与设计器画布同一份绘制代码）
static void drawBuiltControl(XmlCanvas& cv, int id, const RECT& rc)
{
    auto it = g_bindings.find(id);
    if (it == g_bindings.end() || it->second.element == nullptr)
        return;
    const ViewerLeafBinding& b = it->second;
    XmlElement* el = b.element;

    // 复选/单选/开关的外观由 drawLeaf 按元素的 Checked 属性画，
    // 因此绘制前把运行期勾选状态临时写进元素，画完恢复，不污染文档
    const std::wstring& tag = el->tag;
    const bool isToggle = _wcsicmp(tag.c_str(), L"CheckBox") == 0 ||
                          _wcsicmp(tag.c_str(), L"RadioButton") == 0 ||
                          _wcsicmp(tag.c_str(), L"ToggleButton") == 0;
    std::wstring savedChecked;
    bool hadChecked = false;
    if (isToggle && g_host != nullptr)
    {
        const std::wstring now = g_host->getCheckValue(id) ? L"true" : L"false";
        for (auto& kv : el->attrs)
            if (_wcsicmp(kv.first.c_str(), L"Checked") == 0)
            {
                savedChecked = kv.second;
                kv.second = now;
                hadChecked = true;
                break;
            }
        if (!hadChecked)
            el->attrs.emplace_back(L"Checked", now);
    }

    drawLeaf(cv, el, rc, b.ap, b.styled, b.parentBack);

    if (isToggle)
    {
        if (hadChecked)
        {
            for (auto& kv : el->attrs)
                if (_wcsicmp(kv.first.c_str(), L"Checked") == 0)
                {
                    kv.second = savedChecked;
                    break;
                }
        }
        else
        {
            el->attrs.pop_back();
        }
    }
}

static void ensureControlsBuilt(HWND hwnd)
{
    if (g_controlsBuilt || g_root == nullptr)
        return;
    g_controlsBuilt = true;
    if (g_host == nullptr)
        g_host = new ViewerMfcHost();
    g_host->init(hwnd, &drawBuiltControl);
    for (const ViewerLeafBinding& b : viewerControlsBuild(*g_host, g_root, 20, 20))
        g_bindings[b.id] = b;
    // 控件全部建好后再排一次叠放：边建边排会漏掉后建的兄弟
    g_host->applyOrder();
    // 建好这一刻侧板可能还收着：先把它的按钮按当前状态藏起来
    editorShowButtons(g_editOpen);
}

// 侧板上的四个按钮是实体控件：跟着侧板的开合显隐（绘制层管不到子 HWND）
static void editorShowButtons(bool show)
{
    if (g_host == nullptr || g_root == nullptr) return;
    for (const wchar_t* name : { L"btnLoadDown", L"btnLoadUp", L"btnApply", L"btnClose" })
    {
        XmlElement* el = findByName(g_root, name);
        if (el == nullptr) continue;
        for (auto& kv : g_bindings)
        {
            if (kv.second.element != el) continue;
            const HWND h = g_host->ctrlHwnd(kv.second.id);
            if (h != nullptr) ::ShowWindow(h, show ? SW_SHOW : SW_HIDE);
            break;
        }
    }
}

static LRESULT CALLBACK viewerProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    // 外壳选项：PYARX_FRAME=native 走系统标题栏，默认无框（外观由 XML 决定，
    // 与设计器 / 运行时同一套边命中与系统缩放循环 —— 能力在控件库，这里只是调用者）。
    static const bool frameless = []() {
        wchar_t v[16] = {};
        if (::GetEnvironmentVariableW(L"PYARX_FRAME", v, 16) > 0 &&
            _wcsicmp(v, L"native") == 0)
            return false;
        return true;
    }();
    // XML 根节点的标题条高度（preview 里没有该属性时退回 32px 拖动区）
    static const int captionHeight = 32;

    if (frameless)
    {
        bool handled = false;
        LRESULT r = xmlframe::framelessNcProc(hwnd, msg, wp, lp, handled,
                                              captionHeight, 0, 320, 200);
        if (handled)
            return r;
    }
    switch (msg)
    {
    case WM_CREATE:
        if (frameless)
            xmlframe::stripDwmFrame(hwnd);
        return 0;

    case WM_SIZE:
        // 尺寸变化只冻结绘制；松手后由 EXITSIZEMOVE / 静止 60ms 收尾重画一次
        if (frameless && wp != SIZE_MINIMIZED)
        {
            g_inSizeMove = true;
            ::SetTimer(hwnd, 2, 60, nullptr);
        }
        break;

    case WM_TIMER:
        if (wp == 2)
        {
            ::KillTimer(hwnd, 2);
            if (frameless)
                xmlframe::stripDwmFrame(hwnd);   // 稳定态再压描边（每帧压会让 DWM 重算整窗）
            if (g_inSizeMove)
            {
                g_inSizeMove = false;
                ::InvalidateRect(hwnd, nullptr, TRUE);
            }
            return 0;
        }
        break;

    case WM_ENTERSIZEMOVE:
        g_inSizeMove = true;
        return 0;

    case WM_EXITSIZEMOVE:
        g_inSizeMove = false;
        ::InvalidateRect(hwnd, nullptr, TRUE);
        return 0;

    case WM_ACTIVATE:
        // 激活切换后 DWM 会按新状态重画描边，再压一次
        if (frameless && LOWORD(wp) != WA_INACTIVE)
            xmlframe::stripDwmFrame(hwnd);
        break;

    case WM_ERASEBKGND:
        // 每一帧都由渲染后端整幅覆盖，不需要系统先擦一遍（避免"擦白再画"的一帧闪烁）
        return 1;


    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC dc = ::BeginPaint(hwnd, &ps);
        RECT client;
        ::GetClientRect(hwnd, &client);
        const int cw = client.right, ch = client.bottom;

        // 拖动/缩放期间不重画：只擦应用底色，交给 DWM 合成上一帧（Qt 的节奏）
        if (g_inSizeMove)
        {
            ::FillRect(dc, &client, (HBRUSH)::GetStockObject(BLACK_BRUSH));
            ::EndPaint(hwnd, &ps);
            return 0;
        }

        // 先建真控件，再画父窗口表面：DCRenderTarget 画到 BeginPaint 的 DC，
        // 受 WS_CLIPCHILDREN 约束，不会盖住子控件（HwndRenderTarget 直呈会被 DWM
        // 直接合成，反而不受裁剪，最终所有控件都被"糊掉"）。
        ensureControlsBuilt(hwnd);

        // 布局：属性（Width/Height/Left/Top）是唯一真源，改了名字下的属性就重排一次
        if (g_layoutDirty && g_runtime != nullptr && g_root != nullptr)
        {
            g_layoutDirty = false;
            g_runtime->layout(0, 0, g_root->w > 0 ? g_root->w : 640,
                                    g_root->h > 0 ? g_root->h : 480);
        }

        // 每帧重建"名字 → 元素"索引：文档重开时元素指针会换，旧指针不能留
        g_powerByName.clear();
        g_buttonHits.clear();
        g_tableHits.clear();

        static D2DDcCanvas d2d;
        if (d2d.bind(dc, client) && d2d.begin(cw, ch))
        {
            d2d.fillRect(client, RGB(238, 240, 243));
            if (g_root != nullptr)
                drawElement(d2d, g_root, 20, 20, kPageBack);
            d2d.end();
            g_backend = g_bindings.empty() ? L"Direct2D 表面" : L"Direct2D 外观 + MFC 真控件";
        }
        else
        {
            ::FillRect(dc, &client, (HBRUSH)::GetStockObject(WHITE_BRUSH));
            g_backend = L"Direct2D 不可用";
        }

        // 标题上标注实际后端，截图即可判断是否真跑了硬件
        static bool tagged = false;
        if (!tagged)
        {
            tagged = true;
            wchar_t cur[512] = {};
            ::GetWindowTextW(hwnd, cur, 512);
            std::wstring t(cur);
            t += L"  ·  ";
            t += g_backend;
            ::SetWindowTextW(hwnd, t.c_str());
        }
        ::EndPaint(hwnd, &ps);
        return 0;
    }
    case WM_COMMAND:
        // 控件通知（BN_CLICKED 等）发到父窗口：owner-draw 的复选/单选靠这里翻转状态
        if (g_host != nullptr)
        {
            g_host->onCommand(LOWORD(wp));
            return 0;
        }
        break;

    // ---- 配电画布 + Ribbon 交互：几何、命中和视图状态都在控件库 ----
    case WM_LBUTTONDOWN:
    {
        POINT p{ GET_X_LPARAM(lp), GET_Y_LPARAM(lp) };
        // 表格行（节点编辑侧板）：点一行 = 选中该设备，表单与画布同步
        for (auto& kv : g_tableHits)
        {
            // 侧板收着只认右板的表；展开后只认侧板的表（另一块宽度为 0，看不见但矩形还在）
            if (kv.first->name == L"tblNodes" && !g_editOpen) continue;
            if (kv.first->name != L"tblNodes" && g_editOpen) continue;
            const int row = xmltable::hitRow(kv.second, p);
            if (row < 0) continue;
            g_tableSel[kv.first] = row;
            if (kv.first->name == L"tblNodes" && row < (int)kv.second.rows.size())
            {
                const std::vector<std::wstring>& cells = kv.second.rows[(size_t)row];
                if (!cells.empty()) editorSelectNode(cells[0]);
            }
            ::InvalidateRect(hwnd, nullptr, FALSE);
            return 0;
        }
        // 自绘普通按钮（侧板上的 -1/+1/应用/关闭）
        for (auto& kv : g_buttonHits)
        {
            if (!inRectPt(kv.second, p)) continue;
            const bool onEditor = (kv.first->name == L"btnLoadUp" || kv.first->name == L"btnLoadDown" ||
                                   kv.first->name == L"btnApply" || kv.first->name == L"btnClose");
            if (onEditor && !g_editOpen) continue;
            viewerCommand(hwnd, kv.first->name);
            ::InvalidateRect(hwnd, nullptr, FALSE);
            return 0;
        }
        if (PowerSlot* slot = powerSlotAt(p))
        {
            if (slot->view.mini)
            {
                // 小地图：按下即居中，按住拖动则连续跟随（模型坐标由控件库换算）
                g_miniDragEl = powerElOf(slot);
                ::SetCapture(hwnd);
                float mx = 0.0f, my = 0.0f;
                if (slot->view.miniPoint(p.x, p.y, mx, my))
                {
                    if (xmlctl::PowerCanvasView* pv = mainPowerView()) pv->centerOn(mx, my);
                    ::InvalidateRect(hwnd, nullptr, FALSE);
                }
                return 0;
            }
            slot->view.sel = slot->view.hit(p.x, p.y);
            viewerSyncSelection();
            ::InvalidateRect(hwnd, nullptr, FALSE);
            return 0;
        }
        if (g_ribbonValid)
        {
            const xmlctl::XRibbonHit hit = xmlctl::ribbonHitTest(g_ribbonLayout, p);
            if (hit.tab >= 0)
            {
                if (hit.tab != g_ribbonModel.selected)
                {
                    g_ribbonSel = hit.tab;          // 重绘时按它覆盖 XML 的 SelectedTab
                    g_ribbonModel.selected = hit.tab;
                    ::InvalidateRect(hwnd, nullptr, FALSE);
                }
                return 0;
            }
            if (hit.button >= 0 && !g_ribbonModel.tabs.empty())
            {
                const xmlctl::XRibbonTab* tab = g_ribbonModel.current();
                if (tab != nullptr && hit.group >= 0 && hit.group < (int)tab->groups.size())
                {
                    const xmlctl::XRibbonGroup& grp = tab->groups[(size_t)hit.group];
                    if (hit.button < (int)grp.buttons.size())
                        viewerCommand(hwnd, grp.buttons[(size_t)hit.button].name);
                }
                return 0;
            }
        }
        break;
    }
    case WM_LBUTTONUP:
        if (g_miniDragEl != nullptr)
        {
            g_miniDragEl = nullptr;
            if (::GetCapture() == hwnd) ::ReleaseCapture();
            return 0;
        }
        break;

    case WM_MBUTTONDOWN:
    {
        POINT p{ GET_X_LPARAM(lp), GET_Y_LPARAM(lp) };
        if (PowerSlot* slot = powerSlotAt(p); slot != nullptr && !slot->view.mini)
        {
            g_panEl = g_powerSlots.empty() ? nullptr : nullptr;
            for (auto& kv : g_powerSlots) if (&kv.second == slot) g_panEl = kv.first;
            g_panning = true;
            g_panLast = p;
            ::SetCapture(hwnd);
            ::SetCursor(::LoadCursor(nullptr, IDC_SIZEALL));
            return 0;
        }
        break;
    }
    case WM_MBUTTONUP:
        if (g_panning)
        {
            g_panning = false;
            g_panEl = nullptr;
            if (::GetCapture() == hwnd) ::ReleaseCapture();
            return 0;
        }
        break;
    case WM_MOUSEMOVE:
        if (g_miniDragEl != nullptr)
        {
            // 小地图拖动：光标离开小地图也继续跟（SetCapture 保证消息还到这里）
            POINT p{ GET_X_LPARAM(lp), GET_Y_LPARAM(lp) };
            if (auto it = g_powerSlots.find(g_miniDragEl); it != g_powerSlots.end())
            {
                float mx = 0.0f, my = 0.0f;
                if (it->second.view.miniPoint(p.x, p.y, mx, my))
                {
                    if (xmlctl::PowerCanvasView* pv = mainPowerView()) pv->centerOn(mx, my);
                    ::InvalidateRect(hwnd, nullptr, FALSE);
                }
            }
            return 0;
        }
        if (g_panning && g_panEl != nullptr)
        {
            POINT p{ GET_X_LPARAM(lp), GET_Y_LPARAM(lp) };
            if (auto it = g_powerSlots.find(g_panEl); it != g_powerSlots.end())
            {
                it->second.view.panBy(p.x - g_panLast.x, p.y - g_panLast.y);
                g_panLast = p;
                ::InvalidateRect(hwnd, nullptr, FALSE);
            }
            return 0;
        }
        break;
    case WM_MOUSEWHEEL:
    {
        POINT p{ GET_X_LPARAM(lp), GET_Y_LPARAM(lp) };
        ::ScreenToClient(hwnd, &p);
        if (PowerSlot* slot = powerSlotAt(p); slot != nullptr && !slot->view.mini)
        {
            slot->view.wheelAt(p.x, p.y, static_cast<short>(GET_WHEEL_DELTA_WPARAM(wp)));
            ::InvalidateRect(hwnd, nullptr, FALSE);
            return 0;
        }
        break;
    }

    case WM_DRAWITEM:
        // owner-draw：按钮/标签的外观由 D2D 画到控件自己的 HDC
        if (g_host != nullptr)
        {
            g_host->onDrawItem(reinterpret_cast<LPDRAWITEMSTRUCT>(lp));
            return 0;
        }
        break;

    case WM_CTLCOLOREDIT:
    case WM_CTLCOLORLISTBOX:
    case WM_CTLCOLORSTATIC:
        if (g_host != nullptr)
        {
            CDC* pdc = CDC::FromHandle(reinterpret_cast<HDC>(wp));
            CWnd* wnd = CWnd::FromHandle(reinterpret_cast<HWND>(lp));
            if (HBRUSH br = g_host->onCtlColor(pdc, wnd, static_cast<UINT>(msg)))
                return reinterpret_cast<LRESULT>(br);
        }
        break;

    case WM_DESTROY:
        if (g_host != nullptr)
        {
            g_host->destroyAll();
            delete g_host;
            g_host = nullptr;
        }
        g_bindings.clear();
        ::PostQuitMessage(0);
        return 0;
    }
    return ::DefWindowProcW(hwnd, msg, wp, lp);
}


// 进程累计 CPU 时间（内核+用户，毫秒）与工作集（KB），用来把"等 vsync"和"真算力"分开
static DWORDLONG procCpuMs()
{
    FILETIME c, e, k, u;
    if (!::GetProcessTimes(::GetCurrentProcess(), &c, &e, &k, &u)) return 0;
    ULARGE_INTEGER kk = {{k.dwLowDateTime, k.dwHighDateTime}};
    ULARGE_INTEGER uu = {{u.dwLowDateTime, u.dwHighDateTime}};
    return (kk.QuadPart + uu.QuadPart) / 10000ULL;
}

static DWORDLONG procWsKb()
{
    PROCESS_MEMORY_COUNTERS pmc = {};
    if (!::GetProcessMemoryInfo(::GetCurrentProcess(), &pmc, sizeof(pmc))) return 0;
    return static_cast<DWORDLONG>(pmc.WorkingSetSize / 1024);
}

static int showPreview(const std::wstring& title, int contentW, int contentH,
                       const std::wstring& iconPath = std::wstring())
{
    WNDCLASSEXW wc = {0};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = viewerProc;
    wc.hInstance = ::GetModuleHandleW(nullptr);
    wc.hCursor = ::LoadCursorW(nullptr, IDC_ARROW);
    wc.lpszClassName = L"XmlViewerWnd";
    wc.style = CS_HREDRAW | CS_VREDRAW;   // 拉窗口时整幅重绘（D2D 渲染目标随之重建）
    wc.hbrBackground = ::CreateSolidBrush(RGB(238, 240, 243));   // 首帧/未画到的像素一律应用底色，不闪白
    ::RegisterClassExW(&wc);

    RECT rc = {0, 0, contentW, contentH};

    // 外壳两种形态：默认无框（与设计器 / 运行时同观感），PYARX_FRAME=native 走系统标题栏
    wchar_t frameMode[16] = {};
    const bool frameless = !(::GetEnvironmentVariableW(L"PYARX_FRAME", frameMode, 16) > 0 &&
                             _wcsicmp(frameMode, L"native") == 0);
    if (!frameless)
        ::AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);

    HWND hwnd = ::CreateWindowExW(0, L"XmlViewerWnd", title.c_str(),
                                  frameless ? xmlframe::framelessStyle()
                                            : (WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN),
                                  CW_USEDEFAULT, CW_USEDEFAULT,
                                  rc.right - rc.left, rc.bottom - rc.top,
                                  nullptr, nullptr, wc.hInstance, nullptr);
    if (hwnd == nullptr) return 1;

    // Window.Icon → 标题栏左上角图标（相对 exe 目录），与设计器 / CAD 运行时同一套规则
    if (!iconPath.empty())
    {
        std::wstring ip = iconPath;
        const bool isDir = (ip.back() == L'\\' || ip.back() == L'/');
        if (!isDir)
        {
            const bool isAbs = (ip.size() > 2 && ip[1] == L':') || ip[0] == L'\\' || ip[0] == L'/';
            if (!isAbs)
                ip = exeDir() + L"\\" + ip;
            if (HICON icon = xmlIconFromFile(ip))
            {
                ::SendMessageW(hwnd, WM_SETICON, ICON_BIG, reinterpret_cast<LPARAM>(icon));
                ::SendMessageW(hwnd, WM_SETICON, ICON_SMALL, reinterpret_cast<LPARAM>(icon));
            }
        }
    }

    // 摆到工作区内，避免出现在屏幕左侧之外
    RECT work = {0, 0, 0, 0};
    if (::SystemParametersInfoW(SPI_GETWORKAREA, 0, &work, 0))
    {
        RECT wr;
        ::GetWindowRect(hwnd, &wr);
        int x = work.left + 20;
        int y = work.top + 20;
        int ww = (std::min)((int)(wr.right - wr.left), (int)(work.right - work.left) - 40);
        int hh = (std::min)((int)(wr.bottom - wr.top), (int)(work.bottom - work.top) - 60);
        ::MoveWindow(hwnd, x, y, ww, hh, TRUE);
    }

    ::ShowWindow(hwnd, SW_SHOW);
    ::UpdateWindow(hwnd);

    // 基准：连续强制整幅重绘，量化后端的单帧耗时与 GDI 资源占用。
    // XMLVIEWER_BENCH=<帧数> + XMLVIEWER_BENCH_OUT=<输出文件>
    {
        wchar_t en[16] = {};
        const int frames = (::GetEnvironmentVariableW(L"XMLVIEWER_BENCH", en, 16) >= 1) ? _wtoi(en) : 0;
        if (frames > 0)
        {
            LARGE_INTEGER freq = {}, t0 = {}, t1 = {};
            ::QueryPerformanceFrequency(&freq);
            const DWORDLONG cpu0 = procCpuMs();
            const DWORDLONG ws0 = procWsKb();
            ::QueryPerformanceCounter(&t0);
            for (int i = 0; i < frames; ++i)
            {
                ::InvalidateRect(hwnd, nullptr, TRUE);
                ::UpdateWindow(hwnd);
            }
            ::QueryPerformanceCounter(&t1);
            const DWORDLONG cpu1 = procCpuMs();
            const DWORDLONG ws1 = procWsKb();
            const double ms = (t1.QuadPart - t0.QuadPart) * 1000.0 / static_cast<double>(freq.QuadPart);
            const double cpu = static_cast<double>(cpu1 - cpu0);
            const DWORD gdi = ::GetGuiResources(::GetCurrentProcess(), GR_GDIOBJECTS);
            const DWORD usr = ::GetGuiResources(::GetCurrentProcess(), GR_USEROBJECTS);
            wchar_t line[256] = {};
            swprintf_s(line,
                       L"frames=%d wall=%.1fms per_frame=%.3fms cpu=%.1fms cpu_per_frame=%.3fms ws0=%lluKB ws1=%lluKB gdi=%lu user=%lu\n",
                       frames, ms, ms / frames, cpu, cpu / frames,
                       static_cast<unsigned long long>(ws0), static_cast<unsigned long long>(ws1), gdi, usr);
            ::OutputDebugStringW(line);
            wchar_t out[MAX_PATH] = {};
            if (::GetEnvironmentVariableW(L"XMLVIEWER_BENCH_OUT", out, MAX_PATH) >= 1)
            {
                FILE* f = _wfopen(out, L"a, ccs=UTF-8");
                if (f != nullptr) { fputws(line, f); fclose(f); }
            }
            ::DestroyWindow(hwnd);
            return 0;
        }
    }

    MSG msg;
    while (::GetMessageW(&msg, nullptr, 0, 0) > 0)
    {
        ::TranslateMessage(&msg);
        ::DispatchMessageW(&msg);
    }
    return 0;
}

static void dumpTree(XmlElement* el, int depth)
{
    std::wstring pad((size_t)depth * 2, L' ');
    wprintf(L"%s%s[%s] decl(x=%d y=%d w=%d h=%d) prev(x=%d y=%d w=%d h=%d)\n", pad.c_str(),
            el->tag.c_str(), el->name.c_str(), el->x, el->y, el->w, el->h,
            el->previewX, el->previewY, el->previewW, el->previewH);
    for (auto& child : el->children)
        dumpTree(&child, depth + 1);
}

static void dumpTracks(XmlElement* el, int depth)
{
    std::wstring pad((size_t)depth * 2, L' ');
    for (auto& kv : el->gridRows)
        wprintf(L"%s  rowDef %s=%s\n", pad.c_str(), kv.first.c_str(), kv.second.c_str());
    for (auto& kv : el->gridCols)
        wprintf(L"%s  colDef %s=%s\n", pad.c_str(), kv.first.c_str(), kv.second.c_str());
    for (auto& child : el->children)
        dumpTracks(&child, depth + 1);
}

// ---------------------------------------------------------------------------
// 主流程
// ---------------------------------------------------------------------------
static void printUsage()
{
    wprintf(L"XmlViewer —— XML 预览与校验器\n");
    wprintf(L"用法:\n");
    wprintf(L"  XmlViewer.exe <file.xml>              解析 + 校验 + 渲染窗口\n");
    wprintf(L"  XmlViewer.exe --check <file.xml>      只校验，打印错误位置\n");
    wprintf(L"  XmlViewer.exe --check --dump <file>    同时打印布局结果\n");
    wprintf(L"  XmlViewer.exe --check --layout <file>  额外做布局越界检查（诊断用）\n");
    wprintf(L"  XmlViewer.exe --catalog <path> ...     指定控件目录（默认 exe 同目录 control_catalog.xml）\n");
}

static int runViewer(int argc, wchar_t** argv)
{
    ::CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);

    std::wstring filePath;
    std::wstring catalogPath = exeDir() + L"\\control_catalog.xml";
    bool checkOnly = false;
    bool dumpLayout = false;
    bool layoutCheck = false;

    for (int i = 1; i < argc; ++i)
    {
        std::wstring a = argv[i];
        if (a == L"--check")
        {
            checkOnly = true;
        }
        else if (a == L"--dump")
        {
            dumpLayout = true;
        }
        else if (a == L"--layout")
        {
            layoutCheck = true;
        }
        else if (a == L"--catalog" && i + 1 < argc)
        {
            catalogPath = argv[++i];
        }
        else if (a == L"--help" || a == L"-h" || a == L"/?")
        {
            printUsage();
            return 0;
        }
        else
        {
            filePath = a;
        }
    }

    if (filePath.empty())
    {
        printUsage();
        ::MessageBoxW(nullptr,
                      L"用法：\n"
                      L"  XmlViewer.exe <file.xml>              渲染预览（无控制台窗口）\n"
                      L"  XmlViewer.exe --check <file.xml>      只做语法/语义校验，返回错误节点位置\n"
                      L"  XmlViewer.exe --check --dump <file>    校验并打印布局结果\n"
                      L"  --catalog <path>   指定 control_catalog.xml（默认取 exe 同目录）\n\n"
                      L"也可以把 .xml 文件直接拖到本 exe 图标上。",
                      L"XmlViewer", MB_OK | MB_ICONINFORMATION);
        return 2;
    }

    g_catalog = loadCatalog(catalogPath);
    if (g_catalog.empty() && !checkOnly)
        wprintf(L"[提示] 未读到控件目录 %s，未知标签检查将被跳过\n", catalogPath.c_str());

    std::wstring text = readAllText(filePath);
    if (text.empty())
    {
        wprintf(L"[错误] 无法读取文件: %s\n", filePath.c_str());
        if (!checkOnly)
        {
            ::MessageBoxW(nullptr,
                          (L"无法读取 XML 文件：\n\n" + filePath +
                           L"\n\n文件不存在、无权限，或内容为空。").c_str(),
                          L"XmlViewer", MB_OK | MB_ICONERROR);
        }
        return 1;
    }

    // 1) XML 解析（MSXML 给出精确行列）
    CComPtr<IXMLDOMDocument2> dom;
    if (FAILED(::CoCreateInstance(CLSID_DOMDocument60, nullptr, CLSCTX_INPROC_SERVER,
                                  IID_IXMLDOMDocument2, reinterpret_cast<void**>(&dom))))
    {
        wprintf(L"[错误] 无法创建 XML 解析器\n");
        return 1;
    }
    dom->put_async(VARIANT_FALSE);
    dom->put_validateOnParse(VARIANT_FALSE);
    dom->put_resolveExternals(VARIANT_FALSE);

    VARIANT_BOOL loaded = VARIANT_FALSE;
    dom->load(_variant_t(filePath.c_str()), &loaded);

    CComPtr<IXMLDOMParseError> parseError;
    dom->get_parseError(&parseError);
    if (parseError != nullptr)
    {
        long line = 0, linePos = 0;
        parseError->get_line(&line);
        parseError->get_linepos(&linePos);
        CComBSTR reason;
        parseError->get_reason(&reason);

        if (loaded != VARIANT_TRUE || line > 0)
        {
            wprintf(L"[错误] XML 解析失败\n");
            wprintf(L"        文件: %s\n", filePath.c_str());
            wprintf(L"        位置: 第 %ld 行 第 %ld 列\n", line, linePos);
            wprintf(L"        原因: %s\n", reason ? std::wstring(reason).c_str() : L"(未知)");
            if (!checkOnly)
            {
                // 渲染模式没有控制台：必须弹窗，否则用户只看到"什么都没打开"
                std::wstring msg = L"XML 语法错误，无法预览：\n\n" + filePath + L"\n\n";
                msg += L"第 " + std::to_wstring(line) + L" 行，第 " + std::to_wstring(linePos) + L" 列\n";
                msg += (reason ? std::wstring(reason) : std::wstring(L"(未知原因)")) + L"\n\n";
                msg += L"提示：用 XmlViewer.exe --check <file.xml> 可在控制台看到完整诊断。";
                ::MessageBoxW(nullptr, msg.c_str(), L"XmlViewer", MB_OK | MB_ICONERROR);
            }
            return 1;
        }
    }

    // 2) 语义校验
    std::vector<TagSite> sites = scanTagSites(text);
    std::set<std::wstring> seenNames;
    {
        const size_t p = filePath.find_last_of(L"\\/");
        g_docDir = p == std::wstring::npos ? std::wstring() : filePath.substr(0, p);
    }
    // 先让运行时加载一遍，拿到"已合并资源字典"后的模板键表，供 Template 引用校验
    if (checkOnly)
    {
        XmlRuntime probe;
        std::wstring probeError;
        if (probe.loadFile(filePath.c_str(), probeError))
        {
            for (const std::wstring& k : probe.templateKeys())
                g_templateKeys.insert(k);
            g_templateKeysReady = true;
        }
    }
    CComPtr<IXMLDOMElement> rootEl;
    dom->get_documentElement(&rootEl);
    if (rootEl != nullptr)
    {
        size_t cursor = 0;
        validateElement(rootEl, L"", sites, cursor, seenNames);
    }

    // 3) 运行时加载 + 布局（几何越界检查）
    XmlRuntime runtime;
    std::wstring loadError;
    if (!runtime.loadFile(filePath.c_str(), loadError))
    {
        if (checkOnly)
        {
            wprintf(L"[错误] 运行时加载失败: %s\n", loadError.c_str());
            if (g_issues.empty()) return 1;
        }
        else
        {
            // 渲染模式下没有控制台，直接 return 会让进程无声退出，用户只看到"什么都没打开"。
            // 这里必须把失败原因摆到用户面前。
            std::wstring msg = L"XML 加载失败：\n\n" + filePath + L"\n\n";
            if (!loadError.empty()) msg += loadError + L"\n\n";
            const Issue* firstErr = nullptr;
            for (const Issue& it : g_issues)
            {
                if (it.isError) { firstErr = &it; break; }
            }
            if (firstErr != nullptr)
            {
                msg += L"第 " + std::to_wstring(firstErr->line) + L" 行";
                if (!firstErr->path.empty()) msg += L"（" + firstErr->path + L"）";
                msg += L"\n" + firstErr->message + L"\n\n";
            }
            msg += L"提示：用 XmlViewer.exe --check <file.xml> 可列出全部问题。";
            ::MessageBoxW(nullptr, msg.c_str(), L"XmlViewer", MB_OK | MB_ICONERROR);
            return 1;
        }
    }
    else
    {
        XmlElement* root = runtime.root();
        if (root != nullptr)
        {
            int w = root->w > 0 ? root->w : 640;
            int h = root->h > 0 ? root->h : 480;
            runtime.layout(0, 0, w, h);

            // 越界检查（--layout 时才做；跳过菜单项/行列定义等非可视元数据）
            struct WalkItem
            {
                XmlElement* el;
                std::wstring path;
                bool skip;
            };
            std::vector<WalkItem> stack;
            stack.push_back({root, root->tag, true});
            while (!stack.empty())
            {
                WalkItem item = stack.back();
                stack.pop_back();
                XmlElement* el = item.el;

                bool skip = item.skip || !layoutCheck ||
                            _wcsicmp(el->tag.c_str(), L"MenuBar") == 0 ||
                            _wcsicmp(el->tag.c_str(), L"MenuItem") == 0 ||
                            _wcsicmp(el->tag.c_str(), L"Separator") == 0 ||
                            _wcsicmp(el->tag.c_str(), L"RowDefinition") == 0 ||
                            _wcsicmp(el->tag.c_str(), L"ColumnDefinition") == 0 ||
                            el->tag.find(L'.') != std::wstring::npos;

                if (!skip && el != root)
                {
                    if (el->previewX < 0 || el->previewY < 0)
                    {
                        Issue is;
                        is.isError = false;
                        is.line = 0;
                        is.path = item.path;
                        is.message = L"节点坐标为负 (Left=" + std::to_wstring(el->previewX) +
                                     L", Top=" + std::to_wstring(el->previewY) + L")";
                        g_issues.push_back(is);
                    }
                    if (el->previewX + el->previewW > w + 4 || el->previewY + el->previewH > h + 4)
                    {
                        Issue is;
                        is.isError = false;
                        is.line = 0;
                        is.path = item.path;
                        is.message = L"节点超出根范围: 右下 (" +
                                     std::to_wstring(el->previewX + el->previewW) + L", " +
                                     std::to_wstring(el->previewY + el->previewH) + L") > (" +
                                     std::to_wstring(w) + L", " + std::to_wstring(h) + L")";
                        g_issues.push_back(is);
                    }
                }
                for (auto& child : el->children)
                    stack.push_back({&child, item.path + L"/" + child.tag, skip});
            }
        }
    }

    // 4) 输出校验结果
    if (dumpLayout && runtime.root() != nullptr)
    {
        wprintf(L"--- 布局结果 ---\n");
        dumpTree(runtime.root(), 0);
        dumpTracks(runtime.root(), 0);
        wprintf(L"---\n");
    }
    {
        int errCount = 0, warnCount = 0;
        for (auto& is : g_issues)
            (is.isError ? errCount : warnCount)++;

        wprintf(L"%s\n", filePath.c_str());
        for (auto& is : g_issues)
        {
            if (is.line > 0)
                wprintf(L"[%s] 第 %d 行  %s\n", is.isError ? L"错误" : L"警告", is.line, is.path.c_str());
            else
                wprintf(L"[%s] %s\n", is.isError ? L"错误" : L"警告", is.path.c_str());
            wprintf(L"        %s\n", is.message.c_str());
        }
        if (errCount == 0 && warnCount == 0)
            wprintf(L"[通过] 未发现语法或语义问题\n");
        else
            wprintf(L"合计: %d 个错误, %d 个警告\n", errCount, warnCount);

        if (errCount > 0)
        {
            if (checkOnly) return 1;
            // 渲染模式不能直接退出：预览器的价值恰恰是"把有问题的文档也画出来"，
            // 让用户看见坏在哪。这里把问题列出来提示，然后继续预览。
            std::wstring msg = L"文档存在 " + std::to_wstring(errCount) + L" 个错误，仍将继续预览：\n\n";
            int shown = 0;
            for (const Issue& is : g_issues)
            {
                if (!is.isError) continue;
                if (shown >= 6) { msg += L"…（其余用 --check 查看）\n"; break; }
                msg += L"第 " + std::to_wstring(is.line) + L" 行";
                if (!is.path.empty()) msg += L" " + is.path;
                msg += L"\n    " + is.message + L"\n";
                ++shown;
            }
            ::MessageBoxW(nullptr, msg.c_str(), L"XmlViewer — 文档有问题", MB_OK | MB_ICONWARNING);
        }
    }

    if (checkOnly)
        return 0;

    // 5) 渲染预览窗口
    XmlElement* root = runtime.root();
    if (root == nullptr)
    {
        wprintf(L"[错误] 文档为空\n");
        ::MessageBoxW(nullptr,
                      (L"文档解析成功但没有可视根节点：\n\n" + filePath +
                       L"\n\n根元素不是 Window/Page 之类的可视容器。").c_str(),
                      L"XmlViewer", MB_OK | MB_ICONERROR);
        return 1;
    }
    g_root = root;
    // 新文档：画布槽表重来（旧树已释放，指针不能复用）
    g_powerSlots.clear();
    g_powerByName.clear();
    g_powerMainEl = nullptr;
    g_panEl = nullptr;
    g_editOpen = false;
    g_editCode.clear();
    g_editLoad = 0.0;
    g_tableSel.clear();
    g_tableHits.clear();
    g_buttonHits.clear();
    g_layoutDirty = true;
    int contentW = (root->w > 0 ? root->w : 640) + 40;
    int contentH = (root->h > 0 ? root->h : 480) + 40;
    if (contentW < 320) contentW = 320;
    if (contentH < 240) contentH = 240;
    if (contentW > 1900) contentW = 1900;
    if (contentH > 1100) contentH = 1100;

    // 不超出屏幕工作区，免得窗口比屏幕还大导致被别的窗口挡住
    RECT work;
    if (::SystemParametersInfoW(SPI_GETWORKAREA, 0, &work, 0))
    {
        const int maxW = work.right - work.left - 30;
        const int maxH = work.bottom - work.top - 50;
        if (contentW > maxW) contentW = maxW;
        if (contentH > maxH) contentH = maxH;
    }

    // WPF 对齐：根节点的 Title 作为窗口标题；没写就退回"文件名"模板
    std::wstring title = L"XmlViewer - ";
    if (root != nullptr)
    {
        const std::wstring xmlTitle = root->get(L"Title");
        if (!xmlTitle.empty())
        {
            title += xmlTitle + L" (" + filePath + L")";
        }
        else
            title += filePath;
    }
    else
        title += filePath;
    // 预览期间把文档对象挂到全局：窗口过程按需调 layout（属性改了要能重排）
    g_runtime = &runtime;
    g_layoutDirty = true;
    int rc = showPreview(title, contentW, contentH,
                         (root != nullptr) ? root->icon : std::wstring());
    g_runtime = nullptr;
    ::CoUninitialize();
    return rc;
}

// 入口：Windows 子系统 -> 渲染时不会弹出控制台窗口
int WINAPI wWinMain(HINSTANCE hInst, HINSTANCE, LPWSTR, int)
{
    // MFC 控件类（CButton/CComboBox…）要求先初始化 MFC 运行时；
    // --check 模式也用得到（解析本身不建控件，但统一初始化最省心）
    if (!AfxWinInit(hInst, nullptr, ::GetCommandLineW(), 0))
        return 2;

    int argc = 0;
    LPWSTR* argv = ::CommandLineToArgvW(::GetCommandLineW(), &argc);
    if (argv == nullptr)
        return 1;
    int result = runViewer(argc, argv);
    ::LocalFree(argv);
    return result;
}
