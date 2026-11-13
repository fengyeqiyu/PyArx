#include "stdafx.h"
#include "PyEngine.h"
#include "logPath.h"
#include <acedads.h>
#include <acutads.h>
#include "PyRedir.h"
#include "PyCmdBinder.h"      // 命令管理（PyCmd）：随引擎挂载 PyArxEngine.Cmd
#include "PyDispatchBinder.h" // 上下文投递（Dispatch）：随引擎挂载 PyArxEngine.Dispatch
#include "PyAppDispatcher.h"  // Dispatch 队列清理（~PyEngine 在解释器终结前调用 Clear()）

// ---------------------------------------------------------------------------
// 私有工具：把 std::filesystem::path 转成 Python 宽字符串（reinterpret_steal）
// ---------------------------------------------------------------------------
namespace {
	inline py::str toPyStr(const std::filesystem::path& p) {
		return py::reinterpret_steal<py::str>(PyUnicode_FromWideChar(p.wstring().c_str(), -1));
	}

	// AcString → Python str（UTF-8；py::str(const char*) 按 UTF-8 编译）
	inline py::str toPyStr(const AcString& s) {
		return py::str(s.utf8Ptr());
	}

	// 取 ARX 所在目录：优先 Win32 GetModuleFileNameW 定位 arx 模块（与 acrxEntryPoint::setSearchPath 一致）。
	// 无头 accoreconsole 下 acedGetAppName 返回空，若回退到 current_path 会把 Python 环境根错指到工作目录，
	// 导致 "No usable python314 with stdlib" 初始化失败。
	inline std::filesystem::path currentAppDir() {
		// 1) 优先用 acrxEntryPoint kInitAppMsg 保存的 ARX 模块句柄（GetModuleFileNameW 可靠取 arx 全路径）。
		//    ObjectARX 由 acrxDynamicLinker 加载 arx，不在标准模块表，GetModuleHandleW 按名查不到，
		//    因此模块句柄必须在 arxload 时由入口传入并保存。
		if (PyEngine::arxHandle()) {
			wchar_t modulePath[MAX_PATH] = { 0 };
			if (GetModuleFileNameW(PyEngine::arxHandle(), modulePath, MAX_PATH) != 0 && modulePath[0])
				return std::filesystem::path(modulePath).parent_path();
		}
		// 2) acad 命令上下文回退：acedGetAppName（kInitAppMsg 阶段无命令上下文时返回空）
		const ACHAR* pApp = ::acedGetAppName();
		if (pApp && pApp[0]) {
			std::filesystem::path p(pApp);
			if (std::filesystem::exists(p)) return p.parent_path();
		}
		// 3) 最后回退：按模块名查找（仅对标准名 PyArxCore.arx 有效）
		wchar_t modulePath2[MAX_PATH] = { 0 };
		HMODULE hMod = GetModuleHandleW(L"PyArxCore.arx");
		if (hMod && GetModuleFileNameW(hMod, modulePath2, MAX_PATH) != 0 && modulePath2[0])
			return std::filesystem::path(modulePath2).parent_path();
		return std::filesystem::current_path();
	}
}

// ARX 模块句柄（kInitAppMsg 由 acrxEntryPoint 传入，供 currentAppDir 定位 arx 路径）
HMODULE PyEngine::s_arxHandle = nullptr;
void PyEngine::setArxHandle(HMODULE hMod) { s_arxHandle = hMod; }

