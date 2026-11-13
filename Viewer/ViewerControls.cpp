// ViewerControls —— 预览器的控件实体化实现。
//
// 结构：
//   * 静态 tag → 控件种类表（不用 if-else 链，加控件只加一行）
//   * walk()：与绘制同一套矩形语义（previewX/Y/W/H + 原点偏移），
//     容器继续递归、叶子建原生控件
//   * 创建期样式（多行/只读/限长/占位/下拉风格/多选）从 XML 属性解析后交给宿主
//
// 编码：本文件 UTF-8 with BOM。

#include "ViewerControls.h"
#include "../MfcXmlCore/controls/XmlMfcTags.h"   // 容器标签唯一来源（与运行时同一份）

#include <cwctype>
#include <map>

namespace
{
    enum LeafKind
    {
        K_None = 0,
        K_Button,
        K_Toggle,
        K_BitmapButton,
        K_Check,
        K_Radio,
        K_Label,
        K_Hyperlink,
        K_Edit,
        K_RichText,
        K_Combo,
        K_List,
        K_Progress,
        K_Slider,
        K_Tab,
        K_Tree,
        K_ListCtrl,
        K_GridCtrl,
        K_Date,
        K_Calendar,
        K_StaticBitmap,
        K_Panel,
        K_Splitter,
        K_Line,
        K_Scrolled,
    };

    // 标签 → 控件种类。加一种控件只在这里加一行。
    struct TagKind
    {
        const wchar_t* tag;
        LeafKind kind;
    };

    const TagKind kTagKinds[] = {
        {L"Button", K_Button},
        {L"ToggleButton", K_Toggle},
        {L"IconButton", K_BitmapButton},
        {L"CheckBox", K_Check},
        {L"RadioButton", K_Radio},
        {L"Label", K_Label},
        {L"Label", K_Label},
        {L"StatusBar", K_Label},
        {L"Hyperlink", K_Hyperlink},
        {L"TextField", K_Edit},
        {L"Edit", K_Edit},
        {L"MaskedTextBox", K_Edit},
        {L"TextArea", K_RichText},
        {L"ComboBox", K_Combo},
        {L"Choice", K_Combo},
        {L"ListBox", K_List},
        {L"ProgressBar", K_Progress},
        {L"Slider", K_Slider},
        {L"TabBar", K_Tab},
        {L"Tree", K_Tree},
        {L"TreeView", K_Tree},
        {L"ListCtrl", K_ListCtrl},
        {L"GridLayout", K_GridCtrl},
        {L"DataGrid", K_GridCtrl},
        {L"DatePicker", K_Date},
        {L"Calendar", K_Calendar},
        {L"Image", K_StaticBitmap},
        {L"Image", K_StaticBitmap},
        {L"Pane", K_Panel},
        {L"SplitView", K_Splitter},
        {L"Divider", K_Line},
        {L"Separator", K_Line},
        {L"ScrollView", K_Scrolled},
    };

    LeafKind kindOf(const std::wstring& tag)
    {
        for (const TagKind& k : kTagKinds)
            if (_wcsicmp(tag.c_str(), k.tag) == 0)
                return k.kind;
        return K_None;
    }

    std::wstring attrOf(const XmlElement& el, const wchar_t* key)
    {
        for (const auto& kv : el.attrs)
            if (_wcsicmp(kv.first.c_str(), key) == 0)
                return kv.second;
        for (const auto& kv : el.styledAttrs)
            if (_wcsicmp(kv.first.c_str(), key) == 0)
                return kv.second;
        return {};
    }

    int intAttrOf(const XmlElement& el, const wchar_t* key, int fallback)
    {
        const std::wstring v = attrOf(el, key);
        return v.empty() ? fallback : _wtoi(v.c_str());
    }

    bool boolAttrOf(const XmlElement& el, const wchar_t* key)
    {
        const std::wstring v = attrOf(el, key);
        return _wcsicmp(v.c_str(), L"true") == 0 || _wcsicmp(v.c_str(), L"yes") == 0 || v == L"1";
    }

    // 元素的有效外观：本地声明优先，其余取样式/绑定注入层（与绘制口径一致）
    xmlapp::XmlAppearance appearanceOf(const XmlElement& el)
    {
        xmlapp::XmlAppearance ap;
        for (const auto& kv : el.attrs)
            xmlapp::SetProperty(ap, kv.first, kv.second);
        for (const auto& kv : el.styledAttrs)
        {
            bool local = false;
            for (const auto& a : el.attrs)
                if (_wcsicmp(a.first.c_str(), kv.first.c_str()) == 0)
                {
                    local = true;
                    break;
                }
            if (!local)
                xmlapp::SetProperty(ap, kv.first, kv.second);
        }
        return ap;
    }

