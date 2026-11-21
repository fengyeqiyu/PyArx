// PyMfcHostInternal.h —— PyMfcHost 实现拆分后的内部共享声明（非公开接口）。
#pragma once
#include "PyMfcHost.h"
#include "PyMfcPerf.h"
#include "PyMfcDwgGL.h"
#include "XmlSurface.h"
#include "XmlGeom.h"
#include <sstream>
#include <cmath>
#include <algorithm>

bool PyMfcHoverMark(HWND h);
bool PyMfcHoverUnmark(HWND h);
bool PyMfcHovered(HWND h);
// void pinTrace(const wchar_t* fmt, ...);  // 临时插桩，已清理
PyMfcPerfCounters& pyMfcPerf();
struct DragBarDrag
{
    bool  active = false;
    POINT downScreen = { 0, 0 };   // 按下时的光标屏幕位置
    int   downLeft = 0;            // 按下时控件左边界（父客户坐标）
    int   parentW = 0;             // 按下时父客户区宽度（拖动中不变）
    int   barW = 0;                // 控件自身宽度
};
std::map<HWND, DragBarDrag>& dragBarState();
int DragBarTargetLeft(HWND hwnd, LPARAM lParam, const DragBarDrag& st);
bool pyMfcImagePixelSize(const std::wstring& path, int& outW, int& outH);
LRESULT CALLBACK PyMfcEventSubProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);
std::wstring hostClipboardText();
struct TableEventCtx
{
    PyMfcHost* host = nullptr;
    int id = 0;
};
std::wstring MonthCalDateText(const SYSTEMTIME& st);
HCURSOR cursorOfCss(const std::wstring& k);
std::string wideToUtf8Text(const std::wstring& src);
COLORREF v3Shade(COLORREF c, float k);
bool v3ParseColor(const std::wstring& s, COLORREF& out);
int borderLinePixelCode(int depth, int along, int th, const wchar_t* style);
int borderPixelCodeCorners(int x, int y, int w, int h, int tl, int tr, int br, int bl, int th, const wchar_t* style);
int borderPixelCode(int x, int y, int w, int h, int rad, int th, const wchar_t* style);
std::unordered_map<int, std::pair<HWND, HWND>>& idCacheStore();
