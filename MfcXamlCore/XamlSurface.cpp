// XamlSurface.cpp —— IXamlSurface 的首个后端：D3D11 + DXGI SwapChain1 + D2D1.1 DC + DWrite。
//
// 本文件是 d3d11/dxgi/d2d1 类型唯一允许出现的地方；对外只经 XamlSurface.h 的抽象接口。
// 不引入 D3D12 / D3D11on12 / 自研 2D 三角化渲染器；D2D 保留为 2D 加速层。

#include "XamlSurface.h"

#include <d3d11.h>
#include <dxgi.h>
#include <dxgi1_2.h>
#include <d2d1.h>
#include <d2d1_1.h>
#include <dwrite.h>

#include <cstdio>
#include <cstring>
#include <new>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")

namespace
{
constexpr DXGI_FORMAT kBackBufferFormat = DXGI_FORMAT_B8G8R8A8_UNORM;
constexpr UINT kBufferCount = 2;

D3D_FEATURE_LEVEL featureLevels[] = {
    D3D_FEATURE_LEVEL_11_1, D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_1,
    D3D_FEATURE_LEVEL_10_0,  D3D_FEATURE_LEVEL_9_3,  D3D_FEATURE_LEVEL_9_2,
    D3D_FEATURE_LEVEL_9_1,
};
}

class XamlSurfaceD3D11 final : public IXamlSurface
{
public:
    XamlSurfaceD3D11() = default;
    ~XamlSurfaceD3D11() override { releaseAll(); }

    // ---- IXamlSurface -----------------------------------------------------

    bool Init(HWND hwnd, int width, int height) override
    {
        releaseAll();
        m_hwnd = hwnd;
        m_w = width > 0 ? width : 1;
        m_h = height > 0 ? height : 1;

        if (!createDevice()) return false;
        if (!createSwapChain()) return false;
        if (!createD2D()) return false;
        if (!rebuildTarget()) return false;

        // DWrite 共享工厂（进程级可复用；这里随表面持有，重建路径一并刷新）
        if (FAILED(::DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED,
                                          __uuidof(IDWriteFactory),
                                          reinterpret_cast<IUnknown**>(&m_dwrite))))
            m_dwrite = nullptr;

        m_lost = false;
        m_lastRemoveReason = S_OK;
        return true;
    }

    bool BeginFrame(float r, float g, float b, float a) override
    {
        if (m_lost)
        {
            if (!Recreate()) return false;
        }
        if (m_d2dCtx == nullptr) return false;
        if (m_targetBitmap == nullptr && !rebuildTarget()) return false;

        m_d2dCtx->BeginDraw();
        const D2D1_COLOR_F clear = { r, g, b, a };
        // 不透明清屏：ClearType 文本要求目标底层不透明，首帧与每帧都按不透明 Clear。
        m_d2dCtx->Clear(clear);
        return true;
    }

    long EndFrame(int syncInterval) override
    {
        if (m_d2dCtx == nullptr) return (long)E_POINTER;

        HRESULT hr = m_d2dCtx->EndDraw();
        m_lastD2dHr = hr;

        if (hr == D2DERR_RECREATE_TARGET)
        {
            m_lost = true;
        }
        else
        {
            const HRESULT dr = m_device != nullptr ? m_device->GetDeviceRemovedReason()
                                                   : DXGI_ERROR_DEVICE_REMOVED;
            m_lastRemoveReason = dr;
            if (dr != S_OK) m_lost = true;
        }

        if (m_swap != nullptr)
            m_swap->Present(syncInterval, 0);
        return (long)hr;
    }

    bool Resize(int width, int height) override
    {
        if (m_swap == nullptr) return false;
        const int w = width > 0 ? width : 1;
        const int h = height > 0 ? height : 1;

        if (m_d2dCtx != nullptr) m_d2dCtx->SetTarget(nullptr);
        releaseTarget();

        HRESULT hr = m_swap->ResizeBuffers(kBufferCount, w, h, kBackBufferFormat, 0);
        if (FAILED(hr))
        {
            m_lastRemoveReason = hr;
            return false;
        }
        m_w = w;
        m_h = h;
        return rebuildTarget();
    }

    bool IsDeviceLost() const override { return m_lost; }

    bool Recreate() override
    {
        releaseAll();
        m_lost = false;
        return Init(m_hwnd, m_w, m_h);
    }

    void* GetRenderTarget() const override { return m_d2dCtx; }
    void* GetDWriteFactory() const override { return m_dwrite; }

    const wchar_t* BackendName() const override { return L"D3D11"; }

    void GetDiagnostics(wchar_t* out, size_t cap) const override
    {
        if (out == nullptr || cap == 0) return;
        out[0] = L'\0';

        wchar_t adapterDesc[128] = L"(unknown)";
        if (m_adapter != nullptr)
        {
            DXGI_ADAPTER_DESC desc = {};
            if (SUCCEEDED(m_adapter->GetDesc(&desc)))
                ::wcsncpy_s(adapterDesc, _countof(adapterDesc), desc.Description,
                            _TRUNCATE);
        }

        const wchar_t* fl = L"?";
        switch (m_featureLevel)
        {
        case D3D_FEATURE_LEVEL_11_1: fl = L"11_1"; break;
        case D3D_FEATURE_LEVEL_11_0: fl = L"11_0"; break;
        case D3D_FEATURE_LEVEL_10_1: fl = L"10_1"; break;
        case D3D_FEATURE_LEVEL_10_0: fl = L"10_0"; break;
        case D3D_FEATURE_LEVEL_9_3: fl = L"9_3"; break;
        case D3D_FEATURE_LEVEL_9_2: fl = L"9_2"; break;
        case D3D_FEATURE_LEVEL_9_1: fl = L"9_1"; break;
        default: break;
        }

        const wchar_t* driver = m_driverIsWarp ? L"WARP" : L"HARDWARE";
        swprintf_s(out, cap,
                   L"backend=D3D11 driver=%ls FL=%ls size=%dx%d buffers=%u format=B8G8R8A8_UNORM "
                   L"swapEffect=FLIP_DISCARD alpha=IGNORE adapter=%ls removedReason=0x%08X d2dHr=0x%08X",
                   driver, fl, m_w, m_h, kBufferCount, adapterDesc,
                   static_cast<unsigned>(m_lastRemoveReason),
                   static_cast<unsigned>(m_lastD2dHr));
    }

    void SimulateDeviceLost() override
    {
        // 测试钩子：释放全部后端资源并标记丢失，下一帧 BeginFrame 走 Recreate 全量重建。
        releaseAll();
        m_lost = true;
        m_lastRemoveReason = DXGI_ERROR_DEVICE_REMOVED;
    }

