//-----------------------------------------------------------------------------
// PyApp.cpp — PyArxCore 的 Application 门面（静态类），对齐 .NET
//   Autodesk.AutoCAD.ApplicationServices.Application。
//
// 说明：
//   * 本文件刻意只依赖 ObjectARX 全局 API + MFC 基础类型，不依赖 PyMfc 的
//     窗口/侧栏实现（那是 GUI 层职责）。所有侧栏能力由 PyPaletteSet 提供。
//   * 无 C++ 直接等价物的 .NET 成员（InfoCenter / Preferences / UIBindings /
//     MenuBar / MenuGroups / DisplayTextScreen / ShowModal* / DoDragDrop /
//     InvokeHelp 等）统一抛 NotImplementedError，见 _unsupported。
//   * 静态门面形态：py::class_<PyApp> 上全 def_static / def_property_static，
//     Python 侧 PyArxCore.PyApp.DocumentManager 式访问。
// 编码：UTF-8 带 BOM。
//-----------------------------------------------------------------------------

#include "PyApp.h"
#include "logPath.h"

#include <acedCmdNF.h>
#include <adscodes.h>
#include <acedads.h>

namespace
{
    // 无 C++ 等价物的成员统一降级
    [[noreturn]] void Unsupported(const char* member)
    {
        throw py::type_error(std::string(member) +
            " 在 ObjectARX C++ 侧无直接等价物，PyArxCore.PyApp 不提供该成员");
    }

    HWND AcadMainHwnd()
    {
        return ::adsw_acadMainWnd();
    }
}

// ===========================================================================
// PyApp —— 静态门面
// ===========================================================================
namespace
{
    struct PyApp
    {
        // ---- 文档管理 ------------------------------------------------------
        static PyApDocManager DocumentManager()
        {
            return PyApDocManager(::acDocManager);
        }

        // ---- 文档窗口集合 ---------------------------------------------------
        static int DocumentWindowCount()
        {
            AcApDocWindowManager* m = ::acDocWindowManager;
            return m != nullptr ? m->documentWindowCount() : 0;
        }

        // ---- 主窗口 / 非在位主窗口 -----------------------------------------
        static long long MainWindow()
        {
            return reinterpret_cast<long long>(static_cast<void*>(AcadMainHwnd()));
        }

        static long long NonInPlaceMainWindow()
        {
            return reinterpret_cast<long long>(static_cast<void*>(AcadMainHwnd()));
        }

        // ---- 状态栏 ---------------------------------------------------------
        static AcApStatusBar* StatusBar()
        {
            return ::acedGetApplicationStatusBar();
        }

        // ---- 菜单 -----------------------------------------------------------
        static bool LoadPartialMenu(const std::wstring& filename)
        {
            return ::acedLoadPartialMenu(filename.c_str());
        }

        static bool UnloadPartialMenu(const std::wstring& filename)
        {
            return ::acedUnloadPartialMenu(filename.c_str());
        }

        static void ReloadAllMenus()
        {
            ::acedReloadMenus(true);
        }

        static bool SetCurrentWorkspace(const std::wstring& name)
        {
            // 无直接 C++ API；经 acedCommandS 发 "-WSSET" 命令实现。
            int rc = ::acedCommandS(RTSTR, L"-WSSET", RTSTR, name.c_str(), 0);
            return rc == RTNORM;
        }

        // ---- COM 主程序对象 ---------------------------------------------------
        static long long AcadApplication()
        {
            IDispatch* p = ::acedGetIDispatch(false);
            return reinterpret_cast<long long>(static_cast<void*>(p));
        }

        // ---- 刷新图形显示 ------------------------------------------------------
        // 轻量刷新当前视口显示（acedUpdateDisplay，非 REGEN）。
        // 绘制实体后主动刷新，避免“窗体抢焦点后需切回 CAD 才显示新图元”。
        static void UpdateDisplay()
        {
            ::acedUpdateDisplay();

            HWND main = ::adsw_acadMainWnd();
            HWND client = main ? ::FindWindowExW(main, nullptr, L"MDIClient", nullptr) : nullptr;
            HWND view = client ? ::GetWindow(client, GW_CHILD) : nullptr;

            if (main)
                ::RedrawWindow(main, nullptr, nullptr,
                               RDW_INVALIDATE | RDW_UPDATENOW | RDW_ALLCHILDREN);
            if (client)
                ::RedrawWindow(client, nullptr, nullptr,
                               RDW_INVALIDATE | RDW_UPDATENOW | RDW_ALLCHILDREN);
            if (view)
            {
                ::InvalidateRect(view, nullptr, FALSE);
                ::UpdateWindow(view);
            }
        }

