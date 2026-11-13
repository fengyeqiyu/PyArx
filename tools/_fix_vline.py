import io
p = 'G:/PyArx/PyMfcXaml/PyMfcProcs.cpp'
s = io.open(p, encoding='utf-8-sig').read()
old = """        const int y = (itemRc.top + itemRc.bottom) / 2;"""
new = """        const int y = (itemRc.top + itemRc.bottom) / 2;
        const int wSpan = static_cast<int>(itemRc.right - itemRc.left);
        const int hSpan = static_cast<int>(itemRc.bottom - itemRc.top);"""
assert s.count(old) == 1
s = s.replace(old, new)

old2 = """        cv->fillRect(itemRc, surfaceColorAt(CRect(wrParent)));
        cv->fillRect(RECT{ itemRc.left, y, itemRc.right, y + 1 }, m_border);"""
new2 = """        cv->fillRect(itemRc, surfaceColorAt(CRect(wrParent)));
        // 按形状决定方向：竖条（高 >> 宽）画竖直中线，横条画水平中线。
        // 原来一律按"横线"画，传进来的竖向分割条就只剩一条看不见的横短线。
        if (hSpan > wSpan * 3)
        {
            const int x = (itemRc.left + itemRc.right) / 2;
            cv->fillRect(RECT{ x, itemRc.top, x + 1, itemRc.bottom }, m_border);
        }
        else
        {
            cv->fillRect(RECT{ itemRc.left, y, itemRc.right, y + 1 }, m_border);
        }"""
assert s.count(old2) == 1
s = s.replace(old2, new2)
io.open(p, 'w', encoding='utf-8-sig', newline='').write(s)
print('细线支持竖向')
