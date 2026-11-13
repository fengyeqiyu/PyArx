import io
p='G:/PyArx/PyMfcXaml/Table.h'
s=io.open(p,encoding='utf-8-sig').read()
s=s.replace('''typedef void (*TableEventFn)(void* ctx, int row, int col, int what);''',
'''typedef void (*TableEventFn)(void* ctx, int row, int col, int what);

// 列数据类型（对应 C# DataGridViewColumn.ValueType / DataList 的列定义）
// 由 Python 逐列指定，控件据此解析、对齐、排序，不再猜。
enum class ColType : int
{
    Text   = 0,   // 字符串（默认）：左对齐、按 wstring 比较
    Int    = 1,   // 整数：右对齐、按 long long 比较
    Double = 2,   // 浮点：右对齐、按 double 比较
    Bool   = 3,   // 布尔：居中，true/false/1/0/是/否 都接受
};''')
s=s.replace('''    void SetCallback(TableEventFn fn, void* ctx) { m_fn = fn; m_ctx = ctx; }''',
'''    // ---- 列类型（Python 指定）----
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

    void SetCallback(TableEventFn fn, void* ctx) { m_fn = fn; m_ctx = ctx; }''')
s=s.replace('''    int m_headerH = 24, m_rowH = 21;''',
'''    int m_headerH = 24, m_rowH = 21;
    std::vector<ColType> m_colType;                   // cols 的类型表（默认 Text）''')
io.open(p,'w',encoding='utf-8-sig',newline='').write(s)

