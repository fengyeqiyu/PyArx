#pragma once
// ViewerControls —— 预览器"叶子控件 → 原生控件"实体化层。
//
// 职责单一：遍历文档树，把每个叶子节点建成真控件（MFC 宿主创建），
// 并回传"控件 id → 元素 / 矩形 / 外观"的绑定表，供绘制回调与事件使用。
// 绘制、窗口过程、消息循环都不在这里。
//
// 后端（控件能力）：PyMfc 控件契约 + MFC 控件类；
// 前端（界面声明）：XML 文档。本层只是把两者接起来，不发明任何样式。
//
// 编码：本文件 UTF-8 with BOM。

#ifndef VIEWER_CONTROLS_H
#define VIEWER_CONTROLS_H

#include <winsock2.h>   // 必须先于 windows.h：MFC 的 afxwin.h 要求
#include <windows.h>
#include <string>
#include <vector>

#include "XmlCore.h"
#include "XmlAppearance.h"
#include "XmlViewerMfcHost.h"

// 一个已实体化的叶子控件
struct ViewerLeafBinding
{
    int id = 0;
    XmlElement* element = nullptr;
    RECT rect = {0, 0, 0, 0};
    xmlapp::XmlAppearance ap;
    bool styled = false;
    COLORREF parentBack = RGB(250, 250, 250);
};

// 控件的构造样式（创建期属性），从 XML 属性解析
struct ViewerLeafOptions
{
    bool multiline = false;
    bool readOnly = false;
    int maxLength = 0;
    std::wstring placeholder;
    bool dropDownList = true;      // ComboBox：DropDownList（只选）/ DropDown（可编辑）
    bool multipleSelect = false;   // ListBox：SelectionMode
};

// 建控件：递归整棵树，返回全部绑定
std::vector<ViewerLeafBinding> viewerControlsBuild(ViewerMfcHost& host, XmlElement* root,
                                                   int originX, int originY);

// 销毁全部（宿主负责）
void viewerControlsDestroy(ViewerMfcHost& host);

// 容器判定（与主文件 isContainerTag 同一语义，集中在这里，避免两处各写一份）
bool viewerIsContainerTag(const std::wstring& tag);

#endif // VIEWER_CONTROLS_H
