import io
p='G:/PyArx/PyMfcXaml/Table.h'
s=io.open(p,encoding='utf-8-sig').read()
s=s.replace('''    void SetCallback(TableEventFn fn, void* ctx) { m_fn = fn; m_ctx = ctx; }''',
'''    // ---- 表头过滤器 / 排序（对标 HTML 表头的漏斗图标）----
    void   SetFilter(int col, const std::wstring& text);   // 设置某列筛选（空=取消）
    std::wstring FilterOf(int col) const;
    void   ClearFilters();                                  // 清空全部筛选
    void   SortBy(int col, bool desc);                      // 按列类型排序
    int    ViewRows() const { return static_cast<int>(m_view.size()); }  // 过滤后可见行数

    void SetCallback(TableEventFn fn, void* ctx) { m_fn = fn; m_ctx = ctx; }''')
s=s.replace('''    std::vector<ColType> m_colType;                   // cols 的类型表（默认 Text）''',
'''    std::vector<ColType> m_colType;                   // cols 的类型表（默认 Text）
    std::vector<std::wstring> m_filter;               // 每列筛选词（子串匹配）
    std::vector<int> m_view;                          // 过滤后的可见行（存数据行号）
    HWND m_hFilterEdit = nullptr;                     // 表头筛选输入框（临时子窗口）

    void RebuildView();
    int  ViewPos(int dataRow) const;                  // 数据行 -> 可视位置，-1 表示被过滤掉
    int  ColAtX(int x) const;
    void RunHeaderFilter(int col);                    // 点漏斗：弹出输入框并模态收字
    void RunHeaderMenu(int col);                      // 双击表头：弹出菜单
    void ApplySort(int col, bool desc);''')
io.open(p,'w',encoding='utf-8-sig',newline='').write(s)

p2='G:/PyArx/PyMfcXaml/Table.cpp'
c=io.open(p2,encoding='utf-8-sig').read()
c=c.replace('''bool TableControl::Hit(int x, int y, int& row, int& col) const
{
    row = -1; col = -1;
    if (y < m_headerH) return false;
    const int r = (y - m_headerH) / m_rowH;
    if (r < 0 || r >= Rows()) return false;''',
'''bool TableControl::Hit(int x, int y, int& row, int& col) const
{
    row = -1; col = -1;
    if (y < m_headerH) return false;
    const int pos = (y - m_headerH) / m_rowH;
    if (pos < 0 || pos >= ViewRows()) return false;
    row = m_view[pos];                       // 可视位置 -> 数据行（过滤后仍对得上）''')
c=c.replace('''    int acc = 0, c = -1;
    for (int i = 0; i < Cols(); ++i)
    {
        if (x >= acc && x < acc + m_colW[i]) { c = i; break; }
        acc += m_colW[i];
    }
    if (c < 0) return false;
    row = r; col = c;
    return true;
}''',
'''    col = ColAtX(x);
    return col >= 0;
}''')
c=c.replace('''RECT TableControl::RowRect(int row) const
{
    RECT rc = { 0,0,0,0 };
    if (row < 0 || row >= Rows()) return rc;
    rc.left = 0;
    rc.top = m_headerH + row * m_rowH;''',
'''RECT TableControl::RowRect(int row) const
{
    RECT rc = { 0,0,0,0 };
    const int pos = ViewPos(row);
    if (pos < 0) return rc;
    rc.left = 0;
    rc.top = m_headerH + pos * m_rowH;''')
c=c.replace('''    int x = 0;
    for (int i = 0; i < col; ++i) x += m_colW[i];
    rc.left = x;
    rc.right = x + m_colW[col];
    rc.top = m_headerH + row * m_rowH;
    rc.bottom = rc.top + m_rowH;
    return rc;''',
''',略''') if False else c
c=c.replace('''    int x = 0;
    for (int i = 0; i < col; ++i) x += m_colW[i];
    rc.left = x;
    rc.right = x + m_colW[col];
    rc.top = m_headerH + row * m_rowH;
    rc.bottom = rc.top + m_rowH;
    return rc;
}''',
'''    int x = 0;
    for (int i = 0; i < col; ++i) x += m_colW[i];
    rc.left = x;
    rc.right = x + m_colW[col];
    rc.top = m_headerH + ViewPos(row) * m_rowH;
    rc.bottom = rc.top + m_rowH;
    return rc;
}''')
# 追加新方法
c=c.replace('''void TableControl::AppendRow(const std::vector<std::wstring>& cells)
{ m_rows.push_back(cells); Invalidate(); }''',
'''void TableControl::AppendRow(const std::vector<std::wstring>& cells)
{ m_rows.push_back(cells); RebuildView(); Invalidate(); }

// ------------------------- 过滤视图 / 排序 -------------------------
void TableControl::Clear()
{ m_rows.clear(); m_sel = -1; m_view.clear(); Invalidate(); }

void TableControl::RebuildView()
{
    m_view.clear();
    const int rows = Rows(), cols = Cols();
    for (int r = 0; r < rows; ++r)
    {
        bool keep = true;
        for (int c = 0; c < cols && keep; ++c)
        {
            if (c >= static_cast<int>(m_filter.size())) continue;
            const std::wstring& f = m_filter[c];
            if (f.empty()) continue;
            std::wstring cell = (c < static_cast<int>(m_rows[r].size())) ? m_rows[r][c] : L"";
            std::wstring lc = cell, lf = f;
            for (wchar_t& ch : lc) ch = static_cast<wchar_t>(::towlower(ch));
            for (wchar_t& ch : lf) ch = static_cast<wchar_t>(::towlower(ch));
            if (lc.find(lf) == std::wstring::npos) keep = false;
        }
        if (keep) m_view.push_back(r);
    }
    if (m_view.empty() && rows > 0 && cols == 0) m_view.clear();
}

int TableControl::ViewPos(int dataRow) const
{
    for (size_t i = 0; i < m_view.size(); ++i) if (m_view[i] == dataRow) return static_cast<int>(i);
    return -1;
}

int TableControl::ColAtX(int x) const
{
    int acc = 0;
    for (int i = 0; i < Cols(); ++i)
    {
        if (x >= acc && x < acc + m_colW[i]) return i;
        acc += m_colW[i];
    }
    return -1;
}

void TableControl::SetFilter(int col, const std::wstring& text)
{
    if (col < 0) return;
    if (static_cast<int>(m_filter.size()) <= col) m_filter.resize(col + 1);
    m_filter[col] = text;
    RebuildView();
    Invalidate();
}

std::wstring TableControl::FilterOf(int col) const
{
    if (col < 0 || col >= static_cast<int>(m_filter.size())) return L"";
    return m_filter[col];
}

void TableControl::ClearFilters()
{
    m_filter.assign(Cols(), L"");
    RebuildView();
    Invalidate();
}

void TableControl::SortBy(int col, bool desc)
{
    if (col < 0 || col >= Cols()) return;
    const long long savedSel = (m_sel >= 0 && m_sel < Rows()) ? m_sel : -1;
    std::stable_sort(m_view.begin(), m_view.end(), [&](int a, int b) {
        const int cmp = CompareCell(a, col, b);
        return desc ? cmp > 0 : cmp < 0;
    });
    if (m_view.empty()) { RebuildView(); }
    (void)savedSel;
    Invalidate();
}

void TableControl::ApplySort(int col, bool desc) { SortBy(col, desc); }''')
io.open(p2,'w',encoding='utf-8-sig',newline='').write(c)
print('过滤视图/排序骨架已加入')
