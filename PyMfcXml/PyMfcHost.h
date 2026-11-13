// PyMfcHost.h —— 宿主类 PyMfcHost 的接口（控件库 / 皮肤 / 事件 / XML 桥）。
// 这里只放声明、嵌套类型、数据成员与类内宏；实现全部在 PyMfcHost.cpp（312 个成员）。
// 改接口改这个文件，改实现改 PyMfcHost.cpp。
#pragma once

#include "PyMfcCommon.h"
#include "Table.h"
#include "controls/XmlTypes.h"   // XmlSkinKind：自绘件种类号的唯一定义（控件侧也用它）

// S1 渲染表面抽象（MfcXmlCore）：这里只前向声明，d3d11/dxgi/d2d1 类型不进宿主头。
class IXmlSurface;

// 表面探针消息（wParam: 1=模拟设备丢失, 2=强制重建, 3=写诊断到日志/成员串）
constexpr UINT kPyMfcSurfaceProbeMsg = WM_USER + 0x521;

class PyMfcHost : public CWnd, public IXmlMfcHost
{
    DECLARE_DYNAMIC(PyMfcHost)
public:
    PyMfcHost() = default;
~PyMfcHost() override;

    // 同上：宿主是 PyMfcWindow / PyMfcPanel 的成员，SubclassWindow 绑定，
    // 窗口销毁时不能被 MFC 默认的 PostNcDestroy 删掉。
void PostNcDestroy() override;

BOOL create(HWND parent, const CRect& rect);

void setTheme(bool dark);

void applyFont(HWND hwnd) const;

// ---------------------------------------------------------------------------
// 选项卡的内容区：SysTabControl32 自己把页签下方整块刷成系统浅色，
// 深色皮肤里就是一块刺眼的白板。这里只补"刷底色"一件事：
// 先让原类过程把页签画完，再按主题色填满内容区（页签本身仍由系统画，交互不变）。
static constexpr const wchar_t* kTabOldProcProp = L"PyMfcTabOldProc";
static constexpr const wchar_t* kTabBrushProp = L"PyMfcTabBrush";

static LRESULT CALLBACK PyMfcTabBodyProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

// ---------------------------------------------------------------------------
// 全控件自绘：这些原生控件保留系统类的状态与交互（焦点、键盘、输入法、滚动、
// 选中、命中测试），但不再自己画像素 —— WM_PAINT 被掏空，像素由父窗口的
// D2D 层按主题统一绘制。这样"所有控件一个样子"，又不用重写交互。
static constexpr const wchar_t* kSkinOldProcProp = L"PyMfcSkinOldProc";
// 自绘控件要回到宿主取主题色/外观声明，用属性挂回宿主指针
static constexpr const wchar_t* kSkinHostProp = L"PyMfcSkinHost";
// PyMfcSkinBox：本仓库自建的"复合小控件"画布窗口，kind 记在窗口属性上，
// 皮肤窗口过程和绘制分派都靠它区分（Switch/SpinBox/RangeSlider/BusyIndicator/PageIndicator）。
static constexpr const wchar_t* kSkinKindProp = L"PyMfcSkinKind";
// 种类号由 XmlTypes.h 的 XmlSkinKind 唯一给出，这里只是给宿主内部起短名
static constexpr int kSkinBoxSwitch = XmlSkinId(XmlSkinKind::Switch);
static constexpr int kSkinBoxSpin = XmlSkinId(XmlSkinKind::Spin);
static constexpr int kSkinBoxRange = XmlSkinId(XmlSkinKind::Range);
static constexpr int kSkinBoxBusy = XmlSkinId(XmlSkinKind::Busy);
static constexpr int kSkinBoxPages = XmlSkinId(XmlSkinKind::Pages);
static constexpr int kSkinBoxDial = XmlSkinId(XmlSkinKind::Dial);
static constexpr int kSkinBoxTumbler = XmlSkinId(XmlSkinKind::Tumbler);
static constexpr int kSkinBoxScrollBar = XmlSkinId(XmlSkinKind::ScrollBar);
static constexpr int kSkinBoxTable = XmlSkinId(XmlSkinKind::Table);
static constexpr int kSkinBoxTree = XmlSkinId(XmlSkinKind::Tree);
static constexpr int kSkinBoxToolBar = XmlSkinId(XmlSkinKind::ToolBar);
static constexpr int kSkinBoxToolTip = XmlSkinId(XmlSkinKind::ToolTip);
static constexpr int kSkinBoxMenu = XmlSkinId(XmlSkinKind::Menu);
static constexpr int kSkinBoxMenuBar = XmlSkinId(XmlSkinKind::MenuBar);
static constexpr int kSkinBoxPopup = XmlSkinId(XmlSkinKind::Popup);
static constexpr UINT_PTR kBusyTimerId = 0x52;
static constexpr UINT_PTR kTipTimerId = 0x54;    // ToolTip 自动消失
static constexpr UINT_PTR kFlickTimerId = 0x55;  // 列表惯性滚动
static constexpr int kSkinBoxListView = XmlSkinId(XmlSkinKind::ListView);   // 虚拟化列表
static constexpr int kSkinBoxPager = XmlSkinId(XmlSkinKind::Pager);         // 页容器（Page / StackView / SwipeView）
static constexpr int kSkinBoxGridView = XmlSkinId(XmlSkinKind::GridView);   // 网格委托（GridView）
static constexpr int kSkinBoxPathView = XmlSkinId(XmlSkinKind::PathView);   // 路径委托（PathView）
static constexpr int kSkinBoxQCanvas = XmlSkinId(XmlSkinKind::QCanvas);     // 画命令画布（QCanvas）
static constexpr int kSkinBoxFlick = XmlSkinId(XmlSkinKind::Flick);         // 可拖拽内容视口（Flickable）
static constexpr int kSkinBoxShadow = XmlSkinId(XmlSkinKind::Shadow);       // 带 box-shadow 的圆角底板（浮层背面）
static constexpr UINT_PTR kPageTimerId = 0x56;   // 切页滑动
static constexpr UINT_PTR kAnimTimerId = 0x53;
static constexpr wchar_t kAnimHostClass[] = L"PyMfcAnimHost";
static constexpr const wchar_t* kSkinBoxClass = L"PyMfcSkinBox";
// 自绘光标闪烁定时器的 ID（每个控件窗口各自一份）
static constexpr UINT_PTR kCaretTimerId = 0x51;

// 自绘光标的闪烁相位：控件自己的 WM_PAINT 被掏空后，系统的 caret 不再闪，
// 得由宿主按同一条系统节奏（GetCaretBlinkTime）自己开关一次再重画。
static std::map<HWND, bool>& caretBlinkStates();

static bool caretBlinkOn(HWND hwnd);

// 注册一次 PyMfcSkinBox 窗口类：窗口过程用 DefWindowProc，真正干活的是后面子类化进去的
// 皮肤过程（和系统类控件的做法一致，避免另开一套窗口过程体系）。
static void ensureSkinBoxClass();

// 原类过程同时记在静态表里：属性/表双保险，转发链绝不空
static std::map<HWND, WNDPROC>& skinOldProcs();

static bool isSkinPaintClass(const wchar_t* cls);

// 重画这个控件自己（自绘控件在自己的 WM_PAINT 里画，所以只失效自己）
static void requestSkinRepaint(HWND hwnd);

// 自绘控件的窗口过程定义在 PyMfcHost 之后（它是类静态成员，要用到宿主的外观/主题）

// owner-draw 控件的窗口过程：系统对 Static/Button 的默认绘制（背景 + 一圈边框）
// 会和 XML 声明的外观打架，白底页上表现为"每个控件都被描了一个深色圆角框"。
// 这里接管 WM_PAINT/WM_ERASEBKGND：擦除直接吃掉，绘制一律转发父窗口的 WM_DRAWITEM，
// 界面外观 100% 由 XML 决定。
// 原窗口过程存进窗口属性：Button/Static 的类过程负责 BN_CLICKED、按下/悬停状态与
// BM_CLICK 语义，不能被丢弃。这里只接管擦除与绘制，其余消息转发回原类过程。
static constexpr const wchar_t* kOldProcProp = L"PyMfcOldWndProc";

// 配电画布：窗口属性里存宿主指针，鼠标消息交给宿主自己的状态机处理。
// owner-draw 过程定义在 PyMfcHost 之前，拿不到成员函数，所以走"属性 + 自由函数"。
static constexpr const wchar_t* kPlanHostProp = L"PyMfcPlanHost";

// 控件级事件（鼠标/键盘/焦点）：在控件创建末尾挂一层子类化过程，
// 用窗口属性存回宿主指针与"上一层窗口过程"，事件转发不打断原有绘制/交互链。
static constexpr const wchar_t* kEvtHostProp = L"PyMfcEvtHost";
static constexpr const wchar_t* kEvtPrevProcProp = L"PyMfcEvtPrevProc";

// 这些控件的"样子"就是它们自己的原生绘制内容（进度条绿条、列表项、树节点、
// 表格行、选项卡页签、日历格子……）。它们被同一个系统过程接管 WM_PAINT 的话，
// 只会被画成一块统一底板 —— 表现为"只有壳、没有内容"。
// 所以按类名判定：原生控件一律转发回原类过程，自绘只留给 Button/Static 这类
// 本来就没有内容的控件。
static bool isNativePaintClass(const wchar_t* cls);

// 可拖动分割条：一个 6px 的 owner-draw STATIC，鼠标按下后捕获鼠标，
// 位移回调给 Python（它按位移改侧板宽度并重排），其余消息照旧转给原过程保证还会画。
static LRESULT CALLBACK PyMfcDragBarProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

static LRESULT CALLBACK PyMfcOwnerDrawProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

// ---------------------------------------------------------------------------
// 图片装载：StaticBitmap / BitmapButton 以前只有窗口没有图（BM_GETIMAGE=0），
// XML 里写的 Image/Icon 只进了自绘画布，原生控件本身仍是空的。
// 这里用 WIC 解码 png/jpg，转成 32bpp HBITMAP 交给 STM_SETIMAGE / BM_SETIMAGE。
static void ensureWicCom();

static HBITMAP loadBitmapFromFile(const std::wstring& path);

// ---------------------------------------------------------------------------
// Tab 键焦点切换：这些控件是普通子窗口，不是对话框控件，系统默认不会把 Tab
// 变成焦点切换（按一下只会"哔"一声，焦点原地不动）。这里在消息泵队列上挂
// 线程级钩子，把落在 PyMfcHost 里的 Tab 转成"下一个/上一个 WS_TABSTOP 控件"。
static constexpr const wchar_t* kTabHostProp = L"PyMfcTabHost";
inline static HHOOK g_tabHook = nullptr;

// 窗口属性里的整数值（属性句柄本身就是整数，只是类型是 HANDLE）
static int propInt(HWND h, const wchar_t* name, int dflt);

// Tab 遍历：按 (TabIndex, PyMfcSeq) 排序，而不是按 z 序。
// 用 GetNextDlgTabItem 会把"谁压在上面"和"Tab 走到谁"绑在一起 ——
// 于是调一次 Level、键盘顺序就跟着变，这是不能接受的耦合。
static HWND pyMfcNextTabItem(HWND focus, bool back);

static LRESULT CALLBACK PyMfcTabMsgHook(int nCode, WPARAM wParam, LPARAM lParam);

static void ensureTabHook();

// ---------------------------------------------------------------------------
// 分隔条：以前是个纯 STATIC（既不能拖也影响不到邻居）。这里接管鼠标消息，
// 拖动时移动自身并同时缩放紧邻的那个控件（竖直方向）。
static constexpr const wchar_t* kSplitterOwnerProp = L"PyMfcSplitterOwner";

// 找紧贴分隔条"上/下"两侧的兄弟控件（水平方向要有交叠，竖直方向 4px 内贴合）。
// 只找一个是不够的：真正的分隔条挪动时，上面那块要缩、下面那块要跟着位移，
// 否则拖动后两块之间会裂出一条缝。
static void splitterNeighbors(HWND self, HWND* above, HWND* below);

static LRESULT CALLBACK PyMfcSplitterProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

// 新自绘表格事件回调（what: 1=单击 2=双击 3=选中变化 4=F2 5=Delete）
static void tableEventCb(void* ctx, int row, int col, int what);

HWND createControl(const wchar_t* cls, const std::wstring& text,
                       DWORD style, int x, int y, int w, int h, int id);

void addButton(int id, const std::wstring& text, int x, int y, int w, int h);

void addLabel(int id, const std::wstring& text, int x, int y, int w, int h);

void addEdit(int id, const std::wstring& text, int x, int y, int w, int h);

void addListBox(int id, int x, int y, int w, int h);

void addCheckBox(int id, const std::wstring& text, int x, int y, int w, int h);

void addRadio(int id, const std::wstring& text, int x, int y, int w, int h);

void addGroup(int id, const std::wstring& text, int x, int y, int w, int h);

void addCombo(int id, int x, int y, int w, int h);

