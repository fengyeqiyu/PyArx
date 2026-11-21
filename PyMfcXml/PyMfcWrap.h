// PyMfcWrap.h —— Python 侧的窗口/面板包装（PyMfcWindow / PyMfcPanel / PyMfcStore）。
// 类体内只留声明，实现在 PyMfcWrap.cpp。

#pragma once

#include "PyMfcCommon.h"
#include "PyMfcHost.h"
#include "PyMfcTopWnd.h"

    // Python 侧的 holder deleter：不做任何事。
    // 原生对象由 PyMfcStore 持有到进程结束，Python 引用计数归零时不能 delete。
    struct PyMfcNoDelete
    {
        template <typename T>
        void operator()(T*) const noexcept {}
    };

    template <typename T>
    using PyMfcHandle = std::unique_ptr<T, PyMfcNoDelete>;

class PyMfcWindow
{
public:
    PyMfcWindow() = default;
~PyMfcWindow();

    // Window.Icon：XML 里写 Icon="res/xxx.png"，建窗前登记，建窗时设成窗口图标
    // （WPF 的 Window.Icon 是 ImageSource，这里约定成相对 exe 目录的图片路径）
void setWindowIcon(const std::wstring& path);

void applyWindowIcon(HWND hwnd);

bool create(const std::wstring& title, int width, int height);

HWND hwnd() const;
PyMfcHost* Host();

    // 验收取证：整窗（含 D2D 自绘子窗）存成 PNG。
    // D2D 的内容画不进 PrintWindow（抓到全黑），所以这里抓真屏幕：先把窗口提到前台、
    // 强制出一帧，再 BitBlt(GetDC(NULL))，最后交给 WIC 编码。
bool savePng(const std::wstring& path);

    // 嵌入 CAD 主窗口的非侧栏窗体：owner = 外部传入的 HWND（如 PyApp.mainWindow）。
    // 保持 PyMfc 纯 MFC 解耦：PyMfc 不调 CAD API，owner HWND 由调用方（PyArxCore.PyApp）提供。
    // 注意：用 owned popup（WS_OVERLAPPED + owner）而非 WS_CHILD —— 若做成 MDI 主窗口的
    // WS_CHILD 子窗口，会被铺满客户区的 MDIClient 遮住看不到。owned 窗口浮动于 CAD 之上、
    // 随 owner 最小化/激活，属于 CAD 窗口体系，图形刷新直接可见。
bool CreateEmbedded(HWND parent, const std::wstring& title, int width, int height);

void setTheme(bool dark);

void setResizeCallback(py::object cb);
std::pair<int, int> ClientSize() const;
void setBackground(int r, int g, int b);
std::pair<int, int> getSize(int id) const;
// 控件矩形（宿主客户区坐标）：用来核对布局结果，含控件层 flex 容器排出来的子项位置
py::tuple getRect(int id) const;
void notifyResize(int w, int h);

void show();

void markSilentBuild();

