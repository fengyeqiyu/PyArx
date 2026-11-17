#pragma once
// IXmlMfcHost —— 标准 MFC 控件工厂所依赖的宿主抽象接口。
//
// PyMfcHost（G:\PyArx\PyMfcCore\PyMfcCore.cpp）实现本接口；各控件类（XmlButton 等）
// 只依赖本接口，不直接触碰 PyMfcHost/MFC 细节，从而实现"一个类一个文件、
// 逐个包装到 XML"的工程化组织，且未来设计器可复用同一套控件实现。
//
// 编码：本文件 UTF-8 with BOM。

#include <string>

class XmlElement;

namespace xmlapp
{
    struct XmlAppearance;
}

struct IXmlMfcHost
{
    virtual ~IXmlMfcHost() = default;

    // 创建控件（宿主负责 ID 校验、自动字体、DarkMode 主题）
    virtual void addButton(int id, const std::wstring& text, int x, int y, int w, int h) = 0;
    virtual void addLabel(int id, const std::wstring& text, int x, int y, int w, int h) = 0;
    virtual void addEdit(int id, const std::wstring& text, int x, int y, int w, int h) = 0;
    virtual void addListBox(int id, int x, int y, int w, int h) = 0;
    virtual void addCheckBox(int id, const std::wstring& text, int x, int y, int w, int h) = 0;
    virtual void addRadio(int id, const std::wstring& text, int x, int y, int w, int h) = 0;
    virtual void addGroup(int id, const std::wstring& text, int x, int y, int w, int h) = 0;
    virtual void addCombo(int id, int x, int y, int w, int h) = 0;
    virtual void addProgress(int id, int x, int y, int w, int h) = 0;
    virtual void addSlider(int id, int minValue, int maxValue, int x, int y, int w, int h) = 0;
    // 扩展控件
    virtual void addTab(int id, int x, int y, int w, int h) = 0;
    virtual void addTree(int id, int x, int y, int w, int h) = 0;
    virtual void addListCtrl(int id, int x, int y, int w, int h) = 0;
    virtual void addChoice(int id, int x, int y, int w, int h) = 0;
    virtual void addDatePicker(int id, int x, int y, int w, int h) = 0;
    virtual void addPanel(int id, int x, int y, int w, int h) = 0;
    // 白底矩形（侧板/浮层背景）。默认退化成普通面板，宿主想画白底才覆盖
    virtual void addWhiteRect(int id, int x, int y, int w, int h) { addPanel(id, x, y, w, h); }
    virtual void addSplitter(int id, int x, int y, int w, int h) = 0;
    virtual void addStaticLine(int id, int x, int y, int w, int h) = 0;
    virtual void addHyperlink(int id, const std::wstring& text, int x, int y, int w, int h) = 0;
    virtual void addRichText(int id, int x, int y, int w, int h) = 0;
    virtual void addToggle(int id, const std::wstring& text, int x, int y, int w, int h) = 0;
    virtual void addBitmapButton(int id, const std::wstring& text, int x, int y, int w, int h) = 0;
    virtual void addStaticBitmap(int id, int x, int y, int w, int h) = 0;
    virtual void addGrid(int id, int x, int y, int w, int h) = 0;
    virtual void addScrolledWindow(int id, int x, int y, int w, int h) = 0;
    virtual void addCalendar(int id, int x, int y, int w, int h) = 0;

    // 日历选中日期的读写（值格式 "YYYY-MM-DD"）。默认实现不支持，控件属性会读到空。
    virtual std::wstring getCalendarDate(int /*id*/) const { return {}; }
    virtual bool setCalendarDate(int /*id*/, const std::wstring& /*value*/) { return false; }

    // 创建"带基础功能"的输入类控件（文本框可多行/只读/限长/占位提示，
    // 下拉框可选可编辑或只读下拉，列表框可多选）。默认实现退回普通创建，
    // 让只关心外观的实现（如设计器自绘侧）不必关心这些细节。
    virtual void addEditAdvanced(int id, const std::wstring& text, bool multiline, bool readOnly,
                                 int maxLength, const std::wstring& placeholder,
                                 int x, int y, int w, int h)
    {
        addEdit(id, text, x, y, w, h);
        if (readOnly) setReadOnly(id, true);
        if (maxLength > 0) setLimitText(id, maxLength);
        if (!placeholder.empty()) setCueBanner(id, placeholder);
    }
    virtual void addComboAdvanced(int id, bool dropDownList, int x, int y, int w, int h)
    {
        addCombo(id, x, y, w, h);
    }

