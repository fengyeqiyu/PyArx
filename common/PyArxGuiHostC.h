#pragma once

//-----------------------------------------------------------------------------
// PyArxGuiHostC.h — 常驻 PyArxGuiHost.dll 的 C 接口。
//
// PyArxGuiHost.dll 持有 wxApp + MFC/adui palette，永不卸载；
// PyArxGui.pyd 只做 pybind11 绑定并通过这些函数调用 Host，因此 pyd 可以
// 卸载 / 覆盖 / 热替换。
// 编码：UTF-8 带 BOM。
//-----------------------------------------------------------------------------

#include <Python.h>

#ifdef PYARXGUIHOST_EXPORTS
#define PYARXGUIHOST_API __declspec(dllexport)
#else
#define PYARXGUIHOST_API
#endif

#ifdef __cplusplus
extern "C" {
#endif

// 初始化 Host（创建 wxApp、接管 wxPython）；GUI 宿主不可用时返回 0。
PYARXGUIHOST_API int  PyArxGuiHostInitialize(void);
PYARXGUIHOST_API int  PyArxGuiHostIsReady(void);
PYARXGUIHOST_API const char* PyArxGuiHostVersion(void);

// 侧栏生命周期
PYARXGUIHOST_API void* PyArxGuiHostCreatePaletteSet(const char* name,
                                                    const char* guid,
                                                    int width,
                                                    int height);
PYARXGUIHOST_API void  PyArxGuiHostDestroyPaletteSet(void* set);
PYARXGUIHOST_API void  PyArxGuiHostDestroyAll(void);

// 页 / 显隐
PYARXGUIHOST_API int  PyArxGuiHostAddPanel(void* set, const char* name, PyObject* panel);
PYARXGUIHOST_API int  PyArxGuiHostSetVisible(void* set, int show);
PYARXGUIHOST_API int  PyArxGuiHostIsVisible(void* set);
PYARXGUIHOST_API int  PyArxGuiHostIsCreated(void* set);

// 查询
PYARXGUIHOST_API long long PyArxGuiHostGetHwnd(void* set);
PYARXGUIHOST_API int       PyArxGuiHostGetPaletteCount(void* set);
PYARXGUIHOST_API long long PyArxGuiHostGetPaletteHwnd(void* set, int index);
PYARXGUIHOST_API PyObject* PyArxGuiHostGetPanel(void* set, int index); // 新引用
PYARXGUIHOST_API const char* PyArxGuiHostGetName(void* set);

// 修改
PYARXGUIHOST_API int PyArxGuiHostSetName(void* set, const char* name);
PYARXGUIHOST_API int PyArxGuiHostSetToolId(void* set, const char* guid);
PYARXGUIHOST_API int PyArxGuiHostSetSize(void* set, int x, int y);
PYARXGUIHOST_API int PyArxGuiHostGetFullRect(void* set, int out[4]);
PYARXGUIHOST_API int PyArxGuiHostSetOpacity(void* set, int value);
PYARXGUIHOST_API int PyArxGuiHostGetOpacity(void* set);
PYARXGUIHOST_API int PyArxGuiHostSetDockState(void* set, int style);
PYARXGUIHOST_API int PyArxGuiHostGetDockState(void* set);
PYARXGUIHOST_API int PyArxGuiHostSetActivePalette(void* set, int index);
PYARXGUIHOST_API int PyArxGuiHostGetActivePaletteTabIndex(void* set);
PYARXGUIHOST_API int PyArxGuiHostRemovePalette(void* set, int index);
PYARXGUIHOST_API int PyArxGuiHostRollOut(void* set, int delay);
PYARXGUIHOST_API int PyArxGuiHostRollUp(void* set);
PYARXGUIHOST_API int PyArxGuiHostRolledUp(void* set);
PYARXGUIHOST_API int PyArxGuiHostSetAutoRollup(void* set, int enable);
PYARXGUIHOST_API int PyArxGuiHostGetAutoRollup(void* set);
PYARXGUIHOST_API int PyArxGuiHostGetPaletteSetStyle(void* set);
PYARXGUIHOST_API int PyArxGuiHostSetPaletteSetStyle(void* set, unsigned long style);
PYARXGUIHOST_API int PyArxGuiHostIsFloating(void* set);

// 注册表 / 查找（palettes / get_palette / destroy_palette(name) 用）
PYARXGUIHOST_API void* PyArxGuiHostFind(const char* name);
PYARXGUIHOST_API int   PyArxGuiHostDestroyByName(const char* name);
PYARXGUIHOST_API int   PyArxGuiHostGetSetCount(void);
PYARXGUIHOST_API const char* PyArxGuiHostGetSetName(int index);
PYARXGUIHOST_API const char* PyArxGuiHostGetPaletteName(void* set, int index);

#ifdef __cplusplus
}
#endif
