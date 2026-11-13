#include "stdafx.h"
#include "DesignCanvas.h"

BEGIN_MESSAGE_MAP(uidesign::DesignCanvas, CWnd)
    ON_WM_PAINT()
    ON_WM_SIZE()
    ON_WM_ERASEBKGND()
    ON_WM_SETCURSOR()
    ON_WM_LBUTTONDOWN()
    ON_WM_RBUTTONDOWN()
    ON_WM_MOUSEMOVE()
    ON_WM_LBUTTONUP()
    ON_WM_LBUTTONDBLCLK()
    ON_WM_CHAR()
    ON_WM_KEYDOWN()
END_MESSAGE_MAP()