// ===========================================================================
// 构造：定位运行根 → 校验标准库 → 配置并初始化解释器 → 重定向输出 → 虚拟环境
// ===========================================================================
PyEngine::PyEngine() {
	try {
		dbgLog(L"PyEngine 构造开始");

		// 1. ARX 所在目录作为配置/环境根
		appDir = currentAppDir();
		dbgLog(L"appDir=%ls", appDir.wstring().c_str());

		// 2. Python 运行时根 pyHome = ARX 所在目录（自包含嵌入式运行时，目录结构同标准安装）
		pyHome = appDir;
		// 3. 标准库校验：Lib\encodings 目录 或 python314.zip 归档（解释器初始化必需）。
		const auto usable = [](const Path& p) {
#ifdef Py_NO_ENABLE_SHARED
			// 静态链接：Lib\encodings 目录 或 python314.zip 均可
			return std::filesystem::exists(p / L"Lib" / L"encodings")
				|| std::filesystem::exists(p / L"python314.zip");
#else
			constexpr const wchar_t* kPyRuntimeDll = L"python314.dll";
			return std::filesystem::exists(p / kPyRuntimeDll)
				&& (std::filesystem::exists(p / L"Lib" / L"encodings")
					|| std::filesystem::exists(p / L"python314.zip"));
#endif
		};
		if (!usable(pyHome)) {
			acutPrintf(L"\n[PyArx] Python 标准库不可用: %ls（缺 Lib\\encodings）",
				pyHome.wstring().c_str());
			dbgLog(L"Python 标准库不可用: %ls", pyHome.wstring().c_str());
			throw std::runtime_error("No usable python stdlib");
		}
		dbgLog(L"pyHome(标准库根)=%ls", pyHome.wstring().c_str());

		// 4. 初始化解释器（隔离式配置）。Py_Finalize 注释后解释器驻留进程，
		//    再次 arxload 时 Py_IsInitialized() 为真：直接复用现有解释器与
		//    sys.modules 中已注册的 PyArx 模块，跳过重复初始化——重复初始化
		//    会失败，且 PyArx 模块 init 再次执行会触发 pybind11 重复注册
		//    （generic_type: ... an object with that name is already defined）。
		// 调试：重定向 stderr 到文件，捕获 Python verbose 输出
		// GUI 进程无有效控制台 stderr（_dup2 目标句柄无效），用 _wfreopen 直接固定到文件
		{
			_wfreopen((pyHome / L"python_stderr.log").c_str(), L"w", stderr);
		}

		initConfig();
		if (Py_IsInitialized()) {
			acutPrintf(L"\n[PyArx] Python 解释器已驻留，复用现有环境（跳过重复初始化）");
			dbgLog(L"Py_IsInitialized()=true，复用解释器，跳过 Py_InitializeFromConfig");
		} else {
			dbgLog(L"调用 Py_InitializeFromConfig");
			PyStatus initStatus = Py_InitializeFromConfig(&config);
			dbgLog(L"Py_InitializeFromConfig 返回 exception=%d err=%hs",
				PyStatus_Exception(initStatus) ? 1 : 0,
				initStatus.err_msg ? initStatus.err_msg : "(none)");
			if (PyStatus_Exception(initStatus)) {
				acutPrintf(L"\n[PyArx] Python 解释器初始化失败: %hs",
					initStatus.err_msg ? initStatus.err_msg : "unknown");
				throw std::runtime_error("Py_InitializeFromConfig failed");
			}
			PyEval_InitThreads();
		}
		dbgLog(L"[STAGE 1] Python 解释器初始化完成");

		// 5. 输出重定向：print 输出到 CAD 命令行
		dbgLog(L"[STAGE 2] redirectOut 开始");
		redirectOut();
		dbgLog(L"[STAGE 2] redirectOut 完成");

		// 6. 虚拟环境：接入 site-packages
		dbgLog(L"[STAGE 3] setupVenv 开始");
		setupVenv();
		dbgLog(L"[STAGE 3] setupVenv 完成");

		// 7. 全部完成后才打印成功（避免中途抛异常时误导）
		acutPrintf(L"\n[PyArx] Python 引擎初始化成功。\n");
		dbgLog(L"PyEngine 构造完成");
	}
	catch (const std::exception& e) {
		dbgLog(L"PyEngine 构造异常: %ls", (const wchar_t*)AcString(e.what(), AcString::Utf8));
		acutPrintf(L"\n[PyArx] 初始化异常: %s\n", AcString(e.what(), AcString::Utf8));
		// pybind11 类型重复注册（AcGeTol already defined / is already registered）
		// 表明上次模块 init 中途失败、类型注册残留。解释器驻留无法在同会话内
		// 恢复干净绑定状态，提示用户重启 CAD 以获得干净环境。
		const std::string msg = e.what();
		if (msg.find("already defined") != std::string::npos ||
			msg.find("already registered") != std::string::npos) {
			acutPrintf(L"\n[PyArx] 提示: 检测到 pybind11 类型重复注册（上次加载中断的残留）。"
				L"请重启 AutoCAD 后重新加载本 ARX。");
		}
		throw; // 由 acrxEntryPoint 捕获，避免 ARX 初始化崩溃
	}
	catch (...) {
		dbgLog(L"PyEngine 构造未知异常");
		throw;
	}
}

