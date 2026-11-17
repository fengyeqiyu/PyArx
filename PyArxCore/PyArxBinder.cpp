#include "stdafx.h"
#include "logPath.h"
#include <fstream>
#include <cstring>
#include <string>
#include <windows.h>
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
#include "PyArxCompat.h"     // PyRx 兼容别名层（P0）
#include "PyArxExtraEnums.h" // PyRx 枚举补齐（生成物）
#include "PyDbIdTypes.h"     // PyRx 对齐：ObjectId / Handle（P1）
#include "PyDbValueTypes.h"  // PyRx 对齐：Extents / ObjectIdArray / IdPair（P1）
#include "PyEdEditor.h"      // PyRx 对齐：Ed.Editor / Ed.EditorReactor（P1）
#include "PyDbField.h"       // PyRx 对齐：Db.Field + 5 组字段枚举


// ===========================================================================
// 命令行输出编码陷阱：acutPrintf 的格式串是 ACHAR（宽），%s 吃的是宽串。
// 直接把 char*（UTF-8 窄串）喂进去，会被当 wchar_t* 读 → 整段 CJK 乱码。
// 所有窄串先过这个转换。
// ===========================================================================
namespace
{
    std::wstring W(const char* s)
    {
        if (!s) return std::wstring();
        const int n = static_cast<int>(std::strlen(s));
        if (n <= 0) return std::wstring();
        std::wstring w(static_cast<size_t>(n), L'\0');
        const int got = ::MultiByteToWideChar(CP_UTF8, 0, s, n, w.data(), n);
        if (got <= 0) return std::wstring(s, s + n);
        w.resize(static_cast<size_t>(got));
        return w;
    }
}

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
// 热编译：模块名由 HotModName.h 覆盖（默认 PyArxCore），该文件只被本 TU 包含，
// 所以换名热编译只重编这一个文件 + 链接。构建成 PyArxCoreH<tag>.pyd 后，
// 在 CAD 内 import 新模块即可接管 sys.modules["PyArxCore"]。
#include "HotModName.h"

PYBIND11_MODULE(PYARX_HOT_MODULE, m) {
    const char* selfName = PyModule_GetName(m.ptr());
    const bool hotBuild = selfName && std::strcmp(selfName, "PyArxCore") != 0;
    if (hotBuild) {
        // 热编译第二个及以后的模块：先清掉旧的 C++→Python 类型注册，
        // 否则 pybind11 会报 "type X is already registered"。
        auto& it = py::detail::get_internals();
        it.registered_types_cpp.clear();
        it.registered_types_py.clear();
    }
    acutPrintf(L"\n[PyArx] PYBIND11_MODULE(%s) 入口执行\n",
               W(selfName ? selfName : "PyArxCore").c_str());
    m.doc() = "PyArxCore - AutoCAD ObjectARX pybind11 封装绑定（独立 pyd，可热替换）";

    // 先把主模块登记进 sys.modules，使子模块内 py::module_::import("PyArxCore.X") 命中
    {
        PyObject* sysModules = PyImport_GetModuleDict();
        PyDict_SetItemString(sysModules, "PyArxCore", m.ptr());
    }

    // 创建子模块的同时，在 sys.modules 里登记规范名 "PyArxCore.<name>"。
    // 热编译时模块真名是 PyArxCoreH<tag>，但各 binder 内部按 "PyArxCore.X" 互相 import，
    // 不做别名就会命中旧的（或缺失的）子模块 —— 导致 "PyArxCore is not a package"。
    auto sub = [&](const char* subName, const char* subDoc) {
        py::module_ s = m.def_submodule(subName, subDoc);
        std::string key = std::string("PyArxCore.") + subName;
        PyDict_SetItemString(PyImport_GetModuleDict(), key.c_str(), s.ptr());
        return s;
    };

    auto ge = sub("Ge", "AutoCAD ARX AcGe 几何模块");
    bind_PyGe(ge);

    auto en = sub("Enum", "AutoCAD ARX 枚举定义（含 ErrorStatus）");
    bind_PyEnum(en);

    auto rx = sub("Rx", "AutoCAD ARX AcRx 运行时（PyRxObject/PyRxClass）");
    bind_PyRx(rx);

    auto db = sub("Db", "AutoCAD ARX AcDb 数据库模块");
    pybind11_init_PyDb(db);
    bind_PyDbIdTypes(db);   // P1：ObjectId / Handle 基础类型（对齐 PyRx Db.ObjectId / Db.Handle）
    bind_PyDbValueTypes(db); // P1：Extents / Extents2d / ObjectIdArray / IdPair
    bind_PyDbField(db);      // P1：Db.Field / 5 组字段枚举


    auto ed = sub("Ed", "AutoCAD aced* 编辑器全局函数");
    bind_PyEd(ed);
    bind_PyEdEditor(ed);     // P1：Ed.Editor 转发 + Ed.EditorReactor 骨架

    auto gi = sub("Gi", "AutoCAD ARX AcGi 图形接口");
    bind_PyGi(gi);

    auto core = sub("Core", "PyArx 核心工具函数");
    bind_PyCore(core);

    auto utility = sub("Utility", "模块管理（pyreload 等）");
    bind_PyUtility(utility);

    auto debug = sub("Debug", "脚本调试（debug_attach/debug_break/debug_stop/debug_run_script）");
    bind_PyDebug(debug);

    // Ap：Application 门面 + 侧栏（CAD 应用级接口，收进 PyArxCore）
    auto ap = sub("Ap", "AutoCAD ARX Application 门面（PyApp/PaletteSet 侧栏）");
    ap.def("_ap_probe", []() { return 42; }, "探针：验证 Ap 子模块绑定链路");
    acutPrintf(L"\n[PyArx] Ap 子模块对象创建完成 (id=%p)\n", (void*)ap.ptr());

    // 逐项隔离绑定，异常消息写文件精确定位（UTF-8）
    auto ApLog = [](const char* which, const std::exception& e) {
        acutPrintf(L"\n[PyArx] Ap 绑定 %s 异常: %s\n", W(which).c_str(), W(e.what()).c_str());
        try {
            std::wofstream wf(pyarxLogFile(L"ap_bind_err.txt", L"PyArxCore.pyd"), std::ios::app);
            wf << L"[" << W(which).c_str() << L"] " << W(e.what()).c_str() << L"\n";
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

    // PyRx 兼容别名层（P0）：在所有子模块注册完成后追加短名/跨模块/Core 别名，
    // 不改动任何既有绑定；失败不影响主模块可用性。
    try { bind_PyArxCompat(m); }
    catch (const py::error_already_set& e) { ApLog("PyArxCompat(py)", e); }
    catch (const std::exception& e) { ApLog("PyArxCompat", e); }

    // PyRx 枚举补齐（生成物 PyArxExtraEnums.cpp）：PyRx 有、PyArx 原本没有的枚举类，
    // 值取自 PyRx 桩的真实数值；同样做异常隔离，不影响主模块。
    try { bind_PyArxExtraEnums(m); }
    catch (const py::error_already_set& e) { ApLog("PyArxExtraEnums(py)", e); }
    catch (const std::exception& e) { ApLog("PyArxExtraEnums", e); }
}
