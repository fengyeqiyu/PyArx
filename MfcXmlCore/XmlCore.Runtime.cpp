// XmlCore.Runtime.cpp —— 拆自 XmlCore.cpp 第 5967-6339 行。
#include "XmlCoreCommon.h"
#include "XmlCoreInternal.h"

// ===========================================================================
// XmlRuntime
// ===========================================================================
XmlRuntime::~XmlRuntime()
{
    m_controls.clear();
    m_root.reset();
}

bool XmlRuntime::loadFile(const wchar_t* path, std::wstring& error)
{
    ::CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    IXMLDOMDocument2* document = nullptr;
    HRESULT hr = ::CoCreateInstance(__uuidof(DOMDocument60), nullptr, CLSCTX_INPROC_SERVER,
                                    IID_IXMLDOMDocument2, reinterpret_cast<void**>(&document));
    if (FAILED(hr) || document == nullptr)
    {
        error = L"无法创建 MSXML 文档对象";
        return false;
    }
    document->put_async(VARIANT_FALSE);
    document->put_validateOnParse(VARIANT_FALSE);
    VARIANT_BOOL ok = VARIANT_FALSE;
    document->load(_variant_t(path), &ok);
    if (ok != VARIANT_TRUE)
    {
        error = L"XML 解析失败（文件不存在或格式错误）";
        document->Release();
        return false;
    }
    IXMLDOMElement* rootElement = nullptr;
    document->get_documentElement(&rootElement);
    if (rootElement == nullptr)
    {
        error = L"XML 无根元素";
        document->Release();
        return false;
    }
    m_root = std::make_unique<XmlElement>();
    m_styles.clear();
    m_templates.clear();
    m_bindings.clear();
    // 资源字典 Source 的相对路径以本文件所在目录为基准；resStack 挡住循环引用
    const std::wstring baseDir = PathDirOf(NormPath(path));
    std::vector<std::wstring> resStack;
    resStack.push_back(NormPath(path));
    ParseElement(rootElement, *m_root, &m_styles, &m_templates, baseDir, &resStack);
    rootElement->Release();
    document->Release();
    htmlsugar::ApplyStyleFlexProps(*m_root);  // style 里的 flex 声明先落本地属性（糖化会重建节点）
    xmlApplyHtmlSugar(*m_root);   // HTML 词汇 → X 控件（XAML 写法原样不动）
    applyDeclarations();
    return true;
}

bool XmlRuntime::loadString(const wchar_t* xml, std::wstring& error)
{
    if (xml == nullptr)
    {
        error = L"XML 字符串为空";
        return false;
    }
    ::CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    IXMLDOMDocument2* document = nullptr;
    HRESULT hr = ::CoCreateInstance(__uuidof(DOMDocument60), nullptr, CLSCTX_INPROC_SERVER,
                                    IID_IXMLDOMDocument2, reinterpret_cast<void**>(&document));
    if (FAILED(hr) || document == nullptr)
    {
        error = L"无法创建 MSXML 文档对象";
        return false;
    }
    document->put_async(VARIANT_FALSE);
    document->put_validateOnParse(VARIANT_FALSE);
    BSTR bstrXml = ::SysAllocString(xml);
    VARIANT_BOOL ok = VARIANT_FALSE;
    document->loadXML(bstrXml, &ok);
    ::SysFreeString(bstrXml);
    if (ok != VARIANT_TRUE)
    {
        error = L"XML 解析失败";
        document->Release();
        return false;
    }
    IXMLDOMElement* rootElement = nullptr;
    document->get_documentElement(&rootElement);
    if (rootElement == nullptr)
    {
        error = L"XML 无根元素";
        document->Release();
        return false;
    }
    m_root = std::make_unique<XmlElement>();
    m_styles.clear();
    m_templates.clear();
    m_bindings.clear();
    ParseElement(rootElement, *m_root, &m_styles, &m_templates, std::wstring(), nullptr);
    rootElement->Release();
    document->Release();
    htmlsugar::ApplyStyleFlexProps(*m_root);  // style 里的 flex 声明先落本地属性（糖化会重建节点）
    xmlApplyHtmlSugar(*m_root);   // HTML 词汇 → X 控件（XAML 写法原样不动）
    applyDeclarations();
    return true;
}

void XmlRuntime::layout(int rootX, int rootY, int rootW, int rootH)
{
    if (!m_root)
        return;
    LayoutNodeAt(*m_root, rootX, rootY, rootW, rootH);
}