// ===========================================================================
// 析构：先释放 pybind 对象，再 Py_FinalizeEx 清理解释器
// ===========================================================================
PyEngine::~PyEngine() {
	// 先释放所有 Python 对象
	py_sys.release();
	ArxOutPut::release();

	if (Py_IsInitialized()) {
		PyGILState_Ensure();
		// 清空 Dispatch 待执行队列：在解释器终结前、GIL 持有期间释放 py::object
		// 强引用（侧栏按钮投递的任务若未跑完，避免终结后访问悬挂对象）。
		PyAppDispatcher::Instance().Clear();
		Py_FinalizeEx();
	}
	PyConfig_Clear(&config);
	acutPrintf(L"\nPyEngine 已安全卸载！\n");
}

// ===========================================================================
// 调试日志：追加写 <ARX 目录>\pyengine.log（UTF-8），崩溃后仍可定位
// ===========================================================================
void PyEngine::dbgLog(const wchar_t* fmt, ...) {
	try {
		// 日志目录：PYARX_LOG_DIR 优先，否则宿主目录下的 log 子目录
		std::filesystem::path logDir = pyarxLogDir();
		std::error_code ec;
		std::filesystem::create_directories(logDir, ec);
		auto logPath = logDir / L"pyengine.log";
		FILE* f = nullptr;
		if (_wfopen_s(&f, logPath.c_str(), L"a, ccs=UTF-8") == 0 && f) {
			va_list args;
			va_start(args, fmt);
			fwprintf(f, L"[%lu] ", GetTickCount());
			vfwprintf(f, fmt, args);
			fwprintf(f, L"\n");
			va_end(args);
			fclose(f);
		}
	}
	catch (...) { /* 日志失败不影响主流程 */ }
}

// ===========================================================================
// 初始化 PyConfig：隔离式，锁定运行根与模块搜索路径
// ===========================================================================
void PyEngine::initConfig() {
	// 调试：先设置 PYTHONPATH 环境变量，确保 Python 能找到标准库
	{
		std::wstring pythonPath = L"PYTHONPATH=" + (pyHome / L"Lib").wstring() + L";" + (pyHome / L"python314.zip").wstring();
		_wputenv(pythonPath.c_str());
	}

	PyConfig_InitPythonConfig(&config);  // 调试：从 IsolatedConfig 改成普通 PythonConfig
	// 动态链接 python314.dll，自包含部署：home/prefix/exec_prefix 均指向产物目录
	config.verbose = 0;
	PyConfig_SetString(&config, &config.home, pyHome.c_str());
	PyConfig_SetString(&config, &config.prefix, pyHome.c_str());
	PyConfig_SetString(&config, &config.base_prefix, pyHome.c_str());
	PyConfig_SetString(&config, &config.exec_prefix, pyHome.c_str());
	PyConfig_SetString(&config, &config.base_exec_prefix, pyHome.c_str());
	config.parse_argv = 0;
	config.install_signal_handlers = 0;
	config.site_import = 0;  // 手动管理 sys.path，跳过 site.py
	config.module_search_paths_set = 1;
	// 自包含搜索路径：根目录(产物+pyd) → Lib(标准库) → Lib\site-packages(第三方) → DLLs
	PyWideStringList_Append(&config.module_search_paths, pyHome.c_str());
	PyWideStringList_Append(&config.module_search_paths, (pyHome / L"Lib").c_str());
	PyWideStringList_Append(&config.module_search_paths, (pyHome / L"Lib" / L"site-packages").c_str());
	PyWideStringList_Append(&config.module_search_paths, (pyHome / L"DLLs").c_str());
	PyWideStringList_Append(&config.module_search_paths, (pyHome / L"python314.zip").c_str());
}

