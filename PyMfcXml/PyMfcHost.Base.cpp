// PyMfcHost.Base.cpp —— 公共前置、文件级辅助与静态回调（拆分自 PyMfcHost.cpp）。
#include "PyMfcHostInternal.h"

bool PyMfcHoverMark(HWND h)
{
    if (::GetPropW(h, kPyMfcHoverProp) != nullptr)
        return false;
    ::SetPropW(h, kPyMfcHoverProp, reinterpret_cast<HANDLE>(1));
    return true;
}

bool PyMfcHoverUnmark(HWND h)
{
    if (::GetPropW(h, kPyMfcHoverProp) == nullptr)
        return false;
    ::RemovePropW(h, kPyMfcHoverProp);
    return true;
}

bool PyMfcHovered(HWND h) { return ::GetPropW(h, kPyMfcHoverProp) != nullptr; }

PyMfcPerfCounters& pyMfcPerf()
{
    static PyMfcPerfCounters c;
    return c;
}

std::map<HWND, DragBarDrag>& dragBarState()
{
    static std::map<HWND, DragBarDrag> s_map;
    return s_map;
}

int DragBarTargetLeft(HWND hwnd, LPARAM lParam, const DragBarDrag& st)
{
    POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
    ::ClientToScreen(hwnd, &pt);
    int left = st.downLeft + (pt.x - st.downScreen.x);
    const int maxLeft = st.parentW - st.barW;
    if (left < 0)
        left = 0;
    if (left > maxLeft)
        left = maxLeft;
    return left;
}

bool pyMfcImagePixelSize(const std::wstring& path, int& outW, int& outH)
{
    outW = outH = 0;
    if (path.empty())
        return false;
    {
        static bool once = false;
        if (!once)
        {
            once = true;
            ::CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
        }
    }
    IWICImagingFactory* factory = nullptr;
    HRESULT hr = ::CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER,
                                    IID_PPV_ARGS(&factory));
    if (FAILED(hr) || factory == nullptr)
        return false;
    bool ok = false;
    IWICBitmapDecoder* decoder = nullptr;
    IWICBitmapFrameDecode* frame = nullptr;
    if (SUCCEEDED(factory->CreateDecoderFromFilename(path.c_str(), nullptr, GENERIC_READ,
                                                     WICDecodeMetadataCacheOnDemand, &decoder)) &&
        decoder != nullptr && SUCCEEDED(decoder->GetFrame(0, &frame)) && frame != nullptr)
    {
        UINT w = 0, h = 0;
        if (SUCCEEDED(frame->GetSize(&w, &h)) && w > 0 && h > 0)
        {
            outW = static_cast<int>(w);
            outH = static_cast<int>(h);
            ok = true;
        }
    }
    if (frame) frame->Release();
    if (decoder) decoder->Release();
    factory->Release();
    return ok;
}

LRESULT CALLBACK PyMfcEventSubProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    PyMfcHost* host = reinterpret_cast<PyMfcHost*>(::GetPropW(hwnd, PyMfcHost::kEvtHostProp));
    if (host != nullptr)
    {
        const int id = static_cast<int>(::GetDlgCtrlID(hwnd));
        switch (msg)
        {
        case WM_LBUTTONDOWN: host->fireControlEvent(id, L"mousedown"); break;
        case WM_LBUTTONUP:   host->fireControlEvent(id, L"mouseup");   break;
        case WM_MOUSEMOVE:
            host->fireControlEvent(id, L"mousemove");
            // CSS :hover：进入只在状态翻转那一刻重绘一次（WM_MOUSEMOVE 高频不刷帧）
            if (PyMfcHoverMark(hwnd))
            {
                // CSS transition：hover 切换若有过渡声明，就从当前色插值到目标色
                host->startHoverTransition(id, true);
                ::InvalidateRect(hwnd, nullptr, FALSE);
            }
            break;
        case WM_MOUSELEAVE:
            if (PyMfcHoverUnmark(hwnd))
            {
                host->startHoverTransition(id, false);
                ::InvalidateRect(hwnd, nullptr, FALSE);
            }
            break;
        case WM_KEYDOWN:     host->fireControlEvent(id, L"keydown");   break;
        case WM_KEYUP:       host->fireControlEvent(id, L"keyup");     break;
        case WM_SETFOCUS:    host->fireControlEvent(id, L"gotfocus");  break;
        case WM_KILLFOCUS:   host->fireControlEvent(id, L"lostfocus"); break;
        default: break;
        }
    }
    WNDPROC prev = reinterpret_cast<WNDPROC>(::GetPropW(hwnd, PyMfcHost::kEvtPrevProcProp));
    if (prev != nullptr)
        return ::CallWindowProcW(prev, hwnd, msg, wp, lp);
    return ::DefWindowProcW(hwnd, msg, wp, lp);
}

    std::wstring hostClipboardText()
    {
        std::wstring out;
        if (!::OpenClipboard(nullptr)) return out;
        if (HANDLE h = ::GetClipboardData(CF_UNICODETEXT))
        {
            if (const wchar_t* p = static_cast<const wchar_t*>(::GlobalLock(h)))
            {
                out = p;
                ::GlobalUnlock(h);
            }
        }
        ::CloseClipboard();
        return out;
    }

