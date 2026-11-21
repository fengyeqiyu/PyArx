// PyMfcTopWnd.cpp —— PyMfcTopWnd 实现（MFC 侧独立 TU，NotUsing PCH）。

#include "PyMfcTopWnd.h"

BEGIN_MESSAGE_MAP(PyMfcTopWnd, CWnd)
    ON_WM_SIZE()
END_MESSAGE_MAP()

void PyMfcTopWnd::OnSize(UINT nType, int cx, int cy)
{
    CWnd::OnSize(nType, cx, cy);
    if (relayout)
        relayout(cx, cy);
    if (resize)
        resize(cx, cy);
}

void PyMfcTopWnd::PostNcDestroy()
{}
