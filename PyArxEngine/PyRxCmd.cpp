#include "stdafx.h"
#include "PyRxCmd.h"

//------------------------------------------------------------------------------------------------
// 静态数据定义
std::map<AcString, PyCommandCall> PyCmd::s_cmdDict;

//------------------------------------------------------------------------------------------------
// 构造
PyCommandCall::PyCommandCall(const AcString& name,
    std::function<void()> fun,
    int flags,
    std::source_location loc)
    : cmdName(name), func(std::move(fun)), cmdFlags(flags), regLoc(loc)
{
    cmdName.makeUpper();
}

PyCommandCall::PyCommandCall(const AcString& name,
    std::function<void()> fun,
    int flags,
    const AcString& moduleName,
    const AcString& funcName,
    std::source_location loc)
    : cmdName(name), func(std::move(fun)), cmdFlags(flags), regLoc(loc),
      moduleName(moduleName), funcName(funcName)
{
    cmdName.makeUpper();
}

PyCommandCall::PyCommandCall(const AcString& name,
    std::function<void()> fun,
    int flags,
    const AcString& moduleName,
    const AcString& funcName,
    const AcString& scriptPath,
    std::source_location loc)
    : cmdName(name), func(std::move(fun)), cmdFlags(flags), regLoc(loc),
      moduleName(moduleName), funcName(funcName), scriptPath(scriptPath)
{
    cmdName.makeUpper();
}

//------------------------------------------------------------------------------------------------
// 统一异常处理
void PyCommandCall::invoke() const {
    try {
        if (func) func();
    }
    catch (const std::exception& e) {
        AcString msg(e.what(), AcString::Utf8);
        AcString file(regLoc.file_name(), AcString::Utf8);
        AcString fun(regLoc.function_name(), AcString::Utf8);

        acutPrintf(L"\nerror: %s\n  at %s:%d in %s\n",
            msg, file, (int)regLoc.line(), fun);
    }
    catch (...) {
        AcString file(regLoc.file_name(), AcString::Utf8);
        AcString fun(regLoc.function_name(), AcString::Utf8);

        acutPrintf(L"\nunknown exception in command %s\n  at %s:%d in %s\n",
            cmdName, file, (int)regLoc.line(), fun);
    }
}

//------------------------------------------------------------------------------------------------
AcString PyCmd::getGroupName() {
    std::filesystem::path dllPath((const wchar_t*)acedGetAppName());
    return dllPath.stem().wstring().c_str();
}

AcString PyCmd::getCurCmdName() {
    AcString name;
    if (acedGetCommandForDocument(curDoc(), name) != eOk)
        return {};
    name.makeUpper();
    return name;
}

//------------------------------------------------------------------------------------------------
// CAD 回调：用"当前命令名"反查 std::function 并执行
void PyCmd::CallStdFunc() {
    AcString name = getCurCmdName();
    if (name.isEmpty()) {
        acutPrintf(L"\nCallStdFunc: no current command\n");
        return;
    }

    auto it = s_cmdDict.find(name);
    if (it == s_cmdDict.end()) {
        acutPrintf(L"\nCommand (%s) not found\n", name);
        return;
    }
    it->second.invoke();
}

//------------------------------------------------------------------------------------------------
// 添加命令
Acad::ErrorStatus PyCmd::addCmd(const AcString& name,
    std::function<void()> fun,
    int flags,
    std::source_location loc)
{
    return addCmd(name, std::move(fun), flags, AcString(), AcString(), loc);
}

Acad::ErrorStatus PyCmd::addCmd(const AcString& name,
    std::function<void()> fun,
    int flags,
    const AcString& moduleName,
    const AcString& funcName,
    std::source_location loc)
{
    PyCommandCall call(name, std::move(fun), flags, moduleName, funcName, loc);

    if (s_cmdDict.contains(call.cmdName)) {
        acedRegCmds->removeCmd(getGroupName(), call.cmdName);
        s_cmdDict.erase(call.cmdName);
    }

    AcString group = getGroupName();
    if (auto es = acedRegCmds->addCommand(
        group, call.cmdName, call.cmdName, flags, &CallStdFunc); es != eOk)
    {
        acutPrintf(L"\naddCommand failed: %s\n", acadErrorStatusText(es));
        return es;
    }

    s_cmdDict.emplace(call.cmdName, std::move(call));
    return Acad::eOk;
}

Acad::ErrorStatus PyCmd::addCmd(const AcString& name,
    std::function<void()> fun,
    int flags,
    const AcString& moduleName,
    const AcString& funcName,
    const AcString& scriptPath,
    std::source_location loc)
{
    PyCommandCall call(name, std::move(fun), flags, moduleName, funcName, scriptPath, loc);

    if (s_cmdDict.contains(call.cmdName)) {
        acedRegCmds->removeCmd(getGroupName(), call.cmdName);
        s_cmdDict.erase(call.cmdName);
    }

    AcString group = getGroupName();
    if (auto es = acedRegCmds->addCommand(
        group, call.cmdName, call.cmdName, flags, &CallStdFunc); es != eOk)
    {
        acutPrintf(L"\naddCommand failed: %s\n", acadErrorStatusText(es));
        return es;
    }

    s_cmdDict.emplace(call.cmdName, std::move(call));
    return Acad::eOk;
}

//------------------------------------------------------------------------------------------------
// 删除命令
Acad::ErrorStatus PyCmd::removeCmd(const AcString& name) {
    AcString key = name;
    key.makeUpper();

    auto it = s_cmdDict.find(key);
    if (it == s_cmdDict.end())
        return Acad::eKeyNotFound;

    if (auto es = acedRegCmds->removeCmd(getGroupName(), key); es != eOk)
        acutPrintf(L"\nremoveCmd failed: %s\n", acadErrorStatusText(es));

    s_cmdDict.erase(it);
    return Acad::eOk;
}

//------------------------------------------------------------------------------------------------
// 清空命令
void PyCmd::clear() {
    if (auto es = acedRegCmds->removeGroup(getGroupName()); es != eOk)
        acutPrintf(L"\nremoveGroup failed: %s\n", acadErrorStatusText(es));

    s_cmdDict.clear();
}