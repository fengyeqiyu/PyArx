#include "stdafx.h"
#include "logPath.h"
#include <fstream>
#include "PyGeBinder.h"
#include "PyAdsCode.h"
#include "PyRxBinder.h"
#include "PyDbBinder.h"
#include "PyEdBinder.h"
#include "PyGiBinder.h"
#include "PyUt.h"
#include "PyApDocManager.h"
#include "PyApp.h"
#include "PyPaletteSet.h"

// ===========================================================================
// PyArxBinder.cpp — PyArxCore.pyd 主模块聚合器（独立扩展模块，非 EMBEDDED）。
// 各 binder 的注册函数（bind_PyXxx(pym& m)）集中挂到 PyArxCore 的子模块：
// Ge/Enum/Rx/Db/Ed/Gi/Core/Utility/Debug。
// Redir（ArxOutPut + pip）与 Cmd（PyCmd）由 Engine 在 import PyArxCore 后挂载
// （bind_PyRedir / bind_PyCmd 均在 PyArxEngine 工程，PyCmd 已随命令管理迁至引擎）。
// 子模块创建顺序 = 依赖顺序：
//   Ge(底层值类型) → Enum(枚举/ErrorStatus) → Rx(PyRxObject 基类) →
//   Db(依赖 Rx 基类) → Ed(依赖 Ge) →
//   Gi(依赖 Db) → Core → Utility → Debug
// ===========================================================================
PYBIND11_MODULE(PyArxCore, m) {
    acutPrintf(L"\n[PyArx] PYBIND11_MODULE(PyArxCore) 入口执行\n");
    m.doc() = "PyArxCore - AutoCAD ObjectARX pybind11 封装绑定（独立 pyd，可热替换）";

    // 先把主模块登记进 sys.modules，使子模块内 py::module_::import("PyArxCore.X") 命中
    {
        PyObject* sysModules = PyImport_GetModuleDict();
        PyDict_SetItemString(sysModules, "PyArxCore", m.ptr());
    }

    auto ge = m.def_submodule("Ge", "AutoCAD ARX AcGe 几何模块");
    bind_PyGe(ge);

    auto en = m.def_submodule("Enum", "AutoCAD ARX 枚举定义（含 ErrorStatus）");
    bind_PyEnum(en);

    auto rx = m.def_submodule("Rx", "AutoCAD ARX AcRx 运行时（PyRxObject/PyRxClass）");
    bind_PyRx(rx);

    auto db = m.def_submodule("Db", "AutoCAD ARX AcDb 数据库模块");
    pybind11_init_PyDb(db);

    auto ed = m.def_submodule("Ed", "AutoCAD aced* 编辑器全局函数");
    bind_PyEd(ed);

    auto gi = m.def_submodule("Gi", "AutoCAD ARX AcGi 图形接口");
    bind_PyGi(gi);

    auto core = m.def_submodule("Core", "PyArx 核心工具函数");
    bind_PyCore(core);

    auto utility = m.def_submodule("Utility", "模块管理（pyreload 等）");
    bind_PyUtility(utility);

    auto debug = m.def_submodule("Debug", "脚本调试（debug_attach/debug_break/debug_stop/debug_run_script）");
    bind_PyDebug(debug);

    // Ap：Application 门面 + 侧栏（CAD 应用级接口，收进 PyArxCore）
    auto ap = m.def_submodule("Ap", "AutoCAD ARX Application 门面（PyApp/PaletteSet 侧栏）");
    ap.def("_ap_probe", []() { return 42; }, "探针：验证 Ap 子模块绑定链路");
    acutPrintf(L"\n[PyArx] Ap 子模块对象创建完成 (id=%p)\n", (void*)ap.ptr());

    // 逐项隔离绑定，异常消息写文件精确定位（UTF-8）
    auto ApLog = [](const char* which, const std::exception& e) {
        acutPrintf(L"\n[PyArx] Ap 绑定 %hs 异常: %hs\n", which, e.what());
        try {
            std::wofstream wf(pyarxLogFile(L"ap_bind_err.txt", L"PyArxCore.pyd"), std::ios::app);
            wf << "[" << which << "] " << e.what() << L"\n";
        } catch (...) {}
    };
    // PyApDocument（Document）必须先于 DocumentManager 注册（返回值类型依赖）。
    try { bind_PyApDocument(ap); } catch (const py::error_already_set& e) { ApLog("PyApDocument(py)", e); }
    catch (const std::exception& e) { ApLog("PyApDocument", e); }
    // PyApDocManager（DocumentManager）收进 Ap 子模块（用户定案：docmanager 放 PyApp）。
    // 原在 PyDbBinder.cpp:406 挂载 Db 子模块，已迁移至此，避免重复注册冲突。
    try { bind_PyApDocManager(ap); } catch (const py::error_already_set& e) { ApLog("PyApDocManager(py)", e); }
    catch (const std::exception& e) { ApLog("PyApDocManager", e); }
    try { bind_PyApp(ap); } catch (const py::error_already_set& e) { ApLog("PyApp(py)", e); }
    catch (const std::exception& e) { ApLog("PyApp", e); }
    try { bind_PyStatusBar(ap); } catch (const py::error_already_set& e) { ApLog("PyStatusBar(py)", e); }
    catch (const std::exception& e) { ApLog("PyStatusBar", e); }
    try { bind_PyPaletteSet(ap); } catch (const py::error_already_set& e) { ApLog("PyPaletteSet(py)", e); }
    catch (const std::exception& e) { ApLog("PyPaletteSet", e); }
}
