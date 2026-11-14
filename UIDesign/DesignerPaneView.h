#pragma once
// DesignerPaneView：设计器内容视图（可嵌入任意 MFC 宿主）
//
// 本类只管"画什么、点了怎么反应"，不再假定自己是顶层框架窗口：
//   * 独立运行  → DesignerFrame（CFrameWndEx）里用 CDockablePane 承载
//   * AutoCAD   → DesignerPalette（CAdUiPalette）里承载同一个视图
//   * 普通对话框/面板 → 直接 create(parent, rc, id) 即可
//
// 统一运行时重做版：
//   * 编辑文档模型 = XmlRuntime m_doc（XmlElement 树）
//   * 自身界面（designer_layout.xml） = XmlRuntime m_ui 加载 + 布局
//   * 画布/文档树/属性面板全部基于 XmlElement
// 选中节点用路径（uidesign::Path）表达，避免值语义下指针失效。
#include "stdafx.h"
#include <set>
#include "XmlCore.h"
#include "UiDocUtil.h"
#include "DesignCanvas.h"
#include "ControlCatalog.h"
#include "XmlControls.h"
#include "../MfcXmlCore/controls/CodeEditor.h"   // 源码区用的独立代码编辑器控件

namespace uidesign
{
    constexpr UINT kId_TREE = 1001;
    constexpr UINT kId_WIDGETBOX = 1002;
    constexpr UINT kId_OBJINSPECTOR = 1003;
    constexpr UINT kId_CANVAS = 1004;
    constexpr UINT kId_CMD_MIN = 0x9000;
    constexpr UINT kId_CMD_MAX = 0x9FFF;

    class DesignerPaneView : public CWnd
    {
    public:
        DesignerPaneView() = default;
        ~DesignerPaneView() override
        {
            // 按底色缓存的画刷（每个声明颜色一支，控件数不受限）
            for (auto& kv : m_brushByColor)
                delete kv.second;
            m_brushByColor.clear();
        }

        bool openFile(const CString& path);

        // 关窗/退出前确认：有未保存修改时弹“保存 / 不保存 / 取消”，返回 false 表示用户取消
        bool confirmClose();
        bool isModified() const { return m_modified; }
        void markModified() { m_modified = true; }

        // 新建空白文档（宿主启动时没有文件参数也走这里，保证有根 Window 可编辑）
        void newDocument() { onNew(); }

        // 撤销 / 重做（宿主转发 Ctrl+Z / Ctrl+Y 用；与工具栏同一套实现）
        void undoOrRedo(bool undo);

        // 剪贴板动作（宿主转发 Ctrl+C / Ctrl+X / Ctrl+V 用）：焦点在框架本身时也要能复制/剪切/粘贴节点。
        void clipboardAction(UINT vk);

        // 全选（Ctrl+A）：选中当前容器（无选中时根窗体）下的全部子节点。
        void selectAllChildren();

        // Delete 删除选中节点：焦点在本视图子树时由 PreTranslateMessage 调，焦点在框架本身时
        // 由 DesignerFrame::PreTranslateMessage 转发（两者共用同一套判定，避免重复删除）。
        bool handleDeleteKey(HWND hwnd);

        // 宿主入口：以子窗口方式创建（CDockablePane / CAdUiPalette / 普通面板通用）。
        BOOL create(CWnd* parent, const CRect& rect, UINT id = 0x8001);

        // 宿主切换时刷新视图尺寸：pane 从浮动改停靠、Arx 侧栏改宽度都会走这里。
        void resizeTo(const CRect& rc);

        // 窄宽度自适应：停靠成 CAD 侧栏时宽度可能只有 400~700，
        // 固定 248+308 的两侧栏会把画布挤没，按档位收窄左/右栏。
        void applyResponsiveLayout(int width);
    protected:
        afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
        afx_msg void OnSize(UINT type, int cx, int cy);
        afx_msg void OnTimer(UINT_PTR nIDEvent);   // 1 = 尺寸静止检测，收尾预览态
        afx_msg void OnDynamicCmd(UINT id);
        afx_msg void OnMouseMove(UINT nFlags, CPoint pt);
        afx_msg void OnLButtonUp(UINT nFlags, CPoint pt);
        afx_msg void OnLButtonDown(UINT nFlags, CPoint pt);
        afx_msg void OnLButtonDblClk(UINT nFlags, CPoint pt);
        afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
        afx_msg LRESULT OnNcHitTest(CPoint point);
        afx_msg void OnDrawItem(int id, LPDRAWITEMSTRUCT dis);
        afx_msg void OnMeasureItem(int id, LPMEASUREITEMSTRUCT mis);
        afx_msg LRESULT OnCanvasSelect(WPARAM wParam, LPARAM lParam);
        afx_msg LRESULT OnDocChanged(WPARAM wParam, LPARAM lParam);   // 画布上的就地编辑/下拉/勾选
        afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
        afx_msg BOOL OnEraseBkgnd(CDC* pDC);
        afx_msg void OnPaint();
        DECLARE_MESSAGE_MAP()