    // 代码编辑器：语法高亮（xml / python）。默认实现退回"多行编辑框"，
    // 让运行时/预览器即便还没接上自绘代码视图也能先把控件建出来。
    virtual void addCodeEditor(int id, const std::wstring& text, const std::wstring& syntax,
                               bool readOnly, int x, int y, int w, int h)
    {
        addEditAdvanced(id, text, true, readOnly, 0, std::wstring(), x, y, w, h);
    }
    virtual void addListBoxAdvanced(int id, bool multipleSelect, int x, int y, int w, int h)
    {
        addListBox(id, x, y, w, h);
    }

    // 编辑框的输入长度上限与空内容占位提示
    virtual void setLimitText(int, int) {}
    virtual void setCueBanner(int, const std::wstring&) {}

    // 属性操作
    virtual void setText(int id, const std::wstring& text) = 0;
    virtual std::wstring getText(int id) const = 0;
    virtual void setVisible(int id, bool visible) = 0;
    virtual void setEnabled(int id, bool enabled) = 0;
    virtual void moveControl(int id, int x, int y, int w, int h) = 0;
    virtual void addString(int id, const std::wstring& text) = 0;
    virtual void clearStrings(int id) = 0;
    virtual int countStrings(int id) const = 0;
    virtual std::wstring getString(int id, int index) const = 0;
    virtual void deleteString(int id, int index) = 0;
    virtual int getCurSel(int id) const = 0;
    virtual void setCurSel(int id, int index) = 0;
    virtual int getCheck(int id) const = 0;
    virtual void setCheck(int id, bool checked) = 0;
    virtual void setRange(int id, int minValue, int maxValue) = 0;
    virtual void setPos(int id, int value) = 0;
    virtual int getPos(int id) const = 0;
    virtual void setReadOnly(int id, bool readOnly) = 0;
    virtual bool getReadOnly(int id) const = 0;

    // 内容填充：Tab/Tree/ListCtrl/Grid 只有窗口不等于有内容，XML 属性与 Python
    // 接口都要落到这几个方法上，否则它们只能显示空白壳。
    virtual void addTabItem(int, const std::wstring&) {}
    virtual void addTreeNode(int, const std::wstring&, int /*parentIndex*/) {}
    virtual void addColumn(int, const std::wstring&, int /*width*/) {}
    virtual void addRow(int, const std::wstring&) {}

    // 第二批功能接口：链接地址、图片、富文本格式、滚动范围。
    // 默认实现什么都不做，让只关心外观的实现（设计器自绘侧）不必实现它们。
    virtual void setUrl(int, const std::wstring&) {}
    virtual bool setImage(int, const std::wstring&) { return false; }
    virtual void setRichFormat(int, int /*sizePt*/, bool /*bold*/, bool /*italic*/, unsigned long /*rgb*/) {}
    virtual void setScrollRange(int, int /*minValue*/, int /*maxValue*/) {}

    // QML 对齐的一批"自建画布"小控件（Switch / SpinBox / RangeSlider / BusyIndicator /
    // PageIndicator）。Win32 没有对应系统类，宿主单独建一个画布窗口，状态留在宿主里。
    virtual void addSkinBox(int /*id*/, int /*kind*/, int /*x*/, int /*y*/, int /*w*/, int /*h*/) {}

