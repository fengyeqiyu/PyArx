// PyArxQmlLog.cpp 与 PyArxQml.cpp / QmlBackend.cpp 一起编译。
#include "PyArxQmlLog.h"
#include "windows.h"
#include <cstdio>

static FILE* s_log = nullptr;
static char  s_path[MAX_PATH] = "G:\\PyArx\\docs\\_pyarx_qml.log";

void PyArxQmlSetLogPath(const char* path)
{
    if (s_log) { std::fclose(s_log); s_log = nullptr; }
    if (path && path[0]) std::snprintf(s_path, sizeof(s_path), "%s", path);
}

void LogWrite(const char* fmt, ...)
{
    if (!s_log) s_log = std::fopen(s_path, "a");
    if (!s_log) return;

    SYSTEMTIME st;
    ::GetLocalTime(&st);
    std::fprintf(s_log, "%02d:%02d:%02d.%03d  ", st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);

    va_list ap;
    va_start(ap, fmt);
    std::vfprintf(s_log, fmt, ap);
    va_end(ap);
    std::fprintf(s_log, "\n");
    std::fflush(s_log);
}
