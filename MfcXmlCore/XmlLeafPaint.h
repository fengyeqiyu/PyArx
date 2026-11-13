#pragma once
// XmlLeafPaint.h — 叶子控件"怎么画"的唯一实现。
//
// 之前的事实：预览器（Viewer/XmlViewer.cpp::drawLeaf）和设计器画布
// （UIDesign/DesignCanvas.h::drawNode）各写了一套叶子绘制代码，于是同一个 Button
// 在两处圆角/边框/配色都不一样 —— 用户看到的就是"预览器与设计器显示不一样"。
//
// 现在只有这一份：按控件类型分派，配色取 XmlTheme.h，几何取 XmlDraw / XmlControls。
// 设计器画布、预览器调用同一个函数，观感不会再分叉。
//
// 用法：
//   xmlleaf::drawLeaf(cv, el, rc, appearance, styled, parentBack, 图标根目录);

#include <windows.h>
#include <string>
#include <initializer_list>

#include "XmlCore.h"
#include "controls/XmlMfcTags.h"
#include "XmlAppearance.h"
#include "XmlDraw.h"
#include "XmlControls.h"
#include "XmlTheme.h"
#include "PyMfcSkin.h"   // 运行时那套自绘（开关/数值/表盘/滚筒等直接复用）

namespace xmlleaf
{

inline bool isAnyTag(const std::wstring& tag, std::initializer_list<const wchar_t*> names)
{
    for (const wchar_t* n : names)
        if (_wcsicmp(tag.c_str(), n) == 0) return true;
    return false;
}

inline bool truthy(const std::wstring& v)
{
    return _wcsicmp(v.c_str(), L"true") == 0 || _wcsicmp(v.c_str(), L"yes") == 0 || v == L"1";
}

// 图标写法是相对 exe 目录的路径；绝对路径原样用
inline std::wstring resolveIconPath(const std::wstring& base, const std::wstring& p)
{
    if (p.empty()) return std::wstring();
    if (p.size() > 2 && p[1] == L':') return p;
    if (!base.empty() && p[0] != L'\\') return base + L"\\" + p;
    return p;
}

// 浅色主题喂给 pymfcskin 那套自绘：几何沿用运行时，配色跟随 XmlTheme
inline pymfcskin::Palette lightPalette()
{
    pymfcskin::Palette p;
    p.back   = xmlmfc::theme::field;
    p.fore   = xmlmfc::theme::ink;
    p.border = xmlmfc::theme::border;
    p.accent = xmlmfc::theme::accent;
    p.muted  = xmlmfc::theme::track;
    p.ptSize = 9;
    p.face   = L"Microsoft YaHei UI";
    return p;
}

inline void drawLeaf(XmlCanvas& cv, XmlElement* el, const RECT& rc,
                     const xmlapp::XmlAppearance& ap, bool styled, COLORREF parentBack,
                     const std::wstring& urlBase)
{
    // 标签可能是 XButton/XTextField 这种 X+QML 写法：统一取本意名再分派
    const std::wstring tag = xmlmfc::BareTag(el->tag);

    // 设计器外壳叶子：文档标签 / 分类胶囊
    if (isAnyTag(tag, {L"Tab"}))
    {
        xmlctl::xmlDrawTab(cv, rc, el->text, truthy(el->get(L"Active")), false, 9);
        return;
    }
    if (isAnyTag(tag, {L"Chip"}))
    {
        xmlctl::xmlDrawChip(cv, rc, el->text, truthy(el->get(L"Checked")), false, 9);
        return;
    }

    // 按钮：运行时是 owner-draw —— RoundRect(按钮面, 边框, 8) + 文字居中
    // 有 Icon 时：左侧方形图标 + 文字右移；只有图标时图标居中
    if (isAnyTag(tag, {L"Button", L"PushButton", L"IconButton", L"ToggleButton"}))
    {
        // 设计器外壳的两种按钮角色：Primary/Ghost 画胶囊，IconTop 画"图标 + 标题"。
        // 预览器与设计器必须同形，否则外壳一进预览就散架。
        const std::wstring role = el->get(L"Role");
        const std::wstring iconProbe = resolveIconPath(urlBase, el->icon);
        if (_wcsicmp(role.c_str(), L"Primary") == 0 || _wcsicmp(role.c_str(), L"Ghost") == 0)
        {
            xmlctl::xmlDrawPillButton(cv, rc, el->text,
                                        _wcsicmp(role.c_str(), L"Primary") == 0, false, 9);
            return;
        }
        if (!iconProbe.empty() && !el->text.empty() && (rc.bottom - rc.top) >= 36)
        {
            xmlctl::xmlDrawToolButtonCaption(cv, rc, iconProbe, el->text, false, false, false,
                                               xmlctl::ToolButtonStyle(), 8);
            return;
        }

        if (styled)
            xmlDrawBackplate(cv, rc, ap, xmlmfc::theme::button, xmlmfc::theme::border);
        else
        {
            cv.fillRect(rc, parentBack);
            cv.fillRoundRect(rc, xmlmfc::theme::button, xmlmfc::theme::border, xmlmfc::theme::radius);
        }
        const std::wstring iconPath = resolveIconPath(urlBase, el->icon);
        if (!iconPath.empty() && rc.bottom > rc.top)
        {
            const int side = (rc.bottom - rc.top) - 8;
            if (side > 0)
            {
                RECT ir = { rc.left + 4, rc.top + 4, rc.left + 4 + side, rc.top + 4 + side };
                if (el->text.empty())
                {
                    ir.left = rc.left + ((rc.right - rc.left) - side) / 2;
                    ir.right = ir.left + side;
                }
                if (ir.right <= rc.right)
                {
                    cv.drawImage(ir, iconPath);
                    if (!el->text.empty())
                    {
                        RECT tr = rc;
                        tr.left = ir.right + 4;
                        xmlDrawText(cv, tr, el->text, ap, styled, xmlmfc::theme::ink, DT_LEFT, 0);
                    }
                    return;
                }
            }
        }
        xmlDrawText(cv, rc, el->text, ap, styled, xmlmfc::theme::ink, DT_CENTER, 0);
        return;
    }

    // 图片：有 Source 就按比例画出来，没有就画一个统一的占位框（画布与预览器完全一致）
    if (isAnyTag(tag, {L"Image"}))
    {
        const std::wstring src = el->get(L"Source").empty() ? el->get(L"Icon") : el->get(L"Source");
        const std::wstring imgPath = resolveIconPath(urlBase, src);
        if (!imgPath.empty() && rc.right > rc.left && rc.bottom > rc.top)
        {
            cv.drawImage(rc, imgPath);
            return;
        }
        cv.strokeRect(rc, xmlmfc::theme::border, 1);
        xmlDrawText(cv, rc, L"XImage", ap, styled, xmlmfc::theme::muted, DT_CENTER, 0);
        return;
    }

    // 静态文本：运行时 Static 不铺底，只有文字
    if (isAnyTag(tag, {L"Label", L"Static", L"LinkLabel"}))
    {
        if (styled && (!ap.backColor.empty() || !ap.frontColor.empty() || !ap.borderColor.empty()))
            xmlDrawBackplate(cv, rc, ap, parentBack, xmlmfc::theme::border);
        xmlDrawText(cv, rc, el->text, ap, styled, xmlmfc::theme::ink, DT_LEFT, 6);
        return;
    }

    // 输入框：窗口色 + 2px 凹陷 3D 边框（等价 WS_EX_CLIENTEDGE）
    if (isAnyTag(tag, {L"TextField", L"Edit", L"MaskedTextBox"}))
    {
        const COLORREF win = xmlmfc::theme::field;
        if (styled)
            xmlDrawBackplate(cv, rc, ap, win, xmlmfc::theme::border);
        else
            cv.fillRoundRect(rc, xmlmfc::theme::field, xmlmfc::theme::border, xmlmfc::theme::radius);
        xmlDrawText(cv, rc, el->text, ap, styled, xmlmfc::theme::ink, DT_LEFT, 4);
        return;
    }

    // 复选框：13×13 凹陷方框 + 勾 + 右侧文字
    if (isAnyTag(tag, {L"CheckBox"}))
    {
        if (styled) xmlDrawBackplate(cv, rc, ap, parentBack, xmlmfc::theme::border);
        const int side = 13;
        RECT box = rc;
        box.left = rc.left + 2;
        box.top = rc.top + (rc.bottom - rc.top - side) / 2;
        box.right = box.left + side;
        box.bottom = box.top + side;
        cv.fillRoundRect(box, xmlmfc::theme::field, xmlmfc::theme::border, 2);
        if (truthy(el->get(L"Checked")) || truthy(el->get(L"IsChecked")))
        {
            cv.strokeLine({box.left + 3, box.top + 7}, {box.left + 5, box.top + 9}, xmlmfc::theme::ink, 2);
            cv.strokeLine({box.left + 5, box.top + 9}, {box.left + 10, box.top + 3}, xmlmfc::theme::ink, 2);
        }
        RECT tr = rc;
        tr.left += side + 8;
        xmlDrawText(cv, tr, el->text, ap, styled, xmlmfc::theme::ink, DT_LEFT, 0);
        return;
    }

    // 单选：12×12 圆框 + 实心圆点 + 右侧文字
    if (isAnyTag(tag, {L"RadioButton", L"RadioBox"}))
    {
        if (styled) xmlDrawBackplate(cv, rc, ap, parentBack, xmlmfc::theme::border);
        const int side = 12;
        RECT cir = rc;
        cir.left = rc.left + 2;
        cir.top = rc.top + (rc.bottom - rc.top - side) / 2;
        cir.right = cir.left + side;
        cir.bottom = cir.top + side;
        if (truthy(el->get(L"Checked")) || truthy(el->get(L"IsChecked")))
        {
            cv.fillEllipse(cir, xmlmfc::theme::field, xmlmfc::theme::border);
            RECT dot = cir;
            ::InflateRect(&dot, -3, -3);
            if (dot.right > dot.left && dot.bottom > dot.top)
                cv.fillEllipse(dot, xmlmfc::theme::ink, xmlmfc::theme::ink);
        }
        else
        {
            cv.fillEllipse(cir, xmlmfc::theme::field, xmlmfc::theme::border);
        }
        RECT tr = rc;
        tr.left += side + 8;
        xmlDrawText(cv, tr, el->text, ap, styled, xmlmfc::theme::ink, DT_LEFT, 0);
        return;
    }

    // 列表类：窗口色 + 凹陷边框 + 逐行条目
    // 与设计器画布 / 运行时共用同一实现（XmlControls::xmlDrawListLike），
    // 否则三端会各自漂移成"这里浅色、那里深色"。
    if (isAnyTag(tag, {L"TreeView"}))
    {
        xmlctl::xmlDrawTreeLike(cv, rc, *el, styled, ap, xmlmfc::theme::ink, xmlmfc::theme::border);
        return;
    }
    if (isAnyTag(tag, {L"PropertyGrid"}))
    {
        xmlctl::xmlDrawPropRows(cv, rc, *el, styled, ap, xmlmfc::theme::ink, xmlmfc::theme::border);
        return;
    }
    if (isAnyTag(tag, {L"ListBox", L"ListView"}))
    {
        xmlctl::xmlDrawListLike(cv, rc, *el, styled, ap, xmlmfc::theme::ink, xmlmfc::theme::border);
        return;
    }

    // 下拉框：窗口色 + 右侧按钮面 + 下箭头 + 文字（收起状态）
    if (isAnyTag(tag, {L"ComboBox"}))
    {
        const COLORREF win = xmlmfc::theme::field;
        const int arrowW = 18;
        if (styled)
            xmlDrawBackplate(cv, rc, ap, win, xmlmfc::theme::border);
        else
            cv.fillRoundRect(rc, xmlmfc::theme::field, xmlmfc::theme::border, xmlmfc::theme::radius);
        RECT btn = rc;
        ::InflateRect(&btn, -2, -2);
        btn.left = btn.right - arrowW;
        if (btn.right > btn.left)
        {
            cv.fillRect(btn, xmlmfc::theme::button);
            cv.strokeLine({btn.left, rc.top + 2}, {btn.left, rc.bottom - 3}, xmlmfc::theme::border, 1);
            const int mx = (btn.left + btn.right) / 2;
            const int my = (btn.top + btn.bottom) / 2;
            cv.strokeLine({mx - 5, my - 2}, {mx, my + 4}, xmlmfc::theme::ink, 1);
            cv.strokeLine({mx, my + 4}, {mx + 5, my - 2}, xmlmfc::theme::ink, 1);
        }
        RECT tr = rc;
        tr.right = btn.left - 2;
        xmlDrawText(cv, tr, el->text, ap, styled, xmlmfc::theme::ink, DT_LEFT, 4);
        return;
    }

    // 进度条：浅槽 + 绿色填充（Value 0-100，缺省 40）
    if (isAnyTag(tag, {L"ProgressBar"}))
    {
        cv.fillRoundRect(rc, xmlmfc::theme::track, xmlmfc::theme::border, xmlmfc::theme::radius);
        int pct = _wtoi(el->get(L"Value").c_str());
        if (pct <= 0) pct = 40;
        if (pct > 100) pct = 100;
        RECT fill = rc;
        ::InflateRect(&fill, -2, -2);
        if (fill.right > fill.left)
            fill.right = fill.left + ::MulDiv(fill.right - fill.left, pct, 100);
        if (fill.right > fill.left + 2)
            cv.fillRoundRect(fill, xmlmfc::theme::accent, xmlmfc::theme::accent, 2);
        return;
    }

    // 滑块：横向沟槽 + 居中圆角滑块
    if (isAnyTag(tag, {L"Slider", L"TrackBar"}))
    {
        RECT groove = rc;
        const int gh = 4;
        groove.top = rc.top + (rc.bottom - rc.top - gh) / 2;
        groove.bottom = groove.top + gh;
        groove.left += 8;
        groove.right -= 8;
        cv.fillRect(groove, xmlmfc::theme::track);
        cv.strokeRect(groove, xmlmfc::theme::border, 1);

        const int tw = 12, th = 20;
        RECT thumb = rc;
        thumb.left = (rc.left + rc.right) / 2 - tw / 2;
        thumb.right = thumb.left + tw;
        thumb.top = (rc.top + rc.bottom) / 2 - th / 2;
        thumb.bottom = thumb.top + th;
        cv.fillRoundRect(thumb, xmlmfc::theme::button, xmlmfc::theme::border, 4);
        return;
    }

    // 状态栏
    if (isAnyTag(tag, {L"StatusBar"}))
    {
        cv.fillRect(rc, xmlmfc::theme::button);
        xmlDrawText(cv, rc, el->text, ap, styled, xmlmfc::theme::muted, DT_LEFT, 6);
        return;
    }

    // ---- 绘图图元 / 分隔线 / 超链接 / 选择项 ----
    {
        COLORREF fill = xmlmfc::theme::surface;
        COLORREF stroke = xmlmfc::theme::border;
        int appAlpha = 255;
        COLORREF c = 0;
        if (xmlapp::ParseColor(ap.backColor, c)) fill = c;
        if (!xmlapp::ParseColor(ap.borderColor, c, &appAlpha) &&
            !xmlapp::ParseColor(ap.foreColor, c))
            c = xmlmfc::theme::border;
        stroke = c;

        if (isAnyTag(tag, {L"Rectangle", L"Ellipse", L"Line"}))
        {
            const int thick = 1;
            if (isAnyTag(tag, {L"Ellipse"}))
                cv.fillEllipse(rc, fill, stroke);
            else if (isAnyTag(tag, {L"Line"}))
            {
                const std::wstring dir = el->get(L"Direction");
                POINT a{ rc.left, rc.top }, b{ rc.right, rc.bottom };
                if (_wcsicmp(dir.c_str(), L"Up") == 0) { a.y = rc.bottom; b.y = rc.top; }
                else if (_wcsicmp(dir.c_str(), L"H") == 0 ||
                         _wcsicmp(dir.c_str(), L"Horizontal") == 0)
                    a.y = b.y = rc.top + (rc.bottom - rc.top) / 2;
                else if (_wcsicmp(dir.c_str(), L"V") == 0 ||
                         _wcsicmp(dir.c_str(), L"Vertical") == 0)
                    a.x = b.x = rc.left + (rc.right - rc.left) / 2;
                cv.strokeLine(a, b, stroke, thick);
            }
            else
                cv.fillRoundRect(rc, fill, stroke, 0);
            return;
        }
        if (isAnyTag(tag, {L"Divider"}))
        {
            // 方向、粗细、颜色全部来自 XML：Orientation / Thickness / Background / Foreground
            COLORREF ink = xmlmfc::theme::border;
            COLORREF c1;
            if (xmlapp::ParseColor(el->get(L"Background"), c1)) ink = c1;
            else if (xmlapp::ParseColor(el->get(L"Foreground"), c1)) ink = c1;
            int thick = _wtoi(el->get(L"Thickness").c_str());
            if (thick <= 0) thick = 1;
            const std::wstring dir = el->get(L"Orientation");
            const bool vertical = _wcsicmp(dir.c_str(), L"Vertical") == 0 ||
                                  _wcsicmp(dir.c_str(), L"V") == 0 ||
                                  (dir.empty() && (rc.right - rc.left) <= 4);
            RECT line = rc;
            if (vertical)
            {
                line.left = (rc.left + rc.right) / 2;
                line.right = line.left + thick;
            }
            else
            {
                line.top = (rc.top + rc.bottom) / 2;
                line.bottom = line.top + thick;
            }
            cv.fillRect(line, ink);
            return;
        }
        if (isAnyTag(tag, {L"Hyperlink"}))
        {
            xmlDrawText(cv, rc, el->text, ap, styled, xmlmfc::theme::accent, DT_LEFT, 2);
            return;
        }
        if (isAnyTag(tag, {L"Choice"}))
        {
            cv.fillRoundRect(rc, xmlmfc::theme::field, xmlmfc::theme::border, xmlmfc::theme::radius);
            RECT txt = rc; txt.left += 8; txt.right -= 24;
            xmlDrawText(cv, txt, el->text.empty() ? el->get(L"Value") : el->text, ap, styled,
                         xmlmfc::theme::ink, DT_LEFT, 0);
            // 展开箭头（自绘一个小 V，不依赖按钮标题绘制）
            const int cx = rc.right - 14, cy = (rc.top + rc.bottom) / 2;
            cv.strokeLine(POINT{ cx - 4, cy - 3 }, POINT{ cx, cy + 2 }, xmlmfc::theme::muted, 1);
            cv.strokeLine(POINT{ cx, cy + 2 }, POINT{ cx + 4, cy - 3 }, xmlmfc::theme::muted, 1);
            return;
        }
    }

    // ---- 开关 / 数值 / 滑块组 / 日期 / 表盘 / 滚筒 / 滚动条 / 等待环 ----
    // 直接复用运行时那套 pymfcskin 自绘（几何一致），配色换成主题浅色，
    // 于是设计器画布与预览器不会再各画一版。
    {
        const pymfcskin::Palette pal = lightPalette();
        const int v = _wtoi(el->get(L"Value").c_str());
        if (isAnyTag(tag, {L"Switch"}))
        {
            const bool on = truthy(el->get(L"Checked")) || truthy(el->get(L"IsChecked")) ||
                            truthy(el->get(L"On")) || truthy(el->get(L"Value"));
            pymfcskin::drawSwitch(cv, rc, on, pal, false);
            return;
        }
        if (isAnyTag(tag, {L"SpinBox"}))
        {
            pymfcskin::drawSpinBox(cv, rc, v, pal, false, false);
            return;
        }
        if (isAnyTag(tag, {L"RangeSlider"}))
        {
            const int lo = _wtoi(el->get(L"LowerValue").c_str());
            const int hi = _wtoi(el->get(L"UpperValue").c_str());
            pymfcskin::drawRangeSlider(cv, rc, lo, hi, 0, 100, pal);
            return;
        }
        if (isAnyTag(tag, {L"DatePicker"}))
        {
            pymfcskin::drawDatePicker(cv, rc, el->text.empty() ? el->get(L"Value") : el->text, pal);
            return;
        }
        if (isAnyTag(tag, {L"Dial"}))
        {
            pymfcskin::drawDial(cv, rc, v, 0, 100, pal, false);
            return;
        }
        if (isAnyTag(tag, {L"Tumbler"}))
        {
            int count = _wtoi(el->get(L"Count").c_str());
            if (count <= 0) count = 12;
            pymfcskin::drawTumbler(cv, rc, v, count, pal, false);
            return;
        }
        if (isAnyTag(tag, {L"ScrollBar"}))
        {
            const bool vertical = rc.bottom - rc.top >= rc.right - rc.left;
            pymfcskin::drawScrollBar(cv, rc, v, 0, 100, 20, vertical, pal, false);
            return;
        }
        if (isAnyTag(tag, {L"BusyIndicator"}))
        {
            pymfcskin::drawBusyRing(cv, rc, 0, pal);
            return;
        }
    }

    // 其它：有文字就按文字画，避免出现调试标注
    if (!el->text.empty())
        xmlDrawText(cv, rc, el->text, ap, styled, xmlmfc::theme::ink, DT_LEFT, 6);
}

// 容器：背景/边框 + 返回给子节点继承的底色

} // namespace xmlleaf
