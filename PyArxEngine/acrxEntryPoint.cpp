// acrxEntryPoint.cpp — PyArxEngine ARX 宿主入口（acad.exe=arx / accoreconsole.exe=crx）。
// kInitAppMsg 只注册命令（加载期零重量），Python 引擎延后到首次命令懒初始化。
// 卸载：先清 palette → 清 sys.modules → Py_FinalizeEx → FreeLibrary pyd 句柄。
// 编码：UTF-8 带 BOM。

#include "stdafx.h"
#include <thread>
#include <aced.h>
#include <acedCmdNF.h>
#include "PyEngineC.h"
#include "PyEngine.h"
#include "logPath.h"

// ---------------------------------------------------------------------------
// 加载期轻量化：kInitAppMsg 只注册命令，不执行 Py_Initialize。
// 重量初始化延后到首次命令（ensurePyEngine）。
// ---------------------------------------------------------------------------

static HMODULE g_arxHandle = nullptr;
static bool g_pyEngineReady = false;

// 内联文件日志（定义在文件末，入口处需前向声明）
static void palLog(const wchar_t* msg);

// 命令组名（与 acedGetAppName stem 一致，Core.pyd 的 PyCmd 同组，removeGroup 统一清理）
static const wchar_t* kCmdGroup = L"PyArxEngine";

// 懒初始化：首次命令才 Py_Initialize + import PyArxCore
static void ensurePyEngine() {
	if (g_pyEngineReady) { palLog(L"[ENSURE] 已就绪，跳过"); return; }
	palLog(L"[ENSURE] 开始初始化");
	PyEngine_SetArxHandle(g_arxHandle);
	palLog(L"[ENSURE] 调 PyEngine_Init");
	int rc = PyEngine_Init();
	palLog(rc == 0 ? L"[ENSURE] PyEngine_Init 返回 0" : L"[ENSURE] PyEngine_Init 返回非 0");
	if (rc == 0) {
		g_pyEngineReady = true;
		acutPrintf(L"\n[PyArx] Python 引擎已就绪（首次按需初始化完成）。\n");
	}
	else {
		acutPrintf(L"\n[PyArx] Python 引擎初始化失败，请查看 pyengine.log。\n");
	}
}

// ---------------------------------------------------------------------------
// 命令实现
// ---------------------------------------------------------------------------

static void pyload() {
	ensurePyEngine();
	if (!g_pyEngineReady) return;
	auto py_engine = PyEngine::instance();
	resbuf* rb = acutNewRb(RTSTR);
	if (!rb) return;
	int rc = acedGetFileD(L"选择 Python 脚本", L"", L"py", 0, rb);
	if (rc == RTNORM && rb->resval.rstring && *rb->resval.rstring) {
		std::filesystem::path fp(rb->resval.rstring);
		py_engine->RunScript(fp);
	}
	acutRelRb(rb);
}

static AcString gPyScriptPath;
static AcString gPyCode;

static void pyRunFileGo() {
	palLog(L"[PYRUN goat] 进入");
	ensurePyEngine();
	palLog(g_pyEngineReady ? L"[PYRUN goat] 引擎就绪" : L"[PYRUN goat] 引擎未就绪");
	if (!g_pyEngineReady) return;
	if (gPyScriptPath.isEmpty()) {
		ACHAR path[1024] = {};
		palLog(L"[PYRUN goat] 等待 acedGetString（此行为卡点则说明在等命令行输入）");
		if (acedGetString(0, L"输入 Python 脚本路径: ", path) != RTNORM) { palLog(L"[PYRUN goat] acedGetString 取消"); return; }
		palLog(L"[PYRUN goat] acedGetString 返回");
		gPyScriptPath = path;
	}
	std::filesystem::path fp(gPyScriptPath.kwszPtr());
	gPyScriptPath = AcString();

	palLog(L"[PYRUN goat] 调 RunScript");
	acutPrintf(L"\n[PYRUN] 开始执行: %ls\n", fp.wstring().c_str());
	int rc = PyEngine_RunScript(fp.wstring().c_str());
	acutPrintf(L"[PYRUN] RunScript 返回 rc=%d\n", rc);
	acutPrintf(L"[PYRUN] 完成\n");
}

