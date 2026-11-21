// PyMfcHost.Controls.cpp —— 拆分自 PyMfcHost.cpp（按职责分组；接口声明仍在 PyMfcHost.h）。
#include "PyMfcHostInternal.h"

PyMfcHost::~PyMfcHost()
{
        if (m_surface) { delete m_surface; m_surface = nullptr; }
        if (m_bgBrush) ::DeleteObject(m_bgBrush);
        if (m_btnBrush) ::DeleteObject(m_btnBrush);
        if (m_font) ::DeleteObject(m_font);
        for (auto& kv : m_apBrush)
            if (kv.second) ::DeleteObject(kv.second);
        m_apBrush.clear();
        for (auto& kv : m_containerPatternBrush)
        {
            if (kv.second.brush) ::DeleteObject(kv.second.brush);
            if (kv.second.bitmap) ::DeleteObject(kv.second.bitmap);
        }
        m_containerPatternBrush.clear();
    }

void PyMfcHost::splitterNeighbors(HWND self, HWND* above, HWND* below)
{
    *above = nullptr;
    *below = nullptr;
    RECT me = {};
    ::GetWindowRect(self, &me);
    HWND parent = ::GetParent(self);
    if (parent == nullptr)
        return;
    // 容差要覆盖布局间距：StackPanel Spacing=8 时两块之间本来就隔着 8px，
    // 只容忍 4px 就会"明明紧挨着却找不到邻居"，拖了也带不动别人
    long bestAbove = 17, bestBelow = 17;
    // 遍历的是父窗口的兄弟控件；以前写成 GetWindow(self, GW_CHILD)，
    // 那是分隔条自己的子窗口（它一个子窗口都没有），所以邻居恒为空、永远拖不动别人
    for (HWND child = ::GetWindow(parent, GW_CHILD); child != nullptr;
         child = ::GetWindow(child, GW_HWNDNEXT))
    {
        if (child == self || !::IsWindowVisible(child))
            continue;
        RECT rc = {};
        ::GetWindowRect(child, &rc);
        const long overlap = (std::min)(me.right, rc.right) - (std::max)(me.left, rc.left);
        if (overlap <= 20)
            continue;
        const long dAbove = labs(me.top - rc.bottom);
        const long dBelow = labs(rc.top - me.bottom);
        if (dAbove < bestAbove)
        {
            bestAbove = dAbove;
            *above = child;
        }
        if (dBelow < bestBelow)
        {
            bestBelow = dBelow;
            *below = child;
        }
    }
}

HWND PyMfcHost::createControl(const wchar_t* cls, const std::wstring& text,
                       DWORD style, int x, int y, int w, int h, int id)
{
        AFX_MANAGE_STATE(AfxGetAppModuleState());
        PyMfcPerfScope _perf(&pyMfcPerf().createMs, &pyMfcPerf().nCreate);

        // 通知只能经 ON_COMMAND_RANGE(kFirstId, kLastId) 路由，
        // 低位 ID（如 101）建得出来但回调永远不会触发，必须显式拒绝。
        if (id < static_cast<int>(kFirstId) || id > static_cast<int>(kLastId))
            throw std::invalid_argument("PyMfc 控件 ID 必须在 1000..65000 之间");

        // 复用页面时会重复 add_*：同 ID 的旧控件先销毁，避免出现两个同 ID 控件
        if (m_hWnd != nullptr)
        {
            if (HWND old = findByIdDeep(m_hWnd, id))
            {
                ::SetWindowLongPtrW(old, GWLP_ID, 0);
                ::DestroyWindow(old);
            }
        }

        if (_wcsicmp(cls, kSkinBoxClass) == 0)
            ensureSkinBoxClass();

        HWND hwnd = nullptr;
        {
            PyMfcPerfScope _p(&pyMfcPerf().cweMs, &pyMfcPerf().nCwe);
            hwnd = ::CreateWindowExW(
                0, cls, text.c_str(),
                // WS_CLIPSIBLINGS 是关键：没有它，兄弟窗互相之间不裁剪，
                // 叠放次序（Level）就算改了，看上去仍是"最后重画的那块在上面"。
                WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | style,
                x, y, w, h, m_hWnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)),
                AfxGetInstanceHandle(), nullptr);
        }
        registerChildId(::GetAncestor(m_hWnd, GA_ROOT), static_cast<int>(id), hwnd);
        {
            PyMfcPerfScope _p(&pyMfcPerf().fontMs, &pyMfcPerf().nFont);
            applyFont(hwnd);
        }
        // 焦点类控件补 WS_TABSTOP：没有它，系统认为这些控件"不可停"，
        // Tab 键既走不过去，自研的 TabIndex 也拿不到候选。
        if (_wcsicmp(cls, L"EDIT") == 0 || _wcsicmp(cls, L"BUTTON") == 0 ||
            _wcsicmp(cls, L"COMBOBOX") == 0 || _wcsicmp(cls, L"LISTBOX") == 0 ||
            _wcsicmp(cls, L"SYSLINK") == 0)
        {
            ::SetWindowLongPtrW(hwnd, GWL_STYLE,
                                ::GetWindowLongPtrW(hwnd, GWL_STYLE) | WS_TABSTOP);
        }
        // owner-draw 控件的外观由 OnDrawItem 全权绘制；若再套 DarkMode_Explorer，
        // 系统会额外描一圈深色边框，白底页上就变成"每个控件都有黑框"
        const bool ownerDraw = ((style & SS_OWNERDRAW) != 0) || ((style & BS_OWNERDRAW) != 0);
        if (!ownerDraw)
        {
            PyMfcPerfScope _p(&pyMfcPerf().themeMs, &pyMfcPerf().nTheme);
            ::SetWindowTheme(hwnd, L"DarkMode_Explorer", nullptr);
        }
        else
        {
            // owner-draw 控件的窗口不该带系统边框：否则客户区比窗口小一圈，
            // 那 1px 系统边框会露出深色，看上去就是"每个控件都有黑框"
            const LONG_PTR st = ::GetWindowLongPtrW(hwnd, GWL_STYLE);
            if ((st & WS_BORDER) != 0)
            {
                ::SetWindowLongPtrW(hwnd, GWL_STYLE, st & ~static_cast<LONG_PTR>(WS_BORDER));
                ::SetWindowPos(hwnd, nullptr, 0, 0, 0, 0,
                               SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
            }
            // 先取出类过程再换成本进程的绘制过程：类过程负责 BN_CLICKED、
            // 按下/悬停状态与 BM_CLICK，丢掉它按钮就只剩一张画，点不动。
            WNDPROC oldProc = reinterpret_cast<WNDPROC>(
                ::SetWindowLongPtrW(hwnd, GWLP_WNDPROC,
                                    reinterpret_cast<LONG_PTR>(&PyMfcOwnerDrawProc)));
            if (oldProc != nullptr)
                ::SetPropW(hwnd, kOldProcProp, reinterpret_cast<HANDLE>(oldProc));
            // owner-draw 控件也要能被光标逻辑找到宿主（按钮/卡片 hover 手型）
            ::SetPropW(hwnd, kSkinHostProp, reinterpret_cast<HANDLE>(this));
        }

        // 选项卡：内容区底色跟随主题，否则深色皮肤里是一块白板
        if (_wcsicmp(cls, L"SysTabControl32") == 0)        {
            ::SetPropW(hwnd, kTabBrushProp, reinterpret_cast<HANDLE>(m_bgBrush));
            WNDPROC tabOld = reinterpret_cast<WNDPROC>(
                ::SetWindowLongPtrW(hwnd, GWLP_WNDPROC,
                                    reinterpret_cast<LONG_PTR>(&PyMfcTabBodyProc)));
            if (tabOld != nullptr)
                ::SetPropW(hwnd, kTabOldProcProp, reinterpret_cast<HANDLE>(tabOld));
        }

        // 自绘皮肤接管：控件保留状态与交互，绘制改由控件自己的 WM_PAINT 完成
        if (isSkinPaintClass(cls) || m_scrollAreas.count(static_cast<int>(id)) != 0 ||
            m_imageBoxes.count(static_cast<int>(id)) != 0)
        {
            WNDPROC skinOld = reinterpret_cast<WNDPROC>(
                ::SetWindowLongPtrW(hwnd, GWLP_WNDPROC,
                                    reinterpret_cast<LONG_PTR>(&PyMfcSkinWindowProc)));
            if (skinOld != nullptr)
            {
                skinOldProcs()[hwnd] = skinOld;
                ::SetPropW(hwnd, kSkinOldProcProp, reinterpret_cast<HANDLE>(skinOld));
                ::SetPropW(hwnd, kSkinHostProp, reinterpret_cast<HANDLE>(this));
            }
            debugLogf(L"[sknproc] install id=%d cls=%ls old=%p", ::GetDlgCtrlID(hwnd), cls,
                      reinterpret_cast<void*>(skinOld));

            // ListView 的表头是系统自己建的内部子窗口（不经过这里），
            // 不接管它的话表格顶上会留一条原生白条。
            if (_wcsicmp(cls, L"SysListView32") == 0)
            {
                HWND hdr = reinterpret_cast<HWND>(::SendMessageW(hwnd, LVM_GETHEADER, 0, 0));
                if (hdr != nullptr)
                {
                    WNDPROC hdrOld = reinterpret_cast<WNDPROC>(
                        ::SetWindowLongPtrW(hdr, GWLP_WNDPROC,
                                            reinterpret_cast<LONG_PTR>(&PyMfcSkinWindowProc)));
                    if (hdrOld != nullptr)
                    {
                        skinOldProcs()[hdr] = hdrOld;
                        ::SetPropW(hdr, kSkinOldProcProp, reinterpret_cast<HANDLE>(hdrOld));
                        ::SetPropW(hdr, kSkinHostProp, reinterpret_cast<HANDLE>(this));
                    }
                    debugLogf(L"[sknproc] install header id=%d old=%p", ::GetDlgCtrlID(hdr),
                              reinterpret_cast<void*>(hdrOld));
                }
            }
        }

        // 控件级事件：装在最后，链在"当前窗口过程"之后，不影响既有绘制/交互。
        // owner-draw 控件（Button/Static 的 *_OWNERDRAW）即使没注册 Python 事件回调也要装：
        // CSS :hover 的进入/离开就是靠这条链触发重绘的，不能只在 m_anyCtlEvent 时才生效。
        const bool ownerDrawForHover =
            (static_cast<DWORD>(::GetWindowLongPtrW(hwnd, GWL_STYLE)) & 0x0F) == 0x0B;
        if (hwnd != nullptr && (m_anyCtlEvent || ownerDrawForHover))
        {
            WNDPROC cur = reinterpret_cast<WNDPROC>(
                ::SetWindowLongPtrW(hwnd, GWLP_WNDPROC,
                                    reinterpret_cast<LONG_PTR>(&PyMfcEventSubProc)));
            ::SetPropW(hwnd, kEvtPrevProcProp, reinterpret_cast<HANDLE>(cur));
            ::SetPropW(hwnd, kEvtHostProp, reinterpret_cast<HANDLE>(this));
        }

        // 原生控件先按主题默认色上一遍，XML 声明外观时 setControlAppearance 会再覆盖
        applyNativeThemeColors(hwnd, m_bg, m_text);
        return hwnd;
    }

void PyMfcHost::addButton(int id, const std::wstring& text, int x, int y, int w, int h)
{
        HWND hwnd = createControl(L"BUTTON", text, BS_OWNERDRAW, x, y, w, h, id);
        // =1 表面路径：按钮降级为节点（保留隐藏 HWND 取文字/显隐），宿主状态机承担命中/四态。
        if (m_newSurface)
        {
            registerPaintNode(id, 5, NodeLayerControl, x, y, w, h);
            if (hwnd != nullptr) ::ShowWindow(hwnd, SW_HIDE);
        }
    }

void PyMfcHost::addLabel(int id, const std::wstring& text, int x, int y, int w, int h)
{
        HWND hwnd = createControl(L"STATIC", text, SS_OWNERDRAW, x, y, w, h, id);
        // =1 表面路径：标签无输入，隐藏 HWND + 节点绘制；文字仍由保留句柄承载。
        if (m_newSurface)
        {
            registerPaintNode(id, 4, NodeLayerControl, x, y, w, h);
            if (hwnd != nullptr) ::ShowWindow(hwnd, SW_HIDE);
        }
    }

void PyMfcHost::addEdit(int id, const std::wstring& text, int x, int y, int w, int h)
{
        createControl(L"EDIT", text, WS_BORDER | ES_AUTOHSCROLL, x, y, w, h, id);
    }

void PyMfcHost::addListBox(int id, int x, int y, int w, int h)
{
        createControl(L"LISTBOX", L"", WS_BORDER | LBS_NOTIFY | WS_VSCROLL, x, y, w, h, id);
    }

void PyMfcHost::addCheckBox(int id, const std::wstring& text, int x, int y, int w, int h)
{
        createControl(L"BUTTON", text, BS_AUTOCHECKBOX, x, y, w, h, id);
    }

void PyMfcHost::addRadio(int id, const std::wstring& text, int x, int y, int w, int h)
{
        // 本 host 内所有 radio 视为一组：第一个（尚无 radio 时）加 WS_GROUP 起点
        bool hasRadio = false;
        if (m_hWnd != nullptr)
        {
            for (HWND child = ::GetWindow(m_hWnd, GW_CHILD); child != nullptr;
                 child = ::GetWindow(child, GW_HWNDNEXT))
            {
                if (isClassOf(child, L"BUTTON") &&
                    (::GetWindowLongPtrW(child, GWL_STYLE) & BS_TYPEMASK) == BS_AUTORADIOBUTTON)
                {
                    hasRadio = true;
                    break;
                }
            }
        }
        createControl(L"BUTTON", text,
                      BS_AUTORADIOBUTTON | (hasRadio ? 0 : WS_GROUP), x, y, w, h, id);
    }

void PyMfcHost::addGroup(int id, const std::wstring& text, int x, int y, int w, int h)
{
        createControl(L"BUTTON", text, BS_GROUPBOX, x, y, w, h, id);
    }

void PyMfcHost::addCombo(int id, int x, int y, int w, int h)
{
        // h 要包含展开后的下拉列表高度，否则列表会被裁掉
        createControl(L"COMBOBOX", L"", WS_VSCROLL | CBS_DROPDOWNLIST, x, y, w, h, id);
    }

void PyMfcHost::addEditAdvanced(int id, const std::wstring& text, bool multiline, bool readOnly,
                         int maxLength, const std::wstring& placeholder,
                         int x, int y, int w, int h)
{
        const DWORD style = multiline
                                ? (WS_BORDER | WS_VSCROLL | ES_MULTILINE | ES_AUTOVSCROLL | ES_WANTRETURN)
                                : (WS_BORDER | ES_AUTOHSCROLL);
        createControl(L"EDIT", text, style, x, y, w, h, id);
        if (readOnly)
            setReadOnly(id, true);
        if (maxLength > 0)
            setLimitText(id, maxLength);
        if (!placeholder.empty())
            setCueBanner(id, placeholder);
    }

void PyMfcHost::addComboAdvanced(int id, bool dropDownList, int x, int y, int w, int h)
{
        // CBS_DROPDOWN 是"可在下拉项之外手打文字"的可编辑下拉框，CBS_DROPDOWNLIST 只能选
        createControl(L"COMBOBOX", L"",
                      WS_VSCROLL | (dropDownList ? CBS_DROPDOWNLIST : CBS_DROPDOWN),
                      x, y, w, h, id);
    }

void PyMfcHost::addListBoxAdvanced(int id, bool multipleSelect, int x, int y, int w, int h)
{
        createControl(L"LISTBOX", L"", WS_BORDER | LBS_NOTIFY | WS_VSCROLL |
                                            (multipleSelect ? LBS_MULTIPLESEL : 0),
                      x, y, w, h, id);
    }

void PyMfcHost::setLimitText(int id, int n)
{
        if (HWND h = findByIdDeep(m_hWnd, id))
            ::SendMessageW(h, EM_SETLIMITTEXT, static_cast<WPARAM>(n > 0 ? n : 0), 0);
    }

void PyMfcHost::setCueBanner(int id, const std::wstring& text)
{
        if (HWND h = findByIdDeep(m_hWnd, id))
            ::SendMessageW(h, EM_SETCUEBANNER, TRUE, reinterpret_cast<LPARAM>(text.c_str()));
    }

void PyMfcHost::setPasswordChar(int id, wchar_t ch)
{
        if (HWND h = findByIdDeep(m_hWnd, id))
            ::SendMessageW(h, EM_SETPASSWORDCHAR, static_cast<WPARAM>(ch), 0);
    }

void PyMfcHost::addProgress(int id, int x, int y, int w, int h)
{
        createControl(L"msctls_progress32", L"", PBS_SMOOTH, x, y, w, h, id);
        setRange(id, 0, 100);
        setPos(id, 0);
    }

void PyMfcHost::addSlider(int id, int minValue, int maxValue, int x, int y, int w, int h)
{
        createControl(L"msctls_trackbar32", L"", TBS_AUTOTICKS | TBS_HORZ, x, y, w, h, id);
        setRange(id, minValue, maxValue);
        setPos(id, minValue);
    }

void PyMfcHost::addTab(int id, int x, int y, int w, int h)
{
        createControl(L"SysTabControl32", L"", WS_TABSTOP, x, y, w, h, id);
    }

void PyMfcHost::addTree(int id, int x, int y, int w, int h)
{
        createControl(L"SysTreeView32", L"",
                      WS_BORDER | TVS_HASLINES | TVS_LINESATROOT | TVS_HASBUTTONS | WS_TABSTOP,
                      x, y, w, h, id);
    }

void PyMfcHost::addListCtrl(int id, int x, int y, int w, int h)
{
        // HTML 的 .editor table 没有外框、也没有竖网格线，只有行底 1px 细线
        createControl(L"SysListView32", L"", LVS_REPORT | LVS_SHOWSELALWAYS | LVS_NOSORTHEADER,
                      x, y, w, h, id);
        if (HWND hwnd = findByIdDeep(m_hWnd, id))
        {
            ::SendMessageW(hwnd, LVM_SETEXTENDEDLISTVIEWSTYLE, 0,
                           LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);
            // 可编辑表格钩子：记录活动单元格 + 拦 F2/Delete/Insert/Ctrl+C/V
            ::SetPropW(hwnd, kGridHostProp, reinterpret_cast<HANDLE>(this));
            {
                WNDPROC gridOld = reinterpret_cast<WNDPROC>(
                    ::SetWindowLongPtrW(hwnd, GWLP_WNDPROC,
                                        reinterpret_cast<LONG_PTR>(&PyMfcGridProc)));
                ::SetPropW(hwnd, kGridOldProp, reinterpret_cast<HANDLE>(gridOld));
            }
            // 行高交给字体决定（LVM_SETITEMHEIGHT 传 -1 在本机 SysListView32 上会 AV，别碰）
            // 默认 8pt 太小：换成 13px 的雅黑，行高才跟 HTML 的 12px 表格相当
            static HFONT s_tableFont = nullptr;
            if (s_tableFont == nullptr)
            {
                LOGFONTW lf = {};
                lf.lfHeight = -13;
                lf.lfWeight = FW_NORMAL;
                lf.lfCharSet = DEFAULT_CHARSET;
                lf.lfQuality = CLEARTYPE_QUALITY;
                lf.lfFaceName[0] = L'\0';
                ::wcscpy_s(lf.lfFaceName, L"Microsoft YaHei UI");
                s_tableFont = ::CreateFontIndirectW(&lf);
            }
            if (s_tableFont != nullptr)
                ::SendMessageW(hwnd, WM_SETFONT, reinterpret_cast<WPARAM>(s_tableFont), TRUE);
        }
    }

TableControl* PyMfcHost::findNewTable(int id) const
{
    auto it = m_newTables.find(id);
    return it != m_newTables.end() ? it->second : nullptr;
}