    // guiload 建完整棵控件树后调用：先把每个子窗口都标脏并整窗重画一遍，再显示。    // 少了这一步，Embedded 窗口在加载过程中被宿主看到的就是"空壳逐块填充"。
void endSilentBuild();

void close();

void closeAsync();

void hide();

void clearCallbacks();

void addButton(int id, const std::wstring& text, int x, int y, int w, int h);
void addLabel(int id, const std::wstring& text, int x, int y, int w, int h);
void addEdit(int id, const std::wstring& text, int x, int y, int w, int h);
void addListBox(int id, int x, int y, int w, int h);
void addCheckBox(int id, const std::wstring& text, int x, int y, int w, int h);
void addRadio(int id, const std::wstring& text, int x, int y, int w, int h);
void addGroup(int id, const std::wstring& text, int x, int y, int w, int h);
void addCombo(int id, int x, int y, int w, int h);
void addEditAdvanced(int id, const std::wstring& text, bool multiline, bool readOnly,
                         int maxLength, const std::wstring& placeholder, int x, int y, int w, int h);
void addComboAdvanced(int id, bool dropDownList, int x, int y, int w, int h);
void addListBoxAdvanced(int id, bool multipleSelect, int x, int y, int w, int h);
void addProgress(int id, int x, int y, int w, int h);
void addSlider(int id, int minValue, int maxValue, int x, int y, int w, int h);
void addTab(int id, int x, int y, int w, int h);
void addTree(int id, int x, int y, int w, int h);
void addListCtrl(int id, int x, int y, int w, int h);
void addChoice(int id, int x, int y, int w, int h);
void addDatePicker(int id, int x, int y, int w, int h);
void addPanel(int id, int x, int y, int w, int h);
void addWhiteRect(int id, int x, int y, int w, int h);
void addShadowBox(int id, int x, int y, int w, int h, int pad, int radius, int dx, int dy, int blur, const std::wstring& shadowColor, const std::wstring& bgColor, const std::wstring& backColor, const std::wstring& borderColor, int borderW);
void setControlFont(int id, const std::wstring& face, int px, bool bold);
void setControlAccent(int id, int r, int g, int b);
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
void addTabItem(int id, const std::wstring& text);
void addTreeNode(int id, const std::wstring& text, int parentIndex);
void addColumn(int id, const std::wstring& text, int width);
void addRow(int id, const std::wstring& text);
void setUrl(int id, const std::wstring& url);
std::wstring getUrl(int id) const;
bool setImage(int id, const std::wstring& path);
void setRichFormat(int id, int sizePt, bool bold, bool italic, unsigned long rgb);
void setScrollRange(int id, int minValue, int maxValue);
std::pair<int, int> getScrollRange(int id) const;
int getScrollPos(int id) const;
std::pair<int, int> getSelection(int id) const;
void setSelection(int id, int from, int to) const;

void onClick(int id, py::object cb);
void setWheelRegion(int x, int y, int w, int h, int step);
void onWheel(py::object cb);
void onChange(int id, py::object cb);
void onSelect(int id, py::object cb);
void onDoubleClick(int id, py::object cb);
void onDropDown(int id, py::object cb);
void onValue(int id, py::object cb);
void onChecked(int id, py::object cb);

void setText(int id, const std::wstring& text);
std::wstring getText(int id) const;
void setVisible(int id, bool visible);
void setEnabled(int id, bool enabled);
void moveControl(int id, int x, int y, int w, int h);
void moveMany(py::list items);
    // XML 属性写入 + 表达式重算 + 重新布局（按 x:Name 定位元素）
bool setXmlValue(const std::wstring& name, const std::wstring& prop, const std::wstring& value);
    std::wstring getXmlValue(int id, const std::wstring& prop);
bool reflowXml();
void addString(int id, const std::wstring& text);
void clearStrings(int id);
int countStrings(int id) const;
std::wstring getString(int id, int index) const;
void deleteString(int id, int index);
int getCurSel(int id) const;
void setCurSel(int id, int index);
bool saveControlPng(int id, const std::wstring& path);
int getCheck(int id) const;
void setCheck(int id, bool checked);
void setRange(int id, int minValue, int maxValue);
void setPos(int id, int value);
int getPos(int id) const;
void setReadOnly(int id, bool readOnly);
bool getReadOnly(int id) const;
void setFocusTo(int id);
bool getVisible(int id) const;
bool getEnabled(int id) const;
    // HTML 对齐小控件
bool getSwitch(int id) const;
void setSwitch(int id, bool on);
int getSpin(int id) const;
void setSpin(int id, int v);
void setSpinRange(int id, int lo, int hi);
void setRangeValues(int id, int lo, int hi);
int getRangeLow(int id) const;
int getRangeHigh(int id) const;
void setPageCount(int id, int count);
int getPageCount(int id) const;
void setPageIndex(int id, int index);
int getPageIndex(int id) const;
int getDial(int id) const;
void setDial(int id, int v);
int getTumblerCount(int id) const;
void setTumblerCount(int id, int c);
int getTumblerValue(int id) const;
void setTumblerValue(int id, int v);
void setScrollBarRange(int id, int lo, int hi, int page, bool vertical);
int getScrollBarValue(int id) const;
void setScrollBarValue(int id, int v);
    // 表格类控件与工具栏
void setTableColumns(int id, const std::wstring& s);
void addTableRow(int id, const std::wstring& s);
void setTableRows(int id, const std::wstring& blob);
void setColumnWidth(int id, int col, int px);
void setColumnWidths(int id, const std::vector<int>& widths);
py::object getTableMetrics(int id);
void onBlur(int id, py::object cb);
void onDrag(int id, py::object cb);
void onCanvasEvent(int id, py::object cb);
py::object cellRect(int id, int row, int col);
py::dict perfStats(bool reset);
void onGridAction(py::object cb);
    void onGridActionFor(int id, py::object cb);
    void onControlEvent(int id, const std::wstring& ev, py::object cb);
void clearTableRows(int id);
int getTableRowCount(int id) const;
std::wstring getTableCell(int id, int r, int c) const;
void setTableCell(int id, int r, int c, const std::wstring& s);
int getTableSelectedRow(int id) const;
void setTableSelectedRow(int id, int r);
void sortTable(int id, int col);
    // 新自绘表格（PyMfcTable）接线
void addTable(int id, int x, int y, int w, int h);
void tableClear(int id);
int tableRowCount(int id) const;
void tableSetColumns(int id, const std::vector<int>& widths, const std::vector<std::wstring>& titles);
void tableSetColTypes(int id, const std::vector<int>& types);
void appendTableRow(int id, const std::vector<std::wstring>& cells);
void tableSetCell(int id, int row, int col, const std::wstring& s);
std::wstring tableGetCell(int id, int row, int col) const;
py::object tableCellRect(int id, int row, int col) const;
int tableSelectedRow(int id) const;
void tableSetSelectedRow(int id, int row);
void tableEnsureVisible(int id, int row);
void tableSetFilter(int id, int col, const std::wstring& s);
void tableClearFilters(int id);
void tableSort(int id, int col, bool desc);
int tableViewRows(int id) const;
void addSkinBox(int id, int kind, int x, int y, int w, int h);
void addTreeItem(int id, const std::wstring& s, int level, bool kids);
void clearTreeItems(int id);
int getTreeSelectedRow(int id) const;
bool isTreeItemCollapsed(int id, int r) const;
void setTreeItemCollapsed(int id, int r, bool c);
void setTreeSelectedRow(int id, int r);
void addToolBarItem(int id, const std::wstring& s);
void clearToolBarItems(int id);
int getToolBarSelected(int id) const;
void setToolBarSelected(int id, int i);
    // ---- 动画（QML: PropertyAnimation / NumberAnimation / ColorAnimation / Behavior） ----
void animate(int id, const std::wstring& prop, double to, int dur, const std::string& ease,
                 int delay, bool loop);
void animateFrom(int id, const std::wstring& prop, double from, double to, int dur,
                     const std::string& ease, int delay, bool loop);
void animateColor(int id, int rgb, int dur, const std::string& ease, int delay);
void stopAnimations(int id);
void setBehavior(int id, const std::wstring& prop, int dur, const std::string& ease);
    // ---- 浮层 ----
void showToolTip(int ownerId, const std::wstring& text, int durationMs);
void hideToolTip();
void showMenu(int id, const std::vector<std::wstring>& items, int x, int y);
void hideMenu(int id);
int getMenuSelected(int id) const;
void setMenuItems(int id, const std::vector<std::wstring>& items);
void addMenuBarItem(int id, const std::wstring& text);
void clearMenuBarItems(int id);
int getMenuBarSelected(int id) const;
void setMenuBarSelected(int id, int i);
void showPopup(int id, const std::wstring& title, const std::vector<std::wstring>& lines,
                   int x, int y, int w, int h);
void hidePopup(int id);
bool isPopupVisible(int id) const;
    // ---- 虚拟化列表 ----


void setCanvasCommands(int id, const std::wstring& cmds);
std::wstring getCanvasCommands(int id) const;
    // 绘图画布（DWG 为主，兼容已算坐标的布局输入）：建窗 + 喂数据（JSON / SVG）+ 读写视图
void addDwgCanvas(int id, int x, int y, int w, int h);
void loadDwgLayout(int id, const std::wstring& json);
void loadDwgScene(int id, const std::wstring& json);
    void copyDwgScene(int toId, int fromId);
void loadDwgSvg(int id, const std::wstring& svg);
void loadDwgXml(int id, const std::wstring& sceneXml);
    // 叠加层（选中框/橡皮筋/光标）：底图不动，只换这一层
void loadDwgOverlay(int id, const std::wstring& svg);
void loadDwgOverlayXml(int id, const std::wstring& sceneXml);
void setDwgView(int id, float zoom, int ox, int oy);
float getDwgZoom(int id) const;
std::wstring getDwgSelection(int id) const;
// FileTab 诊断：回读模型/布局几何 JSON（控件客户区坐标）
std::wstring getFileTabInfo(int id) const;
    // 画布选项：连线标注电缆 / 小地图模式（mini 的画布画主画布 miniOf 的视口红框）
void setDwgOptions(int id, bool showCable, bool mini, int miniOf);
    // 视口参数 (zoom, ox, oy)：居中定位、小地图联动都要用
py::object getDwgView(int id) const;
    // 画布鼠标诊断：返回 (lastMsg, upX, upY, wasDrag, moved_now, lastHit)
py::object dwgProbe(int id);

