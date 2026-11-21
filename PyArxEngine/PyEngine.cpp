#include "stdafx.h"
#include "PyEngine.h"
#include "logPath.h"
#include <acedads.h>
#include <acutads.h>
#include "PyRedir.h"
#include "PyThreadScope.h"  // free-threaded: no GIL, thread-state guard only
#include "PyCmdBinder.h"      // ????(PyCmd):????? PyArxEngine.Cmd
#include "PyDispatchBinder.h" // ?????(Dispatch):????? PyArxEngine.Dispatch
#include "PyAppDispatcher.h"  // Dispatch ????(~PyEngine ????????? Clear())

// ---------------------------------------------------------------------------
// ????:? std::filesystem::path ?? Python ????(reinterpret_steal)
// ---------------------------------------------------------------------------
namespace {
	inline py::str toPyStr(const std::filesystem::path& p) {
		return py::reinterpret_steal<py::str>(PyUnicode_FromWideChar(p.wstring().c_str(), -1));
	}

	// AcString ? Python str(UTF-8;py::str(const char*) ? UTF-8 ??)
	inline py::str toPyStr(const AcString& s) {
		return py::str(s.utf8Ptr());
	}

	// ? ARX ????:?? Win32 GetModuleFileNameW ?? arx ??(? acrxEntryPoint::setSearchPath ??)?
	// ?? accoreconsole ? acedGetAppName ???,???? current_path ?? Python ??????????,
	// ?? "No usable python315 with stdlib" ??????
	inline std::filesystem::path currentAppDir() {
		// 1) ??? acrxEntryPoint kInitAppMsg ??? ARX ????(GetModuleFileNameW ??? arx ???)?
		//    ObjectARX ? acrxDynamicLinker ?? arx,???????,GetModuleHandleW ?????,
		//    ????????? arxload ??????????
		if (PyEngine::arxHandle()) {
			wchar_t modulePath[MAX_PATH] = { 0 };
			if (GetModuleFileNameW(PyEngine::arxHandle(), modulePath, MAX_PATH) != 0 && modulePath[0])
				return std::filesystem::path(modulePath).parent_path();
		}
		// 2) acad ???????:acedGetAppName(kInitAppMsg ????????????)
		const ACHAR* pApp = ::acedGetAppName();
		if (pApp && pApp[0]) {
			std::filesystem::path p(pApp);
			if (std::filesystem::exists(p)) return p.parent_path();
		}
		// 3) ????:??????(????? PyArxCore.arx ??)
		wchar_t modulePath2[MAX_PATH] = { 0 };
		HMODULE hMod = GetModuleHandleW(L"PyArxCore.arx");
		if (hMod && GetModuleFileNameW(hMod, modulePath2, MAX_PATH) != 0 && modulePath2[0])
			return std::filesystem::path(modulePath2).parent_path();
		return std::filesystem::current_path();
	}
}

// ARX ????(kInitAppMsg ? acrxEntryPoint ??,? currentAppDir ?? arx ??)
HMODULE PyEngine::s_arxHandle = nullptr;
void PyEngine::setArxHandle(HMODULE hMod) { s_arxHandle = hMod; }

