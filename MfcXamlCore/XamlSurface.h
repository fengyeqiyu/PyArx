// XamlSurface.h —— S1 设备层：渲染表面抽象接口 + 后端工厂。
//
// 分层约定（为后续 S2~S5 与 D3D12 升级留口）：
//   * 宿主（PyMfcXaml / 设计器 / 预览器）只依赖本头里的纯抽象 IXamlSurface；
//   * d3d11.h / dxgi.h / d2d1.h / ID2D1DeviceContext 等具体类型只出现在 MfcXamlCore
//     内部实现（XamlSurface.cpp），不透出本头；对外绘制目标以 void* 透出。
//   * 将来新增 XamlSurfaceD3D12（或 D3D11on12）只需再实现同一接口，调用方零改动。
//
// 后端职责（首个实现 XamlSurfaceD3D11）：
//   D3D11 设备 + IDXGISwapChain1（FLIP_DISCARD / BufferCount=2 / B8G8R8A8_UNORM /
//   CreateSwapChainForHwnd / AlphaMode=IGNORE）+ ID2D1DeviceContext（SetTarget 到后台
//   buffer）+ IDWriteFactory；首帧不透明 Clear；Resize 走 ResizeBuffers；
//   D2DERR_RECREATE_TARGET 与 DXGI_ERROR_DEVICE_REMOVED 均可全量重建。
#pragma once

#include <windows.h>
#include <cstddef>

#ifdef PYMFCXAML_EXPORTS
#define XAML_SURFACE_API __declspec(dllexport)
#else
#define XAML_SURFACE_API __declspec(dllimport)
#endif

// 不透明的帧绘制目标句柄：D3D11 后端内部是 ID2D1DeviceContext*，抽象层只给 void*，
// 宿主侧不 include d2d1.h。S2 起控件绘制层通过它拿 2D 目标。
struct XamlSurfaceFrame
{
    void* renderTarget = nullptr;   // 后端 2D 绘制目标（D2D DeviceContext）
    void* dwriteFactory = nullptr;  // IDWriteFactory*，同样以 void* 透出
};

class XAML_SURFACE_API IXamlSurface
{
public:
    virtual ~IXamlSurface() = default;

    // 绑定 HWND 并按初始尺寸创建设备 / 交换链 / D2D 上下文 / DWrite。
    // 失败返回 false：调用方必须回退旧 GDI 路径，不得在空表面上继续绘制。
    virtual bool Init(HWND hwnd, int width, int height) = 0;

    // 每帧开始：必要时重建目标，随后不透明清屏（不透明 Clear 是 ClearType 文本的前提）。
    // r/g/b/a 为 0~1 浮点；BeginFrame 返回 false 表示本帧无法开始（已尝试重建仍失败）。
    virtual bool BeginFrame(float r, float g, float b, float a) = 0;

    // 每帧结束：D2D EndDraw + Present。syncInterval 透传给 Present（0=立即，1=垂直同步）。
    // 返回 HRESULT 风格状态；设备丢失判定走 IsDeviceLost()，不在返回值里泄漏后端码语义。
    virtual long EndFrame(int syncInterval) = 0;

    // 尺寸变化：释放 target 引用后 ResizeBuffers，再重建后台 buffer 的 D2D 目标。
    // 失败返回 false。
    virtual bool Resize(int width, int height) = 0;

    // 设备丢失判定：DXGI_ERROR_DEVICE_REMOVED（设备被拔/重置/驱动崩溃）或
    // D2DERR_RECREATE_TARGET（D2D 目标失效）。
    virtual bool IsDeviceLost() const = 0;

    // 设备丢失重建：重取 D3D 设备、重建交换链、D2D 设备/上下文/target 与 DWrite。
    // 成功返回 true；重建失败返回 false，调用方继续回退旧路径。
    virtual bool Recreate() = 0;

    // 对外绘制目标（当前帧的 2D 目标）；抽象层不暴露具体类型。
    virtual void* GetRenderTarget() const = 0;
    virtual void* GetDWriteFactory() const = 0;

    // 诊断信息：后端名 / 特性级别 / 交换链格式 / 缓冲数 / AlphaMode / 设备描述。
    // 写入 out（UTF-16），容量 cap 含结尾 NUL；供 S1 探针与日志取证。
    virtual const wchar_t* BackendName() const = 0;
    virtual void GetDiagnostics(wchar_t* out, size_t cap) const = 0;

    // 测试钩子：模拟设备丢失（释放内部设备资源，下一帧强制全量重建）。
    // 仅用于 S1 重建路径取证，生产路径不调用。
    virtual void SimulateDeviceLost() = 0;
};

// 工厂：当前唯一后端 D3D11。返回对象由调用方 delete（抽象虚析构）。
// 失败返回 nullptr。
XAML_SURFACE_API IXamlSurface* XamlSurfaceCreateD3D11();