    // 配电规划图画布：同样没有系统类，宿主建一个自绘窗口，把布局结果画上去。
    // 缩放/平移状态也留在宿主里，因为重绘是宿主发起的（InvalidateRect）。
    virtual void addPowerCanvas(int /*id*/, int /*x*/, int /*y*/, int /*w*/, int /*h*/) {}
    // 传 JSON（= powerplan.toJson 的输出）。宿主解析后自己重排、自己算 fit 视图。
    virtual void setPowerCanvasPlan(int /*id*/, const std::wstring& /*json*/) {}
    // 传"坐标已算好"的场景 JSON（powerplan.toJson 同构）。宿主只按坐标画，不排版。
    virtual void setPowerCanvasScene(int /*id*/, const std::wstring& /*json*/) {}
    // 传 SVG 文本，走同一条绘制路径（几何 + 文字都已解析成场景）。
    virtual void setPowerCanvasSvg(int /*id*/, const std::wstring& /*svgText*/) {}
    // 叠加层 SVG：只重解析这一层（选中框/橡皮筋/光标），底图保持不变。
    virtual void setPowerCanvasOverlay(int /*id*/, const std::wstring& /*svgText*/) {}
    virtual void setPowerCanvasView(int /*id*/, float /*zoom*/, int /*offsetX*/, int /*offsetY*/) {}
    virtual float getPowerCanvasZoom(int /*id*/) const { return 1.0f; }
    virtual std::wstring getPowerCanvasSelection(int /*id*/) const { return {}; }
    // 画布选项：电缆标注开关 / 小地图模式（mini 时本画布只做缩略图并画主画布的视口红框）
    virtual void setPowerCanvasOptions(int /*id*/, bool /*showCable*/, bool /*mini*/, int /*miniOf*/) {}

    // ---- Ribbon（控件库 xmlctl::XRibbon 模型 + 布局 + 绘制 + 命中，宿主只建自绘窗口）----
    virtual void addRibbon(int /*id*/, int /*x*/, int /*y*/, int /*w*/, int /*h*/) {}
    // 把 Ribbon 的 XML 子树（Tab/Group/Button）交给宿主解析成模型；el 为 XmlElement*
    virtual void setRibbonElement(int /*id*/, const XmlElement* /*el*/) {}
    virtual void getPowerCanvasView(int /*id*/, float& zoom, int& ox, int& oy) const
    {
        zoom = 1.0f;
        ox = oy = 0;
    }
    virtual bool skinBoxMouse(HWND /*hwnd*/, UINT /*msg*/, WPARAM /*wp*/, LPARAM /*lp*/) { return false; }

    // ---- 三维视图画布（View3D）----
    // 宿主建自绘窗口；几何以文本记录喂进去（见 XmlView3D.h 的 Data 格式），
    // 相机（yaw/pitch/zoom/pan）留在宿主里，鼠标拖动旋转、滚轮缩放都在宿主处理。
    virtual void addView3D(int /*id*/, int /*x*/, int /*y*/, int /*w*/, int /*h*/) {}
    virtual void setView3DModel(int /*id*/, const std::wstring& /*geom*/) {}
    virtual void setView3DCamera(int /*id*/, float /*yaw*/, float /*pitch*/, float /*zoom*/,
                                 int /*panX*/, int /*panY*/) {}
    virtual void getView3DCamera(int /*id*/, float& yaw, float& pitch, float& zoom,
                                 int& panX, int& panY) const
    {
        yaw = 35.0f; pitch = 22.0f; zoom = 1.0f; panX = panY = 0;
    }
    virtual void view3DFit(int /*id*/) {}
    // 造型：背景色 + 是否画坐标轴 + 是否画网格地面
    virtual void setView3DStyle(int /*id*/, COLORREF /*bg*/, bool /*axes*/, bool /*grid*/) {}

    virtual void setRangeValues(int /*id*/, int /*low*/, int /*high*/) {}
    virtual int getRangeLow(int /*id*/) const { return 0; }
    virtual int getRangeHigh(int /*id*/) const { return 0; }
    virtual bool getSwitchValue(int /*id*/) const { return false; }
    virtual void setSwitchValue(int /*id*/, bool /*on*/) {}
    virtual int getSpinValue(int /*id*/) const { return 0; }
    virtual void setSpinValue(int /*id*/, int /*value*/) {}
    virtual void setSpinRange(int /*id*/, int /*lo*/, int /*hi*/) {}
    virtual void setPageCount(int /*id*/, int /*count*/) {}
    virtual int getPageCount(int /*id*/) const { return 0; }
    virtual void setPageIndex(int /*id*/, int /*index*/) {}
    virtual int getPageIndex(int /*id*/) const { return 0; }

