// PyUt.cpp - PyUt 模块：封装 acut* 通用工具函数。
// 约定：SDK 调用失败不抛异常、不崩溃，返回安全默认值；中文走 UTF-8 经 AcString。
#include "stdafx.h"
#include "PyUt.h"
#include "PyString.h"
#include "PyEngineCompat.h"   // PyEngine::pipRun —— pip 包管理底层
// PyCmd 命令管理已迁至 PyArxEngine（PyArxCore.Cmd 由引擎挂载）；pyreloadAll 经 Python 层访问。
#include <acutads.h>
#include <acutil.h>
#include <acedads.h>
#include <acdbabb.h>
#include <fstream>
#include <set>

namespace py = pybind11;

namespace {

// 单字符 -> ACHAR 码（Python 侧传单字符 str）。
int singleCharCode(const AcString& s, const char* what) {
    if (s.isEmpty()) throw py::value_error(std::string(what) + " requires a single character");
    return static_cast<int>(s.kwszPtr()[0]);
}

} // namespace

void bind_PyCore(pym& m) {
    // ---- 命令行输出 ----
    // 直接把文本显示到 CAD 命令行（不解析 C 格式串，避免格式注入；自动补换行）。
    m.def("printf",
        [](const AcString& text) {
            acutPrintf(L"%ls\n", text.kwszPtr());
        },
        py::arg("text"),
        "将文本直接打印到 CAD 命令行（原样输出，不解析格式串）。");

    // ---- 通配匹配 ----
    m.def("wcMatch",
        [](const AcString& str, const AcString& pattern, bool ignoreCase) {
            return ignoreCase
                ? acutWcMatchEx(str.kwszPtr(), pattern.kwszPtr(), true)
                : (acutWcMatch(str.kwszPtr(), pattern.kwszPtr()) == RTNORM);
        },
        py::arg("string"), py::arg("pattern"), py::arg("ignore_case") = false,
        "通配匹配（支持 * ? # @ . 等模式符）；ignore_case=True 忽略大小写。");

    // ---- 字符串处理 ----
    m.def("trimWhiteSpace",
        [](const AcString& s) {
            std::wstring buf = s.isEmpty() ? std::wstring() : std::wstring(s.kwszPtr(), s.length());
            acutTrimWhiteSpace(buf.data()); // 原地修剪（std::wstring 缓冲可写）
            return AcString(buf.c_str());
        },
        py::arg("s"),
        "去除首尾空白，返回新字符串。");

    m.def("xStrLength",
        [](const AcString& s) { return static_cast<size_t>(acutXStrLength(s.kwszPtr())); },
        py::arg("s"),
        "返回扩展字符串长度（与 acutXStrLength 一致）。");

    // ---- ads 几何 ----
    m.def("angleBetween",
        [](const AcGePoint3d& p1, const AcGePoint3d& p2) {
            ads_point a{ p1.x, p1.y, p1.z }, b{ p2.x, p2.y, p2.z };
            return acutAngle(a, b); // 弧度
        },
        py::arg("p1"), py::arg("p2"),
        "两点间连线与 X 轴夹角（弧度），等价 acutAngle。参数为 AcGePoint3d。");

    m.def("distanceBetween",
        [](const AcGePoint3d& p1, const AcGePoint3d& p2) {
            ads_point a{ p1.x, p1.y, p1.z }, b{ p2.x, p2.y, p2.z };
            return acutDistance(a, b);
        },
        py::arg("p1"), py::arg("p2"),
        "两点距离，等价 acutDistance。参数为 AcGePoint3d。");

    m.def("polarPoint",
        [](const AcGePoint3d& pt, double angle, double dist) {
            ads_point a{ pt.x, pt.y, pt.z }, res{};
            acutPolar(a, angle, dist, res);
            return AcGePoint3d(res[0], res[1], res[2]);
        },
        py::arg("point"), py::arg("angle"), py::arg("distance"),
        "由基点、极角（弧度）、距离求极坐标点，返回 AcGePoint3d，等价 acutPolar。");

    // ---- 单位换算（失败返回 None，不抛异常）----
    m.def("cvUnit",
        [](double value, const AcString& oldUnit, const AcString& newUnit) -> py::object {
            double result = 0.0;
            if (acutCvUnit(value, oldUnit.kwszPtr(), newUnit.kwszPtr(), &result) != RTNORM) {
                return py::none();
            }
            return py::float_(result);
        },
        py::arg("value"), py::arg("old_unit"), py::arg("new_unit"),
        "单位换算（如 米->毫米）；失败返回 None，等价 acutCvUnit。");

    // ---- 字符判定 / 转换 ----
    m.def("isAlpha",  [](const AcString& c) { return acutIsAlpha(singleCharCode(c, "isAlpha")) != 0; }, py::arg("c"), "是否字母。");
    m.def("isUpper",  [](const AcString& c) { return acutIsUpper(singleCharCode(c, "isUpper")) != 0; }, py::arg("c"), "是否大写字母。");
    m.def("isLower",  [](const AcString& c) { return acutIsLower(singleCharCode(c, "isLower")) != 0; }, py::arg("c"), "是否小写字母。");
    m.def("isDigit",  [](const AcString& c) { return acutIsDigit(singleCharCode(c, "isDigit")) != 0; }, py::arg("c"), "是否数字。");
    m.def("isXDigit", [](const AcString& c) { return acutIsXDigit(singleCharCode(c, "isXDigit")) != 0; }, py::arg("c"), "是否十六进制数字。");
    m.def("isSpace",  [](const AcString& c) { return acutIsSpace(singleCharCode(c, "isSpace")) != 0; }, py::arg("c"), "是否空白字符。");
    m.def("isPunct",  [](const AcString& c) { return acutIsPunct(singleCharCode(c, "isPunct")) != 0; }, py::arg("c"), "是否标点。");
    m.def("isAlNum",  [](const AcString& c) { return acutIsAlNum(singleCharCode(c, "isAlNum")) != 0; }, py::arg("c"), "是否字母或数字。");
    m.def("isPrint",  [](const AcString& c) { return acutIsPrint(singleCharCode(c, "isPrint")) != 0; }, py::arg("c"), "是否可打印字符。");
    m.def("isCntrl", [](const AcString& c) { return acutIsCntrl(singleCharCode(c, "isCntrl")) != 0; }, py::arg("c"), "是否控制字符。");
    m.def("isGraph", [](const AcString& c) { return acutIsGraph(singleCharCode(c, "isGraph")) != 0; }, py::arg("c"), "是否图形字符（可打印且非空白）。");
    m.def("toUpper",  [](const AcString& c) -> AcString {
        AcString r; r += static_cast<wchar_t>(acutToUpper(singleCharCode(c, "toUpper"))); return r;
    }, py::arg("c"), "字符转大写。");
    m.def("toLower",  [](const AcString& c) -> AcString {
        AcString r; r += static_cast<wchar_t>(acutToLower(singleCharCode(c, "toLower"))); return r;
    }, py::arg("c"), "字符转小写。");

    // 生成 pybind11 stub (.pyi)：不依赖 pybind11-stubgen，自己解析模块成员
    // 默认输出到 site-packages，IDE 自动识别
    m.def("genStub", [](const std::optional<AcString>& module_name,
                         const std::optional<AcString>& out_dir) -> bool {
        namespace py = pybind11;
        try {
            // 默认输出到 site-packages
            std::string od;
            if (out_dir.has_value()) {
                od = (*out_dir).isEmpty() ? std::string() : std::string((*out_dir).utf8Ptr());
            } else {
                py::module_ sys = py::module_::import("sys");
                std::string prefix = py::str(sys.attr("prefix"));
                od = prefix + "\\Lib\\site-packages";
            }
            py::module_ os = py::module_::import("os");
            os.attr("makedirs")(od, py::arg("exist_ok") = true);

            std::vector<std::string> mods;
            if (module_name.has_value()) mods.push_back((*module_name).isEmpty() ? std::string() : std::string((*module_name).utf8Ptr()));
            else mods = {"PyArx.Enum", "PyArx.Ge", "PyArx.Db", "PyArx.Core", "PyArx.Ed"};

            // UTF-8 → wstring 辅助（避免乱码）
            auto to_w = [](const std::string& s) -> std::wstring {
                if (s.empty()) return L"";
                int n = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), nullptr, 0);
                std::wstring w(n, 0);
                MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), &w[0], n);
                return w;
            };

            for (auto& mn : mods) {
                try {
                    py::module_ m = py::module_::import(mn.c_str());
                    std::string out_path = od + "\\" + mn + ".pyi";

                    std::ofstream f(out_path, std::ios::binary);
                    f << "# -*- coding: utf-8 -*-\n";
                    f << "# " << mn + ".pyi - auto-generated by PyUt.genStub\n\n";

                    // 遍历模块成员
                    py::dict dict = m.attr("__dict__");
                    std::vector<std::string> imports;
                    std::vector<std::string> lines;

                    for (auto item : dict) {
                        std::string name = py::str(item.first);
                        if (name.empty() || name[0] == '_') continue;
                        py::object val = py::reinterpret_borrow<py::object>(item.second);
                        std::string qual = py::str(py::repr(val));

                        // 类
                        if (py::isinstance<py::type>(val)) {
                            std::string cls = name;
                            // 基类
                            py::object bases = val.attr("__bases__");
                            std::string base_str;
                            for (auto b : bases) {
                                std::string bn = py::str(b.attr("__name__"));
                                if (bn != "object") base_str = "(" + bn + ")";
                            }
                            lines.push_back("class " + cls + base_str + ":");
                            // 遍历类成员
                            py::dict cd = val.attr("__dict__");
                            for (auto ci : cd) {
                                std::string mname = py::str(ci.first);
                                if (mname.empty() || mname[0] == '_') continue;
                                py::object mval = py::reinterpret_borrow<py::object>(ci.second);
                                std::string doc;
                                if (hasattr(mval, "__doc__")) {
                                    try { doc = py::str(mval.attr("__doc__")); } catch (...) {}
                                }
                                if (!doc.empty() && doc != "None") {
                                    // docstring 第一行通常是签名
                                    std::string sig;
                                    auto nl = doc.find('\n');
                                    if (nl != std::string::npos) sig = doc.substr(0, nl);
                                    else sig = doc;
                                    // 清理
                                    if (sig.find("->") != std::string::npos) {
                                        lines.push_back("    def " + mname + sig + ": ...");
                                    } else {
                                        lines.push_back("    def " + mname + "(self, *args, **kwargs): ...");
                                    }
                                } else {
                                    lines.push_back("    def " + mname + "(self, *args, **kwargs): ...");
                                }
                            }
                            lines.push_back("");
                        }
                        // 函数
                        else if (py::isinstance<py::function>(val)) {
                            std::string doc;
                            if (hasattr(val, "__doc__")) {
                                try { doc = py::str(val.attr("__doc__")); } catch (...) {}
                            }
                            if (!doc.empty() && doc != "None") {
                                std::string sig;
                                auto nl = doc.find('\n');
                                if (nl != std::string::npos) sig = doc.substr(0, nl);
                                else sig = doc;
                                lines.push_back("def " + name + sig + ": ...");
                            } else {
                                lines.push_back("def " + name + "(*args, **kwargs): ...");
                            }
                        }
                        // 常量
                        else {
                            lines.push_back(name + " = " + qual);
                        }
                    }

                    // pybind11 生成的类型表示 pybind.object / pybind11.object → 标准 object
                    for (auto& l : lines) {
                        std::string out = l;
                        const std::string k1 = "pybind11.object";
                        const std::string k2 = "pybind.object";
                        size_t p;
                        while ((p = out.find(k1)) != std::string::npos) out.replace(p, k1.size(), "object");
                        while ((p = out.find(k2)) != std::string::npos) out.replace(p, k2.size(), "object");
                        f << out << "\n";
                    }
                    f.close();
                    acutPrintf(L"[genStub] %s OK -> %s\n",
                        to_w(mn).c_str(),
                        to_w(out_path).c_str());
                } catch (std::exception& e) {
                    acutPrintf(L"[genStub] %s FAIL: %hs\n",
                        to_w(mn).c_str(), e.what());
                }
            }
            return true;
        } catch (std::exception& e) {
            acutPrintf(L"[genStub] error: %hs\n", e.what());
            return false;
        }
    }, py::arg("module_name") = py::none(), py::arg("out_dir") = py::none(),
       "Generate .pyi stub by introspecting pybind11 modules (no external deps).");
}