std::wstring MonthCalDateText(const SYSTEMTIME& st)
{
        wchar_t buf[32];
        _snwprintf_s(buf, _countof(buf), _TRUNCATE, L"%04u-%02u-%02u", st.wYear, st.wMonth, st.wDay);
        return std::wstring(buf);
    }

HCURSOR cursorOfCss(const std::wstring& k)
{
    std::wstring s;
    s.reserve(k.size());
    for (wchar_t c : k) s.push_back(static_cast<wchar_t>(towlower(c)));
    auto sys = [](LPCWSTR id) { return ::LoadCursorW(nullptr, id); };
    if (s == L"pointer" || s == L"hand" || s == L"grab") return sys(IDC_HAND);
    if (s == L"default") return sys(IDC_ARROW);
    if (s == L"text" || s == L"ibeam") return sys(IDC_IBEAM);
    if (s == L"wait" || s == L"progress") return sys(IDC_WAIT);
    if (s == L"help") return sys(IDC_HELP);
    if (s == L"crosshair") return sys(IDC_CROSS);
    if (s == L"move" || s == L"all-scroll") return sys(IDC_SIZEALL);
    if (s == L"not-allowed" || s == L"no-drop") return sys(IDC_NO);
    if (s == L"col-resize" || s == L"e-resize" || s == L"w-resize") return sys(IDC_SIZEWE);
    if (s == L"row-resize" || s == L"n-resize" || s == L"s-resize") return sys(IDC_SIZENS);
    if (s == L"nwse-resize" || s == L"se-resize" || s == L"nw-resize") return sys(IDC_SIZENWSE);
    if (s == L"nesw-resize" || s == L"ne-resize" || s == L"sw-resize") return sys(IDC_SIZENESW);
    return nullptr;   // auto / 未知关键字：不接管
}

std::string wideToUtf8Text(const std::wstring& src)
{
    if (src.empty())
        return std::string();
    const int n = ::WideCharToMultiByte(CP_UTF8, 0, src.c_str(),
                                        static_cast<int>(src.size()), nullptr, 0, nullptr, nullptr);
    if (n <= 0)
        return std::string();
    std::string out(static_cast<size_t>(n), '\0');
    ::WideCharToMultiByte(CP_UTF8, 0, src.c_str(), static_cast<int>(src.size()),
                          out.data(), n, nullptr, nullptr);
    return out;
}

COLORREF v3Shade(COLORREF c, float k)
{
    if (k < 0.0f) k = 0.0f;
    if (k > 1.6f) k = 1.6f;
    int r = static_cast<int>(GetRValue(c) * k);
    int g = static_cast<int>(GetGValue(c) * k);
    int b = static_cast<int>(GetBValue(c) * k);
    return RGB(r > 255 ? 255 : r, g > 255 ? 255 : g, b > 255 ? 255 : b);
}

bool v3ParseColor(const std::wstring& s, COLORREF& out)
{
    if (s.size() != 6) return false;
    unsigned v = 0;
    for (wchar_t ch : s)
    {
        int d;
        if (ch >= L'0' && ch <= L'9') d = ch - L'0';
        else if (ch >= L'a' && ch <= L'f') d = ch - L'a' + 10;
        else if (ch >= L'A' && ch <= L'F') d = ch - L'A' + 10;
        else return false;
        v = v * 16 + static_cast<unsigned>(d);
    }
    out = RGB((v >> 16) & 0xFF, (v >> 8) & 0xFF, v & 0xFF);
    return true;
}