// ===========================================================================
// ??:????? ? ????? ? ????????? ? ????? ? ????
// ===========================================================================
PyEngine::PyEngine() {
	try {
		dbgLog(L"PyEngine ????");

		// 1. ARX ????????/???
		appDir = currentAppDir();
		dbgLog(L"appDir=%ls", appDir.wstring().c_str());

		// 2. Python ???? pyHome = ARX ????(?????????,?????????)
		pyHome = appDir;
		// 3. ?????:Lib\encodings ?? ? python315.zip ??(????????)?
		const auto usable = [](const Path& p) {
#ifdef Py_NO_ENABLE_SHARED
			// ????:Lib\encodings ?? ? python315.zip ??
			return std::filesystem::exists(p / L"Lib" / L"encodings")
				|| std::filesystem::exists(p / L"python315.zip");
#else
#ifdef Py_GIL_DISABLED
			constexpr const wchar_t* kPyRuntimeDll = L"python315t.dll";
#else
			constexpr const wchar_t* kPyRuntimeDll = L"python315.dll";
#endif
			return std::filesystem::exists(p / kPyRuntimeDll)
				&& (std::filesystem::exists(p / L"Lib" / L"encodings")
					|| std::filesystem::exists(p / L"python315.zip"));
#endif
		};
		if (!usable(pyHome)) {
			acutPrintf(L"\n[PyArx] Python ??????: %ls(? Lib\\encodings)",
				pyHome.wstring().c_str());
			dbgLog(L"Python ??????: %ls", pyHome.wstring().c_str());
			throw std::runtime_error("No usable python stdlib");
		}
		dbgLog(L"pyHome(????)=%ls", pyHome.wstring().c_str());

		// 4. ??????(?????)?Py_Finalize ??????????,
		//    ?? arxload ? Py_IsInitialized() ??:??????????
		//    sys.modules ????? PyArx ??,???????--?????
		//    ???,? PyArx ?? init ??????? pybind11 ????
		//    (generic_type: ... an object with that name is already defined)?
		// ??:??? stderr ???,?? Python verbose ??
		// GUI ???????? stderr(_dup2 ??????),? _wfreopen ???????
		{
			_wfreopen((pyHome / L"python_stderr.log").c_str(), L"w", stderr);
		}

		initConfig();
		if (Py_IsInitialized()) {
			acutPrintf(L"\n[PyArx] Python ??????,??????(???????)");
			dbgLog(L"Py_IsInitialized()=true,?????,?? Py_InitializeFromConfig");
		} else {
			dbgLog(L"?? Py_InitializeFromConfig");
			PyStatus initStatus = Py_InitializeFromConfig(&config);
			dbgLog(L"Py_InitializeFromConfig ?? exception=%d err=%hs",
				PyStatus_Exception(initStatus) ? 1 : 0,
				initStatus.err_msg ? initStatus.err_msg : "(none)");
			if (PyStatus_Exception(initStatus)) {
				acutPrintf(L"\n[PyArx] Python ????????: %hs",
					initStatus.err_msg ? initStatus.err_msg : "unknown");
				throw std::runtime_error("Py_InitializeFromConfig failed");
			}
			// free-threaded(3.15t)：没有 GIL，PyEval_InitThreads 已无意义，删掉。
		}
		dbgLog(L"[STAGE 1] Python ????????");

		// 5. ?????:print ??? CAD ???
		dbgLog(L"[STAGE 2] redirectOut ??");
		redirectOut();
		dbgLog(L"[STAGE 2] redirectOut ??");

		// 6. ????:?? site-packages
		dbgLog(L"[STAGE 3] setupVenv ??");
		setupVenv();
		dbgLog(L"[STAGE 3] setupVenv ??");

		// 7. ??????????(??????????)
		acutPrintf(L"\n[PyArx] Python ????????\n");
		dbgLog(L"PyEngine ????");
	}
	catch (const std::exception& e) {
		dbgLog(L"PyEngine ????: %ls", (const wchar_t*)AcString(e.what(), AcString::Utf8));
		acutPrintf(L"\n[PyArx] ?????: %s\n", AcString(e.what(), AcString::Utf8));
		// pybind11 ??????(AcGeTol already defined / is already registered)
		// ?????? init ????????????????????????
		// ????????,?????? CAD ????????
		const std::string msg = e.what();
		if (msg.find("already defined") != std::string::npos ||
			msg.find("already registered") != std::string::npos) {
			acutPrintf(L"\n[PyArx] ??: ??? pybind11 ??????(?????????)?"
				L"??? AutoCAD ?????? ARX?");
		}
		throw; // ? acrxEntryPoint ??,?? ARX ?????
	}
	catch (...) {
		dbgLog(L"PyEngine ??????");
		throw;
	}
}

// ===========================================================================
// ??:??? pybind ??,? Py_FinalizeEx ?????
// ===========================================================================
PyEngine::~PyEngine() {
	// ????? Python ??
	py_sys.release();
	ArxOutPut::release();

	if (Py_IsInitialized()) {
		// free-threaded：没有 GIL 可取；只要求当前线程已有线程状态（初始化线程成立）。
		// 若确实不在挂接线程上，宁可不 Finalize 也不能让 CPython 断言崩进程。
		if (!PyThreadScope::Ready()) {
			dbgLog(L"[FINALIZE] 当前线程无 Python 线程状态，跳过 Py_FinalizeEx");
			PyAppDispatcher::Instance().Clear();
			PyConfig_Clear(&config);
			return;
		}
		// ?? Dispatch ?????:????????GIL ?????? py::object
		// ???(?????????????,???????????)?
		PyAppDispatcher::Instance().Clear();
		Py_FinalizeEx();
	}
	PyConfig_Clear(&config);
	acutPrintf(L"\nPyEngine ?????!\n");
}

