#pragma once
// XmlTheme.h — 一套配色，三处共用（设计器画布 / 预览器 / 运行时自绘）。
//
// 之前的实际状况：设计器画布用 DesignCanvas.h 里的一套浅色（surface 250 / border 173），
// 预览器用 XmlViewer.cpp 里另一套（btnFace 240,242,245 / 3D 立体边框），
// 于是"预览器与设计器显示不一样"成了常态。配色与圆角收到这里之后，
// 改主题只改这一个文件，三端必然一致。
//
// 值取自设计器画布那套（浅色、扁平，贴近 QML 默认控件外观）。

#include <windows.h>

namespace xmlmfc
{
namespace theme
{
// ---- 面 ----
const COLORREF surface  = RGB(250, 250, 250);   // 页面 / 容器底
const COLORREF button   = RGB(240, 240, 240);   // 按钮、下拉按钮面
const COLORREF field    = RGB(255, 255, 255);   // 输入框、列表、表格
const COLORREF head     = RGB(238, 241, 245);   // 表头
const COLORREF track    = RGB(235, 237, 240);   // 轨道、空槽

// ---- 线与字 ----
const COLORREF border   = RGB(173, 178, 186);   // 控件边框
const COLORREF grid     = RGB(214, 218, 224);   // 表格分隔线
const COLORREF ink      = RGB(32, 36, 42);      // 正文
const COLORREF muted    = RGB(118, 124, 132);   // 次要文字、占位
const COLORREF accent   = RGB(0, 120, 212);     // 选中、进度、勾选

// ---- 形状 ----
const int radius      = 4;    // 控件圆角（QML 默认扁平观感）
const int borderWidth = 1;    // 描边宽度：扁平 1px，不再用 3D 立体框

} // namespace theme
} // namespace xmlmfc
