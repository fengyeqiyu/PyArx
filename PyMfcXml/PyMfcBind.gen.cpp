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

// 第三参数声明本模块不使用 GIL：3.15t(free-threaded) 下 import 不再回开 GIL。

template <class C, class Cls>
static void RegisterSurfaceApi(Cls& cls)
{
    cls.def("add_button", &C::addButton);
    cls.def("add_label", &C::addLabel);
    cls.def("add_edit", &C::addEdit);
    cls.def("add_list_box", &C::addListBox);
    cls.def("add_check_box", &C::addCheckBox);
    cls.def("add_radio", &C::addRadio);
    cls.def("add_group", &C::addGroup);
    cls.def("add_combo", &C::addCombo);
    cls.def("add_progress", &C::addProgress);
    cls.def("add_slider", &C::addSlider);
    cls.def("add_tab", &C::addTab);
    cls.def("add_tree", &C::addTree);
    cls.def("add_list_ctrl", &C::addListCtrl);
    cls.def("add_choice", &C::addChoice);
    cls.def("add_date_picker", &C::addDatePicker);
    cls.def("add_panel", &C::addPanel);
    cls.def("add_white_rect", &C::addWhiteRect);
    cls.def("add_shadow_box", &C::addShadowBox);
    cls.def("set_control_font", &C::setControlFont,
             py::arg("id"), py::arg("face"), py::arg("px"), py::arg("bold") = false);
    cls.def("add_splitter", &C::addSplitter);
    cls.def("add_static_line", &C::addStaticLine);
    cls.def("add_hyperlink", &C::addHyperlink);
    cls.def("add_rich_text", &C::addRichText);
    cls.def("add_toggle", &C::addToggle);
    cls.def("add_bitmap_button", &C::addBitmapButton);
    cls.def("add_static_bitmap", &C::addStaticBitmap);
    cls.def("add_grid", &C::addGrid);
    cls.def("add_scrolled_window", &C::addScrolledWindow);
    cls.def("add_calendar", &C::addCalendar);
    cls.def("add_tab_item", &C::addTabItem, py::arg("id"), py::arg("text"));
    cls.def("add_tree_node", &C::addTreeNode, py::arg("id"), py::arg("text"),
             py::arg("parent_index") = -1);
    cls.def("add_column", &C::addColumn, py::arg("id"), py::arg("text"),
             py::arg("width") = 120);
    cls.def("add_row", &C::addRow, py::arg("id"), py::arg("text"));
    cls.def("set_url", &C::setUrl, py::arg("id"), py::arg("url"));
    cls.def("get_url", &C::getUrl);
    cls.def("set_image", &C::setImage, py::arg("id"), py::arg("path"),
             "给 StaticBitmap / BitmapButton 装图片（png/jpg 均可）");
    cls.def("set_rich_format", &C::setRichFormat, py::arg("id"),
             py::arg("size_pt") = 9, py::arg("bold") = false, py::arg("italic") = false,
             py::arg("rgb") = 0xFFFFFF, "设置 RichText 的字号/粗体/斜体/颜色");
    cls.def("set_scroll_range", &C::setScrollRange, py::arg("id"),
             py::arg("min_value"), py::arg("max_value"), "设置 ScrolledWindow 的滚动范围");
    cls.def("get_scroll_range", &C::getScrollRange);
    cls.def("get_scroll_pos", &C::getScrollPos, py::arg("id"),
             "读 ScrolledWindow 当前的滚动位置");
    cls.def("get_selection", &C::getSelection, py::arg("id"),
             "读文本控件当前的选区，返回 (起点, 终点)");
    cls.def("set_selection", &C::setSelection, py::arg("id"), py::arg("from"),
             py::arg("to"), "设置文本控件选区");
    cls.def("on_click", &C::onClick);
    cls.def("set_wheel_region", &C::setWheelRegion, py::arg("x"), py::arg("y"),
             py::arg("w"), py::arg("h"), py::arg("step") = 3,
             "注册收滚轮的区域（客户区坐标）：鼠标在里面滚动就回调 onWheel(±step)");
    cls.def("on_wheel", &C::onWheel, py::arg("cb"),
             "cb(delta)，delta 为“往下为正”的行增量；MFCXml 不替子控件滚动，行位置自己算");
    cls.def("on_change", &C::onChange);
    cls.def("on_select", &C::onSelect);
    cls.def("on_double_click", &C::onDoubleClick);
    cls.def("on_drop_down", &C::onDropDown);
    cls.def("on_value", &C::onValue);
    cls.def("on_checked", &C::onChecked);
    cls.def("set_text", &C::setText);
    cls.def("get_text", &C::getText);
    cls.def("set_visible", &C::setVisible);
    cls.def("get_visible", &C::getVisible);
    cls.def("set_enabled", &C::setEnabled);
    cls.def("get_enabled", &C::getEnabled);
    cls.def("reflow_xml", &C::reflowXml);
    cls.def("add_string", &C::addString);
    cls.def("clear_strings", &C::clearStrings);
    cls.def("count_strings", &C::countStrings);
    cls.def("get_string", &C::getString);
    cls.def("delete_string", &C::deleteString);
    cls.def("get_cur_sel", &C::getCurSel);
    cls.def("set_cur_sel", &C::setCurSel);
    cls.def("get_check", &C::getCheck);
    cls.def("set_check", &C::setCheck);
    cls.def("set_range", &C::setRange);
    cls.def("set_pos", &C::setPos);
    cls.def("get_pos", &C::getPos);
    cls.def("set_read_only", &C::setReadOnly);
    cls.def("get_read_only", &C::getReadOnly);
    cls.def("get_switch", &C::getSwitch);
    cls.def("set_switch", &C::setSwitch);
    cls.def("get_spin", &C::getSpin);
    cls.def("set_spin", &C::setSpin);
    cls.def("set_spin_range", &C::setSpinRange);
    cls.def("set_range_values", &C::setRangeValues);
    cls.def("get_range_low", &C::getRangeLow);
    cls.def("get_range_high", &C::getRangeHigh);
    cls.def("set_page_count", &C::setPageCount);
    cls.def("get_page_count", &C::getPageCount);
    cls.def("get_page_index", &C::getPageIndex);
    cls.def("get_dial", &C::getDial);
    cls.def("set_dial", &C::setDial);
    cls.def("get_tumbler_count", &C::getTumblerCount);
    cls.def("set_tumbler_count", &C::setTumblerCount);
    cls.def("get_tumbler_value", &C::getTumblerValue);
    cls.def("set_tumbler_value", &C::setTumblerValue);
    cls.def("set_scroll_bar_range", &C::setScrollBarRange);
    cls.def("get_scroll_bar_value", &C::getScrollBarValue);
    cls.def("set_scroll_bar_value", &C::setScrollBarValue);
    cls.def("set_table_columns", &C::setTableColumns);
    cls.def("add_table_row", &C::addTableRow);
    cls.def("set_table_rows", &C::setTableRows,
                 "整表一次灌入：行间\\n、格间\\t");
    cls.def("on_grid_action", &C::onGridAction,
             "表格按键/排序回调 cb(action, row, col, text)");
    cls.def("on_grid_action_for", &C::onGridActionFor, py::arg("id"), py::arg("callback"),
             "按表格控件 id 登记回调：多张表并存时按 id 分流，不串台");
    cls.def("on_control_event", &C::onControlEvent, py::arg("id"), py::arg("event"), py::arg("callback"),
             "控件级原始事件：mousedown/mouseup/mousemove/keydown/keyup/gotfocus/lostfocus");
    cls.def("set_column_width", &C::setColumnWidth, "设置某列宽度 px");
    cls.def("get_table_metrics", &C::getTableMetrics, py::arg("id"),
             "表格度量：(行高, 表头高)，用于把表格高度对齐整行");
    cls.def("on_blur", &C::onBlur, py::arg("id"), py::arg("callback"),
             "EDIT 失焦回调（原地单元格编辑提交用）");
    cls.def("on_drag", &C::onDrag, py::arg("id"), py::arg("callback"),
             "可拖分割条：回调 (phase, x)，phase=down/move/up");
    cls.def("cell_rect", &C::cellRect, py::arg("id"), py::arg("row"), py::arg("col"),
             "单元格矩形 (x, y, w, h)，ListView 客户区坐标");
    cls.def("clear_table_rows", &C::clearTableRows);
    cls.def("get_table_row_count", &C::getTableRowCount);
    cls.def("get_table_cell", &C::getTableCell);
    cls.def("set_table_cell", &C::setTableCell);
    cls.def("get_table_selected_row", &C::getTableSelectedRow);
    cls.def("set_table_selected_row", &C::setTableSelectedRow);
    cls.def("sort_table", &C::sortTable);
    cls.def("add_table", &C::addTable, py::arg("id"), py::arg("x"), py::arg("y"), py::arg("w"), py::arg("h"),
             "创建新自绘表格 PyMfcTable");
    cls.def("table_clear", &C::tableClear);
    cls.def("table_row_count", &C::tableRowCount);
    cls.def("set_table_columns", &C::tableSetColumns, py::arg("id"), py::arg("widths"), py::arg("titles"),
             "新表：列宽 + 列标题");
    cls.def("set_table_col_types", &C::tableSetColTypes, py::arg("id"), py::arg("types"),
             "新表：列类型 0 Text/1 Int/2 Double/3 Bool");
    cls.def("append_table_row", &C::appendTableRow, py::arg("id"), py::arg("cells"));
    cls.def("table_set_cell", &C::tableSetCell);
    cls.def("table_get_cell", &C::tableGetCell);
    cls.def("table_cell_rect", &C::tableCellRect, py::arg("id"), py::arg("row"), py::arg("col"));
    cls.def("table_selected_row", &C::tableSelectedRow);
    cls.def("table_set_selected_row", &C::tableSetSelectedRow);
    cls.def("table_ensure_visible", &C::tableEnsureVisible);
    cls.def("table_set_filter", &C::tableSetFilter);
    cls.def("table_clear_filters", &C::tableClearFilters);
    cls.def("table_sort", &C::tableSort, py::arg("id"), py::arg("col"), py::arg("desc"));
    cls.def("table_view_rows", &C::tableViewRows);
    cls.def("add_tree_item", &C::addTreeItem);
    cls.def("clear_tree_items", &C::clearTreeItems);
    cls.def("get_tree_selected_row", &C::getTreeSelectedRow);
    cls.def("is_tree_item_collapsed", &C::isTreeItemCollapsed);
    cls.def("set_tree_item_collapsed", &C::setTreeItemCollapsed);
    cls.def("set_tree_selected_row", &C::setTreeSelectedRow);
    cls.def("add_tool_bar_item", &C::addToolBarItem);
    cls.def("clear_tool_bar_items", &C::clearToolBarItems);
    cls.def("get_tool_bar_selected", &C::getToolBarSelected);
    cls.def("set_tool_bar_selected", &C::setToolBarSelected);
    cls.def("animate", &C::animate, py::arg("id"), py::arg("prop"), py::arg("to"),
             py::arg("duration") = 300, py::arg("easing") = "outCubic",
             py::arg("delay") = 0, py::arg("loop") = false);
    cls.def("animate_from", &C::animateFrom, py::arg("id"), py::arg("prop"),
             py::arg("from"), py::arg("to"), py::arg("duration") = 300,
             py::arg("easing") = "outCubic", py::arg("delay") = 0, py::arg("loop") = false);
    cls.def("animate_color", &C::animateColor, py::arg("id"), py::arg("color"),
             py::arg("duration") = 300, py::arg("easing") = "outCubic", py::arg("delay") = 0);
    cls.def("stop_animations", &C::stopAnimations, py::arg("id") = 0);
    cls.def("set_behavior", &C::setBehavior, py::arg("id"), py::arg("prop"),
             py::arg("duration"), py::arg("easing") = "outCubic");
    cls.def("show_tool_tip", &C::showToolTip, py::arg("ownerId"), py::arg("text"),
             py::arg("durationMs") = 3000);
    cls.def("hide_tool_tip", &C::hideToolTip);
    cls.def("show_menu", &C::showMenu, py::arg("id"), py::arg("items"),
             py::arg("x"), py::arg("y"));
    cls.def("hide_menu", &C::hideMenu);
    cls.def("get_menu_selected", &C::getMenuSelected);
    cls.def("set_menu_items", &C::setMenuItems);
    cls.def("add_menu_bar_item", &C::addMenuBarItem);
    cls.def("clear_menu_bar_items", &C::clearMenuBarItems);
    cls.def("get_menu_bar_selected", &C::getMenuBarSelected);
    cls.def("set_menu_bar_selected", &C::setMenuBarSelected);
    cls.def("show_popup", &C::showPopup, py::arg("id"), py::arg("title"),
             py::arg("lines"), py::arg("x"), py::arg("y"), py::arg("w"), py::arg("h"));
    cls.def("hide_popup", &C::hidePopup);
    cls.def("is_popup_visible", &C::isPopupVisible);
    cls.def("set_canvas_commands", &C::setCanvasCommands);
    cls.def("get_canvas_commands", &C::getCanvasCommands);
    cls.def("set_canvas_editable", &C::setCanvasEditable, py::arg("id"), py::arg("on") = true);
    cls.def("get_canvas_editable", &C::getCanvasEditable);
    cls.def("add_dwg_canvas", &C::addDwgCanvas, py::arg("id"),
             py::arg("x"), py::arg("y"), py::arg("w"), py::arg("h"),
             "建一块绘图画布（DWG 为主，兼容已算坐标的布局输入）（自绘窗口，滚轮缩放 / 拖动平移 / 单击选中卡片）");
    cls.def("load_dwg_scene", &C::loadDwgScene, py::arg("id"), py::arg("scene"),
             "喂已算好坐标的场景 JSON（nodes + layout），不重排、只按坐标画");
    cls.def("load_dwg_layout", &C::loadDwgLayout, py::arg("id"), py::arg("json"),
             "喂布局结果（powerplan.toJson 的 JSON 文本），自动重排并按窗口大小 fit");
    cls.def("load_dwg_svg", &C::loadDwgSvg, py::arg("id"), py::arg("svg"),
             "喂 SVG 文本，走同一条绘制路径");
    cls.def("load_dwg_xml", &C::loadDwgXml, py::arg("id"), py::arg("scene_xml"),
             "喂 <XScene> 声明式场景（XLine/XCircle/XArc/XRect/XPolyline/XText/XGroup），"
             "与 loadDwgSvg 同一条绘制路径，但不做 SVG 序列化 + 二次解析");
    cls.def("load_dwg_overlay", &C::loadDwgOverlay, py::arg("id"), py::arg("svg"),
             "叠加层 SVG（选中框/橡皮筋/光标）：底图不重解析，只换这一层");
    cls.def("load_dwg_overlay_xml", &C::loadDwgOverlayXml, py::arg("id"),
             py::arg("scene_xml"),
             "叠加层的 XML 场景版本（同上语义）");
    cls.def("set_dwg_view", &C::setDwgView, py::arg("id"), py::arg("zoom"),
             py::arg("offset_x"), py::arg("offset_y"));
    cls.def("get_dwg_zoom", &C::getDwgZoom);
    cls.def("add_view3d", &C::addView3D, py::arg("id"),
             py::arg("x"), py::arg("y"), py::arg("w"), py::arg("h"),
             "建一块三维画布（box/line/tri 图元，左键旋转、滚轮缩放、中键平移）");
    cls.def("view3d_load", &C::view3DLoad, py::arg("id"), py::arg("geom"),
             "喂几何文本：box x0 y0 z0 x1 y1 z1 rrggbb; line ...; 用 ; 或换行分隔");
    cls.def("view3d_append_box", &C::view3DAppendBox, py::arg("id"),
             py::arg("x0"), py::arg("y0"), py::arg("z0"), py::arg("x1"), py::arg("y1"),
             py::arg("z1"), py::arg("color") = 0xA8C6E2, "追加一个轴对齐盒子");
    cls.def("view3d_append_line", &C::view3DAppendLine, py::arg("id"),
             py::arg("x0"), py::arg("y0"), py::arg("z0"), py::arg("x1"), py::arg("y1"),
             py::arg("z1"), py::arg("color") = 0x30507A, py::arg("width") = 1.0f);
    cls.def("view3d_set_camera", &C::view3DSetCamera, py::arg("id"),
             py::arg("yaw") = 35.0f, py::arg("pitch") = 22.0f, py::arg("zoom") = 1.0f,
             py::arg("pan_x") = 0, py::arg("pan_y") = 0);
    cls.def("view3d_get_camera", &C::view3DGetCamera,
             "返回 (yaw, pitch, zoom, pan_x, pan_y)");
    cls.def("view3d_fit", &C::view3DFitView, py::arg("id"), "复位相机并重新适配");
    cls.def("view3d_set_style", &C::view3DSetStyle, py::arg("id"),
             py::arg("bg") = 0xF7FAFD, py::arg("axes") = true, py::arg("grid") = false);
    cls.def("get_dwg_selection", &C::getDwgSelection,
             "当前选中的卡片编号（未选中返回空串）");
    cls.def("save_control_png", &C::saveControlPng,
             "把控件按当前外观导出成 PNG（截图取证）");
    cls.def("set_dwg_options", &C::setDwgOptions, py::arg("id"),
             py::arg("show_cable") = true, py::arg("mini") = false, py::arg("mini_of") = -1,
             "画布选项：连线标注电缆 / 小地图模式（mini=True 时画主画布 mini_of 的视口红框）");
    cls.def("get_dwg_view", &C::getDwgView,
             "视口参数 (zoom, ox, oy)");
    cls.def("set_detail_card", &C::setDetailCard, py::arg("id"), py::arg("rect"), py::arg("texts"),
             "画布层详情卡：本地矩形 + 纯展示文本行");
    cls.def("clear_detail_card", &C::clearDetailCard, py::arg("id"),
             "清除画布层详情卡");
    cls.def("set_level", &C::setLevel, py::arg("id"), py::arg("level"));
    cls.def("get_level", &C::getLevel);
    cls.def("set_cursor", &C::setCursor, py::arg("id"), py::arg("keyword"),
             "CSS cursor：pointer/text/move/not-allowed/col-resize…，空串或 auto 表示不接管");
    cls.def("get_cursor", &C::getCursor);
    cls.def("get_pointer_events", &C::getPointerEvents);
    cls.def("set_pointer_events", &C::setPointerEvents);
    cls.def("is_mouse_transparent", &C::isMouseTransparent);
    cls.def("bring_to_front", &C::bringToFront);
    cls.def("raise_many", &C::raiseMany, py::arg("ids"), "批量置顶：一次重排 z 序（逐个 bringToFront 是 O(n^2)，312 个控件要 36 秒）");
    cls.def("send_to_back", &C::sendToBack);
    cls.def("set_tab_index", &C::setTabIndex, py::arg("id"), py::arg("index"));
    cls.def("get_tab_index", &C::getTabIndex);
    cls.def("get_canvas_item_count", &C::getCanvasItemCount);
    cls.def("get_canvas_item_op", &C::getCanvasItemOp);
    cls.def("get_canvas_item_box", &C::getCanvasItemBox);
    cls.def("get_canvas_selection", &C::getCanvasSelection);
    cls.def("set_canvas_selection", &C::setCanvasSelection);
    cls.def("clear_canvas_selection", &C::clearCanvasSelection);
    cls.def("remove_canvas_items", &C::removeCanvasItems);
    cls.def("remove_canvas_selection", &C::removeCanvasSelection);
    cls.def("on_canvas_select", &C::onCanvasSelect);
    cls.def("set_layer_opacity", &C::setLayerOpacity);
    cls.def("get_layer_opacity", &C::getLayerOpacity);
    cls.def("add_list_item", &C::addListItem);
    cls.def("clear_list_items", &C::clearListItems);
    cls.def("get_list_item_count", &C::getListItemCount);
    cls.def("get_list_item", &C::getListItem);
    cls.def("get_list_selected", &C::getListSelected);
    cls.def("set_list_selected", &C::setListSelected);
    cls.def("get_list_scroll", &C::getListScroll);
    cls.def("set_list_scroll", &C::setListScroll);
    cls.def("add_page", &C::addPage);
    cls.def("clear_pages", &C::clearPages);
    cls.def("get_view_count", &C::getViewCount);
    cls.def("get_view_index", &C::getViewIndex);
    cls.def("set_view_index", &C::setViewIndex);
    cls.def("animate_page", &C::animatePage);
    cls.def("set_page_index", &C::setPageIndex);
    cls.def("get_xml_value", &C::getXmlValue, py::arg("id"), py::arg("prop"),
             "按控件 id + 属性名读值（text/checked/index/value/visible/enabled/外观属性）");
    cls.def("set_focus", &C::setFocusTo);
}