    // 带基础功能的创建：多行/只读/限长/占位提示，下拉框可编辑或只读下拉，列表框多选
void addEditAdvanced(int id, const std::wstring& text, bool multiline, bool readOnly,
                         int maxLength, const std::wstring& placeholder,
                         int x, int y, int w, int h) override;

void addComboAdvanced(int id, bool dropDownList, int x, int y, int w, int h) override;

void addListBoxAdvanced(int id, bool multipleSelect, int x, int y, int w, int h) override;

void setLimitText(int id, int n) override;

void setCueBanner(int id, const std::wstring& text) override;

void addProgress(int id, int x, int y, int w, int h);

void addSlider(int id, int minValue, int maxValue, int x, int y, int w, int h);

void addTab(int id, int x, int y, int w, int h);
void addTree(int id, int x, int y, int w, int h);
void addListCtrl(int id, int x, int y, int w, int h);

// 新自绘表格（PyMfcTable）：命中/选中/矩形同源，替代 SysListView32 那条易差一行的链路。
void addTable(int id, int x, int y, int w, int h);
TableControl* findNewTable(int id) const;
void tableClear(int id);
int tableRowCount(int id) const;
void tableSetColumns(int id, const std::vector<int>& widths, const std::vector<std::wstring>& titles);
void tableSetColTypes(int id, const std::vector<int>& types);
void tableAppendRow(int id, const std::vector<std::wstring>& cells);
void tableSetCell(int id, int row, int col, const std::wstring& text);
std::wstring tableGetCell(int id, int row, int col) const;
py::object tableCellRect(int id, int row, int col) const;
int tableSelectedRow(int id) const;
void tableSelectRow(int id, int row);
void tableEnsureVisible(int id, int row);
void tableSetFilter(int id, int col, const std::wstring& text);
void tableClearFilters(int id);
void tableSort(int id, int col, bool desc);
int tableViewRows(int id) const;

void addChoice(int id, int x, int y, int w, int h);
void addDatePicker(int id, int x, int y, int w, int h);
void addPanel(int id, int x, int y, int w, int h);

    // SS_WHITERECT = 白色填充矩形（不带文字），WS_BORDER 给一圈 1px 边：就是侧板底板
void addWhiteRect(int id, int x, int y, int w, int h) override;

    // 给单个原生控件换字体（lfHeight 传负的像素高，如 13 = 13px；与 HTML 的 font-size:13px 对齐）
void setControlFont(int id, const std::wstring& face, int px, bool bold);
    // 让某个按钮变成 HTML 的 .btn.primary：白底、蓝框、蓝字
void setControlAccent(int id, int r, int g, int b);
    // 标签文字色（HTML 里 .status/.detail label/.editor-foot 各有一套灰阶）
void setLabelColor(int id, int r, int g, int b);
void addSplitter(int id, int x, int y, int w, int h);
void addStaticLine(int id, int x, int y, int w, int h);
void addHyperlink(int id, const std::wstring& text, int x, int y, int w, int h);
void addRichText(int id, int x, int y, int w, int h);
void addToggle(int id, const std::wstring& text, int x, int y, int w, int h);
void addBitmapButton(int id, const std::wstring& text, int x, int y, int w, int h);
void addStaticBitmap(int id, int x, int y, int w, int h);
void addGrid(int id, int x, int y, int w, int h);
void addScrolledWindow(int id, int x, int y, int w, int h);
void addCalendar(int id, int x, int y, int w, int h);

	// 日历选中日期：SysMonthCal32 走 MCM_GETCURSEL / MCM_SETCURSEL，值格式 "YYYY-MM-DD"。
std::wstring getCalendarDate(int id) const override;
bool setCalendarDate(int id, const std::wstring& value) override;


