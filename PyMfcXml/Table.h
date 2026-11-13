#pragma once
#include <windows.h>
#include <commctrl.h>
#include <vector>
#include <algorithm>

struct FilterPopupData;

// =====================================================================
// Table —— 表格控件（SysListView32 的唯一几何真相来源）
// 背景：之前"点击某行→蓝框落到下一行""原位编辑框与选中行错开一行"，
// 根因是命中行号、选中行号、编辑框矩形分别由三处各算一套。
// 这个类把 rows / cols / rect / cell 收进一个对象，调用方只问它：
//   rect()      -> 控件客户区
//   cell(r,c)   -> 第 r 行第 c 列格的矩形（客户坐标）
//   rowAt(y)    -> 客户 y 落在第几行（-1 = 表头或空白）
//   cellAt(x,y) -> 命中 (row,col)，与 cell(row,col) 完全一致
// 命中、选中、编辑框从此共用同一份计算，不会再差一行。
// =====================================================================
class Table
{
public:
    Table() : m_hwnd(nullptr) {}
    explicit Table(HWND hwnd) : m_hwnd(hwnd) {}

    void attach(HWND hwnd) { m_hwnd = hwnd; }
    HWND hwnd() const { return m_hwnd; }
    bool valid() const { return m_hwnd != nullptr && ::IsWindow(m_hwnd) == TRUE; }

    int rows() const;                        // 行数
    int cols() const;                        // 列数
    std::vector<int> colWidths() const;      // 每列宽度
    int headerHeight() const;                // 表头高度（= 第 0 行矩形 top）
    int rowHeight() const;                   // 行高（第 0 行矩形高度）

    RECT rect() const;                       // 控件客户区
    RECT rowRect(int row) const;             // 第 row 行矩形（失败返回全 0）
    RECT cell(int row, int col) const;       // 单元格矩形（表头/空白/越界返回全 0）

    int rowAt(int y) const;                  // 客户 y → 行号，-1 表示不在数据行上
    bool cellAt(int x, int y, int& row, int& col) const;  // 命中，与 cell() 同源

    int selectedRow() const;                 // 原生选中行，-1 无
    void selectRow(int row);                 // 设选中行（clamp 到 [0, rows-1]）
    void ensureVisible(int row);
private:
    HWND m_hwnd;
};


// =====================================================================
// TableControl —— 独立控件套装：注册自己的窗口类，自带数据(rows/cols)、
// 自带几何(rect/cell)、自带绘制与命中，不再依赖宿主内嵌 SysListView32。
//   Register(hInst)  注册窗口类 "PyMfcTable"
//   Create(parent,x,y,w,h)  创建实例，返回 HWND
//   AppendRow / SetCell / Cell / CellRect / RowRect / Rect
//   Select(row) / Selected()
//   SetCallback(fn, ctx)   命中/选中/按键回调（fn(void*,int row,int col,int what)）
// what: 1=单击 2=双击 3=选中行变化 4=F2 5=Delete
// =====================================================================
typedef void (*TableEventFn)(void* ctx, int row, int col, int what);

// 列数据类型（对应 C# DataGridViewColumn.ValueType / DataList 的列定义）
// 由 Python 逐列指定，控件据此解析、对齐、排序，不再猜。
enum class ColType : int
{
    Text   = 0,   // 字符串（默认）：左对齐、按 wstring 比较
    Int    = 1,   // 整数：右对齐、按 long long 比较
    Double = 2,   // 浮点：右对齐、按 double 比较
    Bool   = 3,   // 布尔：居中，true/false/1/0/是/否 都接受
};

class TableControl
{
public:
    static bool Register(HINSTANCE hInst);
    static HWND Create(HWND parent, int x, int y, int w, int h, int id);
    static TableControl* From(HWND hwnd);
    ~TableControl() { ReleaseGdi(); }

    void AppendRow(const std::vector<std::wstring>& cells);
    void Clear();
    void SetColumns(const std::vector<int>& widths, const std::vector<std::wstring>& titles);
    void SetCell(int row, int col, const std::wstring& text);

    int Rows() const { return static_cast<int>(m_rows.size()); }
    int Cols() const { return static_cast<int>(m_colW.size()); }
    RECT Rect() const;
    RECT RowRect(int row) const;
    RECT Cell(int row, int col) const;
    bool Hit(int x, int y, int& row, int& col) const;