// ===========================================================================
// ????:??? <ARX ??>\pyengine.log(UTF-8),???????
// ===========================================================================
void PyEngine::dbgLog(const wchar_t* fmt, ...) {
	try {
		// ????:PYARX_LOG_DIR ??,???????? log ???
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
	catch (...) { /* ?????????? */ }
}

// ===========================================================================
// ??? PyConfig:???,????????????
// ===========================================================================
void PyEngine::initConfig() {
	// ??:??? PYTHONPATH ????,?? Python ??????
	{
		std::wstring pythonPath = L"PYTHONPATH=" + (pyHome / L"Lib").wstring() + L";" + (pyHome / L"python315.zip").wstring();
		_wputenv(pythonPath.c_str());
	}

	PyConfig_InitPythonConfig(&config);  // ??:? IsolatedConfig ???? PythonConfig
	// ???? python315.dll,?????:home/prefix/exec_prefix ???????
	config.verbose = 0;
	PyConfig_SetString(&config, &config.home, pyHome.c_str());
	PyConfig_SetString(&config, &config.prefix, pyHome.c_str());
	PyConfig_SetString(&config, &config.base_prefix, pyHome.c_str());
	PyConfig_SetString(&config, &config.exec_prefix, pyHome.c_str());
	PyConfig_SetString(&config, &config.base_exec_prefix, pyHome.c_str());
	config.parse_argv = 0;
	config.install_signal_handlers = 0;
	config.site_import = 0;  // ???? sys.path,?? site.py
	config.module_search_paths_set = 1;
	// ???????:???(??+pyd) ? Lib(???) ? Lib\site-packages(???) ? DLLs
	PyWideStringList_Append(&config.module_search_paths, pyHome.c_str());
	PyWideStringList_Append(&config.module_search_paths, (pyHome / L"Lib").c_str());
	PyWideStringList_Append(&config.module_search_paths, (pyHome / L"Lib" / L"site-packages").c_str());
	PyWideStringList_Append(&config.module_search_paths, (pyHome / L"DLLs").c_str());
	PyWideStringList_Append(&config.module_search_paths, (pyHome / L"python315.zip").c_str());
}

// ===========================================================================
// ?????(?? PyArxEngine):
//   ?????? Python ?? PyArxEngine ? ?? Redir ???(ArxOutPut + pip)?
//   ? sys.stdout/stderr = ArxOutPut ?? ? import PyArxCore(????/Enum)?
//   ?? Cmd ???(????)?
// ???????? PyArxCore:?? PyArxCore ????,print ??? CAD ????
// ===========================================================================
void PyEngine::redirectOut() {
	// ???(?????)????:PyArxEngine ?? Redir/Cmd/Dispatch ???????
	// ????? def_submodule + py::class_ ?? "already registered"(??
	// py::error_already_set,??? catch(...)),?????????????,
	// ????????? 2 ????????????? stdout/stderr ??????
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
				dbgLog(L"[REDIRECT REUSE] ??????:??????,??? sys.stdout/stderr");
				return;
			}
		}
		catch (const std::exception& e) {
			dbgLog(L"[REDIRECT REUSE] ??????,??????: %hs", e.what());
		}
		catch (...) {
			dbgLog(L"[REDIRECT REUSE] ????????,??????");
		}
	}

	// 0. ?????? Python ?? PyArxEngine(????????????,??? PyArxCore ?)
	//    ? PyImport_AddModule ?????????(create_extension_module ????????
	//    ?????"????"(PyModule_Create)?????? C++ ??),?????? sys.modules,
	//    ??? def_submodule + ???? import PyArxEngine ???
	pym engMod;
	try {
		py::object m = py::reinterpret_borrow<py::object>(PyImport_AddModule("PyArxEngine"));
		if (!m) throw std::runtime_error("PyImport_AddModule ?? NULL");
		engMod = m.cast<pym>();
		dbgLog(L"[REDIRECT 0] ?? PyArxEngine ????");
	}
	catch (const std::exception& e) {
		acutPrintf(L"\n[PyArx] ?? PyArxEngine ????: %hs\n", e.what());
		dbgLog(L"[REDIRECT 0] ?? PyArxEngine ????: %hs", e.what());
	}
	catch (...) {
		dbgLog(L"[REDIRECT 0] ?? PyArxEngine ????? C++ ??");
	}

	// 1. ? PyArxEngine ??? Redir ?????? ArxOutPut(bind_PyRedir ????,????)
	//    ?????? PyArxCore:??? PyArxCore ?????
	pym redir;
	if (engMod) {
		try {
			redir = engMod.def_submodule("Redir", "?????(ArxOutPut)");
			bind_PyRedir(redir);
			dbgLog(L"[REDIRECT 2] Redir ???????(PyArxEngine.Redir)");
		}
		catch (const py::error_already_set& e) {
			acutPrintf(L"\n[PyArx] ?? Redir ?????: %hs\n", e.what());
			dbgLog(L"[REDIRECT 2] ?? Redir ?????: %hs", e.what());
		}
		catch (...) {
			dbgLog(L"[REDIRECT 2] ?? Redir ?????? C++ ??");
		}
	}

	// 1.5 PyArxEngine ??? pip ??(????,python ????:pip ???;
	//     ? Redir/Cmd ??,????? Redir ???)
	if (engMod) {
		try {
			bind_PyPip(engMod);
			dbgLog(L"[REDIRECT 2.1] pip ??????(PyArxEngine.pip_*)");
		}
		catch (const py::error_already_set& e) {
			acutPrintf(L"\n[PyArx] ?? pip ????: %hs\n", e.what());
			dbgLog(L"[REDIRECT 2.1] ?? pip ????: %hs", e.what());
		}
		catch (...) {
			dbgLog(L"[REDIRECT 2.1] ?? pip ????? C++ ??");
		}
	}

	// 2. import PyArxCore(ObjectARX ?? pyd;?? Enum/??,? Cmd ?? ErrorStatus ??)
	//    ??? return:?????? PyEngine ?????(????????),
	//    ?? PyArxCore ?????????(?? print ??????????)?
	dbgLog(L"[REDIRECT 1] import PyArxCore ??");
	bool coreOk = false;
	try {
		pym::import("PyArxCore");
		dbgLog(L"[REDIRECT 1] import PyArxCore ??");
		coreOk = true;
	}
	catch (const py::error_already_set& e) {
		acutPrintf(L"\n[PyArx] import PyArxCore ??: %hs\n", e.what());
		dbgLog(L"[REDIRECT 1] import PyArxCore ??: %hs", e.what());
		// ??:????????????
	}

	// 3. ? PyArxEngine ??? Cmd ?????? PyCmd(bind_PyCmd ????,????????)
	//    Cmd ? Acad::ErrorStatus ? PyArxCore.Enum ??,?? coreOk?
	if (engMod) {
		try {
			pym cmd = engMod.def_submodule("Cmd", "AutoCAD ????(PyCmd)");
			bind_PyCmd(cmd);
			dbgLog(L"[REDIRECT 2.5] Cmd ???????(PyArxEngine.Cmd)");
		}
		catch (const py::error_already_set& e) {
			acutPrintf(L"\n[PyArx] ?? Cmd ?????: %hs\n", e.what());
			dbgLog(L"[REDIRECT 2.5] ?? Cmd ?????: %hs", e.what());
		}
		catch (...) {
			dbgLog(L"[REDIRECT 2.5] ?? Cmd ?????? C++ ??");
		}
	}

	// 3.5 ? PyArxEngine ??? Dispatch ?????? PyAppDispatcher
	//     (bind_PyDispatch ????,?????????;??? PyArxCore)
	if (engMod) {
		try {
			pym disp = engMod.def_submodule("Dispatch", "? Python ???????/???????");
			bind_PyDispatch(disp);
			dbgLog(L"[REDIRECT 2.6] Dispatch ???????(PyArxEngine.Dispatch)");
		}
		catch (const py::error_already_set& e) {
			acutPrintf(L"\n[PyArx] ?? Dispatch ?????: %hs\n", e.what());
			dbgLog(L"[REDIRECT 2.6] ?? Dispatch ?????: %hs", e.what());
		}
		catch (...) {
			dbgLog(L"[REDIRECT 2.6] ?? Dispatch ?????? C++ ??");
		}
	}

	// 4. sys.stdout/stderr = ArxOutPut ??
	//    ?????(???????);redir ??(??/????)????
	try {
		if (redir) {
			dbgLog(L"[REDIRECT 3.1] ? ArxOutPut ?");
			py::object arxoutCls = redir.attr("ArxOutPut");
			dbgLog(L"[REDIRECT 3.2] ??? ArxOutPut");
			py::object arxout = arxoutCls();
			dbgLog(L"[REDIRECT 3.3] import sys");
			py_sys = pym::import("sys");
			dbgLog(L"[REDIRECT 3.4] ?? sys.stdout/stderr");
			py_sys.attr("stdout") = arxout;
			py_sys.attr("stderr") = arxout;
			dbgLog(L"[REDIRECT 3.5] stdout/stderr ?????");
		}
		else {
			dbgLog(L"[REDIRECT 3] redir ??,?????");
		}
	}
	catch (...) {
		acutPrintf(L"\n[PyArx] stdout ?????(???????)\n");
		dbgLog(L"[REDIRECT 3] stdout ?????(ArxOutPut ???)");
	}
}