    // ---- PyMfcSkinBox：QML 对齐的几个"小控件"，共用一个自建画布窗口 ----
    // 只有一块画布 + 宿主里的状态，没有系统类可用（Switch/RangeSlider/SpinBox/Busy/PageDots
    // 在 Win32 里都没有对应控件，硬套 TrackBar/Edit 反而更难对齐观感）。
void addSkinBox(int id, int kind, int x, int y, int w, int h) override;

    // HTML box-shadow 的等价底板：窗口比视觉矩形大 pad，阴影画在 pad 区里，
    // 底色/圆角/边框由参数给定（默认就是 .detail 那套白底 + #D0D5DD + radius 6）。
    // WS_EX_TRANSPARENT 让鼠标事件穿过阴影区，不挡住它背后的画布。
void addShadowBox(int id, int x, int y, int w, int h, int pad, int radius,
                      int dx, int dy, int blur,
                      const std::wstring& shadowColor, const std::wstring& bgColor,
                      const std::wstring& backColor, const std::wstring& borderColor,
                      int borderW);

    // 画布窗口的鼠标/滚轮：命中按几何算（拇指/三角/点），不做子窗口
bool skinBoxMouse(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) override;

int cwOf(int id) const;

int rangeMinOf(int id) const;
int rangeMaxOf(int id) const;
int getRangeLow(int id) const;
int getRangeHigh(int id) const;
void setRangeValues(int id, int low, int high) override;
bool getSwitchValue(int id) const override;
void setSwitchValue(int id, bool on) override;
int getSpinValue(int id) const override;
void setSpinValue(int id, int value) override;
void setSpinRange(int id, int lo, int hi) override;
void stepSpin(int id, int delta);
    // BusyIndicator 的旋转相位：定时器每 tick +1，画的时候按相位决定哪一段最亮
void bumpBusyPhase(int id);
void setPageCount(int id, int count) override;
int getPageCount(int id) const override;
void setPageIndex(int id, int index) override;
int getPageIndex(int id) const override;

    // ---- Dial ----
int getDial(int id) const override;
void setDial(int id, int value) override;
void stepDial(int id, int delta);

    // ---- Tumbler ----
int getTumblerCount(int id) const override;
void setTumblerCount(int id, int count) override;
int getTumblerValue(int id) const override;
void setTumblerValue(int id, int index) override;
void stepTumbler(int id, int delta) override;

    // ---- 独立 ScrollBar ----
void setScrollBarRange(int id, int lo, int hi, int page, bool vertical) override;
int getScrollBarValue(int id) const override;
void setScrollBarValue(int id, int value) override;
void setScrollValueInternal(int id, int value);
    // 用户交互（点轨道 / 拖滑块 / 滚轮）改值：值真变了才回调 ValueChanged。
    // 与 setScrollBarValue（Python 主动赋值）分开，避免"Python 设值 -> 回调 -> 再设值"自激。
void setScrollValueUser(int id, int value);

    // ---- TableView ----
    // 列宽自适应：侧板宽度变化时按比例重设列宽（不清数据、不重建控件）
void setColumnWidth(int id, int col, int px);
// 批量设列宽：WM_SETREDRAW(FALSE) 包住整轮 LVM_SETCOLUMNWIDTH，只在结尾失效一次。
// 逐列裸调每列都会触发整表重排+重绘（39 行实测单列 ~22 ms），批处理后只重画一遍。
void setColumnWidths(int id, const std::vector<int>& widths);

    // 表格度量：返回 (行高, 表头高)。Python 用它在布局时把表格高度按整行对齐，
    // 否则最后一行会被裁掉一截（选中行露出一条蓝边，看起来像残留）。
py::object tableMetrics(int id);

void setTableColumns(int id, const std::wstring& tabJoined) override;
void addTableRow(int id, const std::wstring& tabJoined) override;
// 一次灌满整表（行间 \n、格间 \t）：39 次逐行插入 = 39 次重绘 ≈ 22 ms，
// 合成一次插入 + 一次失效后只重画一遍。
void setTableRows(int id, const std::wstring& blob);
void clearTableRows(int id) override;
int getTableRowCount(int id) const override;
bool isTreeItemCollapsed(int id, int row) const override;
void setTreeItemCollapsed(int id, int row, bool collapsed) override;
std::wstring getTableCell(int id, int row, int col) const override;
void setTableCell(int id, int row, int col, const std::wstring& text) override;
int getTableSelectedRow(int id) const override;
void setTableSelectedRow(int id, int row) override;
    // 点表头排序：同一列再点一次反向；数据真的重排，不是只画个箭头
void sortTable(int id, int col) override;

    // ---- TreeView ----
void addTreeItem(int id, const std::wstring& text, int level, bool hasKids) override;
void clearTreeItems(int id) override;
int getTreeSelectedRow(int id) const override;
void setTreeSelectedRow(int id, int row) override;
bool isTreeRowHidden(int id, int row) const;
void toggleTreeRow(int id, int row);

    // ---- ToolBar ----
void addToolBarItem(int id, const std::wstring& text) override;
void clearToolBarItems(int id) override;
int getToolBarSelected(int id) const override;
void setToolBarSelected(int id, int index) override;

static std::vector<std::wstring> splitTabs(const std::wstring& s);
static bool parseNum(const std::wstring& s, double& out);
    // 选中行不在可视区时把首行挪过去（行高 24、表头 26）
void ensureVisible(int id, int row);


    // ---- 内容填充（Tab / Tree / ListCtrl / Grid）----
    // 这些控件"建出来"只是一块空窗口，页签/节点/列/行必须真插进原生控件里。
void addTabItem(int id, const std::wstring& text) override;

void addTreeNode(int id, const std::wstring& text, int parentIndex) override;

void addColumn(int id, const std::wstring& text, int width) override;

void addRow(int id, const std::wstring& text) override;

void setText(int id, const std::wstring& text);

void applyLinkText(int id);

std::wstring getText(int id) const;

void setVisible(int id, bool visible);

void setEnabled(int id, bool enabled);

void moveControl(int id, int x, int y, int w, int h);

    // 批量布局：一次 DeferWindowPos 提交所有行控件的位移 + 显隐。
    // 逐条 moveControl / ShowWindow 时，每次调用都会让父窗口补画一次露出来的空白，
    // 312 个控件滚一下就是一片闪烁；这里用 SWP_NOREDRAW 攒成一次提交，再统一重画。
void moveMany(py::list items);

    // 父表面绘制的装饰矩形：没有自己的 HWND，由父窗口在 OnEraseBkgnd 里铺底色；
    // z 序由父表面统一保证（分割条不会被表格这类兄弟窗口盖掉），也省掉一次独立 WM_PAINT。
    struct SurfaceItem
    {
        RECT rc = { 0, 0, 0, 0 };
        COLORREF fill = RGB(255, 255, 255);
        bool visible = true;
    };
    std::map<int, SurfaceItem> m_surfaces;
    std::vector<int> m_surfaceOrder;
    void setSurface(int id, int x, int y, int w, int h, int fill);
    void moveSurfaces(py::list items);
    py::tuple getSurfaceRect(int id);

    // ============================================================ S2-A 绘制节点
    // 纯装饰件从子 HWND 降级为保留树里的 scene node：=0 旧 GDI 路径不变，
    // =1 由表面帧按 z 层统一绘制（clipPush/节点矩形/clipPop）。
    // 分层：静态底板层 < 控件层 < 浮层层；同层按注册次序。
    enum XmlNodeLayer
    {
        NodeLayerBackdrop = 0,   // 容器/装饰铺底（m_surfaces 那类）
        NodeLayerControl = 1,     // 面板、分割线、标签、按钮
        NodeLayerOverlay = 2      // 阴影底板、浮层卡片
    };
    struct XmlPaintNode
    {
        int id = 0;
        RECT rc = { 0, 0, 0, 0 };
        int layer = NodeLayerControl;
        int order = 0;
        bool visible = true;
        bool dirty = true;
        int kind = 0;            // 1=whiteRect 2=shadowBox 3=staticLine；4=label 5=button（S2-B）
        HWND hWnd = nullptr;     // 保留树句柄（隐藏 HWND 仍可取文字/显隐，API 契约不变）
        bool hover = false;      // 按钮四态：悬停
        bool pressed = false;    // 按钮四态：按下（替代 BM_GETSTATE/ODS_SELECTED）
        bool focus = false;      // 按钮四态：键盘焦点环
        std::function<void(XmlCanvas&, const RECT&)> draw;
    };
    std::map<int, XmlPaintNode> m_nodes;
    std::vector<int> m_nodeOrder;