    // 垂直滚动：当前可视首行（数据行号）、可视行数、最大滚动位置
    int  ScrollPos() const { return m_scroll; }
    int  VisibleCount() const;
    int  MaxScroll() const;
    // 水平滚动：当前横向偏移（像素）、最大偏移、按像素滚动
    int  HScrollPos() const { return m_colScroll; }
    int  MaxHScroll() const;
    void ScrollXBy(int px);
    void ScrollBy(int rows);
    void EnsureVisible(int row);

    int Selected() const { return m_sel; }
    void Select(int row);

    // ---- 列类型（Python 指定）----
    void SetColType(int col, ColType t);                       // 单列指定
    void SetColTypes(const std::vector<int>& types);            // 整表指定（一次灌完）
    ColType ColTypeOf(int col) const;                           // 取某列类型
    // ---- 按类型取值（解析失败返回 false，text 永远是原文）----
    bool GetInt(int row, int col, long long& out) const;
    bool GetDouble(int row, int col, double& out) const;
    bool GetBool(int row, int col, bool& out) const;
    const std::wstring GetText(int row, int col) const;
    // ---- 按类型比较（排序/查找用，跨类型安全）----
    int CompareCell(int r1, int c, int r2) const;

    // ---- 表头过滤器 / 排序（对标 HTML 表头的漏斗图标）----
    void   SetFilter(int col, const std::wstring& text);   // 子串筛选（空=取消）
    std::wstring FilterOf(int col) const;
    // 按“值”多选筛选（Excel 漏斗：勾选候选值，空 = 取消本列）
    void   SetFilterValues(int col, const std::vector<std::wstring>& values);
    std::vector<std::wstring> FilterValuesOf(int col) const;
    void   SetFilterValue(int col, const std::wstring& value);   // 单值便捷入口
    void   ClearFilters();                                  // 清空全部筛选
    void   SortBy(int col, bool desc);                      // 按列类型排序
    int    ViewRows() const { return static_cast<int>(m_view.size()); }  // 过滤后可见行数
    int    RowCount() const { return Rows(); }
    std::vector<std::wstring> RowAt(int row) const;
    std::wstring ColumnTitle(int col) const;

    void SetCallback(TableEventFn fn, void* ctx) { m_fn = fn; m_ctx = ctx; }
    const std::wstring& pendingText() const { return m_pendingText; }
    void SetTextStyle(const std::wstring& face, int pt, COLORREF fg, COLORREF line,
                      COLORREF selBg, COLORREF hdrBg);
    void Invalidate();

private:
    HWND m_hwnd = nullptr;
    int m_id = 0;
    std::vector<std::vector<std::wstring>> m_rows;   // rows
    std::vector<int> m_colW;                         // cols
    std::vector<std::wstring> m_colTitle;
    int m_sel = -1;
    int m_headerH = 24, m_rowH = 21;
    int m_scroll = 0;                                  // 可视首行（数据行号）
    std::vector<ColType> m_colType;                   // cols 的类型表（默认 Text）
    std::vector<std::wstring> m_filter;               // 每列筛选词（子串匹配）
    std::vector<int> m_view;                          // 过滤后的可见行（存数据行号）
    int m_scrollTop = 0;                              // 纵向滚动量（像素）
    // 表头拖列宽：命中列右边界按下 → 拖动改 m_colW[col] → 松开结束
    int m_resizeCol = -1;                             // 正在拖的列（-1 = 没在拖）
    int m_resizeX0 = 0, m_resizeW0 = 0;                // 按下时的鼠标 x / 原列宽
    HWND m_hFilterEdit = nullptr;                     // 表头筛选输入框（临时子窗口）
    // 自绘纵向滚动条：拖动拇指 / 点击空白轨道翻页
    int m_sbDrag = 0;                                 // 1 = 正在拖拇指
    int m_sbGrab = 0;                                  // 按下点相对拇指顶端的偏移
    RECT SbTrack() const;
    void SbThumb(int& thumbTop, int& thumbH) const;
    int  SbScrollFromY(int y, int thumbH) const;
    // 自绘底部横向滚动条（复用纵向那套几何：绘制与命中同一份算法）
    int m_colScroll = 0;                               // 横向滚动量（像素）
    int m_sbHDrag = 0;                                 // 1 = 正在拖横向拇指
    int m_hsbGrab = 0;                                 // 按下点相对拇指左端的偏移
    int ContentW() const;                              // 所有列宽合计
    bool HasHBar() const;                              // 需要横向滚动条
    RECT SbTrackX() const;
    void SbThumbX(int& thumbLeft, int& thumbW) const;
    int  SbScrollFromX(int x, int thumbW) const;
    void ClampXScroll();                              // 列宽/行数变化后夹住横向偏移