static void pyExecGo() {
	ensurePyEngine();
	if (!g_pyEngineReady) return;
	if (gPyCode.isEmpty()) {
		ACHAR code[1024] = {};
		if (acedGetString(0, L"输入 Python 代码: ", code) != RTNORM) return;
		gPyCode = code;
	}
	AcString code = gPyCode;
	gPyCode = AcString();
	int rc = PyEngine_ExecString(code.kwszPtr());
	acutPrintf(L"\n[PyArx] PYEXEC %s\n", rc == 0 ? L"完成" : L"失败");
}

static void postPyCmd(const wchar_t* cmdName) {
	acedCommandS(RTSTR, cmdName, RTNONE);
}

static int pyRunFileFunc() {
	// AutoLISP 子程序直接执行，不经 acedCommandS 转发命令（addCommand 命令 + acedCommand
	// 在 LISP 回调上下文存在已知失败问题，见 Autodesk 文档）。
	palLog(L"[PYRUNFILE] 被调用");
	resbuf* args = acedGetArgs();
	if (!args || args->restype != RTSTR || !args->resval.rstring) { palLog(L"[PYRUNFILE] 无参数/非字符串"); acedRetNil(); return RTNORM; }
	AcString path(args->resval.rstring);
	palLog(L"[PYRUNFILE] 路径获取成功");
	acedRetNil();
	palLog(L"[PYRUNFILE] ensurePyEngine 前");
	ensurePyEngine();
	palLog(g_pyEngineReady ? L"[PYRUNFILE] g_pyEngineReady=true" : L"[PYRUNFILE] g_pyEngineReady=false");
	if (!g_pyEngineReady) { acutPrintf(L"\n[PYRUNFILE] Python 引擎未就绪，脚本未执行: %ls\n", path.kwszPtr()); return RTNORM; }
	palLog(L"[PYRUNFILE] RunScript 前");
	PyEngine_RunScript(path.kwszPtr());
	palLog(L"[PYRUNFILE] RunScript 完成");
	return RTNORM;
}

static int pyExecFunc() {
	// 同 pyRunFileFunc：AutoLISP 子程序直接执行，不经命令转发。
	resbuf* args = acedGetArgs();
	if (!args || args->restype != RTSTR || !args->resval.rstring) { acedRetNil(); return RTNORM; }
	AcString code(args->resval.rstring);
	acedRetNil();
	ensurePyEngine();
	if (!g_pyEngineReady) { acutPrintf(L"\n[PYEXEC] Python 引擎未就绪，代码未执行\n"); return RTNORM; }
	acutPrintf(L"\n[PYEXEC] 直接执行代码\n");
	PyEngine_ExecString(code.kwszPtr());
	acutPrintf(L"[PYEXEC] 完成\n");
	return RTNORM;
}

static void pyPipGo() {
	ensurePyEngine();
	if (!g_pyEngineReady) return;
	::acedInitGet(0, L"install list show uninstall update upgrade");
	ACHAR sub[32] = {};
	if (::acedGetKword(L"PIPM 子命令 [install/list/show/uninstall/update/upgrade]: ", sub) != RTNORM) return;
	AcString op(sub);
	if (op != L"install" && op != L"list" && op != L"show" &&
		op != L"uninstall" && op != L"update" && op != L"upgrade") {
		acutPrintf(L"\n[PIPM] 未知子命令: %ls", op.kwszPtr());
		return;
	}
	std::vector<const wchar_t*> pkgs;
	if (op != L"list" && op != L"update") {
		ACHAR p[1024] = {};
		if (::acedGetString(0, L"包名（空格分隔多个）: ", p) != RTNORM) return;
		AcString cur;
		for (const wchar_t* cp = p; cp && *cp; ++cp) {
			const wchar_t c = *cp;
			if (c == L' ' || c == L',' || c == L'\t') {
				if (!cur.isEmpty()) { pkgs.push_back(cur.kwszPtr()); cur = AcString(); }
			}
			else cur += c;
		}
		if (!cur.isEmpty()) pkgs.push_back(cur.kwszPtr());
	}
	int rc = PyEngine_PipRun(op.kwszPtr(), pkgs.empty() ? nullptr : pkgs.data(), (int)pkgs.size());
	acutPrintf(L"\n[PIPM] %ls 返回码 = %d", op.kwszPtr(), rc);
}

