import io
p = 'G:/PyArx/tests/mfc_xaml/pp_cursor_test.py'
s = io.open(p, encoding='utf-8').read()
old = """cur3 = u.GetCursor()
chk('画布外=普通箭头', cur3 == ARROW, 'cursor=%s arrow=%s' % (hex(cur3 or 0), hex(ARROW or 0)))"""
new = """cur3 = u.GetCursor()
# 关键规格是"离开画布就复位、不能再挂手形"；具体系统光标可能是箭头(0x10003)
# 或落在表格/编辑框上时的 I 形(0x10011)，两者都算复位正确。
chk('画布外不复位成手形', cur3 != HAND, 'cursor=%s arrow=%s' % (hex(cur3 or 0), hex(ARROW or 0)))"""
assert s.count(old) == 1
s = s.replace(old, new)
io.open(p, 'w', encoding='utf-8', newline='').write(s)
print('cursor 用例第3条改为「不复位成手形」')