    private:
        bool loadLayout();                 // 用 XmlRuntime 加载自身界面 + 递归创建控件
        void createControlForElement(XmlElement* el);   // 按 tag 创建 MFC 控件
        void applyLayoutToControls();      // 用 XmlElement preview 重排控件
        void buildMenu();
        void onOpen();
        void onSave();
        void onExit();
        void addControl(const CString& tag);
        void addControlAt(const CString& tag, const CString& name, int x, int y, int w, int h);
        // 画布左侧 Draw 工具条点了图元：记成"待放置 tag"，下一次在画布上拖动即按矩形创建
        void pickCreateTag(const std::wstring& tag);
        Path parentPath(XmlElement* root, const Path& selPath, XmlElement* parent, XmlElement* child);
        void onApply();

        void rebuildTree();
        void populateWidgetBox();
        void selectNode(const Path& p);
        void fillProperties(XmlElement* node);
        void updateStatus();

        UINT nextCtrlId() { return m_nextCtrl++; }
        UINT ensureAction(const CString& name);

        // 撤销 / 重做（快照式，参照 wxFormBuilder CommandProcessor）
        void pushUndo();                 // 修改文档前快照当前树
        void undo();
        void redo();
        void restoreSnapshot(XmlElement& snap);
        std::vector<XmlElement> m_undoStack;
        std::vector<XmlElement> m_redoStack;

        // 自身界面（designer_layout.xml）
        XmlRuntime m_ui;
        std::map<int, CWnd*> m_ctrlByIndex;
        std::map<XmlElement*, CWnd*> m_ctrlByNode;   // 节点 -> 控件（自身界面，树稳定不动）
        std::map<CString, UINT> m_actionCmd;
        std::map<UINT, CString> m_cmdAction;
        UINT m_nextCmd = kId_CMD_MIN;
        UINT m_nextCtrl = 1200;
        CMenu m_menu;
        CToolTipCtrl m_tooltip;

        // 编辑文档
        XmlRuntime m_doc;
        ControlCatalog m_catalog;              // XML 声明式控件目录（参照 wxFormBuilder）
        // Property Editor 值列就地编辑（CListCtrl 本身不可编辑，叠一个 CEdit）
        BOOL PreTranslateMessage(MSG* pMsg) override;
        void applyPropVal(const CString& key, const CString& val);
        CEdit m_propEdit;
        // 源码区：控件库提供的独立 CodeEditor（自带窗口 + D2D 自绘 + 智能提示）
        std::unique_ptr<CodeEditor> m_codeEditor;
        bool m_codeEditing = false;
        void beginCodeEdit();                          // 进入编辑：聚焦代码编辑器
        void commitCodeEdit();                         // 解析回文档，失败则报错保留原文
        void cancelCodeEdit();
        void syncCodeEditor();                         // 把编辑器贴到代码区矩形并同步文本
        void applyCodeText(const std::wstring& xml);   // 编辑器内容变化 -> 文档模型
        CString propOptions(const CString& tag, const CString& key);  // catalog 里声明的 enum 候选
        CString widgetTagFor(const CString& display);   // 控件箱显示名 -> XML tag（catalog 驱动）
        CString m_pendingCreateTag;
        DesignCanvas* m_canvas = nullptr;
        Path m_selected;                       // 选中节点路径
        std::vector<Path> m_selSet;            // 多选集合（首项 = 主选中 m_selected）
        bool m_docRevealSel = true;            // 需要把对象检查器滚到选中行（每次选中置 true）
        int m_nextId = 1;

        bool m_showGrid = true;
        int m_respLeft = -1;                 // 上一次应用的左栏宽度档位（-1 = 未应用）
        int m_respRight = -1;                // 上一次应用的右栏宽度档位
        void setupXmlCodeView();            // 在 xmlCodeHost 内创建只读多行 CEdit
        void updateXmlView();               // 用当前文档的 XML 序列化刷新代码视图
        CBrush m_brushPanel;
        CBrush m_brushEdit;
        CBrush m_brushGroup;
        std::map<HWND, COLORREF> m_bgByHwnd;
        std::map<HWND, COLORREF> m_fgByHwnd;
        std::map<COLORREF, CBrush*> m_brushByColor;   // 声明底色 -> 画刷（按色复用，不能只留一支）
        CBrush* brushFor(COLORREF c);