void XmlRuntime::setControlCallback(ControlCallback cb)
{
    m_onControl = std::move(cb);
}

    // 控件层 flex 容器的子项清单（实例化之后才有 control/id，所以只能在 build 末尾灌）。
    // 主轴尺寸取已经算好的 preview（含 grow 分配结果），交叉轴没声明就交 -1 让容器 stretch。
    static std::vector<XmlFlexItem> BuildPanelItems(XmlElement& node, bool row);

     bool AttachPanel(XmlElement& el)
    {
        if (el.control == nullptr)
            return false;
        auto* pnl = dynamic_cast<XmlFlexPanel*>(static_cast<XmlMfcControlBase*>(el.control));
        if (pnl == nullptr)
            return false;
        // 关键：容器自己的属性必须翻到**真实容器实例**上。
        // LayoutFlex 里那个 XmlFlexPanel 是栈上临时对象（只给 XmlCore 算坐标用），
        // 属性不灌到 el.control 的话，缩放时 reflowItems 用的全是默认值
        // （nowrap / gap=0 / padding=0 / justify=start / align=stretch），XML 写的排版会跳回默认。
        // <column> 天生竖排、<row> 天生横排（HTML 写法），显式 direction 属性仍可覆盖。
        if (KindOf(el.tag) == L"column")
            pnl->setDirection(XmlFlexDir::Column);
        for (const auto& kv : el.attrs)
            pnl->setOwnProperty(LocalName(kv.first).c_str(), kv.second);
        if (pnl->rowGap() <= 0 && pnl->columnGap() <= 0 && el.spacing > 0)
            pnl->setGap(el.spacing, el.spacing);
        const XmlFlexDir d = pnl->direction();
        const bool rowDir = (d == XmlFlexDir::Row || d == XmlFlexDir::RowReverse);
        std::vector<XmlFlexItem> its = BuildPanelItems(el, rowDir);
        pnl->clearItems();
        for (const XmlFlexItem& it : its)
            pnl->addItem(it);
        pnl->reflowItems();
        return true;
    }

    static std::vector<XmlFlexItem> BuildPanelItems(XmlElement& node, bool row)
    {
        std::vector<XmlFlexItem> out;
        out.reserve(node.children.size());
        for (auto& child : node.children)
        {
            if (child.templateChild)
                continue;
            if (ElHidden(child))
                continue;   // 隐藏项不参与容器重排，也不占主轴空间
            XmlFlexItem it;
            if (child.control != nullptr)
                it.id = static_cast<XmlMfcControlBase*>(child.control)->Id();
            const int mainPx = row ? child.previewW : child.previewH;
            const int crossPx = row ? child.previewH : child.previewW;
            it.naturalW = child.previewW;
            it.naturalH = child.previewH;
            it.basis = child.flexBasis >= 0 ? child.flexBasis : (std::max)(1, mainPx);
            it.basisPct = child.flexBasisPct >= 0.0f ? child.flexBasisPct
                                                   : (row ? child.pctW : child.pctH);
            it.crossPct = row ? child.pctH : child.pctW;
            const bool hasCross = row ? child.hasH : child.hasW;
            it.crossSize = hasCross ? (std::max)(1, crossPx) : -1;
            it.grow = child.flexGrow;
            it.shrink = child.flexShrink;
            it.selfAlign = child.flexSelf;
            it.marginL = child.marginL;
            it.marginT = child.marginT;
            it.marginR = child.marginR;
            it.marginB = child.marginB;
            it.marginLAuto = child.marginLAuto;
            it.marginTAuto = child.marginTAuto;
            it.marginRAuto = child.marginRAuto;
            it.marginBAuto = child.marginBAuto;
            // CSS 尺寸上下限（真实容器重排时同样要遵守，否则缩放会把卡片压穿 min-width）
            auto lim = [&](const wchar_t* a, const wchar_t* b) -> int {
                const std::wstring v1 = child.get(b);
                const std::wstring v2 = child.get(a);
                const std::wstring v = !v1.empty() ? v1 : v2;
                return v.empty() ? -1 : (int)wcstol(v.c_str(), nullptr, 10);
            };
            it.minW = lim(L"min-width", L"MinWidth");
            it.maxW = lim(L"max-width", L"MaxWidth");
            it.minH = lim(L"min-height", L"MinHeight");
            it.maxH = lim(L"max-height", L"MaxHeight");
            // align-items:baseline（真实容器重排时也要有基线信息，否则一律顶边对齐）
            if (row)
            {
                const std::wstring k2 = KindOf(child.tag);
                const bool texty = k2 == L"label" || k2 == L"span" || k2 == L"p" ||
                                   k2 == L"button" || k2 == L"edit" || k2 == L"text";
                if (texty)
                    it.baseline = BaselineFromTop(child, (std::max)(1, crossPx));
            }
            it.absolute = child.hasX;
            out.push_back(it);
        }
        return out;
    }