    // 注册/更新/取出节点；注册返回节点引用。draw 闭包在节点帧里由表面画布调用。
    void registerPaintNode(int id, int kind, int layer, int x, int y, int w, int h);
    void updatePaintNodeRect(int id, int x, int y, int w, int h, bool visible);
    const XmlPaintNode* findPaintNode(int id) const;
    // 一帧内按 z 层/次序画完所有可见节点；每个节点画前 clipPush(rc)、画完 clipPop。
    void drawPaintNodes(XmlCanvas& cv);
    // 节点闭包复用的分类型绘制（S2-B 标签/按钮直接接这里）
    void drawNodeWhiteRect(XmlCanvas& cv, const XmlPaintNode& n) const;
    void drawNodeShadowBox(XmlCanvas& cv, const XmlPaintNode& n) const;
    void drawNodeStaticLine(XmlCanvas& cv, const XmlPaintNode& n) const;
    void drawNodeLabel(XmlCanvas& cv, const XmlPaintNode& n) const;
    void drawNodeButton(XmlCanvas& cv, const XmlPaintNode& n) const;

    // S2-B 按钮宿主状态机：命中/悬停/按下/焦点；命中释放后走 OnCommandRange（与 BN_CLICKED 同一分发）。
    int hitNode(const CPoint& pt) const;
    void OnMouseMove(UINT nFlags, CPoint pt);
    void OnLButtonDown(UINT nFlags, CPoint pt);
    void OnLButtonUp(UINT nFlags, CPoint pt);
    // 复用 OnDrawItem 的外观/底色/图标/文字/焦点环分支，节点帧直接喂表面画布。
    void drawOwnerNode(XmlCanvas& cv, const XmlPaintNode& n, bool isStatic) const;

    // XML 运行时交接：加载完成后由宿主托管，表达式才能在"改值"后重新布局。
    // names/context 一并留存：ItemView 的 model 变了要按新行数重建行控件，
    // 重建时同样要登记 x:Name→id、按回调名在 context 里解析事件处理函数。
void attachXml(std::shared_ptr<XmlRuntime> rt, const std::map<std::wstring, int>& ids,
                py::dict names, py::dict context, int nextId,
                void* ownerWindow, void* ownerPage);

    // 运行时改 model 时用：行控件的销毁/重建要拿到 runtime、名字表与回调上下文
std::shared_ptr<XmlRuntime> xmlRuntime() const { return m_xml; }
py::dict& xmlNames() { return m_xmlNames; }
py::dict& xmlContext() { return m_xmlContext; }
int& xmlNextId() { return m_xmlNextId; }
void* xmlOwnerWindow() const { return m_xmlOwnerWindow; }
void* xmlOwnerPage() const { return m_xmlOwnerPage; }

    // 销毁一个控件并从 x:Name 映射里摘掉（ItemView 重建行前清旧行用）
void removeControlById(int id);

    // 表达式重算 → 重新布局 → 把新矩形推给各控件
bool reflowXml();

    // 写 XML 属性（按 x:Name 定位），随即重算表达式并重排
bool setXmlProperty(const std::wstring& name, const std::wstring& prop, const std::wstring& value);

    // ============================================================ 浮层族
    // ToolTip / Menu / MenuBar / Popup 都做成"宿主内的覆盖子窗口"：还是 PyMfcSkinBox，
    // 仍由 pymfcskin 画、仍由 skinBoxMouse 命中，但用 HWND_TOP 顶到兄弟窗口最上层。
    std::map<int, std::wstring> m_tipText;
    std::map<int, std::vector<std::wstring>> m_menuItems;
    std::map<int, int> m_menuHover;
    std::map<int, int> m_menuSel;
    std::map<int, std::vector<std::wstring>> m_mbItems;
    std::map<int, int> m_mbSel;
    std::map<int, int> m_mbHover;
    std::map<int, std::wstring> m_popTitle;
    std::map<int, std::vector<std::wstring>> m_popLines;

void ensureOverlay(int id, int kind);

void hideOverlay(int id);

bool isOverlayVisible(int id) const;

void placeOverlay(int id, int x, int y, int w, int hgt);

    // ToolTip：贴在某个控件右下方，duration 毫秒后自动消失（0 表示一直留着）
void showToolTip(int ownerId, const std::wstring& text, int durationMs);

void hideToolTip();

    // Menu：弹出式菜单，hover 跟鼠标，点击把行号写进 m_menuSel 并收起
void showMenu(int id, const std::vector<std::wstring>& items, int x, int y);

void hideMenu(int id);

int menuRowAt(int id, int y) const;

int getMenuSelected(int id) const;

    // XML 里写 <Menu Items="..."/> 时只灌数据，不主动弹出
void setMenuItems(int id, const std::vector<std::wstring>& items);

    // MenuBar：横排标题栏，点哪个标题就把 m_mbSel 设成哪个
void addMenuBarItem(int id, const std::wstring& text);

void clearMenuBarItems(int id);

int getMenuBarSelected(int id) const;

void setMenuBarSelected(int id, int i);

    // Popup：通用浮层面板（标题条 + 多行正文），点正文即关闭
void showPopup(int id, const std::wstring& title, const std::vector<std::wstring>& lines,
                   int x, int y, int w, int hgt);

void hidePopup(int id);

bool isPopupVisible(int id) const;

    // ============================================================ 虚拟化列表（ListModel + delegate）
    // 数据在宿主里，画的时候只截回"可见窗口内"的行交给 pymfcskin::drawListView，
    // 所以一万行和十行的绘制量一样——行本身不建 HWND。
    std::map<int, std::vector<std::wstring>> m_lvT;
    std::map<int, std::vector<std::wstring>> m_lvS;
    std::map<int, std::vector<std::wstring>> m_lvV;
    std::map<int, int> m_lvSel;
    std::map<int, int> m_lvHover;
    std::map<int, int> m_lvScroll;
    std::map<int, int> m_lvItemH;
    // GridView 的格子尺寸（0 = 自适应：按客户区宽度均分列、高度按 96/92 比例）
    std::map<int, int> m_gvCellW;
    std::map<int, int> m_gvCellH;
    std::map<int, int> m_lvDragY;
    std::map<int, int> m_lvDragScroll;
    std::map<int, int> m_lvLastY;         // 上一次拖动 y（算每帧速度用）
    std::map<int, int> m_lvOver;          // 越过边界的像素量（回弹用，负=顶端）
    std::map<int, int> m_flickVel;
    std::map<int, ULONGLONG> m_flickStamp;

    // GridView 的格子度量：宽度按客户区均分（目标格宽 92），高度按 96/92 比例。
    // 滚动上限、命中测试、绘制三处必须用同一套数，否则滚过末尾后底部格子点不动。
void gvMetrics(int id, int& cols, int& cw, int& ch) const;

int lvRowH(int id) const;

int lvMaxScroll(int id) const;

void lvClampScroll(int id);

    // 一行数据用 "标题|副标题|右侧值" 表达；三个字段都可缺
    // ---- QCanvas：画命令列表（数据化，不跑脚本） ----
    std::map<int, std::wstring> m_cvCmd;
    // 图元表与选中态：命令串是"画什么"，这里是"哪一条"。
    // 每次 setCanvasCommands 重解析（顺序即绘制顺序，命中按倒序取最上面那条）。
    std::map<int, std::vector<pymfcskin::QcPiece>> m_cvItems;
    std::map<int, std::vector<int>> m_cvSel;
    std::map<int, bool> m_cvEdit;          // 打开才算可编辑（默认关，老画布行为不变）
    std::map<int, POINT> m_cvDown;         // 按下点，用来判"点选 vs 框选"
    std::map<int, RECT> m_cvBand;          // 框选橡皮筋
    std::map<UINT, py::object> m_cvSelCb;  // 选中变化回调

