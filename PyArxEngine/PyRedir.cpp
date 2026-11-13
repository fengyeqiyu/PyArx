#include "stdafx.h"
#include "PyRedir.h"
#include "PyEngine.h"
#include "PyString.h"   // type_caster<AcString>：ArxOutPut::write(AcString) ↔ Python str
#include <acdocman.h>
#include <aced.h>
#include <gepnt3d.h>
#include <cwctype>
#include <fstream>
#include <memory>




// =========================================================================
// PyRedir 仅保留：ArxOutPut（输出重定向） + pip 系列（pip 包管理）
// 事务统一使用 PyDb 模块的 PyDbTrans（免 with 由 PyDbTrans 析构提交承载）
// =========================================================================

void bind_ArxOutPut(py::module& m)
{
	py::class_<ArxOutPut>(m, "ArxOutPut")
		.def(py::init<>())           // 必须绑定无参构造：redirectOut 需实例化并赋给 sys.stdout/stderr
		.def("write", &ArxOutPut::write)
		.def("flush", &ArxOutPut::flush);
}

void bind_PyRedir(pym& m) {
	m.doc() = "AutoCAD ARX Python Utilities"; // 添加模块文档

	// 顺序关键：PyEd 的 acedSSGet 默认参数 filter=TVList 依赖 PyDb 先注册
	// TVList，故必须先 import PyDb 再 import PyEd。
	// 但这些 import 失败不应致命——仅影响依赖类型的绑定，不影响 ArxOutPut 基础输出。
	try { py::module_::import("PyArxCore.Ge"); } catch (...) {}
	try { py::module_::import("PyArxCore.Db"); } catch (...) {}
	try { py::module_::import("PyArxCore.Ed"); } catch (...) {}
	try { py::module_::import("PyArxCore.Gi"); } catch (...) {}

	bind_ArxOutPut(m);
}

// ===========================================================================
// pip 系列 —— 归属 PyArxEngine（python 环境设置：pip 包管理）。
// 挂载到 PyArxEngine 顶层（bind_PyPip 由 redirectOut 对 PyArxEngine 模块调用），
// 与 Redir（输出重定向）、Cmd（命令管理）平级；不再嵌套在 Redir 子模块。
// ===========================================================================
void bind_PyPip(pym& m) {
	// 拆分包名字符串（空格 / 逗号 / Tab 分隔）为列表
	auto splitPkgs = [](const AcString& s) {
		std::vector<AcString> v;
		AcString cur;
		for (const wchar_t* p = s.kwszPtr(); p && *p; ++p) {
			const wchar_t c = *p;
			if (c == L' ' || c == L',' || c == L'\t') {
				if (!cur.isEmpty()) { v.push_back(cur); cur = AcString(); }
			}
			else cur += c;
		}
		if (!cur.isEmpty()) v.push_back(cur);
		return v;
	};

	// 完整 pip 包管理器（统一走 PyEngine::pipRun，目标 <arx目录>\Lib\site-packages）
	m.def("pip_install",
		[splitPkgs](const AcString& packages) {
			return PyEngine::instance()->pipRun(L"install", splitPkgs(packages)) == 0;
		},
		py::arg("packages"),
		"安装第三方包到 <arx目录>\\Lib\\site-packages，返回是否成功。");
	m.def("pip_uninstall",
		[splitPkgs](const AcString& packages) {
			return PyEngine::instance()->pipRun(L"uninstall", splitPkgs(packages)) == 0;
		},
		py::arg("packages"),
		"卸载第三方包（从 <arx目录>\\Lib\\site-packages）。");
	m.def("pip_list",
		[]() {
			return PyEngine::instance()->pipRun(L"list", {}) == 0;
		},
		"列出 <arx目录>\\Lib\\site-packages 已安装的包（pip 输出到 CAD 命令行）。");
	m.def("pip_show",
		[splitPkgs](const AcString& packages) {
			return PyEngine::instance()->pipRun(L"show", splitPkgs(packages)) == 0;
		},
		py::arg("packages"),
		"查看包详细信息（pip 输出到 CAD 命令行）。");
	m.def("pip_upgrade",
		[splitPkgs](const AcString& packages) {
			return PyEngine::instance()->pipRun(L"upgrade", splitPkgs(packages)) == 0;
		},
		py::arg("packages"),
		"升级第三方包到最新版。");
	m.def("pip_update",
		[]() {
			return PyEngine::instance()->pipRun(L"upgrade", { AcString(L"pip") }) == 0;
		},
		"更新 pip 自身到最新版。");
	m.def("pip_download",
		[splitPkgs](const AcString& packages) {
			return PyEngine::instance()->pipRun(L"download", splitPkgs(packages)) == 0;
		},
		py::arg("packages"),
		"下载包（不安装）到 <arx目录>\\Lib\\site-packages。");
	m.def("pip_freeze",
		[]() {
			return PyEngine::instance()->pipRun(L"freeze", {}) == 0;
		},
		"列出已装包冻结格式（pip freeze）。");
	m.def("pip_check",
		[]() {
			return PyEngine::instance()->pipRun(L"check", {}) == 0;
		},
		"检查依赖冲突（pip check）。");
	m.def("pip_wheel",
		[splitPkgs](const AcString& packages) {
			return PyEngine::instance()->pipRun(L"wheel", splitPkgs(packages)) == 0;
		},
		py::arg("packages"),
		"构建 wheel 包。");
	m.def("pip_cache",
		[](const AcString& sub) {
			return PyEngine::instance()->pipRun(L"cache", { sub }) == 0;
		},
		py::arg("sub"),
		"pip cache 子命令（list/purge/info）。");
}