void bind_PyDebug(pym& m) {
    // ---- 脚本断点调试（debugpy，支持 VSCode Attach）----
    m.def("debugAttach", [](int port) -> bool {
        try {
            py::module_ dbg = py::module_::import("debugpy");
            if (py::hasattr(dbg, "listen")) {
                // debugpy.listen 会用 sys.executable 启动 adapter 子进程等待其连回（超时30s）。
                // CAD 嵌入式 Python 的 sys.executable 指向无效路径，adapter 起不来导致超时。
                // 故先 configure(python=真实python.exe)，并确保 PYTHONPATH 含 site-packages 使 adapter 能找到 debugpy。
                // 动态推导 site-packages：debugpy.__file__ = <site-packages>/debugpy/__init__.py，取父父目录
                // （不再硬编码绝对路径，保证工程可移植；换目录无需改代码）
                py::module_ os = py::module_::import("os");
                try {
                    py::object dbg_file = dbg.attr("__file__");
                    // dp 仅透传给 os.path.dirname（Python 边界），保留 std::string
                    std::string dp = py::cast<std::string>(py::str(dbg_file));
                    py::object path_m = py::module_::import("os.path");
                    AcString sp = py::cast<AcString>(path_m.attr("dirname")(path_m.attr("dirname")(dp)));
                    // 确保 adapter 子进程能 import debugpy（PYTHONPATH 追加该 site-packages）
                    py::object env = os.attr("environ");
                    if (py::hasattr(env, "get")) {
                        py::object cur = env.attr("get")("PYTHONPATH", py::str(""));
                        AcString c = py::cast<AcString>(cur);
                        if (c.find(sp) < 0) {
                            AcString np = c.isEmpty() ? sp : (c + L";" + sp);
                            env["PYTHONPATH"] = py::str(np.utf8Ptr());
                        }
                    }
                    // in_process_debug_adapter=True 时不需要 adapter 子进程，故不 configure(python=...)，
                    // 不依赖任何外部 python.exe 路径（CAD 嵌入式 Python 无独立 python.exe）。
                } catch (std::exception& e) {
                    acutPrintf(L"[PyUt] debug_attach 动态推导跳过: %hs\n", e.what());
                }
                // 端口占用预检：若 127.0.0.1:port 已被其他 CAD/进程监听，快速报错返回，避免 listen 卡住 30 秒
                try {
                    py::module_ socket_m = py::module_::import("socket");
                    py::object s = socket_m.attr("socket")(socket_m.attr("AF_INET"), socket_m.attr("SOCK_STREAM"));
                    s.attr("settimeout")(1.0);
                    py::object rv = s.attr("connect_ex")(py::make_tuple("127.0.0.1", port));
                    s.attr("close")();
                    int r = py::cast<int>(rv);
                    if (r == 0) {
                        acutPrintf(L"[PyUt] 端口 %d 已被占用（可能已有 CAD 实例在监听调试），请先关闭该调试会话再 PYDEBUG\n", port);
                        return false;
                    }
                } catch (std::exception& e) {
                    acutPrintf(L"[PyUt] 端口预检跳过: %hs\n", e.what());
                }
                // 主线程同步 listen：debugpy.listen(port) 返回 {host,port}，不阻塞（adapter 子进程已起）
                // in_process_debug_adapter=True：不启动 adapter 子进程（CAD 里子进程拉不起来导致超时），
                // 在进程内做 DAP adapter，VSCode attach 直连。
                py::object res = dbg.attr("listen")(port, py::arg("in_process_debug_adapter") = true);
                // listen 返回 (client_host, client_port)——VSCode 需连接该 client 端口，而非传入的 port
                try {
                    AcString r = py::cast<AcString>(py::repr(res));
                    acutPrintf(L"[PyUt] debugpy listen 返回 %ls\n", r.kwszPtr());
                } catch (...) {
                    acutPrintf(L"[PyUt] debugpy listen 返回（不可解析）\n");
                }
                acutPrintf(L"[PyUt] debugpy 已在端口 %d 监听成功，等待 VSCode 附加（脚本内用 PyUt.debugBreak 设断点）\n", port);
                return true;
            }
            acutPrintf(L"[PyUt] debugpy 不可用，请先安装: PyUt.pip_install('debugpy')\n");
            return false;
        } catch (py::error_already_set& e) {
            // Python 异常原文写入 log 文件，避免控制台 ANSI 乱码（路径动态推导自 arx 目录，可移植）
            std::string msg = e.what();
            try {
                wchar_t modPath[MAX_PATH] = { 0 };
                if (GetModuleFileNameW(PyEngine::arxHandle(), modPath, MAX_PATH) != 0 && modPath[0]) {
                    auto arxDir = std::filesystem::path(modPath).parent_path();
                    auto logPath = arxDir / L"logs" / L"debug_attach_err.log";
                    std::ofstream f(logPath.wstring(), std::ios::binary);
                    f << "debug_attach error:\n" << msg << "\n";
                }
            } catch (...) {}
            acutPrintf(L"[PyUt] debug_attach 失败（详见 <arxDir>\\logs\\debug_attach_err.log）\n");
            return false;
        } catch (std::exception& e) {
            acutPrintf(L"[PyUt] debug_attach 失败: %hs\n", e.what());
            return false;
        }
    }, py::arg("port") = 5678,
       "启动 debugpy 调试服务器监听端口，供 VSCode Attach 连接（需先安装 debugpy）。");

    m.def("debugBreak", []() {
        try {
            py::module_ dbg = py::module_::import("debugpy");
            // 先等待 VSCode 附加（wait_for_client 阻塞直到调试器连入），再暂停——确保断点命中
            if (py::hasattr(dbg, "wait_for_client")) {
                dbg.attr("wait_for_client")();
            }
            dbg.attr("breakpoint")();
            acutPrintf(L"[PyUt] debugBreak: 已附加并命中断点\n");
        } catch (std::exception& e) {
            acutPrintf(L"[PyUt] debugBreak 失败: %hs\n", e.what());
        }
    }, "插入断点：先等待 VSCode 附加再暂停（等价 debugpy.wait_for_client + breakpoint），需先 PYDEBUG。");

    m.def("debugStop", []() {
        try {
            // in_process adapter 模式下 debugpy 无公开 disconnect API。
            // 用 pydevd.stoptrace()：恢复 sys.settrace(None) 并 dispose_and_kill
            // 所有 pydevd 线程（含 adapter 的 socket/监听线程），卸载 ARX 前调用，
            // 避免 Finalize 时与 debugpy 冲突崩溃（e0000001/KERNELBASE）。
            py::module_ pydevd = py::module_::import("debugpy._vendored.pydevd.pydevd");
            pydevd.attr("stoptrace")();
            acutPrintf(L"[PyUt] debug_stop: 已停止调试（pydevd 已退出）\n");
        } catch (std::exception& e) {
            acutPrintf(L"[PyUt] debug_stop 失败: %hs\n", e.what());
        }
    }, "停止调试：恢复系统 trace 并终止所有 pydevd 线程（含 adapter 监听线程），卸载 ARX 前调用。");

    m.def("debugRunScript", [](const AcString& path) -> bool {
        try {
            py::module_ dbg = py::module_::import("debugpy");
            if (!py::hasattr(dbg, "listen")) {
                acutPrintf(L"[PyUt] debugpy 不可用，请先安装: PyUt.pip_install('debugpy')\n");
                return false;
            }
            py::module_ runpy = py::module_::import("runpy");
            runpy.attr("run_path")(path);
            return true;
        } catch (std::exception& e) {
            acutPrintf(L"[PyUt] debug_run_script 失败: %hs\n", e.what());
            return false;
        }
    }, py::arg("path"),
       "以调试模式运行 Python 脚本（需先 debug_attach 供 VSCode 断点调试）。");
}

