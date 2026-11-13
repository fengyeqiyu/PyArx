#pragma once
// CodeEditor —— 控件库的独立代码编辑器控件（完全自绘，不依赖任何宿主形状）。
//
// 为什么自己维护文本缓冲：
//   早先的做法是"内部塞一个多行 EDIT 当输入内核，只把它的 WM_PAINT 掏空改由 D2D 画"。
//   那套做法的死结在于"两把尺子"：EDIT 用 GDI 排版算光标/命中，我们却用 DWrite 画字，
//   于是点一下光标落在别的字上、方向键移动后光标又跳回去、拖选边缘永远差几个像素，
//   而且系统光标、系统 IME 组合窗都按 EDIT 的坐标摆位，只能一个个去打补丁。
//   所以这一版把文本缓冲、光标、选区、滚动、键盘、剪贴板、输入法全部收归本控件，
//   排版与绘制只有一套坐标，鼠标点哪儿光标就在哪儿。
//
// 结构：
//   * m_lines       —— 文本按行存放（不含换行符）
//   * m_caret       —— 光标 (行, 列)
//   * m_anchor      —— 选区锚点；与光标不同即表示有选区
//   * m_topLine     —— 纵向滚动（以行为单位）
//   * m_hScroll     —— 横向滚动（像素）
//   绘制、命中、光标、IME 组合串共用同一套 widthOf（DWrite 量宽）。
//
// 语法：xml / python，决定着色规则与智能提示的词表。
// 编码：本文件 UTF-8 with BOM。

#include "XmlCore.h"
#include "XmlD2D.h"

#include <windows.h>
#include <windowsx.h>
#include <imm.h>
#pragma comment(lib, "imm32.lib")

#include <algorithm>
#include <cstdio>
#include <functional>
#include <memory>
#include <string>
#include <vector>

class CodeEditor
{
public:
    enum class Syntax { Xml, Python };

    // ---- 布局常量 ----
    static constexpr int kGutterW = 48;     // 行号槽基准宽度（缩放后按位数自适应）
    static constexpr int kLineH = 17;       // 9pt 时的行高（缩放后用 m_lineH）
    static constexpr int kPadLeft = 6;