void PyMfcHost::tableEventCb(void* ctx, int row, int col, int what)
{
    TableEventCtx* tc = static_cast<TableEventCtx*>(ctx);
    PyMfcHost* h = tc ? tc->host : nullptr;
    if (h == nullptr) return;
    const char* act = "select";
    switch (what)
    {
    case 1: act = "cellclick"; break;
    case 2: act = "dbl";       break;
    case 3: act = "select";    break;
    case 4: act = "edit";       break;
    case 5: act = "delete";     break;
    case 6: act = "insert";     break;
    case 7: act = "paste";      break;
    case 8: act = "sort";       break;   // 点表头 → 按该列排序
    default: break;
    }
    std::wstring text;
    if (act == std::string("paste")) text = hostClipboardText();
    h->fireGridAction(tc ? tc->id : 0, act, row, col, text);
}

void PyMfcHost::addTable(int id, int x, int y, int w, int h)
{
    HWND hw = TableControl::Create(m_hWnd, x, y, w, h, id);
    if (hw == nullptr) return;
    TableControl* tc = TableControl::From(hw);
    if (tc == nullptr) return;
    m_newTables[id] = tc;
    void* evtCtx = m_tableCtx.count(id) ? m_tableCtx[id] : nullptr;
    if (evtCtx == nullptr)
    {
        evtCtx = new TableEventCtx{this, id};
        m_tableCtx[id] = evtCtx;
    }
    tc->SetCallback(&PyMfcHost::tableEventCb, evtCtx);
}

void PyMfcHost::tableClear(int id)
{
    if (TableControl* tc = findNewTable(id)) tc->Clear();
}

int PyMfcHost::tableRowCount(int id) const
{
    if (TableControl* tc = findNewTable(id)) return tc->Rows();
    return 0;
}

void PyMfcHost::tableSetColumns(int id, const std::vector<int>& widths,
                                const std::vector<std::wstring>& titles)
{
    if (TableControl* tc = findNewTable(id)) tc->SetColumns(widths, titles);
}

void PyMfcHost::tableSetColTypes(int id, const std::vector<int>& types)
{
    if (TableControl* tc = findNewTable(id)) tc->SetColTypes(types);
}

void PyMfcHost::tableAppendRow(int id, const std::vector<std::wstring>& cells)
{
    if (TableControl* tc = findNewTable(id)) tc->AppendRow(cells);
}

void PyMfcHost::tableSetCell(int id, int row, int col, const std::wstring& text)
{
    if (TableControl* tc = findNewTable(id)) tc->SetCell(row, col, text);
}

std::wstring PyMfcHost::tableGetCell(int id, int row, int col) const
{
    if (TableControl* tc = findNewTable(id)) return tc->GetText(row, col);
    return L"";
}

py::object PyMfcHost::tableCellRect(int id, int row, int col) const
{
    if (TableControl* tc = findNewTable(id))
    {
        const RECT rc = tc->Cell(row, col);
        return py::make_tuple(rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top);
    }
    return py::none();
}

int PyMfcHost::tableSelectedRow(int id) const
{
    if (TableControl* tc = findNewTable(id)) return tc->Selected();
    return -1;
}

void PyMfcHost::tableSelectRow(int id, int row)
{
    if (TableControl* tc = findNewTable(id)) { tc->Select(row); tc->EnsureVisible(row); }
}

void PyMfcHost::tableEnsureVisible(int id, int row)
{
    if (TableControl* tc = findNewTable(id)) tc->EnsureVisible(row);
}

void PyMfcHost::tableSetFilter(int id, int col, const std::wstring& text)
{
    if (TableControl* tc = findNewTable(id)) tc->SetFilter(col, text);
}

void PyMfcHost::tableClearFilters(int id)
{
    if (TableControl* tc = findNewTable(id)) tc->ClearFilters();
}

void PyMfcHost::tableSort(int id, int col, bool desc)
{
    if (TableControl* tc = findNewTable(id)) tc->SortBy(col, desc);
}

int PyMfcHost::tableViewRows(int id) const
{
    if (TableControl* tc = findNewTable(id)) return tc->ViewRows();
    return 0;
}

void PyMfcHost::addChoice(int id, int x, int y, int w, int h)
{
        createControl(L"COMBOBOX", L"", WS_VSCROLL | CBS_DROPDOWNLIST, x, y, w, h, id);
    }

void PyMfcHost::addDatePicker(int id, int x, int y, int w, int h)
{
        createControl(L"SysDateTimePick32", L"", WS_TABSTOP, x, y, w, h, id);
    }

void PyMfcHost::addPanel(int id, int x, int y, int w, int h)
{
        // 容器（div / 面板）保持普通 STATIC，**不能**用 SS_OWNERDRAW：
        // owner-draw 会把容器交给 OnDrawItem 的 D2D 通道，而 D2D 不受 GDI 裁剪区约束、
        // 也不受 WS_CLIPCHILDREN 约束 —— 容器背板会直接糊在兄弟控件之上，表现为整页只剩底色。
        // 正确做法是走 WM_CTLCOLORSTATIC：透明 div 返回空刷子（不涂），有 background 的 div
        // 返回对应纯色刷子，正好对上 HTML 的 <div> 语义。
        // WS_CLIPCHILDREN 是容器成为真父窗口后的必要条件：容器每次刷新底色时，
        // 系统会把它整块客户区涂掉；没有这条样式，涂的范围包含子控件区域，
        // 子控件（owner-draw 的 label/button）刚画好的内容立刻被容器底色盖住 ——
        // 表现就是"层级、坐标、可见性全对，但卡片里一片空白"。
        HWND hw = createControl(L"STATIC", L"", WS_CHILD | WS_VISIBLE | SS_LEFT | WS_CLIPCHILDREN, x, y, w, h, id);
        // 容器的 :hover 由宿主 OnMouseMove 统一判定（容器窗口被画面层盖住，自己收不到鼠标消息），
        // 这里不需要再给 STATIC 挂窗口过程。
        (void)hw;
        m_containerIds.insert(id);
    }

void PyMfcHost::addWhiteRect(int id, int x, int y, int w, int h)
{
        HWND hwnd = createControl(L"STATIC", L"", WS_CHILD | WS_VISIBLE | SS_WHITERECT | WS_BORDER, x, y, w, h, id);
        // =1 表面路径：白板降级为绘制节点，HWND 隐藏（句柄保留，GetDlgItem/SetWindowText 等 API 仍走通）。
        if (m_newSurface)
        {
            registerPaintNode(id, 1, NodeLayerControl, x, y, w, h);
            if (hwnd != nullptr) ::ShowWindow(hwnd, SW_HIDE);
        }
    }

void PyMfcHost::setControlFont(int id, const std::wstring& face, int px, bool bold)
{
        HWND h = findByIdDeep(m_hWnd, id);
        if (h == nullptr || px <= 0) return;
        static std::map<std::wstring, HFONT> cache;
        const std::wstring key = face + L"|" + std::to_wstring(px) + (bold ? L"|b" : L"|n");
        HFONT f = nullptr;
        auto it = cache.find(key);
        if (it != cache.end()) f = it->second;
        else
        {
            LOGFONTW lf = { 0 };
            lf.lfHeight = -px;
            lf.lfWeight = bold ? FW_SEMIBOLD : FW_NORMAL;
            lf.lfCharSet = DEFAULT_CHARSET;
            lf.lfQuality = CLEARTYPE_QUALITY;
            ::wcsncpy_s(lf.lfFaceName, LF_FACESIZE, face.c_str(), _TRUNCATE);
            f = ::CreateFontIndirectW(&lf);
            cache[key] = f;
        }
        if (f != nullptr)
            ::SendMessageW(h, WM_SETFONT, reinterpret_cast<WPARAM>(f), TRUE);
    }

void PyMfcHost::setControlAccent(int id, int r, int g, int b)
{
        m_accent[id] = RGB(r, g, b);
        if (HWND h = findByIdDeep(m_hWnd, id)) ::InvalidateRect(h, nullptr, TRUE);
    }

void PyMfcHost::setLabelColor(int id, int r, int g, int b)
{
        m_labelFg[id] = RGB(r, g, b);
        if (HWND h = findByIdDeep(m_hWnd, id)) ::InvalidateRect(h, nullptr, TRUE);
    }

void PyMfcHost::addSplitter(int id, int x, int y, int w, int h)
{
        HWND hwnd = createControl(L"STATIC", L"", WS_CHILD | WS_VISIBLE, x, y, w, h, id);
        // 原生 STATIC 收到鼠标消息什么也不做：不换成自己这个过程，
        // 分隔条就是个画出来的方块，拖不动也影响不到邻居
        if (hwnd != nullptr)
            ::SetWindowLongPtrW(hwnd, GWLP_WNDPROC,
                                reinterpret_cast<LONG_PTR>(&PyMfcSplitterProc));
    }

void PyMfcHost::addStaticLine(int id, int x, int y, int w, int h)
{
        // 登记必须早于 createControl：创建过程中就会触发首帧 WM_DRAWITEM，
        // 登记晚一步的话首帧按"普通标签"画出去，之后又不一定再有重绘，
        // 屏幕上就永远停在那张没有线的图（滚动区当年踩的是同一个坑）。
        m_lines.insert(id);
        createControl(L"STATIC", L"", SS_OWNERDRAW, x, y, w, h, id);
        // =1 表面路径：分割线注册为节点；HWND 保留（拖拽热条在 onDrag 后装命中/捕获过程），
        // 其自身 WM_PAINT 由 OwnerDrawProc 在节点存在时短路，不再直绘。
        if (m_newSurface)
            registerPaintNode(id, 3, NodeLayerControl, x, y, w, h);
        debugLogf(L"[line-reg] id=%d hwnd=%p lines=%d", id,
                  static_cast<void*>(findByIdDeep(m_hWnd, id)), static_cast<int>(m_lines.size()));
    }

void PyMfcHost::addHyperlink(int id, const std::wstring& text, int x, int y, int w, int h)
{
        createControl(L"SysLink", text, WS_TABSTOP, x, y, w, h, id);
    }

void PyMfcHost::addRichText(int id, int x, int y, int w, int h)
{
        createControl(L"RICHEDIT50W", L"", WS_BORDER | ES_MULTILINE | ES_AUTOVSCROLL | ES_AUTOHSCROLL, x, y, w, h, id);
    }

void PyMfcHost::addToggle(int id, const std::wstring& text, int x, int y, int w, int h)
{
        createControl(L"BUTTON", text, WS_TABSTOP | BS_AUTOCHECKBOX, x, y, w, h, id);
    }

void PyMfcHost::addBitmapButton(int id, const std::wstring& text, int x, int y, int w, int h)
{
        createControl(L"BUTTON", text, WS_TABSTOP | BS_OWNERDRAW, x, y, w, h, id);
    }

void PyMfcHost::addStaticBitmap(int id, int x, int y, int w, int h)
{
        // 图片盒子改自绘而不是 SS_BITMAP：原生位图 Static 只会按原图尺寸居中/裁剪，
        // CSS 的 width/height/object-fit 一条都落不了地（HTML 里 <img> 会自动缩放）。
        // 必须先登记再 createControl —— createControl 据此决定装不装自绘过程。
        m_imageBoxes[id] = std::wstring();
        createControl(L"STATIC", L"", SS_OWNERDRAW, x, y, w, h, id);
    }

void PyMfcHost::addGrid(int id, int x, int y, int w, int h)
{
        createControl(L"SysListView32", L"", WS_BORDER | LVS_REPORT | LVS_SHOWSELALWAYS, x, y, w, h, id);
    }

void PyMfcHost::addScrolledWindow(int id, int x, int y, int w, int h)
{
        // 必须先登记：createControl 里据此决定给这个 Static 装自绘过程
        m_scrollAreas.insert(id);
        createControl(L"STATIC", L"", SS_OWNERDRAW, x, y, w, h, id);
    }

void PyMfcHost::addCalendar(int id, int x, int y, int w, int h)
{
        createControl(L"SysMonthCal32", L"", WS_TABSTOP, x, y, w, h, id);
    }

std::wstring PyMfcHost::getCalendarDate(int id) const
{
        HWND h = findByIdDeep(m_hWnd, id);
        if (h == nullptr)
            return {};
        SYSTEMTIME st = {};
        if (::SendMessageW(h, MCM_GETCURSEL, 0, reinterpret_cast<LPARAM>(&st)) == 0)
            return {};
        return MonthCalDateText(st);
    }

bool PyMfcHost::setCalendarDate(int id, const std::wstring& value)
{
        HWND h = findByIdDeep(m_hWnd, id);
        if (h == nullptr)
            return false;
        int y = 0, m = 0, d = 0;
        if (swscanf_s(value.c_str(), L"%d-%d-%d", &y, &m, &d) != 3 || y < 1601 || m < 1 || m > 12 ||
            d < 1 || d > 31)
            return false;
        SYSTEMTIME st = {};
        st.wYear = static_cast<WORD>(y);
        st.wMonth = static_cast<WORD>(m);
        st.wDay = static_cast<WORD>(d);
        ::SendMessageW(h, MCM_SETCURSEL, 0, reinterpret_cast<LPARAM>(&st));
        ::InvalidateRect(h, nullptr, TRUE);
        return true;
    }

void PyMfcHost::addSkinBox(int id, int kind, int x, int y, int w, int h)
{
        createControl(kSkinBoxClass, L"", WS_TABSTOP, x, y, w, h, id);
        m_skinKinds[id] = kind;
        if (HWND hwnd = findByIdDeep(m_hWnd, id))
        {
            ::SetPropW(hwnd, kSkinKindProp, reinterpret_cast<HANDLE>(static_cast<INT_PTR>(kind)));
            if (kind == kSkinBoxBusy)
            {
                m_busyPhase[id] = 0;
                ::SetTimer(hwnd, kBusyTimerId, 80, nullptr);   // 12fps 旋转
            }
        }
        ::InvalidateRect(findByIdDeep(m_hWnd, id), nullptr, TRUE);
    }

void PyMfcHost::addShadowBox(int id, int x, int y, int w, int h, int pad, int radius,
                      int dx, int dy, int blur,
                      const std::wstring& shadowColor, const std::wstring& bgColor,
                      const std::wstring& backColor, const std::wstring& borderColor,
                      int borderW)
{
        pymfcskin::ShadowBoxStyle s;
        s.pad = pad > 0 ? pad : 0;
        s.radius = radius;
        s.dx = dx;
        s.dy = dy;
        s.blur = blur > 0 ? blur : 0;
        s.borderW = borderW;
        int a = 255;
        COLORREF c = 0;
        if (xmlapp::ParseColor(shadowColor, c, &a))
        {
            s.shadow = c;
            s.alpha = a < 0 ? 0 : (a > 255 ? 255 : a);
        }
        COLORREF tmp = 0;
        if (xmlapp::ParseColor(bgColor, tmp)) s.bg = tmp;
        if (xmlapp::ParseColor(backColor, tmp)) s.back = tmp;
        if (xmlapp::ParseColor(borderColor, tmp)) s.border = tmp;

        m_shadowBox[id] = s;
        addSkinBox(id, kSkinBoxShadow, x - s.pad, y - s.pad, w + 2 * s.pad, h + 2 * s.pad);
        // =1 表面路径：阴影底板降级为浮层节点，HWND 隐藏（阴影纯装饰，无输入语义）。
        if (m_newSurface)
        {
            registerPaintNode(id, 2, NodeLayerOverlay, x - s.pad, y - s.pad,
                               w + 2 * s.pad, h + 2 * s.pad);
            if (HWND hw = findByIdDeep(m_hWnd, id)) ::ShowWindow(hw, SW_HIDE);
        }
        if (HWND hw = findByIdDeep(m_hWnd, id))
        {
            const LONG_PTR ex = ::GetWindowLongPtrW(hw, GWL_EXSTYLE);
            // 默认**不加** WS_EX_TRANSPARENT：加上之后本窗口被合成器当成纯装饰，
            // 宿主重绘时会从父窗口透出黑缝（详情浮层内部出现短黑线）。
            // 阴影区吃不吃鼠标由调用方自己权衡，这里不替它决定。
            (void)ex;
            ::SetWindowLongPtrW(hw, GWL_EXSTYLE, ex & ~WS_EX_TRANSPARENT);
            ::SetWindowPos(hw, nullptr, 0, 0, 0, 0,
                           SWP_NOZORDER | SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE |
                               SWP_FRAMECHANGED);
        }
    }

void PyMfcHost::setControlCursor(int id, const std::wstring& keyword)
{
        std::wstring s;
        for (wchar_t c : keyword) s.push_back(static_cast<wchar_t>(towlower(c)));
        if (s.empty() || s == L"auto")
            m_cssCursor.erase(id);
        else
            m_cssCursor[id] = s;
    }

std::wstring PyMfcHost::getControlCursor(int id) const
{
        auto it = m_cssCursor.find(id);
        return it != m_cssCursor.end() ? it->second : std::wstring();
    }

void PyMfcHost::setControlPointerEvents(int id, const std::wstring& keyword)
    {
        std::wstring s;
        for (wchar_t c : keyword) s.push_back(static_cast<wchar_t>(towlower(c)));
        // 只认 none：auto / 空 / 其它一律视作"正常接收"，不留在表里
        const bool none = (s == L"none");
        if (none)
            m_cssPointerEvents[id] = L"none";
        else
            m_cssPointerEvents.erase(id);
        // 真正的落地点：WS_EX_TRANSPARENT 让窗口从鼠标命中链上整体消失，
        // 点击/滚轮/悬停都落到下面的兄弟控件 —— 与 CSS pointer-events:none 一致。
        HWND h = findByIdDeep(m_hWnd, id);
        if (h != nullptr && ::IsWindow(h))
        {
            LONG ex = static_cast<LONG>(::GetWindowLongPtrW(h, GWL_EXSTYLE));
            const LONG want = static_cast<LONG>(WS_EX_TRANSPARENT);
            const LONG got = ex & want;
            if (none && (got == 0))
                ::SetWindowLongPtrW(h, GWL_EXSTYLE, ex | want);
            else if (!none && (got != 0))
                ::SetWindowLongPtrW(h, GWL_EXSTYLE, ex & ~want);
        }
    }

std::wstring PyMfcHost::getControlPointerEvents(int id) const
    {
        auto it = m_cssPointerEvents.find(id);
        return it != m_cssPointerEvents.end() ? it->second : std::wstring(L"auto");
    }

bool PyMfcHost::isControlMouseTransparent(int id) const
    {
        HWND h = findByIdDeep(m_hWnd, id);
        if (h == nullptr || !::IsWindow(h))
            return false;
        const LONG ex = static_cast<LONG>(::GetWindowLongPtrW(h, GWL_EXSTYLE));
        return (ex & static_cast<LONG>(WS_EX_TRANSPARENT)) != 0;
    }