void bind_PyUtility(pym& m) {
    m.def("pyreload", [](const AcString& module_name) -> bool {
        try {
            py::module_ sys = py::module_::import("sys");
            py::module_ importlib = py::module_::import("importlib");
            if (!py::hasattr(sys.attr("modules"), module_name.utf8Ptr())) {
                acutPrintf(L"[PyUt] pyreload: 模块 %ls 尚未加载（请先 PYRUN/PYLOAD 加载一次再重载）\n", module_name.kwszPtr());
                return false;
            }
            py::object mod = sys.attr("modules").attr("get")(module_name);
            importlib.attr("reload")(mod);
            // addCmd 已自动覆盖已存在命令，重跑顶层注册即热更新命令回调
            acutPrintf(L"[PyUt] pyreload: %ls 已重新加载，命令已热更新为最新脚本\n", module_name.kwszPtr());
            return true;
        } catch (std::exception& e) {
            acutPrintf(L"[PyUt] pyreload 失败: %hs\n", e.what());
            return false;
        }
    }, py::arg("module_name"),
       "重新加载已加载的 Python 模块（importlib.reload），重跑顶层注册代码；"
       "配合 PyCmd.addCmd 自动覆盖，命令热更新为最新脚本（PyUt.pyreload('test_draw10')）。");

    // pyreloadAll：遍历所有已注册命令，reload 其关联模块，检查函数是否存在，不存在则删除命令
    m.def("pyreloadAll", []() -> int {
        try {
            py::module_ sys = py::module_::import("sys");
            py::module_ importlib = py::module_::import("importlib");
            py::dict sysModules = sys.attr("modules");

            // PyCmd 已迁至 PyArxEngine：命令管理经 PyArxCore.Cmd 访问（引擎初始化时挂载，与 Redir 同模式）
            py::object pyCmd = py::module_::import("PyArxCore.Cmd").attr("PyCmd");
            py::list names = pyCmd.attr("allNames")();
            int reloaded = 0, removed = 0;
            std::set<std::string> reloadedMods;  // 同一模块只 reload 一次

            for (auto item : names) {
                AcString name = py::cast<AcString>(item);
                py::object info = pyCmd.attr("find")(name);
                if (info.is_none()) continue;
                AcString mn = py::cast<AcString>(info.attr("moduleName"));
                AcString fn = py::cast<AcString>(info.attr("funcName"));
                if (mn.isEmpty()) continue;

                std::string mnStr = mn.utf8Ptr();
                // reload 模块（只 reload 一次）
                if (!reloadedMods.count(mnStr)) {
                    if (py::hasattr(sysModules, mn.utf8Ptr())) {
                        py::object mod = sysModules.attr("get")(mn);
                        importlib.attr("reload")(mod);
                        reloadedMods.insert(mnStr);
                        reloaded++;
                    }
                }

                // 检查函数是否存在
                if (py::hasattr(sysModules, mn.utf8Ptr())) {
                    py::object mod = sysModules.attr("get")(mn);
                    if (!py::hasattr(mod, fn.utf8Ptr())) {
                        pyCmd.attr("removeCmd")(name);
                        removed++;
                        acutPrintf(L"[pyreloadAll] 命令 %ls: 函数 %ls.%ls 不存在，已移除\n",
                            name.kwszPtr(), mn.kwszPtr(), fn.kwszPtr());
                    }
                }
            }

            acutPrintf(L"[pyreloadAll] 重载 %d 个模块，移除 %d 个失效命令\n", reloaded, removed);
            return reloaded;
        } catch (std::exception& e) {
            acutPrintf(L"[pyreloadAll] 失败: %hs\n", e.what());
            return -1;
        }
    },
       "重载所有已注册命令的 Python 模块；函数已删除的命令自动移除。"
       "用法：PyUt.pyreloadAll()。");
}