static void pyDebugGo() {
	ensurePyEngine();
	auto* e = PyEngine::instance();
	if (!e) return;
	bool ok = e->ExecString("from PyArxCore.Debug import debugAttach; debugAttach(5678)");
	acutPrintf(L"\n[PYDEBUG] debugpy 已在 127.0.0.1:5678 监听（in-process adapter），请在 VSCode 选择『Python: Attach to AutoCAD』附加后执行脚本（脚本首行放 PyArxCore.Debug.debugBreak() 可停在首行）。%ls\n",
		ok ? L"已启动。" : L"启动失败：请确认 debugpy 已安装（PIPM install debugpy）。");
}

// 侧栏：CAdUiPaletteSet 归 PyArxCore.Ap（CAD 侧），页内控件由 PyMfcCore 铺
// 内联文件日志（带 flush）
static void palLog(const wchar_t* msg) {
	FILE* pf = nullptr;
	_wfopen_s(&pf, pyarxLogFile(L"pyengine.log").c_str(), L"a, ccs=UTF-8");
	if (pf) {
		fwprintf(pf, L"[%lu] %ls\n", GetTickCount(), msg);
		fflush(pf);
		fclose(pf);
	}
}



// ---------------------------------------------------------------------------
// PYARXDEPLOY：把依赖 DLL 目录写入 AutoCAD 支持文件搜索路径（SFSP）。
// ---------------------------------------------------------------------------
static void pyArxDeployGo() {
	ensurePyEngine();
	if (!g_pyEngineReady) return;

	wchar_t modulePath[MAX_PATH] = { 0 };
	GetModuleFileNameW(g_arxHandle, modulePath, MAX_PATH);
	std::filesystem::path arxDir(modulePath);
	arxDir = arxDir.parent_path();

	std::vector<std::wstring> dirs;
	dirs.push_back(arxDir.wstring());
	dirs.push_back(L"C:\\Program Files\\Autodesk\\AutoCAD 2024");

	struct resbuf* cur = acutNewRb(RTSTR);
	acedGetVar(L"ACAD", cur);
	std::wstring curPath;
	if (cur && cur->restype == RTSTR && cur->resval.rstring) {
		curPath = cur->resval.rstring;
		acutRelRb(cur);
	}

	std::wstring newPath = curPath;
	for (const auto& d : dirs) {
		if (newPath.find(d) == std::wstring::npos) {
			if (!newPath.empty() && newPath.back() != L';') newPath += L';';
			newPath += d;
		}
	}

	resbuf rb;
	rb.restype = RTSTR;
	rb.resval.rstring = const_cast<wchar_t*>(newPath.c_str());
	if (acedSetVar(L"ACAD", &rb) == RTNORM) {
		acutPrintf(L"\n[PYARXDEPLOY] 已把以下目录加入支持文件搜索路径:\n");
		for (const auto& d : dirs) acutPrintf(L"  %ls\n", d.c_str());
	}
	else {
		acutPrintf(L"\n[PYARXDEPLOY] acedSetVar 失败（无头环境可能不支持，需手动写注册表）。\n");
	}
}