    // ---------------------------------------------------------------- 生命周期
    bool create(HWND parent, int id, int x, int y, int w, int h,
                const std::wstring& syntax = L"xml")
    {
        ensureClass();
        m_parent = parent;
        m_id = id;
        m_hwnd = ::CreateWindowExW(
            0, kClassName, L"",
            WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_CLIPSIBLINGS,
            x, y, w, h, parent, nullptr, ::GetModuleHandleW(nullptr), nullptr);
        if (m_hwnd == nullptr)
            return false;

        ::SetWindowLongPtrW(m_hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
        m_syntax = (_wcsicmp(syntax.c_str(), L"python") == 0) ? Syntax::Python : Syntax::Xml;
        m_lines.assign(1, std::wstring());
        m_caret = {0, 0};
        m_anchor = m_caret;
        applyFont();
        ::ShowWindow(m_hwnd, SW_SHOW);
        return true;
    }

    void destroy()
    {
        stopCaretTimer();
        closeTip();
        if (m_tipHwnd != nullptr)
        {
            ::DestroyWindow(m_tipHwnd);
            m_tipHwnd = nullptr;
        }
        if (m_hwnd != nullptr)
        {
            ::SetWindowLongPtrW(m_hwnd, GWLP_USERDATA, 0);
            ::DestroyWindow(m_hwnd);
            m_hwnd = nullptr;
        }
        if (m_font != nullptr)
        {
            ::DeleteObject(m_font);
            m_font = nullptr;
        }
        m_cv.reset();
    }

    HWND hwnd() const { return m_hwnd; }

    void setBounds(int x, int y, int w, int h)
    {
        if (m_hwnd == nullptr) return;
        ::SetWindowPos(m_hwnd, nullptr, x, y, w, h,
                       SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOCOPYBITS);
        ::InvalidateRect(m_hwnd, nullptr, FALSE);
    }

    // ---------------------------------------------------------------- 文本
    // 换行统一按 \n 拆分（\r 丢掉），外部给 LF 或 CRLF 都行。
    void setText(const std::wstring& s)
    {
        m_lines.clear();
        std::wstring cur;
        for (wchar_t c : s)
        {
            if (c == L'\r') continue;
            if (c == L'\n')
            {
                m_lines.push_back(cur);
                cur.clear();
            }
            else
            {
                cur += c;
            }
        }
        m_lines.push_back(cur);
        if (m_lines.empty()) m_lines.push_back(std::wstring());

        m_caret = {0, 0};
        m_anchor = m_caret;
        m_topLine = 0;
        m_hScroll = 0;
        m_undo.clear();
        if (m_hwnd != nullptr)
            ::InvalidateRect(m_hwnd, nullptr, FALSE);
    }

    // 对外统一给 CRLF：调用方（设计器）拿它和文档原文比对是否被改过。
    std::wstring text() const
    {
        std::wstring out;
        for (size_t i = 0; i < m_lines.size(); ++i)
        {
            if (i != 0) out += L"\r\n";
            out += m_lines[i];
        }
        return out;
    }

    void setSyntax(const std::wstring& syntax)
    {
        const Syntax s = (_wcsicmp(syntax.c_str(), L"python") == 0) ? Syntax::Python : Syntax::Xml;
        if (s == m_syntax) return;
        m_syntax = s;
        closeTip();
        if (m_hwnd != nullptr)
            ::InvalidateRect(m_hwnd, nullptr, FALSE);
    }

    std::wstring syntax() const { return m_syntax == Syntax::Python ? L"python" : L"xml"; }

    void setReadOnly(bool ro)
    {
        m_readOnly = ro;
        if (m_hwnd != nullptr)
            ::InvalidateRect(m_hwnd, nullptr, FALSE);
    }
    bool readOnly() const { return m_readOnly; }

    void setOnTextChanged(std::function<void(const std::wstring&)> cb) { m_onChange = std::move(cb); }
    void setOnBlur(std::function<void()> cb) { m_onBlur = std::move(cb); }
    void setOnFocus(std::function<void()> cb) { m_onFocus = std::move(cb); }
    void focusEditor() { if (m_hwnd != nullptr) ::SetFocus(m_hwnd); }

    // 由宿主转发进来的键盘消息：
    // MFC 的 CFrameWnd 预翻译会把焦点窗口的按键先吃掉（回车/Tab 走对话框导航，
    // Ctrl+A/C/V 之类去查框架加速键），消息到不了本控件，表现就是"换行换不了、Ctrl+C 不灵"。
    // 所以视图在 PreTranslateMessage 里把落在这个窗口上的 WM_KEYDOWN 整条转过来，
    // 由本控件独占处理（含字符兜底）。
    void handleKeyMessage(WPARAM vk, LPARAM lp)
    {
        if (m_hwnd == nullptr) return;
        m_caretOn = true;
        stopCaretTimer();
        startCaretTimer();
        if (onKeyDown(vk))
        {
            ensureCaretVisible();
            ::InvalidateRect(m_hwnd, nullptr, FALSE);
            return;
        }
        typeCharFromKey(vk, lp);
    }

    // 字号（磅）。Ctrl+滚轮 1pt 一档；字体、行高、行号槽宽度一起改。
    void setZoom(int pt)
    {
        const int next = (std::max)(6, (std::min)(28, pt));
        if (next == m_pt && m_font != nullptr) return;
        m_pt = next;
        applyFont();
    }
    int zoom() const { return m_pt; }

    // 只滚动视口、不动光标：设计器"点中控件 → 代码滚到对应节点"用它。
    // 纯浏览动作，不改变编辑状态、移动光标或动撤销栈。
    void revealLine(int line)
    {
        if (m_hwnd == nullptr) return;
        if (line < 0) line = 0;
        if (line >= static_cast<int>(m_lines.size())) line = static_cast<int>(m_lines.size()) - 1;
        m_topLine = (std::max)(0, line - 2);
        ::InvalidateRect(m_hwnd, nullptr, FALSE);
    }

    // 输入法此刻是否在组合。宿主要转发键盘消息前必须先问一句：
    // 组合期间的 Enter/方向键属于输入法，抢走会让中文打不出来。
    bool isImeComposing() const
    {
        if (m_imeComposing) return true;
        if (m_hwnd == nullptr) return false;
        HIMC hic = ::ImmGetContext(m_hwnd);
        if (hic == nullptr) return false;
        const long compLen = ::ImmGetCompositionStringW(hic, GCS_COMPSTR, nullptr, 0);
        ::ImmReleaseContext(m_hwnd, hic);
        return compLen > 0;
    }

private:
    struct Pos
    {
        int line = 0;
        int col = 0;
    };
    bool posEq(const Pos& a, const Pos& b) const { return a.line == b.line && a.col == b.col; }
    bool posLess(const Pos& a, const Pos& b) const
    {
        return a.line < b.line || (a.line == b.line && a.col < b.col);
    }

    struct Run
    {
        std::wstring text;
        COLORREF color = RGB(0, 0, 0);
    };
    struct TipItem { std::wstring text; };
    struct Snapshot { std::vector<std::wstring> lines; Pos caret; Pos anchor; };

    // ---------------------------------------------------------------- 窗口类
    static constexpr wchar_t kClassName[] = L"PyMfcCodeEditorWnd";
    static constexpr wchar_t kTipClass[] = L"PyMfcCodeTip";
    static constexpr UINT_PTR kCaretTimerId = 0x61;

    static void ensureClass()
    {
        static bool once = false;
        if (once) return;
        WNDCLASSEXW wc{};
        wc.cbSize = sizeof(wc);
        wc.style = CS_DBLCLKS;
        wc.lpfnWndProc = &CodeEditor::wndProc;
        wc.hInstance = ::GetModuleHandleW(nullptr);
        wc.hCursor = ::LoadCursorW(nullptr, IDC_IBEAM);
        wc.hbrBackground = nullptr;
        wc.lpszClassName = kClassName;
        ::RegisterClassExW(&wc);
        once = true;
    }

    static void ensureTipClass()
    {
        static bool once = false;
        if (once) return;
        WNDCLASSEXW wc{};
        wc.cbSize = sizeof(wc);
        wc.lpfnWndProc = &CodeEditor::tipProc;
        wc.hInstance = ::GetModuleHandleW(nullptr);
        wc.hbrBackground = nullptr;
        wc.lpszClassName = kTipClass;
        ::RegisterClassExW(&wc);
        once = true;
    }

    // 诊断：设了环境变量 PYMFC_CE_TRACE 才写日志，用来确认键盘/输入法消息到底有没有到控件、
    // 以及输入法把按键变成了什么。注意：诊断只读不写，不影响控件行为。
    static void traceMsg(HWND h, UINT msg, WPARAM wp, LPARAM lp)
    {
        static int enabled = -1;
        if (enabled < 0)
        {
            wchar_t v[16] = {};
            enabled = (::GetEnvironmentVariableW(L"PYMFC_CE_TRACE", v, 16) != 0) ? 1 : 0;
        }
        if (enabled == 0) return;

        const wchar_t* name = nullptr;
        wchar_t nameBuf[32] = {};
        switch (msg)
        {
        case WM_CHAR: name = L"WM_CHAR"; break;
        case WM_SYSCHAR: name = L"WM_SYSCHAR"; break;
        case WM_KEYDOWN: name = L"WM_KEYDOWN"; break;
        case WM_KEYUP: name = L"WM_KEYUP"; break;
        case WM_SYSKEYDOWN: name = L"WM_SYSKEYDOWN"; break;
        case WM_SYSKEYUP: name = L"WM_SYSKEYUP"; break;
        case WM_IME_CHAR: name = L"WM_IME_CHAR"; break;
        case WM_IME_COMPOSITION: name = L"WM_IME_COMPOSITION"; break;
        case WM_IME_STARTCOMPOSITION: name = L"WM_IME_STARTCOMPOSITION"; break;
        case WM_IME_ENDCOMPOSITION: name = L"WM_IME_ENDCOMPOSITION"; break;
        case WM_IME_SETCONTEXT: name = L"WM_IME_SETCONTEXT"; break;
        case WM_IME_NOTIFY: name = L"WM_IME_NOTIFY"; break;
        case WM_SETFOCUS: name = L"WM_SETFOCUS"; break;
        case WM_KILLFOCUS: name = L"WM_KILLFOCUS"; break;
        default: return;
        }

        FILE* f = nullptr;
        if (_wfopen_s(&f, L"G:\\PyArx\\docs\\_ce_msg.log", L"a") != 0 || f == nullptr) return;

        // 通用的"这个键是什么字符"
        unsigned extChar = 0;
        if (msg == WM_CHAR || msg == WM_IME_CHAR)
            extChar = static_cast<unsigned>(wp);
        else if (msg == WM_KEYDOWN || msg == WM_KEYUP || msg == WM_SYSKEYDOWN || msg == WM_SYSKEYUP)
            extChar = 0;

        std::fprintf(f, "%ls msg=%04X wp=%04X lp=%08X", name, msg,
                     static_cast<unsigned>(wp), static_cast<unsigned>(lp));
        if (msg == WM_CHAR || msg == WM_IME_CHAR)
            std::fprintf(f, " ch='%c'(U+%04X)", wp >= 32 ? static_cast<int>(wp) : '.', extChar);

        // 按键时顺带记下"此刻输入法是否存在组合串"：
        //   有组合串 → 数字/标点会被输入法当成候选键吃掉，控件根本收不到字符；
        //   没组合串却没有 WM_CHAR → 才是控件自己的问题。
        if ((msg == WM_KEYDOWN || msg == WM_SYSKEYDOWN) && h != nullptr)
        {
            HIMC himc = ::ImmGetContext(h);
            if (himc != nullptr)
            {
                const LONG len = ::ImmGetCompositionStringW(himc, GCS_COMPSTR, nullptr, 0);
                std::fprintf(f, " composingLen=%d", static_cast<int>(len));
                ::ImmReleaseContext(h, himc);
            }
        }

        // 组合消息：把输入法当前给出的组合串/结果串原样记下来
        if (msg == WM_IME_COMPOSITION && h != nullptr)
        {
            HIMC himc = ::ImmGetContext(h);
            if (himc == nullptr)
            {
                std::fprintf(f, " himc=NULL");
            }
            else
            {
                std::fprintf(f, " flags=");
                if (lp & GCS_RESULTSTR) std::fprintf(f, "RESULT ");
                if (lp & GCS_COMPSTR) std::fprintf(f, "COMP ");
                if (lp & GCS_CURSORPOS) std::fprintf(f, "CURSOR ");
                if (lp & GCS_DELTASTART) std::fprintf(f, "DELTA ");

                auto dump = [&](DWORD kind, const char* tag)
                {
                    const LONG len = ::ImmGetCompositionStringW(himc, kind, nullptr, 0);
                    if (len <= 0) return;
                    std::wstring s(static_cast<size_t>(len) / sizeof(wchar_t), L'\0');
                    ::ImmGetCompositionStringW(himc, kind, &s[0], static_cast<DWORD>(len));
                    std::fprintf(f, " %s=[", tag);
                    for (wchar_t c : s)
                        std::fprintf(f, "%04X ", static_cast<unsigned>(c));
                    std::fprintf(f, "]");
                };
                dump(GCS_RESULTSTR, "result");
                dump(GCS_COMPSTR, "comp");
                ::ImmReleaseContext(h, himc);
            }
        }
        std::fprintf(f, "\n");
        std::fclose(f);
    }

    static LRESULT CALLBACK wndProc(HWND h, UINT msg, WPARAM wp, LPARAM lp)
    {
        traceMsg(h, msg, wp, lp);
        CodeEditor* self = reinterpret_cast<CodeEditor*>(::GetWindowLongPtrW(h, GWLP_USERDATA));
        switch (msg)
        {
        case WM_PAINT:
        {
            PAINTSTRUCT ps{};
            HDC hdc = ::BeginPaint(h, &ps);
            if (self != nullptr) self->paint(hdc);
            ::EndPaint(h, &ps);
            return 0;
        }
        case WM_ERASEBKGND:
            return 1;

        case WM_SETFOCUS:
            if (self != nullptr)
            {
                self->m_focused = true;
                self->startCaretTimer();
                ::InvalidateRect(h, nullptr, FALSE);
                if (self->m_onFocus) self->m_onFocus();
            }
            return 0;
        case WM_KILLFOCUS:
            if (self != nullptr)
            {
                self->m_focused = false;
                self->stopCaretTimer();
                self->closeTip();
                ::InvalidateRect(h, nullptr, FALSE);
                if (self->m_onBlur) self->m_onBlur();
            }
            return 0;

        case WM_TIMER:
            if (self != nullptr && wp == kCaretTimerId)
            {
                self->m_caretOn = !self->m_caretOn;
                ::InvalidateRect(h, nullptr, FALSE);
                return 0;
            }
            break;

        case WM_SIZE:
            if (self != nullptr)
            {
                self->ensureCaretVisible();
                ::InvalidateRect(h, nullptr, FALSE);
            }
            return 0;

        // ---- 键盘 ----
        case WM_KEYDOWN:
            if (self != nullptr)
            {
                if (self->onKeyDown(wp))
                    return 0;
                // 兜底：正常应用里 TranslateMessage 会把按键转成 WM_CHAR，
                // 但宿主消息泵/输入法有时不转（实测本设计器里数字键到达后一条 WM_CHAR 都没有）。
                // 这里用 ToUnicode 自己算出字符直接插入；系统若随后仍送来同字符的 WM_CHAR，就把它吞掉。
                self->typeCharFromKey(wp, lp);
            }
            break;

        // ---- 字符输入 ----
        case WM_CHAR:
            if (self != nullptr)
            {
                // 回车/退格/删除/Tab 都在 WM_KEYDOWN 里处理过，这里一律不重复
                if (wp == L'\r' || wp == L'\n' || wp == 8 || wp == 127 || wp == 9)
                    return 0;
                if (wp >= 32)
                {
                    const wchar_t ch = static_cast<wchar_t>(wp);
                    auto it = std::find(self->m_pendingChars.begin(), self->m_pendingChars.end(), ch);
                    if (it != self->m_pendingChars.end())
                    {
                        // 这个字符已经由 typeCharFromKey 插进去了，吞掉避免重复
                        self->m_pendingChars.erase(it);
                        return 0;
                    }
                    self->typeChar(ch);
                    return 0;
                }
            }
            break;

        // ---- 鼠标：命中全由本控件排版算 ----
        case WM_LBUTTONDOWN:
            if (self != nullptr)
            {
                ::SetFocus(h);
                self->closeTip();
                const Pos p = self->hitTest(GET_X_LPARAM(lp), GET_Y_LPARAM(lp));
                self->m_caret = p;
                self->m_anchor = p;
                self->m_dragging = true;
                ::SetCapture(h);
                self->ensureCaretVisible();
                ::InvalidateRect(h, nullptr, FALSE);
                return 0;
            }
            break;
        case WM_MOUSEMOVE:
            if (self != nullptr && self->m_dragging && (wp & MK_LBUTTON) != 0)
            {
                const Pos p = self->hitTest(GET_X_LPARAM(lp), GET_Y_LPARAM(lp));
                self->m_caret = p;
                self->ensureCaretVisible();
                ::InvalidateRect(h, nullptr, FALSE);
                return 0;
            }
            break;
        // ---- 右键：复制 / 剪切 / 粘贴 / 全选（与对象检查器同一套动作，只是操作位置在代码区）----
        case WM_RBUTTONUP:
            if (self != nullptr)
            {
                ::SetFocus(h);
                POINT scr = { GET_X_LPARAM(lp), GET_Y_LPARAM(lp) };
                ::ClientToScreen(h, &scr);
                HMENU menu = ::CreatePopupMenu();
                const bool hasSel = (self->m_caret.line != self->m_anchor.line ||
                                     self->m_caret.col != self->m_anchor.col);
                ::AppendMenuW(menu, MF_STRING | (hasSel ? MF_ENABLED : MF_GRAYED), 1, L"复制");
                ::AppendMenuW(menu, MF_STRING | (hasSel ? MF_ENABLED : MF_GRAYED), 2, L"剪切");
                ::AppendMenuW(menu, MF_STRING | (::IsClipboardFormatAvailable(CF_UNICODETEXT) ? MF_ENABLED : MF_GRAYED), 3, L"粘贴");
                ::AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
                ::AppendMenuW(menu, MF_STRING, 4, L"全选");
                const int cmd = ::TrackPopupMenu(menu, TPM_RETURNCMD | TPM_LEFTALIGN | TPM_TOPALIGN,
                                                 scr.x, scr.y, 0, h, nullptr);
                ::DestroyMenu(menu);
                if (cmd == 1) self->copyToClipboard();
                else if (cmd == 2) self->cutToClipboard();
                else if (cmd == 3) self->pasteFromClipboard();
                else if (cmd == 4)
                {
                    self->m_anchor = Pos{ 0, 0 };
                    self->m_caret = Pos{ static_cast<int>(self->m_lines.size()) - 1,
                                          static_cast<int>(self->m_lines.back().size()) };
                    ::InvalidateRect(h, nullptr, FALSE);
                }
                return 0;
            }
            break;

        case WM_LBUTTONUP:
            if (self != nullptr && self->m_dragging)
            {
                self->m_dragging = false;
                ::ReleaseCapture();
                ::InvalidateRect(h, nullptr, FALSE);
                return 0;
            }
            break;
        case WM_LBUTTONDBLCLK:
            if (self != nullptr)
            {
                const Pos p = self->hitTest(GET_X_LPARAM(lp), GET_Y_LPARAM(lp));
                Pos a = p, b = p;
                self->wordRangeAt(p, a, b);
                self->m_anchor = a;
                self->m_caret = b;
                self->m_dragging = true;
                ::SetCapture(h);
                ::InvalidateRect(h, nullptr, FALSE);
                return 0;
            }
            break;
        case WM_CAPTURECHANGED:
            if (self != nullptr) self->m_dragging = false;
            break;

        case WM_MOUSEWHEEL:
            if (self != nullptr)
            {
                const int keys = GET_KEYSTATE_WPARAM(wp);
                const int delta = GET_WHEEL_DELTA_WPARAM(wp);
                if ((keys & MK_CONTROL) != 0)
                {
                    if (delta != 0) self->setZoom(self->m_pt + (delta > 0 ? 1 : -1));
                    return 0;
                }
                if (delta != 0) self->scrollLines(delta > 0 ? -3 : 3);
                return 0;
            }
            break;

        case WM_SETCURSOR:
            ::SetCursor(::LoadCursorW(nullptr, IDC_IBEAM));
            return TRUE;

        // ---- 输入法 ----
        // 做法：组合串交给系统（DefWindowProc），我们只负责
        //   1) 把组合窗/候选窗定位到自绘光标处；
        //   2) 在 WM_IME_CHAR 里把上屏字符插进自己的缓冲。
        // 之前"自己接管 WM_IME_COMPOSITION"会让输入法判定宿主已处理，
        // 结果既不产生 WM_CHAR 也不给结果串（数字、标点直接被吞）。
        case WM_IME_SETCONTEXT:
            break;
        case WM_IME_STARTCOMPOSITION:
            if (self != nullptr)
                self->updateImeWindow();
            break;
        case WM_IME_COMPOSITION:
            if (self != nullptr)
                self->updateImeWindow();
            break;
        case WM_IME_ENDCOMPOSITION:
            break;
        // 输入法上屏的字符走这里。自己吃下来插入，避免 DefWindowProc 再转一次 WM_CHAR 造成重复。
        case WM_IME_CHAR:
            if (self != nullptr && !self->m_readOnly)
            {
                const wchar_t ch = static_cast<wchar_t>(wp);
                if (ch >= 32 || ch == L'\n')
                {
                    self->typeChar(ch);
                    return 0;
                }
            }
            break;

        default:
            break;
        }
        return ::DefWindowProcW(h, msg, wp, lp);
    }

    // ---------------------------------------------------------------- 键盘
    bool onKeyDown(WPARAM vk)
    {
        const bool shift = (::GetKeyState(VK_SHIFT) & 0x8000) != 0;
        const bool ctrl = (::GetKeyState(VK_CONTROL) & 0x8000) != 0;

        if (m_readOnly)
        {
            // 只读也允许移动光标与复制
            switch (vk)
            {
            case VK_LEFT: case VK_RIGHT: case VK_UP: case VK_DOWN:
            case VK_HOME: case VK_END: case VK_PRIOR: case VK_NEXT:
                break;
            case 'C': case VK_INSERT:
                if (ctrl) { copyToClipboard(); return true; }
                return false;
            default:
                return false;
            }
        }

        // 提示窗打开时的导航键
        if (vk == VK_ESCAPE && m_tipOpen) { closeTip(); return true; }
        if (vk == VK_DOWN && m_tipOpen && !shift) { tipMove(1); return true; }
        if (vk == VK_UP && m_tipOpen && !shift) { tipMove(-1); return true; }
        if ((vk == VK_RETURN || vk == VK_TAB) && m_tipOpen) { acceptCompletion(); return true; }

        switch (vk)
        {
        case VK_LEFT:
            if (ctrl) moveWordLeft(shift); else moveCaret(-1, 0, shift, true);
            return true;
        case VK_RIGHT:
            if (ctrl) moveWordRight(shift); else moveCaret(1, 0, shift, true);
            return true;
        case VK_UP:
            moveCaret(0, -1, shift, false);
            return true;
        case VK_DOWN:
            moveCaret(0, 1, shift, false);
            return true;
        case VK_HOME:
            if (ctrl) setCaret(Pos{0, 0}, shift);
            else setCaret(Pos{m_caret.line, 0}, shift);
            return true;
        case VK_END:
            if (ctrl) setCaret(Pos{static_cast<int>(m_lines.size()) - 1,
                                 lineLen(static_cast<int>(m_lines.size()) - 1)}, shift);
            else setCaret(Pos{m_caret.line, lineLen(m_caret.line)}, shift);
            return true;
        case VK_PRIOR:
            movePage(-1, shift);
            return true;
        case VK_NEXT:
            movePage(1, shift);
            return true;
        case VK_BACK:
            if (ctrl) { deleteWordLeft(); return true; }
            backspace();
            return true;
        case VK_DELETE:
            deleteForward();
            return true;
        case VK_RETURN:
            newline();
            return true;
        case VK_TAB:
            if (ctrl) return false;
            insertTabSpaces();
            return true;
        case 'A':
            if (ctrl)
            {
                m_anchor = Pos{0, 0};
                m_caret = Pos{static_cast<int>(m_lines.size()) - 1,
                              lineLen(static_cast<int>(m_lines.size()) - 1)};
                scrollCaretIntoView();
                ::InvalidateRect(m_hwnd, nullptr, FALSE);
                return true;
            }
            return false;
        case 'C':
            if (ctrl) { copyToClipboard(); return true; }
            return false;
        case 'X':
            if (ctrl) { cutToClipboard(); return true; }
            return false;
        case 'V':
            if (ctrl) { pasteFromClipboard(); return true; }
            return false;
        case 'Z':
            if (ctrl) { undo(); return true; }
            return false;
        case 'Y':
            if (ctrl) { redo(); return true; }
            return false;
        default:
            return false;
        }
    }

    void moveCaret(int dCol, int dLine, bool extend, bool wrap)
    {
        Pos p = m_caret;
        if (dLine != 0)
        {
            p.line += dLine;
            p.line = (std::max)(0, (std::min)(p.line, static_cast<int>(m_lines.size()) - 1));
            p.col = (std::min)(p.col, lineLen(p.line));
        }
        else if (dCol != 0)
        {
            p.col += dCol;
            if (p.col < 0)
            {
                if (wrap && p.line > 0) { p.line--; p.col = lineLen(p.line); }
                else p.col = 0;
            }
            else if (p.col > lineLen(p.line))
            {
                if (wrap && p.line + 1 < static_cast<int>(m_lines.size())) { p.line++; p.col = 0; }
                else p.col = lineLen(p.line);
            }
        }
        setCaret(p, extend);
    }

    void movePage(int dir, bool extend)
    {
        const int page = (std::max)(1, pageLines());
        Pos p = m_caret;
        p.line += dir * page;
        p.line = (std::max)(0, (std::min)(p.line, static_cast<int>(m_lines.size()) - 1));
        p.col = (std::min)(p.col, lineLen(p.line));
        setCaret(p, extend);
        m_topLine += dir * page;
    }

    void setCaret(const Pos& p, bool extend)
    {
        if (!extend)
            m_anchor = p;
        m_caret = p;
        clampCaret();
        scrollCaretIntoView();
        ::InvalidateRect(m_hwnd, nullptr, FALSE);
    }

    void clampCaret()
    {
        if (m_lines.empty()) m_lines.push_back(std::wstring());
        m_caret.line = (std::max)(0, (std::min)(m_caret.line, static_cast<int>(m_lines.size()) - 1));
        m_caret.col = (std::max)(0, (std::min)(m_caret.col, lineLen(m_caret.line)));
        m_anchor.line = (std::max)(0, (std::min)(m_anchor.line, static_cast<int>(m_lines.size()) - 1));
        m_anchor.col = (std::max)(0, (std::min)(m_anchor.col, lineLen(m_anchor.line)));
    }

    void moveWordLeft(bool extend)
    {
        Pos p = m_caret;
        while (p.col > 0 && !isWordChar(lineRef(p.line)[static_cast<size_t>(p.col - 1)])) p.col--;
        while (p.col > 0 && isWordChar(lineRef(p.line)[static_cast<size_t>(p.col - 1)])) p.col--;
        setCaret(p, extend);
    }

    void moveWordRight(bool extend)
    {
        Pos p = m_caret;
        const std::wstring& s = lineRef(p.line);
        while (p.col < static_cast<int>(s.size()) && isWordChar(s[static_cast<size_t>(p.col)]) ) p.col++;
        while (p.col < static_cast<int>(s.size()) && !isWordChar(s[static_cast<size_t>(p.col)]) ) p.col++;
        setCaret(p, extend);
    }

    // ---------------------------------------------------------------- 编辑操作
    void pushUndo()
    {
        Snapshot s;
        s.lines = m_lines;
        s.caret = m_caret;
        s.anchor = m_anchor;
        m_undo.push_back(std::move(s));
        if (m_undo.size() > 64) m_undo.erase(m_undo.begin());
        m_redo.clear();
    }

    void undo()
    {
        if (m_undo.empty()) return;
        Snapshot cur;
        cur.lines = m_lines;
        cur.caret = m_caret;
        cur.anchor = m_anchor;
        m_redo.push_back(std::move(cur));

        Snapshot s = std::move(m_undo.back());
        m_undo.pop_back();
        m_lines = std::move(s.lines);
        m_caret = s.caret;
        m_anchor = s.anchor;
        clampCaret();
        scrollCaretIntoView();
        ::InvalidateRect(m_hwnd, nullptr, FALSE);
        notifyChange();
        updateCompletion();
    }

    void redo()
    {
        if (m_redo.empty()) return;
        Snapshot cur;
        cur.lines = m_lines;
        cur.caret = m_caret;
        cur.anchor = m_anchor;
        m_undo.push_back(std::move(cur));

        Snapshot s = std::move(m_redo.back());
        m_redo.pop_back();
        m_lines = std::move(s.lines);
        m_caret = s.caret;
        m_anchor = s.anchor;
        clampCaret();
        scrollCaretIntoView();
        ::InvalidateRect(m_hwnd, nullptr, FALSE);
        notifyChange();
        updateCompletion();
    }

    bool hasSelection() const { return !posEq(m_caret, m_anchor); }

    // 选区的有序区间
    void selection(Pos& a, Pos& b) const
    {
        if (posLess(m_caret, m_anchor)) { a = m_caret; b = m_anchor; }
        else { a = m_anchor; b = m_caret; }
    }

    void deleteSelection()
    {
        Pos a, b;
        selection(a, b);
        if (!posLess(a, b)) return;

        if (a.line == b.line)
        {
            m_lines[static_cast<size_t>(a.line)].erase(static_cast<size_t>(a.col),
                                                       static_cast<size_t>(b.col - a.col));
        }
        else
        {
            std::wstring head = lineRef(a.line).substr(0, static_cast<size_t>(a.col));
            std::wstring tail = lineRef(b.line).substr(static_cast<size_t>(b.col));
            m_lines.erase(m_lines.begin() + a.line + 1, m_lines.begin() + b.line + 1);
            m_lines[static_cast<size_t>(a.line)] = head + tail;
        }
        m_caret = a;
        m_anchor = a;
        clampCaret();
    }

    // 在光标处插入字符串（\n 会拆成多行），替换掉选区
    void insertText(const std::wstring& s)
    {
        if (m_readOnly) return;
        pushUndo();
        if (hasSelection()) deleteSelection();

        std::vector<std::wstring> parts;
        std::wstring cur;
        for (wchar_t c : s)
        {
            if (c == L'\r') continue;
            if (c == L'\n') { parts.push_back(cur); cur.clear(); }
            else cur += c;
        }
        parts.push_back(cur);

        std::wstring& ln = m_lines[static_cast<size_t>(m_caret.line)];
        std::wstring head = ln.substr(0, static_cast<size_t>(m_caret.col));
        std::wstring tail = ln.substr(static_cast<size_t>(m_caret.col));

        if (parts.size() == 1)
        {
            ln = head + parts[0] + tail;
            m_caret.col += static_cast<int>(parts[0].size());
        }
        else
        {
            ln = head + parts[0];
            std::vector<std::wstring> mid(parts.begin() + 1, parts.end());
            m_lines.insert(m_lines.begin() + m_caret.line + 1, mid.begin(), mid.end());
            m_lines[static_cast<size_t>(m_caret.line + parts.size() - 1)] += tail;
            m_caret.line += static_cast<int>(parts.size()) - 1;
            m_caret.col = static_cast<int>(parts.back().size());
        }
        m_anchor = m_caret;
        clampCaret();
        scrollCaretIntoView();
        ::InvalidateRect(m_hwnd, nullptr, FALSE);
        notifyChange();
        updateCompletion();
    }

    void typeChar(wchar_t c)
    {
        if (m_readOnly) return;
        insertText(std::wstring(1, c));
        traceBuffer(L"typeChar");
    }

    // 诊断：把编辑后的光标位置和行内容记进日志（同样只在 PYMFC_CE_TRACE 打开时写）。
    void traceBuffer(const wchar_t* why)
    {
        static int enabled = -1;
        if (enabled < 0)
        {
            wchar_t v[16] = {};
            enabled = (::GetEnvironmentVariableW(L"PYMFC_CE_TRACE", v, 16) != 0) ? 1 : 0;
        }
        if (enabled == 0) return;
        FILE* f = nullptr;
        if (_wfopen_s(&f, L"G:\\PyArx\\docs\\_ce_msg.log", L"a") != 0 || f == nullptr) return;
        std::fprintf(f, "[EDIT %ls] caret=(%d,%d) lines=%d line0=[", why, m_caret.line, m_caret.col,
                     static_cast<int>(m_lines.size()));
        std::fwprintf(f, L"%ls", m_lines.empty() ? L"" : m_lines.front().c_str());
        std::fprintf(f, "] cur=[");
        if (m_caret.line >= 0 && m_caret.line < static_cast<int>(m_lines.size()))
            std::fwprintf(f, L"%ls", m_lines[static_cast<size_t>(m_caret.line)].c_str());
        std::fprintf(f, "]\n");
        std::fclose(f);
    }

    // 按键兜底转字符：
    //   * 输入法正在组合（或该键已被输入法接管 VK_PROCESSKEY）时不动，交给输入法；
    //   * Ctrl/Alt 组合键不动，那是命令不是字符；
    //   * 其余用 ToUnicode 按当前键盘状态（含 Shift/CapsLock）算字符，直接插入。
    void typeCharFromKey(WPARAM vk, LPARAM lp)
    {
        if (m_readOnly) return;
        if (vk == VK_PROCESSKEY) return;
        if ((::GetKeyState(VK_CONTROL) & 0x8000) != 0) return;
        if ((::GetKeyState(VK_MENU) & 0x8000) != 0) return;

        // 输入法有组合串时不抢字符
        HIMC himc = ::ImmGetContext(m_hwnd);
        if (himc != nullptr)
        {
            const LONG compLen = ::ImmGetCompositionStringW(himc, GCS_COMPSTR, nullptr, 0);
            ::ImmReleaseContext(m_hwnd, himc);
            if (compLen > 0) return;
        }

        BYTE keyState[256] = {};
        ::GetKeyboardState(keyState);

        const UINT scan = static_cast<UINT>((lp >> 16) & 0xFF);
        WCHAR out[8] = {};
        const int n = ::ToUnicode(static_cast<UINT>(vk), scan, keyState, out, 8, 0);
        if (n <= 0) return;

        for (int i = 0; i < n; ++i)
        {
            const wchar_t c = out[i];
            if (c < 32) continue;               // 控制字符（退格/回车等）已在 onKeyDown 处理
            if (m_pendingChars.size() >= 16) m_pendingChars.erase(m_pendingChars.begin());
            m_pendingChars.push_back(c);
            typeChar(c);
        }
    }

    void newline()
    {
        if (m_readOnly) return;

        // 自动缩进：承接当前行的前导空白；再按语法决定是否再加一级。
        //   XML —— 行尾是未闭合的开标签（如 `<Button ...>` 或单独一个 `<`）时缩进一级；
        //   Python —— 行尾是 `:`、`{`、`(`、`[` 时缩进一级。
        const std::wstring& ln = m_lines[static_cast<size_t>(m_caret.line)];
        std::wstring indent;
        for (wchar_t c : ln)
        {
            if (c == L' ' || c == L'\t') indent += c;
            else break;
        }

        std::wstring before = ln.substr(0, static_cast<size_t>(m_caret.col));
        while (!before.empty() && (before.back() == L' ' || before.back() == L'\t'))
            before.pop_back();

        if (!before.empty())
        {
            if (m_syntax == Syntax::Xml)
            {
                if (before.back() == L'<' || lastXmlTagIsOpen(before))
                    indent += L"    ";
            }
            else
            {
                const wchar_t last = before.back();
                if (last == L':' || last == L'{' || last == L'(' || last == L'[')
                    indent += L"    ";
            }
        }

        insertText(L"\n" + indent);
        traceBuffer(L"newline");
    }

    // 判断 `s` 里最后一个标签是不是"还没闭合的开标签"：
    //   `<Button>`      → true
    //   `<Button/>`     → false
    //   `<?xml ...?>`   → false
    //   `</Button>`     → false
    //   `<Name="x"`     → false（还没写完，不缩进）
    static bool lastXmlTagIsOpen(const std::wstring& s)
    {
        const size_t lt = s.rfind(L'<');
        if (lt == std::wstring::npos) return false;
        if (s.back() != L'>') return false;
        if (lt + 1 >= s.size()) return false;
        const wchar_t first = s[lt + 1];
        if (first == L'/' || first == L'?' || first == L'!') return false;
        if (s.substr(lt).find(L"/>") != std::wstring::npos) return false;   // 自闭合
        return true;
    }

    void insertTabSpaces()
    {
        if (m_readOnly) return;
        const int n = 4 - (m_caret.col % 4);
        insertText(std::wstring(static_cast<size_t>(n), L' '));
    }

    void backspace()
    {
        if (m_readOnly) return;
        if (hasSelection()) { pushUndo(); deleteSelection(); afterEdit(); return; }

        // 软缩进：光标停在前导空白里时，一次退格退掉整段缩进，而不是一个空格一个空格退
        if (m_caret.col > 0)
        {
            const std::wstring& ln = m_lines[static_cast<size_t>(m_caret.line)];
            bool allLeadingWs = true;
            for (int i = 0; i < m_caret.col && i < static_cast<int>(ln.size()); ++i)
            {
                if (ln[static_cast<size_t>(i)] != L' ' && ln[static_cast<size_t>(i)] != L'\t')
                {
                    allLeadingWs = false;
                    break;
                }
            }
            if (allLeadingWs && m_caret.col >= 2)
            {
                pushUndo();
                m_lines[static_cast<size_t>(m_caret.line)].erase(0, static_cast<size_t>(m_caret.col));
                m_caret.col = 0;
                m_anchor = m_caret;
                afterEdit();
                return;
            }
        }

        if (m_caret.col > 0)
        {
            pushUndo();
            m_lines[static_cast<size_t>(m_caret.line)].erase(static_cast<size_t>(m_caret.col - 1), 1);
            m_caret.col--;
            m_anchor = m_caret;
        }
        else if (m_caret.line > 0)
        {
            pushUndo();
            const int prevLen = lineLen(m_caret.line - 1);
            m_lines[static_cast<size_t>(m_caret.line - 1)] += lineRef(m_caret.line);
            m_lines.erase(m_lines.begin() + m_caret.line);
            m_caret.line--;
            m_caret.col = prevLen;
            m_anchor = m_caret;
        }
        afterEdit();
    }

    void deleteForward()
    {
        if (m_readOnly) return;
        if (hasSelection()) { pushUndo(); deleteSelection(); afterEdit(); return; }
        const std::wstring& ln = lineRef(m_caret.line);
        if (m_caret.col < static_cast<int>(ln.size()))
        {
            pushUndo();
            m_lines[static_cast<size_t>(m_caret.line)].erase(static_cast<size_t>(m_caret.col), 1);
        }
        else if (m_caret.line + 1 < static_cast<int>(m_lines.size()))
        {
            pushUndo();
            m_lines[static_cast<size_t>(m_caret.line)] += lineRef(m_caret.line + 1);
            m_lines.erase(m_lines.begin() + m_caret.line + 1);
        }
        afterEdit();
    }

    void deleteWordLeft()
    {
        if (m_readOnly) return;
        Pos from = m_caret;
        const std::wstring& s = lineRef(m_caret.line);
        while (from.col > 0 && !isWordChar(s[static_cast<size_t>(from.col - 1)]) ) from.col--;
        while (from.col > 0 && isWordChar(s[static_cast<size_t>(from.col - 1)]) ) from.col--;
        m_anchor = from;
        deleteSelection();
        afterEdit();
    }

    void afterEdit()
    {
        clampCaret();
        scrollCaretIntoView();
        ::InvalidateRect(m_hwnd, nullptr, FALSE);
        notifyChange();
        // Tab/回车补全刚插入的这一刻不要再把提示弹回来（否则补全后提示框赖着不走）
        if (m_skipTipOnce)
        {
            m_skipTipOnce = false;
            return;
        }
        updateCompletion();
    }

    void notifyChange()
    {
        if (m_onChange == nullptr) return;
        const std::wstring now = text();
        if (now != m_lastSeen)
        {
            m_lastSeen = now;
            m_onChange(now);
        }
    }

    // ---------------------------------------------------------------- 剪贴板
    std::wstring selectedText() const
    {
        Pos a, b;
        selection(a, b);
        if (!posLess(a, b)) return {};
        std::wstring out;
        if (a.line == b.line)
            out = lineRef(a.line).substr(static_cast<size_t>(a.col),
                                        static_cast<size_t>(b.col - a.col));
        else
        {
            out = lineRef(a.line).substr(static_cast<size_t>(a.col));
            for (int i = a.line + 1; i < b.line; ++i) { out += L"\r\n"; out += lineRef(i); }
            out += L"\r\n";
            out += lineRef(b.line).substr(0, static_cast<size_t>(b.col));
        }
        return out;
    }

    bool openClipboardForWrite()
    {
        for (int i = 0; i < 5; ++i)
        {
            if (::OpenClipboard(m_hwnd)) return true;
            ::Sleep(10);
        }
        return false;
    }

    void copyToClipboard()
    {
        const std::wstring s = selectedText();
        if (s.empty()) return;
        if (!openClipboardForWrite()) return;
        ::EmptyClipboard();
        const size_t bytes = (s.size() + 1) * sizeof(wchar_t);
        HGLOBAL hg = ::GlobalAlloc(GMEM_MOVEABLE, bytes);
        if (hg != nullptr)
        {
            void* p = ::GlobalLock(hg);
            if (p != nullptr)
            {
                memcpy(p, s.c_str(), bytes);
                ::GlobalUnlock(hg);
                ::SetClipboardData(CF_UNICODETEXT, hg);
            }
        }
        ::CloseClipboard();
    }

    void cutToClipboard()
    {
        if (m_readOnly) return;
        copyToClipboard();
        if (hasSelection())
        {
            pushUndo();
            deleteSelection();
            afterEdit();
        }
    }

    void pasteFromClipboard()
    {
        if (m_readOnly) return;
        if (!::IsClipboardFormatAvailable(CF_UNICODETEXT)) return;
        if (!::OpenClipboard(m_hwnd)) return;
        std::wstring s;
        HGLOBAL hg = ::GetClipboardData(CF_UNICODETEXT);
        if (hg != nullptr)
        {
            const wchar_t* p = static_cast<const wchar_t*>(::GlobalLock(hg));
            if (p != nullptr)
            {
                s.assign(p);
                ::GlobalUnlock(hg);
            }
        }
        ::CloseClipboard();
        if (!s.empty())
            insertText(s);
    }

    // ---------------------------------------------------------------- 滚动
    int pageLines() const
    {
        RECT rc;
        ::GetClientRect(m_hwnd, &rc);
        return (std::max)(1, static_cast<int>(rc.bottom) / m_lineH);
    }

    void scrollLines(int delta)
    {
        const int total = static_cast<int>(m_lines.size());
        m_topLine += delta;
        m_topLine = (std::max)(0, (std::min)(m_topLine, (std::max)(0, total - 1)));
        ::InvalidateRect(m_hwnd, nullptr, FALSE);
    }

    void scrollCaretIntoView()
    {
        const int page = pageLines();
        if (m_caret.line < m_topLine) m_topLine = m_caret.line;
        if (m_caret.line >= m_topLine + page) m_topLine = m_caret.line - page + 1;
        m_topLine = (std::max)(0, m_topLine);
    }

    void ensureCaretVisible()
    {
        RECT rc;
        ::GetClientRect(m_hwnd, &rc);
        const int w = rc.right;
        const int textLeft = textOriginX();
        const int cx = measureInt(lineRef(m_caret.line).substr(0, static_cast<size_t>(m_caret.col)));
        const int room = w - textLeft - 12;
        if (cx - m_hScroll > room) m_hScroll = cx - room;
        else if (cx - m_hScroll < 0) m_hScroll = (std::max)(0, cx - 24);
        scrollCaretIntoView();
    }

    // ---------------------------------------------------------------- 命中测试
    int textOriginX() const { return m_gutterW + kPadLeft + 2 - m_hScroll; }

    Pos hitTest(int x, int y)
    {
        const int line = (std::max)(0, (std::min)(m_topLine + (y > 0 ? y / m_lineH : 0),
                                                  static_cast<int>(m_lines.size()) - 1));
        const int rel = x - textOriginX();
        int col = 0;
        if (rel > 0)
        {
            const std::wstring& s = lineRef(line);
            int prev = 0;
            for (int i = 1; i <= static_cast<int>(s.size()); ++i)
            {
                const int wNext = widthOf(s.substr(0, static_cast<size_t>(i)));
                if (rel < (prev + wNext) / 2) { col = i - 1; return Pos{line, col}; }
                prev = wNext;
            }
            col = static_cast<int>(s.size());
        }
        return Pos{line, col};
    }

    void wordRangeAt(const Pos& p, Pos& a, Pos& b)
    {
        const std::wstring& s = lineRef(p.line);
        int l = p.col, r = p.col;
        if (l > static_cast<int>(s.size())) l = static_cast<int>(s.size());
        while (l > 0 && isWordChar(s[static_cast<size_t>(l - 1)])) l--;
        while (r < static_cast<int>(s.size()) && isWordChar(s[static_cast<size_t>(r)])) r++;
        a = Pos{p.line, l};
        b = Pos{p.line, r};
    }

    static bool isWordChar(wchar_t c)
    {
        return iswalnum(c) != 0 || c == L'_' || c == L'-' || c == L'.' || c == L':';
    }

    // ---------------------------------------------------------------- 辅助
    int lineLen(int i) const
    {
        if (i < 0 || i >= static_cast<int>(m_lines.size())) return 0;
        return static_cast<int>(m_lines[static_cast<size_t>(i)].size());
    }

    const std::wstring& lineRef(int i) const
    {
        static const std::wstring empty;
        if (i < 0 || i >= static_cast<int>(m_lines.size())) return empty;
        return m_lines[static_cast<size_t>(i)];
    }

    void applyFont()
    {
        RECT rc;
        ::GetClientRect(m_hwnd, &rc);
        const int px = (std::max)(8, (m_pt * 96 + 36) / 72);
        LOGFONTW lf{};
        lf.lfHeight = -px;
        lf.lfWeight = FW_NORMAL;
        lf.lfCharSet = DEFAULT_CHARSET;
        lf.lfPitchAndFamily = FIXED_PITCH | FF_MODERN;
        wcscpy_s(lf.lfFaceName, L"Consolas");
        HFONT made = ::CreateFontIndirectW(&lf);
        if (made != nullptr)
        {
            if (m_font != nullptr) ::DeleteObject(m_font);
            m_font = made;
        }
        m_lineH = px + 5;
        const int perChar = (std::max)(4, px / 2);
        const int digits = (std::max)(1, static_cast<int>(std::to_wstring(m_lines.size()).size()));
        m_gutterW = (std::max)(kGutterW, 12 + perChar * digits);
        (void)rc;
        ::InvalidateRect(m_hwnd, nullptr, FALSE);
    }

    // 量宽：与绘制同一把尺子（画布 DWrite），失败时退回 GDI
    float measureF(const std::wstring& s)
    {
        if (s.empty()) return 0.0f;
        if (!m_cv) m_cv = std::make_unique<D2DDcCanvas>();
        float w = m_cv->textWidth(s, m_pt, L"Consolas");
        if (w < 0.0f)
        {
            HDC hdc = ::GetDC(m_hwnd);
            HGDIOBJ old = (m_font != nullptr) ? ::SelectObject(hdc, m_font) : nullptr;
            SIZE sz{};
            ::GetTextExtentPoint32W(hdc, s.c_str(), static_cast<int>(s.size()), &sz);
            if (old != nullptr) ::SelectObject(hdc, old);
            ::ReleaseDC(m_hwnd, hdc);
            w = static_cast<float>(sz.cx);
        }
        return w;
    }

    int measureInt(const std::wstring& s) { return static_cast<int>(measureF(s) + 0.5f); }

    // 兼容旧名字（内部实现已统一为 measureInt）
    int widthOf(const std::wstring& s) { return measureInt(s); }

    // ---------------------------------------------------------------- 光标闪烁
    void startCaretTimer()
    {
        if (m_caretTimer) return;
        m_caretOn = true;
        ::SetTimer(m_hwnd, kCaretTimerId, static_cast<UINT>(::GetCaretBlinkTime()), nullptr);
        m_caretTimer = true;
    }

    void stopCaretTimer()
    {
        if (!m_caretTimer) return;
        ::KillTimer(m_hwnd, kCaretTimerId);
        m_caretTimer = false;
        m_caretOn = false;
    }

    // ---------------------------------------------------------------- 输入法
    Pos caretPos() const { return m_caret; }

    void updateImeWindow()
    {
        if (m_hwnd == nullptr) return;
        HIMC himc = ::ImmGetContext(m_hwnd);
        if (himc == nullptr) return;

        POINT pt = caretScreenPoint();
        COMPOSITIONFORM cf{};
        cf.dwStyle = CFS_POINT;
        cf.ptCurrentPos = pt;
        ::ImmSetCompositionWindow(himc, &cf);

        CANDIDATEFORM cd{};
        cd.dwIndex = 0;
        cd.dwStyle = CFS_CANDIDATEPOS;
        cd.ptCurrentPos = pt;
        ::ImmSetCandidateWindow(himc, &cd);
        ::ImmReleaseContext(m_hwnd, himc);
    }

    POINT caretScreenPoint()
    {
        POINT p{textOriginX() + measureInt(lineRef(m_caret.line).substr(0,
                     static_cast<size_t>(m_caret.col))),
                (m_caret.line - m_topLine) * m_lineH + m_lineH};
        return p;   // 客户区坐标（ImmSetCompositionWindow 用客户区）
    }

    bool onImeComposition(LPARAM lp)
    {
        if (m_readOnly) return false;
        HIMC himc = ::ImmGetContext(m_hwnd);
        if (himc == nullptr) return false;

        bool consumed = false;
        bool changed = false;
        if ((lp & GCS_RESULTSTR) != 0)
        {
            const LONG len = ::ImmGetCompositionStringW(himc, GCS_RESULTSTR, nullptr, 0);
            if (len > 0)
            {
                std::wstring s(static_cast<size_t>(len) / sizeof(wchar_t), L'\0');
                ::ImmGetCompositionStringW(himc, GCS_RESULTSTR, &s[0], static_cast<DWORD>(len));
                m_preedit.clear();
                m_imeComposing = false;
                insertText(s);
                changed = true;
            }
            consumed = true;
        }
        if ((lp & GCS_COMPSTR) != 0)
        {
            const LONG len = ::ImmGetCompositionStringW(himc, GCS_COMPSTR, nullptr, 0);
            if (len > 0)
            {
                std::wstring s(static_cast<size_t>(len) / sizeof(wchar_t), L'\0');
                ::ImmGetCompositionStringW(himc, GCS_COMPSTR, &s[0], static_cast<DWORD>(len));
                m_preedit = s;
            }
            else
            {
                m_preedit.clear();
            }
            changed = true;
            consumed = true;
        }
        ::ImmReleaseContext(m_hwnd, himc);

        if (consumed)
        {
            updateImeWindow();
            if (changed)
                ::InvalidateRect(m_hwnd, nullptr, FALSE);
        }
        return consumed;
    }

    // ---------------------------------------------------------------- 绘制
    void paint(HDC hdc)
    {
        RECT rc;
        ::GetClientRect(m_hwnd, &rc);
        const int w = rc.right;
        const int h = rc.bottom;
        if (w <= 0 || h <= 0) return;

        if (!m_cv)
            m_cv = std::make_unique<D2DDcCanvas>();
        m_cv->bind(hdc, RECT{0, 0, w, h});
        if (!m_cv->begin(w, h))
            return;

        XmlCanvas& cv = *m_cv;
        cv.fillRect(rc, cBack());

        // 行号槽
        cv.fillRect(RECT{0, 0, m_gutterW, h}, cGutter());
        cv.fillRect(RECT{m_gutterW, 0, m_gutterW + 1, h}, cGutterLine());

        // 当前行底纹
        if (m_focused && m_caret.line >= m_topLine)
        {
            const int y = (m_caret.line - m_topLine) * m_lineH;
            cv.fillRect(RECT{m_gutterW + 1, y, w, y + m_lineH}, cBand());
        }

        const int textX = textOriginX();
        const int total = static_cast<int>(m_lines.size());
        const int visible = h / m_lineH + 1;
        const int lastVisible = (std::min)(total, m_topLine + visible);

        Pos selA{}, selB{};
        const bool hasSel = hasSelection();
        if (hasSel) selection(selA, selB);

        // 行号槽：在裁剪区之外画（裁剪区从 m_gutterW+1 开始，先画行号再压裁剪）
        for (int i = m_topLine; i < lastVisible; ++i)
        {
            const int y = (i - m_topLine) * m_lineH;
            cv.text(RECT{0, y, m_gutterW - 6, y + m_lineH}, std::to_wstring(i + 1), cNum(),
                    DT_RIGHT | DT_VCENTER, (std::max)(7, m_pt - 2), L"Consolas");
        }

        cv.clipPush(RECT{m_gutterW + 1, 0, w, h});

        for (int i = m_topLine; i < lastVisible; ++i)
        {
            const int y = (i - m_topLine) * m_lineH;
            const std::wstring& raw = lineRef(i);

            // 选区
            if (hasSel && i >= selA.line && i <= selB.line)
            {
                int c0 = (i == selA.line) ? selA.col : 0;
                int c1 = (i == selB.line) ? selB.col : lineLen(i);
                if (c1 > c0)
                {
                    const int x0 = textX + measureInt(raw.substr(0, static_cast<size_t>(c0)));
                    const int x1 = textX + measureInt(raw.substr(0, static_cast<size_t>(c1)));
                    cv.fillRect(RECT{x0, y, x1, y + m_lineH}, cSel());
                }
                // 跨行选区在行尾补一小段，视觉上才连成一片
                if (i < selB.line && c1 == lineLen(i))
                {
                    const int xe = textX + measureInt(raw) + 4;
                    cv.fillRect(RECT{xe - 4, y, xe + 4, y + m_lineH}, cSel());
                }
            }

            // 语法着色
            std::vector<Run> runs;
            if (m_syntax == Syntax::Python) tokenizePython(raw, runs);
            else tokenizeXml(raw, runs);

            int x = textX;
            for (const Run& r : runs)
            {
                if (x > w) break;
                cv.text(RECT{x, y, w, y + m_lineH}, r.text, r.color, DT_LEFT, m_pt, L"Consolas");
                x += measureInt(r.text);
            }

            // 光标
            if (m_focused && i == m_caret.line)
            {
                const int cx = textX + measureInt(raw.substr(0, static_cast<size_t>(m_caret.col)));
                if (m_imeComposing && !m_preedit.empty())
                {
                    cv.text(RECT{cx, y, w, y + m_lineH}, m_preedit, RGB(235, 200, 120),
                            DT_LEFT, m_pt, L"Consolas");
                    const int pw = measureInt(m_preedit);
                    cv.fillRect(RECT{cx, y + m_lineH - 3, cx + pw, y + m_lineH - 2},
                                RGB(150, 130, 90));
                }
                else if (m_caretOn)
                {
                    cv.fillRect(RECT{cx, y + 2, cx + 2, y + m_lineH - 2}, cCaret());
                }
            }
        }

        cv.clipPop();

        // 竖向滚动条：内容超出可视行数才出现，滑块位置对应视口首行。
        // 没有它时整块代码区看不出还能往下翻，是"难用"的一大来源。
        const int shown = h / m_lineH;
        if (total > shown && shown > 0)
        {
            const int sbX = w - 10;
            cv.fillRect(RECT{sbX, 0, w, h}, RGB(26, 31, 38));
            const int trackH = h;
            int thumbH = static_cast<int>(static_cast<long long>(trackH) * shown / total);
            if (thumbH < 24) thumbH = 24;
            if (thumbH > trackH) thumbH = trackH;
            const int maxTop = total - shown;
            int off = 0;
            if (maxTop > 0)
                off = static_cast<int>(static_cast<long long>(trackH - thumbH) * m_topLine / maxTop);
            if (off < 0) off = 0;
            if (off > trackH - thumbH) off = trackH - thumbH;
            cv.fillRect(RECT{sbX + 1, off, w - 1, off + thumbH}, RGB(76, 88, 104));
        }

        cv.end();
    }

    // ---------------------------------------------------------------- 语法着色
    static COLORREF cBack()      { return RGB(33, 39, 48); }
    static COLORREF cGutter()    { return RGB(26, 31, 38); }
    static COLORREF cGutterLine(){ return RGB(44, 51, 61); }
    static COLORREF cBand()      { return RGB(40, 47, 57); }
    static COLORREF cText()      { return RGB(203, 213, 224); }
    static COLORREF cPunct()     { return RGB(138, 150, 166); }
    static COLORREF cTag()       { return RGB(126, 178, 255); }
    static COLORREF cAttr()      { return RGB(156, 214, 240); }   // 属性名：浅青，与标签蓝、值绿区分清楚
    static COLORREF cValue()     { return RGB(150, 206, 150); }
    static COLORREF cComment()   { return RGB(126, 176, 108); }
    static COLORREF cKeyword()   { return RGB(86, 156, 214); }
    static COLORREF cNumber()    { return RGB(181, 206, 168); }
    static COLORREF cFunc()      { return RGB(220, 220, 170); }
    static COLORREF cNum()       { return RGB(140, 154, 172); }   // 行号：别用近黑色，深底上看不见
    static COLORREF cSel()       { return RGB(38, 79, 120); }
    static COLORREF cCaret()     { return RGB(220, 227, 236); }

    static void appendRun(std::vector<Run>& runs, const std::wstring& s, COLORREF c)
    {
        if (s.empty()) return;
        if (!runs.empty() && runs.back().color == c) runs.back().text += s;
        else runs.push_back(Run{s, c});
    }

    static void tokenizeXml(const std::wstring& line, std::vector<Run>& runs)
    {
        const size_t cmt = line.find(L"<!--");
        if (cmt != std::wstring::npos)
        {
            appendRun(runs, line.substr(0, cmt), cText());
            appendRun(runs, line.substr(cmt), cComment());
            return;
        }

        std::wstring run;
        int mode = 0;
        wchar_t quote = 0;
        auto flush = [&](COLORREF c) { appendRun(runs, run, c); run.clear(); };

        for (size_t i = 0; i < line.size(); ++i)
        {
            const wchar_t ch = line[i];
            if (mode == 0)
            {
                if (ch == L'<')
                {
                    flush(cText());
                    if (i + 1 < line.size() && line[i + 1] == L'/')
                    {
                        appendRun(runs, L"</", cPunct());
                        ++i;
                    }
                    else appendRun(runs, L"<", cPunct());
                    mode = 1;
                }
                else if (ch == L'>')
                {
                    flush(cText());
                    appendRun(runs, L">", cPunct());
                }
                else run += ch;
                continue;
            }
            if (mode == 1)
            {
                if (ch == L'>') { flush(cTag()); appendRun(runs, L">", cPunct()); mode = 0; continue; }
                if (ch == L'/') { flush(cTag()); appendRun(runs, L"/", cPunct()); mode = 0; continue; }
                if (ch == L' ' || ch == L'\t')
                {
                    flush(cTag());
                    while (i < line.size() && (line[i] == L' ' || line[i] == L'\t')) { run += line[i]; ++i; }
                    --i;
                    flush(cText());
                    mode = 2;
                    continue;
                }
                run += ch;
                continue;
            }
            if (mode == 2)
            {
                if (ch == L' ' || ch == L'\t') { run += ch; continue; }
                flush(cText());
                if (ch == L'>') { appendRun(runs, L">", cPunct()); mode = 0; continue; }
                if (ch == L'/') { appendRun(runs, L"/", cPunct()); mode = 0; continue; }
                run += ch;
                mode = 3;
                continue;
            }
            if (mode == 3)
            {
                if (ch == L'=')
                {
                    flush(cAttr());
                    appendRun(runs, L"=", cPunct());
                    mode = 4;
                    continue;
                }
                if (ch == L' ' || ch == L'\t') { flush(cAttr()); run += ch; mode = 2; continue; }
                if (ch == L'>') { flush(cAttr()); appendRun(runs, L">", cPunct()); mode = 0; continue; }
                run += ch;
                continue;
            }
            if (quote == 0)
            {
                if (ch == L'"' || ch == L'\'') { quote = ch; run += ch; }
                else if (ch == L'>') { flush(cValue()); appendRun(runs, L">", cPunct()); mode = 0; }
                else run += ch;
            }
            else
            {
                run += ch;
                if (ch == quote) { quote = 0; flush(cValue()); mode = 2; }
            }
        }
        flush(mode == 1 ? cTag() : (mode == 4 ? cValue() : (mode == 3 ? cAttr() : cText())));
    }

    static bool isPyKeyword(const std::wstring& w)
    {
        static const wchar_t* kws[] = {
            L"def", L"class", L"return", L"if", L"elif", L"else", L"for", L"while", L"import",
            L"from", L"as", L"in", L"and", L"or", L"not", L"is", L"True", L"False", L"None",
            L"try", L"except", L"finally", L"with", L"lambda", L"global", L"nonlocal",
            L"pass", L"break", L"continue", L"raise", L"yield", L"assert", L"del", L"await",
            L"async", L"self", L"print"
        };
        for (const wchar_t* k : kws) if (w == k) return true;
        return false;
    }

    static void tokenizePython(const std::wstring& line, std::vector<Run>& runs)
    {
        std::wstring run;
        auto flush = [&](COLORREF c) { appendRun(runs, run, c); run.clear(); };

        size_t i = 0;
        while (i < line.size())
        {
            const wchar_t ch = line[i];
            if (ch == L'#')
            {
                flush(cText());
                appendRun(runs, line.substr(i), cComment());
                return;
            }
            if (ch == L'"' || ch == L'\'')
            {
                const std::wstring triple(3, ch);
                const size_t open = line.compare(i, 3, triple) == 0 ? 3 : 1;
                flush(cText());
                std::wstring s = line.substr(i, open);
                i += open;
                while (i < line.size())
                {
                    s += line[i];
                    if (line[i] == L'\\' && i + 1 < line.size()) { s += line[i + 1]; i += 2; continue; }
                    if (open == 3)
                    {
                        if (line.compare(i, 2, std::wstring(2, ch)) == 0) { s += ch; s += ch; i += 2; break; }
                    }
                    else if (line[i] == ch) { ++i; break; }
                    ++i;
                }
                appendRun(runs, s, cValue());
                continue;
            }
            if (iswdigit(ch) != 0)
            {
                flush(cText());
                std::wstring n;
                while (i < line.size() &&
                       (iswdigit(line[i]) != 0 || line[i] == L'.' || line[i] == L'x' ||
                        iswalnum(line[i]) != 0))
                {
                    n += line[i];
                    ++i;
                }
                appendRun(runs, n, cNumber());
                continue;
            }
            if (iswalpha(ch) != 0 || ch == L'_')
            {
                std::wstring id;
                while (i < line.size() && (iswalnum(line[i]) != 0 || line[i] == L'_'))
                {
                    id += line[i];
                    ++i;
                }
                flush(cText());
                if (i < line.size() && line[i] == L'(')
                    appendRun(runs, id, isPyKeyword(id) ? cKeyword() : cFunc());
                else
                    appendRun(runs, id, isPyKeyword(id) ? cKeyword() : cText());
                continue;
            }
            if (ch == L'@')
            {
                flush(cText());
                std::wstring d;
                while (i < line.size() &&
                       (iswalnum(line[i]) != 0 || line[i] == L'_' || line[i] == L'.'))
                {
                    d += line[i];
                    ++i;
                }
                appendRun(runs, d, cAttr());
                continue;
            }
            if (ch == L'(' || ch == L')' || ch == L'[' || ch == L']' ||
                ch == L'{' || ch == L'}' || ch == L':' || ch == L',' ||
                ch == L'+' || ch == L'-' || ch == L'*' || ch == L'/' ||
                ch == L'=' || ch == L'<' || ch == L'>' || ch == L'.')
            {
                flush(cText());
                appendRun(runs, std::wstring(1, ch), cPunct());
                ++i;
                continue;
            }
            run += ch;
            ++i;
        }
        flush(cText());
    }

    // ---------------------------------------------------------------- 智能提示
    static void ensureTip(HWND& tip, CodeEditor* owner)
    {
        if (tip != nullptr) return;
        ensureTipClass();
        tip = ::CreateWindowExW(WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE | WS_EX_TOPMOST,
                                kTipClass, L"", WS_POPUP, 0, 0, 10, 10,
                                owner->m_hwnd, nullptr, ::GetModuleHandleW(nullptr), nullptr);
        if (tip != nullptr)
            ::SetWindowLongPtrW(tip, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(owner));
    }

    static bool prefixMatch(const std::wstring& name, const std::wstring& prefix)
    {
        if (prefix.empty()) return true;
        if (name.size() < prefix.size()) return false;
        return _wcsnicmp(name.c_str(), prefix.c_str(), prefix.size()) == 0;
    }

    // 光标前的当前词（字母数字下划线），以及是否处在未闭合标签内
    void currentWord(std::wstring& word, Pos& start, bool& insideTag, bool& afterTagOpen)
    {
        const std::wstring& s = lineRef(m_caret.line);
        int i = m_caret.col;
        while (i > 0)
        {
            const wchar_t c = s[static_cast<size_t>(i - 1)];
            if (iswalnum(c) != 0 || c == L'_' || c == L'-') --i;
            else break;
        }
        word.assign(s.begin() + i, s.begin() + m_caret.col);
        start = Pos{m_caret.line, i};

        // 全文本里最后一个 < 与 > 决定是否在标签内
        int lastOpen = -1, lastClose = -1;
        for (int ln = 0; ln <= m_caret.line; ++ln)
        {
            const std::wstring& t = lineRef(ln);
            const int end = (ln == m_caret.line) ? m_caret.col : static_cast<int>(t.size());
            for (int k = 0; k < end; ++k)
            {
                if (t[static_cast<size_t>(k)] == L'<') lastOpen = ln * 1000 + k;
                else if (t[static_cast<size_t>(k)] == L'>') lastClose = ln * 1000 + k;
            }
        }
        insideTag = lastOpen > lastClose;
        afterTagOpen = insideTag && (i == lastOpen % 1000 + 1);
    }

    void collectCandidates(const std::wstring& prefix, bool afterTagOpen, bool insideTag)
    {
        m_tipItems.clear();
        if (m_syntax == Syntax::Xml)
        {
            static const wchar_t* kTags[] = {
                L"Button", L"Label", L"TextField", L"Edit", L"ListBox", L"CheckBox",
                L"RadioButton", L"GroupBox", L"ComboBox", L"ProgressBar", L"Slider",
                L"TabBar", L"TreeView", L"ListView", L"Calendar", L"DatePicker",
                L"Layout", L"StackPanel", L"Grid", L"Canvas", L"QCanvas", L"ScrollView",
                L"Flickable", L"PageView", L"CodeEditor", L"Divider", L"Hyperlink",
                L"ToggleButton", L"Switch", L"Dial", L"Tumbler", L"Table", L"SplitView",
                L"Window"
            };
            static const wchar_t* kAttrs[] = {
                L"Name", L"Id", L"X", L"Y", L"Width", L"Height", L"Margin", L"Padding",
                L"Background", L"Foreground", L"BorderBrush", L"FontSize", L"FontWeight",
                L"HorizontalAlignment", L"VerticalAlignment", L"Text", L"Value",
                L"MinValue", L"MaxValue", L"IsReadOnly", L"Multiline", L"MaxLength",
                L"Placeholder", L"Orientation", L"Columns", L"Rows", L"Level", L"TabIndex",
                L"Theme", L"TextAlignment", L"CueBanner", L"Click", L"TextChanged",
                L"ValueChanged", L"SelectionChanged", L"Syntax", L"LineNumbers", L"Wrap"
            };
            if (afterTagOpen)
            {
                for (const wchar_t* t : kTags)
                    if (prefixMatch(t, prefix)) m_tipItems.push_back(TipItem{t});
            }
            else if (insideTag)
            {
                for (const wchar_t* a : kAttrs)
                    if (prefixMatch(a, prefix)) m_tipItems.push_back(TipItem{a});
            }
        }
        else
        {
            static const wchar_t* kPyKw[] = {
                L"def", L"class", L"return", L"if", L"elif", L"else", L"for", L"while",
                L"import", L"from", L"as", L"in", L"and", L"or", L"not", L"is", L"True",
                L"False", L"None", L"try", L"except", L"finally", L"with", L"lambda",
                L"global", L"nonlocal", L"pass", L"break", L"continue", L"raise", L"yield",
                L"assert", L"del", L"await", L"async", L"self", L"print", L"len", L"range",
                L"str", L"int", L"float", L"list", L"dict", L"set", L"tuple", L"open"
            };
            if (prefix.empty()) return;
            for (const wchar_t* k : kPyKw)
                if (prefixMatch(k, prefix)) m_tipItems.push_back(TipItem{k});

            // 文档里出现过的自定义标识符
            std::vector<std::wstring> seen;
            for (const std::wstring& ln : m_lines)
            {
                std::wstring id;
                for (size_t i = 0; i <= ln.size(); ++i)
                {
                    const wchar_t c = i < ln.size() ? ln[i] : L' ';
                    if (iswalnum(c) != 0 || c == L'_') id += c;
                    else if (!id.empty())
                    {
                        const bool known = std::find(seen.begin(), seen.end(), id) != seen.end();
                        if (!known && id.size() >= 3 && prefixMatch(id, prefix) &&
                            _wcsicmp(id.c_str(), prefix.c_str()) != 0)
                        {
                            seen.push_back(id);
                            m_tipItems.push_back(TipItem{id});
                        }
                        id.clear();
                    }
                }
            }
        }
        if (m_tipItems.size() > 8) m_tipItems.resize(8);
    }

    void updateCompletion()
    {
        if (!m_focused || m_readOnly) { closeTip(); return; }
        std::wstring word;
        Pos start{};
        bool insideTag = false, afterTagOpen = false;
        currentWord(word, start, insideTag, afterTagOpen);

        collectCandidates(word, afterTagOpen, insideTag);
        if (m_tipItems.empty() || (!insideTag && word.empty())) { closeTip(); return; }

        m_tipTokenStart = start;
        m_tipTokenLen = static_cast<int>(word.size());
        if (m_tipIdx >= static_cast<int>(m_tipItems.size())) m_tipIdx = 0;
        showTip();
    }

    void showTip()
    {
        ensureTip(m_tipHwnd, this);
        if (m_tipHwnd == nullptr) return;

        RECT cr;
        ::GetClientRect(m_hwnd, &cr);
        const int rows = static_cast<int>(m_tipItems.size());

        int wPx = 150;
        for (const TipItem& it : m_tipItems)
            wPx = (std::max)(wPx, measureInt(it.text) + 28);

        const int caretX = textOriginX() + measureInt(lineRef(m_caret.line).substr(0,
                             static_cast<size_t>(m_caret.col)));
        int cx = caretX + 4;
        int cy = (m_caret.line - m_topLine) * m_lineH + m_lineH + 2;
        if (cy + rows * kTipRowH + 6 > cr.bottom)
            cy = (std::max)(2, (m_caret.line - m_topLine) * m_lineH - rows * kTipRowH - 2);
        if (cx + wPx > cr.right) cx = (std::max)(2, static_cast<int>(cr.right) - wPx - 2);

        POINT pt{cx, cy};
        ::ClientToScreen(m_hwnd, &pt);
        const int hPx = rows * kTipRowH + 6;
        ::SetWindowPos(m_tipHwnd, HWND_TOPMOST, pt.x, pt.y, wPx, hPx,
                       SWP_NOACTIVATE | SWP_SHOWWINDOW);
        m_tipOpen = true;
        ::InvalidateRect(m_tipHwnd, nullptr, FALSE);
    }

    static LRESULT CALLBACK tipProc(HWND h, UINT msg, WPARAM wp, LPARAM lp)
    {
        CodeEditor* self = reinterpret_cast<CodeEditor*>(::GetWindowLongPtrW(h, GWLP_USERDATA));
        switch (msg)
        {
        case WM_MOUSEACTIVATE:
            return MA_NOACTIVATE;
        case WM_PAINT:
        {
            PAINTSTRUCT ps{};
            HDC hdc = ::BeginPaint(h, &ps);
            if (self != nullptr) self->paintTip(hdc);
            ::EndPaint(h, &ps);
            return 0;
        }
        case WM_ERASEBKGND:
            return 1;
        case WM_LBUTTONDOWN:
            if (self != nullptr)
            {
                const int idx = HIWORD(static_cast<DWORD>(lp)) / kTipRowH - 1;
                if (idx >= 0 && idx < static_cast<int>(self->m_tipItems.size()))
                {
                    self->m_tipIdx = idx;
                    self->acceptCompletion();
                }
            }
            return 0;
        default:
            break;
        }
        return ::DefWindowProcW(h, msg, wp, lp);
    }

    void paintTip(HDC hdc)
    {
        RECT rc;
        ::GetClientRect(m_tipHwnd, &rc);
        const int w = rc.right, ht = rc.bottom;
        if (w <= 0 || ht <= 0) return;

        if (!m_tipCv)
            m_tipCv = std::make_unique<D2DDcCanvas>();
        m_tipCv->bind(hdc, RECT{0, 0, w, ht});
        if (!m_tipCv->begin(w, ht)) return;

        XmlCanvas& cv = *m_tipCv;
        cv.fillRect(rc, RGB(24, 29, 36));
        cv.strokeRect(rc, RGB(66, 76, 90), 1);
        for (int i = 0; i < static_cast<int>(m_tipItems.size()); ++i)
        {
            const int y = 3 + i * kTipRowH;
            if (i == m_tipIdx) cv.fillRect(RECT{1, y, w - 1, y + kTipRowH}, cSel());
            cv.text(RECT{10, y, w - 6, y + kTipRowH}, m_tipItems[i].text,
                    i == m_tipIdx ? RGB(235, 241, 247) : RGB(176, 186, 198),
                    DT_LEFT | DT_VCENTER, 9, L"Consolas");
        }
        cv.end();
    }

    void tipMove(int delta)
    {
        if (!m_tipOpen || m_tipItems.empty()) return;
        const int n = static_cast<int>(m_tipItems.size());
        m_tipIdx = (m_tipIdx + delta + n) % n;
        ::InvalidateRect(m_tipHwnd, nullptr, FALSE);
    }

    void closeTip()
    {
        if (m_tipHwnd != nullptr && m_tipOpen)
            ::ShowWindow(m_tipHwnd, SW_HIDE);
        m_tipOpen = false;
        m_tipItems.clear();
        m_tipIdx = 0;
    }

    void acceptCompletion()
    {
        if (!m_tipOpen || m_tipItems.empty()) { closeTip(); return; }
        const std::wstring ins = m_tipItems[m_tipIdx].text;
        m_anchor = m_tipTokenStart;
        m_caret = Pos{m_tipTokenStart.line, m_tipTokenStart.col + m_tipTokenLen};
        closeTip();
        m_skipTipOnce = true;          // 补全插入本身不触发重新弹提示
        insertText(ins);
    }

    // ---------------------------------------------------------------- 成员
    HWND m_parent = nullptr;
    HWND m_hwnd = nullptr;
    int m_id = 0;
    HFONT m_font = nullptr;
    int m_pt = 9;
    int m_lineH = kLineH;
    int m_gutterW = kGutterW;

    std::vector<std::wstring> m_lines{L""};
    Pos m_caret{0, 0};
    Pos m_anchor{0, 0};
    int m_topLine = 0;
    int m_hScroll = 0;

    bool m_dragging = false;
    bool m_focused = false;
    bool m_caretOn = false;
    bool m_caretTimer = false;
    bool m_readOnly = false;
    Syntax m_syntax = Syntax::Xml;
    std::wstring m_lastSeen;
    std::vector<Snapshot> m_undo;
    std::vector<Snapshot> m_redo;

    std::function<void(const std::wstring&)> m_onChange;
    std::function<void()> m_onBlur;
    std::function<void()> m_onFocus;
    std::unique_ptr<D2DDcCanvas> m_cv;

    // ---- 智能提示状态 ----
    static constexpr int kTipRowH = 22;
    HWND m_tipHwnd = nullptr;
    bool m_tipOpen = false;
    bool m_skipTipOnce = false;    // 补全刚落地，跳过本次提示刷新
    int m_tipIdx = 0;
    Pos m_tipTokenStart{0, 0};
    int m_tipTokenLen = 0;
    std::vector<TipItem> m_tipItems;
    std::unique_ptr<D2DDcCanvas> m_tipCv;

    // ---- 输入法状态 ----
    bool m_imeComposing = false;
    std::wstring m_preedit;

    // ---- 按键兜底 ----
    // 记录"刚从 WM_KEYDOWN 自己转出来、已经插入"的字符：
    // 宿主若仍然送来同一个 WM_CHAR，就把它吞掉，避免一个键打出两个字符。
    std::vector<wchar_t> m_pendingChars;
};