    void RebuildView();
    bool RowPassesFilter(const std::vector<std::wstring>& cells) const;
public:
    void SetRowsBulk(const std::vector<std::vector<std::wstring>>& rows);
private:
    void InvalidateScrollOnly();
    int  ViewPos(int dataRow) const;                  // 数据行 -> 可视位置，-1 表示被过滤掉
    int  ColAtX(int x) const;
    void RunHeaderFilter(int col);                    // 点漏斗：在表头格开输入框（非模态）
    void ApplyHeaderFilter();                         // 回车/失焦：取文字并应用筛选
    void CancelHeaderFilter();                        // Esc：关掉输入框，不筛选
    int  m_filterCol = -1;                            // 当前筛选输入框对应的列
    std::vector<std::vector<std::wstring>> m_filterValues;   // 按值多选筛选（Excel 漏斗勾选）
    FilterPopupData* m_fp = nullptr;                  // 表头漏斗面板（画在表格自己身上）
    void OpenFilterPopup(int col);
    void CloseFilterPopup();
    static void ApplyFilterPopup(FilterPopupData* d);
    bool HandleFilterPanelClick(int mx, int my);
    bool HandleFilterPanelKey(UINT msg, WPARAM wp);
    static WNDPROC s_headerEditProc;                   // 输入框原窗口过程（子类化用）
    static LRESULT CALLBACK HeaderEditProc(HWND, UINT, WPARAM, LPARAM);
    void RunHeaderMenu(int col);                      // 双击表头：弹出菜单
    void ApplySort(int col, bool desc);
    std::wstring m_face = L"Microsoft YaHei UI";
    int m_pt = 9;
    COLORREF m_fg = RGB(0x1F,0x29,0x37), m_line = RGB(0xE4,0xE7,0xEC),
             m_selBg = RGB(0x1F,0x6F,0xEB), m_hdrBg = RGB(0xF7,0xF8,0xFA);
    TableEventFn m_fn = nullptr;
    void* m_ctx = nullptr;
    std::wstring m_pendingText;                          // 回调附带文本（粘贴剪贴板内容等）

    // 绘制缓存：表格滚动时每帧都要整块重画，原来每帧 CreateCompatibleDC/Bitmap、
    // 建字体、逐行建画刷 —— 这些 GDI 对象的创建比真正画内容还贵。
    HDC m_memDc = nullptr;
    HBITMAP m_memBmp = nullptr;
    int m_memW = 0, m_memH = 0;
    HFONT m_font = nullptr;
    int m_fontPt = 0;
    std::wstring m_fontFace;
    HBRUSH m_brHdr = nullptr, m_brSel = nullptr, m_brLine = nullptr;
    HBRUSH m_brTrack = nullptr, m_brThumb = nullptr;
    COLORREF m_brHdrC = 0, m_brSelC = 0, m_brLineC = 0, m_brTrackC = 0, m_brThumbC = 0;

    bool EnsureBuffers(const RECT& rc);
    void ReleaseGdi();
    // 内容带缓存：整表所有行预先画进一张离屏 DIB，滚动时只 BitBlt 切片。
    // m_ctxDirty 在数据/列宽/选中变化时置真（见 Invalidate）。
    HDC m_ctxDc = nullptr;
    HBITMAP m_ctxBmp = nullptr;
    int m_ctxW = 0, m_ctxH = 0;
    int m_ctxFrom = 0, m_ctxRows = 0;      // 内容带覆盖的可视行区间 [from, from+rows)
    int m_ctxX = 0;                        // 内容带绘制时的横向偏移（变了要整带重建）
    bool m_ctxDirty = true;

    static LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
    LRESULT OnPaint();
    void Fire(int row, int col, int what, const std::wstring& text = L"");
};