private:
    // ---- 内部 -------------------------------------------------------------

    void releaseTarget()
    {
        if (m_targetBitmap != nullptr) { m_targetBitmap->Release(); m_targetBitmap = nullptr; }
    }

    void releaseAll()
    {
        releaseTarget();
        if (m_d2dCtx != nullptr) { m_d2dCtx->Release(); m_d2dCtx = nullptr; }
        if (m_d2dDevice != nullptr) { m_d2dDevice->Release(); m_d2dDevice = nullptr; }
        if (m_dwrite != nullptr) { m_dwrite->Release(); m_dwrite = nullptr; }
        if (m_swap != nullptr) { m_swap->Release(); m_swap = nullptr; }
        if (m_factory != nullptr) { m_factory->Release(); m_factory = nullptr; }
        if (m_adapter != nullptr) { m_adapter->Release(); m_adapter = nullptr; }
        if (m_dxgiDevice != nullptr) { m_dxgiDevice->Release(); m_dxgiDevice = nullptr; }
        if (m_immediate != nullptr) { m_immediate->Release(); m_immediate = nullptr; }
        if (m_device != nullptr) { m_device->Release(); m_device = nullptr; }
    }

    bool createDevice()
    {
        const UINT flags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;

        HRESULT hr = D3D11CreateDevice(
            nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, flags, featureLevels,
            _countof(featureLevels), D3D11_SDK_VERSION, &m_device, &m_featureLevel,
            &m_immediate);
        m_driverIsWarp = false;

        if (FAILED(hr) || m_device == nullptr)
        {
            if (m_device != nullptr) { m_device->Release(); m_device = nullptr; }
            if (m_immediate != nullptr) { m_immediate->Release(); m_immediate = nullptr; }
            hr = D3D11CreateDevice(
                nullptr, D3D_DRIVER_TYPE_WARP, nullptr, flags, featureLevels,
                _countof(featureLevels), D3D11_SDK_VERSION, &m_device, &m_featureLevel,
                &m_immediate);
            m_driverIsWarp = true;
            if (FAILED(hr) || m_device == nullptr)
            {
                releaseAll();
                return false;
            }
        }

        if (FAILED(m_device->QueryInterface(__uuidof(IDXGIDevice1),
                                           reinterpret_cast<void**>(&m_dxgiDevice))))
        {
            m_dxgiDevice = nullptr;
            releaseAll();
            return false;
        }
        if (FAILED(m_dxgiDevice->GetAdapter(&m_adapter)))
        {
            m_adapter = nullptr;
            releaseAll();
            return false;
        }
        if (FAILED(m_adapter->GetParent(__uuidof(IDXGIFactory2),
                                        reinterpret_cast<void**>(&m_factory))))
        {
            m_factory = nullptr;
            releaseAll();
            return false;
        }
        return true;
    }

    bool createSwapChain()
    {
        DXGI_SWAP_CHAIN_DESC1 desc = {};
        desc.BufferCount = kBufferCount;
        desc.Width = static_cast<UINT>(m_w);
        desc.Height = static_cast<UINT>(m_h);
        desc.Format = kBackBufferFormat;
        desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        desc.SampleDesc.Count = 1;
        desc.SampleDesc.Quality = 0;
        desc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
        desc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

        HRESULT hr = m_factory->CreateSwapChainForHwnd(
            m_device, m_hwnd, &desc, nullptr, nullptr, &m_swap);
        if (FAILED(hr) || m_swap == nullptr)
        {
            if (m_swap != nullptr) { m_swap->Release(); m_swap = nullptr; }
            return false;
        }
        return true;
    }

    bool createD2D()
    {
        D2D1_CREATION_PROPERTIES props = {};
        props.threadingMode = D2D1_THREADING_MODE_SINGLE_THREADED;
        props.debugLevel = D2D1_DEBUG_LEVEL_NONE;
        props.options = D2D1_DEVICE_CONTEXT_OPTIONS_NONE;

        HRESULT hr = D2D1CreateDevice(m_dxgiDevice, &props, &m_d2dDevice);
        if (FAILED(hr) || m_d2dDevice == nullptr)
        {
            m_d2dDevice = nullptr;
            return false;
        }
        hr = m_d2dDevice->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE, &m_d2dCtx);
        if (FAILED(hr) || m_d2dCtx == nullptr)
        {
            m_d2dCtx = nullptr;
            return false;
        }
        m_d2dCtx->SetUnitMode(D2D1_UNIT_MODE_PIXELS);
        m_d2dCtx->SetDpi(96.0f, 96.0f);
        return true;
    }

    bool rebuildTarget()
    {
        if (m_swap == nullptr || m_d2dCtx == nullptr) return false;

        ID3D11Texture2D* back = nullptr;
        HRESULT hr = m_swap->GetBuffer(0, __uuidof(ID3D11Texture2D),
                                       reinterpret_cast<void**>(&back));
        if (FAILED(hr) || back == nullptr)
        {
            if (back != nullptr) back->Release();
            return false;
        }
        IDXGISurface* dxgiSurface = nullptr;
        back->QueryInterface(__uuidof(IDXGISurface), reinterpret_cast<void**>(&dxgiSurface));
        back->Release();
        if (dxgiSurface == nullptr) return false;

        D2D1_BITMAP_PROPERTIES1 bp = {};
        bp.pixelFormat.format = kBackBufferFormat;
        bp.pixelFormat.alphaMode = D2D1_ALPHA_MODE_IGNORE;
        bp.bitmapOptions = D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW;

        hr = m_d2dCtx->CreateBitmapFromDxgiSurface(dxgiSurface, &bp, &m_targetBitmap);
        dxgiSurface->Release();
        if (FAILED(hr) || m_targetBitmap == nullptr)
        {
            if (m_targetBitmap != nullptr) { m_targetBitmap->Release(); m_targetBitmap = nullptr; }
            return false;
        }
        m_d2dCtx->SetTarget(m_targetBitmap);
        return true;
    }

private:
    HWND m_hwnd = nullptr;
    int m_w = 1, m_h = 1;
    bool m_lost = false;
    bool m_driverIsWarp = false;
    HRESULT m_lastRemoveReason = S_OK;
    HRESULT m_lastD2dHr = S_OK;
    D3D_FEATURE_LEVEL m_featureLevel = D3D_FEATURE_LEVEL_11_0;

    ID3D11Device* m_device = nullptr;
    ID3D11DeviceContext* m_immediate = nullptr;
    IDXGIDevice1* m_dxgiDevice = nullptr;
    IDXGIAdapter* m_adapter = nullptr;
    IDXGIFactory2* m_factory = nullptr;
    IDXGISwapChain1* m_swap = nullptr;

    ID2D1Device* m_d2dDevice = nullptr;
    ID2D1DeviceContext* m_d2dCtx = nullptr;
    ID2D1Bitmap1* m_targetBitmap = nullptr;
    IDWriteFactory* m_dwrite = nullptr;
};

XAML_SURFACE_API IXamlSurface* XamlSurfaceCreateD3D11()
{
    return new (std::nothrow) XamlSurfaceD3D11();
}