// ===========================================================================
// 重定向输出（归属 PyArxEngine）：
//   创建引擎自有 Python 模块 PyArxEngine → 挂载 Redir 子模块（ArxOutPut + pip）→
//   设 sys.stdout/stderr = ArxOutPut 实例 → import PyArxCore（注册类型/Enum）→
//   挂载 Cmd 子模块（命令管理）。
// 重定向本身不依赖 PyArxCore：即使 PyArxCore 导入失败，print 仍能进 CAD 命令行。
// ===========================================================================
void PyEngine::redirectOut() {
	// 软重载（解释器驻留）快速路径：PyArxEngine 及其 Redir/Cmd/Dispatch 子模块上一轮已
	// 注册。再次 def_submodule + py::class_ 会抛 "already registered"（不是
	// py::error_already_set，会落到 catch(...)），虽然被吞掉但绑定状态不可控，
	// 历史上正是软重载第 2 轮卡死的触发点。此处只恢复 stdout/stderr 后直接返回。
	if (Py_IsInitialized()) {
		try {
			py::object mods = py::module_::import("sys").attr("modules");
			py::object existing = mods.attr("get")("PyArxEngine");
			if (existing && !existing.is_none() && py::hasattr(existing, "Redir")) {
				pym eng = existing.cast<pym>();
				py::object arxout = eng.attr("Redir").attr("ArxOutPut")();
				py_sys = py::module_::import("sys");
				py_sys.attr("stdout") = arxout;
				py_sys.attr("stderr") = arxout;
				dbgLog(L"[REDIRECT REUSE] 解释器已驻留：跳过重复绑定，仅恢复 sys.stdout/stderr");
				return;
			}
		}
		catch (const std::exception& e) {
			dbgLog(L"[REDIRECT REUSE] 复用检查异常，回落完整绑定: %hs", e.what());
		}
		catch (...) {
			dbgLog(L"[REDIRECT REUSE] 复用检查未知异常，回落完整绑定");
		}
	}

	// 0. 创建引擎自有 Python 模块 PyArxEngine（重定向与命令管理归属引擎，不挂在 PyArxCore 下）
	//    用 PyImport_AddModule 运行时创建普通模块（create_extension_module 在已初始化解释器
	//    运行时创建"扩展模块"（PyModule_Create）不可靠、易抛 C++ 异常），创建后登记进 sys.modules，
	//    供后续 def_submodule + 用户脚本 import PyArxEngine 命中。
	pym engMod;
	try {
		py::object m = py::reinterpret_borrow<py::object>(PyImport_AddModule("PyArxEngine"));
		if (!m) throw std::runtime_error("PyImport_AddModule 返回 NULL");
		engMod = m.cast<pym>();
		dbgLog(L"[REDIRECT 0] 创建 PyArxEngine 模块成功");
	}
	catch (const std::exception& e) {
		acutPrintf(L"\n[PyArx] 创建 PyArxEngine 模块失败: %hs\n", e.what());
		dbgLog(L"[REDIRECT 0] 创建 PyArxEngine 模块失败: %hs", e.what());
	}
	catch (...) {
		dbgLog(L"[REDIRECT 0] 创建 PyArxEngine 模块抛未知 C++ 异常");
	}

	// 1. 在 PyArxEngine 上创建 Redir 子模块并绑定 ArxOutPut（bind_PyRedir 在本工程，仅重定向）
	//    重定向独立于 PyArxCore：不依赖 PyArxCore 导入结果。
	pym redir;
	if (engMod) {
		try {
			redir = engMod.def_submodule("Redir", "输出重定向（ArxOutPut）");
			bind_PyRedir(redir);
			dbgLog(L"[REDIRECT 2] Redir 子模块挂载完成（PyArxEngine.Redir）");
		}
		catch (const py::error_already_set& e) {
			acutPrintf(L"\n[PyArx] 挂载 Redir 子模块失败: %hs\n", e.what());
			dbgLog(L"[REDIRECT 2] 挂载 Redir 子模块失败: %hs", e.what());
		}
		catch (...) {
			dbgLog(L"[REDIRECT 2] 挂载 Redir 子模块抛未知 C++ 异常");
		}
	}

	// 1.5 PyArxEngine 顶层挂 pip 系列（归属引擎，python 环境设置：pip 包管理；
	//     与 Redir/Cmd 平级，不再嵌套在 Redir 子模块）
	if (engMod) {
		try {
			bind_PyPip(engMod);
			dbgLog(L"[REDIRECT 2.1] pip 系列挂载完成（PyArxEngine.pip_*）");
		}
		catch (const py::error_already_set& e) {
			acutPrintf(L"\n[PyArx] 挂载 pip 系列失败: %hs\n", e.what());
			dbgLog(L"[REDIRECT 2.1] 挂载 pip 系列失败: %hs", e.what());
		}
		catch (...) {
			dbgLog(L"[REDIRECT 2.1] 挂载 pip 系列抛未知 C++ 异常");
		}
	}

	// 2. import PyArxCore（ObjectARX 封装 pyd；注册 Enum/类型，供 Cmd 绑定 ErrorStatus 引用）
	//    失败不 return：输出重定向是 PyEngine 的独立能力（跟随项目启动必建），
	//    不因 PyArxCore 导入失败而整体跳过（否则 print 无出口且影响后续命令）。
	dbgLog(L"[REDIRECT 1] import PyArxCore 开始");
	bool coreOk = false;
	try {
		pym::import("PyArxCore");
		dbgLog(L"[REDIRECT 1] import PyArxCore 成功");
		coreOk = true;
	}
	catch (const py::error_already_set& e) {
		acutPrintf(L"\n[PyArx] import PyArxCore 失败: %hs\n", e.what());
		dbgLog(L"[REDIRECT 1] import PyArxCore 失败: %hs", e.what());
		// 继续：仍尝试建立基础输出重定向
	}

	// 3. 在 PyArxEngine 上创建 Cmd 子模块并绑定 PyCmd（bind_PyCmd 在本工程，命令管理归属引擎）
	//    Cmd 的 Acad::ErrorStatus 由 PyArxCore.Enum 提供，故需 coreOk。
	if (engMod) {
		try {
			pym cmd = engMod.def_submodule("Cmd", "AutoCAD 命令管理（PyCmd）");
			bind_PyCmd(cmd);
			dbgLog(L"[REDIRECT 2.5] Cmd 子模块挂载完成（PyArxEngine.Cmd）");
		}
		catch (const py::error_already_set& e) {
			acutPrintf(L"\n[PyArx] 挂载 Cmd 子模块失败: %hs\n", e.what());
			dbgLog(L"[REDIRECT 2.5] 挂载 Cmd 子模块失败: %hs", e.what());
		}
		catch (...) {
			dbgLog(L"[REDIRECT 2.5] 挂载 Cmd 子模块抛未知 C++ 异常");
		}
	}

	// 3.5 在 PyArxEngine 上创建 Dispatch 子模块并绑定 PyAppDispatcher
	//     （bind_PyDispatch 在本工程，上下文投递归属引擎；不依赖 PyArxCore）
	if (engMod) {
		try {
			pym disp = engMod.def_submodule("Dispatch", "把 Python 回调投递到命令/应用上下文执行");
			bind_PyDispatch(disp);
			dbgLog(L"[REDIRECT 2.6] Dispatch 子模块挂载完成（PyArxEngine.Dispatch）");
		}
		catch (const py::error_already_set& e) {
			acutPrintf(L"\n[PyArx] 挂载 Dispatch 子模块失败: %hs\n", e.what());
			dbgLog(L"[REDIRECT 2.6] 挂载 Dispatch 子模块失败: %hs", e.what());
		}
		catch (...) {
			dbgLog(L"[REDIRECT 2.6] 挂载 Dispatch 子模块抛未知 C++ 异常");
		}
	}

	// 4. sys.stdout/stderr = ArxOutPut 实例
	//    失败仅记录（不影响引擎启动）；redir 为空（模块/挂载失败）时跳过。
	try {
		if (redir) {
			dbgLog(L"[REDIRECT 3.1] 取 ArxOutPut 类");
			py::object arxoutCls = redir.attr("ArxOutPut");
			dbgLog(L"[REDIRECT 3.2] 实例化 ArxOutPut");
			py::object arxout = arxoutCls();
			dbgLog(L"[REDIRECT 3.3] import sys");
			py_sys = pym::import("sys");
			dbgLog(L"[REDIRECT 3.4] 赋值 sys.stdout/stderr");
			py_sys.attr("stdout") = arxout;
			py_sys.attr("stderr") = arxout;
			dbgLog(L"[REDIRECT 3.5] stdout/stderr 重定向完成");
		}
		else {
			dbgLog(L"[REDIRECT 3] redir 为空，跳过重定向");
		}
	}
	catch (...) {
		acutPrintf(L"\n[PyArx] stdout 重定向失败（不影响引擎启动）\n");
		dbgLog(L"[REDIRECT 3] stdout 重定向失败（ArxOutPut 不可用）");
	}
}

