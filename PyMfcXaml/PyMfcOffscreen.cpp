// PyMfcOffscreen.cpp —— 离屏渲染：BGRA→PNG(WIC) / D2D 配电图 / nanosvg SVG。
// 独立编译单元：不依赖 PyMfcHost，只依赖 PyMfcCommon.h 与 MfcXamlCore。

#include "PyMfcCommon.h"

// ---------------------------------------------------------------------------
// 验收取证：把一块 BGRA 像素写成 PNG（WIC 编码器，不引入第三方库）
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// 配电规划图离屏渲染：Direct2D（DCRenderTarget）+ DirectWrite 画到内存 DIB，再编码 PNG。
// 不建窗口、不依赖控件树——纯粹量"mfcxaml 这套 D2D/DWrite 渲染栈"画出来是什么质量，
// 可与 HTML / QML 版同尺寸直接对比。
// 1 DIP = 1 px（渲染目标固定 96 DPI）；scale 放大目标位图并按比例放大字号，
// 得到的是"同一构图的高分辨率版"，用来判断字符轮廓和曲线是否真的锐利。
// ---------------------------------------------------------------------------
bool RenderPowerPlanPng(const std::wstring& path, const xamlplan::PowerPlan& plan, int scale)
{
    if (scale < 1) scale = 1;

    // WIC 编码器是 COM 组件：Python 进程默认没有 CoInitialize，直接 CoCreateInstance
    // 会返回 CO_E_NOTINITIALIZED（表现为"写 PNG 失败"）。这里补一次，幂等；若宿主已经
    // 初始化成别的 apartment 模式，返回值忽略即可（WIC 两种模式都可用）。
    static bool comReady = false;
    if (!comReady)
    {
        ::CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
        comReady = true;
    }

    const int W = plan.width * scale;
    const int H = plan.height * scale;
    if (W <= 0 || H <= 0) return false;

    HDC screen = ::GetDC(nullptr);
    if (screen == nullptr) return false;

    BITMAPINFO bi = {};
    bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bi.bmiHeader.biWidth = W;
    bi.bmiHeader.biHeight = -H;   // 顶朝下，WIC 直接按 BGRA 顶行起始编码
    bi.bmiHeader.biPlanes = 1;
    bi.bmiHeader.biBitCount = 32;
    bi.bmiHeader.biCompression = BI_RGB;

    void* bits = nullptr;
    HBITMAP dib = ::CreateDIBSection(screen, &bi, DIB_RGB_COLORS, &bits, nullptr, 0);
    HDC mem = ::CreateCompatibleDC(screen);
    bool ok = false;

    if (dib != nullptr && mem != nullptr && bits != nullptr)
    {
        HGDIOBJ old = ::SelectObject(mem, dib);

        D2DDcCanvas canvas;
        RECT bindRc = { 0, 0, W, H };
        canvas.bind(mem, bindRc);
        const bool began = canvas.begin(W, H);
        debugLogf(L"[powerplan] render %dx%d scale=%d begin=%d hw=%d hr=0x%08X",
                  W, H, scale, began ? 1 : 0, canvas.isHardware() ? 1 : 0,
                  static_cast<unsigned>(canvas.lastError()));
        if (began)
        {
            ::drawPowerPlanContent(canvas, plan, static_cast<float>(scale), 0, 0,
                                   std::wstring());

            canvas.end();

            // D2D 画进内存 DIB 后需要一次 GdiFlush，等到绘制指令真正落到像素上
            ::GdiFlush();
            ok = wicWritePng(path, W, H, bits);
            debugLogf(L"[powerplan] wic png=%d path=%ls", ok ? 1 : 0, path.c_str());
        }

        ::SelectObject(mem, old);
    }

    if (mem != nullptr) ::DeleteDC(mem);
    if (dib != nullptr) ::DeleteObject(dib);
    ::ReleaseDC(nullptr, screen);
    return ok;
}