HCURSOR PyMfcHost::hoverCursor(HWND hwnd) const
{
        if (hwnd == nullptr)
            return nullptr;
        const int id = ::GetDlgCtrlID(hwnd);

        // 显式声明的 CSS cursor 优先：这是 HTML 里唯一能"精确说我要什么光标"的写法，
        // 不能让按控件类型猜出来的手型把它盖掉。
        {
            auto itc = m_cssCursor.find(id);
            if (itc != m_cssCursor.end())
            {
                if (HCURSOR c = cursorOfCss(itc->second))
                    return c;
            }
        }

        auto itKind = m_skinKinds.find(id);
        if (itKind != m_skinKinds.end())
        {
            switch (itKind->second)
            {
            case kSkinBoxQCanvas:                 // 画布：静止十字，按住拖动 grabbing
            case kSkinBoxFlick:
                return pyMfcCanvasCursor(hwnd);
            case kSkinBoxScrollBar:
            case kSkinBoxSwitch:
            case kSkinBoxSpin:
            case kSkinBoxRange:
            case kSkinBoxPages:
            case kSkinBoxDial:
            case kSkinBoxTumbler:
            case kSkinBoxTable:
            case kSkinBoxTree:
            case kSkinBoxListView:
            case kSkinBoxGridView:
            case kSkinBoxPathView:
            case kSkinBoxMenu:
            case kSkinBoxMenuBar:
                return pyMfcHandCursor();
            default:                              // 容器 / 忙碌指示 / 阴影底板：箭头
                return nullptr;
            }
        }

        if (m_scrollAreas.count(id) != 0)
        {
            // 滚动区域只有右侧那一条滚动条是"可点"的，内容区仍是普通箭头
            POINT pt = {};
            ::GetCursorPos(&pt);
            ::ScreenToClient(hwnd, &pt);
            RECT cr = {};
            ::GetClientRect(hwnd, &cr);
            if (pt.x >= cr.right - 18 && pt.y >= cr.top && pt.y < cr.bottom)
                return pyMfcHandCursor();
            return nullptr;
        }

        wchar_t cls[64] = {};
        ::GetClassNameW(hwnd, cls, 64);
        if (_wcsicmp(cls, L"BUTTON") == 0 || _wcsicmp(cls, L"SysLink") == 0 ||
            _wcsicmp(cls, L"ComboBox") == 0)
            return pyMfcHandCursor();

        // 明确绑了点击/双击的控件（卡片、图标、自定义块）同样算可点
        if (!m_click.empty() && m_click.count(static_cast<UINT>(id)) != 0)
            return pyMfcHandCursor();
        if (!m_dblclick.empty() && m_dblclick.count(static_cast<UINT>(id)) != 0)
            return pyMfcHandCursor();
        return nullptr;
}

