#pragma once
#include "stdafx.h"

class PyClipBoard {
public:
    // 构造函数：打开剪贴板
    PyClipBoard();

    // 析构函数：关闭剪贴板
    ~PyClipBoard();

    // 禁止拷贝，避免重复关闭剪贴板
    PyClipBoard(const PyClipBoard&) = delete;
    PyClipBoard& operator=(const PyClipBoard&) = delete;

    // 剪贴板是否成功打开
    bool isOpen() const;

    // 手动关闭剪贴板（可提前释放，析构时会自动判断）
    bool close();

    // 清空剪贴板
    bool clear();

    // 判断剪贴板中是否有 Unicode 字符串
    bool hasText() const;

    // 读取字符串
    bool getText(AcString& data);

    // 写字符串
    bool setText(const AcString& data);

private:
    bool m_bOpen;   // 剪贴板是否成功打开
};