// ===========================================================================
// ??????
// ===========================================================================
bool PyEngine::RunScript(const Path& scriptPath) {
	dbgLog(L"[RUNSCRIPT 1] ??: %ls", scriptPath.c_str());
	PyThreadScope scope{};
	dbgLog(L"[RUNSCRIPT 2] free-threaded: no GIL");
	try {
		dbgLog(L"[RUNSCRIPT 3] py::eval_file ?");
		py::object result = py::eval_file(scriptPath.string());
		dbgLog(L"[RUNSCRIPT 4] py::eval_file ??");

		// ????????(? GIL ????)
		result.dec_ref();
		result.release();
		dbgLog(L"[RUNSCRIPT 5] ??????");

		// ??? sys.stdout(?????,?? flush ??????)
		dbgLog(L"[RUNSCRIPT 6] ?? true");
		return true;
	}
	catch (const py::error_already_set& e) {
		std::string errMsg = e.what();
		acutPrintf(L"%s\n", AcString(errMsg.c_str(), AcString::Utf8));
		dbgLog(L"[RUNSCRIPT ??] Python ??: %s", AcString(errMsg.c_str(), AcString::Utf8));
		return false;
	}
	catch (...) {
		dbgLog(L"[RUNSCRIPT ??] ?? C++ ??");
		return false;
	}
}

