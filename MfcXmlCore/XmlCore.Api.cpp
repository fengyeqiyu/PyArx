// XmlCore.Api.cpp —— 拆自 XmlCore.cpp 第 3495-3685 行。
#include "XmlCoreCommon.h"
#include "XmlCoreInternal.h"

// ===========================================================================
// 工具导出
// ===========================================================================
namespace xmlcore_impl
{
    // 供 xmlLayoutSubtree 使用的入口
    void LayoutNodeAt(XmlElement& root, int rootX, int rootY, int rootW, int rootH)
    {
        root.previewX = rootX;
        root.previewY = rootY;
        root.previewW = (std::max)(1, rootW > 0 ? rootW : root.w);
        root.previewH = (std::max)(1, rootH > 0 ? rootH : root.h);
        // 根节点按普通容器处理：根自身无外边距，子元素贴顶贴左
        XmlFlow flow;
        flow.width = root.previewW;
        flow.height = root.previewH;
        flow.offsetX = root.previewX;
        flow.offsetY = root.previewY;
        flow.margin = 0;
        flow.spacing = root.spacing;
        flow.padding = root.padL;
        flow.paddingY = root.padT;
        flow.cursorX = root.padL;        flow.cursorY = root.padT;
        if (g_layoutDbg)
        {
            FILE* f = nullptr;
            const std::wstring p = xmlprop2::LogDir() + L"\\xml_layout.log";
            if (_wfopen_s(&f, p.c_str(), L"a") == 0 && f != nullptr)
            {
                fprintf(f, "=== root tag=%s addr=%p spacing=%d pad=%d rect=%d,%d,%d,%d children=%d\n",
                        xmlprop::WideToUtf8(root.tag).c_str(), (void*)&root, root.spacing, root.padL,
                        rootX, rootY, root.previewW, root.previewH,
                        (int)root.children.size());
                fclose(f);
            }
        }
        LayoutNode(root, flow);
    }
}

void xmlLayoutSubtree(XmlElement& root, int rootX, int rootY, int rootW, int rootH)
{
    LayoutNodeAt(root, rootX, rootY, rootW, rootH);
}

int xmlNaturalHeight(const XmlElement& el)
{
    if (el.hasH)
        return el.h;
    const std::wstring tag = KindOf(el.tag);
    if (el.children.empty())
        return DefaultHeight(tag);
    int content = 0;
    int count = 0;
    if (tag == L"grid")
    {
        // 网格按"行"量：同一行取最高单元格，行高再相加（逐格累加会把并排的格子算两遍）
        std::map<int, int> rowH;
        for (const XmlElement& child : el.children)
        {
            const std::wstring ct = KindOf(child.tag);
            if (ct == L"rowdefinition" || ct == L"columndefinition" || ct == L"page" ||
                ct == L"style" || ct == L"resources")
                continue;
            const int ch = xmlNaturalHeight(child) + child.marginT + child.marginB;
            auto it = rowH.find(child.row);
            if (it == rowH.end())
                rowH.emplace(child.row, ch);
            else
                it->second = (std::max)(it->second, ch);
        }
        int total = 0;
        for (const auto& kv : rowH)
            total += kv.second;
        return (std::max)(1, total + el.padT + el.padB);
    }
    for (const XmlElement& child : el.children)
    {
        const std::wstring ct = KindOf(child.tag);
        if (ct == L"rowdefinition" || ct == L"columndefinition" || ct == L"page" ||
            ct == L"style" || ct == L"resources")
            continue;
        const int ch = xmlNaturalHeight(child);
        content = el.horizontal ? (std::max)(content, ch) : content + ch;
        ++count;
    }
    const int gap = el.horizontal ? 0 : el.spacing * (std::max)(0, count - 1);
    return (std::max)(1, content + gap + el.padT + el.padB);
}

void xmlSetDefaultSize(const wchar_t* tag, int w, int h)
{
    if (tag == nullptr || tag[0] == 0)
        return;
    if (w <= 0) w = 120;
    if (h <= 0) h = 28;
    const std::wstring key = KindOf(std::wstring(tag));
    for (auto& it : layoutDefaults().sizes)
    {
        if (it.first == key)
        {
            it.second.first = w;
            it.second.second = h;
            return;
        }
    }
    layoutDefaults().sizes.emplace_back(key, std::make_pair(w, h));
}

bool xmlGetDefaultSize(const wchar_t* tag, int& w, int& h)
{
    const std::wstring key = KindOf(tag != nullptr ? std::wstring(tag) : std::wstring());
    for (const auto& it : layoutDefaults().sizes)
    {
        if (it.first == key)
        {
            w = it.second.first;
            h = it.second.second;
            return true;
        }
    }
    return false;
}

void xmlSetStarMinPx(int px)
{
    layoutDefaults().starMin = px > 0 ? px : 30;
}

void xmlSetGroupInset(int left, int top)
{
    layoutDefaults().groupLeft = left >= 0 ? left : 12;
    layoutDefaults().groupTop = top >= 0 ? top : 24;
}

std::wstring xmlColorToString(COLORREF c)
{
    wchar_t buf[16];
    swprintf_s(buf, L"#%02X%02X%02X", GetRValue(c), GetGValue(c), GetBValue(c));
    return buf;
}

bool xmlParseColor(const std::wstring& s, COLORREF& out)
{
    std::wstring v = s;
    // trim
    size_t b = v.find_first_not_of(L" \t\r\n");
    if (b == std::wstring::npos) return false;
    v = v.substr(b);
    size_t e = v.find_last_not_of(L" \t\r\n");
    if (e != std::wstring::npos) v = v.substr(0, e + 1);

    if (v.empty()) return false;
    if (v[0] == L'#')
    {
        // #AARRGGBB（8 位）的数值可能是 0xFFxxxxxx，超过 LONG_MAX。
        // 用有符号 long 接会溢出成 LONG_MAX，颜色整体变白。
        const unsigned long n = wcstoul(v.c_str() + 1, nullptr, 16);
        out = RGB((n >> 16) & 0xFF, (n >> 8) & 0xFF, n & 0xFF);
        return true;
    }
    if (v.find(L"RGB") == 0 || v.find(L"rgb") == 0)
    {
        int a, b, c;
        if (swscanf_s(v.c_str(), L"RGB(%d,%d,%d)", &a, &b, &c) == 3 ||
            swscanf_s(v.c_str(), L"rgb(%d,%d,%d)", &a, &b, &c) == 3)
        {
            out = RGB(a, b, c);
            return true;
        }
    }
    return false;
}

std::vector<std::wstring> xmlSplit(const std::wstring& s, wchar_t sep)
{
    std::vector<std::wstring> out;
    std::wstring cur;
    for (wchar_t c : s)
    {
        if (c == sep)
        {
            if (!cur.empty()) out.push_back(cur);
            cur.clear();
        }
        else cur.push_back(c);
    }
    if (!cur.empty()) out.push_back(cur);
    return out;
}