int borderLinePixelCode(int depth, int along, int th, const wchar_t* style)
{
    if (th <= 0 || depth < 0 || depth >= th)
        return 0;
    if (_wcsicmp(style, L"Double") == 0)
    {
        if (th < 3) return 1;
        const int t1 = (std::max)(1, th / 3);
        return (depth < t1 || depth >= th - t1) ? 1 : 0;
    }
    if (_wcsicmp(style, L"Dashed") == 0)
    {
        const int dash = (std::max)(1, th * 3);
        return (along % (dash * 2)) < dash ? 1 : 0;
    }
    if (_wcsicmp(style, L"Dotted") == 0)
    {
        const int dot = (std::max)(1, th);
        return (along % (dot * 2)) < dot ? 1 : 0;
    }
    if (_wcsicmp(style, L"Groove") == 0 || _wcsicmp(style, L"Inset") == 0 ||
        _wcsicmp(style, L"Sunken") == 0)
        return depth < (th + 1) / 2 ? 3 : 2;
    if (_wcsicmp(style, L"Ridge") == 0 || _wcsicmp(style, L"Outset") == 0 ||
        _wcsicmp(style, L"Raised") == 0)
        return depth < (th + 1) / 2 ? 2 : 3;
    return 1;
}

int borderPixelCodeCorners(int x, int y, int w, int h,
                                         int tl, int tr, int br, int bl, int th,
                                         const wchar_t* style)
{
    if (th <= 0 || w <= 0 || h <= 0)
        return 0;
    const int dTop = y, dBot = h - 1 - y, dLeft = x, dRight = w - 1 - x;
    int depth = dTop, edge = 0;
    if (dLeft < depth)  { depth = dLeft;  edge = 3; }
    if (dRight < depth) { depth = dRight; edge = 1; }
    if (dBot < depth)   { depth = dBot;   edge = 2; }
    int along = (edge == 0 || edge == 2) ? x : y;

    int rad = 0, cx = 0, cy = 0;
    if (tl > 0 && x < tl && y < tl)
    { rad = tl; cx = tl - x; cy = tl - y; }
    else if (tr > 0 && x >= w - tr && y < tr)
    { rad = tr; cx = x - (w - 1 - tr); cy = tr - y; }
    else if (br > 0 && x >= w - br && y >= h - br)
    { rad = br; cx = x - (w - 1 - br); cy = y - (h - 1 - br); }
    else if (bl > 0 && x < bl && y >= h - bl)
    { rad = bl; cx = bl - x; cy = y - (h - 1 - bl); }
    if (rad > 0 && cx != 0 && cy != 0)
    {
        const int d2 = cx * cx + cy * cy;
        if (d2 > rad * rad)
            return -1;
        depth = static_cast<int>(static_cast<double>(rad) -
                                 std::sqrt(static_cast<double>(d2)));
        along = x + y;
    }
    if (depth >= th)
        return 0;
    return borderLinePixelCode(depth, along, th, style);
}

int borderPixelCode(int x, int y, int w, int h, int rad, int th,
                                  const wchar_t* style)
{
    if (th <= 0 || w <= 0 || h <= 0)
        return 0;
    const int dTop = y, dBot = h - 1 - y, dLeft = x, dRight = w - 1 - x;
    int depth = dTop, edge = 0;                     // 0 上 1 右 2 下 3 左
    if (dLeft < depth)  { depth = dLeft;  edge = 3; }
    if (dRight < depth) { depth = dRight; edge = 1; }
    if (dBot < depth)   { depth = dBot;   edge = 2; }
    int along = (edge == 0 || edge == 2) ? x : y;   // 沿边的位置（虚线分段用）

    if (rad > 0)
    {
        const int cx = (x < rad) ? rad - x : (x > w - 1 - rad ? x - (w - 1 - rad) : 0);
        const int cy = (y < rad) ? rad - y : (y > h - 1 - rad ? y - (h - 1 - rad) : 0);
        if (cx != 0 && cy != 0)
        {
            // 圆角段：超出圆弧的像素在 CSS 里根本不属于这个盒子，必须露出页面底色。
            const int d2 = cx * cx + cy * cy;
            if (d2 > rad * rad)
                return -1;
            const double dist = std::sqrt(static_cast<double>(d2));
            depth = static_cast<int>(static_cast<double>(rad) - dist);
            along = x + y;                          // 弧长用切向坐标近似
        }
    }

    if (depth >= th)
        return 0;
    return borderLinePixelCode(depth, along, th, style);
}

std::unordered_map<int, std::pair<HWND, HWND>>& idCacheStore()
{
    static std::unordered_map<int, std::pair<HWND, HWND>> s;
    return s;
}

