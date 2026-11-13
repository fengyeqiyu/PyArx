import io
p='G:/PyArx/PyMfcXaml/Table.h'
s=io.open(p,encoding='utf-8-sig').read()
old='''    void ApplySort(int col, bool desc);
    int  MaxScroll() const { return (std::max)(0, ViewRows() * m_rowH - (Rect().bottom - m_headerH)); }
    void ScrollBy(int dy) { m_scrollTop += dy; const int mx = MaxScroll();
                            if (m_scrollTop < 0) m_scrollTop = 0; if (m_scrollTop > mx) m_scrollTop = mx; Invalidate(); }'''
new='''    void ApplySort(int col, bool desc);'''
assert s.count(old)==1
s=s.replace(old,new,1)
io.open(p,'w',encoding='utf-8-sig',newline='').write(s)
print('去重复定义')
