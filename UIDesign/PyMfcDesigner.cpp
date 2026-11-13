// PyMfcDesigner —— 基于 PyMfcXml 统一运行时的可视化界面设计器
// 入口：CDesignApp（CWinAppEx，为 CFrameWndEx/CDockablePane 提供 MFC Feature Pack 管理器）
#include "stdafx.h"
#include "DesignerFrame.h"
#include <afxwinappex.h>
#include <afxvisualmanagerwindows.h>

using namespace uidesign;

class CDesignApp : public CWinAppEx
{
public:
    BOOL InitInstance() override
    {
        if (!CWinAppEx::InitInstance())
            return FALSE;

        ::CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);

        // CFrameWndEx::PreCreateWindow → CFrameImpl::RestorePosition 会读窗口位置记忆，
        // 没有 registry key 时 GetProfileBinary 走空路径直接抛异常，进程还没出窗口就退出。
        SetRegistryKey(_T("PyArx\\PyMfcDesigner"));
        m_bLoadWindowPlacement = FALSE;   // 设计器位置按 Create 传入的 rect，不记忆上次

        // CFrameWndEx 依赖的三件套：不初始化会在停靠/右键菜单等路径上 ASSERT
        if (!InitContextMenuManager()) return FALSE;
        if (!InitKeyboardManager()) return FALSE;
        if (!InitTooltipManager()) return FALSE;

        CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerWindows));
        CDockingManager::SetDockingMode(DT_SMART);

        auto* frame = new DesignerFrame();
        if (!frame->createFrame())
            return FALSE;

        m_pMainWnd = frame;
        frame->ShowWindow(SW_SHOW);
        frame->UpdateWindow();

        CString cmdLine(m_lpCmdLine);
        cmdLine.Trim();
        if (!cmdLine.IsEmpty())
        {
            if (cmdLine.GetLength() >= 2 && cmdLine[0] == _T('"') && cmdLine[cmdLine.GetLength() - 1] == _T('"'))
                cmdLine = cmdLine.Mid(1, cmdLine.GetLength() - 2);
            frame->openFile(cmdLine);
        }
        else
        {
            // 不带文件启动 = 新建一个空白文档（否则根节点为空，画布没东西可编辑）
            frame->newDocument();
        }

        return TRUE;
    }

    int ExitInstance() override
    {
        ::CoUninitialize();
        return CWinAppEx::ExitInstance();
    }
};

CDesignApp theApp;
