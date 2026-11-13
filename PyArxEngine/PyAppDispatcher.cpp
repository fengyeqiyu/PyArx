// PyAppDispatcher.cpp — 把 Python callable 投递到 AutoCAD command/application context
// 执行（PyRx AcApDocManager 同款机制）。归属 PyArxEngine。
// 编码：UTF-8 带 BOM。
#include "stdafx.h"
#include "PyAppDispatcher.h"
#include "logPath.h"
#include <acdocman.h>   // acDocManager 全局（AcApDocManager*），executeIn*Context API
#include <aced.h>

namespace {

// 便捷诊断：统一追加写 <exe>\log\pyengine.log（可用 PYARX_LOG_DIR 覆盖，UTF-8），
// 不依赖 CAD 命令行（崩溃后可回溯），与 PyEngine::dbgLog 同款实现。
void dispatchLog(const wchar_t* fmt, ...) {
    FILE* pf = nullptr;
    if (_wfopen_s(&pf, pyarxLogFile(L"pyengine.log").c_str(), L"a, ccs=UTF-8") == 0 && pf) {
        va_list args;
        va_start(args, fmt);
        fwprintf(pf, L"[%lu] ", GetTickCount());
        vfwprintf(pf, fmt, args);
        fwprintf(pf, L"\n");
        va_end(args);
        fclose(pf);
    }
}

// 把 py::object 里的异常转成可打印串（error_already_set → what()）
inline const wchar_t* PyErrToW(const py::error_already_set& e) {
    static thread_local std::wstring s_buf;
    s_buf = (const wchar_t*)AcString(e.what(), AcString::Utf8);
    return s_buf.c_str();
}

} // namespace

PyAppDispatcher& PyAppDispatcher::Instance() {
    static PyAppDispatcher inst;
    return inst;
}

void PyAppDispatcher::Dispatch(py::function func, py::args args, DispatchContext ctx) {
    // 调用方已持有 GIL（按钮回调由 Python 侧进入本 C++，必然持 GIL）。
    // 这里只做 push + 请求 CAD 调度，不主动调 Python、不释放 GIL。
    Task t{ func, py::tuple(args) };
    PushAndRequest(std::move(t), ctx);
}

void PyAppDispatcher::PushAndRequest(Task&& t, DispatchContext ctx) {
    {
        std::lock_guard<std::mutex> lk(m_mutex);
        m_queue.push_back(std::move(t));
    }

    AcApDocManager* pMgr = acDocManager;
    if (!pMgr) {
        dispatchLog(L"[DISPATCH] acDocManager 为空，任务丢弃（无头环境或宿主未就绪）");
        // 队列里的 Task 强引用会在 Clear()/析构时释放；此处直接回滚本次入队
        std::lock_guard<std::mutex> lk(m_mutex);
        m_queue.pop_back();
        return;
    }

    switch (ctx) {
    case DispatchContext::Application:
        // 同步：阻塞到 RunOneTrampoline 跑完。注意调用方在 idle 泵里，
        // 同步会让 idle 泵阻塞一会儿（这正是 PyRx executeInApplicationContext 的语义）。
        dispatchLog(L"[DISPATCH] executeInApplicationContext 请求");
        pMgr->executeInApplicationContext(&PyAppDispatcher::RunOneTrampoline, nullptr);
        dispatchLog(L"[DISPATCH] executeInApplicationContext 完成");
        break;
    case DispatchContext::BeginApplication:
        dispatchLog(L"[DISPATCH] beginExecuteInApplicationContext 请求");
        pMgr->beginExecuteInApplicationContext(&PyAppDispatcher::RunOneTrampoline, nullptr);
        break;
    case DispatchContext::BeginCommand:
        // 最推荐：把任务当成一条新命令排队，CAD 命令行空闲时自动跑，
        // 文档自动拿到命令锁，里面 PyDbTrans() 安全。异步，idle 泵立即返回。
        dispatchLog(L"[DISPATCH] beginExecuteInCommandContext 请求");
        pMgr->beginExecuteInCommandContext(&PyAppDispatcher::RunOneTrampoline, nullptr);
        break;
    default:
        dispatchLog(L"[DISPATCH] 未知 DispatchContext");
        std::lock_guard<std::mutex> lk(m_mutex);
        m_queue.pop_back();
        break;
    }
}

void PyAppDispatcher::RunOneTrampoline(void* /*pv*/) {
    // CAD 回调进到这里：application context 或 command context，主线程。
    // 此时 GIL 可能未被本线程持有（idle 泵的 gil_scoped_acquire 在 Yield 返回后
    // 已析构），必须显式 Ensure。
    PyGILState_STATE gstate = PyGILState_Ensure();
    try {
        Task t;
        {
            // RunOneTrampoline 是 static 回调：实例状态统一从 Instance() 取。
            auto& self = PyAppDispatcher::Instance();
            std::lock_guard<std::mutex> lk(self.m_mutex);
            if (self.m_queue.empty()) {
                PyGILState_Release(gstate);
                return;
            }
            t = std::move(self.m_queue.front());
            self.m_queue.pop_front();
        }
        dispatchLog(L"[DISPATCH] RunOneTrampoline 取到任务，开始执行");
        try {
            t.func(*t.args);
        }
        catch (const py::error_already_set& e) {
            acutPrintf(L"\n[PyArx.Dispatch] Python 异常: %s\n", PyErrToW(e));
            dispatchLog(L"[DISPATCH] Python 异常: %s", PyErrToW(e));
        }
        catch (const std::exception& e) {
            acutPrintf(L"\n[PyArx.Dispatch] C++ 异常: %s\n",
                (const wchar_t*)AcString(e.what(), AcString::Utf8));
            dispatchLog(L"[DISPATCH] C++ 异常: %s",
                (const wchar_t*)AcString(e.what(), AcString::Utf8));
        }
        catch (...) {
            acutPrintf(L"\n[PyArx.Dispatch] 未知异常\n");
            dispatchLog(L"[DISPATCH] 未知异常");
        }
        dispatchLog(L"[DISPATCH] RunOneTrampoline 任务执行完成");
    }
    catch (...) {
        dispatchLog(L"[DISPATCH] RunOneTrampoline 外层异常");
    }
    PyGILState_Release(gstate);
}

void PyAppDispatcher::Clear() {
    // 必须在 GIL 持有期间调用（PyArxEngine 卸载路径：unloadRelease 里
    // PyGILState_Ensure 之后、PyEngine_Release 之前）。清空队列使 py::object
    // 引用被释放，避免解释器终结后访问悬挂对象。
    std::lock_guard<std::mutex> lk(m_mutex);
    m_queue.clear();
    dispatchLog(L"[DISPATCH] Clear() 队列已清空");
}