// ===========================================================================
// ???????
// ===========================================================================
bool PyEngine::ExecString(const AcString& code) {
	dbgLog(L"ExecString ??");
	PyThreadScope scope{};
	dbgLog(L"ExecString free-threaded: no GIL");
	try {
		dbgLog(L"ExecString py::exec ?");
		py::exec(code.utf8Ptr());
		dbgLog(L"ExecString py::exec ??");
		return true;
	}
	catch (const py::error_already_set& e) {
		std::string errMsg = e.what();
		acutPrintf(L"%s\n", AcString(errMsg.c_str(), AcString::Utf8));
		dbgLog(L"ExecString Python ??: %s", AcString(errMsg.c_str(), AcString::Utf8));
		return false;
	}
}

// ===========================================================================
// ??????:? ARX ????????(<arxDir>\Lib\site-packages ?? sys.path)
// ===========================================================================
void PyEngine::setupVenv() {
	PyThreadScope scope{};
	try {
		dbgLog(L"setupVenv ??");
		// 1. ARX ?????????
		arxDir = currentAppDir();

		// 2. site-packages ??:????? sys.path
		auto sitePkgs = arxDir / L"Lib" / L"site-packages";
		std::error_code ec;
		std::filesystem::create_directories(sitePkgs, ec);
		dbgLog(L"site-packages ?? ec=%d dir=%ls", (int)ec.value(), sitePkgs.wstring().c_str());

		auto sys = py::module::import("sys");
		auto pathList = sys.attr("path").cast<py::list>();
		// ????(?????? sys.path ??????,???????????)
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

		acutPrintf(L"\n[PyArx] Python ???: %ls", arxDir.wstring().c_str());
		dbgLog(L"setupVenv ??");
	}
	catch (const std::exception& e) {
		acutPrintf(L"\n[PyArx] ?????????: %s",
			AcString(e.what(), AcString::Utf8));
	}
}

