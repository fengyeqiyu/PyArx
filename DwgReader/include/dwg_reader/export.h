#pragma once

/// DLL 导入/导出宏：编 DLL 时定义 DWREADER_BUILD_DLL，外部使用方默认 dllimport。
#ifdef DWREADER_BUILD_DLL
#define DWREADER_API __declspec(dllexport)
#else
#define DWREADER_API __declspec(dllimport)
#endif
