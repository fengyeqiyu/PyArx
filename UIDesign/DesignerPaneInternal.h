#pragma once
#include "stdafx.h"
#include "DesignerPaneView.h"

// 设计器各编译单元共用的界面小工具。
// 原先它们是 DesignerPaneView.cpp 里的文件级 static，拆分后必须 inline 才能共享。
namespace uidesign::detail
{
    // exe 目录（图标资源相对它定位）
    inline const std::wstring& surfaceExeDir()
    {
        static std::wstring dir;
        static bool tried = false;
        if (!tried)
        {
            tried = true;
            wchar_t p[MAX_PATH] = {};
            ::GetModuleFileNameW(nullptr, p, MAX_PATH);
            std::wstring s = p;
            const size_t k = s.find_last_of(L'\\');
            dir = (k == std::wstring::npos) ? std::wstring() : s.substr(0, k);
        }
        return dir;
    }

    // 控件显示文字：优先 Text，其次节点文本
    inline CString uiNodeText(XmlElement* node)
    {
        std::wstring content = node->get(L"Text");
        if (content.empty()) content = node->get(L"Content");
        if (content.empty()) content = node->text;
        return CString(content.c_str());
    }

    inline CString uiFileName(const CString& path)
    {
        int p = path.ReverseFind(L'\\');
        if (p < 0) p = path.ReverseFind(L'/');
        return (p >= 0) ? path.Mid(p + 1) : path;
    }
}