bool PyMfcHost::skinBoxMouse(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{        const int id = ::GetDlgCtrlID(hwnd);
        auto itKind = m_skinKinds.find(id);
        if (itKind == m_skinKinds.end())
            return false;
        const int kind = itKind->second;
        POINT pt = {static_cast<LONG>(static_cast<short>(LOWORD(lp))),
                    static_cast<LONG>(static_cast<short>(HIWORD(lp)))};
        RECT rc = {};
        ::GetClientRect(hwnd, &rc);

        if (msg == WM_MOUSEWHEEL && (kind == kSkinBoxSpin || kind == kSkinBoxSwitch))
        {
            const int delta = GET_WHEEL_DELTA_WPARAM(wp) > 0 ? 1 : -1;
            if (kind == kSkinBoxSpin)
                stepSpin(id, delta);
            else
                setSwitchValue(id, !getSwitchValue(id));
            ::InvalidateRect(hwnd, nullptr, TRUE);
            return true;
        }

        // 可编辑画布：点选 / 框选 / Delete 删除。默认不开（m_cvEdit 里没有这个 id 就不介入），
        // 这样既有画布用例的点击语义一点没变。
        if (kind == kSkinBoxQCanvas && m_cvEdit.count(id) != 0)
        {
            if (msg == WM_LBUTTONDOWN)
            {
                ::SetFocus(hwnd);
                ::SetCapture(hwnd);
                m_cvDown[id] = pt;
                const int hit = canvasHitTest(id, pt);
                if (hit >= 0)
                {
                    m_cvBand.erase(id);
                    setCanvasSelection(id, std::vector<int>{hit});
                }
                else
                {
                    m_cvSel[id].clear();
                    m_cvBand[id] = RECT{pt.x, pt.y, pt.x, pt.y};
                    ::InvalidateRect(hwnd, nullptr, TRUE);
                }
                return true;
            }
            if (msg == WM_MOUSEMOVE && m_cvBand.count(id) != 0)
            {
                POINT d = m_cvDown.count(id) ? m_cvDown[id] : pt;
                RECT b = {d.x, d.y, pt.x, pt.y};
                if (b.right < b.left) { const LONG t = b.left; b.left = b.right; b.right = t; }
                if (b.bottom < b.top) { const LONG t = b.top; b.top = b.bottom; b.bottom = t; }
                m_cvBand[id] = b;
                ::InvalidateRect(hwnd, nullptr, TRUE);
                return true;
            }
            if (msg == WM_LBUTTONUP)
            {
                if (::GetCapture() == hwnd)
                    ::ReleaseCapture();
                auto bi = m_cvBand.find(id);
                if (bi != m_cvBand.end())
                {
                    RECT b = bi->second;
                    m_cvBand.erase(bi);
                    if (b.right - b.left > 4 && b.bottom - b.top > 4)
                    {
                        std::vector<int> picked;
                        auto ii = m_cvItems.find(id);
                        if (ii != m_cvItems.end())
                        {
                            for (int k = 0; k < static_cast<int>(ii->second.size()); ++k)
                            {
                                if (!ii->second[k].selectable)
                                    continue;
                                RECT u = {};
                                if (::IntersectRect(&u, &ii->second[k].box, &b))
                                    picked.push_back(k);
                            }
                        }
                        setCanvasSelection(id, picked);
                    }
                    else
                    {
                        m_cvSel[id].clear();
                        fireCanvasSelect(id);
                        ::InvalidateRect(hwnd, nullptr, TRUE);
                    }
                }
                return true;
            }
            if (msg == WM_KEYDOWN && wp == VK_DELETE)
            {
                removeCanvasSelection(id);
                return true;
            }
        }

        if (kind == kSkinBoxSwitch)
        {
            if (msg == WM_LBUTTONDOWN)
            {
                setSwitchValue(id, !getSwitchValue(id));
                ::InvalidateRect(hwnd, nullptr, TRUE);
                return true;
            }
            return false;
        }

        if (kind == kSkinBoxSpin)
        {
            if (msg == WM_LBUTTONDOWN)
            {
                const int btnW = 18;
                if (pt.x >= rc.right - btnW)
                {
                    const int mid = rc.bottom / 2;
                    stepSpin(id, pt.y < mid ? +1 : -1);
                    ::InvalidateRect(hwnd, nullptr, TRUE);
                    return true;
                }
            }
            if (msg == WM_LBUTTONDBLCLK)
            {
                stepSpin(id, +1);
                ::InvalidateRect(hwnd, nullptr, TRUE);
                return true;
            }
            return false;
        }

        if (kind == kSkinBoxRange)
        {
            const int knob = 14;
            const int lo = rangeMinOf(id);
            const int hi = rangeMaxOf(id);
            const int vlo = getRangeLow(id);
            const int vhi = getRangeHigh(id);
            const int usable = (rc.right - knob) - rc.left;
            auto posOf = [&](int v) -> int {
                const int span = hi > lo ? hi - lo : 0;
                if (span <= 0) return rc.left + knob / 2;
                return rc.left + knob / 2 +
                       static_cast<int>((static_cast<long long>(usable) * (v - lo)) / span);
            };
            auto valueOf = [&](int x) -> int {
                if (usable <= 0 || hi <= lo) return lo;
                int v = lo + static_cast<int>((static_cast<long long>(hi - lo) * (x - rc.left - knob / 2)) / usable);
                return v < lo ? lo : (v > hi ? hi : v);
            };

            if (msg == WM_LBUTTONDOWN)
            {
                ::SetFocus(hwnd);
                ::SetCapture(hwnd);
                const int kLo = posOf(vlo);
                const int kHi = posOf(vhi);
                const int dLo = abs(pt.x - kLo);
                const int dHi = abs(pt.x - kHi);
                m_dragThumb[id] = (dLo <= dHi) ? 1 : 2;
                // 直接跳到点击位置：拖动手感更接近 QML 的 Slider
                const int xv = valueOf(pt.x);
                if (m_dragThumb[id] == 1)
                {
                    const int lv = xv < vhi ? xv : vhi;
                    setRangeValues(id, lv, vhi);
                }
                else
                {
                    const int hv = xv > vlo ? xv : vlo;
                    setRangeValues(id, vlo, hv);
                }
                ::InvalidateRect(hwnd, nullptr, TRUE);
                return true;
            }
            if (msg == WM_MOUSEMOVE && m_dragThumb.count(id) != 0)
            {
                const int v = valueOf(pt.x);
                if (m_dragThumb[id] == 1)
                {
                    const int curHi = getRangeHigh(id);
                    setRangeValues(id, v < curHi ? v : curHi, curHi);
                }
                else
                {
                    const int curLo = getRangeLow(id);
                    setRangeValues(id, curLo, v > curLo ? v : curLo);
                }
                ::InvalidateRect(hwnd, nullptr, TRUE);
                return true;
            }
            if (msg == WM_LBUTTONUP)
            {
                m_dragThumb.erase(id);
                ::ReleaseCapture();
                return true;
            }
            return false;
        }

        if (kind == kSkinBoxPages)
        {
            if (msg == WM_LBUTTONDOWN)
            {
                const int count = m_pageCount.count(id) ? m_pageCount[id] : 3;
                const int dot = 8, gap = 6;
                const int totalW = count * dot + (count - 1) * gap;
                const int x0 = (rc.right - totalW) / 2;
                const int idx = (pt.x - x0) / (dot + gap);
                if (idx >= 0 && idx < count)
                {
                    m_pageCur[id] = idx;
                    ::InvalidateRect(hwnd, nullptr, TRUE);
                    return true;
                }
            }
            return false;
        }
        if (kind == kSkinBoxDial)
        {
            const int lo = rangeMinOf(id);
            const int hi = rangeMaxOf(id);
            auto valueOf = [&](int px, int py) -> int {
                const int cx = (rc.left + rc.right) / 2;
                const int cy = (rc.top + rc.bottom) / 2;
                double a = std::atan2(static_cast<double>(py - cy), static_cast<double>(px - cx));
                double deg = a * 180.0 / 3.14159265358979323846;   // -180..180
                if (deg < 0.0)
                    deg += 360.0;                                   // 0..360
                // 弧从 135° 起顺时针扫 270°（0..360 里是 135 -> 405，绕回来到 45）
                double t = deg - 135.0;
                if (t < 0.0)
                    t += 360.0;
                if (t > 270.0)
                {
                    // 落在缺口的 90° 里，吸到最近的端点
                    t = t < 315.0 ? 270.0 : 0.0;
                }
                const int span = hi > lo ? hi - lo : 0;
                return lo + static_cast<int>((t * span) / 270.0 + 0.5);
            };
            if (msg == WM_LBUTTONDOWN)
            {
                ::SetFocus(hwnd);
                ::SetCapture(hwnd);
                m_dragMode[id] = 1;
                m_dialVal[id] = valueOf(pt.x, pt.y);
                ::InvalidateRect(hwnd, nullptr, TRUE);
                return true;
            }
            if (msg == WM_MOUSEMOVE && m_dragMode.count(id) != 0 && m_dragMode[id] == 1)
            {
                m_dialVal[id] = valueOf(pt.x, pt.y);
                ::InvalidateRect(hwnd, nullptr, TRUE);
                return true;
            }
            if (msg == WM_LBUTTONUP && m_dragMode.count(id) != 0)
            {
                m_dragMode.erase(id);
                ::ReleaseCapture();
                return true;
            }
            if (msg == WM_MOUSEWHEEL)
            {
                const int d = GET_WHEEL_DELTA_WPARAM(wp) > 0 ? 1 : -1;
                stepDial(id, d);
                ::InvalidateRect(hwnd, nullptr, TRUE);
                return true;
            }
            return false;
        }

        if (kind == kSkinBoxTumbler)
        {
            if (msg == WM_LBUTTONDOWN)
            {
                ::SetFocus(hwnd);
                const int h = rc.bottom - rc.top;
                stepTumbler(id, pt.y < rc.top + h / 3 ? 1 : (pt.y > rc.top + (2 * h) / 3 ? -1 : 0));
                ::InvalidateRect(hwnd, nullptr, TRUE);
                return true;
            }
            if (msg == WM_MOUSEWHEEL)
            {
                stepTumbler(id, GET_WHEEL_DELTA_WPARAM(wp) > 0 ? -1 : 1);
                ::InvalidateRect(hwnd, nullptr, TRUE);
                return true;
            }
            return false;
        }

        if (kind == kSkinBoxScrollBar)
        {
            const int lo = rangeMinOf(id);
            const int hi = rangeMaxOf(id);
            const int page = m_scrollPage.count(id) ? m_scrollPage[id] : 10;
            const bool vert = m_scrollVert.count(id) && m_scrollVert[id];
            const int cur = m_scrollVal.count(id) ? m_scrollVal[id] : lo;
            const int span = hi > lo ? hi - lo : 1;
            const int total = vert ? (rc.bottom - rc.top) : (rc.right - rc.left);
            int thumb = static_cast<int>((static_cast<long long>(total) * page) / (span + page));
            if (thumb < 12)
                thumb = 12;
            if (thumb > total)
                thumb = total;
            const int free = total - thumb;
            const int at = vert ? (pt.y - rc.top) : (pt.x - rc.left);

            if (msg == WM_LBUTTONDOWN)
            {
                ::SetFocus(hwnd);
                const int t0 = span > 0 ? static_cast<int>((static_cast<long long>(free) * (cur - lo)) / span) : 0;
                if (at >= t0 && at <= t0 + thumb)
                {
                    m_dragMode[id] = 2;
                    m_dragOrigin[id] = at - t0;
                    ::SetCapture(hwnd);
                }
                else
                {
                    // 点轨道：跳一页
                    const int v = cur + (at < t0 ? -page : page);
                    setScrollValueUser(id, v);
                }
                ::InvalidateRect(hwnd, nullptr, TRUE);
                return true;
            }
            if (msg == WM_MOUSEMOVE && m_dragMode.count(id) != 0 && m_dragMode[id] == 2)
            {
                const int pos = at - (m_dragOrigin.count(id) ? m_dragOrigin[id] : 0);
                const int v = free > 0 ? lo + static_cast<int>((static_cast<long long>(span) * pos) / free) : lo;
                setScrollValueUser(id, v);
                ::InvalidateRect(hwnd, nullptr, TRUE);
                return true;
            }
            if (msg == WM_LBUTTONUP && m_dragMode.count(id) != 0)
            {
                m_dragMode.erase(id);
                m_dragOrigin.erase(id);
                ::ReleaseCapture();
                return true;
            }
            if (msg == WM_MOUSEWHEEL)
            {
                const int d = GET_WHEEL_DELTA_WPARAM(wp) > 0 ? -1 : 1;
                setScrollValueUser(id, cur + d * (page / 2 > 1 ? page / 2 : 1));
                ::InvalidateRect(hwnd, nullptr, TRUE);
                return true;
            }
            return false;
        }

        if (kind == kSkinBoxTable)
        {
            const int headH = 26, rowH = 24;
            const int ncol = static_cast<int>(m_tblCols.count(id) ? m_tblCols[id].size() : 0);
            const int total = getTableRowCount(id);
            if (msg == WM_MOUSEWHEEL)
            {
                const int d = GET_WHEEL_DELTA_WPARAM(wp) > 0 ? -1 : 1;
                int first = m_tblScroll.count(id) ? m_tblScroll[id] : 0;
                first += d;
                if (first < 0)
                    first = 0;
                if (first > total - 1)
                    first = total > 0 ? total - 1 : 0;
                m_tblScroll[id] = first;
                ::InvalidateRect(hwnd, nullptr, TRUE);
                return true;
            }
            if (msg == WM_LBUTTONDOWN)
            {
                ::SetFocus(hwnd);
                if (pt.y < rc.top + headH)
                {
                    // 点表头 → 排序（同一列再点反向）
                    if (ncol > 0 && cwOf(id) > 0)
                        sortTable(id, (pt.x - rc.left) / cwOf(id));
                    return true;
                }
                const int row = (m_tblScroll.count(id) ? m_tblScroll[id] : 0) +
                                (pt.y - rc.top - headH) / rowH;
                if (row >= 0 && row < total)
                {
                    m_tblSel[id] = row;
                    ::InvalidateRect(hwnd, nullptr, TRUE);
                }
                return true;
            }
            return false;
        }

        if (kind == kSkinBoxTree)
        {
            const int rowH = 24;
            const std::vector<int> levels = m_treeLevel.count(id) ? m_treeLevel[id] : std::vector<int>();
            // 可见行 -> 原始下标（和绘制时同样的折叠过滤）
            auto rowAt = [&](int visRow) -> int {
                int n = 0;
                for (int i = 0; i < static_cast<int>(m_treeText[id].size()); ++i)
                {
                    if (isTreeRowHidden(id, i))
                        continue;
                    if (n == visRow)
                        return i;
                    ++n;
                }
                return -1;
            };
            if (msg == WM_MOUSEWHEEL)
            {
                const int d = GET_WHEEL_DELTA_WPARAM(wp) > 0 ? -1 : 1;
                int first = m_treeScroll.count(id) ? m_treeScroll[id] : 0;
                first += d;
                if (first < 0)
                    first = 0;
                m_treeScroll[id] = first;
                ::InvalidateRect(hwnd, nullptr, TRUE);
                return true;
            }
            if (msg == WM_LBUTTONDOWN)
            {
                ::SetFocus(hwnd);
                const int vis = (m_treeScroll.count(id) ? m_treeScroll[id] : 0) +
                                (pt.y - rc.top) / rowH;
                const int row = rowAt(vis);
                if (row >= 0)
                {
                    // 点在箭头附近 → 展开/折叠；其余位置 → 选中
                    const int level = row < static_cast<int>(levels.size()) ? levels[row] : 0;
                    const int arrowX = rc.left + 6 + level * 16;
                    if (m_treeKids[id][row] && pt.x >= arrowX - 4 && pt.x <= arrowX + 12)
                        toggleTreeRow(id, row);
                    else
                    {
                        m_treeSel[id] = row;
                        ::InvalidateRect(hwnd, nullptr, TRUE);
                    }
                }
                return true;
            }
            return false;
        }

        if (kind == kSkinBoxFlick)
        {
            if (!m_flOff.count(id))
                m_flOff[id] = POINT{0, 0};
            if (msg == WM_LBUTTONDOWN)
            {
                ::SetFocus(hwnd);
                ::SetCapture(hwnd);
                m_flPos[id] = pt;
                m_flRaw[id] = POINT{m_flOff[id].x, m_flOff[id].y};
                m_flVel[id] = POINT{0, 0};
                m_flStamp[id] = ::GetTickCount64();
                return true;
            }
            if (msg == WM_MOUSEMOVE)
            {
                if (::GetCapture() == hwnd)
                {
                    int vw, vh, cw, ch, minX, minY;
                    flickMetrics(id, vw, vh, cw, ch, minX, minY);
                    POINT raw = m_flRaw.count(id) ? m_flRaw[id] : POINT{0, 0};
                    raw.x += pt.x - m_flPos[id].x;
                    raw.y += pt.y - m_flPos[id].y;
                    m_flRaw[id] = raw;
                    // 越界部分按 0.5 阻尼：拉得动，但拉不远（松手再由回弹收回来）
                    POINT off = raw;
                    if (off.x > 0) off.x = off.x / 2;
                    else if (off.x < minX) off.x = minX + (off.x - minX) / 2;
                    if (off.y > 0) off.y = off.y / 2;
                    else if (off.y < minY) off.y = minY + (off.y - minY) / 2;
                    const POINT last = m_flPos[id];
                    m_flOff[id] = off;
                    flickApply(id);
                    // 速度只取"这一帧的位移"，累积位移会把速度放大成天文数字
                    m_flVel[id] = POINT{(pt.x - last.x) * 60, (pt.y - last.y) * 60};
                    m_flPos[id] = pt;
                    m_flStamp[id] = ::GetTickCount64();
                }
                return true;
            }
            if (msg == WM_LBUTTONUP)
            {
                if (::GetCapture() == hwnd)
                    ::ReleaseCapture();
                const POINT v = m_flVel.count(id) ? m_flVel[id] : POINT{0, 0};
                if (v.x != 0 || v.y != 0)
                    ::SetTimer(hwnd, kFlickTimerId, 16, nullptr);
                return true;
            }
            if (msg == WM_MOUSEWHEEL)
            {
                int vw, vh, cw, ch, minX, minY;
                flickMetrics(id, vw, vh, cw, ch, minX, minY);
                const int d = GET_WHEEL_DELTA_WPARAM(wp) / WHEEL_DELTA;
                setFlickOffset(id, m_flOff[id].x, m_flOff[id].y + d * 60);
                (void)minY;
                return true;
            }
            return false;
        }

        if (kind == kSkinBoxPager)
        {
            const int cnt = viewCount(id);
            RECT hr = {};
            ::GetClientRect(hwnd, &hr);
            if (msg == WM_LBUTTONDOWN)
            {
                m_pgDragX[id] = pt.x;
                ::SetFocus(hwnd);
                return true;
            }
            if (msg == WM_LBUTTONUP)
            {
                const int dx = pt.x - (m_pgDragX.count(id) ? m_pgDragX[id] : pt.x);
                if (dx <= -40)
                    viewAnimate(id, viewIndex(id) + 1, 260);
                else if (dx >= 40)
                    viewAnimate(id, viewIndex(id) - 1, 260);
                else if (pt.y >= hr.bottom - 26 && cnt > 0)
                {
                    // 点页码条：左三分之一上一页、右三分之一下一页
                    if (pt.x < hr.left + (hr.right - hr.left) / 3)
                        viewAnimate(id, viewIndex(id) - 1, 200);
                    else if (pt.x > hr.right - (hr.right - hr.left) / 3)
                        viewAnimate(id, viewIndex(id) + 1, 200);
                }
                m_pgDragX[id] = pt.x;
                return true;
            }
            return msg == WM_MOUSEMOVE;
        }

        if (kind == kSkinBoxListView)
        {
            const int rowH = lvRowH(id);
            const int sc = getListScroll(id);
            auto rowAt = [&](int y) -> int {
                const int i = (y + sc) / rowH;
                return i >= 0 && i < getListItemCount(id) ? i : -1;
            };
            if (msg == WM_MOUSEWHEEL)
            {
                const int d = GET_WHEEL_DELTA_WPARAM(wp);
                setListScroll(id, sc - d * rowH * 3 / 120);
                m_flickVel[id] = 0;
                return true;
            }
            if (msg == WM_LBUTTONDOWN)
            {
                ::SetFocus(hwnd);
                const int i = rowAt(pt.y);
                if (i >= 0)
                    m_lvSel[id] = i;
                m_lvDragY[id] = pt.y;
                m_lvLastY[id] = pt.y;
                m_lvDragScroll[id] = sc;
                m_flickVel[id] = 0;
                ::InvalidateRect(hwnd, nullptr, TRUE);
                return true;
            }
            if (msg == WM_MOUSEMOVE)
            {
                if (wp & MK_LBUTTON)
                {
                    const int dy = m_lvDragY.count(id) ? (m_lvDragY[id] - pt.y) : 0;
                    setListScroll(id, (m_lvDragScroll.count(id) ? m_lvDragScroll[id] : 0) + dy);
                    // 速度采样：只取"这一帧的位移"，累积位移会把速度放大成天文数字。
                    auto lp = m_lvLastY.find(id);
                    const int step = (lp != m_lvLastY.end()) ? (lp->second - pt.y) : 0;
                    m_lvLastY[id] = pt.y;
                    m_flickVel[id] = step * 60;
                    m_flickStamp[id] = ::GetTickCount64();
                    return true;
                }
                const int i = rowAt(pt.y);
                if ((m_lvHover.count(id) ? m_lvHover[id] : -1) != i)
                {
                    m_lvHover[id] = i;
                    ::InvalidateRect(hwnd, nullptr, TRUE);
                }
                return true;
            }
            if (msg == WM_LBUTTONUP)
            {
                if ((m_flickVel.count(id) ? m_flickVel[id] : 0) != 0)
                    ::SetTimer(hwnd, kFlickTimerId, 16, nullptr);   // 松手后惯性
                return true;
            }
            return false;
        }

        if (kind == kSkinBoxGridView)
        {
            RECT cgr;
            ::GetClientRect(hwnd, &cgr);
            int cols = 1, cw = 92, chh = 96;
            gvMetrics(id, cols, cw, chh);
            const int viewW = cgr.right;
            const int viewH = cgr.bottom;
            const int cnt = getListItemCount(id);
            const int rows = (cnt + cols - 1) / cols;
            const int maxSc = (rows * chh - viewH) > 0 ? (rows * chh - viewH) : 0;
            const int sc = getListScroll(id);
            auto cellAt = [&](int x, int y) -> int {
                const int col = x / cw;
                const int row = (y + sc) / chh;
                if (col < 0 || col >= cols || row < 0)
                    return -1;
                const int i = row * cols + col;
                return i < cnt ? i : -1;
            };
            if (msg == WM_MOUSEWHEEL)
            {
                const int d = GET_WHEEL_DELTA_WPARAM(wp);
                setListScroll(id, sc - d * chh * 3 / 120);
                m_flickVel[id] = 0;
                ::InvalidateRect(hwnd, nullptr, TRUE);
                return true;
            }
            if (msg == WM_LBUTTONDOWN)
            {
                ::SetFocus(hwnd);
                const int i = cellAt(pt.x, pt.y);
                if (i >= 0)
                    m_lvSel[id] = i;
                m_lvDragY[id] = pt.y;
                m_lvLastY[id] = pt.y;
                m_lvDragScroll[id] = sc;
                m_flickVel[id] = 0;
                ::InvalidateRect(hwnd, nullptr, TRUE);
                return true;
            }
            if (msg == WM_MOUSEMOVE)
            {
                if (wp & MK_LBUTTON)
                {
                    const int dy = m_lvDragY.count(id) ? (m_lvDragY[id] - pt.y) : 0;
                    setListScroll(id, (m_lvDragScroll.count(id) ? m_lvDragScroll[id] : 0) + dy);
                    auto lp = m_lvLastY.find(id);
                    const int step = (lp != m_lvLastY.end()) ? (lp->second - pt.y) : 0;
                    m_lvLastY[id] = pt.y;
                    m_flickVel[id] = step * 60;
                    m_flickStamp[id] = ::GetTickCount64();
                    return true;
                }
                const int i = cellAt(pt.x, pt.y);
                if ((m_lvHover.count(id) ? m_lvHover[id] : -1) != i)
                {
                    m_lvHover[id] = i;
                    ::InvalidateRect(hwnd, nullptr, TRUE);
                }
                return true;
            }
            if (msg == WM_LBUTTONUP)
            {
                if ((m_flickVel.count(id) ? m_flickVel[id] : 0) != 0)
                    ::SetTimer(hwnd, kFlickTimerId, 16, nullptr);
                return true;
            }
            return false;
        }

        if (kind == kSkinBoxPathView)
        {
            RECT cpr;
            ::GetClientRect(hwnd, &cpr);
            const int cnt = getListItemCount(id);
            auto stepTo = [&](int idx) {
                if (cnt <= 0)
                    return;
                if (idx < 0) idx = 0;
                if (idx > cnt - 1) idx = cnt - 1;
                if ((m_lvSel.count(id) ? m_lvSel[id] : -1) != idx)
                {
                    m_lvSel[id] = idx;
                    ::InvalidateRect(hwnd, nullptr, TRUE);
                }
            };
            const int cur = m_lvSel.count(id) ? m_lvSel[id] : 0;
            if (msg == WM_MOUSEWHEEL)
            {
                const int d = GET_WHEEL_DELTA_WPARAM(wp);
                stepTo(cur - d / 120);
                return true;
            }
            if (msg == WM_LBUTTONDOWN)
            {
                ::SetFocus(hwnd);
                stepTo(cpr.right / 2 > pt.x ? cur - 1 : cur + 1);
                return true;
            }
            return false;
        }

        if (kind == kSkinBoxMenu)
        {
            const int row = menuRowAt(id, pt.y);
            if (msg == WM_MOUSEMOVE)
            {
                if ((m_menuHover.count(id) ? m_menuHover[id] : -1) != row)
                {
                    m_menuHover[id] = row;
                    ::InvalidateRect(hwnd, nullptr, TRUE);
                }
                return true;
            }
            if (msg == WM_LBUTTONDOWN)
            {
                if (row >= 0)
                {
                    m_menuSel[id] = row;
                    ::InvalidateRect(hwnd, nullptr, TRUE);
                    ::ShowWindow(hwnd, SW_HIDE);
                }
                return true;
            }
            return false;
        }

        if (kind == kSkinBoxMenuBar)
        {
            const int cellW = 88;
            auto indexAt = [&](int x) -> int {
                const int i = (x - rc.left - 2) / cellW;
                if (x < rc.left + 2 || i >= static_cast<int>(m_mbItems[id].size()))
                    return -1;
                return i;
            };
            if (msg == WM_LBUTTONDOWN)
            {
                ::SetFocus(hwnd);
                const int i = indexAt(pt.x);
                if (i >= 0)
                {
                    m_mbSel[id] = i;
                    ::InvalidateRect(hwnd, nullptr, TRUE);
                }
                return true;
            }
            if (msg == WM_MOUSEMOVE)
            {
                const int i = indexAt(pt.x);
                if ((m_mbHover.count(id) ? m_mbHover[id] : -1) != i)
                {
                    m_mbHover[id] = i;
                    ::InvalidateRect(hwnd, nullptr, TRUE);
                }
                return true;
            }
            return false;
        }

        if (kind == kSkinBoxToolTip)
        {
            // ToolTip 不吃点击：让底下的窗口照常收到
            return false;
        }

        if (kind == kSkinBoxPopup)
        {
            if (msg == WM_LBUTTONDOWN)
            {
                ::ShowWindow(hwnd, SW_HIDE);     // 点正文即关闭
                return true;
            }
            return msg == WM_MOUSEMOVE;
        }

        if (kind == kSkinBoxToolBar)
        {
            const int btnW = 74, step = btnW + 4;
            auto indexAt = [&](int x) -> int {
                const int i = (x - rc.left - 4) / step;
                if (x < rc.left + 4 || i >= static_cast<int>(m_tbItems[id].size()))
                    return -1;
                return i;
            };
            if (msg == WM_LBUTTONDOWN)
            {
                ::SetFocus(hwnd);
                const int i = indexAt(pt.x);
                if (i >= 0)
                {
                    m_tbSel[id] = i;
                    ::InvalidateRect(hwnd, nullptr, TRUE);
                    // 点条目要能让 Python 知道（否则工具条只是画出来的死图）。
                    // 回调带条目序号：同一个控件只有一个 onClick，靠序号区分点了哪个。
                    fireToolBarItem(id, i);
                }
                return true;
            }
            if (msg == WM_MOUSEMOVE)
            {
                const int i = indexAt(pt.x);
                if ((m_tbHover.count(id) ? m_tbHover[id] : -1) != i)
                {
                    m_tbHover[id] = i;
                    ::InvalidateRect(hwnd, nullptr, TRUE);
                }
                return true;
            }
            return false;
        }
        return false;
    }

int PyMfcHost::cwOf(int id) const
{
        HWND h = findByIdDeep(m_hWnd, id);
        if (h == nullptr)
            return 0;
        RECT rc = {};
        ::GetClientRect(h, &rc);
        const auto it = m_tblCols.find(id);
        const int n = it != m_tblCols.end() ? static_cast<int>(it->second.size()) : 0;
        return n > 0 ? (rc.right - rc.left) / n : 0;
    }

int PyMfcHost::rangeMinOf(int id) const
{
        auto it = m_boxRange.find(id);
        return it != m_boxRange.end() ? it->second.first : 0;
    }

int PyMfcHost::rangeMaxOf(int id) const
{
        auto it = m_boxRange.find(id);
        return it != m_boxRange.end() ? it->second.second : 100;
    }

int PyMfcHost::getRangeLow(int id) const
{
        auto it = m_rangeLo.find(id);
        return it != m_rangeLo.end() ? it->second : rangeMinOf(id);
    }

int PyMfcHost::getRangeHigh(int id) const
{
        auto it = m_rangeHi.find(id);
        return it != m_rangeHi.end() ? it->second : rangeMaxOf(id);
    }

void PyMfcHost::setRangeValues(int id, int low, int high)
{
        m_rangeLo[id] = low;
        m_rangeHi[id] = high;
    }

bool PyMfcHost::getSwitchValue(int id) const
{
        auto it = m_switchOn.find(id);
        return it != m_switchOn.end() && it->second;
    }

void PyMfcHost::setSwitchValue(int id, bool on)
{
        m_switchOn[id] = on;
        if (HWND h = findByIdDeep(m_hWnd, id))
            ::InvalidateRect(h, nullptr, TRUE);
    }

int PyMfcHost::getSpinValue(int id) const
{
        auto it = m_spinVal.find(id);
        return it != m_spinVal.end() ? it->second : 0;
    }

void PyMfcHost::setSpinValue(int id, int value)
{
        {
            int d = 0;
            std::string e;
            if (!m_animApplying && behaviorFor(id, L"Value", d, e))
            {
                animateFrom(id, L"Value", getSpinValue(id), value, d, e, 0, false);
                return;
            }
        }
        const int lo = m_spinLo.count(id) ? m_spinLo[id] : 0;
        const int hi = m_spinHi.count(id) ? m_spinHi[id] : 100;
        int v = value < lo ? lo : (value > hi ? hi : value);
        m_spinVal[id] = v;
        if (HWND h = findByIdDeep(m_hWnd, id))
            ::InvalidateRect(h, nullptr, TRUE);
    }

void PyMfcHost::setSpinRange(int id, int lo, int hi)
{
        m_spinLo[id] = lo;
        m_spinHi[id] = hi;
        if (m_spinVal.count(id) == 0 || m_spinVal[id] < lo || m_spinVal[id] > hi)
            m_spinVal[id] = lo;
    }

void PyMfcHost::setPageCount(int id, int count)
{ m_pageCount[id] = count; }

int PyMfcHost::getPageCount(int id) const
{
        auto it = m_pageCount.find(id);
        return it != m_pageCount.end() ? it->second : 0;
    }

void PyMfcHost::setPageIndex(int id, int index)
{
        const int count = getPageCount(id);
        if (count <= 0)
            return;
        m_pageCur[id] = index < 0 ? 0 : (index >= count ? count - 1 : index);
    }

int PyMfcHost::getPageIndex(int id) const
{
        auto it = m_pageCur.find(id);
        return it != m_pageCur.end() ? it->second : 0;
    }

int PyMfcHost::getDial(int id) const
{
        auto it = m_dialVal.find(id);
        return it != m_dialVal.end() ? it->second : 0;
    }

void PyMfcHost::setDial(int id, int value)
{
        {
            int d = 0;
            std::string e;
            if (!m_animApplying && behaviorFor(id, L"Value", d, e))
            {
                animateFrom(id, L"Value", getDial(id), value, d, e, 0, false);
                return;
            }
        }
        const int lo = rangeMinOf(id);
        const int hi = rangeMaxOf(id);
        int v = value < lo ? lo : (value > hi ? hi : value);
        m_dialVal[id] = v;
        if (HWND h = findByIdDeep(m_hWnd, id))
            ::InvalidateRect(h, nullptr, TRUE);
    }

void PyMfcHost::stepDial(int id, int delta)
{
        setDial(id, getDial(id) + delta);
    }

int PyMfcHost::getTumblerCount(int id) const
{
        auto it = m_tumblerCount.find(id);
        return it != m_tumblerCount.end() ? it->second : 10;
    }

void PyMfcHost::setTumblerCount(int id, int count)
{
        const int c = count > 0 ? count : 1;
        m_tumblerCount[id] = c;
        if (auto it = m_tumblerVal.find(id); it != m_tumblerVal.end() && it->second >= c)
            it->second = c - 1;
        if (HWND h = findByIdDeep(m_hWnd, id))
            ::InvalidateRect(h, nullptr, TRUE);
    }

int PyMfcHost::getTumblerValue(int id) const
{
        auto it = m_tumblerVal.find(id);
        return it != m_tumblerVal.end() ? it->second : 0;
    }

void PyMfcHost::setTumblerValue(int id, int index)
{
        const int c = getTumblerCount(id);
        int v = index < 0 ? 0 : (index >= c ? c - 1 : index);
        m_tumblerVal[id] = v;
        if (HWND h = findByIdDeep(m_hWnd, id))
            ::InvalidateRect(h, nullptr, TRUE);
    }

void PyMfcHost::stepTumbler(int id, int delta)
{
        if (delta == 0)
            return;
        // 循环滚动，像 QML Tumbler 那样可以绕回
        const int c = getTumblerCount(id);
        int v = getTumblerValue(id) + delta;
        v = ((v % c) + c) % c;
        m_tumblerVal[id] = v;
        if (HWND h = findByIdDeep(m_hWnd, id))
            ::InvalidateRect(h, nullptr, TRUE);
    }

void PyMfcHost::setScrollBarRange(int id, int lo, int hi, int page, bool vertical)
{
        m_boxRange[id] = std::make_pair(lo, hi);
        m_scrollPage[id] = page > 0 ? page : 1;
        m_scrollVert[id] = vertical;
        if (m_scrollVal.count(id) == 0 || m_scrollVal[id] < lo)
            m_scrollVal[id] = lo;
        if (m_scrollVal[id] > hi)
            m_scrollVal[id] = hi;
        if (HWND h = findByIdDeep(m_hWnd, id))
            ::InvalidateRect(h, nullptr, TRUE);
    }

int PyMfcHost::getScrollBarValue(int id) const
{
        auto it = m_scrollVal.find(id);
        return it != m_scrollVal.end() ? it->second : 0;
    }

void PyMfcHost::setScrollBarValue(int id, int value)
{
        setScrollValueInternal(id, value);
        if (HWND h = findByIdDeep(m_hWnd, id))
            ::InvalidateRect(h, nullptr, TRUE);
    }

void PyMfcHost::setScrollValueInternal(int id, int value)
{
        const int lo = rangeMinOf(id);
        const int hi = rangeMaxOf(id);
        m_scrollVal[id] = value < lo ? lo : (value > hi ? hi : value);
    }

void PyMfcHost::setScrollValueUser(int id, int value)
{
        const int before = m_scrollVal.count(id) != 0 ? m_scrollVal[id] : rangeMinOf(id);
        setScrollValueInternal(id, value);
        const int after = m_scrollVal.count(id) != 0 ? m_scrollVal[id] : before;
        if (after == before)
            return;                       // 夹边/同值不发事件，滚动到底不会反复回调
        auto it = m_value.find(id);
        if (it != m_value.end())
            invokePython(it->second);
        // Scroll 与 ValueChanged 同源：滚条动了也算滚动，统一用 "scroll" 一并通知
        if (m_anyCtlEvent)
            fireControlEvent(id, L"scroll");
    }

void PyMfcHost::setColumnWidth(int id, int col, int px)
{
        if (HWND h = findByIdDeep(m_hWnd, id))
        {
            wchar_t cls[64] = {};
            ::GetClassNameW(h, cls, 64);
            if (_wcsicmp(cls, L"SysListView32") == 0)
                ::SendMessageW(h, LVM_SETCOLUMNWIDTH, static_cast<WPARAM>(col),
                               MAKELPARAM(static_cast<short>(px), 0));
            ::InvalidateRect(h, nullptr, FALSE);
        }
    }

void PyMfcHost::setColumnWidths(int id, const std::vector<int>& widths)
{
    HWND h = findByIdDeep(m_hWnd, id);
    if (h == nullptr)
        return;
    wchar_t cls[64] = {};
    ::GetClassNameW(h, cls, 64);
    if (_wcsicmp(cls, L"SysListView32") == 0)
    {
        ::SendMessageW(h, WM_SETREDRAW, FALSE, 0);
        for (size_t col = 0; col < widths.size(); ++col)
            ::SendMessageW(h, LVM_SETCOLUMNWIDTH, static_cast<WPARAM>(col),
                           MAKELPARAM(static_cast<short>(widths[col]), 0));
        ::SendMessageW(h, WM_SETREDRAW, TRUE, 0);
    }
    ::InvalidateRect(h, nullptr, TRUE);
}

py::object PyMfcHost::tableMetrics(int id)
{
        HWND h = findByIdDeep(m_hWnd, id);
        if (h == nullptr || !::IsWindow(h))
            return py::make_tuple(0, 0);
        const LRESULT sp = ::SendMessageW(h, LVM_GETITEMSPACING, TRUE, 0);
        const int rowH = static_cast<int>((sp >> 16) & 0xFFFF);
        int hdr = 0;
        if (HWND ph = reinterpret_cast<HWND>(::SendMessageW(h, LVM_GETHEADER, 0, 0)))
        {
            RECT rc = { 0, 0, 1, 1 };
            ::GetWindowRect(ph, &rc);
            hdr = rc.bottom - rc.top;
        }
        return py::make_tuple(rowH > 0 ? rowH : 21, hdr > 0 ? hdr : 24);
    }

void PyMfcHost::setTableColumns(int id, const std::wstring& tabJoined)
{
        m_tblCols[id] = splitTabs(tabJoined);
        if (HWND h = findByIdDeep(m_hWnd, id))
            ::InvalidateRect(h, nullptr, TRUE);
    }

void PyMfcHost::addTableRow(int id, const std::wstring& tabJoined)
{
        std::vector<std::wstring> cells = splitTabs(tabJoined);
        m_tblRows[id].push_back(cells);
        if (HWND h = findByIdDeep(m_hWnd, id))
        {
            // addListCtrl 建的是原生 SysListView32：数据只在 m_tblRows 里是看不见的，
            // 必须真的往控件插行，否则表格只显示表头。
            if (isClassOf(h, L"SysListView32"))
            {
                const int row = static_cast<int>(m_tblRows[id].size()) - 1;
                LVITEMW it = {};
                it.mask = LVIF_TEXT;
                it.iItem = row;
                it.iSubItem = 0;
                it.pszText = const_cast<LPWSTR>(cells.empty() ? L"" : cells[0].c_str());
                ::SendMessageW(h, LVM_INSERTITEMW, 0, reinterpret_cast<LPARAM>(&it));
                for (int c = 1; c < static_cast<int>(cells.size()); ++c)
                {
                    LVITEMW sub = {};
                    sub.iSubItem = c;
                    sub.pszText = const_cast<LPWSTR>(cells[c].c_str());
                    ::SendMessageW(h, LVM_SETITEMTEXTW, static_cast<WPARAM>(row),
                                   reinterpret_cast<LPARAM>(&sub));
                }
            }
            ::InvalidateRect(h, nullptr, TRUE);
        }
    }

void PyMfcHost::setTableRows(int id, const std::wstring& blob)
{
        // 行间 '\n'、格间 '\t'。整表一次灌进去：逐行 addTableRow 时每条都在控件
        // 重绘开启状态下插行（39 行实测 22.5 ms），这里 WM_SETREDRAW(FALSE) 包住
        // 全部插入，只在最后失效一次。
        std::vector<std::vector<std::wstring>> rows;
        std::wstring cur;
        auto flushRow = [&]() {
            rows.push_back(splitTabs(cur));
            cur.clear();
        };
        for (wchar_t ch : blob)
        {
            if (ch == L'\n')
                flushRow();
            else if (ch != L'\r')
                cur.push_back(ch);
        }
        if (!cur.empty())
            flushRow();

        m_tblRows[id] = rows;
        m_tblSel[id] = -1;
        m_tblScroll[id] = 0;
        if (TableControl* nt = findNewTable(id)) { nt->SetRowsBulk(rows); return; }

        HWND h = findByIdDeep(m_hWnd, id);
        if (h == nullptr)
            return;
        wchar_t cls[64] = {};
        ::GetClassNameW(h, cls, 64);
        if (_wcsicmp(cls, L"SysListView32") == 0)
        {
            ::SendMessageW(h, WM_SETREDRAW, FALSE, 0);
            ::SendMessageW(h, LVM_DELETEALLITEMS, 0, 0);
            for (size_t r = 0; r < rows.size(); ++r)
            {
                const std::vector<std::wstring>& cells = rows[r];
                LVITEMW it = {};
                it.mask = LVIF_TEXT;
                it.iItem = static_cast<int>(r);
                it.pszText = const_cast<LPWSTR>(cells.empty() ? L"" : cells[0].c_str());
                ::SendMessageW(h, LVM_INSERTITEMW, 0, reinterpret_cast<LPARAM>(&it));
                for (size_t c = 1; c < cells.size(); ++c)
                {
                    LVITEMW sub = {};
                    sub.iSubItem = static_cast<int>(c);
                    sub.pszText = const_cast<LPWSTR>(cells[c].c_str());
                    ::SendMessageW(h, LVM_SETITEMTEXTW, static_cast<WPARAM>(r),
                                   reinterpret_cast<LPARAM>(&sub));
                }
            }
            ::SendMessageW(h, WM_SETREDRAW, TRUE, 0);
        }
        ::InvalidateRect(h, nullptr, TRUE);
}

void PyMfcHost::clearTableRows(int id)
{
        if (TableControl* nt = findNewTable(id)) { nt->Clear(); return; }
        m_tblRows[id].clear();
        m_tblSel[id] = -1;
        m_tblScroll[id] = 0;
        if (HWND h = findByIdDeep(m_hWnd, id))
        {
            wchar_t cls[64] = {};
            ::GetClassNameW(h, cls, 64);
            if (_wcsicmp(cls, L"SysListView32") == 0)
                ::SendMessageW(h, LVM_DELETEALLITEMS, 0, 0);
            ::InvalidateRect(h, nullptr, TRUE);
        }
    }

int PyMfcHost::getTableRowCount(int id) const
{
        if (findNewTable(id)) return tableRowCount(id);
        auto it = m_tblRows.find(id);
        return it != m_tblRows.end() ? static_cast<int>(it->second.size()) : 0;
    }

bool PyMfcHost::isTreeItemCollapsed(int id, int row) const
{
        auto it = m_treeCollapsed.find(id);
        if (it == m_treeCollapsed.end() || row < 0 || row >= static_cast<int>(it->second.size()))
            return false;
        return it->second[row];
    }

void PyMfcHost::setTreeItemCollapsed(int id, int row, bool collapsed)
{
        auto it = m_treeCollapsed.find(id);
        if (it == m_treeCollapsed.end() || row < 0 || row >= static_cast<int>(it->second.size()))
            return;
        it->second[row] = collapsed;
        if (HWND h = findByIdDeep(m_hWnd, id))
            ::InvalidateRect(h, nullptr, TRUE);
    }

std::wstring PyMfcHost::getTableCell(int id, int row, int col) const
{
        if (TableControl* nt = findNewTable(id)) return nt->GetText(row, col);
        auto it = m_tblRows.find(id);
        if (it == m_tblRows.end() || row < 0 || row >= static_cast<int>(it->second.size()))
            return {};
        const auto& line = it->second[row];
        if (col < 0 || col >= static_cast<int>(line.size()))
            return {};
        return line[col];
    }

void PyMfcHost::setTableCell(int id, int row, int col, const std::wstring& text)
{
        if (TableControl* nt = findNewTable(id)) { nt->SetCell(row, col, text); return; }
        auto it = m_tblRows.find(id);
        if (it == m_tblRows.end() || row < 0 || row >= static_cast<int>(it->second.size()))
            return;
        auto& line = it->second[row];
        if (col < 0 || col >= static_cast<int>(line.size()))
            return;
        line[col] = text;
        if (HWND h = findByIdDeep(m_hWnd, id))
            ::InvalidateRect(h, nullptr, TRUE);
    }

int PyMfcHost::getTableSelectedRow(int id) const
{
        if (TableControl* nt = findNewTable(id)) return nt->Selected();
        // 原生 ListView：真实选中项以控件为准（用户点行时宿主状态不会自己更新）
        if (HWND h = findByIdDeep(m_hWnd, id))
        {
            wchar_t cls[64] = {};
            ::GetClassNameW(h, cls, 64);
            if (_wcsicmp(cls, L"SysListView32") == 0)
            {
                const int r = static_cast<int>(
                    ::SendMessageW(h, LVM_GETNEXTITEM, static_cast<WPARAM>(-1), LVNI_SELECTED));
                if (r >= 0)
                    return r;
            }
        }
        auto it = m_tblSel.find(id);
        return it != m_tblSel.end() ? it->second : -1;
    }

void PyMfcHost::setTableSelectedRow(int id, int row)
{
        if (TableControl* nt = findNewTable(id)) { nt->Select(row); return; }
        auto it = m_tblRows.find(id);
        const int total = it != m_tblRows.end() ? static_cast<int>(it->second.size()) : 0;
        int v = row < 0 ? -1 : (row >= total ? total - 1 : row);
        const int prevSel = (m_tblSel.find(id) != m_tblSel.end()) ? m_tblSel[id] : -1;
        m_tblSel[id] = v;
        ensureVisible(id, v);
        if (HWND h = findByIdDeep(m_hWnd, id))
        {
            wchar_t cls[64] = {};
            ::GetClassNameW(h, cls, 64);
            if (_wcsicmp(cls, L"SysListView32") == 0)
            {
                // 只动两行：清掉旧选中、置新选中。
                // 原先是 for 循环逐项 SetItemState，39 行就触发 39 次重画，
                // 屏幕上能看到蓝色一行行扫过去 —— 这就是"选中时拖影严重"。
                if (prevSel >= 0 && prevSel < total && prevSel != v)
                {
                    LVITEMW st = {};
                    st.stateMask = LVIS_SELECTED | LVIS_FOCUSED;
                    st.state = 0;
                    ::SendMessageW(h, LVM_SETITEMSTATE, static_cast<WPARAM>(prevSel),
                                   reinterpret_cast<LPARAM>(&st));
                }
                if (v >= 0 && v < total)
                {
                    LVITEMW st = {};
                    st.stateMask = LVIS_SELECTED | LVIS_FOCUSED;
                    st.state = LVIS_SELECTED | LVIS_FOCUSED;
                    ::SendMessageW(h, LVM_SETITEMSTATE, static_cast<WPARAM>(v),
                                   reinterpret_cast<LPARAM>(&st));
                    ::SendMessageW(h, LVM_ENSUREVISIBLE, static_cast<WPARAM>(v), FALSE);
                }
            }
            // erase 参数必须 FALSE：TRUE 会先把整块表格擦白再画，一帧一次闪
            ::InvalidateRect(h, nullptr, FALSE);
        }
    }

void PyMfcHost::sortTable(int id, int col)
{
        auto cols = m_tblCols.find(id);
        auto rows = m_tblRows.find(id);
        if (cols == m_tblCols.end() || rows == m_tblRows.end())
            return;
        if (col < 0 || col >= static_cast<int>(cols->second.size()))
            return;
        bool asc = true;
        auto s = m_tblSortCol.find(id);
        if (s != m_tblSortCol.end() && s->second == col)
            asc = !(m_tblSortAsc.count(id) && m_tblSortAsc[id]);
        m_tblSortCol[id] = col;
        m_tblSortAsc[id] = asc;

        auto keyOf = [&](const std::vector<std::wstring>& line) -> std::wstring {
            return col < static_cast<int>(line.size()) ? line[col] : std::wstring();
        };
        std::stable_sort(rows->second.begin(), rows->second.end(),
                         [&](const std::vector<std::wstring>& a, const std::vector<std::wstring>& b) {
                             // 数字列按数值排，其余按字典序
                             double na = 0, nb = 0;
                             bool oka = parseNum(keyOf(a), na);
                             bool okb = parseNum(keyOf(b), nb);
                             if (oka && okb)
                                 return asc ? na < nb : na > nb;
                             return asc ? keyOf(a) < keyOf(b) : keyOf(a) > keyOf(b);
                         });
        m_tblSel[id] = -1;
        if (HWND h = findByIdDeep(m_hWnd, id))
            ::InvalidateRect(h, nullptr, TRUE);
    }

void PyMfcHost::addTreeItem(int id, const std::wstring& text, int level, bool hasKids)
{
        m_treeText[id].push_back(text);
        m_treeLevel[id].push_back(level < 0 ? 0 : level);
        m_treeKids[id].push_back(hasKids);
        m_treeCollapsed[id].push_back(false);
        if (HWND h = findByIdDeep(m_hWnd, id))
            ::InvalidateRect(h, nullptr, TRUE);
    }

void PyMfcHost::clearTreeItems(int id)
{
        m_treeText[id].clear();
        m_treeLevel[id].clear();
        m_treeKids[id].clear();
        m_treeCollapsed[id].clear();
        m_treeSel[id] = -1;
        m_treeScroll[id] = 0;
        if (HWND h = findByIdDeep(m_hWnd, id))
            ::InvalidateRect(h, nullptr, TRUE);
    }

int PyMfcHost::getTreeSelectedRow(int id) const
{
        auto it = m_treeSel.find(id);
        return it != m_treeSel.end() ? it->second : -1;
    }

void PyMfcHost::setTreeSelectedRow(int id, int row)
{
        m_treeSel[id] = row;
        if (HWND h = findByIdDeep(m_hWnd, id))
            ::InvalidateRect(h, nullptr, TRUE);
    }

bool PyMfcHost::isTreeRowHidden(int id, int row) const
{
        auto lv = m_treeLevel.find(id);
        auto col = m_treeCollapsed.find(id);
        if (lv == m_treeLevel.end() || col == m_treeCollapsed.end())
            return false;
        const std::vector<int>& levels = lv->second;
        const std::vector<bool>& collapsed = col->second;
        // 往上找最近的祖先：只要任一层折叠且本行层级更深，就隐藏
        const int myLevel = row < static_cast<int>(levels.size()) ? levels[row] : 0;
        for (int i = row - 1; i >= 0; --i)
        {
            if (levels[i] < myLevel)
            {
                if (i < static_cast<int>(collapsed.size()) && collapsed[i])
                    return true;
                if (levels[i] == myLevel - 1)
                    break;
            }
        }
        return false;
    }

void PyMfcHost::toggleTreeRow(int id, int row)
{
        auto col = m_treeCollapsed.find(id);
        if (col == m_treeCollapsed.end() || row < 0 || row >= static_cast<int>(col->second.size()))
            return;
        col->second[row] = !col->second[row];
        if (HWND h = findByIdDeep(m_hWnd, id))
            ::InvalidateRect(h, nullptr, TRUE);
    }

void PyMfcHost::addToolBarItem(int id, const std::wstring& text)
{
        m_tbItems[id].push_back(text);
        if (HWND h = findByIdDeep(m_hWnd, id))
            ::InvalidateRect(h, nullptr, TRUE);
    }

void PyMfcHost::clearToolBarItems(int id)
{
        m_tbItems[id].clear();
        m_tbSel[id] = -1;
        if (HWND h = findByIdDeep(m_hWnd, id))
            ::InvalidateRect(h, nullptr, TRUE);
    }

int PyMfcHost::getToolBarSelected(int id) const
{
        auto it = m_tbSel.find(id);
        return it != m_tbSel.end() ? it->second : -1;
    }

void PyMfcHost::setToolBarSelected(int id, int index)
{
        m_tbSel[id] = index;
        if (HWND h = findByIdDeep(m_hWnd, id))
            ::InvalidateRect(h, nullptr, TRUE);
    }

std::vector<std::wstring> PyMfcHost::splitTabs(const std::wstring& s)
{
        std::vector<std::wstring> out;
        std::wstring cur;
        for (wchar_t ch : s)
        {
            if (ch == L'\t' || ch == L'|')
            {
                out.push_back(cur);
                cur.clear();
            }
            else
            {
                cur.push_back(ch);
            }
        }
        out.push_back(cur);
        return out;
    }

bool PyMfcHost::parseNum(const std::wstring& s, double& out)
{
        if (s.empty())
            return false;
        wchar_t end = 0;
        out = wcstod(s.c_str(), nullptr);
        (void)end;
        return out != 0.0 || s == L"0";
    }

void PyMfcHost::ensureVisible(int id, int row)
{
        if (row < 0)
            return;
        HWND h = findByIdDeep(m_hWnd, id);
        if (h == nullptr)
            return;
        RECT rc = {};
        ::GetClientRect(h, &rc);
        auto kind = m_skinKinds.find(id);
        const int headH = (kind != m_skinKinds.end() && kind->second == kSkinBoxTable) ? 26 : 0;
        const int visible = (rc.bottom - rc.top - headH) / 24;
        int first = m_tblScroll.count(id) ? m_tblScroll[id] : 0;
        if (kind != m_skinKinds.end() && kind->second == kSkinBoxTree)
            first = m_treeScroll.count(id) ? m_treeScroll[id] : 0;
        if (row < first)
            first = row;
        else if (row >= first + visible)
            first = row - visible + 1;
        if (first < 0)
            first = 0;
        if (kind != m_skinKinds.end() && kind->second == kSkinBoxTree)
            m_treeScroll[id] = first;
        else
            m_tblScroll[id] = first;
    }

void PyMfcHost::addTabItem(int id, const std::wstring& text)
{
        HWND h = findByIdDeep(m_hWnd, id);
        if (h == nullptr || !isClassOf(h, L"SysTabControl32"))
            return;
        TCITEMW item = {};
        item.mask = TCIF_TEXT;
        item.pszText = const_cast<LPWSTR>(text.c_str());
        const int index = TabCtrl_GetItemCount(h);
        ::SendMessageW(h, TCM_INSERTITEMW, static_cast<WPARAM>(index),
                       reinterpret_cast<LPARAM>(&item));
        // 只选中页码、不真的切换可见页，避免页签看起来"没反应"
        if (index == 0)
            ::SendMessageW(h, TCM_SETCURFOCUS, 0, 0);
        ::InvalidateRect(h, nullptr, TRUE);
        ::UpdateWindow(h);
    }

void PyMfcHost::addTreeNode(int id, const std::wstring& text, int parentIndex)
{
        HWND h = findByIdDeep(m_hWnd, id);
        if (h == nullptr || !isClassOf(h, L"SysTreeView32"))
            return;
        HTREEITEM parent = TVI_ROOT;
        if (parentIndex >= 0)
        {
            auto it = m_treeNodes.find(id);
            if (it != m_treeNodes.end() && parentIndex < static_cast<int>(it->second.size()))
                parent = it->second[static_cast<size_t>(parentIndex)];
            else
                return;  // 父节点不存在时宁可不插，也不插成根节点骗人
        }
        TVINSERTSTRUCTW ins = {};
        ins.hParent = parent;
        ins.hInsertAfter = TVI_LAST;
        ins.item.mask = TVIF_TEXT;
        ins.item.pszText = const_cast<LPWSTR>(text.c_str());
        const HTREEITEM node = reinterpret_cast<HTREEITEM>(
            ::SendMessageW(h, TVM_INSERTITEMW, 0, reinterpret_cast<LPARAM>(&ins)));
        if (node != nullptr)
            m_treeNodes[id].push_back(node);
        // 父节点必须先展开，否则子节点插在收起的分支里，界面上只看得到根 —— 又变成"空壳"
        if (parent == TVI_ROOT)
            ::SendMessageW(h, TVM_EXPAND, TVE_EXPAND, reinterpret_cast<LPARAM>(node));
        else
            ::SendMessageW(h, TVM_EXPAND, TVE_EXPAND, reinterpret_cast<LPARAM>(parent));
        ::InvalidateRect(h, nullptr, TRUE);
        ::UpdateWindow(h);
    }

void PyMfcHost::addColumn(int id, const std::wstring& text, int width)
{
        HWND h = findByIdDeep(m_hWnd, id);
        if (h == nullptr || !isClassOf(h, L"SysListView32"))
            return;
        HWND header = reinterpret_cast<HWND>(::SendMessageW(h, LVM_GETHEADER, 0, 0));
        const int index = header != nullptr
            ? static_cast<int>(::SendMessageW(header, HDM_GETITEMCOUNT, 0, 0))
            : 0;
        LVCOLUMNW col = {};
        col.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
        col.cx = width > 0 ? width : 120;
        col.pszText = const_cast<LPWSTR>(text.c_str());
        col.iSubItem = index;
        ::SendMessageW(h, LVM_INSERTCOLUMNW, static_cast<WPARAM>(index),
                       reinterpret_cast<LPARAM>(&col));
        ::InvalidateRect(h, nullptr, TRUE);
        ::UpdateWindow(h);
    }

void PyMfcHost::addRow(int id, const std::wstring& text)
{
        HWND h = findByIdDeep(m_hWnd, id);
        if (h == nullptr || !isClassOf(h, L"SysListView32"))
            return;
        // 一个字符串里用 \t 分隔多列：表格控件要能一次填满一行，而不是只有第一列
        const int row = ListView_GetItemCount(h);
        const std::wstring firstCell = text.substr(0, text.find(L'\t'));
        LVITEMW item = {};
        item.mask = LVIF_TEXT;
        item.iItem = row;
        item.iSubItem = 0;
        item.pszText = const_cast<LPWSTR>(firstCell.c_str());
        ::SendMessageW(h, LVM_INSERTITEMW, 0, reinterpret_cast<LPARAM>(&item));

        size_t start = text.find(L'\t');
        int sub = 1;
        while (start != std::wstring::npos)
        {
            const size_t next = text.find(L'\t', start + 1);
            const std::wstring cell = (next == std::wstring::npos)
                ? text.substr(start + 1)
                : text.substr(start + 1, next - start - 1);
            LVITEMW si = {};
            si.mask = LVIF_TEXT;
            si.iItem = row;
            si.iSubItem = sub;
            si.pszText = const_cast<LPWSTR>(cell.c_str());
            ::SendMessageW(h, LVM_SETITEMTEXTW, static_cast<WPARAM>(row),
                           reinterpret_cast<LPARAM>(&si));
            ++sub;
            start = next;
        }
        ::InvalidateRect(h, nullptr, TRUE);
        ::UpdateWindow(h);
    }

void PyMfcHost::setText(int id, const std::wstring& text)
{
        // 超链接的窗口文本必须是链接标记，直接 SetWindowText 会把标记冲掉，
        // 所以显示文字单独记着，由 applyLinkText 组回 <a href="..."> 形式
        if (m_urls.count(id) != 0)
        {
            m_links[id] = text;
            applyLinkText(id);
            return;
        }
        if (HWND h = findByIdDeep(m_hWnd, id))
            ::SetWindowTextW(h, text.c_str());
    }

void PyMfcHost::applyLinkText(int id)
{
        HWND h = findByIdDeep(m_hWnd, id);
        auto u = m_urls.find(id);
        if (h == nullptr || u == m_urls.end())
            return;
        std::wstring label = m_links.count(id) != 0 ? m_links[id] : std::wstring();
        if (label.empty() || label.find(L"<") != std::wstring::npos)
            label = u->second;
        const std::wstring markup =
            L"<a href=\"" + u->second + L"\">" + label + L"</a>";
        ::SetWindowTextW(h, markup.c_str());
    }

std::wstring PyMfcHost::getText(int id) const
{
        HWND h = findByIdDeep(m_hWnd, id);
        if (!h)
            return {};
        const int len = ::GetWindowTextLengthW(h);
        std::wstring text(static_cast<size_t>(len) + 1, L'\0');
        ::GetWindowTextW(h, text.data(), len + 1);
        text.resize(static_cast<size_t>(len));
        return text;
    }

void PyMfcHost::showToolTip(int ownerId, const std::wstring& text, int durationMs)
{
        const int tipId = 9001;
        ensureOverlay(tipId, kSkinBoxToolTip);
        m_tipText[tipId] = text;
        int w = 28;
        for (wchar_t ch : text)
            w += (ch < 128 ? 8 : 14);
        if (w > 320) w = 320;
        const int hgt = 28;
        HWND owner = ::GetDlgItem(m_hWnd, ownerId);
        int x = 8, y = 8;
        if (owner != nullptr)
        {
            RECT r = {};
            ::GetWindowRect(owner, &r);
            POINT p = {r.right, r.top};
            ::MapWindowPoints(HWND_DESKTOP, m_hWnd, &p, 1);
            x = p.x + 4;
            y = p.y;
        }
        placeOverlay(tipId, x, y, w, hgt);
        if (HWND h = ::GetDlgItem(m_hWnd, tipId))
        {
            ::KillTimer(h, kTipTimerId);
            if (durationMs > 0)
                ::SetTimer(h, kTipTimerId, static_cast<UINT>(durationMs), nullptr);
        }
    }

void PyMfcHost::hideToolTip()
{
        hideOverlay(9001);
    }

void PyMfcHost::showMenu(int id, const std::vector<std::wstring>& items, int x, int y)
{
        ensureOverlay(id, kSkinBoxMenu);
        m_menuItems[id] = items;
        m_menuHover[id] = -1;
        m_menuSel[id] = -1;
        int w = 120;
        for (const auto& s : items)
        {
            int cw = 24;
            for (wchar_t ch : s)
                cw += (ch < 128 ? 8 : 14);
            if (cw > w) w = cw;
        }
        int hgt = 8;
        for (const auto& s : items)
            hgt += (s == L"-") ? 8 : 26;
        placeOverlay(id, x, y, w, hgt + 4);
    }

void PyMfcHost::hideMenu(int id)
{
        hideOverlay(id);
    }

int PyMfcHost::menuRowAt(int id, int y) const
{
        auto it = m_menuItems.find(id);
        if (it == m_menuItems.end())
            return -1;
        int cy = 4;
        for (int i = 0; i < static_cast<int>(it->second.size()); ++i)
        {
            const int rowH = (it->second[i] == L"-") ? 8 : 26;
            if (y >= cy && y < cy + rowH)
                return it->second[i] == L"-" ? -1 : i;
            cy += rowH;
        }
        return -1;
    }

int PyMfcHost::getMenuSelected(int id) const
{
        auto it = m_menuSel.find(id);
        return it != m_menuSel.end() ? it->second : -1;
    }

void PyMfcHost::setMenuItems(int id, const std::vector<std::wstring>& items)
{
        m_menuItems[id] = items;
        m_menuHover[id] = -1;
        m_menuSel[id] = -1;
        ::InvalidateRect(findByIdDeep(m_hWnd, id), nullptr, TRUE);
    }

void PyMfcHost::addMenuBarItem(int id, const std::wstring& text)
{
        m_mbItems[id].push_back(text);
    }

void PyMfcHost::clearMenuBarItems(int id)
{
        m_mbItems[id].clear();
        m_mbSel[id] = -1;
        ::InvalidateRect(findByIdDeep(m_hWnd, id), nullptr, TRUE);
    }

int PyMfcHost::getMenuBarSelected(int id) const
{
        auto it = m_mbSel.find(id);
        return it != m_mbSel.end() ? it->second : -1;
    }

void PyMfcHost::setMenuBarSelected(int id, int i)
{
        m_mbSel[id] = i;
        ::InvalidateRect(findByIdDeep(m_hWnd, id), nullptr, TRUE);
    }

void PyMfcHost::showPopup(int id, const std::wstring& title, const std::vector<std::wstring>& lines,
                   int x, int y, int w, int hgt)
{
        ensureOverlay(id, kSkinBoxPopup);
        m_popTitle[id] = title;
        m_popLines[id] = lines;
        placeOverlay(id, x, y, w, hgt);
    }

void PyMfcHost::hidePopup(int id)
{
        hideOverlay(id);
    }

bool PyMfcHost::isPopupVisible(int id) const
{
        return isOverlayVisible(id);
    }

void PyMfcHost::gvMetrics(int id, int& cols, int& cw, int& ch) const
{
        RECT rc = {};
        HWND h = findByIdDeep(m_hWnd, id);
        if (h != nullptr)
            ::GetClientRect(h, &rc);
        const int viewW = (rc.right - rc.left) > 8 ? (rc.right - rc.left) : 92;
        auto iw = m_gvCellW.find(id);
        const int wantW = (iw != m_gvCellW.end() && iw->second > 24) ? iw->second : 92;
        cols = viewW / wantW;
        if (cols < 1) cols = 1;
        cw = (iw != m_gvCellW.end() && iw->second > 24) ? wantW : (viewW / cols);
        auto ih = m_gvCellH.find(id);
        ch = (ih != m_gvCellH.end() && ih->second > 24) ? ih->second : (cw * 96 / 92);
        if (ch < 24) ch = 24;
    }

int PyMfcHost::lvRowH(int id) const
{
        auto it = m_lvItemH.find(id);
        if (it != m_lvItemH.end() && it->second > 8)
            return it->second;
        const auto k = m_skinKinds.find(id);
        if (k != m_skinKinds.end() && k->second == kSkinBoxGridView)
        {
            int cols = 1, cw = 92, ch = 96;
            gvMetrics(id, cols, cw, ch);
            return ch;
        }
        return 44;
    }

int PyMfcHost::lvMaxScroll(int id) const
{
        auto it = m_lvT.find(id);
        const int cnt = it != m_lvT.end() ? static_cast<int>(it->second.size()) : 0;
        RECT rc = {};
        HWND h = findByIdDeep(m_hWnd, id);
        if (h != nullptr)
            ::GetClientRect(h, &rc);
        const int viewH = rc.bottom - rc.top;
        int rows = cnt;
        const auto k = m_skinKinds.find(id);
        if (k != m_skinKinds.end() && k->second == kSkinBoxGridView)
        {
            int cols = 1, cw = 92, ch = 96;
            gvMetrics(id, cols, cw, ch);
            rows = (cnt + cols - 1) / cols;
            const int need2 = rows * ch - viewH;
            return need2 > 0 ? need2 : 0;
        }
        const int need = rows * lvRowH(id) - viewH;
        return need > 0 ? need : 0;
    }

void PyMfcHost::lvClampScroll(int id)
{
        const int max = lvMaxScroll(id);
        int v = m_lvScroll.count(id) ? m_lvScroll[id] : 0;
        if (v > max) v = max;
        if (v < 0) v = 0;
        m_lvScroll[id] = v;
    }

void PyMfcHost::getView3DCamera(int id, float& yaw, float& pitch, float& zoom,
                                int& panX, int& panY) const
{
    auto it = m_view3D.find(id);
    if (it == m_view3D.end())
    {
        yaw = 35.0f; pitch = 22.0f; zoom = 1.0f; panX = panY = 0;
        return;
    }
    const View3DState& st = it->second;
    yaw = st.yaw; pitch = st.pitch; zoom = st.zoom; panX = st.panX; panY = st.panY;
}

void PyMfcHost::setGridCellSize(int id, int cellW, int cellH)
{
        if (cellW > 24) m_gvCellW[id] = cellW; else m_gvCellW.erase(id);
        if (cellH > 24) m_gvCellH[id] = cellH; else m_gvCellH.erase(id);
        HWND gh = findByIdDeep(m_hWnd, id);
        if (gh != nullptr)
            ::InvalidateRect(gh, nullptr, TRUE);
    }

void PyMfcHost::setCanvasCommands(int id, const std::wstring& cmds)
{
        m_cvCmd[id] = cmds;
        repaintCanvas(id);
    }

std::wstring PyMfcHost::getCanvasCommands(int id) const
{
        auto it = m_cvCmd.find(id);
        return it != m_cvCmd.end() ? it->second : std::wstring();
    }

void PyMfcHost::setCanvasEditable(int id, bool on)
{
        if (on)
            m_cvEdit[id] = true;
        else
            m_cvEdit.erase(id);
        repaintCanvas(id);
    }

bool PyMfcHost::getCanvasEditable(int id) const
{
        return m_cvEdit.count(id) != 0;
    }

int PyMfcHost::canvasItemCount(int id) const
{
        auto it = m_cvItems.find(id);
        return it != m_cvItems.end() ? static_cast<int>(it->second.size()) : 0;
    }

void PyMfcHost::canvasItem(int id, int index, int& outOp, RECT& outBox) const
{
        outOp = 0;
        outBox = RECT{0, 0, 0, 0};
        auto it = m_cvItems.find(id);
        if (it == m_cvItems.end() || index < 0 || index >= static_cast<int>(it->second.size()))
            return;
        outBox = it->second[index].box;
        outOp = 1;   // 只表示"存在"，命令名由 getCanvasItemOp 取
    }

std::wstring PyMfcHost::canvasItemOp(int id, int index) const
{
        auto it = m_cvItems.find(id);
        if (it == m_cvItems.end() || index < 0 || index >= static_cast<int>(it->second.size()))
            return std::wstring();
        return it->second[index].f.empty() ? std::wstring() : it->second[index].f[0];
    }

int PyMfcHost::canvasHitTest(int id, POINT pt) const
{
        auto it = m_cvItems.find(id);
        if (it == m_cvItems.end())
            return -1;
        const std::vector<pymfcskin::QcPiece>& v = it->second;
        for (int i = static_cast<int>(v.size()) - 1; i >= 0; --i)
        {
            if (!v[i].selectable)
                continue;
            RECT b = v[i].box;
            ::InflateRect(&b, 2, 2);
            if (pt.x >= b.left && pt.x < b.right && pt.y >= b.top && pt.y < b.bottom)
                return i;
        }
        return -1;
    }

std::vector<int> PyMfcHost::getCanvasSelection(int id) const
{
        auto it = m_cvSel.find(id);
        return it != m_cvSel.end() ? it->second : std::vector<int>();
    }

void PyMfcHost::setCanvasSelection(int id, const std::vector<int>& idx)
{
        std::vector<int> v;
        const int n = canvasItemCount(id);
        for (int i : idx)
        {
            if (i >= 0 && i < n && m_cvItems[id][i].selectable)
                v.push_back(i);
        }
        m_cvSel[id] = v;
        repaintCanvas(id);
        fireCanvasSelect(id);
    }

void PyMfcHost::clearCanvasSelection(int id)
{
        m_cvSel[id].clear();
        repaintCanvas(id);
        fireCanvasSelect(id);
    }

int PyMfcHost::removeCanvasItems(int id, const std::vector<int>& idx)
{
        auto it = m_cvCmd.find(id);
        if (it == m_cvCmd.end())
            return 0;
        std::vector<bool> drop;
        if (m_cvItems.count(id))
        {
            drop.assign(m_cvItems[id].size(), false);
            for (int i : idx)
            {
                if (i >= 0 && i < static_cast<int>(drop.size()))
                    drop[i] = true;
            }
        }
        std::vector<int> keep;
        auto itemsIt = m_cvItems.find(id);
        const int n = itemsIt != m_cvItems.end() ? static_cast<int>(itemsIt->second.size()) : 0;
        for (int i = 0; i < n; ++i)
        {
            if (i < static_cast<int>(drop.size()) && drop[i])
                continue;
            keep.push_back(i);
        }

        // 用"解析后的片段顺序"重建：只保留解析器认得出来的片段（与绘制一致）
        std::wstring out;
        if (itemsIt != m_cvItems.end())
        {
            for (int i : keep)
            {
                if (!out.empty())
                    out += L"; ";
                out += itemsIt->second[i].raw;
            }
        }
        m_cvCmd[id] = out;
        m_cvSel[id].clear();
        const int removed = n - static_cast<int>(keep.size());
        repaintCanvas(id);
        fireCanvasSelect(id);
        return removed;
    }

int PyMfcHost::removeCanvasSelection(int id)
{
        const std::vector<int> sel = getCanvasSelection(id);
        return sel.empty() ? 0 : removeCanvasItems(id, sel);
    }

void PyMfcHost::addListItem(int id, const std::wstring& line)
{
        std::vector<std::wstring> f(3);
        int k = 0;
        for (wchar_t ch : line)
        {
            if (ch == L'|' || ch == L'\t')
            {
                if (k < 2) ++k;
                continue;
            }
            f[k].push_back(ch);
        }
        m_lvT[id].push_back(f[0]);
        m_lvS[id].push_back(f[1]);
        m_lvV[id].push_back(f[2]);
        ::InvalidateRect(findByIdDeep(m_hWnd, id), nullptr, TRUE);
    }

void PyMfcHost::clearListItems(int id)
{
        m_lvT[id].clear();
        m_lvS[id].clear();
        m_lvV[id].clear();
        m_lvSel[id] = -1;
        m_lvScroll[id] = 0;
        m_lvOver[id] = 0;
        ::InvalidateRect(findByIdDeep(m_hWnd, id), nullptr, TRUE);
    }

int PyMfcHost::getListItemCount(int id) const
{
        auto it = m_lvT.find(id);
        return it != m_lvT.end() ? static_cast<int>(it->second.size()) : 0;
    }

std::wstring PyMfcHost::getListItem(int id, int index) const
{
        auto it = m_lvT.find(id);
        if (it == m_lvT.end() || index < 0 || index >= static_cast<int>(it->second.size()))
            return {};
        std::wstring s = it->second[index];
        auto s2 = m_lvS.find(id);
        if (s2 != m_lvS.end() && index < static_cast<int>(s2->second.size()) && !s2->second[index].empty())
            s += L"|" + s2->second[index];
        auto v2 = m_lvV.find(id);
        if (v2 != m_lvV.end() && index < static_cast<int>(v2->second.size()) && !v2->second[index].empty())
            s += L"|" + v2->second[index];
        return s;
    }

int PyMfcHost::getListSelected(int id) const
{
        auto it = m_lvSel.find(id);
        return it != m_lvSel.end() ? it->second : -1;
    }

void PyMfcHost::setListSelected(int id, int index)
{
        // 序号一律夹到合法区间：越界值会让绘制层按错误下标取数据
        const int cnt = getListItemCount(id);
        if (cnt > 0)
        {
            if (index < 0) index = 0;
            if (index > cnt - 1) index = cnt - 1;
        }
        else
        {
            index = -1;
        }
        m_lvSel[id] = index;
        if (index < 0)
            return;
        // 选中项要保证在可见窗口里：滚动过去
        int rowH = lvRowH(id);
        int rowIdx = index;
        const auto kg = m_skinKinds.find(id);
        if (kg != m_skinKinds.end() && kg->second == kSkinBoxGridView)
        {
            int cols = 1, cw = 92, ch = 96;
            gvMetrics(id, cols, cw, ch);
            rowIdx = index / cols;
            rowH = ch;
        }
        const int top = rowIdx * rowH;
        const int max = lvMaxScroll(id);
        int sc = m_lvScroll.count(id) ? m_lvScroll[id] : 0;
        if (top < sc)
            sc = top;
        else if (top + rowH > sc + max + rowH)
            sc = top - max;
        if (sc > max) sc = max;
        if (sc < 0) sc = 0;
        m_lvScroll[id] = sc;
        ::InvalidateRect(findByIdDeep(m_hWnd, id), nullptr, TRUE);
    }

int PyMfcHost::getListScroll(int id) const
{
        auto it = m_lvScroll.find(id);
        return it != m_lvScroll.end() ? it->second : 0;
    }

void PyMfcHost::setListScroll(int id, int pos)
{
        m_lvScroll[id] = pos;
        lvClampScroll(id);
        ::InvalidateRect(findByIdDeep(m_hWnd, id), nullptr, TRUE);
    }

void PyMfcHost::setFlickContent(int id, const std::vector<int>& kids, int cw, int ch)
{
        HWND hfl = findByIdDeep(m_hWnd, id);
        if (hfl == nullptr)
            return;
        m_flKids[id] = kids;
        m_flBase[id].clear();
        for (int kid : kids)
        {
            HWND hk = ::GetDlgItem(m_hWnd, kid);
            if (hk == nullptr)
                continue;
            RECT cr = {};
            ::GetWindowRect(hk, &cr);
            POINT tl = {cr.left, cr.top};
            POINT br2 = {cr.right, cr.bottom};
            ::MapWindowPoints(HWND_DESKTOP, hfl, &tl, 1);
            ::MapWindowPoints(HWND_DESKTOP, hfl, &br2, 1);
            RECT base = {tl.x, tl.y, br2.x, br2.y};
            m_flBase[id][kid] = base;
            ::SetParent(hk, hfl);            // 变成视口子窗 → 超出视口的部分自然被裁掉
            ::SetWindowPos(hk, HWND_TOP, base.left, base.top,
                           base.right - base.left, base.bottom - base.top,
                           SWP_NOACTIVATE);
        }
        m_flCW[id] = cw;
        m_flCH[id] = ch;
        m_flOff[id] = POINT{0, 0};
        m_flVel[id] = POINT{0, 0};
        flickApply(id);
        ::InvalidateRect(hfl, nullptr, TRUE);
    }

void PyMfcHost::clearFlickContent(int id)
{
        HWND hfl = findByIdDeep(m_hWnd, id);
        auto it = m_flBase.find(id);
        if (it != m_flBase.end() && hfl != nullptr)
            for (const auto& kv : it->second)
                if (HWND hk = ::GetDlgItem(hfl, kv.first))
                {
                    RECT r = kv.second;
                    ::MapWindowPoints(hfl, m_hWnd, reinterpret_cast<LPPOINT>(&r), 2);
                    ::SetParent(hk, m_hWnd);
                    ::SetWindowPos(hk, nullptr, r.left, r.top, r.right - r.left,
                                   r.bottom - r.top, SWP_NOZORDER | SWP_NOACTIVATE);
                    ::ShowWindow(hk, SW_SHOW);
                }
        m_flKids.erase(id);
        m_flBase.erase(id);
        m_flOff.erase(id);
        m_flVel.erase(id);
    }

void PyMfcHost::setFlickOffset(int id, int x, int y)
{
        int vw, vh, cw, ch, minX, minY;
        flickMetrics(id, vw, vh, cw, ch, minX, minY);
        if (x > 0) x = 0;
        if (x < minX) x = minX;
        if (y > 0) y = 0;
        if (y < minY) y = minY;
        m_flOff[id] = POINT{x, y};
        m_flVel[id] = POINT{0, 0};
        flickApply(id);
        ::InvalidateRect(findByIdDeep(m_hWnd, id), nullptr, TRUE);
    }

int PyMfcHost::getFlickOffsetX(int id) const
{
        auto it = m_flOff.find(id);
        return it != m_flOff.end() ? it->second.x : 0;
    }

int PyMfcHost::getFlickOffsetY(int id) const
{
        auto it = m_flOff.find(id);
        return it != m_flOff.end() ? it->second.y : 0;
    }

void PyMfcHost::pageRegister(int id)
{
        if (!m_pgIdx.count(id))
            m_pgIdx[id] = 0;
    }

void PyMfcHost::clearPages(int id)
{
        // 先把所有页的子控件还原成原样（可见、位置回到基准）
        auto it = m_pgPages.find(id);
        if (it != m_pgPages.end())
            for (auto& pg : it->second)
                for (int kid : pg)
                {
                    HWND hk = ::GetDlgItem(m_hWnd, kid);
                    if (hk != nullptr)
                    {
                        auto b = m_pgBase[id].find(kid);
                        if (b != m_pgBase[id].end())
                            ::SetWindowPos(hk, nullptr, b->second.left, b->second.top,
                                           b->second.right - b->second.left,
                                           b->second.bottom - b->second.top,
                                           SWP_NOZORDER | SWP_NOACTIVATE);
                        ::ShowWindow(hk, SW_SHOW);
                    }
                }
        if (it != m_pgPages.end())
            it->second.clear();
        m_pgIdx[id] = 0;
        m_pgAnimDur.erase(id);
        ::InvalidateRect(findByIdDeep(m_hWnd, id), nullptr, TRUE);
    }

RECT PyMfcHost::pageHostRect(int id) const
{
        RECT rc = { 0, 0, 0, 0 };
        HWND hh = findByIdDeep(m_hWnd, id);
        if (hh != nullptr)
        {
            ::GetWindowRect(hh, &rc);
            ::MapWindowPoints(nullptr, m_hWnd, reinterpret_cast<LPPOINT>(&rc), 2);
        }
        return rc;
    }

void PyMfcHost::pageLayout(int id, int which, int offset, bool show)
{
        auto it = m_pgPages.find(id);
        if (it == m_pgPages.end() || which < 0 || which >= static_cast<int>(it->second.size()))
            return;
        const RECT hr = pageHostRect(id);
        // 页内容必须压在页容器上面。z 序用 HWND_TOP：每次插入都落到兄弟链最顶，
        // 所以按"视觉自底向上"的顺序摆（先插的在下、后插的在上）：
        //   Level 小的先插（Level 大的压在上面）；同级按声明顺序，后声明的压在上面。
        // 早先这里用 SetWindowPos(hk, 页容器, …)，而 Win32 的 insert-after 语义是
        // "排在它后面"——结果是页内容被插到容器下面，容器的opaque底板把整页盖成一片空。
        const auto& kids = it->second[which];
        std::vector<int> order(kids.begin(), kids.end());
        std::sort(order.begin(), order.end(), [&](int a, int b) {
            const int la = levelOf(a);
            const int lb = levelOf(b);
            if (la != lb)
                return la < lb;      // Level 小的先插（先插 = 更靠下）
            return a < b;            // 同级按声明顺序：后声明的后插、压在上面
        });
        for (int kid : order)
        {
            HWND hk = ::GetDlgItem(m_hWnd, kid);
            if (hk == nullptr)
                continue;
            auto b = m_pgBase[id].find(kid);
            if (b == m_pgBase[id].end())
            {
                RECT cr = { 0, 0, 0, 0 };
                ::GetWindowRect(hk, &cr);
                ::MapWindowPoints(nullptr, m_hWnd, reinterpret_cast<LPPOINT>(&cr), 2);
                m_pgBase[id][kid] = cr;
                b = m_pgBase[id].find(kid);
            }
            // 页内容必须压在页容器上面：容器先建，落在 z 序顶端会把整页内容盖住。
            // 位置 = 基准矩形相对页容器的偏移 + 滑动偏移，尺寸保持 XML 里声明的尺寸：
            // 早先这里把子控件拉成"整页大小"，切走再切回就再也回不到初始样子。
            const RECT& base = b->second;
            const int bx = base.left - hr.left;
            const int by = base.top - hr.top;
            ::SetWindowPos(hk, HWND_TOP,
                           hr.left + bx + offset, hr.top + by,
                           base.right - base.left, base.bottom - base.top,
                           SWP_NOACTIVATE);
            ::ShowWindow(hk, show ? SW_SHOW : SW_HIDE);
            if (show)
                ::InvalidateRect(hk, nullptr, TRUE);
        }
    }

void PyMfcHost::addPage(int id, const std::vector<int>& kids)
{
        // 记录基准坐标（以后切页会覆盖成容器矩形 + 偏移）
        for (int kid : kids)
        {
            HWND hk = ::GetDlgItem(m_hWnd, kid);
            if (hk == nullptr)
                continue;
            RECT cr = { 0, 0, 0, 0 };
            ::GetWindowRect(hk, &cr);
            ::MapWindowPoints(nullptr, m_hWnd, reinterpret_cast<LPPOINT>(&cr), 2);
            m_pgBase[id][kid] = cr;
        }
        const int idx = static_cast<int>(m_pgPages[id].size());
        m_pgPages[id].push_back(kids);
        const int cur = m_pgIdx.count(id) ? m_pgIdx[id] : 0;
        pageLayout(id, idx, 0, idx == cur);
        ::InvalidateRect(findByIdDeep(m_hWnd, id), nullptr, TRUE);
    }

int PyMfcHost::viewCount(int id) const
{
        auto it = m_pgPages.find(id);
        return it != m_pgPages.end() ? static_cast<int>(it->second.size()) : 0;
    }

int PyMfcHost::viewIndex(int id) const
{
        auto it = m_pgIdx.find(id);
        return it != m_pgIdx.end() ? it->second : 0;
    }

void PyMfcHost::viewSetIndex(int id, int index)
{
        const int cnt = viewCount(id);
        if (cnt <= 0)
        {
            m_pgIdx[id] = index;
            return;
        }
        if (index < 0) index = 0;
        if (index >= cnt) index = cnt - 1;
        const int old = viewIndex(id);
        m_pgIdx[id] = index;
        m_pgAnimDur.erase(id);
        ::KillTimer(findByIdDeep(m_hWnd, id), kPageTimerId);
        pageLayout(id, old, 0, false);
        pageLayout(id, index, 0, true);
        ::InvalidateRect(findByIdDeep(m_hWnd, id), nullptr, TRUE);
    }

void PyMfcHost::addString(int id, const std::wstring& text)
{
        if (HWND h = findByIdDeep(m_hWnd, id))
            ::SendMessageW(h, isComboBox(h) ? CB_ADDSTRING : LB_ADDSTRING, 0,
                           reinterpret_cast<LPARAM>(text.c_str()));
    }

void PyMfcHost::clearStrings(int id)
{
        if (HWND h = findByIdDeep(m_hWnd, id))
            ::SendMessageW(h, isComboBox(h) ? CB_RESETCONTENT : LB_RESETCONTENT, 0, 0);
    }

int PyMfcHost::countStrings(int id) const
{
        HWND h = findByIdDeep(m_hWnd, id);
        return h ? static_cast<int>(::SendMessageW(h, isComboBox(h) ? CB_GETCOUNT : LB_GETCOUNT, 0, 0)) : 0;
    }

std::wstring PyMfcHost::getString(int id, int index) const
{
        HWND h = findByIdDeep(m_hWnd, id);
        if (h == nullptr || index < 0)
            return {};
        const UINT msg = isComboBox(h) ? CB_GETLBTEXT : LB_GETTEXT;
        const int len = static_cast<int>(::SendMessageW(h, isComboBox(h) ? CB_GETLBTEXTLEN : LB_GETTEXTLEN,
                                                        static_cast<WPARAM>(index), 0));
        if (len <= 0)
            return {};
        std::wstring text(static_cast<size_t>(len) + 1, L'\0');
        ::SendMessageW(h, msg, static_cast<WPARAM>(index), reinterpret_cast<LPARAM>(text.data()));
        text.resize(static_cast<size_t>(len));
        return text;
    }

void PyMfcHost::deleteString(int id, int index)
{
        if (index < 0)
            return;
        if (HWND h = findByIdDeep(m_hWnd, id))
            ::SendMessageW(h, isComboBox(h) ? CB_DELETESTRING : LB_DELETESTRING,
                           static_cast<WPARAM>(index), 0);
    }

int PyMfcHost::getCurSel(int id) const
{
        HWND h = findByIdDeep(m_hWnd, id);
        return h ? static_cast<int>(::SendMessageW(h, isComboBox(h) ? CB_GETCURSEL : LB_GETCURSEL, 0, 0)) : -1;
    }

void PyMfcHost::setCurSel(int id, int index)
{
        if (HWND h = findByIdDeep(m_hWnd, id))
            ::SendMessageW(h, isComboBox(h) ? CB_SETCURSEL : LB_SETCURSEL,
                           static_cast<WPARAM>(index), 0);
    }

int PyMfcHost::getCheck(int id) const
{
        HWND h = findByIdDeep(m_hWnd, id);
        return h ? static_cast<int>(::SendMessageW(h, BM_GETCHECK, 0, 0)) : 0;
    }

void PyMfcHost::setCheck(int id, bool checked)
{
        HWND h = findByIdDeep(m_hWnd, id);
        if (h == nullptr)
            return;
        ::SendMessageW(h, BM_SETCHECK, checked ? BST_CHECKED : BST_UNCHECKED, 0);
        // BM_SETCHECK 只改状态不重绘，界面上的勾选标记不会跟着变
        ::InvalidateRect(h, nullptr, TRUE);
        ::UpdateWindow(h);
    }

void PyMfcHost::setRange(int id, int minValue, int maxValue)
{
        // PyMfcSkinBox 的 RangeSlider 没有系统类，量程记在宿主里
        auto itKind = m_skinKinds.find(id);
        if (itKind != m_skinKinds.end() &&
            (itKind->second == kSkinBoxRange || itKind->second == kSkinBoxDial ||
             itKind->second == kSkinBoxScrollBar))
        {
            m_boxRange[id] = std::make_pair(minValue, maxValue);
            // 量程变了只夹一下已有的两个拇指，不能清掉 —— XML 属性顺序里
            // LowValue/HighValue 常常写在 Minimum/Maximum 前面
            auto lo = m_rangeLo.find(id);
            if (lo != m_rangeLo.end())
                lo->second = lo->second < minValue ? minValue : (lo->second > maxValue ? maxValue : lo->second);
            auto hi = m_rangeHi.find(id);
            if (hi != m_rangeHi.end())
                hi->second = hi->second < minValue ? minValue : (hi->second > maxValue ? maxValue : hi->second);
            return;
        }
        HWND h = findByIdDeep(m_hWnd, id);
        if (h == nullptr)
            return;
        if (isProgressBar(h))
        {
            ::SendMessageW(h, PBM_SETRANGE32, static_cast<WPARAM>(minValue),
                           static_cast<LPARAM>(maxValue));
        }
        else if (isTrackBar(h))
        {
            ::SendMessageW(h, TBM_SETRANGE, TRUE,
                           static_cast<LPARAM>(MAKELPARAM(minValue, maxValue)));
        }
    }

void PyMfcHost::setPos(int id, int value)
{
        HWND h = findByIdDeep(m_hWnd, id);
        if (h == nullptr)
            return;
        if (isProgressBar(h))
        {
            ::SendMessageW(h, PBM_SETPOS, static_cast<WPARAM>(value), 0);
            // PB/TB 设值默认只更新内部状态，界面不重画：值对了但进度条/滑块看不见
            ::InvalidateRect(h, nullptr, TRUE);
            ::UpdateWindow(h);
        }
        else if (isTrackBar(h))
        {
            ::SendMessageW(h, TBM_SETPOS, TRUE, static_cast<LPARAM>(value));
            ::InvalidateRect(h, nullptr, TRUE);
            ::UpdateWindow(h);
        }
    }

int PyMfcHost::getPos(int id) const
{
        HWND h = findByIdDeep(m_hWnd, id);
        if (h == nullptr)
            return 0;
        if (isProgressBar(h))
            return static_cast<int>(::SendMessageW(h, PBM_GETPOS, 0, 0));
        if (isTrackBar(h))
            return static_cast<int>(::SendMessageW(h, TBM_GETPOS, 0, 0));
        return 0;
    }

void PyMfcHost::setReadOnly(int id, bool readOnly)
{
        if (HWND h = findByIdDeep(m_hWnd, id))
            ::SendMessageW(h, EM_SETREADONLY, readOnly ? TRUE : FALSE, 0);
    }

bool PyMfcHost::getReadOnly(int id) const
{
        HWND h = findByIdDeep(m_hWnd, id);
        return h != nullptr && (::GetWindowLongPtrW(h, GWL_STYLE) & ES_READONLY) != 0;
    }

void PyMfcHost::setControlTheme(int id, const std::wstring& theme)
{ m_theme[id] = theme; }

std::wstring PyMfcHost::getControlTheme(int id) const
{
        auto it = m_theme.find(id);
        return it == m_theme.end() ? std::wstring() : it->second;
    }

bool PyMfcHost::isTextInput(HWND h)
{
        if (h == nullptr)
            return false;
        wchar_t cls[64] = {};
        ::GetClassNameW(h, cls, 64);
        return _wcsicmp(cls, L"Edit") == 0 || _wcsicmp(cls, L"RICHEDIT50W") == 0 ||
               _wcsicmp(cls, L"RichEdit20W") == 0;
    }

void PyMfcHost::stripEditFrame(HWND h)
{
        // 1) 去掉系统边框样式（client edge 是那个凹陷的浅色框）
        const LONG_PTR frameEx = WS_EX_CLIENTEDGE | 0x00020000L;   // 0x00020000 = WS_EX_STATICEDGE
        const LONG_PTR frameSt = WS_BORDER | WS_DLGFRAME;
        LONG_PTR ex = ::GetWindowLongPtrW(h, GWL_EXSTYLE);
        LONG_PTR st = ::GetWindowLongPtrW(h, GWL_STYLE);
        if ((ex & frameEx) != 0 || (st & frameSt) != 0)
        {
            ::SetWindowLongPtrW(h, GWL_EXSTYLE, ex & ~frameEx);
            ::SetWindowLongPtrW(h, GWL_STYLE, st & ~frameSt);
            ::SetWindowPos(h, nullptr, 0, 0, 0, 0,
                           SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE |
                               SWP_FRAMECHANGED);
        }
        // 2) 关掉 Visual Theme：无边框的 Edit 仍会被主题引擎补一条 1px 灰边，
        //    只有 SetWindowTheme(L"", L"") 才画纯色底。uxtheme 动态取，避免链接依赖。
        typedef HRESULT(WINAPI * FnSetWindowTheme)(HWND, LPCWSTR, LPCWSTR);
        static FnSetWindowTheme fn = nullptr;
        static bool probed = false;
        if (!probed)
        {
            probed = true;
            if (HMODULE ux = ::LoadLibraryW(L"uxtheme.dll"))
                fn = reinterpret_cast<FnSetWindowTheme>(
                    ::GetProcAddress(ux, "SetWindowTheme"));
        }
        if (fn != nullptr)
            fn(h, L"", L"");
    }

void PyMfcHost::setControlIcon(int id, const std::wstring& path)
{
        if (path.empty())
        {
            m_icons.erase(id);
            return;
        }
        m_icons[id] = path;
        if (HWND h = findByIdDeep(m_hWnd, id))
            ::InvalidateRect(h, nullptr, TRUE);
    }

const std::wstring* PyMfcHost::findIcon(int id) const
{
        auto it = m_icons.find(id);
        return it == m_icons.end() ? nullptr : &it->second;
    }

const xmlapp::XmlAppearance* PyMfcHost::findAppearance(int id) const
{
        auto it = m_ap.find(id);
        return it == m_ap.end() ? nullptr : &it->second;
    }

COLORREF PyMfcHost::contrastText(COLORREF bg)
{
        const int lum = (GetRValue(bg) * 299 + GetGValue(bg) * 587 + GetBValue(bg) * 114) / 1000;
        return lum > 140 ? RGB(32, 36, 42) : RGB(235, 235, 235);
    }

void PyMfcHost::releaseContainerPatternBrush(int id)
{
    auto it = m_containerPatternBrush.find(id);
    if (it == m_containerPatternBrush.end())
        return;
    if (it->second.brush) ::DeleteObject(it->second.brush);
    if (it->second.bitmap) ::DeleteObject(it->second.bitmap);
    m_containerPatternBrush.erase(it);
}

HBRUSH PyMfcHost::containerBackdropBrush(int id, const CRect& rect,
                              const xmlapp::XmlAppearance& ap, COLORREF fallbackBack)
{
    const int w = rect.Width();
    const int h = rect.Height();
    if (w <= 0 || h <= 0)
        return nullptr;

    // 内投影：同一条 box-shadow 原文里的 inset 分支，只在盒内合成，不进盒外老路径。
    // 多条阴影时取"最强的一条 inset"（alpha 最大，同值取 blur+spread 大的）——
    // GDI 位图刷只合成一层内投影，把多条拼成一条会得到语义错位的怪阴影。
    xmlapp::BoxShadowSpec inset;
    bool hasInset = false;
    {
        std::vector<xmlapp::BoxShadowSpec> shadows;
        xmlapp::ParseBoxShadowSpecList(ap.boxShadow, shadows);
        for (const xmlapp::BoxShadowSpec& sp : shadows)
        {
            if (!sp.inset)
                continue;
            if (!hasInset || sp.alpha > inset.alpha ||
                (sp.alpha == inset.alpha && (sp.blur + sp.spread) > (inset.blur + inset.spread)))
            {
                inset = sp;
                hasInset = true;
            }
        }
    }

    COLORREF g1 = 0, g2 = 0;
    int gAng = 180;
    const bool hasGradient = !ap.bgImage.empty() &&
        xmlapp::ParseLinearGradient(ap.bgImage, g1, g2, gAng);

    // CSS 边框：容器（div/面板）是空 STATIC，自己不 WM_PAINT；宿主 DC 上的描边
    // 又被 WS_CLIPCHILDREN 从父 DC 裁掉 —— 于是 `border:2px solid red` 写在 <div> 上
    // 永远看不见（15 轮遗留的真缺口）。容器唯一可见落点是 CTLCOLOR 返回的位图刷：
    // 把边框直接合成进这张"客户区大小"的位图，等价于 HTML 的"边框画在盒子内边缘"。
    COLORREF borderCol = 0;
    const bool borderColOk = !ap.borderColor.empty() &&
                             xmlapp::ParseColor(ap.borderColor, borderCol);
    // CSS 单边边框（border-bottom:1px solid #333）：容器的整圈描边走 borderPixelCode，
    // 单边要走下面的逐边分支 —— 否则 <div> 上画一条分割线在 XML 侧仍然做不到。
    const bool anySide = ap.borderSideW[0] >= 0 || ap.borderSideW[1] >= 0 ||
                         ap.borderSideW[2] >= 0 || ap.borderSideW[3] >= 0;
    const bool hasBorder = (xmlapp::HasVisibleBorder(ap) && borderColOk) || anySide;
    COLORREF sideCol[4];
    int sideW[4];
    std::wstring sideStyle[4];
    for (int i = 0; i < 4; ++i)
    {
        sideW[i] = anySide
            ? (ap.borderSideW[i] >= 0 ? ap.borderSideW[i]
                                      : (ap.borderThickness > 0 ? ap.borderThickness : 0))
            : 0;
        sideCol[i] = m_border;
        if (!ap.borderSideColor[i].empty())
        {
            COLORREF c = 0;
            if (xmlapp::ParseColor(ap.borderSideColor[i], c)) sideCol[i] = c;
        }
        else if (borderColOk)
            sideCol[i] = borderCol;
        sideStyle[i] = ap.borderSideStyle[i].empty() ? ap.borderStyle : ap.borderSideStyle[i];
    }
    const int bt = ap.borderThickness > 0 ? ap.borderThickness : 1;
    // 显式写了圆角（含分角）的容器，即使没有边框也要切角 —— HTML 里"圆角卡片不画边框"
    // 最常见，之前只有 hasBorder 才出位图，于是圆角永远看不见。
    const bool explicitRadius = ap.radius > 0 || ap.radiusPct >= 0 ||
                                ap.radiusTL >= 0 || ap.radiusTR >= 0 ||
                                ap.radiusBR >= 0 || ap.radiusBL >= 0;
    // 单边声明时不参与圆角切角（简化口径，与 xmlDrawBackplate 一致）。
    const int rad = ((hasBorder || explicitRadius) && !anySide)
                        ? xmlapp::ResolveRadius(rect, ap) : 0;
    // CSS 分角圆角：四角半径不同时走逐角版本（单一半径的 RoundedRect 表达不了）
    int cn[4] = {0, 0, 0, 0};
    bool splitCorners = false;
    if ((hasBorder || explicitRadius) && !anySide)
    {
        xmlapp::ResolveCorners(rect, ap, cn);
        splitCorners = (cn[0] != cn[1] || cn[1] != cn[2] || cn[2] != cn[3]);
    }

    // 纯色且无 inset 且无边框且无圆角：保持第 86 轮快路径
    if (!hasGradient && !hasInset && !hasBorder && !explicitRadius)
        return nullptr;

    const int op = (std::min)(ap.opacity, ap.bgAlpha);

    // 缓存键：无 inset 时与第 86 轮原键串逐段一致；仅当内阴影存在才追加签名字段。
    std::wstring sig;
    COLORREF base = fallbackBack;
    if (!ap.backColor.empty())
    {
        COLORREF c = 0;
        if (xmlapp::ParseColor(ap.backColor, c))
            base = xmlapp::Blend(c, fallbackBack, op);
    }
    if (hasGradient)
        sig = ap.bgImage + L"|" +
            std::to_wstring(w) + L"x" + std::to_wstring(h) + L"|" +
            std::to_wstring(op) + L"|" + std::to_wstring(fallbackBack);
    else
        sig = L"solid|" + std::to_wstring(base) + L"|" +
            std::to_wstring(w) + L"x" + std::to_wstring(h);
    if (hasInset)
    {
        sig += L"|in=" + std::to_wstring(inset.ox) + L"," + std::to_wstring(inset.oy) + L"," +
               std::to_wstring(inset.blur) + L"," + std::to_wstring(inset.color) + L"," +
               std::to_wstring(inset.alpha);
        // 仅当 spread 非 0 才追加签名；spread=0 的单 inset 键串逐字节不变，避免像素漂移。
        if (inset.spread != 0)
            sig += L"," + std::to_wstring(inset.spread);
    }
    if (hasBorder)
        sig += L"|bd=" + std::to_wstring(borderCol) + L"," + std::to_wstring(bt) +
               L"," + std::to_wstring(rad) + L"," + std::to_wstring(ap.opacity) +
               L"," + ap.borderStyle;
    else if (splitCorners || rad > 0)
        sig += L"|r=" + std::to_wstring(rad);
    if (splitCorners)
        sig += L"|rc=" + std::to_wstring(cn[0]) + L"," + std::to_wstring(cn[1]) + L"," +
               std::to_wstring(cn[2]) + L"," + std::to_wstring(cn[3]);
    if (anySide)
    {
        for (int i = 0; i < 4; ++i)
            sig += L"|s" + std::to_wstring(i) + L"=" + std::to_wstring(sideW[i]) + L"," +
                   std::to_wstring(sideCol[i]) + L"," + sideStyle[i];
    }

    auto cached = m_containerPatternBrush.find(id);
    if (cached != m_containerPatternBrush.end() && cached->second.brush != nullptr &&
        cached->second.width == w && cached->second.height == h &&
        cached->second.signature == sig)
        return cached->second.brush;

    releaseContainerPatternBrush(id);

    BITMAPINFO bi = {};
    bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bi.bmiHeader.biWidth = w;
    bi.bmiHeader.biHeight = -h;   // 自顶向下，与屏幕坐标一致
    bi.bmiHeader.biPlanes = 1;
    bi.bmiHeader.biBitCount = 32;
    bi.bmiHeader.biCompression = BI_RGB;
    void* bits = nullptr;
    HBITMAP bmp = ::CreateDIBSection(nullptr, &bi, DIB_RGB_COLORS, &bits, nullptr, 0);
    if (bmp == nullptr || bits == nullptr)
    {
        if (bmp) ::DeleteObject(bmp);
        return nullptr;
    }
    auto* px = static_cast<DWORD*>(bits);

    const COLORREF c1 = hasGradient ? xmlapp::Blend(g1, fallbackBack, op) : base;
    const COLORREF c2 = hasGradient ? xmlapp::Blend(g2, fallbackBack, op) : base;
    const bool vertical = (gAng % 180) != 90;

    for (int y = 0; y < h; ++y)
    {
        for (int x = 0; x < w; ++x)
        {
            COLORREF col;
            if (hasGradient)
            {
                const int t = vertical
                    ? (h > 1 ? y * 255 / (h - 1) : 0)
                    : (w > 1 ? x * 255 / (w - 1) : 0);
                col = RGB(
                    GetRValue(c1) + (GetRValue(c2) - GetRValue(c1)) * t / 255,
                    GetGValue(c1) + (GetGValue(c2) - GetGValue(c1)) * t / 255,
                    GetBValue(c1) + (GetBValue(c2) - GetBValue(c1)) * t / 255);
            }
            else
                col = base;

            // inset 合成：按到内边缘的距离衰减，只在盒内叠加；blur 决定渐隐带宽。
            // offset 把阴影层向对应方向推移：指向侧的对侧出暗带（inset 0 2px → 上沿
            // 带），指向侧不收窄带；offset 为 0 时四边对称。与底色预混，位图即客户区
            // 尺寸，不出盒外。
            if (hasInset)
            {
                const int band = (std::max)(1, inset.blur);
                auto edgeA = [&](int d) {
                    if (d < 0) d = 0;
                    int a = inset.alpha * (band - d) / band;
                    return (std::min)(255, (std::max)(0, a));
                };
                // spread 作用在距离基准上：d' = d - spread；正 spread → d' 变小 → 暗带向内扩展，
                // 负 spread → d' 变大 → 暗带更贴边；d' 为负按 0 处理（等价已越过阴影范围）。
                // 位图即客户区几何，不改位图尺寸。
                const int dTop    = (std::max)(0, y - inset.oy - inset.spread);
                const int dBottom = (std::max)(0, h - 1 - y - inset.spread);
                const int dLeft   = (std::max)(0, x - inset.ox - inset.spread);
                const int dRight  = (std::max)(0, w - 1 - x - inset.spread);
                int a = 0;
                if (inset.oy >= 0) a = (std::max)(a, edgeA(dTop));
                if (inset.oy <= 0) a = (std::max)(a, edgeA(dBottom));
                if (inset.ox >= 0) a = (std::max)(a, edgeA(dLeft));
                if (inset.ox <= 0) a = (std::max)(a, edgeA(dRight));
                if (a > 0)
                    col = xmlapp::Blend(inset.color, col, a);
            }
            // CSS 边框：外沿贴盒边、向内 bt 像素；线型/明暗半道/圆角外角都由 borderPixelCode 判。
            if (anySide)
            {
                const int dTop = y, dBot = h - 1 - y, dLeft = x, dRight = w - 1 - x;
                for (int i = 0; i < 4; ++i)
                {
                    if (sideW[i] <= 0) continue;
                    int depth = 0, along = 0;
                    if (i == 0)      { depth = dTop;   along = x; }
                    else if (i == 1) { depth = dRight; along = y; }
                    else if (i == 2) { depth = dBot;   along = x; }
                    else             { depth = dLeft;  along = y; }
                    const int code = borderLinePixelCode(depth, along, sideW[i],
                                                         sideStyle[i].c_str());
                    if (code == 0) continue;
                    if (code == 1)
                        col = xmlapp::Blend(sideCol[i], col, ap.opacity);
                    else if (code == 2)
                        col = xmlapp::Blend(xmlapp::Blend(RGB(255, 255, 255), sideCol[i], 110),
                                            col, ap.opacity);
                    else
                        col = xmlapp::Blend(xmlapp::Blend(RGB(0, 0, 0), sideCol[i], 110),
                                            col, ap.opacity);
                    break;              // 角落两条都命中：按 CSS 对角斜切近似取第一条
                }
            }
            else if (hasBorder || explicitRadius)
            {
                const int code = splitCorners
                    ? borderPixelCodeCorners(x, y, w, h, cn[0], cn[1], cn[2], cn[3], bt,
                                             ap.borderStyle.c_str())
                    : borderPixelCode(x, y, w, h, rad, bt, ap.borderStyle.c_str());
                if (code < 0)
                    col = fallbackBack;         // 圆角之外的像素在 CSS 里不属于盒子
                else if (hasBorder && code == 1)
                    col = xmlapp::Blend(borderCol, col, ap.opacity);
                else if (hasBorder && code == 2)
                    col = xmlapp::Blend(xmlapp::Blend(RGB(255, 255, 255), borderCol, 110),
                                        col, ap.opacity);
                else if (hasBorder && code == 3)
                    col = xmlapp::Blend(xmlapp::Blend(RGB(0, 0, 0), borderCol, 110),
                                        col, ap.opacity);
            }
            // DIB 32bpp BI_RGB 的内存字节序是 R,G,B,A；COLORREF 是 0x00BBGGRR，
            // 直接 static_cast 会把红蓝互换（linear-gradient(#ff0000) 画成纯蓝，真 bug）。
            px[y * w + x] = static_cast<DWORD>(((col & 0x000000FFu) << 16) |
                                               (col & 0x0000FF00u) |
                                               ((col & 0x00FF0000u) >> 16));
        }
    }

    HBRUSH brush = ::CreatePatternBrush(bmp);
    if (brush == nullptr)
    {
        ::DeleteObject(bmp);
        return nullptr;
    }
    ContainerPatternBrush entry;
    entry.brush = brush;
    entry.bitmap = bmp;
    entry.width = w;
    entry.height = h;
    entry.signature = sig;
    m_containerPatternBrush[id] = entry;
    return brush;
}

void PyMfcHost::addContainerBackdrop(int x, int y, int w, int h,
                              const xmlapp::XmlAppearance& ap)
{
        if (ap.empty())
            return;
        m_backdrops.emplace_back(CRect(x, y, x + w, y + h), ap);
    }

void PyMfcHost::clearContainerBackdrops()
{
        m_backdrops.clear();
    }

bool PyMfcHost::getVisible(int id) const
{
        HWND h = findByIdDeep(m_hWnd, id);
        return h != nullptr && ::IsWindowVisible(h) != FALSE;
    }

bool PyMfcHost::getEnabled(int id) const
{
        HWND h = findByIdDeep(m_hWnd, id);
        return h != nullptr && ::IsWindowEnabled(h) != FALSE;
    }

py::object PyMfcHost::cellRect(int id, int row, int col)
{
        if (TableControl* nt = findNewTable(id)) return tableCellRect(id, row, col);
        HWND h = findByIdDeep(m_hWnd, id);
        if (h == nullptr)
            return py::none();
        wchar_t cls[64] = {};
        ::GetClassNameW(h, cls, 64);
        if (_wcsicmp(cls, L"SysListView32") != 0)
            return py::none();
        // LVM_GETSUBITEMRECT 在本机 SysListView32 上水平方向恒返回 0 宽（left==right），
        // 拿不到单元格左右边界。改成"行矩形 + 累加列宽"自己算，实测稳定。
        RECT rowRc = { 0, 0, 0, 0 };
        if (::SendMessageW(h, LVM_GETITEMRECT, static_cast<WPARAM>(row),
                           reinterpret_cast<LPARAM>(&rowRc)) == FALSE)
            return py::none();
        int colX = 0, colW = 0;
        for (int i = 0; i <= col; ++i)
        {
            const int cw = static_cast<int>(::SendMessageW(h, LVM_GETCOLUMNWIDTH,
                                                           static_cast<WPARAM>(i), 0));
            if (cw <= 0)
                return py::none();
            if (i < col)
                colX += cw;
            else
                colW = cw;
        }
        POINT org = { 0, 0 };
        ::SendMessageW(h, LVM_GETORIGIN, 0, reinterpret_cast<LPARAM>(&org));
        const int x = colX - org.x;         // 横向滚动补偿（表格无横滚时 org.x=0）
        const int hgt = rowRc.bottom - rowRc.top;
        debugLogf(L"[cellrect] id=%d row=%d col=%d -> (%d,%d,%d,%d)",
                  id, row, col, x, rowRc.top, colW, hgt);
        if (colW <= 0 || hgt <= 0)
            return py::none();
        return py::make_tuple(x, rowRc.top, colW, hgt);
    }

void PyMfcHost::setUrl(int id, const std::wstring& url)
{
        m_urls[id] = url;
        applyLinkText(id);
    }

std::wstring PyMfcHost::getUrl(int id) const
{
        auto it = m_urls.find(id);
        if (it != m_urls.end())
            return it->second;
        // 没有记录时从控件文本里解析，兼容 XML 直接写成 <a href="..."> 的情况
        const std::wstring text = getText(id);
        const size_t a = text.find(L"href=\"");
        if (a == std::wstring::npos)
            return {};
        const size_t b = text.find(L"\"", a + 6);
        return b == std::wstring::npos ? std::wstring() : text.substr(a + 6, b - a - 6);
    }

void PyMfcHost::setRichFormat(int id, int sizePt, bool bold, bool italic, unsigned long rgb)
{
        HWND h = findByIdDeep(m_hWnd, id);
        if (h == nullptr)
            return;
        CHARFORMAT2W cf = {};
        cf.cbSize = sizeof(cf);
        cf.dwMask = CFM_BOLD | CFM_ITALIC | CFM_SIZE | CFM_COLOR;
        cf.dwEffects = (bold ? CFE_BOLD : 0) | (italic ? CFE_ITALIC : 0);
        cf.yHeight = (sizePt > 0 ? sizePt : 9) * 20;   // 1pt = 20 twips
        cf.crTextColor = RGB((rgb >> 16) & 0xFF, (rgb >> 8) & 0xFF, rgb & 0xFF);
        ::SendMessageW(h, EM_SETCHARFORMAT, SCF_ALL, reinterpret_cast<LPARAM>(&cf));
        ::InvalidateRect(h, nullptr, TRUE);
    }

void PyMfcHost::setScrollRange(int id, int minValue, int maxValue)
{
        HWND h = findByIdDeep(m_hWnd, id);
        if (h == nullptr)
            return;
        m_scroll[id] = std::make_pair(minValue, maxValue);
        if (m_scrollPos.find(id) == m_scrollPos.end())
            m_scrollPos[id] = minValue;      // 初始位置 = 最小值
        // 自绘滚动区域不能调 SetScrollInfo：系统会给窗口补上一条原生滚动条（WS_VSCROLL），
        // 抢走 17px 客户区并在右侧留下一条白条。
        if (m_scrollAreas.count(id) != 0)
        {
            ::InvalidateRect(h, nullptr, FALSE);
            return;
        }
        SCROLLINFO si = {};
        si.cbSize = sizeof(si);
        si.fMask = SIF_RANGE | SIF_PAGE;
        si.nMin = minValue;
        si.nMax = maxValue;
        si.nPage = 10;
        ::SetScrollInfo(h, SB_VERT, &si, TRUE);
        ::InvalidateRect(h, nullptr, TRUE);
    }

std::pair<int, int> PyMfcHost::getScrollRange(int id) const
{
        auto it = m_scroll.find(id);
        if (it != m_scroll.end())
            return std::make_pair(it->second.first, it->second.second);
        HWND h = findByIdDeep(m_hWnd, id);
        if (h == nullptr)
            return {0, 0};
        SCROLLINFO si = {};
        si.cbSize = sizeof(si);
        si.fMask = SIF_RANGE;
        ::GetScrollInfo(h, SB_VERT, &si);
        return {si.nMin, si.nMax};
    }

std::pair<int, int> PyMfcHost::getSelection(int id) const
{
        HWND h = findByIdDeep(m_hWnd, id);
        if (h == nullptr)
            return { 0, 0 };
        wchar_t cls[64] = {};
        ::GetClassNameW(h, cls, 64);
        if (_wcsicmp(cls, L"RICHEDIT50W") == 0)
        {
            CHARRANGE cr = {};
            ::SendMessageW(h, EM_EXGETSEL, 0, reinterpret_cast<LPARAM>(&cr));
            return { static_cast<int>(cr.cpMin), static_cast<int>(cr.cpMax) };
        }
        DWORD s = 0;
        DWORD e = 0;
        ::SendMessageW(h, EM_GETSEL, reinterpret_cast<WPARAM>(&s), reinterpret_cast<LPARAM>(&e));
        return { static_cast<int>(s), static_cast<int>(e) };
    }

void PyMfcHost::setSelection(int id, int from, int to) const
{
        HWND h = findByIdDeep(m_hWnd, id);
        if (h == nullptr)
            return;
        if (to < from)
            to = from;
        ::SendMessageW(h, EM_SETSEL, static_cast<WPARAM>(from), static_cast<LPARAM>(to));
        if (::GetFocus() == h)
            ::SendMessageW(h, EM_SCROLLCARET, 0, 0);
        ::InvalidateRect(h, nullptr, FALSE);
    }

int PyMfcHost::getScrollPos(int id) const
{        auto it = m_scrollPos.find(id);
        if (it != m_scrollPos.end())
            return it->second;
        HWND h = findByIdDeep(m_hWnd, id);
        if (h == nullptr)
            return 0;
        SCROLLINFO si = {};
        si.cbSize = sizeof(si);
        si.fMask = SIF_POS;
        ::GetScrollInfo(h, SB_VERT, &si);
        return si.nPos;
    }