IXmlControl* XmlRuntime::instantiate(void* parent)
{
    if (!m_root)
        return nullptr;
    m_controls.clear();

    // 递归实例化
    std::function<IXmlControl*(XmlElement&)> build = [&](XmlElement& el) -> IXmlControl* {
        if (XmlControlRegistry::instance().isKnown(el.tag.c_str()) == false)
        {
            // 未注册标签：视为容器或布局元素，不创建控件，但继续处理子树
            for (auto& child : el.children)
            {
                if (child.templateChild)
                    continue;
                build(child);
            }
            return nullptr;
        }
        IXmlControl* ctrl = XmlControlRegistry::instance().create(el.tag.c_str());
        if (ctrl == nullptr)
            return nullptr;
        m_controls.emplace_back(ctrl);
        m_ctrlByElement[&el] = ctrl;
        el.control = ctrl;   // 布局阶段要用：flex 容器自己排版时靠它拿到控件
        if (ctrl->create(parent))
        {
            ctrl->setBounds(el.previewX, el.previewY, el.previewW, el.previewH);
            // 属性：按声明顺序写入（Name/事件名由宿主在回调里处理）
            for (const auto& kv : el.attrs)
            {
                const std::wstring key = LocalName(kv.first);
                const bool ok = ctrl->setProperty(key.c_str(), kv.second);
                // Title 是对外统一的"标题"写法：控件没有独立 Title 时按 Text 落
                if (!ok && _wcsicmp(key.c_str(), L"Title") == 0)
                    ctrl->setProperty(L"Text", kv.second);
            }
        }
        if (m_onControl)
            m_onControl(el, ctrl);
        // 子控件（模板实体化出来的视觉层由控件自绘，不进运行时控件树）
        for (auto& child : el.children)
        {
            if (child.templateChild)
                continue;
            build(child);
        }
        // 子控件都建好了，把子项清单交给 flex 容器本体：容器从此自己持有子项，
        // 后续 WM_SIZE / setBounds 由容器自己重排，不再依赖宿主整体 reflowXml。
        if (auto* pnl = dynamic_cast<XmlFlexPanel*>(ctrl))
        {
            AttachPanel(el);
        }
        else if (GetEnvironmentVariableW(L"PYARX_FLEX_DBG", nullptr, 0) != 0)
        {
            wchar_t tmp[MAX_PATH] = {0};
            GetTempPathW(MAX_PATH, tmp);
            FILE* f = _wfopen((std::wstring(tmp) + L"flexpanel.log").c_str(), L"a");
            if (f != nullptr)
            {
                fwprintf(f, L"nocast tag=%s type=%s kids=%d\n", el.tag.c_str(), ctrl->type(),
                         (int)el.children.size());
                fclose(f);
            }
        }
        return ctrl;
    };

    return build(*m_root);
}

XmlElement* XmlRuntime::root() { return m_root.get(); }
const XmlElement* XmlRuntime::root() const { return m_root.get(); }

std::wstring XmlRuntime::toXml() const
{
    std::wstring out;
    if (!m_root) return out;
    out += L"<?xml version=\"1.0\" encoding=\"utf-8\"?>\n";

    std::function<void(const XmlElement&, int)> write = [&](const XmlElement& n, int depth) {
        std::wstring indent((size_t)depth * 2, L' ');
        out += indent + L"<" + n.tag;
        if (n.attrs.size() <= 3)
        {
            for (const auto& kv : n.attrs)
                out += L" " + kv.first + L"=\"" + XmlEscape(kv.second) + L"\"";
        }
        else
        {
            // 属性多时一个属性一行，避免出现超长的单行
            const std::wstring ai((size_t)(depth + 1) * 2, L' ');
            for (const auto& kv : n.attrs)
                out += L"\n" + ai + kv.first + L"=\"" + XmlEscape(kv.second) + L"\"";
        }
        if (n.children.empty())
        {
            out += L"/>\n";
            return;
        }
        out += L">\n";
        if (!n.gridRows.empty())
        {
            out += indent + L"  <Grid.RowDefinitions>\n";
            for (const auto& kv : n.gridRows)
                out += indent + L"    <RowDefinition Height=\"" + XmlEscape(kv.second) + L"\"/>\n";
            out += indent + L"  </Grid.RowDefinitions>\n";
        }
        if (!n.gridCols.empty())
        {
            out += indent + L"  <Grid.ColumnDefinitions>\n";
            for (const auto& kv : n.gridCols)
                out += indent + L"    <ColumnDefinition Width=\"" + XmlEscape(kv.second) + L"\"/>\n";
            out += indent + L"  </Grid.ColumnDefinitions>\n";
        }
        for (const auto& c : n.children)
        {
            if (c.templateChild)
                continue;   // 模板实例化出来的视觉层不写回文档
            write(c, depth + 1);
        }
        out += indent + L"</" + n.tag + L">\n";
    };
    write(*m_root, 0);
    return out;
}

bool XmlRuntime::save(const wchar_t* path, std::wstring& error) const
{
    if (!m_root || path == nullptr)
    {
        error = L"文档为空或路径无效";
        return false;
    }
    std::wofstream out(path, std::ios::binary);
    if (!out)
    {
        error = L"无法写入文件";
        return false;
    }
    out.imbue(std::locale(".UTF-8"));
    // 与 toXml 共用同一套序列化：磁盘文件和代码视图格式一致
    std::wstring xml = toXml();
    std::wstring crlf;
    crlf.reserve(xml.size() + 64);
    for (wchar_t c : xml)
    {
        if (c == L'\n')
        {
            crlf += L'\r';
            crlf += L'\n';
        }
        else
            crlf += c;
    }
    out << crlf;
    return true;
}

void XmlRuntime::reset()
{
    m_root.reset();
    m_controls.clear();
}