    // ---- 配电规划图画布（PowerCanvas）----
    // 与 m_cvCmd 那套命令串画布不同：这里存结构化数据（布局结果或 SVG 场景），
    // 每帧按 zoom/offset 重新画，所以任意缩放都清晰；命令串方案会把图元坐标写死，
    // 放大就糊。
    struct DetailText
    {
        RECT rc = { 0, 0, 0, 0 };
        std::wstring text;
        int size = 12;
        bool bold = false;
        COLORREF color = RGB(32, 36, 42);
    };

    static constexpr float kPlanZoomMin = 0.05f;
    static constexpr float kPlanZoomMax = 4.0f;
    static float clampPlanZoom(float zoom)
    {
        return zoom < kPlanZoomMin ? kPlanZoomMin : (zoom > kPlanZoomMax ? kPlanZoomMax : zoom);
    }

    struct PlanCanvasState
    {
        xmlplan::PowerPlan plan;
        xmlsvg::SvgScene svg;
        int mode = 0;             // 0=空 1=plan 2=svg
        float zoom = 1.0f;
        int ox = 0, oy = 0;
        std::wstring sel;         // 选中卡片编号（plan 模式）
        bool drag = false;
        POINT down = {};          // 按下点（客户区坐标）
        int downOx = 0, downOy = 0;
        bool moved = false;
        bool showCable = true;    // 连线标注电缆（工具栏复选框）
        bool mini = false;        // 小地图：按客户区 fit 铺满、不画文字、画视口红框
        int miniOf = -1;          // 小地图对应的主画布 id
        // 诊断探针：记录最近一次 up 的坐标与命中结果，便于从 Python 侧看清
        // "真实鼠标点到底把什么坐标送来、命中了没有"。
        int upX = 0, upY = 0;
        bool upWasDrag = false;
        std::wstring lastHit;
        int lastMsg = 0;
        bool detailVisible = false;
        RECT detailRc = { 0, 0, 0, 0 };
        std::vector<DetailText> detailTexts;
    };
    std::map<int, PlanCanvasState> m_planCv;

    // fit：整图居中铺满，最多 100%（放大到 1 以上就是"看不清全貌"）
void fitPlanCanvas(int id);

void addPowerCanvas(int id, int x, int y, int w, int h) override;

void setPowerCanvasPlan(int id, const std::wstring& json) override;

void setPowerCanvasScene(int id, const std::wstring& json) override;
// 同一份场景复制给另一个画布：4.8 MB JSON 再解析一遍约 60 ms，直接拷已解析的结构
void copyPowerCanvasScene(int toId, int fromId);

void setPowerCanvasSvg(int id, const std::wstring& svgText) override;

void setPowerCanvasView(int id, float zoom, int offsetX, int offsetY) override;

float getPowerCanvasZoom(int id) const override;

std::wstring getPowerCanvasSelection(int id) const override;

    // 画布选项：电缆标注开关、小地图模式（mini=true 时本画布只做缩略图 + 视口红框）
void setPowerCanvasOptions(int id, bool showCable, bool mini, int miniOf) override;

    // 视口参数（缩放 + 偏移）：Python 侧居中定位、小地图联动都要用
void getPowerCanvasView(int id, float& zoom, int& ox, int& oy) const override;

    // 命中测试：模型坐标下的卡片矩形
std::wstring planHit(int id, int clientX, int clientY);

void setDetailCard(int id, py::list rect, py::list texts);
void clearDetailCard(int id);
bool detailHit(int id, int clientX, int clientY) const;

    // 卡片在客户端坐标系里的矩形（缩放/平移后）。返回 false = 没有这张卡。
    // 用途：外部脚本定位、编辑表格"点行高亮图表"、自动化测试里的点击坐标。
bool planCardRect(int id, const std::wstring& code, RECT& out) const;


    // ---- 控件级导出（基类能力，任意控件都能调）----
    // PrintWindow 让控件按正常绘制路径画进内存 DIB，再交给 WIC 写 PNG。
    // 自绘控件（含 D2D 画布）在 PyMfcOwnerDrawProc 的 WM_PRINTCLIENT 分支里重画，
    // 抓到的就是屏幕上看到的样子 —— 验收/回归不必再抓真屏幕。
bool saveControlPng(int id, const std::wstring& path);

    // 按编号选中并把该卡片移到视口中央（搜索定位用；也方便外部脚本/测试驱动）。
    // 返回是否找到。缩放不动，只改 offset —— 用户当前的缩放偏好不该被搜索重置。
bool selectPlanItem(int id, const std::wstring& code);

    // 选中/双击都走既有的回调表：Python 侧注册 onClick / onDoubleClick 即可收到
void firePlanSelect(int id, const std::wstring& /*code*/);

void firePlanDoubleClick(int id, const std::wstring& /*code*/);

    // 画整体内容：页面底 → （SVG 场景 | 配电图）全部交给同一套绘制函数
void drawPlanCanvas(XmlCanvas& cv, int id, const RECT& rc);


    // GridView 格子尺寸覆盖（0 = 自适应：宽度按客户区均分、高度 96/92 比例）
void setGridCellSize(int id, int cellW, int cellH) override;


void setCanvasCommands(int id, const std::wstring& cmds);
std::wstring getCanvasCommands(int id) const;

    // ---- 绘图层次（Level）与 Tab 序号（TabIndex）----
    // Level：越大越靠上（越晚画），决定"谁压住谁"，改它只动 z 序。
    // TabIndex：只决定 Tab 键的遍历次序，不参与 z 序。
    // 两件事必须解耦 —— Win32 默认用 z 序当 Tab 序，叠放一改键盘顺序就跟着乱。
    std::map<int, int> m_zLevel;
    std::map<int, int> m_tabIndex;
    std::map<int, int> m_createSeq;   // 控件创建序（= XML 声明顺序），Tab 同级兜底用
    std::map<int, std::wstring> m_theme;   // Theme="Dark"/"Light"…（颜色分配规则待定）
    int m_seqNext = 0;

int levelOf(int id) const;

int tabIndexOf(int id) const;

void noteLevel(int id, int level);

void noteTabIndex(int id, int n);

void raiseMany(const std::vector<int>& ids);
        // 一次提交多个置顶：逐个 bringToFront 每次都全表重排 z 序 + 整窗重画
        void bringToFrontMany(const std::vector<int>& ids);
void setLevel(int id, int level);

int getLevel(int id) const;

int maxLevel() const;

int minLevel() const;

void bringToFront(int id);
void sendToBack(int id);

void setTabIndex(int id, int n);

int getTabIndex(int id) const;

    // Tab 序挂在窗口属性上：Tab 钩子读不到 C++ 成员，只能按属性排序。
    // 创建序 PyMfcSeq 在控件落地时就写死（= XML 声明顺序），
    // 不能等建完整棵树再按叠放序倒推 —— 那时 z 序已经被 Level 改过了。
void noteCreated(int id);

void applyTabProps();

    // 按 (Level, 当前叠放序) 重排宿主直接子窗：升序逐个 HWND_TOP，最后一个最高。
    // 同级不改变彼此相对次序，所以重复调用是幂等的（不会抖）。
void applyZOrder();

void applyLevelsAndTabs();


    // ---- 画布图元：解析 / 选中 / 删除 ----
void repaintCanvas(int id);

void setCanvasEditable(int id, bool on);

bool getCanvasEditable(int id) const;

int canvasItemCount(int id) const;

    // 图元描述：索引、命令名、包围盒（客户区坐标）
void canvasItem(int id, int index, int& outOp, RECT& outBox) const;

std::wstring canvasItemOp(int id, int index) const;

    // 倒序命中：后画的压在上面，点哪儿先中谁
int canvasHitTest(int id, POINT pt) const;

std::vector<int> getCanvasSelection(int id) const;

void fireCanvasSelect(int id);

void setCanvasSelection(int id, const std::vector<int>& idx);

void clearCanvasSelection(int id);

void onCanvasSelect(int id, py::object cb);

    // 删除指定图元：按片段序号从命令串里摘掉，再把剩下的原样拼回去
    // （其余图元的文本一字不改，避免"删一个、动一群"）。
int removeCanvasItems(int id, const std::vector<int>& idx);

int removeCanvasSelection(int id);