// ---------------------------------------------------------------------------
// SVG 离屏渲染：nanosvg 解析几何 → 本栈 XamlCanvas 画 → WIC 出 PNG。
// 与配电图那条路径共用同一套 D2D/DWrite 后端和同一张内存 DIB，所以画质口径一致，
// 可以直接把"导入的 SVG"和"生成的配电图"放在一起比较。
// ---------------------------------------------------------------------------
bool RenderSvgPng(const std::wstring& svgPath, const std::wstring& pngPath, int scale,
                  COLORREF background, int& outW, int& outH)
{
    outW = outH = 0;
    if (scale < 1) scale = 1;

    static bool comReady2 = false;
    if (!comReady2)
    {
        ::CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
        comReady2 = true;
    }

    xamlsvg::SvgScene scene;
    std::wstring err;
    if (!xamlsvg::loadSvgFile(svgPath, scene, err))
    {
        debugLogf(L"[svg] parse fail: %ls", err.c_str());
        return false;
    }

    const int W = static_cast<int>(scene.width * scale + 0.5f);
    const int H = static_cast<int>(scene.height * scale + 0.5f);
    outW = W;
    outH = H;
    debugLogf(L"[svg] parsed shapes=%d paths=%d size=%.1fx%.1f scale=%d",
              static_cast<int>(scene.shapes.size()), scene.pathCount,
              scene.width, scene.height, scale);

    HDC screen = ::GetDC(nullptr);
    if (screen == nullptr) return false;

    BITMAPINFO bi = {};
    bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bi.bmiHeader.biWidth = W;
    bi.bmiHeader.biHeight = -H;
    bi.bmiHeader.biPlanes = 1;
    bi.bmiHeader.biBitCount = 32;
    bi.bmiHeader.biCompression = BI_RGB;

    void* bits = nullptr;
    HBITMAP dib = ::CreateDIBSection(screen, &bi, DIB_RGB_COLORS, &bits, nullptr, 0);
    HDC mem = ::CreateCompatibleDC(screen);
    bool ok = false;

    if (dib != nullptr && mem != nullptr && bits != nullptr)
    {
        HGDIOBJ old = ::SelectObject(mem, dib);

        D2DDcCanvas canvas;
        RECT bindRc = { 0, 0, W, H };
        canvas.bind(mem, bindRc);
        if (canvas.begin(W, H))
        {
            canvas.fillRect(RECT{ 0, 0, W, H }, background);
            xamlsvg::drawSvgScene(canvas, scene, static_cast<float>(scale), 0, 0);
            canvas.end();

            ::GdiFlush();
            ok = wicWritePng(pngPath, W, H, bits);
            debugLogf(L"[svg] png=%d path=%ls", ok ? 1 : 0, pngPath.c_str());
        }
        else
        {
            debugLogf(L"[svg] d2d begin failed hr=0x%08X",
                      static_cast<unsigned>(canvas.lastError()));
        }

        ::SelectObject(mem, old);
    }

    if (mem != nullptr) ::DeleteDC(mem);
    if (dib != nullptr) ::DeleteObject(dib);
    ::ReleaseDC(nullptr, screen);
    return ok;
}

bool wicWritePng(const std::wstring& path, int w, int h, void* bgra)
{
    if (bgra == nullptr || w <= 0 || h <= 0)
        return false;

    IWICImagingFactory* factory = nullptr;
    HRESULT hr = ::CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER,
                                    IID_PPV_ARGS(&factory));
    if (FAILED(hr) || factory == nullptr)
        return false;

    IStream* stream = nullptr;
    IWICBitmapEncoder* enc = nullptr;
    IWICBitmapFrameEncode* frame = nullptr;
    IWICBitmap* bmp = nullptr;
    IPropertyBag2* props = nullptr;
    WICPixelFormatGUID fmt = GUID_WICPixelFormat32bppBGRA;
    const UINT stride = static_cast<UINT>(w) * 4;
    const UINT size = stride * static_cast<UINT>(h);
    bool ok = false;

    if (SUCCEEDED(factory->CreateBitmapFromMemory(static_cast<UINT>(w), static_cast<UINT>(h),
                                                  GUID_WICPixelFormat32bppBGRA, stride, size,
                                                  static_cast<BYTE*>(bgra), &bmp)) &&
        SUCCEEDED(::SHCreateStreamOnFileW(path.c_str(), STGM_CREATE | STGM_WRITE, &stream)) &&
        SUCCEEDED(factory->CreateEncoder(GUID_ContainerFormatPng, nullptr, &enc)) &&
        SUCCEEDED(enc->Initialize(stream, WICBitmapEncoderNoCache)) &&
        SUCCEEDED(enc->CreateNewFrame(&frame, &props)) &&
        SUCCEEDED(frame->Initialize(props)) &&
        SUCCEEDED(frame->SetSize(static_cast<UINT>(w), static_cast<UINT>(h))) &&
        SUCCEEDED(frame->SetPixelFormat(&fmt)) &&
        SUCCEEDED(frame->WriteSource(bmp, nullptr)) &&
        SUCCEEDED(frame->Commit()) &&
        SUCCEEDED(enc->Commit()))
        ok = true;

    if (props) props->Release();
    if (frame) frame->Release();
    if (enc) enc->Release();
    if (bmp) bmp->Release();
    if (stream) stream->Release();
    factory->Release();
    return ok;
}


