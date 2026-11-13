#pragma once
// PyDispatchFlags.h — 上下文投递标志（DispatchContext）。
// 归属 PyArxEngine（Dispatch 子模块绑定处）。与 PyCommandFlags.h 同风格放 common，
// 供 PyAppDispatcher 与 PyDispatchBinder 共用。
// 编码：UTF-8 带 BOM。
//
// 背景：侧栏 wx 按钮回调运行在 acedRegisterOnIdleWinMsg 的 idle 回调里（非命令
// 上下文），直接 lockDocument(kWrite) / 改库会死锁。PyArxEngine.Dispatch 把这三种
// 上下文投递语义暴露给 Python，让用户脚本把「改库回调」投递到 AutoCAD 自己的
// 命令/应用上下文执行，天然拿到命令锁，PyDbTrans() 才安全。

// 投递语义：决定 Python callable 被 AcApDocManager 调度到哪种上下文里跑。
enum class DispatchContext {
    Application,        // executeInApplicationContext：同步，application context（阻塞到完成）
    BeginApplication,   // beginExecuteInApplicationContext：异步，application context（立即返回）
    BeginCommand,       // beginExecuteInCommandContext：异步，当作一条新命令排队（推荐，文档自动拿命令锁）
};