// ---------------------------------------------------------------------------
// 卸载释放：顺序 = 清 palette → PyCmd.clear → 清 sys.modules →
//   Py_FinalizeEx → Toolhelp32 枚举 FreeLibrary .pyd 句柄
// ---------------------------------------------------------------------------
static void unloadRelease() {
	palLog(L"[UNLOAD] 进入 unloadRelease");

	if (g_pyEngineReady) {
		PyGILState_STATE gstate = PyGILState_Ensure();
		try {
			try {
				py::module_::import("PyMfcCore").attr("shutdown")();
			}
			catch (...) { palLog(L"[UNLOAD] PyMfcCore.shutdown 失败（已忽略）"); }

			try {
				auto core = py::module_::import("PyArxCore");
				core.attr("Cmd").attr("PyCmd").attr("clear")();
			}
			catch (...) { palLog(L"[UNLOAD] PyCmd.clear 失败（已忽略）"); }

			try {
				auto core = py::module_::import("PyArxCore");
				core.attr("Debug").attr("debug_stop")();
			}
			catch (...) { palLog(L"[UNLOAD] Debug.debug_stop 失败（已忽略）"); }

			{
				try {
					auto sys = py::module_::import("sys");
					auto modules = sys.attr("modules");
					for (auto name : {
						std::string{"PyArxCore"},
						std::string{"PyArxCore.Ge"}, std::string{"PyArxCore.Db"},
						std::string{"PyArxCore.Ed"}, std::string{"PyArxCore.Rx"},
						std::string{"PyArxCore.Cmd"}, std::string{"PyArxCore.Gi"},
						std::string{"PyArxCore.Enum"}, std::string{"PyArxCore.Redir"},
						std::string{"PyArxCore.Core"}, std::string{"PyArxCore.Utility"},
						std::string{"PyArxCore.Debug"}
						}) {
						if (py::hasattr(modules, name.c_str())) {
							py::delattr(modules, name.c_str());
						}
					}
				}
				catch (...) { palLog(L"[UNLOAD] sys.modules 清理 PyArxCore 失败（已忽略）"); }
			}

			PyGILState_Release(gstate);
		}
		catch (...) {
			palLog(L"[UNLOAD] 卸载过程出现意外异常（已忽略，强制释放 GIL）");
			PyGILState_Release(gstate);
		}

		g_pyEngineReady = false;

		bool mfcLoaded = false;
		try {
			auto mfc = py::module_::import("PyMfcCore");
			mfcLoaded = true; // PyMfcCore 已加载，软卸载避免 MFC 对象析构崩溃
		}
		catch (...) { palLog(L"[UNLOAD] PyMfcCore 探测 失败（已忽略）"); }

		if (mfcLoaded) {
			// 软卸载：解释器、PyArxCore 模块、PyMfcCore 全部保留到进程结束。
			// 再次 arxload 时 PyEngine::instance() 复用现有解释器与模块。
			acutPrintf(L"\n[PyArx] 已软卸载（PyMfcCore 保留，避免 MFC 对象析构崩溃）。\n");
		}
		else {
			PyEngine_Release();

			HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, GetCurrentProcessId());
			if (hSnap != INVALID_HANDLE_VALUE) {
				MODULEENTRY32W me;
				me.dwSize = sizeof(me);
				if (Module32FirstW(hSnap, &me)) {
					do {
						std::wstring name = me.szModule;
						if (name == L"PyArxCore.pyd") {
							FreeLibrary(me.hModule);
						}
					} while (Module32NextW(hSnap, &me));
				}
				CloseHandle(hSnap);
			}
		}
	}

	palLog(acedRegCmds->lookupGlobalCmd(L"PYRUNFILE") ? L"[UNLOAD] removeGroup 前命令仍在" : L"[UNLOAD] removeGroup 前命令已不在");
	Acad::ErrorStatus st = acedRegCmds->removeGroup(kCmdGroup);
	palLog(st == Acad::eOk ? L"[UNLOAD] removeGroup 返回 eOk" : L"[UNLOAD] removeGroup 返回非 eOk");
	palLog(acedRegCmds->lookupGlobalCmd(L"PYRUNFILE") ? L"[UNLOAD] removeGroup 后命令仍在（残留）" : L"[UNLOAD] removeGroup 后命令已清除");
}

