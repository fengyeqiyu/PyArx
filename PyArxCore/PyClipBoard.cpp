#include "stdafx.h"
#include "PyClipBoard.h"

// 构造函数：打开剪贴板
PyClipBoard::PyClipBoard()
    : m_bOpen(OpenClipboard(NULL))
{
}

// 析构函数：关闭剪贴板
PyClipBoard::~PyClipBoard()
{
    close();   // 复用 close()，内部会判断是否已关闭
}

// 剪贴板是否成功打开
bool PyClipBoard::isOpen() const
{
    return m_bOpen;
}

// 手动关闭剪贴板
bool PyClipBoard::close()
{
    if (!m_bOpen) return false;   // 已经关闭，直接返回

    BOOL ret = CloseClipboard();
    m_bOpen = false;              // 无论成功失败都标记为已关闭
    return ret;                   // BOOL 隐式转 bool
}

// 清空剪贴板
bool PyClipBoard::clear()
{
    if (!m_bOpen) return false;
    return EmptyClipboard();      // BOOL 隐式转 bool
}

// 判断剪贴板中是否有 Unicode 字符串
bool PyClipBoard::hasText() const
{
    if (!m_bOpen) return false;
    return IsClipboardFormatAvailable(CF_UNICODETEXT);   // BOOL 隐式转 bool
}

// 读取字符串
bool PyClipBoard::getText(AcString& data)
{
    data.setEmpty();

    if (!m_bOpen) return false;
    if (!IsClipboardFormatAvailable(CF_UNICODETEXT)) return false;

    HANDLE hData = GetClipboardData(CF_UNICODETEXT);
    if (hData == NULL) return false;

    wchar_t* pData = (wchar_t*)GlobalLock(hData);
    if (pData == NULL) return false;

    data.assign(pData);

    GlobalUnlock(hData);
    return true;
}

// 写字符串
bool PyClipBoard::setText(const AcString& data)
{
    if (!m_bOpen) return false;

    // 清空剪贴板
    if (!EmptyClipboard()) return false;

    // 计算内存大小（含结束符）
    size_t size = (data.length() + 1) * sizeof(wchar_t);

    HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, size);
    if (hMem == NULL) return false;

    wchar_t* pMem = (wchar_t*)GlobalLock(hMem);
    if (pMem == NULL) {
        GlobalFree(hMem);
        return false;
    }

    memcpy(pMem, data.kwszPtr(), size);

    GlobalUnlock(hMem);

    if (!SetClipboardData(CF_UNICODETEXT, hMem)) {
        GlobalFree(hMem);
        return false;
    }

    // 成功后 hMem 归剪贴板所有，不要 GlobalFree
    return true;
}