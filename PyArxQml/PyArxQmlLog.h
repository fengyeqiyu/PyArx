// PyArxQmlLog.h/.cpp —— 统一日志出口（默认写 G:\PyArx\docs\_pyarx_qml.log）。
#pragma once
#include <cstdarg>

void PyArxQmlSetLogPath(const char* path);
void LogWrite(const char* fmt, ...);