// ===========================================================================
// ?? pip ??:? packages(?? / ????)??? <arxDir>\Lib\site-packages
// ===========================================================================
bool PyEngine::pipInstall(const AcString& packages) {
	PyThreadScope scope{};
	try {
		if (packages.isEmpty()) {
			acutPrintf(L"\n[PyArx] pipInstall: ????");
			return false;
		}
		dbgLog(L"pipInstall ?: %ls", packages.kwszPtr());

		// ???????
		if (arxDir.empty()) arxDir = currentAppDir();
		auto target = arxDir / L"Lib" / L"site-packages";
		std::error_code ec;
		std::filesystem::create_directories(target, ec);

		// ?? sys.path(??)
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

		acutPrintf(L"\n[PyArx] pip ????: %ls ...(???,????)",
			target.wstring().c_str());

		// ????(?? / ?? / Tab ??)
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

		// ?? pip ??(??? ensurepip bootstrap)
		py::object pipMain;
		try {
			pipMain = py::module::import("pip._internal.cli.main").attr("main");
		}
		catch (const py::error_already_set&) {
			try {
				py::module::import("ensurepip").attr("bootstrap")();
			}
			catch (const py::error_already_set& e) {
				acutPrintf(L"\n[PyArx] ensurepip ??: %s",
					AcString(e.what(), AcString::Utf8));
			}
			pipMain = py::module::import("pip._internal.cli.main").attr("main");
		}

		py::list cmd;
		cmd.append(py::str("install"));
		cmd.append(py::str("--disable-pip-version-check"));
		cmd.append(py::str("--only-binary=:all:"));   // ?? wheel,??????(CAD ??????)
		cmd.append(py::str("--timeout"));
		cmd.append(py::str("30"));
		cmd.append(py::str("--target"));
		cmd.append(toPyStr(target));
		for (const auto& p : pkgs) cmd.append(toPyStr(p));

		long rc = -1;
		try { rc = pipMain(cmd).cast<long>(); }
		catch (const std::exception&) { /* rc ?? -1 */ }
		acutPrintf(L"\n[PyArx] pip ??? = %ld", rc);
		dbgLog(L"pip ??? = %ld", rc);
		return rc == 0;
	}
	catch (const std::exception& e) {
		acutPrintf(L"\n[PyArx] pip ????: %s",
			AcString(e.what(), AcString::Utf8));
		return false;
	}
}

// ===========================================================================
// ?? pip ???:operation = install/uninstall/list/show/upgrade/download
// ?? site-packages = <arxDir>\Lib\site-packages(install/upgrade/download ?? --target)
// pip ?????? CAD ???;?? pip ???(0=??,-1=??)
// ===========================================================================
int PyEngine::pipRun(const AcString& operation, const std::vector<AcString>& packages) {
	PyThreadScope scope{};
	try {
		if (arxDir.empty()) arxDir = currentAppDir();
		auto target = arxDir / L"Lib" / L"site-packages";
		std::error_code ec;
		std::filesystem::create_directories(target, ec);

		// ?? sys.path(??)
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

		// ?? pip ??(??? ensurepip bootstrap)
		py::object pipMain;
		try {
			pipMain = py::module::import("pip._internal.cli.main").attr("main");
		}
		catch (const py::error_already_set&) {
			try { py::module::import("ensurepip").attr("bootstrap")(); }
			catch (const py::error_already_set& e) {
				acutPrintf(L"\n[PyArx] ensurepip ??: %s", AcString(e.what(), AcString::Utf8));
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

		acutPrintf(L"\n[PyArx] pip %ls ?: %ls ...(????????)",
			operation.kwszPtr(), target.wstring().c_str());
		if (operation != L"cache") { for (const auto& p : packages) cmd.append(toPyStr(p)); }

		long rc = -1;
		try { rc = pipMain(cmd).cast<long>(); }
		catch (const std::exception&) { /* rc ?? -1 */ }
		acutPrintf(L"\n[PyArx] pip %ls ??? = %ld", operation.kwszPtr(), rc);
		dbgLog(L"pip %ls ??? = %ld", operation.kwszPtr(), rc);
		return (int)rc;
	}
	catch (const std::exception& e) {
		acutPrintf(L"\n[PyArx] pip %ls ??: %s", operation.kwszPtr(), AcString(e.what(), AcString::Utf8));
		return -1;
	}
}