    ViewerLeafOptions optionsOf(const XmlElement& el)
    {
        ViewerLeafOptions o;
        o.multiline = boolAttrOf(el, L"Multiline");
        o.readOnly = boolAttrOf(el, L"ReadOnly") || boolAttrOf(el, L"IsReadOnly");
        o.maxLength = intAttrOf(el, L"MaxLength", 0);
        o.placeholder = attrOf(el, L"Placeholder");
        const std::wstring style = attrOf(el, L"DropDownStyle");
        o.dropDownList = !(_wcsicmp(style.c_str(), L"DropDown") == 0);
        const std::wstring mode = attrOf(el, L"SelectionMode");
        o.multipleSelect = _wcsicmp(mode.c_str(), L"Multiple") == 0 ||
                           _wcsicmp(mode.c_str(), L"MultiExtended") == 0;
        return o;
    }

    std::wstring textOf(const XmlElement& el)
    {
        if (!el.text.empty())
            return el.text;
        std::wstring t = attrOf(el, L"Text");
        if (t.empty()) t = attrOf(el, L"Content");
        if (t.empty()) t = attrOf(el, L"Header");
        return t;
    }

    // 建一个叶子控件。id 由调用方按顺序分配（唯一且 >= 1000，宿主用它做控件 ID）
    void createLeaf(ViewerMfcHost& host, const XmlElement& el, LeafKind kind, int id,
                    int x, int y, int w, int h)
    {
        const std::wstring text = textOf(el);
        switch (kind)
        {
        case K_Button: host.addButton(id, text, x, y, w, h); break;
        case K_Toggle: host.addToggle(id, text, x, y, w, h); break;
        case K_BitmapButton: host.addBitmapButton(id, text, x, y, w, h); break;
        case K_Check: host.addCheckBox(id, text, x, y, w, h); break;
        case K_Radio: host.addRadio(id, text, x, y, w, h); break;
        case K_Label: host.addLabel(id, text, x, y, w, h); break;
        case K_Hyperlink: host.addHyperlink(id, text, x, y, w, h); break;
        case K_Edit:
        {
            const ViewerLeafOptions o = optionsOf(el);
            host.addEditAdvanced(id, text, o.multiline, o.readOnly, o.maxLength, o.placeholder,
                                 x, y, w, h);
            break;
        }
        case K_RichText: host.addRichText(id, x, y, w, h); break;
        case K_Combo:
        {
            const ViewerLeafOptions o = optionsOf(el);
            host.addComboAdvanced(id, o.dropDownList, x, y, w, h);
            // Items="a|b|c"：按 '|' 拆开逐条加入
            const std::wstring items = attrOf(el, L"Items");
            size_t start = 0;
            while (start < items.size())
            {
                const size_t bar = items.find(L'|', start);
                const std::wstring item = (bar == std::wstring::npos)
                                              ? items.substr(start)
                                              : items.substr(start, bar - start);
                if (!item.empty())
                    host.addString(id, item);
                if (bar == std::wstring::npos)
                    break;
                start = bar + 1;
            }
            const int sel = intAttrOf(el, L"SelectedIndex", -1);
            if (sel >= 0) host.setCurSel(id, sel);
            else if (!text.empty()) host.setText(id, text);
            break;
        }
        case K_List:
        {
            const ViewerLeafOptions o = optionsOf(el);
            host.addListBoxAdvanced(id, o.multipleSelect, x, y, w, h);
            const std::wstring items = attrOf(el, L"Items");
            size_t start = 0;
            while (start < items.size())
            {
                const size_t bar = items.find(L'|', start);
                const std::wstring item = (bar == std::wstring::npos)
                                              ? items.substr(start)
                                              : items.substr(start, bar - start);
                if (!item.empty())
                    host.addString(id, item);
                if (bar == std::wstring::npos)
                    break;
                start = bar + 1;
            }
            const int sel = intAttrOf(el, L"SelectedIndex", -1);
            if (sel >= 0) host.setCurSel(id, sel);
            break;
        }
        case K_Progress: host.addProgress(id, x, y, w, h); break;
        case K_Slider:
            host.addSlider(id, intAttrOf(el, L"Minimum", 0), intAttrOf(el, L"Maximum", 100), x, y, w, h);
            break;
        case K_Tab: host.addTab(id, x, y, w, h); break;
        case K_Tree: host.addTree(id, x, y, w, h); break;
        case K_ListCtrl:
        case K_GridCtrl:
        {
            if (kind == K_GridCtrl) host.addGrid(id, x, y, w, h);
            else host.addListCtrl(id, x, y, w, h);

            // Columns="名称:180|状态:100"，Rows="甲\t1|乙\t2"：
            // 与引擎 xmlextra::addColumns / addRows 同一语法，不解析就是空白列表。
            const std::wstring cols = attrOf(el, L"Columns");
            size_t start = 0;
            while (start < cols.size())
            {
                const size_t bar = cols.find(L'|', start);
                std::wstring seg = (bar == std::wstring::npos) ? cols.substr(start) : cols.substr(start, bar - start);
                int cw = 120;
                const size_t colon = seg.rfind(L':');
                if (colon != std::wstring::npos)
                {
                    const std::wstring wt = seg.substr(colon + 1);
                    if (!wt.empty() && wt.find_first_not_of(L"0123456789") == std::wstring::npos)
                    {
                        cw = _wtoi(wt.c_str());
                        seg = seg.substr(0, colon);
                    }
                }
                if (!seg.empty())
                    host.addColumn(id, seg, cw);
                if (bar == std::wstring::npos) break;
                start = bar + 1;
            }
            const std::wstring rows = attrOf(el, L"Rows");
            start = 0;
            while (start < rows.size())
            {
                const size_t bar = rows.find(L'|', start);
                const std::wstring row = (bar == std::wstring::npos) ? rows.substr(start) : rows.substr(start, bar - start);
                if (!row.empty())
                    host.addRow(id, row);
                if (bar == std::wstring::npos) break;
                start = bar + 1;
            }
            break;
        }
        case K_Date: host.addDatePicker(id, x, y, w, h); break;
        case K_Calendar: host.addCalendar(id, x, y, w, h); break;
        case K_StaticBitmap: host.addStaticBitmap(id, x, y, w, h); break;
        case K_Panel: host.addPanel(id, x, y, w, h); break;
        case K_Splitter: host.addSplitter(id, x, y, w, h); break;
        case K_Line: host.addStaticLine(id, x, y, w, h); break;
        case K_Scrolled: host.addScrolledWindow(id, x, y, w, h); break;
        default: break;
        }
    }