    // 第二批自建画布控件：Dial（圆弧指针）、Tumbler（滚动选择）、独立 ScrollBar
    virtual int getDial(int /*id*/) const { return 0; }
    virtual void setDial(int /*id*/, int /*value*/) {}
    virtual int getTumblerCount(int /*id*/) const { return 10; }
    virtual void setTumblerCount(int /*id*/, int /*count*/) {}
    virtual int getTumblerValue(int /*id*/) const { return 0; }
    virtual void setTumblerValue(int /*id*/, int /*index*/) {}
    virtual void stepTumbler(int /*id*/, int /*delta*/) {}
    virtual void setScrollBarRange(int /*id*/, int /*lo*/, int /*hi*/, int /*page*/,
                                   bool /*vertical*/) {}
    virtual int getScrollBarValue(int /*id*/) const { return 0; }
    virtual void setScrollBarValue(int /*id*/, int /*value*/) {}

    // 表格类（TableView / TreeView）与工具栏。表格参数都用制表符分隔的字符串传，
    // 避免在接口层引入 std::vector<std::string> 的跨模块 ABI。
    virtual void setTableColumns(int /*id*/, const std::wstring& /*tabJoined*/) {}
    virtual void addTableRow(int /*id*/, const std::wstring& /*tabJoined*/) {}
    virtual void clearTableRows(int /*id*/) {}
    virtual int getTableRowCount(int /*id*/) const { return 0; }
    virtual std::wstring getTableCell(int /*id*/, int /*row*/, int /*col*/) const { return {}; }
    virtual void setTableCell(int /*id*/, int /*row*/, int /*col*/, const std::wstring& /*text*/) {}
    virtual int getTableSelectedRow(int /*id*/) const { return -1; }
    virtual void setTableSelectedRow(int /*id*/, int /*row*/) {}
    virtual void sortTable(int /*id*/, int /*col*/) {}
    virtual void addTreeItem(int /*id*/, const std::wstring& /*text*/, int /*level*/,
                             bool /*hasKids*/) {}
    virtual void clearTreeItems(int /*id*/) {}
    virtual int getTreeSelectedRow(int /*id*/) const { return -1; }
    virtual void setTreeSelectedRow(int /*id*/, int /*row*/) {}
    virtual bool isTreeItemCollapsed(int /*id*/, int /*row*/) const { return false; }
    virtual void setTreeItemCollapsed(int /*id*/, int /*row*/, bool /*collapsed*/) {}
    virtual void addToolBarItem(int /*id*/, const std::wstring& /*text*/) {}
    virtual void clearToolBarItems(int /*id*/) {}
    virtual int getToolBarSelected(int /*id*/) const { return -1; }
    virtual void setToolBarSelected(int /*id*/, int /*index*/) {}

    // ---- 浮层族：ToolTip / Menu / MenuBar / Popup ----
    virtual void setMenuItems(int /*id*/, const std::vector<std::wstring>& /*items*/) {}
    virtual int getMenuSelected(int /*id*/) const { return -1; }
    virtual void addMenuBarItem(int /*id*/, const std::wstring& /*text*/) {}
    virtual void clearMenuBarItems(int /*id*/) {}
    virtual int getMenuBarSelected(int /*id*/) const { return -1; }
    virtual void setMenuBarSelected(int /*id*/, int /*index*/) {}
    virtual void showToolTip(int /*ownerId*/, const std::wstring& /*text*/, int /*durationMs*/) {}
    virtual void hideToolTip() {}
    virtual void showMenu(int /*id*/, const std::vector<std::wstring>& /*items*/, int /*x*/, int /*y*/) {}
    virtual void hideMenu(int /*id*/) {}
    virtual void showPopup(int /*id*/, const std::wstring& /*title*/,
                           const std::vector<std::wstring>& /*lines*/,
                           int /*x*/, int /*y*/, int /*w*/, int /*h*/) {}
    virtual void hidePopup(int /*id*/) {}
    virtual bool isPopupVisible(int /*id*/) const { return false; }