    // ---- 控件级真 opacity 图层（WPF `Opacity` 语义：形状 + 文字统一混合） ----
    // 与 XML 的 Appearance.Opacity 不同：那条是"颜色与底色先混合"，文字仍是实心；
    // 这条走 D2D 图层，整块（含文字）按 alpha 合成。
    std::map<int, int> m_layerAlpha;
void setLayerOpacity(int id, int alpha);
int getLayerOpacity(int id) const;

void addListItem(int id, const std::wstring& line);

void clearListItems(int id);

int getListItemCount(int id) const;

std::wstring getListItem(int id, int index) const;

int getListSelected(int id) const;

void setListSelected(int id, int index);

int getListScroll(int id) const;

void setListScroll(int id, int pos);

    // 惯性滚动：每 16ms 推进一次，速度按 v *= 0.92 衰减，|v| 小于 40px/s 就停
int flickTick(int id);

    // ============================================================ 页容器（Page / StackView / SwipeView）
    // 页里的子控件是真 HWND，切页时整组平移；容器自己只画底部页码条。
    std::map<int, std::vector<std::vector<int>>> m_pgPages;
    std::map<int, int> m_pgIdx;
    std::map<int, std::map<int, RECT>> m_pgBase;   // 子控件 -> 父客户区基准矩形（含尺寸）
    std::map<int, int> m_pgAnimFrom;
    std::map<int, int> m_pgAnimTo;
    std::map<int, ULONGLONG> m_pgAnimStart;
    std::map<int, int> m_pgAnimDur;
    std::map<int, int> m_pgDragX;

    // ============================================================ Flickable（可拖拽内容视口）
    // 内容子控件 SetParent 到视口窗（子窗天然被父窗裁剪），拖拽 1:1 跟随，
    // 松手按速度惯性衰减，越过边界时做弹性回弹（rubber band，和列表的触边同一套手感）。
    std::map<int, std::vector<int>> m_flKids;
    std::map<int, std::map<int, RECT>> m_flBase;    // kid -> 相对视口客户区的基准矩形
    std::map<int, POINT> m_flOff;                   // 当前偏移（<= 0）
    std::map<int, POINT> m_flPos;                   // 上一次鼠标点
    std::map<int, POINT> m_flDragOff;               // 按下时的偏移
    std::map<int, POINT> m_flRaw;                   // 按下后的原始累积位移（未阻尼）
    std::map<int, POINT> m_flVel;                   // px/s
    std::map<int, ULONGLONG> m_flStamp;
    std::map<int, int> m_flCW;                      // 内容尺寸（0 = 由子控件外接框推）
    std::map<int, int> m_flCH;

void flickMetrics(int id, int& viewW, int& viewH, int& cw, int& ch,
                      int& minX, int& minY) const;

void flickApply(int id);

void setFlickContent(int id, const std::vector<int>& kids, int cw, int ch);

void clearFlickContent(int id);

void setFlickOffset(int id, int x, int y);

int getFlickOffsetX(int id) const;

int getFlickOffsetY(int id) const;

    // 惯性 + 回弹：每帧 16ms，越界时速度清零、按 0.86 收敛回边界
int flickTickFlickable(int id);

void pageRegister(int id);

void clearPages(int id);

    // 容器在父客户区里的矩形
RECT pageHostRect(int id) const;

void pageLayout(int id, int which, int offset, bool show);

void addPage(int id, const std::vector<int>& kids);

int viewCount(int id) const;

int viewIndex(int id) const;

void viewSetIndex(int id, int index);

    // 动画帧专用：只挪位置，不重排 z 序、不逐个子窗 InvalidateRect。
    // 逐帧 "每个子窗一次 SetWindowPos + InvalidateRect(TRUE)" 会让每个子窗每帧都
    // 整块擦底重画一遍，页内容一多就是肉眼可见的卡顿；批量 + SWP_NOREDRAW 后
    // 只有容器自己补一遍空出来的条纹。
void pageSlide(int id, int which, int offset);

    // 滑动切页：旧页往左滑出、新页从右侧滑入
void viewAnimate(int id, int index, int durMs);

int pageTick(int id);


    // ============================================================ 动画引擎
    // 对齐 QML：PropertyAnimation / NumberAnimation / ColorAnimation /
    // SequentialAnimation / ParallelAnimation / Behavior on xxx / Transition / states /
    // SpringAnimation / easing.bezierCurve。
    // 一条"时间线"由若干 Track 组成：多 Track 同时 t0=0 就是 Parallel，t0 依次累加就是 Sequential。
    struct AnimTrack
    {
        int id = 0;
        std::wstring prop;      // X / Y / Width / Height / Value / Color
        double from = 0, to = 0;
        COLORREF c0 = 0, c1 = 0;
        bool color = false;
        int t0 = 0;             // 相对整条时间线的起点（ms）
        int dur = 300;
        std::string ease = "outCubic";
        bool loop = false;
        bool hasLast = false;
        double lastV = 0;
        COLORREF lastC = 0;
    };

static double easeAt(const std::string& ease, double t);

static COLORREF lerpColor(COLORREF a, COLORREF b, double p);

    HWND m_animWnd = nullptr;
    bool m_animTickOn = false;
    DWORD m_animStartMs = 0;
    std::vector<AnimTrack> m_anims;
    std::map<std::wstring, std::pair<int, std::string>> m_behaviors;  // "id|prop" → 时长/缓动
    std::map<int, COLORREF> m_ctrlBack;                               // ColorAnimation 的落点
    bool m_animApplying = false;

    static LRESULT CALLBACK animHostProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);

void ensureAnimWnd();

void animAdd(const AnimTrack& tr);

    // 把某一帧的数值真正落到控件上。m_animApplying 用来在 setter 里抑制 Behavior 递归。
    // 兄弟窗口不能互相覆盖（每个控件一个 HWND）：某个控件改了几何后，
    // 与"旧矩形 ∪ 新矩形"相交的兄弟必须重新画一遍，
    // 否则被它压过、后来又让出来的那块会留下空洞或残影（表现为"控件时有时无"）。
void repairSiblingsOf(HWND moved, const RECT& oldRc, const RECT& newRc);

void applyAnimValue(const AnimTrack& tr, double v, COLORREF c);

double currentPropValue(int id, const std::wstring& prop) const;

void animate(int id, const std::wstring& prop, double to, int dur,
                 const std::string& ease, int delay, bool loop);

void animateFrom(int id, const std::wstring& prop, double from, double to, int dur,
                     const std::string& ease, int delay, bool loop);

void animateColor(int id, COLORREF to, int dur, const std::string& ease, int delay);

    // Transition / states：一次性把一组 (控件, 属性, 目标值) 并行动画过去
void animateState(int id, const std::wstring& prop, double to, int dur,
                      const std::string& ease, int delay);

void stopAnimations(int id);

void setBehavior(int id, const std::wstring& prop, int dur, const std::string& ease);

bool behaviorFor(int id, const std::wstring& prop, int& dur, std::string& ease) const;

void animTick();

void addString(int id, const std::wstring& text);

void clearStrings(int id);

int countStrings(int id) const;

std::wstring getString(int id, int index) const;

void deleteString(int id, int index);

int getCurSel(int id) const;

void setCurSel(int id, int index);

    // CheckBox / Radio 共用 BST_CHECKED(1) / BST_UNCHECKED(0)
int getCheck(int id) const;

void setCheck(int id, bool checked) override;

    // ProgressBar / TrackBar 共用一个区间接口，内部按控件类型换消息
void setRange(int id, int minValue, int maxValue);

void setPos(int id, int value) override;

int getPos(int id) const;

void setReadOnly(int id, bool readOnly);

bool getReadOnly(int id) const;

void* getDlgItemHandle(int id) const override;


    // 外观由控件类解析 XML 后推过来：存一份、重建背景刷、让控件重绘
    // ---- 通用基础属性（控件基类 XmlMfcControlBase 解析后推过来）----
void setControlLevel(int id, int level) override;
int getControlLevel(int id) const override;
void setControlTabIndex(int id, int index) override;
int getControlTabIndex(int id) const override;

    // 主题：颜色分配规则未定，先把声明存下来（后续按 Theme 分色时从这里读）
void setControlTheme(int id, const std::wstring& theme) override;
std::wstring getControlTheme(int id) const override;

void setControlAppearance(int id, const xmlapp::XmlAppearance& ap) override;

    // 原生控件的配色桥：外观契约（也就是 D2D 那套主题）给什么颜色，
    // 这里就按控件类灌进原生绘制路径。
    // 不做这一步的话，D2D 深色界面里会夹着一串系统白块（Edit/ListView/日历），
    // 主题层再怎么统一也压不住 —— 因为它们是控件自己拿系统色画的。