p2='G:/PyArx/PyMfcXaml/Table.cpp'
c=io.open(p2,encoding='utf-8-sig').read()
if 'TableControl::SetColType' not in c:
    c=c.replace('''void TableControl::SetColumns(const std::vector<int>& widths, const std::vector<std::wstring>& titles)
{ m_colW = widths; m_colTitle = titles; Invalidate(); }''',
'''void TableControl::SetColumns(const std::vector<int>& widths, const std::vector<std::wstring>& titles)
{
    m_colW = widths;
    m_colTitle = titles;
    m_colType.assign(widths.size(), ColType::Text);   // 未指定前一律文本
    Invalidate();
}''')
    c=c.replace('''void TableControl::AppendRow(const std::vector<std::wstring>& cells)''',
'''// ---------------------------- 列类型 ----------------------------
namespace
{
    std::wstring TrimCopy(const std::wstring& s)
    {
        size_t a = s.find_first_not_of(L" \\t\\r\\n");
        if (a == std::wstring::npos) return L"";
        size_t b = s.find_last_not_of(L" \\t\\r\\n");
        return s.substr(a, b - a + 1);
    }

    bool ParseInt(const std::wstring& s, long long& out)
    {
        const std::wstring t = TrimCopy(s);
        if (t.empty()) return false;
        wchar_t* end = nullptr;
        const long long v = ::_wcstoi64(t.c_str(), &end, 10);
        if (end == nullptr || *end != L'\\0') return false;
        out = v;
        return true;
    }

    bool ParseDouble(const std::wstring& s, double& out)
    {
        const std::wstring t = TrimCopy(s);
        if (t.empty()) return false;
        wchar_t* end = nullptr;
        const double v = ::wcstod(t.c_str(), &end);
        if (end == nullptr || *end != L'\\0') return false;
        out = v;
        return true;
    }

    bool ParseBool(const std::wstring& s, bool& out)
    {
        const std::wstring t = TrimCopy(s);
        if (t == L"1" || t == L"true" || t == L"True" || t == L"TRUE" || t == L"是") { out = true;  return true; }
        if (t == L"0" || t == L"false" || t == L"False" || t == L"FALSE" || t == L"否") { out = false; return true; }
        return false;
    }
}

void TableControl::SetColType(int col, ColType t)
{
    if (col < 0) return;
    if (static_cast<int>(m_colType.size()) <= col) m_colType.resize(col + 1, ColType::Text);
    m_colType[col] = t;
    Invalidate();
}

void TableControl::SetColTypes(const std::vector<int>& types)
{
    m_colType.assign(types.size(), ColType::Text);
    for (size_t i = 0; i < types.size(); ++i)
        m_colType[i] = static_cast<ColType>(types[i]);
    Invalidate();
}

ColType TableControl::ColTypeOf(int col) const
{
    if (col < 0 || col >= static_cast<int>(m_colType.size())) return ColType::Text;
    return m_colType[col];
}

const std::wstring TableControl::GetText(int row, int col) const
{
    if (row < 0 || row >= Rows() || col < 0 || col >= static_cast<int>(m_rows[row].size()))
        return L"";
    return m_rows[row][col];
}

bool TableControl::GetInt(int row, int col, long long& out) const
{
    const std::wstring t = GetText(row, col);
    if (ParseInt(t, out)) return true;
    double d = 0.0;
    if (ParseDouble(t, d)) { out = static_cast<long long>(d); return true; }
    return false;
}

bool TableControl::GetDouble(int row, int col, double& out) const
{
    const std::wstring t = GetText(row, col);
    if (ParseDouble(t, out)) return true;
    long long i = 0;
    if (ParseInt(t, i)) { out = static_cast<double>(i); return true; }
    return false;
}

bool TableControl::GetBool(int row, int col, bool& out) const
{
    return ParseBool(GetText(row, col), out);
}

int TableControl::CompareCell(int r1, int c, int r2) const
{
    const ColType t = ColTypeOf(c);
    if (t == ColType::Int)
    {
        long long a = 0, b = 0;
        if (GetInt(r1, c, a) && GetInt(r2, c, b)) return (a < b) ? -1 : (a > b ? 1 : 0);
    }
    else if (t == ColType::Double)
    {
        double a = 0, b = 0;
        if (GetDouble(r1, c, a) && GetDouble(r2, c, b)) return (a < b) ? -1 : (a > b ? 1 : 0);
    }
    else if (t == ColType::Bool)
    {
        bool a = false, b = false;
        if (GetBool(r1, c, a) && GetBool(r2, c, b)) return (a == b) ? 0 : (a ? 1 : -1);
    }
    const std::wstring x = GetText(r1, c), y = GetText(r2, c);
    return (x < y) ? -1 : (x > y ? 1 : 0);
}

void TableControl::AppendRow(const std::vector<std::wstring>& cells)''')
    # 绘制：数值列右对齐
    c=c.replace('''            const std::wstring t = (c < static_cast<int>(m_rows[r].size())) ? m_rows[r][c] : L"";
            ::DrawTextW(dc, t.c_str(), -1, &cellRc, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);''',
'''            const std::wstring t = (c < static_cast<int>(m_rows[r].size())) ? m_rows[r][c] : L"";
            const ColType ct = ColTypeOf(c);
            const UINT align = (ct == ColType::Int || ct == ColType::Double) ? DT_RIGHT
                            : (ct == ColType::Bool ? DT_CENTER : DT_LEFT);
            ::DrawTextW(dc, t.c_str(), -1, &cellRc, align | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);''')
    c=c.replace('''        const std::wstring t = i < static_cast<int>(m_colTitle.size()) ? m_colTitle[i] : L"";
        ::DrawTextW(dc, t.c_str(), -1, &cellRc, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);''',
'''        const std::wstring t = i < static_cast<int>(m_colTitle.size()) ? m_colTitle[i] : L"";
        const ColType ct = ColTypeOf(i);
        const UINT align = (ct == ColType::Int || ct == ColType::Double) ? DT_RIGHT
                        : (ct == ColType::Bool ? DT_CENTER : DT_LEFT);
        ::DrawTextW(dc, t.c_str(), -1, &cellRc, align | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);''')
    io.open(p2,'w',encoding='utf-8-sig',newline='').write(c)
print('列类型枚举与存取已加入')