    const int kFirstControlId = 1000;

    void walk(ViewerMfcHost& host, std::vector<ViewerLeafBinding>& out, XmlElement* el,
              int ox, int oy, COLORREF parentBack)
    {
        if (el == nullptr) return;

        RECT rc = {0, 0, 0, 0};
        rc.left = ox + el->previewX;
        rc.top = oy + el->previewY;
        rc.right = rc.left + el->previewW;
        rc.bottom = rc.top + el->previewH;
        if (rc.right <= rc.left || rc.bottom <= rc.top) return;

        const xmlapp::XmlAppearance ap = appearanceOf(*el);
        const bool styled = !ap.empty();

        if (viewerIsContainerTag(el->tag))
        {
            COLORREF back = parentBack;
            COLORREF c = 0;
            if (!ap.backColor.empty() && xmlapp::ParseColor(ap.backColor, c))
                back = c;
            for (auto& child : el->children)
                walk(host, out, &child, ox, oy, back);
            return;
        }

        const LeafKind kind = kindOf(el->tag);
        if (kind == K_None)
            return;   // 未知标签：不建控件，仍由父窗口按 XML 自绘

        const int id = kFirstControlId + static_cast<int>(out.size());
        createLeaf(host, *el, kind, id, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top);

        // 通用基础属性：与运行时（XmlMfcControlBase）同一套解析口径
        // Level 越大越靠上；TabIndex 只影响 Tab 遍历；Theme 先存着等颜色规则定
        const int level = intAttrOf(*el, L"Level", intAttrOf(*el, L"ZIndex", 0));
        host.setControlLevel(id, level);
        const std::wstring tab = attrOf(*el, L"TabIndex");
        if (!tab.empty()) host.setControlTabIndex(id, _wtoi(tab.c_str()));
        const std::wstring theme = attrOf(*el, L"Theme");
        if (!theme.empty()) host.setControlTheme(id, theme);

        ViewerLeafBinding b;
        b.id = id;
        b.element = el;
        b.rect = rc;
        b.ap = ap;
        b.styled = styled;
        b.parentBack = parentBack;
        out.push_back(b);
    }
} // namespace

bool viewerIsContainerTag(const std::wstring& tag)
{
    // 与运行时（PyMfcCore）共用同一张容器表：以前这里自己抄了一份，
    // 结果 Layout 被当成未知标签、预览器弹"文档存在错误"。
    return xmlmfc::isContainerTag(tag);
}

std::vector<ViewerLeafBinding> viewerControlsBuild(ViewerMfcHost& host, XmlElement* root,
                                                   int originX, int originY)
{
    std::vector<ViewerLeafBinding> out;
    if (root == nullptr)
        return out;
    walk(host, out, root, originX, originY, RGB(250, 250, 250));
    return out;
}

void viewerControlsDestroy(ViewerMfcHost& host)
{
    host.destroyAll();
}
