#pragma once
#include "stdafx.h"
#include "PyCommandFlags.h"   // CommandFlags 已抽为公共头（Enum 模块绑定处）
#include <map>
#include <functional>
#include <source_location>

//------------------------------------------------------------------------------------------------
// 单条命令
struct PyCommandCall {
    AcString cmdName;                    // 统一大写
    std::function<void()> func;
    int cmdFlags = 0;
    std::source_location regLoc;         // 注册位置
    AcString moduleName;                 // 关联 Python 模块名（pyreload 用，可为空）
    AcString funcName;                   // 关联 Python 函数名（pyreload 用，可为空）
    AcString scriptPath;                 // 被装饰函数所在脚本（.py）路径；debug/模块化执行用

    PyCommandCall(const AcString& name,
        std::function<void()> fun,
        int flags,
        std::source_location loc = std::source_location::current());

    PyCommandCall(const AcString& name,
        std::function<void()> fun,
        int flags,
        const AcString& moduleName,
        const AcString& funcName,
        std::source_location loc = std::source_location::current());

    PyCommandCall(const AcString& name,
        std::function<void()> fun,
        int flags,
        const AcString& moduleName,
        const AcString& funcName,
        const AcString& scriptPath,
        std::source_location loc = std::source_location::current());

    PyCommandCall(const PyCommandCall&) = delete;
    PyCommandCall& operator=(const PyCommandCall&) = delete;
    PyCommandCall(PyCommandCall&&) = default;
    PyCommandCall& operator=(PyCommandCall&&) = default;

    void invoke() const;                 // 统一异常处理
};

//------------------------------------------------------------------------------------------------
// 命令管理类：纯静态成员，不提供实例
class PyCmd
{
public:
    static Acad::ErrorStatus addCmd(const AcString& name,
        std::function<void()> fun,
        int flags = CommandFlags::UsePickSet | CommandFlags::Redraw,
        std::source_location loc = std::source_location::current());

    static Acad::ErrorStatus addCmd(const AcString& name,
        std::function<void()> fun,
        int flags,
        const AcString& moduleName,
        const AcString& funcName,
        std::source_location loc = std::source_location::current());

    static Acad::ErrorStatus addCmd(const AcString& name,
        std::function<void()> fun,
        int flags,
        const AcString& moduleName,
        const AcString& funcName,
        const AcString& scriptPath,
        std::source_location loc = std::source_location::current());

    static Acad::ErrorStatus removeCmd(const AcString& name);
    static void clear();

    PyCmd() = delete;
    ~PyCmd() = delete;

private:
    static void CallStdFunc();           // CAD 回调入口
    static AcString getGroupName();
    static AcString getCurCmdName();

    // 纯静态数据
    static std::map<AcString, PyCommandCall> s_cmdDict;   // key = 大写命令名
};

//------------------------------------------------------------------------------------------------
// 注册宏
#define ADD_CMD(name) \
    PyCmd::addCmd(L## #name, &name)