void applyNativeThemeColors(HWND h, COLORREF bg, COLORREF fg);

    // 控件图标：XML 的 Icon 已由加载层解析成绝对路径，这里只存表 + 触发重绘
void setControlIcon(int id, const std::wstring& path) override;

const std::wstring* findIcon(int id) const;

const xmlapp::XmlAppearance* findAppearance(int id) const;

    // 未声明 Foreground 时的默认文字色：按背景明暗取可读色，
    // 否则白底页上仍用主题浅色文字，字会"消失"
static COLORREF contrastText(COLORREF bg);

    // 页/窗口背景：控件自己没声明背景时，用它垫底（否则会退回主题底色，
    // 白底页面上就出现一条条深色块）。也用于 OnEraseBkgnd 的默认底色。
void setPageBackdrop(COLORREF c, bool valid);

    // 容器铺底：容器不建原生控件，由其宿主在 OnEraseBkgnd 里按声明画底色
void addContainerBackdrop(int x, int y, int w, int h,
                              const xmlapp::XmlAppearance& ap) override;

void clearContainerBackdrops() override;

    // 控件自己没声明 Background 时，应当"透出"它所在那一层容器的底色，而不是铺全局主题色。
    // m_backdrops 里登记的是各容器（含模板实例化出来的 Canvas）的矩形与外观，
    // 取"包含该控件中心、面积最小"的那一层即为它的直接父表面。
    // 不这么做的话，白卡（Canvas Background=#FFFFFF）上的 TextBlock 会被刷成页面色，
    // 表现为每行文字后面挂一条不同颜色的横带。
COLORREF surfaceColorAt(const CRect& rc) const;

void setFocusTo(int id);

bool getVisible(int id) const;

bool getEnabled(int id) const;

void onClick(int id, py::object callback);

    // ---- 侧板滚轮 ----------------------------------------------------------
    // 侧板里全是 EDIT 子控件，滚轮只会发给“焦点窗口”（某个 EDIT），EDIT 不处理时
    // 由 DefWindowProc 冒泡到宿主窗口。所以在这一层拦最可靠：命中注册的矩形就折算成
    // 一个“滚动档”回调给 Python，由 Python 重排行的位置（MFCXml 不会替子控件滚动）。
    // ---- 可编辑表格（SysListView32）的键盘/排序/复制粘贴支持 ----
    // 当前活动单元格：控件 id → (行, 列)。ListView 本身没有"活动单元格"概念，
    // 只能在 WM_LBUTTONDOWN 时用 LVM_SUBITEMHITTEST 记下来给 F2/Delete 用。
    std::map<int, std::pair<int, int> > m_gridCell;
    py::object m_gridAction;
    // 按控件 id 分流的表格回调（多张表并存时全局回调会串台）
    std::map<int, py::object> m_gridActionFor;
    // 表格事件回调的上下文（host + 控件 id），SetCallback 的 ctx 只给一个 void*
    std::map<int, void*> m_tableCtx;

void onGridAction(py::object cb);

void fireGridAction(int ctlId, const char* action, int row, int col, const std::wstring& text);

    // 可拖动分割条（表格侧板与画布之间）：鼠标按下后按屏幕位移回调 Python 改布局
    std::map<int, py::object> m_drag;
void onDrag(int id, py::object cb);
void fireDrag(int id, const char* phase, int x);

    // 原地编辑：EDIT 失焦 → 提交（点别处就等于确认，和 Excel 一致）
    std::map<int, py::object> m_blur;
void onBlur(int id, py::object cb);
void fireBlur(int id);

    // 递归按 id 找子窗口：表格可能挂在侧板里，不是主窗的直接子窗口，
    // 只 GetDlgItem(m_hWnd, id) 会拿不到句柄（拿到别的控件时 LVM_* 全部无效）
static HWND findByIdDeep(HWND parent, int id);

    // 单元格矩形（ListView 客户区坐标）：原地编辑要把 EDIT 精确盖到那一格上
py::object cellRect(int id, int row, int col);

    // 把 id 对应的控件变成可拖分割条（记住原过程，转发非鼠标消息）
bool installDragBar(int id);

void setWheelRegion(int x, int y, int w, int h, int step);
void onWheel(py::object cb);
bool fireWheelMessage(WPARAM wp, LPARAM lp);
    RECT m_wheelRect = {0, 0, 0, 0};
    bool m_wheelOn = false;
    int m_wheelStep = 3;
    py::object m_wheelCb;

    // ---- 超链接：地址 + 可点击 ----------------------------------------------
    // SysLink 只认 <a href="...">主题</a> 这种标记文本；老实现往窗口里塞的是
    // 纯文本 "http://"，既没有真实地址，点击也不会产生 BN_CLICKED（只有 NM_CLICK）。
void setUrl(int id, const std::wstring& url);

std::wstring getUrl(int id) const;

    // ---- 图片：StaticBitmap / BitmapButton ---------------------------------
bool setImage(int id, const std::wstring& path);

    // ---- 富文本格式 --------------------------------------------------------
    // RichText 原生支持 Bold/字号/颜色，但 Python 侧一直没有设置入口，
    // 于是它只能显示一段同样样式的纯文字。
void setRichFormat(int id, int sizePt, bool bold, bool italic, unsigned long rgb);

    // ---- 滚动范围：ScrolledWindow ------------------------------------------
void setScrollRange(int id, int minValue, int maxValue);

std::pair<int, int> getScrollRange(int id) const;
    // 文本控件的选区（起, 止）。编辑器类控件掏空 WM_PAINT 之后，选区要自己画，
    // 验证"选中了多少"也得能读出来，否则只能靠肉眼看颜色。
std::pair<int, int> getSelection(int id) const;
    // 程序化设置选区（对应 QML TextInput 的 select(start,end) / selectAll()）
void setSelection(int id, int from, int to) const;

int getScrollPos(int id) const;
    // 按 id + 属性名读值：外观走 m_ap，内容按控件类型落到各自的 getter，
    // 这样 Python 侧读写对称（不再只能按控件类型自己分叉去调 getText/getCurSel）
std::wstring getXmlValue(int id, const std::wstring& prop) const;
void onGridActionFor(int id, py::object callback);
void onChange(int id, py::object callback);
void onSelect(int id, py::object callback);
void onDoubleClick(int id, py::object callback);
void onDropDown(int id, py::object callback);
void onValue(int id, py::object callback);
void onChecked(int id, py::object callback);
    // 控件级事件派发：ev ∈ mousedown/mouseup/mousemove/dblclick/keydown/keyup/gotfocus/lostfocus
    void onControlEvent(int id, const std::wstring& ev, py::object callback);
    void fireControlEvent(int id, const std::wstring& ev);

    // 卸载时清空 Python 回调引用（此时持有 GIL），避免解释器残留悬垂 py::object
void clearCallbacks();

    // 自绘控件在自己的 WM_PAINT 里调用它画自己
    void paintSkinControl(HDC hdc, HWND child);
    // 自绘滚动区域（ScrolledWindow）的交互：滚轮 / 点轨道 / 拖拇指
    bool isScrollArea(HWND child) const;
    // 悬浮光标：可点控件返回手型、画布返回爪型、其余返回 nullptr（沿用系统）
    HCURSOR hoverCursor(HWND hwnd) const;
    void nudgeScroll(HWND child, int delta);
    // 悬浮滚轮：光标压在滚动条上时不必先给焦点，按光标位置分流滚轮（返回 true = 已消费）
    static bool wheelHoverAtPoint(POINT screenPt, int notches);
    bool wheelOnHoveredWindow(HWND w, POINT screenPt, int notches);
    // 位置唯一写入口：夹边、变了才重画、notify 时派发 Scroll 事件
    void setScrollAreaPos(HWND child, int pos, bool notify);
    void scrollAreaMouse(HWND child, UINT msg, WPARAM wParam, LPARAM lParam);
    // 自绘控件的窗口过程（声明成类静态成员，createControl 里的名字查找才明确）
    static LRESULT CALLBACK PyMfcSkinWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