        void onNew();
        void onDelete();
        void toggleGrid();
        void notImpl(const CString& what);
        // 工具栏 / 菜单动作：名字 -> 枚举（静态表映射），分发用 switch
        enum class Action
        {
            None, Open, Save, Exit, New, AddButton, AddLabel, Apply, Delete,
            Grid, Preview, Check, CodeToggle, CodeCopy, Undo, Redo,
            Cut, Copy, Paste, Ruler, ZoomIn, ZoomOut, ZoomFit,
            AlignLeft, AlignHCenter, AlignRight, AlignTop, AlignVCenter, AlignBottom,
            DistributeH, DistributeV,
            About,
        };
        static Action actionOf(const CString& name);
        void dispatchAction(Action a, const CString& name);
        // ---- 节点级剪贴板 / 对齐 / 缩放 / 标尺 ----
        void refreshAfterDocChange();            // 结构变化后：重排 + 重设画布 + 属性表 + 状态栏
        bool copySelectedToClip();               // 选中子树序列化到内部 + 系统剪贴板
        void onCopy();
        void onCut();
        void onPaste();
        // Ctrl+拖动复制：p = 源节点路径，pressPt = 画布文档坐标下的按下点
        void beginCopyDrag(const Path& p, CPoint pressPt);
        void onAlign(const CString& mode);       // left/right/hcenter/top/bottom/vcenter
        void onDistribute(bool horizontal);      // 横向 / 纵向均匀分布
        static void applyGeomAttrs(XmlElement* n, int x, int y);   // 只写 Left/Top
        void selectMulti(const std::vector<Path>& v);
        void clearSelection();
        LRESULT OnCanvasSelectMulti(WPARAM wParam, LPARAM lParam);
        void revealCodeForSelected();                        // 代码坞滚到选中节点
        CString makeUniqueName(const CString& base) const;   // 复制/粘贴时自动改名
        void toggleRuler();
        void zoomBy(float factor);
        void zoomFit();
        std::wstring m_clipXml;                  // 内部剪贴板（XML 片段）
        bool m_showRuler = false;
        void onAbout();

        // ---- 整窗单张 D2D 表面 ----
        // 整个客户区由一块 D2D 表面画出来，不再创建任何子 HWND 控件；
        // 唯一保留的真实窗口是画布（DesignCanvas 本身就是 D2D 直呈）。
    public:
        // 拖动/缩放窗口期间：不重画整窗，直接把上一帧位图拉伸上屏
        // （相当于 Qt Quick 的 layer 缓存 + 视口变换：SC_SIZE 循环里每帧只有一次 BitBlt）
        void beginSizeMovePreview();
        void endSizeMovePreview();
        bool isSizeMoving() const { return m_sizeMove; }
    private:

        void paintSurface(HDC hdc);
        // 常驻双缓冲：每帧 CreateCompatibleDC/CreateCompatibleBitmap 一张 1600x1000 的
        // 位图，等于每帧重新申请并初始化 6MB GDI 内存 —— 缩放时这条就是主要开销。
        // 改成尺寸不变就复用，只有客户区尺寸变了才重建。
        CDC      m_backDc;
        CBitmap  m_backBmp;
        CBitmap* m_backOld = nullptr;
        CSize    m_backSize = CSize(0, 0);
        bool     m_sizeMove = false;      // 系统拖动/缩放循环进行中（只贴拉伸预览）
        // 上一次完整布局算出来的"分区尺寸"，缩放期间按分区贴图（Qt 的 layer 分块）：
        // 左右两栏宽度不变 -> 1:1 贴原位，只有中间画布区双向拉伸。
        int      m_zoneTopH = 0;
        int      m_zoneBottomH = 0;
        int      m_zoneLeftW = 0;
        int      m_zoneRightW = 0;                               // 入口：画到 WM_PAINT 的 DC，整窗绘制
        void paintSurfaceNode(XmlCanvas& cv, XmlElement* node); // 按 tag 分派
        void computeLayoutZones(const CRect& rc);               // 记录分区尺寸（缩放预览用）
        void buildWidgetRows(std::vector<xmlctl::TreeRow>& rows);  // 控件箱数据
        void buildDocRows(XmlElement* node, int level, std::vector<xmlctl::TreeRow>& rows, XmlElement* sel); // 对象树数据
        void buildPropRows(XmlElement* node, std::vector<xmlctl::PropRow>& rows);          // 属性表数据

