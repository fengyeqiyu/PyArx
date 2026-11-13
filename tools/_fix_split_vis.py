import io
# C++：竖条按宽度决定画法 —— 宽条整块铺浅灰 + 中间 1px 深线；细条只画 1px
p = 'G:/PyArx/PyMfcXaml/PyMfcProcs.cpp'
s = io.open(p, encoding='utf-8-sig').read()
old = """        if (hSpan > wSpan * 3)
        {
            const int x = (itemRc.left + itemRc.right) / 2;
            cv->fillRect(RECT{ x, itemRc.top, x + 1, itemRc.bottom }, m_border);
        }"""
new = """        if (hSpan > wSpan * 3)
        {
            // 宽条（可拖动分割条）整块铺一条可见的浅灰，再压 1px 深线；
            // 细条（1~2px）只画中间那条深线。否则 6px 分割条在浅色背景上等于没有。
            if (wSpan >= 3)
            {
                cv->fillRect(itemRc, RGB(0xE4, 0xE7, 0xEC));
                const int x = (itemRc.left + itemRc.right) / 2;
                cv->fillRect(RECT{ x, itemRc.top, x + 1, itemRc.bottom }, m_border);
            }
            else
            {
                const int x = (itemRc.left + itemRc.right) / 2;
                cv->fillRect(RECT{ x, itemRc.top, x + 1, itemRc.bottom }, m_border);
            }
        }"""
assert s.count(old) == 1
s = s.replace(old, new)
io.open(p, 'w', encoding='utf-8-sig', newline='').write(s)

# Python：SPLIT 自己就是那条可见分割条（不再需要单独的 1px 线控件）
p = 'G:/PyArx/tests/mfc_xaml/pp_app.py'
s = io.open(p, encoding='utf-8').read()
old2 = """        w.addPanel(SPLIT, ED_X - SPLIT_W, ED_Y, SPLIT_W, ED_H)
        w.addStaticLine(L_SPLIT, ED_X - 1, ED_Y, 2, ED_H)"""
new2 = """        w.addStaticLine(SPLIT, ED_X - SPLIT_W, ED_Y, SPLIT_W, ED_H)"""
assert s.count(old2) == 1
s = s.replace(old2, new2)
old3 = "            (L_SPLIT, ed_x - 1, ed_y, 2, ed_h, True),\n"
assert s.count(old3) == 1
s = s.replace(old3, "")
io.open(p, 'w', encoding='utf-8', newline='').write(s)
print('SPLIT 自身可见（6px 浅灰 + 1px 深线），去掉多余的 L_SPLIT')