// ---------------------------------------------------------------------------
// ARX 入口
// ---------------------------------------------------------------------------
extern "C" AcRx::AppRetCode acrxEntryPoint(AcRx::AppMsgCode msg, void* appId) {
	switch (msg) {
	case AcRx::kInitAppMsg:
		acrxUnlockApplication(appId);
		acrxRegisterAppMDIAware(appId);
		g_arxHandle = reinterpret_cast<HMODULE>(appId);

		{
			const std::wstring mark = pyarxLogFile(L"loaded_PYARXENGINE.txt");
			HANDLE h = CreateFileW(mark.c_str(), GENERIC_WRITE, 0, nullptr,
				CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
			if (h != INVALID_HANDLE_VALUE) {
				const char* txt = "PyArxEngine loaded (lazy init, dynamic python315t.dll, self-contained)";
				DWORD wr = 0;
				WriteFile(h, txt, (DWORD)strlen(txt), &wr, nullptr);
				CloseHandle(h);
			}
		}
		palLog(L"[INIT] 写入加载标记完成，开始注册命令");
		// 软重载时上一轮 removeGroup 可能未真正清掉组，残留会让 addCommand 全部失败
		// （后果：PYRUNFILE 未注册，LISP (command "PYRUNFILE" ...) 卡在命令行输入）。
		if (acedRegCmds->lookupGlobalCmd(L"PYRUNFILE") != nullptr) {
			palLog(L"[INIT] 检测到残留命令组，先清理");
			palLog(acedRegCmds->removeGroup(kCmdGroup) == Acad::eOk ? L"[INIT] 残留组清理 eOk" : L"[INIT] 残留组清理失败");
		}
		else {
			palLog(L"[INIT] 无残留命令组");
		}

		try {

			palLog(acedRegCmds->addCommand(kCmdGroup, L"PYLOAD", L"PYLOAD", ACRX_CMD_MODAL, &pyload) == Acad::eOk ? L"[INIT] PYLOAD ok" : L"[INIT] PYLOAD 失败");
			palLog(acedRegCmds->addCommand(kCmdGroup, L"PYRUN", L"PYRUN", ACRX_CMD_MODAL, &pyRunFileGo) == Acad::eOk ? L"[INIT] PYRUN ok" : L"[INIT] PYRUN 失败");
			palLog(acedRegCmds->addCommand(kCmdGroup, L"PYEXEC", L"PYEXEC", ACRX_CMD_MODAL, &pyExecGo) == Acad::eOk ? L"[INIT] PYEXEC ok" : L"[INIT] PYEXEC 失败");
			palLog(acedRegCmds->addCommand(kCmdGroup, L"PIPM", L"PIPM", ACRX_CMD_MODAL, &pyPipGo) == Acad::eOk ? L"[INIT] PIPM ok" : L"[INIT] PIPM 失败");
			palLog(acedRegCmds->addCommand(kCmdGroup, L"PYDEBUG", L"PYDEBUG", ACRX_CMD_MODAL, &pyDebugGo) == Acad::eOk ? L"[INIT] PYDEBUG ok" : L"[INIT] PYDEBUG 失败");
			palLog(acedRegCmds->addCommand(kCmdGroup, L"PYARXDEPLOY", L"PYARXDEPLOY", ACRX_CMD_MODAL, &pyArxDeployGo) == Acad::eOk ? L"[INIT] PYARXDEPLOY ok" : L"[INIT] PYARXDEPLOY 失败");
			palLog(acedRegCmds->addCommand(kCmdGroup, L"PYRUNFILE", L"PYRUNFILE", ACRX_CMD_MODAL, &pyRunFileGo) == Acad::eOk ? L"[INIT] PYRUNFILE ok" : L"[INIT] PYRUNFILE 失败");

			palLog(L"[INIT] 命令注册全部完成");
		}

		catch (const std::exception& e) {
			acutPrintf(L"\n[PyArx] 命令注册异常: %s", AcString(e.what(), AcString::Utf8));
		}
		catch (...) {
			acutPrintf(L"\n[PyArx] 命令注册未知异常");
		}
		break;

	case AcRx::kUnloadAppMsg:
		unloadRelease();
		break;
	}
	return AcRx::kRetOK;
}