// ===========================================================================
// 运行脚本文件
// ===========================================================================
bool PyEngine::RunScript(const Path& scriptPath) {
	dbgLog(L"[RUNSCRIPT 1] 进入: %ls", scriptPath.c_str());
	py::gil_scoped_acquire acquire{};
	dbgLog(L"[RUNSCRIPT 2] GIL 获取成功");
	try {
		dbgLog(L"[RUNSCRIPT 3] py::eval_file 前");
		py::object result = py::eval_file(scriptPath.string());
		dbgLog(L"[RUNSCRIPT 4] py::eval_file 成功");

		// 显式释放结果对象（在 GIL 持有期间）
		result.dec_ref();
		result.release();
		dbgLog(L"[RUNSCRIPT 5] 结果对象释放");

		// 不恢复 sys.stdout（保持重定向，避免 flush 访问失效状态）
		dbgLog(L"[RUNSCRIPT 6] 返回 true");
		return true;
	}
	catch (const py::error_already_set& e) {
		std::string errMsg = e.what();
		acutPrintf(L"%s\n", AcString(errMsg.c_str(), AcString::Utf8));
		dbgLog(L"[RUNSCRIPT 异常] Python 异常: %s", AcString(errMsg.c_str(), AcString::Utf8));
		return false;
	}
	catch (...) {
		dbgLog(L"[RUNSCRIPT 异常] 未知 C++ 异常");
		return false;
	}
}

