// PyMfcDesigner —— 基于 PyMfcXml 统一运行时的可视化界面设计器
// 入口：普通 CWinApp（不再需要 MFC Feature Pack 的停靠管理器/视觉管理器），
// 外壳是 DesignerShell —— 一个无框 CWnd，内容只有一个铺满客户区的 DesignerPaneView。
#include "stdafx.h"
#include "DesignerShell.h"

using namespace uidesign;

class CDesignApp : public CWinApp
{
public:
    BOOL InitInstance() override
    {
        if (!CWinApp::InitInstance())
            return FALSE;

        ::CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);

        auto* shell = new DesignerShell();
        if (!shell->createShell(CRect(0, 0, 1360, 880)))
            return FALSE;

        m_pMainWnd = shell;
        shell->ShowWindow(SW_SHOW);
        shell->UpdateWindow();

        CString cmdLine(m_lpCmdLine);
        cmdLine.Trim();
        if (!cmdLine.IsEmpty())
        {
            if (cmdLine.GetLength() >= 2 && cmdLine[0] == _T('"') && cmdLine[cmdLine.GetLength() - 1] == _T('"'))
                cmdLine = cmdLine.Mid(1, cmdLine.GetLength() - 2);
            shell->view().openFile(cmdLine);
        }
        else
        {
            // 不带文件启动 = 新建一个空白文档（否则根节点为空，画布没东西可编辑）
            shell->view().newDocument();
        }

        return TRUE;
    }

    int ExitInstance() override
    {
        ::CoUninitialize();
        return CWinApp::ExitInstance();
    }
};

CDesignApp theApp;