        static void SetFocusToAcad()
        {
            HWND main = ::adsw_acadMainWnd();
            if (main != nullptr)
            {
                ::SetFocus(main);
            }
        }

        // ---- 定制模式（无 C++ 等价物，降级）-----------------------------------
        static bool IsInCustomizationMode() { Unsupported("IsInCustomizationMode"); }

        // ---- 显示文字屏幕（无 C++ 等价物，降级）-------------------------------
        static bool DisplayTextScreen() { Unsupported("DisplayTextScreen"); }

        // ---- 其余 .NET 成员（无 C++ 等价物，统一降级）--------------------------
        static PyObject* InfoCenter() { Unsupported("InfoCenter"); }
        static PyObject* Preferences() { Unsupported("Preferences"); }
        static PyObject* UIBindings() { Unsupported("UIBindings"); }
        static PyObject* MenuBar() { Unsupported("MenuBar"); }
        static PyObject* MenuGroups() { Unsupported("MenuGroups"); }
    };
}

// ===========================================================================
// 绑定
// ===========================================================================
void bind_PyApp(pym& m)
{
    py::class_<PyApp>(m, "PyApp")
        .def_property_readonly_static("documentManager",
            [](py::object) { return PyApp::DocumentManager(); },
            "Application.DocumentManager -> AcApDocManager 包装")
        .def_property_readonly_static("documentWindowCollection",
            [](py::object) {
                // 返回轻量集合对象
                py::dict d;
                d["count"] = PyApp::DocumentWindowCount();
                return d;
            },
            "Application.DocumentWindowCollection -> 文档窗口集合")
        .def_property_readonly_static("mainWindow",
            [](py::object) { return PyApp::MainWindow(); },
            "Application.MainWindow -> 主窗口 HWND")
        .def_property_readonly_static("nonInPlaceMainWindow",
            [](py::object) { return PyApp::NonInPlaceMainWindow(); },
            "Application.NonInPlaceMainWindow -> 真实主窗口 HWND")
        .def_property_readonly_static("statusBar",
            [](py::object) { return PyApp::StatusBar(); },
            py::return_value_policy::reference,
            "Application.StatusBar -> AcApStatusBar*")
        .def_property_readonly_static("acadApplication",
            [](py::object) { return PyApp::AcadApplication(); },
            "Application.AcadApplication -> COM IDispatch*")
        .def_static("setFocus", &PyApp::SetFocusToAcad, "把前台/焦点切回 CAD 主窗口")
        .def_static("updateDisplay", &PyApp::UpdateDisplay,
            "轻量刷新当前视口显示（acedUpdateDisplay，非 REGEN），绘制后调用即时可见")
        .def_static("loadPartialMenu", &PyApp::LoadPartialMenu, py::arg("filename"))
        .def_static("unloadPartialMenu", &PyApp::UnloadPartialMenu, py::arg("filename"))
        .def_static("reloadAllMenus", &PyApp::ReloadAllMenus)
        .def_static("setCurrentWorkspace", &PyApp::SetCurrentWorkspace, py::arg("workspaceName"))
        .def_static("isInCustomizationMode", &PyApp::IsInCustomizationMode)
        .def_static("displayTextScreen", &PyApp::DisplayTextScreen)
        .def_static("infoCenter", &PyApp::InfoCenter)
        .def_static("preferences", &PyApp::Preferences)
        .def_static("uiBindings", &PyApp::UIBindings)
        .def_static("menuBar", &PyApp::MenuBar)
        .def_static("menuGroups", &PyApp::MenuGroups);
}

// ===========================================================================
// AcApStatusBar 轻量包装
// ===========================================================================
void bind_PyStatusBar(pym& m)
{
    py::class_<AcApStatusBar>(m, "StatusBar")
        .def("getPaneCount", [](AcApStatusBar& sb) { return sb.GetPaneCount(); })
        .def("getTrayItemCount", [](AcApStatusBar& sb) { return sb.GetTrayItemCount(); })
        .def("removeAllPanes", [](AcApStatusBar& sb, bool update) { return sb.RemoveAllPanes(update ? TRUE : FALSE); },
             py::arg("update") = true);
}