// ===========================================================================
// 执行代码字符串
// ===========================================================================
bool PyEngine::ExecString(const AcString& code) {
	dbgLog(L"ExecString 进入");
	py::gil_scoped_acquire acquire{};
	dbgLog(L"ExecString GIL 获取");
	try {
		dbgLog(L"ExecString py::exec 前");
		py::exec(code.utf8Ptr());
		dbgLog(L"ExecString py::exec 成功");
		return true;
	}
	catch (const py::error_already_set& e) {
		std::string errMsg = e.what();
		acutPrintf(L"%s\n", AcString(errMsg.c_str(), AcString::Utf8));
		dbgLog(L"ExecString Python 异常: %s", AcString(errMsg.c_str(), AcString::Utf8));
		return false;
	}
}

// ===========================================================================
// 设置虚拟环境：以 ARX 所在目录为环境根（<arxDir>\Lib\site-packages 接入 sys.path）
// ===========================================================================
void PyEngine::setupVenv() {
	py::gil_scoped_acquire acquire{};
	try {
		dbgLog(L"setupVenv 进入");
		// 1. ARX 所在目录作为环境根
		arxDir = currentAppDir();

		// 2. site-packages 目录：创建并接入 sys.path
		auto sitePkgs = arxDir / L"Lib" / L"site-packages";
		std::error_code ec;
		std::filesystem::create_directories(sitePkgs, ec);
		dbgLog(L"site-packages 创建 ec=%d dir=%ls", (int)ec.value(), sitePkgs.wstring().c_str());

		auto sys = py::module::import("sys");
		auto pathList = sys.attr("path").cast<py::list>();
		// 去重插入（解释器复用时 sys.path 已含这些路径，重复插入会污染搜索顺序）
		auto addPathUnique = [&](const std::filesystem::path& p, bool front) {
			py::str sp = toPyStr(p);
			for (py::handle h : pathList) {
				try { if (h.cast<std::wstring>() == p.wstring()) return; }
				catch (...) {}
			}
			if (front) pathList.insert(0, sp); else pathList.append(sp);
		};
		addPathUnique(arxDir, true);
		addPathUnique(sitePkgs, false);

		acutPrintf(L"\n[PyArx] Python 环境根: %ls", arxDir.wstring().c_str());
		dbgLog(L"setupVenv 完成");
	}
	catch (const std::exception& e) {
		acutPrintf(L"\n[PyArx] 虚拟环境初始化失败: %s",
			AcString(e.what(), AcString::Utf8));
	}
}