    // ---- 三维视图画布 ----
    // 建窗 + 喂几何（box/line/tri 文本记录）+ 读写相机（yaw/pitch/zoom/pan）
void addView3D(int id, int x, int y, int w, int h);
void view3DLoad(int id, const std::wstring& geom);
void view3DAppendBox(int id, float x0, float y0, float z0, float x1, float y1, float z1, unsigned color);
void view3DAppendLine(int id, float x0, float y0, float z0, float x1, float y1, float z1,
                      unsigned color, float width);
void view3DSetCamera(int id, float yaw, float pitch, float zoom, int panX, int panY);
py::object view3DGetCamera(int id) const;
void view3DFitView(int id);
void view3DSetStyle(int id, unsigned bg, bool axes, bool grid);
    // 命中测试（客户端坐标）：返回卡片编号，空 = 没命中。搜索/外部联动用
std::wstring hitDwg(int id, int x, int y);
bool selectDwg(int id, const std::wstring& code);
    // 卡片客户端矩形（x, y, w, h）；没有这张卡返回 None
py::object cardRectDwg(int id, const std::wstring& code);
void setDetailCard(int id, py::list rect, py::list texts);
void clearDetailCard(int id);
void setCanvasEditable(int id, bool on);
bool getCanvasEditable(int id) const;
int getLevel(int id) const;
void setLevel(int id, int level);
std::wstring getCursor(int id) const;
void setCursor(int id, const std::wstring& keyword);
std::wstring getPointerEvents(int id) const;
void setPointerEvents(int id, const std::wstring& keyword);
bool isMouseTransparent(int id) const;
void bringToFront(int id);
void raiseMany(const std::vector<int>& ids);
void bringToFrontMany(const std::vector<int>& ids);
void repaint(int id);
void sendToBack(int id);
int getTabIndex(int id) const;
void setTabIndex(int id, int n);
int getCanvasItemCount(int id) const;
std::wstring getCanvasItemOp(int id, int index) const;
std::vector<int> getCanvasItemBox(int id, int index) const;
std::vector<int> getCanvasSelection(int id) const;
void setCanvasSelection(int id, const std::vector<int>& idx);
void clearCanvasSelection(int id);
int removeCanvasItems(int id, const std::vector<int>& idx);
int removeCanvasSelection(int id);
void onCanvasSelect(int id, py::object cb);
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
    // ---- 页容器（Page / StackView / SwipeView） ----
void addPage(int id, const std::vector<int>& kids);
void clearPages(int id);
int getViewCount(int id) const;
int getViewIndex(int id) const;
void setViewIndex(int id, int index);
void animatePage(int id, int index, int durMs);
void setFlickContent(int id, const std::vector<int>& kids, int cw, int ch);
void clearFlickContent(int id);
void setFlickOffset(int id, int x, int y);
int getFlickOffsetX(int id) const;
int getFlickOffsetY(int id) const;

private:
    PyMfcTopWnd m_wnd;
    PyMfcHost m_host;
    py::object m_resize;
    std::wstring m_iconPath;
    HICON m_icon = nullptr;
    bool m_showPending = false;   // CreateEmbedded 建好但还没显示，等 guiload 建完控件树再亮相
};

