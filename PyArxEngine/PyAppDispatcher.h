#pragma once
#include "stdafx.h"
#include <deque>
#include <mutex>
#include "PyDispatchFlags.h"

// ===========================================================================
// PyAppDispatcher — 把 Python callable 从 modeless 面板/窗体/idle 回调投递到
//   AutoCAD 的 application / command context 执行（PyRx DocManager 同款）。
//
// 背景：侧栏按钮回调运行在 modeless 面板的消息回调里，不是
//   命令上下文，直接 lockDocument(kWrite) / 改库会死锁（见
//   log/fix_design_idle_freeze_20261103.md）。本类把 callable 打包成任务入队，
//   再用 AcApDocManager::executeInApplicationContext / beginExecuteInApplicationContext /
//   beginExecuteInCommandContext 三个 API 让 CAD 自己在正确上下文里回调 RunOneTrampoline。
//
// 线程模型（遵循项目"CAD 主线程单线程 + 按钮串行触发"现实，同时为未来多线程兜底）：
//   - 入队（Dispatch）：按钮回调线程 == CAD 主线程。入队时 GIL 已被
//     Python 侧调用持有（Python 进入本 C++ 必然持 GIL）。
//     Dispatch 只做 push + 调 beginXxx，不主动调 Python、不释放 GIL，立即返回。
//   - 触发（RequestRun）：紧接着调 beginXxx，CAD 把 C 回调排到消息/命令队列。
//   - 执行（RunOneTrampoline）：CAD 在自己的上下文里回调（主线程），此时 GIL 未
//     被本线程持有，必须 PyGILState_Ensure 拿 GIL，再 pop 队列、调 callable。
//
// 生命周期：Task 里的 py::object 用强引用持有，任务跑完自动释放；PyArxEngine 卸载
//   时调用 Clear() 清空队列（在 Py_FinalizeEx 之前，GIL 持有期间），避免悬挂引用。
// 并发安全：std::mutex + std::deque 保护队列；按钮连点 N 次会排队 N 个任务依次跑。
// ===========================================================================
namespace py = pybind11;

class PyAppDispatcher {
public:
    // 单例
    static PyAppDispatcher& Instance();

    // Python 侧入口：把 func + args 投递到指定上下文执行。
    // context = DispatchContext::Application（同步阻塞）/
    //           BeginApplication（异步）/
    //           BeginCommand（异步，命令上下文，推荐）。
    // 调用方必须已持有 GIL（按钮回调天然持有）。
    void Dispatch(py::function func, py::args args, DispatchContext context);

    // C 回调：由 AcApDocManager 在 application/command context 调用，
    // 内部 PyGILState_Ensure 拿 GIL 后从队列取一个任务执行。
    static void RunOneTrampoline(void* /*pv*/);

    // 清空队列（PyArxEngine 卸载 / 解释器终结前调用，须在 GIL 持有期间）。
    void Clear();

private:
    PyAppDispatcher() = default;

    struct Task {
        py::object func;
        py::tuple  args;
    };

    // push 任务 + 请求 CAD 回调 RunOneTrampoline
    void PushAndRequest(Task&& t, DispatchContext ctx);

    std::mutex m_mutex;                 // 队列锁（主线程读写；为未来多线程入队兜底）
    std::deque<Task> m_queue;           // 待执行任务队列（FIFO）
};