// ===========================================================================
// 通用 pip 安装：把 packages（空格 / 逗号分隔）安装到 <arxDir>\Lib\site-packages
// ===========================================================================
bool PyEngine::pipInstall(const AcString& packages) {
	py::gil_scoped_acquire acquire{};
	try {
		if (packages.isEmpty()) {
			acutPrintf(L"\n[PyArx] pipInstall: 包名为空");
			return false;
		}
		dbgLog(L"pipInstall 包: %ls", packages.kwszPtr());

		// 确保环境根就绪
		if (arxDir.empty()) arxDir = currentAppDir();
		auto target = arxDir / L"Lib" / L"site-packages";
		std::error_code ec;
		std::filesystem::create_directories(target, ec);

		// 接入 sys.path（去重）
		auto sys = py::module::import("sys");
		auto pathList = sys.attr("path").cast<py::list>();
		auto addPathUnique = [&](const std::filesystem::path& p, bool front) {
			py::str sp = toPyStr(p);
			for (py::handle h : pathList) {
				try { if (h.cast<std::wstring>() == p.wstring()) return; }
				catch (...) {}
			}
			if (front) pathList.insert(0, sp); else pathList.append(sp);
		};
		addPathUnique(arxDir, true);
		addPathUnique(target, false);

		acutPrintf(L"\n[PyArx] pip 安装包到: %ls ...（需联网，可能较慢）",
			target.wstring().c_str());

		// 拆分包名（空格 / 逗号 / Tab 分隔）
		std::vector<AcString> pkgs;
		{
			AcString cur;
			for (const wchar_t* p = packages.kwszPtr(); p && *p; ++p) {
				const wchar_t c = *p;
				if (c == L' ' || c == L',' || c == L'\t') {
					if (!cur.isEmpty()) { pkgs.push_back(cur); cur = AcString(); }
				}
				else cur += c;
			}
			if (!cur.isEmpty()) pkgs.push_back(cur);
		}

		// 确保 pip 可用（必要时 ensurepip bootstrap）
		py::object pipMain;
		try {
			pipMain = py::module::import("pip._internal.cli.main").attr("main");
		}
		catch (const py::error_already_set&) {
			try {
				py::module::import("ensurepip").attr("bootstrap")();
			}
			catch (const py::error_already_set& e) {
				acutPrintf(L"\n[PyArx] ensurepip 失败: %s",
					AcString(e.what(), AcString::Utf8));
			}
			pipMain = py::module::import("pip._internal.cli.main").attr("main");
		}

		py::list cmd;
		cmd.append(py::str("install"));
		cmd.append(py::str("--disable-pip-version-check"));
		cmd.append(py::str("--only-binary=:all:"));   // 只用 wheel，避免源码编译（CAD 内无编译环境）
		cmd.append(py::str("--timeout"));
		cmd.append(py::str("30"));
		cmd.append(py::str("--target"));
		cmd.append(toPyStr(target));
		for (const auto& p : pkgs) cmd.append(toPyStr(p));

		long rc = -1;
		try { rc = pipMain(cmd).cast<long>(); }
		catch (const std::exception&) { /* rc 保持 -1 */ }
		acutPrintf(L"\n[PyArx] pip 返回码 = %ld", rc);
		dbgLog(L"pip 返回码 = %ld", rc);
		return rc == 0;
	}
	catch (const std::exception& e) {
		acutPrintf(L"\n[PyArx] pip 安装失败: %s",
			AcString(e.what(), AcString::Utf8));
		return false;
	}
}