class PyMfcPanel
{
public:
    // 父窗口由调用方给：CAD 侧栏页、停靠面板、嵌入窗体的 HWND 都由上层
    // （PyArxCore / 引擎）创建，核心库只在其上铺控件，不创建 CAD 对象。
explicit PyMfcPanel(HWND parent);

~PyMfcPanel();

bool valid() const;
PyMfcHost* Host() const;
HWND hwnd() const;

    // 父窗口尺寸变化后重新铺满（侧栏页 OnSize 时由 Python 侧调用）
void resize();

void addLabel(int id, const std::wstring& text, int x, int y, int w, int h);
void addButton(int id, const std::wstring& text, int x, int y, int w, int h);
void addEdit(int id, const std::wstring& text, int x, int y, int w, int h);
void addListBox(int id, int x, int y, int w, int h);
void addCheckBox(int id, const std::wstring& text, int x, int y, int w, int h);
void addRadio(int id, const std::wstring& text, int x, int y, int w, int h);
void addGroup(int id, const std::wstring& text, int x, int y, int w, int h);
void addCombo(int id, int x, int y, int w, int h);
void addEditAdvanced(int id, const std::wstring& text, bool multiline, bool readOnly,
                         int maxLength, const std::wstring& placeholder, int x, int y, int w, int h);
void addComboAdvanced(int id, bool dropDownList, int x, int y, int w, int h);
void addListBoxAdvanced(int id, bool multipleSelect, int x, int y, int w, int h);
void addProgress(int id, int x, int y, int w, int h);
void addSlider(int id, int minValue, int maxValue, int x, int y, int w, int h);
void addTab(int id, int x, int y, int w, int h);
void addTree(int id, int x, int y, int w, int h);
void addListCtrl(int id, int x, int y, int w, int h);
void addChoice(int id, int x, int y, int w, int h);
void addDatePicker(int id, int x, int y, int w, int h);
void addPanel(int id, int x, int y, int w, int h);
void addWhiteRect(int id, int x, int y, int w, int h);
void addShadowBox(int id, int x, int y, int w, int h, int pad, int radius, int dx, int dy, int blur, const std::wstring& shadowColor, const std::wstring& bgColor, const std::wstring& backColor, const std::wstring& borderColor, int borderW);
void setControlFont(int id, const std::wstring& face, int px, bool bold);
void setControlAccent(int id, int r, int g, int b);
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
void addTabItem(int id, const std::wstring& text);
void addTreeNode(int id, const std::wstring& text, int parentIndex);
void addColumn(int id, const std::wstring& text, int width);
void addRow(int id, const std::wstring& text);
void setUrl(int id, const std::wstring& url);
std::wstring getUrl(int id) const;
bool setImage(int id, const std::wstring& path);
void setRichFormat(int id, int sizePt, bool bold, bool italic, unsigned long rgb);
void setScrollRange(int id, int minValue, int maxValue);
std::pair<int, int> getScrollRange(int id) const;
int getScrollPos(int id) const;
std::pair<int, int> getSelection(int id) const;
void setSelection(int id, int from, int to) const;
void onClick(int id, py::object cb);
void setWheelRegion(int x, int y, int w, int h, int step);
void onWheel(py::object cb);
void onChange(int id, py::object cb);
void onSelect(int id, py::object cb);
void onDoubleClick(int id, py::object cb);
void onDropDown(int id, py::object cb);
void onValue(int id, py::object cb);
void onChecked(int id, py::object cb);
void setText(int id, const std::wstring& text);
std::wstring getText(int id) const;
void move(int id, int x, int y, int w, int h);
void moveMany(py::list items);
bool setXmlValue(const std::wstring& name, const std::wstring& prop, const std::wstring& value);
    std::wstring getXmlValue(int id, const std::wstring& prop);
bool reflowXml();
void setVisible(int id, bool visible);
void setEnabled(int id, bool enabled);
void addString(int id, const std::wstring& text);
void clearStrings(int id);
int countStrings(int id) const;
std::wstring getString(int id, int index) const;
void deleteString(int id, int index);
int getCurSel(int id) const;
void setCurSel(int id, int index);
bool saveControlPng(int id, const std::wstring& path);
int getCheck(int id) const;
void setCheck(int id, bool checked);
void setRange(int id, int minValue, int maxValue);
void setPos(int id, int value);
int getPos(int id) const;
void setReadOnly(int id, bool readOnly);
bool getReadOnly(int id) const;
void setFocusTo(int id);
bool getVisible(int id) const;
bool getEnabled(int id) const;
    // HTML 对齐小控件
bool getSwitch(int id) const;
void setSwitch(int id, bool on);
int getSpin(int id) const;
void setSpin(int id, int v);
void setSpinRange(int id, int lo, int hi);
void setRangeValues(int id, int lo, int hi);
int getRangeLow(int id) const;
int getRangeHigh(int id) const;
void setPageCount(int id, int count);
int getPageCount(int id) const;
void setPageIndex(int id, int index);
int getPageIndex(int id) const;
int getDial(int id) const;
void setDial(int id, int v);
int getTumblerCount(int id) const;
void setTumblerCount(int id, int c);
int getTumblerValue(int id) const;
void setTumblerValue(int id, int v);
void setScrollBarRange(int id, int lo, int hi, int page, bool vertical);
int getScrollBarValue(int id) const;
void setScrollBarValue(int id, int v);
    // 表格类控件与工具栏
void setTableColumns(int id, const std::wstring& s);
void addTableRow(int id, const std::wstring& s);
void setTableRows(int id, const std::wstring& blob);
void setColumnWidth(int id, int col, int px);
void setColumnWidths(int id, const std::vector<int>& widths);
py::object getTableMetrics(int id);
void onBlur(int id, py::object cb);
void onDrag(int id, py::object cb);
void onCanvasEvent(int id, py::object cb);
py::object cellRect(int id, int row, int col);
py::dict perfStats(bool reset);
void onGridAction(py::object cb);
    void onGridActionFor(int id, py::object cb);
    void onControlEvent(int id, const std::wstring& ev, py::object cb);
void clearTableRows(int id);
int getTableRowCount(int id) const;
std::wstring getTableCell(int id, int r, int c) const;
void setTableCell(int id, int r, int c, const std::wstring& s);
int getTableSelectedRow(int id) const;
void setTableSelectedRow(int id, int r);
void sortTable(int id, int col);
    // 新自绘表格（PyMfcTable）接线
void addTable(int id, int x, int y, int w, int h);
void tableClear(int id);
int tableRowCount(int id) const;
void tableSetColumns(int id, const std::vector<int>& widths, const std::vector<std::wstring>& titles);
void tableSetColTypes(int id, const std::vector<int>& types);
void appendTableRow(int id, const std::vector<std::wstring>& cells);
void tableSetCell(int id, int row, int col, const std::wstring& s);
std::wstring tableGetCell(int id, int row, int col) const;
py::object tableCellRect(int id, int row, int col) const;
int tableSelectedRow(int id) const;
void tableSetSelectedRow(int id, int row);
void tableEnsureVisible(int id, int row);
void tableSetFilter(int id, int col, const std::wstring& s);
void tableClearFilters(int id);
void tableSort(int id, int col, bool desc);
int tableViewRows(int id) const;
void addSkinBox(int id, int kind, int x, int y, int w, int h);
void addTreeItem(int id, const std::wstring& s, int level, bool kids);
void clearTreeItems(int id);
int getTreeSelectedRow(int id) const;
bool isTreeItemCollapsed(int id, int r) const;
void setTreeItemCollapsed(int id, int r, bool c);
void setTreeSelectedRow(int id, int r);
void addToolBarItem(int id, const std::wstring& s);
void clearToolBarItems(int id);
int getToolBarSelected(int id) const;
void setToolBarSelected(int id, int i);
    // ---- 动画 ----
void animate(int id, const std::wstring& prop, double to, int dur, const std::string& ease,
                 int delay, bool loop);
void animateFrom(int id, const std::wstring& prop, double from, double to, int dur,
                     const std::string& ease, int delay, bool loop);
void animateColor(int id, int rgb, int dur, const std::string& ease, int delay);
void stopAnimations(int id);
void setBehavior(int id, const std::wstring& prop, int dur, const std::string& ease);
    // ---- 浮层 ----
void showToolTip(int ownerId, const std::wstring& text, int durationMs);
void hideToolTip();
void showMenu(int id, const std::vector<std::wstring>& items, int x, int y);
void hideMenu(int id);
int getMenuSelected(int id) const;
void setMenuItems(int id, const std::vector<std::wstring>& items);
void addMenuBarItem(int id, const std::wstring& text);
void clearMenuBarItems(int id);
int getMenuBarSelected(int id) const;
void setMenuBarSelected(int id, int i);
void showPopup(int id, const std::wstring& title, const std::vector<std::wstring>& lines,
                   int x, int y, int w, int h);
void hidePopup(int id);
bool isPopupVisible(int id) const;
    // ---- 虚拟化列表 ----


void setCanvasCommands(int id, const std::wstring& cmds);
std::wstring getCanvasCommands(int id) const;
void addDwgCanvas(int id, int x, int y, int w, int h);
void loadDwgLayout(int id, const std::wstring& json);
void loadDwgScene(int id, const std::wstring& json);
    void copyDwgScene(int toId, int fromId);
void loadDwgSvg(int id, const std::wstring& svg);
void loadDwgXml(int id, const std::wstring& sceneXml);
    // 叠加层（选中框/橡皮筋/光标）：底图不动，只换这一层
void loadDwgOverlay(int id, const std::wstring& svg);
void loadDwgOverlayXml(int id, const std::wstring& sceneXml);
void setDwgView(int id, float zoom, int ox, int oy);
float getDwgZoom(int id) const;
std::wstring getDwgSelection(int id) const;
void setDwgOptions(int id, bool showCable, bool mini, int miniOf);
py::object getDwgView(int id) const;
std::wstring hitDwg(int id, int x, int y);
bool selectDwg(int id, const std::wstring& code);
void setDetailCard(int id, py::list rect, py::list texts);
void clearDetailCard(int id);
void setCanvasEditable(int id, bool on);
bool getCanvasEditable(int id) const;
int getLevel(int id) const;
void setLevel(int id, int level);
std::wstring getCursor(int id) const;
void setCursor(int id, const std::wstring& keyword);
std::wstring getPointerEvents(int id) const;
void setPointerEvents(int id, const std::wstring& keyword);
bool isMouseTransparent(int id) const;
void bringToFront(int id);
void raiseMany(const std::vector<int>& ids);
void bringToFrontMany(const std::vector<int>& ids);
void repaint(int id);
void sendToBack(int id);
int getTabIndex(int id) const;
void setTabIndex(int id, int n);
    // ---- 三维视图画布（面板版）----
void addView3D(int id, int x, int y, int w, int h);
void view3DLoad(int id, const std::wstring& geom);
void view3DAppendBox(int id, float x0, float y0, float z0, float x1, float y1, float z1, unsigned color);
void view3DAppendLine(int id, float x0, float y0, float z0, float x1, float y1, float z1,
                      unsigned color, float width);
void view3DSetCamera(int id, float yaw, float pitch, float zoom, int panX, int panY);
py::object view3DGetCamera(int id) const;
void view3DFitView(int id);
void view3DSetStyle(int id, unsigned bg, bool axes, bool grid);
int getCanvasItemCount(int id) const;
std::wstring getCanvasItemOp(int id, int index) const;
std::vector<int> getCanvasItemBox(int id, int index) const;
std::vector<int> getCanvasSelection(int id) const;
void setCanvasSelection(int id, const std::vector<int>& idx);
void clearCanvasSelection(int id);
int removeCanvasItems(int id, const std::vector<int>& idx);
int removeCanvasSelection(int id);
void onCanvasSelect(int id, py::object cb);
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
    // ---- 页容器（Page / StackView / SwipeView） ----
void addPage(int id, const std::vector<int>& kids);
void clearPages(int id);
int getViewCount(int id) const;
int getViewIndex(int id) const;
void setViewIndex(int id, int index);
void animatePage(int id, int index, int durMs);
void setFlickContent(int id, const std::vector<int>& kids, int cw, int ch);
void clearFlickContent(int id);
int getFlickOffsetX(int id) const;
int getFlickOffsetY(int id) const;

private:
    PyMfcHost* m_host = nullptr;
};

struct PyMfcStore
{
    std::vector<PyMfcWindow*> windows;
    std::vector<PyMfcPanel*> panels;

    static PyMfcStore& Get()
    {
        static PyMfcStore store;
        return store;
    }

    PyMfcWindow* NewWindow()
    {
        auto* window = new PyMfcWindow();
        windows.push_back(window);
        return window;
    }

    // 控件容器：挂在调用方给的父窗口上（侧栏页 / 停靠面板），核心库只铺控件
    PyMfcPanel* NewPanel(HWND parent)
    {
        auto* panel = new PyMfcPanel(parent);
        panels.push_back(panel);
        return panel;
    }
};

