#pragma once
#include "stdafx.h"
#include "singleInstance.h"
#include <filesystem>
#include <vector>

// ===========================================================================
// PyEngine —— AutoCAD 内嵌 Python 解释器引擎（单例）
//
// 职责：负责 Python 3.15t 解释器在 ARX 进程内的完整生命周期——
//   - 构造：定位运行时根、校验标准库、配置 PyConfig、初始化解释器、
//           重定向 print 到 CAD 命令行、接入虚拟环境 site-packages；
//   - 运行：执行脚本文件 / 代码字符串 / pip 安装第三方包；
//   - 卸载：安全释放 pybind 对象与解释器。
//
// 设计约定（项目规范）：
//   - SDK / Python 调用失败不抛异常、不崩溃；致命错误抛异常由 acrxEntryPoint
//     入口捕获，非致命错误 acutPrintf 打印到 CAD 命令行并返回失败值；
//   - 所有 setter/动作方法返回 bool（成功 true / 失败 false）；查询失败返回默认值；
//   - 对外接口保持稳定（acrxEntryPoint / PyArxUtils 依赖）。
// ===========================================================================
class PyEngine : public SingleInstance<PyEngine> {
public:
	// 单例：构造即初始化解释器（失败抛异常，由入口捕获）
	PyEngine();
	~PyEngine();

	// 运行 Python 脚本文件（PYRUNFILE 命令入口）
	bool RunScript(const Path& scriptPath);

	// 执行 Python 代码字符串（PYEXEC 命令入口）；失败 acutPrintf 并返回 false
	bool ExecString(const AcString& code);

	// 通用 pip 安装第三方包；强制安装到 <arxDir>\Lib\site-packages。
	// 失败 acutPrintf 并返回 false，不抛异常、不崩溃。
	bool pipInstall(const AcString& packages);

	// 通用 pip 包管理：operation = install / uninstall / list / show / upgrade / download。
	// packages 为包名列表（list 可为空）。install/upgrade 强制 --target 到 <arxDir>\Lib\site-packages。
	// pip 输出重定向到 CAD 命令行；返回 pip 返回码（0=成功，-1=异常/失败）。
	int pipRun(const AcString& operation, const std::vector<AcString>& packages);

	// 调试日志：写 <arx 目录>\pyengine.log（UTF-8），崩溃后仍可定位，不依赖 CAD 命令行
	static void dbgLog(const wchar_t* fmt, ...);

	// 设置 ARX 模块句柄（acrxEntryPoint kInitAppMsg 传入的 appId），供 currentAppDir 用
	// GetModuleFileNameW 定位 arx 路径。ObjectARX 由 acrxDynamicLinker 加载，不在标准模块表，
	// 无法用 GetModuleHandleW 按名查到；须在 arxload 时由入口传入并保存。
	static void setArxHandle(HMODULE hMod);

	// 供 PyEngine.cpp currentAppDir 读取（匿名 namespace 自由函数非类成员，不能直接访问 private 静态成员）
	static HMODULE arxHandle() { return s_arxHandle; }

private:
	void initConfig();     // 初始化 PyConfig（隔离式，锁定运行根与搜索路径）
	void redirectOut();    // 把 sys.stdout/sys.stderr 重定向到 ArxOutPut
	void setupVenv();      // 创建并接入 <arxDir>\Lib\site-packages

	Path appDir;           // ARX 所在目录（配置/环境根）
	Path arxDir;           // ARX 目录（虚拟环境根，site-packages 所在处）
	Path pyHome;           // Python 标准库根（校验 python315t.dll + Lib\encodings）
	PyConfig config;       // Python 解释器配置
	pym py_sys;            // sys 模块句柄（redirectOut 持有）

	static HMODULE s_arxHandle;  // ARX 模块句柄（acrxEntryPoint kInitAppMsg 传入）
};
