// PyMfcBind.cpp —— PYBIND11_MODULE(PyMfcXml)：全部 Python 接口定义。
// 原名 parts\06-bind.inc。整个工程只有这一处模块定义。

#include "PyMfcCommon.h"
#include "PyMfcWrap.h"
#include "PyMfcParts.h"

// ---- QML model 语义：Python 值 -> XML 属性字符串 ----
// 一维 list/tuple -> "A|B|C"（对应 QML 的 ListModel）
// 二维 list[list]  -> 行用 "|"、单元格用 "\t"（对应表格的 model + 多列 delegate）
static std::wstring xmlPyScalar(py::handle v)
{
    if (v.is_none())
        return L"";
    if (py::isinstance<py::str>(v))
        return v.cast<std::wstring>();
    return py::str(v).cast<std::wstring>();
}

static std::wstring xmlPyPropValue(py::handle v)
{
    if (!py::isinstance<py::list>(v) && !py::isinstance<py::tuple>(v))
        return xmlPyScalar(v);
    std::wstring out;
    bool first = true;
    for (py::handle item : v)
    {
        if (!first)
            out += L"|";
        first = false;
        if (py::isinstance<py::list>(item) || py::isinstance<py::tuple>(item))
        {
            bool firstCell = true;
            for (py::handle cell : item)
            {
                if (!firstCell)
                    out += L"\t";
                firstCell = false;
                out += xmlPyScalar(cell);
            }
        }
        else
        {
            out += xmlPyScalar(item);
        }
    }
    return out;
}

// ItemView 的 model 接 Python list 时按 XML 里同一个写法拼：
// 一维 list → 每个元素一行（`甲;乙`）；二维 list → 行内字段用竖线（`a|b;c|d`）。
// 列表控件（ListBox/ListView）仍走旧的 `|` 分行 + 制表符分字段格式，两者不能混。
static std::wstring xmlPyPropValueFor(PyMfcHost* host, const std::wstring& name,
                                       const std::wstring& prop, py::handle v)
{
    const bool isModel = _wcsicmp(prop.c_str(), L"model") == 0;
    const bool isList = py::isinstance<py::list>(v) || py::isinstance<py::tuple>(v);
    if (isModel && isList && host != nullptr)
    {
        std::shared_ptr<XmlRuntime> rt = host->xmlRuntime();
        if (rt && _wcsicmp(rt->kindOfElement(name).c_str(), L"itemview") == 0)
        {
            std::wstring out;
            bool firstRow = true;
            for (py::handle row : v)
            {
                if (!firstRow)
                    out += L";";
                firstRow = false;
                if (py::isinstance<py::list>(row) || py::isinstance<py::tuple>(row))
                {
                    bool firstCell = true;
                    for (py::handle cell : row)
                    {
                        if (!firstCell)
                            out += L"|";
                        firstCell = false;
                        out += xmlPyScalar(cell);
                    }
                }
                else
                    out += xmlPyScalar(row);
            }
            return out;
        }
    }
    return xmlPyPropValue(v);
}

