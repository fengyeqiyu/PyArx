# -*- coding: utf-8 -*-
"""重建 XamlMfcControlBase.cpp 前 70 行（被 repair 脚本误伤的部分）。

71 行往后（pushAppearance 起）是完好的，原样保留。
"""
import io

P = r'G:\PyArx\MfcXamlCore\controls\XamlMfcControlBase.cpp'

HEAD = r'''#include "XamlMfcControlBase.h"

#include <windows.h>

namespace
{
    // 属性值 -> int：解析失败时退回调用方给的默认值
    int propIntOf(const std::wstring& value, int fallback)
    {
        std::wstring text = value;
        const size_t b = text.find_first_not_of(L" \t\r\n");
        if (b == std::wstring::npos)
            return fallback;
        const size_t e = text.find_last_not_of(L" \t\r\n");
        text = (e == std::wstring::npos) ? text.substr(b) : text.substr(b, e - b + 1);
        if (text.empty())
            return fallback;
        try
        {
            return std::stoi(text);
        }
        catch (...)
        {
            return fallback;
        }
    }

    // XAML 事件属性名 -> 统一 kind（小写）
    // 全部来自 XamlEvents 的契约表，不在本文件另立一份，避免两处漂移。
    std::wstring EventKind(const std::wstring& name)
    {
        XamlEventKind k = XamlEventKind::Click;
        if (xamlEventFromProperty(name, k))
            return xamlEventKey(k);
        return {};
    }

    // kind -> 对外展示的事件属性名（用于 getProperty 回读）
    std::wstring EventProperty(const std::wstring& kind)
    {
        static const XamlEventKind kAll[] = {
            XamlEventKind::Click, XamlEventKind::DoubleClick, XamlEventKind::TextChanged,
            XamlEventKind::SelectionChanged, XamlEventKind::ValueChanged, XamlEventKind::CheckedChanged,
            XamlEventKind::DropDown, XamlEventKind::Scroll, XamlEventKind::MouseDown,
            XamlEventKind::MouseUp, XamlEventKind::MouseMove, XamlEventKind::KeyDown,
            XamlEventKind::KeyUp, XamlEventKind::GotFocus, XamlEventKind::LostFocus
        };
        for (XamlEventKind k : kAll)
            if (kind == xamlEventKey(k))
                return xamlEventProperty(k);
        return L"";
    }
}  // namespace

const wchar_t* XamlMfcControlBase::type() const
{
    return m_type.c_str();
}

XamlMfcControlBase::XamlMfcControlBase(std::wstring type)
{
    m_type = std::move(type);
}

void XamlMfcControlBase::attach(IXamlMfcHost* host, int id, int minValue, int maxValue,
                                int x, int y, int w, int h)
{
    m_host = host;
    m_id = id;
    m_min = minValue;
    m_max = maxValue;
    if (host == nullptr)
        return;
    createNative(host, id, minValue, maxValue, x, y, w, h);
    pushAppearance();
}
'''

with open(P, 'r', encoding='utf-8-sig') as f:
    lines = f.read().split('\n')

# 71 行（1-based）之前是坏区；从 "void XamlMfcControlBase::pushAppearance()" 起都是好的
idx = next(i for i, ln in enumerate(lines) if ln.startswith('void XamlMfcControlBase::pushAppearance'))
tail = '\n'.join(lines[idx:])
out = HEAD + '\n' + tail
with open(P, 'w', encoding='utf-8-sig', newline='\r\n') as f:
    f.write(out)
print('保留尾部行数: %d' % (len(lines) - idx))
print('新文件行数: %d' % len(out.split('\n')))