PYBIND11_MODULE(PyMfcXml, m, py::mod_gil_not_used())
{
    m.doc() = "PyMfcCore — MFC 原生控件最小 Python 接口";

    // 建窗/建控件都依赖 MFC 模块态，必须在任何 GUI 调用前完成自举
    ensureMfcApp();

    // 注册全部标准 MFC 控件到统一 XML 运行时注册表（自定义控件由 register_control 追加）
    RegisterPyMfcControls();

    // PyMfcCore.xml：PyMfcXml 加载器（解析/遍历/布局/序列化），不建窗口
    RegisterXmlBindings(m);
    auto clsWin = py::class_<PyMfcWindow, PyMfcHandle<PyMfcWindow>>(m, "Window");
    RegisterSurfaceApi<PyMfcWindow>(clsWin);
    clsWin.def(py::init([](const std::wstring& title, int width, int height) -> PyMfcHandle<PyMfcWindow> {
            auto* window = PyMfcStore::Get().NewWindow();
            if (!window->create(title, width, height))
                throw std::runtime_error("PyMfc Window create failed");
            return PyMfcHandle<PyMfcWindow>(window);
        }), py::arg("title"), py::arg("width"), py::arg("height"));
    clsWin.def_property_readonly("hwnd", [](PyMfcWindow& w) {
            return static_cast<long long>(reinterpret_cast<std::intptr_t>(w.hwnd()));
        }, "窗口句柄（本地 Python 下可交给 pump 之外的 Win32 API 使用）");
    clsWin.def("set_theme", &PyMfcWindow::setTheme, py::arg("dark"));
    clsWin.def("on_resize", &PyMfcWindow::setResizeCallback);
    clsWin.def("client_size", &PyMfcWindow::ClientSize);
    clsWin.def("set_background", &PyMfcWindow::setBackground, py::arg("r"), py::arg("g"), py::arg("b"));
    clsWin.def("get_size", &PyMfcWindow::getSize, py::arg("id"));
    clsWin.def("get_rect", &PyMfcWindow::getRect, py::arg("id"));
    clsWin.def("show", &PyMfcWindow::show);
    clsWin.def("close", &PyMfcWindow::close);
    clsWin.def("save_png", &PyMfcWindow::savePng, py::arg("path"),
             "把整个窗口（含 D2D 自绘子窗）存成 PNG：验收取证用，抓的是真屏幕");
    clsWin.def("on_canvas_event", &PyMfcWindow::onCanvasEvent, py::arg("id"), py::arg("cb"),
             "画布原始鼠标事件：cb(phase, x, y, flags) -> bool；"
             "phase 为 down/move/up/dblclick/wheel，x,y 是画布客户区坐标，"
             "返回 True 表示已消费（正在定点画图），宿主就不再当平移处理");
    clsWin.def("move", &PyMfcWindow::moveControl);
    clsWin.def("move_many", &PyMfcWindow::moveMany, py::arg("items"),
             "items=[(id,x,y,w,h,visible)]，一次 DeferWindowPos 原子提交，滚动时不再一次一重绘");
    clsWin.def("set_xml_value", [](PyMfcWindow& w, const std::wstring& name,
                                const std::wstring& prop, py::object value) {
            return w.setXmlValue(name, prop, xmlPyPropValueFor(w.Host(), name, prop, value));
        }, py::arg("name"), py::arg("prop"), py::arg("value"),
             "value 可为 str，也可为 list（一维 -> model/items，二维 -> 表格行）");
    clsWin.def("set_column_widths", &PyMfcWindow::setColumnWidths, "批量设列宽（关重绘包一轮，结尾单次失效）");
    clsWin.def("perf_stats", &PyMfcWindow::perfStats, py::arg("reset") = true, "性能计数：create/ap/paint 的累计耗时与次数（PYARX_PERF=1 才有值）");
    clsWin.def("copy_dwg_scene", &PyMfcWindow::copyDwgScene, py::arg("to_id"), py::arg("from_id"),
             "把已解析好的场景直接复制给另一个画布（省一次 60 ms 解析）");
    clsWin.def("get_file_tab_info", &PyMfcWindow::getFileTabInfo, py::arg("id"),
             "FileTab 诊断：回读 active/tabs/close/add 几何 JSON");
    clsWin.def("dwg_probe", &PyMfcWindow::dwgProbe, py::arg("id"),
             "画布鼠标诊断：(lastMsg, upX, upY, wasDrag, moved, lastHit)");
    clsWin.def("hit_dwg", &PyMfcWindow::hitDwg, py::arg("id"), py::arg("x"), py::arg("y"),
             "命中测试（客户端坐标）：返回卡片编号，空 = 没命中");
    clsWin.def("select_dwg", &PyMfcWindow::selectDwg, py::arg("id"), py::arg("code"),
             "按编号选中并把卡片移到视口中央（搜索定位），返回是否找到");
    clsWin.def("card_rect_dwg", &PyMfcWindow::cardRectDwg, py::arg("id"), py::arg("code"),
             "卡片在客户端坐标里的矩形 (x, y, w, h)；没有这张卡返回 None");
    clsWin.def("bring_to_front_many", &PyMfcWindow::bringToFrontMany, py::arg("ids"));
    clsWin.def("repaint", &PyMfcWindow::repaint, py::arg("id"));
    clsWin.def("set_flick_content", &PyMfcWindow::setFlickContent);
    clsWin.def("clear_flick_content", &PyMfcWindow::clearFlickContent);
    clsWin.def("set_flick_offset", &PyMfcWindow::setFlickOffset);
    clsWin.def("get_flick_offset_x", &PyMfcWindow::getFlickOffsetX);
    clsWin.def("get_flick_offset_y", &PyMfcWindow::getFlickOffsetY);
    auto clsPane = py::class_<PyMfcPanel, PyMfcHandle<PyMfcPanel>>(m, "Pane");
    RegisterSurfaceApi<PyMfcPanel>(clsPane);
    clsPane.def(py::init([](long long parentHwnd) -> PyMfcHandle<PyMfcPanel> {
            HWND parent = reinterpret_cast<HWND>(static_cast<std::intptr_t>(parentHwnd));
            if (parent == nullptr || !::IsWindow(parent))
                throw std::runtime_error("PyMfcCore.Panel: 父窗口无效");
            return PyMfcHandle<PyMfcPanel>(PyMfcStore::Get().NewPanel(parent));
        }), py::arg("parent_hwnd"));
    clsPane.def_property_readonly("hwnd", [](PyMfcPanel& p) {
            return static_cast<long long>(static_cast<std::intptr_t>(
                reinterpret_cast<std::intptr_t>(p.hwnd())));
        });
    clsPane.def("resize", &PyMfcPanel::resize);
    clsPane.def("move", &PyMfcPanel::move);
    clsPane.def("move_many", &PyMfcPanel::moveMany, py::arg("items"));
    clsPane.def("set_xml_value", [](PyMfcPanel& p, const std::wstring& name,
                                const std::wstring& prop, py::object value) {
            return p.setXmlValue(name, prop, xmlPyPropValueFor(p.Host(), name, prop, value));
        }, py::arg("name"), py::arg("prop"), py::arg("value"),
             "value 可为 str，也可为 list（一维 -> model/items，二维 -> 表格行）");
    m.def("easing_value", [](const std::string& ease, double tt) {
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
    m.def("guiload_text", &GuiloadText,
          py::arg("xml"), py::arg("target") = py::none(), py::arg("context") = py::none(),
          py::arg("names") = py::none(), py::arg("first_id") = 1000,
          py::arg("palette") = py::none(),
          "从 XML 字符串生成界面（与 guiload 同一套解析/布局/注册表，不落盘）：\n"
          "  M.guiload_text('<XWindow Width=300 Height=120><XButton x:Name=\"Ok\" Text=\"确定\"/></XWindow>')\n"
          "  字符串入口没有文件基准目录，ResourceDictionary 的 Source 相对路径不可用"
          "（需要跨文件样式时用 guiload(path)）");
    m.def("xml_layout", &XmlLayoutRects, py::arg("path"), py::arg("width"), py::arg("height"),
          py::arg("overrides") = py::none(),
          "读 XML 按客户区尺寸算布局，返回 {x:Name: (x, y, w, h)}；overrides 可运行时改行列尺寸");
    m.def("prop_schema", &XmlPropertySchemaJson,
          "全部已注册控件的属性 schema（JSON 字符串）：设计器属性面板与 Python 内省共用。\n"
          "  结构：{\"version\":1,\"controls\":[{\"tag\":\"Button\",\"properties\":\n"
          "        [{\"name\":\"Text\",\"type\":\"String\",\"readOnly\":false,\"enum\":\"\"}]}]}");

    // ---- 事件契约 + 加载期诊断：把 XML → 界面 的写错从"静默"变成"可查" ----
    auto w2u = [](const std::wstring& w) -> std::string {
        if (w.empty())
            return std::string();
        int n = ::WideCharToMultiByte(CP_UTF8, 0, w.data(), (int)w.size(),
                                      nullptr, 0, nullptr, nullptr);
        std::string out((size_t)n, '\0');
        ::WideCharToMultiByte(CP_UTF8, 0, w.data(), (int)w.size(),
                              out.data(), n, nullptr, nullptr);
        return out;
    };
    m.def("event_schema", [w2u]() -> std::string {
        return w2u(xmlEventSchemaJson());
    }, "事件契约表（JSON）：XML 属性名 / 内部键 / 回调负载 / 宿主当前是否已派发");

    m.def("control_events", [w2u](const std::string& type) {
        std::vector<std::string> out;
        for (XmlEventKind k : xmlDefaultEvents(std::wstring(type.begin(), type.end())))
            out.push_back(w2u(xmlEventKey(k)));
        return out;
    }, py::arg("type"),
        "某控件类型（type()，如 textbox / button / gridctrl）按契约表该有的事件键列表；\n"
        "  空列表 = 该类型没有登记契约（XML 里声明的事件一律不会触发）");

    m.def("event_warnings", [w2u]() {
        py::list out;
        for (const xmlprop2::EventWarning& w : xmlprop2::EventWarnings())
            out.append(py::make_tuple(w2u(w.Control), w2u(w.Event), w2u(w.Callback)));
        return out;
    }, "本次进程里加载期发现的\"死绑定\"：[(控件类型, 事件键, 回调名)]。\n"
        "  例：('label','change','on_x') —— XLabel 不会产生 TextChanged，那条绑定永远不触发");

    m.def("prop_warnings", [w2u]() {
        py::list out;
        for (const xmlprop2::PropWarning& w : xmlprop2::Warnings())
            out.append(py::make_tuple(w2u(w.Control), w2u(w.Property)));
        return out;
    }, "本次进程里被拒绝的未知属性：[(控件类型, 属性名)]");

    m.def("clear_diagnostics", []() {
        xmlprop2::ClearWarnings();
        xmlprop2::ClearEventWarnings();
    }, "清空属性 / 事件诊断表（测试用例之间互不串台）");

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