PYBIND11_MODULE(PyMfcXml, m)
{
    m.doc() = "PyMfcCore — MFC 原生控件最小 Python 接口";

    // 建窗/建控件都依赖 MFC 模块态，必须在任何 GUI 调用前完成自举
    ensureMfcApp();

    // 注册全部标准 MFC 控件到统一 XML 运行时注册表（自定义控件由 register_control 追加）
    RegisterPyMfcControls();

    // PyMfcCore.xml：PyMfcXml 加载器（解析/遍历/布局/序列化），不建窗口
    RegisterXmlBindings(m);

    py::class_<PyMfcWindow, PyMfcHandle<PyMfcWindow>>(m, "Window")
        .def(py::init([](const std::wstring& title, int width, int height) -> PyMfcHandle<PyMfcWindow> {
            auto* window = PyMfcStore::Get().NewWindow();
            if (!window->create(title, width, height))
                throw std::runtime_error("PyMfc Window create failed");
            return PyMfcHandle<PyMfcWindow>(window);
        }), py::arg("title"), py::arg("width"), py::arg("height"))
        .def_property_readonly("hwnd", [](PyMfcWindow& w) {
            return static_cast<long long>(reinterpret_cast<std::intptr_t>(w.hwnd()));
        }, "窗口句柄（本地 Python 下可交给 pump 之外的 Win32 API 使用）")
        .def("setTheme", &PyMfcWindow::setTheme, py::arg("dark"))
        .def("onResize", &PyMfcWindow::setResizeCallback)
        .def("clientSize", &PyMfcWindow::ClientSize)
        .def("show", &PyMfcWindow::show)
        .def("close", &PyMfcWindow::close)
        .def("addButton", &PyMfcWindow::addButton)
        .def("addLabel", &PyMfcWindow::addLabel)
        .def("addEdit", &PyMfcWindow::addEdit)
        .def("addListBox", &PyMfcWindow::addListBox)
        .def("addCheckBox", &PyMfcWindow::addCheckBox)
        .def("addRadio", &PyMfcWindow::addRadio)
        .def("addGroup", &PyMfcWindow::addGroup)
        .def("addCombo", &PyMfcWindow::addCombo)
        .def("addProgress", &PyMfcWindow::addProgress)
        .def("addSlider", &PyMfcWindow::addSlider)
        // 扩展控件：以前这 16 类只在 XML 里能用，Python 侧根本没有对应方法，
        // 于是"Python 能不能用这些控件"这件事一直没答案
        .def("addTab", &PyMfcWindow::addTab)
        .def("addTree", &PyMfcWindow::addTree)
        .def("addListCtrl", &PyMfcWindow::addListCtrl)
        .def("addChoice", &PyMfcWindow::addChoice)
        .def("addDatePicker", &PyMfcWindow::addDatePicker)
        .def("addPanel", &PyMfcWindow::addPanel)
        .def("addWhiteRect", &PyMfcWindow::addWhiteRect)
    .def("addShadowBox", &PyMfcWindow::addShadowBox)
        .def("setControlFont", &PyMfcWindow::setControlFont,
             py::arg("id"), py::arg("face"), py::arg("px"), py::arg("bold") = false)
        .def("addSplitter", &PyMfcWindow::addSplitter)
        .def("addStaticLine", &PyMfcWindow::addStaticLine)
        .def("addHyperlink", &PyMfcWindow::addHyperlink)
        .def("addRichText", &PyMfcWindow::addRichText)
        .def("addToggle", &PyMfcWindow::addToggle)
        .def("addBitmapButton", &PyMfcWindow::addBitmapButton)
        .def("addStaticBitmap", &PyMfcWindow::addStaticBitmap)
        .def("addGrid", &PyMfcWindow::addGrid)
        .def("addScrolledWindow", &PyMfcWindow::addScrolledWindow)
        .def("addCalendar", &PyMfcWindow::addCalendar)
        // 内容填充：只有窗口没有内容的控件就是空壳，这几条是"能装东西"的入口
        .def("addTabItem", &PyMfcWindow::addTabItem, py::arg("id"), py::arg("text"))
        .def("addTreeNode", &PyMfcWindow::addTreeNode, py::arg("id"), py::arg("text"),
             py::arg("parent_index") = -1)
        .def("addColumn", &PyMfcWindow::addColumn, py::arg("id"), py::arg("text"),
             py::arg("width") = 120)
        .def("addRow", &PyMfcWindow::addRow, py::arg("id"), py::arg("text"))
        // 第二批：链接地址、图片装载、富文本格式、滚动范围
        .def("setUrl", &PyMfcWindow::setUrl, py::arg("id"), py::arg("url"))
        .def("getUrl", &PyMfcWindow::getUrl)
        .def("setImage", &PyMfcWindow::setImage, py::arg("id"), py::arg("path"),
             "给 StaticBitmap / BitmapButton 装图片（png/jpg 均可）")
        .def("setRichFormat", &PyMfcWindow::setRichFormat, py::arg("id"),
             py::arg("size_pt") = 9, py::arg("bold") = false, py::arg("italic") = false,
             py::arg("rgb") = 0xFFFFFF, "设置 RichText 的字号/粗体/斜体/颜色")
        .def("setScrollRange", &PyMfcWindow::setScrollRange, py::arg("id"),
             py::arg("min_value"), py::arg("max_value"), "设置 ScrolledWindow 的滚动范围")
        .def("getScrollRange", &PyMfcWindow::getScrollRange)
        .def("getScrollPos", &PyMfcWindow::getScrollPos, py::arg("id"),
             "读 ScrolledWindow 当前的滚动位置")
        .def("getSelection", &PyMfcWindow::getSelection, py::arg("id"),
             "读文本控件当前的选区，返回 (起点, 终点)")
        .def("savePng", &PyMfcWindow::savePng, py::arg("path"),
             "把整个窗口（含 D2D 自绘子窗）存成 PNG：验收取证用，抓的是真屏幕")
        .def("setSelection", &PyMfcWindow::setSelection, py::arg("id"), py::arg("from"),
             py::arg("to"), "设置文本控件选区")
        .def("onClick", &PyMfcWindow::onClick)
        .def("setWheelRegion", &PyMfcWindow::setWheelRegion, py::arg("x"), py::arg("y"),
             py::arg("w"), py::arg("h"), py::arg("step") = 3,
             "注册收滚轮的区域（客户区坐标）：鼠标在里面滚动就回调 onWheel(±step)")
        .def("onWheel", &PyMfcWindow::onWheel, py::arg("cb"),
             "cb(delta)，delta 为“往下为正”的行增量；MFCXml 不替子控件滚动，行位置自己算")
        .def("onChange", &PyMfcWindow::onChange)
        .def("onSelect", &PyMfcWindow::onSelect)
        .def("onDoubleClick", &PyMfcWindow::onDoubleClick)
        .def("onDropDown", &PyMfcWindow::onDropDown)
        .def("onValue", &PyMfcWindow::onValue)
        .def("onChecked", &PyMfcWindow::onChecked)
        .def("setText", &PyMfcWindow::setText)
        .def("getText", &PyMfcWindow::getText)
        .def("setVisible", &PyMfcWindow::setVisible)
        .def("getVisible", &PyMfcWindow::getVisible)
        .def("setEnabled", &PyMfcWindow::setEnabled)
        .def("getEnabled", &PyMfcWindow::getEnabled)
        .def("move", &PyMfcWindow::moveControl)
        .def("moveMany", &PyMfcWindow::moveMany, py::arg("items"),
             "items=[(id,x,y,w,h,visible)]，一次 DeferWindowPos 原子提交，滚动时不再一次一重绘")
        // XML 表达式：改一个属性值，依赖它的控件按表达式重算并重排
        .def("setXmlValue", [](PyMfcWindow& w, const std::wstring& name,
                                const std::wstring& prop, py::object value) {
            return w.setXmlValue(name, prop, xmlPyPropValueFor(w.Host(), name, prop, value));
        }, py::arg("name"), py::arg("prop"), py::arg("value"),
             "value 可为 str，也可为 list（一维 -> model/items，二维 -> 表格行）")
        .def("reflowXml", &PyMfcWindow::reflowXml)
        .def("addString", &PyMfcWindow::addString)
        .def("clearStrings", &PyMfcWindow::clearStrings)
        .def("countStrings", &PyMfcWindow::countStrings)
        .def("getString", &PyMfcWindow::getString)
        .def("deleteString", &PyMfcWindow::deleteString)
        .def("getCurSel", &PyMfcWindow::getCurSel)
        .def("setCurSel", &PyMfcWindow::setCurSel)
        .def("getCheck", &PyMfcWindow::getCheck)
        .def("setCheck", &PyMfcWindow::setCheck)
        .def("setRange", &PyMfcWindow::setRange)
        .def("setPos", &PyMfcWindow::setPos)
        .def("getPos", &PyMfcWindow::getPos)
        .def("setReadOnly", &PyMfcWindow::setReadOnly)
        .def("getReadOnly", &PyMfcWindow::getReadOnly)
        // QML 对齐小控件
        .def("getSwitch", &PyMfcWindow::getSwitch)
        .def("setSwitch", &PyMfcWindow::setSwitch)
        .def("getSpin", &PyMfcWindow::getSpin)
        .def("setSpin", &PyMfcWindow::setSpin)
        .def("setSpinRange", &PyMfcWindow::setSpinRange)
        .def("setRangeValues", &PyMfcWindow::setRangeValues)
        .def("getRangeLow", &PyMfcWindow::getRangeLow)
        .def("getRangeHigh", &PyMfcWindow::getRangeHigh)
        .def("setPageCount", &PyMfcWindow::setPageCount)
        .def("getPageCount", &PyMfcWindow::getPageCount)
        .def("getPageIndex", &PyMfcWindow::getPageIndex)
        .def("getDial", &PyMfcWindow::getDial)
        .def("setDial", &PyMfcWindow::setDial)
        .def("getTumblerCount", &PyMfcWindow::getTumblerCount)
        .def("setTumblerCount", &PyMfcWindow::setTumblerCount)
        .def("getTumblerValue", &PyMfcWindow::getTumblerValue)
        .def("setTumblerValue", &PyMfcWindow::setTumblerValue)
        .def("setScrollBarRange", &PyMfcWindow::setScrollBarRange)
        .def("getScrollBarValue", &PyMfcWindow::getScrollBarValue)
        .def("setScrollBarValue", &PyMfcWindow::setScrollBarValue)
        .def("setTableColumns", &PyMfcWindow::setTableColumns)
        .def("addTableRow", &PyMfcWindow::addTableRow)
            .def("setTableRows", &PyMfcWindow::setTableRows,
                 "整表一次灌入：行间\\n、格间\\t")
        .def("onGridAction", &PyMfcWindow::onGridAction,
             "表格按键/排序回调 cb(action, row, col, text)")

        .def("onGridActionFor", &PyMfcWindow::onGridActionFor, py::arg("id"), py::arg("callback"),
             "按表格控件 id 登记回调：多张表并存时按 id 分流，不串台")
        .def("onControlEvent", &PyMfcWindow::onControlEvent, py::arg("id"), py::arg("event"), py::arg("callback"),
             "控件级原始事件：mousedown/mouseup/mousemove/keydown/keyup/gotfocus/lostfocus")
        .def("setColumnWidth", &PyMfcWindow::setColumnWidth, "设置某列宽度 px")
        .def("setColumnWidths", &PyMfcWindow::setColumnWidths, "批量设列宽（关重绘包一轮，结尾单次失效）")
        .def("getTableMetrics", &PyMfcWindow::getTableMetrics, py::arg("id"),
             "表格度量：(行高, 表头高)，用于把表格高度对齐整行")
        .def("onBlur", &PyMfcWindow::onBlur, py::arg("id"), py::arg("callback"),
             "EDIT 失焦回调（原地单元格编辑提交用）")
        .def("onDrag", &PyMfcWindow::onDrag, py::arg("id"), py::arg("callback"),
             "可拖分割条：回调 (phase, x)，phase=down/move/up")
        .def("cellRect", &PyMfcWindow::cellRect, py::arg("id"), py::arg("row"), py::arg("col"),
             "单元格矩形 (x, y, w, h)，ListView 客户区坐标")
        .def("clearTableRows", &PyMfcWindow::clearTableRows)
        .def("getTableRowCount", &PyMfcWindow::getTableRowCount)
        .def("getTableCell", &PyMfcWindow::getTableCell)
        .def("setTableCell", &PyMfcWindow::setTableCell)
        .def("getTableSelectedRow", &PyMfcWindow::getTableSelectedRow)
        .def("setTableSelectedRow", &PyMfcWindow::setTableSelectedRow)
        .def("sortTable", &PyMfcWindow::sortTable)
        .def("addTable", &PyMfcWindow::addTable, py::arg("id"), py::arg("x"), py::arg("y"), py::arg("w"), py::arg("h"),
             "创建新自绘表格 PyMfcTable")
        .def("tableClear", &PyMfcWindow::tableClear)
        .def("tableRowCount", &PyMfcWindow::tableRowCount)
        .def("setTableColumns", &PyMfcWindow::tableSetColumns, py::arg("id"), py::arg("widths"), py::arg("titles"),
             "新表：列宽 + 列标题")
        .def("setTableColTypes", &PyMfcWindow::tableSetColTypes, py::arg("id"), py::arg("types"),
             "新表：列类型 0 Text/1 Int/2 Double/3 Bool")
        .def("appendTableRow", &PyMfcWindow::appendTableRow, py::arg("id"), py::arg("cells"))
        .def("tableSetCell", &PyMfcWindow::tableSetCell)
        .def("tableGetCell", &PyMfcWindow::tableGetCell)
        .def("tableCellRect", &PyMfcWindow::tableCellRect, py::arg("id"), py::arg("row"), py::arg("col"))
        .def("tableSelectedRow", &PyMfcWindow::tableSelectedRow)
        .def("tableSetSelectedRow", &PyMfcWindow::tableSetSelectedRow)
        .def("tableEnsureVisible", &PyMfcWindow::tableEnsureVisible)
        .def("tableSetFilter", &PyMfcWindow::tableSetFilter)
        .def("tableClearFilters", &PyMfcWindow::tableClearFilters)
        .def("tableSort", &PyMfcWindow::tableSort, py::arg("id"), py::arg("col"), py::arg("desc"))
        .def("tableViewRows", &PyMfcWindow::tableViewRows)
        .def("addTreeItem", &PyMfcWindow::addTreeItem)
        .def("clearTreeItems", &PyMfcWindow::clearTreeItems)
        .def("getTreeSelectedRow", &PyMfcWindow::getTreeSelectedRow)
        .def("isTreeItemCollapsed", &PyMfcWindow::isTreeItemCollapsed)
        .def("setTreeItemCollapsed", &PyMfcWindow::setTreeItemCollapsed)
        .def("setTreeSelectedRow", &PyMfcWindow::setTreeSelectedRow)
        .def("addToolBarItem", &PyMfcWindow::addToolBarItem)
        .def("clearToolBarItems", &PyMfcWindow::clearToolBarItems)
        .def("getToolBarSelected", &PyMfcWindow::getToolBarSelected)
        .def("setToolBarSelected", &PyMfcWindow::setToolBarSelected)
        .def("animate", &PyMfcWindow::animate, py::arg("id"), py::arg("prop"), py::arg("to"),
             py::arg("duration") = 300, py::arg("easing") = "outCubic",
             py::arg("delay") = 0, py::arg("loop") = false)
        .def("animateFrom", &PyMfcWindow::animateFrom, py::arg("id"), py::arg("prop"),
             py::arg("from"), py::arg("to"), py::arg("duration") = 300,
             py::arg("easing") = "outCubic", py::arg("delay") = 0, py::arg("loop") = false)
        .def("animateColor", &PyMfcWindow::animateColor, py::arg("id"), py::arg("color"),
             py::arg("duration") = 300, py::arg("easing") = "outCubic", py::arg("delay") = 0)
        .def("stopAnimations", &PyMfcWindow::stopAnimations, py::arg("id") = 0)
        .def("setBehavior", &PyMfcWindow::setBehavior, py::arg("id"), py::arg("prop"),
             py::arg("duration"), py::arg("easing") = "outCubic")
        .def("showToolTip", &PyMfcWindow::showToolTip, py::arg("ownerId"), py::arg("text"),
             py::arg("durationMs") = 3000)
        .def("hideToolTip", &PyMfcWindow::hideToolTip)
        .def("showMenu", &PyMfcWindow::showMenu, py::arg("id"), py::arg("items"),
             py::arg("x"), py::arg("y"))
        .def("hideMenu", &PyMfcWindow::hideMenu)
        .def("getMenuSelected", &PyMfcWindow::getMenuSelected)
        .def("setMenuItems", &PyMfcWindow::setMenuItems)
        .def("addMenuBarItem", &PyMfcWindow::addMenuBarItem)
        .def("clearMenuBarItems", &PyMfcWindow::clearMenuBarItems)
        .def("getMenuBarSelected", &PyMfcWindow::getMenuBarSelected)
        .def("setMenuBarSelected", &PyMfcWindow::setMenuBarSelected)
        .def("showPopup", &PyMfcWindow::showPopup, py::arg("id"), py::arg("title"),
             py::arg("lines"), py::arg("x"), py::arg("y"), py::arg("w"), py::arg("h"))
        .def("hidePopup", &PyMfcWindow::hidePopup)
        .def("isPopupVisible", &PyMfcWindow::isPopupVisible)
        .def("setCanvasCommands", &PyMfcWindow::setCanvasCommands)
        .def("getCanvasCommands", &PyMfcWindow::getCanvasCommands)
        .def("setCanvasEditable", &PyMfcWindow::setCanvasEditable, py::arg("id"), py::arg("on") = true)
        .def("getCanvasEditable", &PyMfcWindow::getCanvasEditable)
        // 配电规划图画布：自绘窗口 + 缩放平移 + 卡片命中
        .def("addPowerCanvas", &PyMfcWindow::addPowerCanvas, py::arg("id"),
             py::arg("x"), py::arg("y"), py::arg("w"), py::arg("h"),
             "建一块配电画布（自绘窗口，滚轮缩放 / 拖动平移 / 单击选中卡片）")
        .def("loadPowerScene", &PyMfcWindow::loadPowerScene, py::arg("id"), py::arg("scene"),
             "喂已算好坐标的场景 JSON（nodes + layout），不重排、只按坐标画")
        .def("copyPowerScene", &PyMfcWindow::copyPowerScene, py::arg("to_id"), py::arg("from_id"),
             "把已解析好的场景直接复制给另一个画布（省一次 60 ms 解析）")
        .def("loadPowerPlan", &PyMfcWindow::loadPowerPlan, py::arg("id"), py::arg("json"),
             "喂布局结果（powerplan.toJson 的 JSON 文本），自动重排并按窗口大小 fit")
        .def("loadPowerSvg", &PyMfcWindow::loadPowerSvg, py::arg("id"), py::arg("svg"),
             "喂 SVG 文本，走同一条绘制路径")
        .def("setPlanView", &PyMfcWindow::setPlanView, py::arg("id"), py::arg("zoom"),
             py::arg("offset_x"), py::arg("offset_y"))
        .def("getPlanZoom", &PyMfcWindow::getPlanZoom)
        .def("getPlanSelection", &PyMfcWindow::getPlanSelection,
             "当前选中的卡片编号（未选中返回空串）")
        .def("saveControlPng", &PyMfcWindow::saveControlPng,
             "把控件按当前外观导出成 PNG（截图取证）")
        .def("setPlanOptions", &PyMfcWindow::setPlanOptions, py::arg("id"),
             py::arg("show_cable") = true, py::arg("mini") = false, py::arg("mini_of") = -1,
             "画布选项：连线标注电缆 / 小地图模式（mini=True 时画主画布 mini_of 的视口红框）")
        .def("getPlanView", &PyMfcWindow::getPlanView,
             "视口参数 (zoom, ox, oy)")
        .def("planProbe", &PyMfcWindow::planProbe, py::arg("id"),
             "画布鼠标诊断：(lastMsg, upX, upY, wasDrag, moved, lastHit)")
        .def("hitPlan", &PyMfcWindow::hitPlan, py::arg("id"), py::arg("x"), py::arg("y"),
             "命中测试（客户端坐标）：返回卡片编号，空 = 没命中")
        .def("selectPlan", &PyMfcWindow::selectPlan, py::arg("id"), py::arg("code"),
             "按编号选中并把卡片移到视口中央（搜索定位），返回是否找到")
        .def("cardRectPlan", &PyMfcWindow::cardRectPlan, py::arg("id"), py::arg("code"),
             "卡片在客户端坐标里的矩形 (x, y, w, h)；没有这张卡返回 None")
        .def("setDetailCard", &PyMfcWindow::setDetailCard, py::arg("id"), py::arg("rect"), py::arg("texts"),
             "画布层详情卡：本地矩形 + 纯展示文本行")
        .def("clearDetailCard", &PyMfcWindow::clearDetailCard, py::arg("id"),
             "清除画布层详情卡")
        // 通用基础属性：绘图层次（Level）与 Tab 序号（TabIndex）
        .def("setLevel", &PyMfcWindow::setLevel, py::arg("id"), py::arg("level"))
        .def("getLevel", &PyMfcWindow::getLevel)
        .def("bringToFront", &PyMfcWindow::bringToFront)
        .def("bringToFrontMany", &PyMfcWindow::bringToFrontMany, py::arg("ids"))
        .def("repaint", &PyMfcWindow::repaint, py::arg("id"))
        .def("raiseMany", &PyMfcWindow::raiseMany, py::arg("ids"), "批量置顶：一次重排 z 序（逐个 bringToFront 是 O(n^2)，312 个控件要 36 秒）")
        .def("sendToBack", &PyMfcWindow::sendToBack)
        .def("setTabIndex", &PyMfcWindow::setTabIndex, py::arg("id"), py::arg("index"))
        .def("getTabIndex", &PyMfcWindow::getTabIndex)
        .def("getCanvasItemCount", &PyMfcWindow::getCanvasItemCount)
        .def("getCanvasItemOp", &PyMfcWindow::getCanvasItemOp)
        .def("getCanvasItemBox", &PyMfcWindow::getCanvasItemBox)
        .def("getCanvasSelection", &PyMfcWindow::getCanvasSelection)
        .def("setCanvasSelection", &PyMfcWindow::setCanvasSelection)
        .def("clearCanvasSelection", &PyMfcWindow::clearCanvasSelection)
        .def("removeCanvasItems", &PyMfcWindow::removeCanvasItems)
        .def("removeCanvasSelection", &PyMfcWindow::removeCanvasSelection)
        .def("onCanvasSelect", &PyMfcWindow::onCanvasSelect)
        .def("setLayerOpacity", &PyMfcWindow::setLayerOpacity)
        .def("getLayerOpacity", &PyMfcWindow::getLayerOpacity)
        .def("addListItem", &PyMfcWindow::addListItem)
        .def("clearListItems", &PyMfcWindow::clearListItems)
        .def("getListItemCount", &PyMfcWindow::getListItemCount)
        .def("getListItem", &PyMfcWindow::getListItem)
        .def("getListSelected", &PyMfcWindow::getListSelected)
        .def("setListSelected", &PyMfcWindow::setListSelected)
        .def("getListScroll", &PyMfcWindow::getListScroll)
        .def("setListScroll", &PyMfcWindow::setListScroll)
        .def("addPage", &PyMfcWindow::addPage)
        .def("clearPages", &PyMfcWindow::clearPages)
        .def("getViewCount", &PyMfcWindow::getViewCount)
        .def("getViewIndex", &PyMfcWindow::getViewIndex)
        .def("setViewIndex", &PyMfcWindow::setViewIndex)
        .def("animatePage", &PyMfcWindow::animatePage)
        .def("setPageIndex", &PyMfcWindow::setPageIndex)
        .def("setFlickContent", &PyMfcWindow::setFlickContent)
        .def("clearFlickContent", &PyMfcWindow::clearFlickContent)
        .def("setFlickOffset", &PyMfcWindow::setFlickOffset)
        .def("getFlickOffsetX", &PyMfcWindow::getFlickOffsetX)
        .def("getFlickOffsetY", &PyMfcWindow::getFlickOffsetY)
        .def("getXmlValue", &PyMfcWindow::getXmlValue, py::arg("id"), py::arg("prop"),
             "按控件 id + 属性名读值（text/checked/index/value/visible/enabled/外观属性）")
        .def("setFocus", &PyMfcWindow::setFocusTo);

    py::class_<PyMfcPanel, PyMfcHandle<PyMfcPanel>>(m, "Pane")
        .def(py::init([](long long parentHwnd) -> PyMfcHandle<PyMfcPanel> {
            HWND parent = reinterpret_cast<HWND>(static_cast<std::intptr_t>(parentHwnd));
            if (parent == nullptr || !::IsWindow(parent))
                throw std::runtime_error("PyMfcCore.Panel: 父窗口无效");
            return PyMfcHandle<PyMfcPanel>(PyMfcStore::Get().NewPanel(parent));
        }), py::arg("parent_hwnd"))
        .def_property_readonly("hwnd", [](PyMfcPanel& p) {
            return static_cast<long long>(static_cast<std::intptr_t>(
                reinterpret_cast<std::intptr_t>(p.hwnd())));
        })
        .def("resize", &PyMfcPanel::resize)
        .def("addLabel", &PyMfcPanel::addLabel)
        .def("addButton", &PyMfcPanel::addButton)
        .def("addEdit", &PyMfcPanel::addEdit)
        .def("addListBox", &PyMfcPanel::addListBox)
        .def("addCheckBox", &PyMfcPanel::addCheckBox)
        .def("addRadio", &PyMfcPanel::addRadio)
        .def("addGroup", &PyMfcPanel::addGroup)
        .def("addCombo", &PyMfcPanel::addCombo)
        .def("addProgress", &PyMfcPanel::addProgress)
        .def("addSlider", &PyMfcPanel::addSlider)
        .def("addTab", &PyMfcPanel::addTab)
        .def("addTree", &PyMfcPanel::addTree)
        .def("addListCtrl", &PyMfcPanel::addListCtrl)
        .def("addChoice", &PyMfcPanel::addChoice)
        .def("addDatePicker", &PyMfcPanel::addDatePicker)
        .def("addPanel", &PyMfcPanel::addPanel)
        .def("addWhiteRect", &PyMfcPanel::addWhiteRect)
    .def("addShadowBox", &PyMfcPanel::addShadowBox)
        .def("setControlFont", &PyMfcPanel::setControlFont,
             py::arg("id"), py::arg("face"), py::arg("px"), py::arg("bold") = false)
        .def("addSplitter", &PyMfcPanel::addSplitter)
        .def("addStaticLine", &PyMfcPanel::addStaticLine)
        .def("addHyperlink", &PyMfcPanel::addHyperlink)
        .def("addRichText", &PyMfcPanel::addRichText)
        .def("addToggle", &PyMfcPanel::addToggle)
        .def("addBitmapButton", &PyMfcPanel::addBitmapButton)
        .def("addStaticBitmap", &PyMfcPanel::addStaticBitmap)
        .def("addGrid", &PyMfcPanel::addGrid)
        .def("addScrolledWindow", &PyMfcPanel::addScrolledWindow)
        .def("addCalendar", &PyMfcPanel::addCalendar)
        .def("addTabItem", &PyMfcPanel::addTabItem, py::arg("id"), py::arg("text"))
        .def("addTreeNode", &PyMfcPanel::addTreeNode, py::arg("id"), py::arg("text"),
             py::arg("parent_index") = -1)
        .def("addColumn", &PyMfcPanel::addColumn, py::arg("id"), py::arg("text"),
             py::arg("width") = 120)
        .def("addRow", &PyMfcPanel::addRow, py::arg("id"), py::arg("text"))
        .def("setUrl", &PyMfcPanel::setUrl, py::arg("id"), py::arg("url"))
        .def("getUrl", &PyMfcPanel::getUrl)
        .def("setImage", &PyMfcPanel::setImage, py::arg("id"), py::arg("path"))
        .def("setRichFormat", &PyMfcPanel::setRichFormat, py::arg("id"),
             py::arg("size_pt") = 9, py::arg("bold") = false, py::arg("italic") = false,
             py::arg("rgb") = 0xFFFFFF)
        .def("setScrollRange", &PyMfcPanel::setScrollRange, py::arg("id"),
             py::arg("min_value"), py::arg("max_value"))
        .def("getScrollRange", &PyMfcPanel::getScrollRange)
        .def("getScrollPos", &PyMfcPanel::getScrollPos, py::arg("id"))
        .def("getSelection", &PyMfcPanel::getSelection, py::arg("id"))
        .def("setSelection", &PyMfcPanel::setSelection, py::arg("id"), py::arg("from"),
             py::arg("to"))
        .def("onClick", &PyMfcPanel::onClick)
        .def("setWheelRegion", &PyMfcPanel::setWheelRegion, py::arg("x"), py::arg("y"),
             py::arg("w"), py::arg("h"), py::arg("step") = 3)
        .def("onWheel", &PyMfcPanel::onWheel, py::arg("cb"))
        .def("onChange", &PyMfcPanel::onChange)
        .def("onSelect", &PyMfcPanel::onSelect)
        .def("onDoubleClick", &PyMfcPanel::onDoubleClick)
        .def("onDropDown", &PyMfcPanel::onDropDown)
        .def("onValue", &PyMfcPanel::onValue)
        .def("onChecked", &PyMfcPanel::onChecked)
        .def("setText", &PyMfcPanel::setText)
        .def("getText", &PyMfcPanel::getText)
        .def("move", &PyMfcPanel::move)
        .def("moveMany", &PyMfcPanel::moveMany, py::arg("items"))
        .def("setXmlValue", [](PyMfcPanel& p, const std::wstring& name,
                                const std::wstring& prop, py::object value) {
            return p.setXmlValue(name, prop, xmlPyPropValueFor(p.Host(), name, prop, value));
        }, py::arg("name"), py::arg("prop"), py::arg("value"),
             "value 可为 str，也可为 list（一维 -> model/items，二维 -> 表格行）")
        .def("reflowXml", &PyMfcPanel::reflowXml)
        .def("addString", &PyMfcPanel::addString)
        .def("clearStrings", &PyMfcPanel::clearStrings)
        .def("countStrings", &PyMfcPanel::countStrings)
        .def("getString", &PyMfcPanel::getString)
        .def("deleteString", &PyMfcPanel::deleteString)
        .def("getCurSel", &PyMfcPanel::getCurSel)
        .def("setCurSel", &PyMfcPanel::setCurSel)
        .def("getCheck", &PyMfcPanel::getCheck)
        .def("setCheck", &PyMfcPanel::setCheck)
        .def("setRange", &PyMfcPanel::setRange)
        .def("setPos", &PyMfcPanel::setPos)
        .def("getPos", &PyMfcPanel::getPos)
        .def("setReadOnly", &PyMfcPanel::setReadOnly)
        .def("getReadOnly", &PyMfcPanel::getReadOnly)
        .def("setFocus", &PyMfcPanel::setFocusTo)
        .def("setVisible", &PyMfcPanel::setVisible)
        .def("getVisible", &PyMfcPanel::getVisible)
        .def("setEnabled", &PyMfcPanel::setEnabled)
        .def("getEnabled", &PyMfcPanel::getEnabled)
        // QML 对齐小控件
        .def("getSwitch", &PyMfcPanel::getSwitch)
        .def("setSwitch", &PyMfcPanel::setSwitch)
        .def("getSpin", &PyMfcPanel::getSpin)
        .def("setSpin", &PyMfcPanel::setSpin)
        .def("setSpinRange", &PyMfcPanel::setSpinRange)
        .def("setRangeValues", &PyMfcPanel::setRangeValues)
        .def("getRangeLow", &PyMfcPanel::getRangeLow)
        .def("getRangeHigh", &PyMfcPanel::getRangeHigh)
        .def("setPageCount", &PyMfcPanel::setPageCount)
        .def("getPageCount", &PyMfcPanel::getPageCount)
        .def("getPageIndex", &PyMfcPanel::getPageIndex)
        .def("getDial", &PyMfcPanel::getDial)
        .def("setDial", &PyMfcPanel::setDial)
        .def("getTumblerCount", &PyMfcPanel::getTumblerCount)
        .def("setTumblerCount", &PyMfcPanel::setTumblerCount)
        .def("getTumblerValue", &PyMfcPanel::getTumblerValue)
        .def("setTumblerValue", &PyMfcPanel::setTumblerValue)
        .def("setScrollBarRange", &PyMfcPanel::setScrollBarRange)
        .def("getScrollBarValue", &PyMfcPanel::getScrollBarValue)
        .def("setScrollBarValue", &PyMfcPanel::setScrollBarValue)
        .def("setTableColumns", &PyMfcPanel::setTableColumns)
        .def("addTableRow", &PyMfcPanel::addTableRow)
            .def("setTableRows", &PyMfcPanel::setTableRows,
                 "整表一次灌入：行间\\n、格间\\t")
        .def("onGridAction", &PyMfcPanel::onGridAction,
             "表格按键/排序回调 cb(action, row, col, text)")

        .def("onGridActionFor", &PyMfcPanel::onGridActionFor, py::arg("id"), py::arg("callback"),
             "按表格控件 id 登记回调：多张表并存时按 id 分流，不串台")
        .def("onControlEvent", &PyMfcPanel::onControlEvent, py::arg("id"), py::arg("event"), py::arg("callback"),
             "控件级原始事件：mousedown/mouseup/mousemove/keydown/keyup/gotfocus/lostfocus")
        .def("setColumnWidth", &PyMfcPanel::setColumnWidth, "设置某列宽度 px")
        .def("getTableMetrics", &PyMfcPanel::getTableMetrics, py::arg("id"),
             "表格度量：(行高, 表头高)，用于把表格高度对齐整行")
        .def("onBlur", &PyMfcPanel::onBlur, py::arg("id"), py::arg("callback"),
             "EDIT 失焦回调（原地单元格编辑提交用）")
        .def("onDrag", &PyMfcPanel::onDrag, py::arg("id"), py::arg("callback"),
             "可拖分割条：回调 (phase, x)，phase=down/move/up")
        .def("cellRect", &PyMfcPanel::cellRect, py::arg("id"), py::arg("row"), py::arg("col"),
             "单元格矩形 (x, y, w, h)，ListView 客户区坐标")
        .def("clearTableRows", &PyMfcPanel::clearTableRows)
        .def("getTableRowCount", &PyMfcPanel::getTableRowCount)
        .def("getTableCell", &PyMfcPanel::getTableCell)
        .def("setTableCell", &PyMfcPanel::setTableCell)
        .def("getTableSelectedRow", &PyMfcPanel::getTableSelectedRow)
        .def("setTableSelectedRow", &PyMfcPanel::setTableSelectedRow)
        .def("sortTable", &PyMfcPanel::sortTable)
        .def("addTable", &PyMfcPanel::addTable, py::arg("id"), py::arg("x"), py::arg("y"), py::arg("w"), py::arg("h"))
        .def("tableClear", &PyMfcPanel::tableClear)
        .def("tableRowCount", &PyMfcPanel::tableRowCount)
        .def("setTableColumns", &PyMfcPanel::tableSetColumns, py::arg("id"), py::arg("widths"), py::arg("titles"))
        .def("setTableColTypes", &PyMfcPanel::tableSetColTypes, py::arg("id"), py::arg("types"))
        .def("appendTableRow", &PyMfcPanel::appendTableRow, py::arg("id"), py::arg("cells"))
        .def("tableSetCell", &PyMfcPanel::tableSetCell)
        .def("tableGetCell", &PyMfcPanel::tableGetCell)
        .def("tableCellRect", &PyMfcPanel::tableCellRect, py::arg("id"), py::arg("row"), py::arg("col"))
        .def("tableSelectedRow", &PyMfcPanel::tableSelectedRow)
        .def("tableSetSelectedRow", &PyMfcPanel::tableSetSelectedRow)
        .def("tableEnsureVisible", &PyMfcPanel::tableEnsureVisible)
        .def("tableSetFilter", &PyMfcPanel::tableSetFilter)
        .def("tableClearFilters", &PyMfcPanel::tableClearFilters)
        .def("tableSort", &PyMfcPanel::tableSort, py::arg("id"), py::arg("col"), py::arg("desc"))
        .def("tableViewRows", &PyMfcPanel::tableViewRows)
        .def("addTreeItem", &PyMfcPanel::addTreeItem)
        .def("clearTreeItems", &PyMfcPanel::clearTreeItems)
        .def("getTreeSelectedRow", &PyMfcPanel::getTreeSelectedRow)
        .def("isTreeItemCollapsed", &PyMfcPanel::isTreeItemCollapsed)
        .def("setTreeItemCollapsed", &PyMfcPanel::setTreeItemCollapsed)
        .def("setTreeSelectedRow", &PyMfcPanel::setTreeSelectedRow)
        .def("addToolBarItem", &PyMfcPanel::addToolBarItem)
        .def("clearToolBarItems", &PyMfcPanel::clearToolBarItems)
        .def("getToolBarSelected", &PyMfcPanel::getToolBarSelected)
        .def("setToolBarSelected", &PyMfcPanel::setToolBarSelected)
        .def("animate", &PyMfcPanel::animate, py::arg("id"), py::arg("prop"), py::arg("to"),
             py::arg("duration") = 300, py::arg("easing") = "outCubic",
             py::arg("delay") = 0, py::arg("loop") = false)
        .def("animateFrom", &PyMfcPanel::animateFrom, py::arg("id"), py::arg("prop"),
             py::arg("from"), py::arg("to"), py::arg("duration") = 300,
             py::arg("easing") = "outCubic", py::arg("delay") = 0, py::arg("loop") = false)
        .def("animateColor", &PyMfcPanel::animateColor, py::arg("id"), py::arg("color"),
             py::arg("duration") = 300, py::arg("easing") = "outCubic", py::arg("delay") = 0)
        .def("stopAnimations", &PyMfcPanel::stopAnimations, py::arg("id") = 0)
        .def("setBehavior", &PyMfcPanel::setBehavior, py::arg("id"), py::arg("prop"),
             py::arg("duration"), py::arg("easing") = "outCubic")
        .def("showToolTip", &PyMfcPanel::showToolTip, py::arg("ownerId"), py::arg("text"),
             py::arg("durationMs") = 3000)
        .def("hideToolTip", &PyMfcPanel::hideToolTip)
        .def("showMenu", &PyMfcPanel::showMenu, py::arg("id"), py::arg("items"),
             py::arg("x"), py::arg("y"))
        .def("hideMenu", &PyMfcPanel::hideMenu)
        .def("getMenuSelected", &PyMfcPanel::getMenuSelected)
        .def("setMenuItems", &PyMfcPanel::setMenuItems)
        .def("addMenuBarItem", &PyMfcPanel::addMenuBarItem)
        .def("clearMenuBarItems", &PyMfcPanel::clearMenuBarItems)
        .def("getMenuBarSelected", &PyMfcPanel::getMenuBarSelected)
        .def("setMenuBarSelected", &PyMfcPanel::setMenuBarSelected)
        .def("showPopup", &PyMfcPanel::showPopup, py::arg("id"), py::arg("title"),
             py::arg("lines"), py::arg("x"), py::arg("y"), py::arg("w"), py::arg("h"))
        .def("hidePopup", &PyMfcPanel::hidePopup)
        .def("isPopupVisible", &PyMfcPanel::isPopupVisible)
        .def("setCanvasCommands", &PyMfcPanel::setCanvasCommands)
        .def("getCanvasCommands", &PyMfcPanel::getCanvasCommands)
        .def("setCanvasEditable", &PyMfcPanel::setCanvasEditable, py::arg("id"), py::arg("on") = true)
        .def("getCanvasEditable", &PyMfcPanel::getCanvasEditable)
        .def("addPowerCanvas", &PyMfcPanel::addPowerCanvas, py::arg("id"),
             py::arg("x"), py::arg("y"), py::arg("w"), py::arg("h"))
        .def("loadPowerPlan", &PyMfcPanel::loadPowerPlan, py::arg("id"), py::arg("json"))
        .def("loadPowerScene", &PyMfcPanel::loadPowerScene, py::arg("id"), py::arg("scene"))
        .def("loadPowerSvg", &PyMfcPanel::loadPowerSvg, py::arg("id"), py::arg("svg"))
        .def("setPlanView", &PyMfcPanel::setPlanView, py::arg("id"), py::arg("zoom"),
             py::arg("offset_x"), py::arg("offset_y"))
        .def("getPlanZoom", &PyMfcPanel::getPlanZoom)
        .def("getPlanSelection", &PyMfcPanel::getPlanSelection)
        .def("saveControlPng", &PyMfcPanel::saveControlPng,
             "把控件按当前外观导出成 PNG（截图取证）")
        .def("setPlanOptions", &PyMfcPanel::setPlanOptions, py::arg("id"),
             py::arg("show_cable") = true, py::arg("mini") = false, py::arg("mini_of") = -1)
        .def("setDetailCard", &PyMfcPanel::setDetailCard, py::arg("id"), py::arg("rect"), py::arg("texts"))
        .def("clearDetailCard", &PyMfcPanel::clearDetailCard, py::arg("id"))
        .def("getPlanView", &PyMfcPanel::getPlanView)
        .def("setLevel", &PyMfcPanel::setLevel, py::arg("id"), py::arg("level"))
        .def("getLevel", &PyMfcPanel::getLevel)
        .def("bringToFront", &PyMfcPanel::bringToFront)
        .def("raiseMany", &PyMfcPanel::raiseMany, py::arg("ids"))
        .def("sendToBack", &PyMfcPanel::sendToBack)
        .def("setTabIndex", &PyMfcPanel::setTabIndex, py::arg("id"), py::arg("index"))
        .def("getTabIndex", &PyMfcPanel::getTabIndex)
        .def("getCanvasItemCount", &PyMfcPanel::getCanvasItemCount)
        .def("getCanvasItemOp", &PyMfcPanel::getCanvasItemOp)
        .def("getCanvasItemBox", &PyMfcPanel::getCanvasItemBox)
        .def("getCanvasSelection", &PyMfcPanel::getCanvasSelection)
        .def("setCanvasSelection", &PyMfcPanel::setCanvasSelection)
        .def("clearCanvasSelection", &PyMfcPanel::clearCanvasSelection)
        .def("removeCanvasItems", &PyMfcPanel::removeCanvasItems)
        .def("removeCanvasSelection", &PyMfcPanel::removeCanvasSelection)
        .def("onCanvasSelect", &PyMfcPanel::onCanvasSelect)
        .def("setLayerOpacity", &PyMfcPanel::setLayerOpacity)
        .def("getLayerOpacity", &PyMfcPanel::getLayerOpacity)
        .def("addListItem", &PyMfcPanel::addListItem)
        .def("clearListItems", &PyMfcPanel::clearListItems)
        .def("getListItemCount", &PyMfcPanel::getListItemCount)
        .def("getListItem", &PyMfcPanel::getListItem)
        .def("getListSelected", &PyMfcPanel::getListSelected)
        .def("setListSelected", &PyMfcPanel::setListSelected)
        .def("getListScroll", &PyMfcPanel::getListScroll)
        .def("setListScroll", &PyMfcPanel::setListScroll)
        .def("addPage", &PyMfcPanel::addPage)
        .def("clearPages", &PyMfcPanel::clearPages)
        .def("getViewCount", &PyMfcPanel::getViewCount)
        .def("getViewIndex", &PyMfcPanel::getViewIndex)
        .def("setViewIndex", &PyMfcPanel::setViewIndex)
        .def("animatePage", &PyMfcPanel::animatePage)
        .def("getXmlValue", &PyMfcPanel::getXmlValue, py::arg("id"), py::arg("prop"),
             "按控件 id + 属性名读值")
        .def("setPageIndex", &PyMfcPanel::setPageIndex);

    m.def("easingValue", [](const std::string& ease, double tt) {
        return PyMfcHost::easeAt(ease, tt);
    }, py::arg("easing"), py::arg("t"),
        "缓动求值：linear / inQuad / outQuad / inOutQuad / inCubic / outCubic / inOutCubic / outBack / outElastic / spring / bezier(x1,y1,x2,y2)");

    // -----------------------------------------------------------------------
    // XML 布局计算：只读 XML，按给定客户区尺寸算出每个 x:Name 的矩形。
    // 用途：界面布局/尺寸写在 .xml 里，Python 拿结果 moveMany，不再手算坐标；
    //       控件仍由现有 addXxx 创建（画布/原生表格等 guiload 不支持的类型也能用）。
    // 返回 {name: (x, y, w, h)}，坐标相对客户区。
    // -----------------------------------------------------------------------
    // 找名为 name 的元素（深度优先）


    m.def("guiload", &Guiload,
          py::arg("path"), py::arg("target") = py::none(), py::arg("context") = py::none(),
          py::arg("names") = py::none(), py::arg("first_id") = 1000,
          py::arg("palette") = py::none(),
          "从 XML 生成界面（XML 风格标签与属性同样可用）：\n"
          "  根 <Window title=...> 返回 Window；根 <PaletteSet> 需 palette=PyArxCore.Ap.PaletteSet\n"
          "  target 传入已有 Window/Page 时按 XML 追加控件；context 解析 Click 等回调名\n"
          "  names 传 dict 可回传 x:Name -> 控件 id 的映射；first_id 是自动分配 ID 的起点");
    m.def("xmlLayout", &XmlLayoutRects, py::arg("path"), py::arg("width"), py::arg("height"),
          py::arg("overrides") = py::none(),
          "读 XML 按客户区尺寸算布局，返回 {x:Name: (x, y, w, h)}；overrides 可运行时改行列尺寸");
    m.def("prop_schema", &XmlPropertySchemaJson,
          "全部已注册控件的属性 schema（JSON 字符串）：设计器属性面板与 Python 内省共用。\n"
          "  结构：{\"version\":1,\"controls\":[{\"tag\":\"Button\",\"properties\":\n"
          "        [{\"name\":\"Text\",\"type\":\"String\",\"readOnly\":false,\"enum\":\"\"}]}]}");
    m.def("register_control", &RegisterPythonControl,
          py::arg("tag"), py::arg("factory"),
          "注册自定义控件：register_control('MyControl', factory)。\n"
          "  factory 是可调用，返回一个实现如下 duck-typing 接口的实例：\n"
          "  create(hwnd)->bool / destroy() / set_property(name,value)->bool /\n"
          "  get_property(name)->str / native_handle()->int / set_bounds(x,y,w,h) /\n"
          "  get_bounds()->(x,y,w,h)。注册后可在 XML 中用 <MyControl x:Name=.../> 声明。");

    m.def("create_embedded",
        [](long long parentHwnd, const std::wstring& title, int width, int height) -> PyMfcHandle<PyMfcWindow> {
            HWND parent = reinterpret_cast<HWND>(static_cast<std::intptr_t>(parentHwnd));
            auto* window = PyMfcStore::Get().NewWindow();
            if (!window->CreateEmbedded(parent, title, width, height))
                throw std::runtime_error("PyMfc embedded window create failed");
            return PyMfcHandle<PyMfcWindow>(window);
        },
        py::arg("parent_hwnd"), py::arg("title"), py::arg("width"), py::arg("height"),
        "创建嵌入指定父窗口（如 CAD 主窗口 HWND）的非侧栏窗体，返回 Window。");

    m.def("pump", [](int maxMs) -> bool {
        MSG msg = {};
        const DWORD start = ::GetTickCount();
        bool dispatched = false;
        for (;;)
        {
            while (::PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE))
            {
                if (msg.message == WM_QUIT)
                    return false;
                ::TranslateMessage(&msg);
                ::DispatchMessageW(&msg);
                dispatched = true;
            }
            const DWORD elapsed = ::GetTickCount() - start;
            if (maxMs > 0)
            {
                // 本次调用真派发过消息、队列已空：干完活就走，不留空等地板。
                if (dispatched)
                    return true;
                if (static_cast<int>(elapsed) >= maxMs)
                    return true;
                // 一次消息都没派发：有超时地等下一次输入，别 INFINITE，也别立即返回。
                const DWORD r = ::MsgWaitForMultipleObjects(
                    0, nullptr, FALSE, static_cast<DWORD>(maxMs - elapsed), QS_ALLINPUT);
                if (r == WAIT_TIMEOUT)
                    return true;
                continue;   // 等到消息 → 回到上面派发
            }
            ::MsgWaitForMultipleObjects(0, nullptr, FALSE, INFINITE, QS_ALLINPUT);
        }
    }, py::arg("max_ms") = 0,
       "跑窗口消息泵。本地 Python 直跑 Window/Panel 时必须调用，否则窗口不重绘、不响应；\n"
       "  max_ms>0：派发过消息且队列排空即返回；一次都没派发则等到预算用完；\n"
       "  max_ms=0：阻塞到 WM_QUIT；返回 False 表示收到 WM_QUIT");

    m.def("shutdown", []() {        // 固定顺序：先隐藏 palette，再隐藏窗口，最后 PostMessage 延迟销毁。
        // 只隐藏/异步关闭，不 delete 任何 MFC 原生对象，避免卸载栈内堆损坏。
        PyMfcStore& store = PyMfcStore::Get();
        debugLogf(L"[shutdown] windows=%zu", store.windows.size());
        for (auto* window : store.windows)
        {
            if (window == nullptr)
                continue;
            HWND hwnd = window->hwnd();
            debugLogf(L"[shutdown] window hwnd=0x%llX isWindow=%d visible=%d",
                      reinterpret_cast<unsigned long long>(hwnd),
                      hwnd != nullptr && ::IsWindow(hwnd) ? 1 : 0,
                      hwnd != nullptr && ::IsWindowVisible(hwnd) ? 1 : 0);
            window->hide();
            window->clearCallbacks();
            window->closeAsync();
            debugLogf(L"[shutdown] after hide visible=%d",
                      hwnd != nullptr && ::IsWindow(hwnd) && ::IsWindowVisible(hwnd) ? 1 : 0);
        }
    }, "关闭所有 PyMfcCore 窗口（侧栏由 PyArxCore.Ap 负责，ARX 卸载前调用）");
}