// ===========================================================================
// 通用 pip 包管理：operation = install/uninstall/list/show/upgrade/download
// 目标 site-packages = <arxDir>\Lib\site-packages（install/upgrade/download 自动 --target）
// pip 输出重定向到 CAD 命令行；返回 pip 返回码（0=成功，-1=异常）
// ===========================================================================
int PyEngine::pipRun(const AcString& operation, const std::vector<AcString>& packages) {
	py::gil_scoped_acquire acquire{};
	try {
		if (arxDir.empty()) arxDir = currentAppDir();
		auto target = arxDir / L"Lib" / L"site-packages";
		std::error_code ec;
		std::filesystem::create_directories(target, ec);

		// 接入 sys.path（去重）
		auto sys = py::module::import("sys");
		auto pathList = sys.attr("path").cast<py::list>();
		auto addPathUnique = [&](const std::filesystem::path& p, bool front) {
			py::str sp = toPyStr(p);
			for (py::handle h : pathList) {
				try { if (h.cast<std::wstring>() == p.wstring()) return; }
				catch (...) {}
			}
			if (front) pathList.insert(0, sp); else pathList.append(sp);
		};
		addPathUnique(arxDir, true);
		addPathUnique(target, false);

		// 确保 pip 可用（必要时 ensurepip bootstrap）
		py::object pipMain;
		try {
			pipMain = py::module::import("pip._internal.cli.main").attr("main");
		}
		catch (const py::error_already_set&) {
			try { py::module::import("ensurepip").attr("bootstrap")(); }
			catch (const py::error_already_set& e) {
				acutPrintf(L"\n[PyArx] ensurepip 失败: %s", AcString(e.what(), AcString::Utf8));
			}
			pipMain = py::module::import("pip._internal.cli.main").attr("main");
		}

		py::list cmd;
		cmd.append(toPyStr(operation));
		cmd.append(py::str("--disable-pip-version-check"));
		if (operation == L"install" || operation == L"upgrade" || operation == L"download" || operation == L"wheel") {
			cmd.append(py::str("--only-binary=:all:"));
			cmd.append(py::str("--timeout"));
			cmd.append(py::str("30"));
			cmd.append(py::str("--target"));
			cmd.append(toPyStr(target));
			if (operation == L"upgrade") cmd.append(py::str("--upgrade"));
		}
		if (operation == L"uninstall") { cmd.append(py::str("--target")); cmd.append(toPyStr(target)); }
		if (operation == L"list")      { cmd.append(py::str("--path"));    cmd.append(toPyStr(target)); }
		if (operation == L"cache") {
			if (!packages.empty()) cmd.append(toPyStr(packages[0])); // pip cache <list|purge|info>
		}

		acutPrintf(L"\n[PyArx] pip %ls 到: %ls ...（需联网时可能较慢）",
			operation.kwszPtr(), target.wstring().c_str());
		if (operation != L"cache") { for (const auto& p : packages) cmd.append(toPyStr(p)); }

		long rc = -1;
		try { rc = pipMain(cmd).cast<long>(); }
		catch (const std::exception&) { /* rc 保持 -1 */ }
		acutPrintf(L"\n[PyArx] pip %ls 返回码 = %ld", operation.kwszPtr(), rc);
		dbgLog(L"pip %ls 返回码 = %ld", operation.kwszPtr(), rc);
		return (int)rc;
	}
	catch (const std::exception& e) {
		acutPrintf(L"\n[PyArx] pip %ls 失败: %s", operation.kwszPtr(), AcString(e.what(), AcString::Utf8));
		return -1;
	}
}