    // ---- 虚拟化列表（ListModel + delegate） ----
    // ---- QCanvas：画命令列表 ----
    virtual void setCanvasCommands(int /*id*/, const std::wstring& /*cmds*/) {}
    // GridView 格子尺寸覆盖（0 = 自适应）
    virtual void setGridCellSize(int /*id*/, int /*cellW*/, int /*cellH*/) {}
    virtual std::wstring getCanvasCommands(int /*id*/) const { return {}; }
    virtual void addListItem(int /*id*/, const std::wstring& /*line*/) {}
    virtual void clearListItems(int /*id*/) {}
    virtual int getListItemCount(int /*id*/) const { return 0; }
    virtual std::wstring getListItem(int /*id*/, int /*index*/) const { return {}; }
    virtual int getListSelected(int /*id*/) const { return -1; }
    virtual void setListSelected(int /*id*/, int /*index*/) {}
    virtual int getListScroll(int /*id*/) const { return 0; }
    virtual void setListScroll(int /*id*/, int /*pos*/) {}

    // ---- 页容器（Page / StackView / SwipeView） ----
    virtual void addPage(int /*id*/, const std::vector<int>& /*kids*/) {}
    virtual void clearPages(int /*id*/) {}
    virtual int viewCount(int /*id*/) const { return 0; }
    virtual int viewIndex(int /*id*/) const { return 0; }
    virtual void viewSetIndex(int /*id*/, int /*index*/) {}
    virtual void viewAnimate(int /*id*/, int /*index*/, int /*durMs*/) {}

    // ---- Flickable：可拖拽/惯性/回弹的内容视口 ----
    virtual void setFlickContent(int /*id*/, const std::vector<int>& /*kids*/,
                                 int /*contentW*/, int /*contentH*/) {}
    virtual void clearFlickContent(int /*id*/) {}
    virtual void setFlickOffset(int /*id*/, int /*x*/, int /*y*/) {}
    virtual int getFlickOffsetX(int /*id*/) const { return 0; }
    virtual int getFlickOffsetY(int /*id*/) const { return 0; }

    // ---- 通用基础属性：绘图层次 / Tab 序号（所有控件共用，控件基类负责解析） ----
    // Level     越大越靠上，只改叠放次序；
    // TabIndex  只决定 Tab 键遍历次序，与叠放次序解耦。
    // 默认空实现：不支持排布的后端（如部分自绘侧）不必实现也不报错。
    virtual void setControlLevel(int /*id*/, int /*level*/) {}
    virtual int getControlLevel(int /*id*/) const { return 0; }
    virtual void setControlTabIndex(int /*id*/, int /*index*/) {}
    virtual int getControlTabIndex(int /*id*/) const { return -1; }

    // ---- 通用基础属性：主题 ----
    // 主题名（如 "Dark" / "Light" / 自定义皮肤名）。颜色分配规则还没定，
    // 所以先只做"属性可声明、可回读、能推到宿主"这条通路，宿主可以暂不消费。
    virtual void setControlTheme(int /*id*/, const std::wstring& /*theme*/) {}
    virtual std::wstring getControlTheme(int /*id*/) const { return {}; }

    // 取控件 HWND（供 IXmlControl::nativeHandle 返回）
    virtual void* getDlgItemHandle(int id) const = 0;

    // 外观（形状/颜色/风格/前置背景/后置背景）：控件类解析 XML 后推给宿主，
    // 宿主在绘制（WM_DRAWITEM / WM_CTLCOLOR）时按此渲染。
    virtual void setControlAppearance(int id, const xmlapp::XmlAppearance& ap) = 0;

    // 控件图标（Icon="res/xxx.png"，相对 exe 目录）：owner-draw 时画在控件左侧，
    // 有文字时文字右移。与设计器、预览器共用同一条 XmlCanvas::drawImage 通路。
    virtual void setControlIcon(int id, const std::wstring& path) = 0;

    // 容器背景：Canvas/Grid/DockPanel 等不创建原生控件，其 Background 只能由宿主铺底，
    // 否则 XML 里写的白画布在运行时看不见（设计器和预览器都会画）。
    virtual void addContainerBackdrop(int x, int y, int w, int h,
                                      const xmlapp::XmlAppearance& ap) = 0;
    virtual void clearContainerBackdrops() = 0;
};