        // ---- 单表面模式的命中测试与交互 ----
        struct SurfaceHit
        {
            enum Kind { None, ToolButton, WidgetRow, DocRow, PropRow, Splitter, Code, Status, Chip, Tab, Menu };
            Kind kind = None;
            int index = -1;      // 按钮/行/分割条序号
            bool onIcon = false; // 点在树行的展开/折叠方框上（而不是文字区）
        };
        int m_surfTitleStrip = 28;   // 标题/菜单条下沿（无边框窗体的拖动区）
        std::vector<RECT>        m_surfToolRects;       // 工具栏按钮矩形（与 m_surfToolNames 同序）
        std::vector<std::wstring> m_surfToolNames;      // 按钮 Name（交给命令分发）
        // 菜单条：XML 里 MenuBar 的顶层 MenuItem 横排，点击弹出系统下拉菜单
        std::vector<RECT>        m_surfMenuRects;
        std::vector<XmlElement*> m_surfMenuNodes;
        void paintMenuBar(XmlCanvas& cv, const RECT& rc, XmlElement* node);
        void surfaceMenuPopup(XmlElement* top, POINT screenPt);
        XmlElement* m_surfPendingMenuNode = nullptr;   // 按下瞬间锁定，不随重绘失效
        POINT m_surfPendingMenuAnchor = { 0, 0 };
        std::vector<RECT>        m_surfSplitterRects;   // 分割条矩形（与 m_surfSplitters 同序）
        std::vector<int>         m_surfSplitterIdx;     // -> m_splitters 下标
        std::vector<XmlElement*> m_surfDocNodes;       // 对象树每行 -> 节点
        std::set<XmlElement*>   m_docSelPtrs;          // 对象树里要标蓝的节点（多选集合展开）
        std::vector<int>         m_surfDocLevels;       // 对象树每行 -> 缩进层级（图标命中用）
        std::vector<CString>     m_surfWidgetTags;      // 控件箱每行 -> tag（分类行为空）
        std::vector<CString>     m_surfWidgetCats;      // 控件箱每行 -> 所属分类（叶子行为空）
        std::vector<int>         m_surfWidgetLevels;    // 控件箱每行 -> 缩进层级
        // 展开/折叠状态：按节点指针（文档重载时清空）与分类名保存
        std::set<XmlElement*>   m_collapsedNodes;
        std::set<CString>        m_collapsedCats;
        void pruneCollapsedNodes();                     // 文档重建后剔除已失效的折叠节点
        RECT m_surfWidgetRc = { 0, 0, 0, 0 };
        RECT m_surfDocRc = { 0, 0, 0, 0 };
        RECT m_surfPropRc = { 0, 0, 0, 0 };
        RECT m_surfCodeRc = { 0, 0, 0, 0 };
        int m_surfWidgetScroll = 0, m_surfDocScroll = 0, m_surfPropScroll = 0, m_surfCodeScroll = 0;
        int m_surfWidgetTotal = 0, m_surfDocTotal = 0, m_surfPropTotal = 0, m_surfCodeTotal = 0;
        int m_surfHotTool = -1;      // 悬停的工具栏按钮
        int m_surfHotSplit = -1;     // 悬停的分割条
        int m_surfSelWidgetRow = -1; // 控件箱选中行
        int m_surfSelPropRow = -1;   // 属性表选中行
        RECT m_surfParentRc = { 0, 0, 0, 0 };  // 递归时的父容器矩形（分割条命中区向其拉伸）
    int  m_surfInheritPt = 0;              // 父容器声明的字号（ToolBar/StatusBar 往下传，子元素不必逐个写 FontSize）
        SurfaceHit surfaceHitTest(CPoint pt) const;
        void surfaceLButtonDown(CPoint pt);
        // 无边框窗体：贴边拖拽改尺寸 / 标题行拖拽移动（交给顶层框架按 NC 消息处理）
        bool tryFrameChrome(CPoint pt);
        int  frameEdgesAt(CPoint pt);          // 1=左 2=右 4=上 8=下，0=不在边缘
        // Qt QWindow::startSystemMove / startSystemResize 的等价实现：
        // 主动给顶层框架发 WM_NCLBUTTONDOWN(HTCAPTION/HTLEFT...)，由 DefWindowProc 进入
        // 系统移动/缩放循环，DWM 负责整窗合成，不再自己 SetWindowPos 追鼠标。
        void startSystemChrome(CPoint pt, int edges);
        void surfaceLButtonUp(CPoint pt);
        void surfaceMouseMove(CPoint pt, UINT flags);
        BOOL surfaceMouseWheel(CPoint pt, short delta);
        void surfaceToolClick(int idx);
        bool surfaceScrollRect(const RECT& rc, CPoint pt, int& scroll, int total, int pageH, short delta);
        void clampSurfaceScroll();
        CString m_surfStatus[4];     // 状态栏四格文本（surface 模式由 updateStatus 填）
        afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);

