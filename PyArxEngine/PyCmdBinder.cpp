#include "stdafx.h"
#include "PyCmdBinder.h"
#include "PyThreadScope.h"
#include "PyRxCmd.h"
#include "PyString.h"   // type_caster<AcString>：命令名 name:AcString ↔ Python str 自动转换
#include "logPath.h"
#include <acadstrc.h>
#include <fstream>
#include <iterator>

// ---------------------------------------------------------------------------
// 将 PyCmd 类绑定到 pybind，供 Python 脚本注册 / 移除 / 清空 CAD 命令。
// 命令管理归属 PyArxEngine：本文件编译进 PyArxEngine，bind_PyCmd 由 PyEngine
// 在 import PyArxCore 后调用，挂载 PyArxCore.Cmd 子模块（与 Redir 同模式）。
// Acad::ErrorStatus 由 PyArxCore.Enum 模块提供。
// ---------------------------------------------------------------------------
void bind_PyCmd(pym& m) {
	// PyCmd 纯静态命令管理类：绑定静态方法，供 Python 脚本注册命令。
	// 使用 py::nodelete holder：PyCmd 禁止实例化/析构（构造函数与析构均 delete），
	// Python 侧只能调用其静态方法，无法创建 PyCmd 实例。
	// CommandFlags 枚举定义于 PyArxCore 公共头 PyCommandFlags.h，Enum 绑定仍在核心。
	py::class_<PyCmd, std::unique_ptr<PyCmd, py::nodelete>>(m, "PyCmd",
		"AutoCAD 命令管理类（纯静态）：注册/移除/清空命令，供 Python 脚本使用。")
		.def_static("addCmd",
			[](const AcString& name, py::function func, int flags) -> Acad::ErrorStatus {
				// 从 callable 提取模块名/函数名，供 pyreload_all 热更新使用
				AcString modName;
				AcString funcName;
				if (py::hasattr(func, "__module__")) {
					py::object mn = func.attr("__module__");
					if (!mn.is_none()) modName = py::cast<AcString>(mn);
				}
				if (py::hasattr(func, "__name__")) {
					funcName = py::cast<AcString>(func.attr("__name__"));
				}
				return PyCmd::addCmd(name, std::function<void()>(func), flags, modName, funcName);
			},
			py::arg("name"), py::arg("func"),
			py::arg("flags") = static_cast<int>(CommandFlags::UsePickSet | CommandFlags::Redraw),
			"注册命令：PyCmd.addCmd(\"NAME\", callable[, flags])。"
			"name 统一转大写；func 为无参 Python 可调用；返回 ErrorStatus。")
		// 装饰器形式注册命令（命令执行 = 运行被装饰的 Python 函数）
		.def_static("command",
			[](py::object name, int flags) -> py::object {
				// 辅助：按命令名注册并返回原函数（保持 Python 侧仍可直接调用）
				auto reg = [](py::function func, const py::object& cmdNameObj, int fl) -> py::function {
					// 命令名：显式传入 或 用函数 __name__
					AcString cmdName;
					if (!cmdNameObj.is_none() && py::isinstance<py::str>(cmdNameObj))
						cmdName = py::cast<AcString>(cmdNameObj);
					else
						cmdName = py::cast<AcString>(func.attr("__name__"));
					// 提取 moduleName/funcName（供 pyreload_all 热更新）
					AcString modName, funcName;
					if (py::hasattr(func, "__module__")) {
						py::object mn = func.attr("__module__");
						if (!mn.is_none()) modName = py::cast<AcString>(mn);
					}
					if (py::hasattr(func, "__name__"))
						funcName = py::cast<AcString>(func.attr("__name__"));

					// 提取脚本路径：被装饰函数所在 .py 文件（命令触发时运行此脚本 → 断点命中）
					AcString scriptPath;
					if (py::hasattr(func, "__code__")) {
						py::object code = func.attr("__code__");
						if (py::hasattr(code, "co_filename")) {
							py::object cf = code.attr("co_filename");
							if (!cf.is_none()) scriptPath = py::cast<AcString>(cf);
						}
					}

					// 命令执行器：被装饰函数作为"主函数"——
					//   命令触发时「运行对应脚本（exec 顶层代码 → 调试断点命中）→ 执行对应函数」。
					//   每次触发重跑脚本，脚本修改即时生效，利于 debug（attach 调试器可在脚本源码设断点单步）。
					AcString sp = scriptPath, fName = funcName;
					std::function<void()> executor = [sp, fName]() {
						try {
							std::ofstream df(pyarxLogFile(L"exec_sp.txt"), std::ios::app);
							df << "sp=[" << sp.utf8Ptr() << "] fn=[" << fName.utf8Ptr() << "]\n";
							PyThreadScope gil{};                                 // free-threaded：无 GIL，仅确认线程状态
							if (sp.isEmpty())
								throw std::runtime_error("scriptPath 为空，无法运行脚本（装饰器未捕获 co_filename）");
							// 运行对应脚本到独立命名空间（顶层代码 + 函数定义），避免污染 __main__
							py::dict ns;
							// 用 Python 侧 open 读取脚本（对 Windows 路径/正斜杠/编码处理最可靠）
							std::string src;
							{
								py::object srcobj = py::module_::import("builtins").attr("open")(
									py::str(sp.utf8Ptr()), "rb").attr("read")();
								src = py::cast<std::string>(srcobj);
							}
							if (src.empty())
								throw std::runtime_error("脚本文件为空");
							// 用真实脚本路径作为编译文件名，保证脚本内函数 __code__.co_filename 为真实路径，
							// 从而调试器可对该脚本源码设断点/单步；同时 __module__ 不再退化为 "<string>"。
							std::ofstream sf(pyarxLogFile(L"exec_src.txt"));
							sf << "sp=[" << sp.utf8Ptr() << "]";
							sf << " src_head=[" << std::string(src, 0, 200) << "]\n";
							py::object code = py::module_::import("builtins").attr("compile")(
								py::str(src), py::str(sp.utf8Ptr()), "exec");
							py::exec(code, ns, ns);
							// 从脚本命名空间定位对应函数并执行
							py::object fn = ns[py::str(fName.utf8Ptr())];
							fn();
						}
						catch (const py::error_already_set& e) {
							acutPrintf(L"\n[PyArx] 命令执行失败（运行脚本/执行函数）: %hs\n", e.what());
							std::ofstream ef(pyarxLogFile(L"cmd_exec_error.txt"));
							ef << "[pyerr] " << e.what() << "\n";
						}
						catch (const std::exception& e) {
							acutPrintf(L"\n[PyArx] 命令执行失败: %hs\n", e.what());
							std::ofstream ef(pyarxLogFile(L"cmd_exec_error.txt"));
							ef << "[stdex] " << e.what() << " | scriptPath=[" << sp.utf8Ptr() << "]\n";
						}
						catch (...) {
							acutPrintf(L"\n[PyArx] 命令执行抛未知 C++ 异常\n");
							std::ofstream ef(pyarxLogFile(L"cmd_exec_error.txt"));
							ef << "[unknown]\n";
						}
					};
					PyCmd::addCmd(cmdName, executor, fl, modName, fName, sp);
					return func;
				};
				// 形式一：@PyArxEngine.Cmd.command —— name 即被装饰函数，直接用函数名注册
				if (py::isinstance<py::function>(name)) {
					py::function func = py::reinterpret_borrow<py::function>(name);
					return reg(func, py::none(), flags);
				}
				// 形式二：@PyArxEngine.Cmd.command("NAME", flags=...) —— name 为命令名，返回装饰器
				return py::cpp_function([reg, name, flags](py::function func) -> py::function {
					return reg(func, name, flags);
				});
			},
			py::arg("name") = py::none(),
			py::arg("flags") = static_cast<int>(CommandFlags::UsePickSet | CommandFlags::Redraw),
			"装饰器注册命令（命令执行 = 运行被装饰的 Python 函数）：\n"
			"  @PyArxEngine.Cmd.command                          # 直接用函数名注册\n"
			"  @PyArxEngine.Cmd.command(\"MYCMD\")                # 指定命令名（自动转大写）\n"
			"  @PyArxEngine.Cmd.command(\"MYCMD\", flags=CommandFlags.UsePickSet)  # 带标志\n"
			"装饰器返回原函数（Python 中仍可正常调用），命令触发时执行该函数。")
		.def_static("removeCmd",
			[](const AcString& name) -> Acad::ErrorStatus {
				return PyCmd::removeCmd(name);
			},
			py::arg("name"),
			"移除命令：PyCmd.removeCmd(\"NAME\")。返回 ErrorStatus。")
		.def_static("clear",
			&PyCmd::clear,
			"清空本组所有命令（PyCmd.clear()）。");
}
