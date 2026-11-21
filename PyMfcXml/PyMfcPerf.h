// 性能计数（仅供审计/压测用，PYARX_PERF=1 时才真正计时，默认零开销）
// 用法：在热路径里放一个 PyMfcPerfScope 对象，函数返回时自动累加耗时与调用次数；
// 结果由 Python 侧的 host.perfStats() 读出。
#pragma once
#include <windows.h>

struct PyMfcPerfCounters
{
    double createMs = 0.0, apMs = 0.0, paintMs = 0.0, hostPaintMs = 0.0, layoutMs = 0.0;
    double parseMs = 0.0, declMs = 0.0, loadMs = 0.0, showMs = 0.0;
    double elemMs = 0.0, apOfMs = 0.0;
    double cweMs = 0.0, themeMs = 0.0, fontMs = 0.0;
    double shotMs = 0.0, encMs = 0.0;
    long long nCreate = 0, nAp = 0, nPaint = 0, nHostPaint = 0, nLayout = 0;
    long long nParse = 0, nDecl = 0, nLoad = 0, nShow = 0;
    long long nElem = 0, nApOf = 0;
    long long nCwe = 0, nTheme = 0, nFont = 0;
    long long nShot = 0, nEnc = 0;
};

PyMfcPerfCounters& pyMfcPerf();

inline bool pyMfcPerfEnabled()
{
    static int cached = -1;
    if (cached < 0)
    {
        wchar_t buf[8] = {};
        cached = ::GetEnvironmentVariableW(L"PYARX_PERF", buf, 8) > 0 ? 1 : 0;
    }
    return cached == 1;
}

inline double pyMfcPerfNow()
{
    LARGE_INTEGER f, t;
    ::QueryPerformanceFrequency(&f);
    ::QueryPerformanceCounter(&t);
    return 1000.0 * static_cast<double>(t.QuadPart) / static_cast<double>(f.QuadPart);
}

struct PyMfcPerfScope
{
    double* target;
    long long* count;
    double t0;
    bool on;
    PyMfcPerfScope(double* t, long long* c)
        : target(t), count(c), t0(0.0), on(pyMfcPerfEnabled())
    {
        if (on) t0 = pyMfcPerfNow();
    }
    ~PyMfcPerfScope()
    {
        if (!on) return;
        *target += pyMfcPerfNow() - t0;
        ++*count;
    }
};
