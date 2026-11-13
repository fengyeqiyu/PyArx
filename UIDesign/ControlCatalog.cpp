#include "stdafx.h"
#include "ControlCatalog.h"
#include "XamlCore.h"
#include <msxml6.h>
#pragma comment(lib, "msxml6.lib")

namespace uidesign
{
    static std::wstring Attr(IXMLDOMElement* el, const wchar_t* name)
    {
        BSTR b = nullptr;
        VARIANT v;
        VariantInit(&v);
        std::wstring out;
        if (el->getAttribute(_bstr_t(name), &v) == S_OK && v.vt == VT_BSTR)
        {
            out = v.bstrVal ? v.bstrVal : L"";
        }
        VariantClear(&v);
        (void)b;
        return out;
    }

    bool ControlCatalog::load(const CString& path)
    {
        m_templates.clear();
        if (::GetFileAttributesW(path) == INVALID_FILE_ATTRIBUTES) return false;

        ::CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
        HRESULT hr = S_OK;
        CComPtr<IXMLDOMDocument> doc;
        hr = ::CoCreateInstance(CLSID_DOMDocument60, nullptr, CLSCTX_INPROC_SERVER,
                                IID_IXMLDOMDocument, reinterpret_cast<void**>(&doc));
        if (FAILED(hr)) return false;
        VARIANT_BOOL ok = VARIANT_FALSE;
        hr = doc->load(_variant_t((LPCWSTR)path), &ok);
        if (FAILED(hr) || ok != VARIANT_TRUE) return false;

        CComPtr<IXMLDOMElement> rootEl;
        doc->get_documentElement(&rootEl);
        if (!rootEl) return false;

        CComPtr<IXMLDOMNodeList> objects;
        rootEl->getElementsByTagName(_bstr_t(L"objectinfo"), &objects);
        if (!objects) return false;

        long n = 0;
        objects->get_length(&n);
        for (long i = 0; i < n; i++)
        {
            CComPtr<IXMLDOMNode> node;
            objects->get_item(i, &node);
            if (!node) continue;
            CComPtr<IXMLDOMElement> el;
            node->QueryInterface(IID_IXMLDOMElement, reinterpret_cast<void**>(&el));
            if (!el) continue;

            CtrlTemplate t;
            t.tag = Attr(el, L"tag").c_str();
            t.display = Attr(el, L"display").c_str();
            t.category = Attr(el, L"category").c_str();
            t.dw = _wtoi(Attr(el, L"w").c_str());
            t.dh = _wtoi(Attr(el, L"h").c_str());
            if (t.dw <= 0) t.dw = 100;
            if (t.dh <= 0) t.dh = 28;
            if (t.tag.IsEmpty()) continue;

            // 控件缺省尺寸由 catalog 管控：注入布局引擎，C++ 不再硬编码 120/28
            xamlSetDefaultSize(t.tag, t.dw, t.dh);

            // 子 <property name type>
            CComPtr<IXMLDOMNodeList> props;
            el->getElementsByTagName(_bstr_t(L"property"), &props);
            if (props)
            {
                long pn = 0;
                props->get_length(&pn);
                for (long p = 0; p < pn; p++)
                {
                    CComPtr<IXMLDOMNode> pn2;
                    props->get_item(p, &pn2);
                    if (!pn2) continue;
                    CComPtr<IXMLDOMElement> pel;
                    pn2->QueryInterface(IID_IXMLDOMElement, reinterpret_cast<void**>(&pel));
                    if (!pel) continue;
                    CtrlProp cp;
                    cp.name = Attr(pel, L"name").c_str();
                    cp.type = Attr(pel, L"type").c_str();
                    cp.options = Attr(pel, L"options").c_str();
                    if (cp.type.IsEmpty()) cp.type = L"text";
                    if (!cp.name.IsEmpty()) t.props.push_back(cp);
                }
            }
            m_templates.push_back(t);
        }
        return !m_templates.empty();
    }

    std::vector<CString> ControlCatalog::categories() const
    {
        std::vector<CString> out;
        for (auto& t : m_templates)
        {
            bool found = false;
            for (auto& c : out) if (c == t.category) { found = true; break; }
            if (!found && !t.category.IsEmpty()) out.push_back(t.category);
        }
        return out;
    }

    std::vector<const CtrlTemplate*> ControlCatalog::templates(const CString& category) const
    {
        std::vector<const CtrlTemplate*> out;
        for (auto& t : m_templates)
            if (t.category == category) out.push_back(&t);
        return out;
    }

    const CtrlTemplate* ControlCatalog::byDisplay(const CString& display) const
    {
        for (auto& t : m_templates)
            if (t.display == display) return &t;
        return nullptr;
    }

    const CtrlTemplate* ControlCatalog::byTag(const CString& tag) const
    {
        for (auto& t : m_templates)
            if (t.tag == tag) return &t;
        return nullptr;
    }
}