protected:
    afx_msg void OnCommandRange(UINT id);
    afx_msg void OnEditChange(UINT id);
    afx_msg void OnEditBlur(UINT id);
    afx_msg void OnListChange(UINT id);
    afx_msg void OnComboDropDown(UINT id);
    afx_msg void OnDblClickRange(UINT id);
    afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
    virtual BOOL OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult);
    afx_msg BOOL OnEraseBkgnd(CDC* pDC);
    afx_msg void OnPaint();
    afx_msg void OnSize(UINT nType, int cx, int cy);
    afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
    afx_msg void OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct);
    afx_msg LRESULT onSurfaceProbe(WPARAM wParam, LPARAM lParam);
    // S1：按 PYARX_SURFACE 环境变量创建新渲染表面（=1 启用；默认 0 走旧 GDI 路径）
    void ensureSurface();
    // 自绘皮肤：把某个子控件按主题画到父窗口的 D2D 画布上
    void drawSkinControl(XmlCanvas& cv, HWND child);
    DECLARE_MESSAGE_MAP()

private:
    std::map<UINT, py::object> m_click;
std::map<std::wstring, py::object> m_ctlEvents;
bool m_anyCtlEvent = false;                          // 有控件级事件登记后才挂子类化      // "<id>:<事件名>" -> Python 回调
    std::map<UINT, py::object> m_change;


    std::map<UINT, py::object> m_select;
    std::map<UINT, py::object> m_dblclick;    // DoubleClick
    std::map<UINT, py::object> m_dropdown;    // DropDown（下拉展开）
    // 下拉展开瞬间"已确认"的显示文本：展开后鼠标划过各项时系统会临时改选中项，
    // 但我们只把高亮画在列表里，框里那行文字必须保持到真正点选为止。
    std::map<int, std::wstring> m_comboHold;
    std::map<UINT, py::object> m_value;       // ValueChanged（滑块/进度）
    std::map<UINT, py::object> m_checked;     // CheckedChanged（勾选状态变化）
    // 统一外观：XML 声明的形状/颜色/风格/前后置背景（按控件 id 存）
    std::map<int, xmlapp::XmlAppearance> m_ap;
    std::map<int, HBRUSH> m_apBrush;
    // 控件图标：Icon="res/xxx.png" 已解析成绝对路径，owner-draw 时画在控件左侧
    std::map<int, std::wstring> m_icons;
    // TreeCtrl 已插入节点的句柄（按控件 id），addTreeNode 的 parentIndex 靠它定位父节点
    std::map<int, std::vector<HTREEITEM>> m_treeNodes;
    // 超链接地址与显示文字（SysLink 的窗口文本是 <a href="...">标记</a>，不能直接读）
    std::map<int, std::wstring> m_urls;
    std::map<int, std::wstring> m_links;
    // 已装载的图片路径与 HBITMAP（自绘画布与原生控件两条通路都要）
    std::map<int, std::wstring> m_images;
    std::map<int, HBITMAP> m_ownedBitmaps;
    // ScrolledWindow 的滚动范围
    std::map<int, std::pair<int, int>> m_scroll;
    // ScrolledWindow 的当前滚动位置（自绘滚动条，不走系统滚动条）
    std::map<int, int> m_scrollPos;
    // 分隔线（StaticLine）：owner-draw 时按"一条横线"画，而不是当普通标签
    std::set<int> m_lines;
    // ScrolledWindow 控件 id（自绘滚动区域）
    std::set<int> m_scrollAreas;
    // 拖动滚动条拇指时记住"抓取偏移"
    std::map<int, int> m_dragScroll;
    // PyMfcSkinBox 的状态（一个画布窗口一种语义，按 id 各存一份）
    std::map<int, int> m_skinKinds;          // id -> kSkinBox*
    std::map<int, pymfcskin::ShadowBoxStyle> m_shadowBox;   // 阴影底板参数
    std::map<int, bool> m_switchOn;          // Switch 的开关
    std::map<int, int> m_spinVal;            // SpinBox 当前值
    std::map<int, int> m_spinLo, m_spinHi;   // SpinBox 范围
    std::map<int, int> m_rangeLo, m_rangeHi;  // RangeSlider 两个拇指
    std::map<int, std::pair<int, int>> m_boxRange;  // RangeSlider 的量程（min,max）
    std::map<int, int> m_busyPhase;          // BusyIndicator 旋转相位
    std::map<int, int> m_pageCount;          // PageIndicator 点数
    std::map<int, int> m_pageCur;            // PageIndicator 当前页
    std::map<int, int> m_dragThumb;          // RangeSlider 正在拖哪个拇指（0 无 / 1 低 / 2 高）
    // Dial / Tumbler / 独立 ScrollBar
    std::map<int, int> m_dialVal;            // Dial 当前值
    std::map<int, int> m_tumblerVal;         // Tumbler 当前项下标
    std::map<int, int> m_tumblerCount;       // Tumbler 项数
    std::map<int, int> m_scrollVal;          // ScrollBar 当前值
    std::map<int, int> m_scrollPage;         // ScrollBar 页大小（决定拇指长度）
    std::map<int, bool> m_scrollVert;        // ScrollBar 是否竖直
    std::map<int, int> m_dragMode;           // Dial 1 拖角度 / ScrollBar 2 拖拇指
    std::map<int, int> m_dragOrigin;         // ScrollBar 抓拇指时的相对偏移
    // 表格类控件（TableView / TreeView）与工具栏
    std::map<int, std::vector<std::wstring>> m_tblCols;                       // 列标题
    std::map<int, std::vector<std::vector<std::wstring>>> m_tblRows;          // 数据行
    std::map<int, int> m_tblSel, m_tblScroll;                                 // 选中行 / 首可见行
    std::map<int, TableControl*> m_newTables;                                 // 新自绘表格实例
    std::map<int, int> m_tblSortCol;
    std::map<int, bool> m_tblSortAsc;
    std::map<int, std::vector<std::wstring>> m_treeText;                      // 节点文字
    std::map<int, std::vector<int>> m_treeLevel;                              // 缩进层级
    std::map<int, std::vector<bool>> m_treeKids;                              // 是否有子节点
    std::map<int, std::vector<bool>> m_treeCollapsed;                         // 是否折叠
    std::map<int, int> m_treeSel, m_treeScroll;
    std::map<int, std::vector<std::wstring>> m_tbItems;                       // 工具栏按钮
    std::map<int, int> m_tbSel, m_tbHover;
    // XML 运行时：加载完交给宿主托管，用来做"改一个值 → 表达式重算 → 重新布局 → 控件挪位"。
    // 之前 runtime 是 guiload 里的栈上临时对象，加载完即销毁，所以表达式只能算一次，
    // 改了 Width 也不会重排。
    std::shared_ptr<XmlRuntime> m_xml;
    std::map<std::wstring, int> m_xmlIds;   // x:Name → 控件 id
    py::dict m_xmlNames;                    // 与加载时同一份 x:Name → id 字典
    py::dict m_xmlContext;                  // 加载时的回调上下文（重建行时解析事件用）
    int m_xmlNextId = 1000;                 // 重建行时继续分配的 id
    void* m_xmlOwnerWindow = nullptr;       // 加载时用的 PyMfcWindow*（无则空）
    void* m_xmlOwnerPage = nullptr;         // 加载时用的 PyMfcPanel*（无则空）
    // 容器（Canvas/Grid 等）声明的底色：按矩形存，OnEraseBkgnd 里先铺窗口底色再逐块覆盖
    std::vector<std::pair<CRect, xmlapp::XmlAppearance>> m_backdrops;
    bool m_dark = true;
    COLORREF m_bg = RGB(49, 56, 66);
    COLORREF m_pageBg = RGB(49, 56, 66);
    bool m_hasPageBg = false;
    COLORREF m_text = RGB(235, 235, 235);
    COLORREF m_btn = RGB(62, 70, 80);
    COLORREF m_border = RGB(95, 105, 118);
    // 单个控件的外观微调：primary 按钮的蓝框蓝字、标签的文字色（对应 HTML 的 .btn.primary 与各处 color）
    std::map<int, COLORREF> m_accent;
    std::map<int, COLORREF> m_labelFg;
    HBRUSH m_bgBrush = nullptr;
    HBRUSH m_btnBrush = nullptr;
    HFONT m_font = nullptr;
    // S1 新渲染表面（PYARX_SURFACE=1 时创建）；抽象指针，后端具体类型不透出宿主侧。
    IXmlSurface* m_surface = nullptr;
    bool m_newSurface = false;
    std::wstring m_surfaceDiag;   // 最近一次表面诊断串（探针/日志取证用）
};