        // ---- 外壳构件（深化后的设计器界面：标题条/标签/胶囊/舞台/代码坞）----
        std::vector<RECT>        m_surfChipRects;   // 控件箱分类胶囊矩形
        std::vector<CString>     m_surfChipCats;    // 对应分类名（空 = 全部）
        int m_surfHotChip = -1;
        int m_surfHotTab = -1;
        std::vector<RECT>        m_surfTabRects;
        CString m_docPath;            // 当前文档路径（文档标签与状态栏显示）
        bool m_modified = false;      // 文档是否有未保存的修改（关闭时据此提示）
        CString m_widgetFilter;       // 控件箱分类过滤（空 = 全部）
        bool m_codeCollapsed = false; // XML 代码坞是否收起
        void toggleCodeDock();
        void previewCurrent();
        void checkCurrent();
        void copyXmlToClipboard();
        std::wstring saveToTemp();    // 把当前文档存到临时文件（预览/校验用）
        std::wstring exeDir() const;

        // ---- 属性表就地编辑 / 右键增删复移 ----
        std::vector<xmlctl::PropRow> m_surfPropRows;  // 上一帧属性表内容（双击时用来取 key/value）
        static constexpr int kRenameDocRow = -3;   // 对象检查器双击改名用的特殊编辑行
        int m_surfPropEditRow = -1;                    // 正在编辑的行（-1 = 未编辑，-2 新增属性，-3 对象名）
        CString m_surfPropEditKey;                     // 正在编辑的属性名
        int m_surfEnumPendingRow = -1;                 // 待弹候选的枚举属性行（在 mouse-up 里弹，否则被 up 秒关）
        CPoint m_surfEnumPendingPt = CPoint(0, 0);
        DWORD m_surfClickTick = 0;                     // 上一次左键按下时刻（双击判定）
        CPoint m_surfClickPt = CPoint(0, 0);
        void beginPropEditSurface(int row);        void popupPropEnum(int row);
        void beginRenameDocRow(int row);           // 对象检查器双击改名
        void commitPropEditSurface();
        void cancelPropEditSurface();
        void surfacePropMenu(CPoint pt);
        void surfaceDocMenu(CPoint pt);          // 对象树右键：复制/粘贴/上移/下移/删除节点
        XmlElement m_clipNode;                  // 剪贴板里的节点深拷贝
        bool m_hasClipNode = false;
        afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
        afx_msg void OnContextMenu(CWnd* pWnd, CPoint pos);   // 吞掉系统右键菜单，只留自绘那套

        // 面板分割条（Splitter）拖动调整大小
        struct SplitterInfo
        {
            HWND hwnd = nullptr;          // 分割条窗口（用于定位）
            XmlElement* el = nullptr;    // 分割条节点（单表面模式按 preview 矩形命中）
            XmlElement* grid = nullptr;  // 所属 Grid（改其 gridCols/gridRows）
            int index = 0;                // 被调整的列/行索引
            int sign = 1;                 // 方向因子（拖哪一侧变宽）
            bool vertical = false;        // false=调列宽，true=调行高
            int startValue = 0;           // 拖动起点时的原宽/高
            CPoint startPt;               // 拖动起点（相对 DesignerPaneView 客户区）
        };
        std::vector<SplitterInfo> m_splitters;
        int m_dragSplit = -1;             // 正在拖动的分割条索引，-1=无
        // 无边框窗体自己实现的拖动/缩放（不进系统模态循环，才不会闪白圈）
        int m_frameEdges = 0;             // 0=未拖动，否则为 1/2/4/8 的位或
        CPoint m_frameStart = CPoint(0, 0);   // 屏幕坐标起点
        CRect m_frameRect = CRect(0, 0, 0, 0);
        HWND m_frameRoot = nullptr;
        DWORD m_splitDragLastMove = 0;    // 拖动节流时间戳
        void setupSplitters();            // 收集自身界面中的可拖分割条
        void splitterDrag(int idx, CPoint pt);   // 拖动中：改行列 + relayout
        void splitterEnd(int idx);        // 拖动结束
        void relayoutUi();               // 重排自身界面（layout + 应用到控件）
        int hitTestSplitter(const CPoint& pt);  // 命中竖分割条（返回索引或 -1）
    };
}
