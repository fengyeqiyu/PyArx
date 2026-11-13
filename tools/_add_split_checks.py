import io
p = 'G:/PyArx/tests/mfc_xaml/pp_split_test.py'
s = io.open(p, encoding='utf-8').read()
old = """# 画布与侧板不重叠（分割条夹在中间）"""
new = """# 边界是算出来的，不是写死的：上限必须给画布留 CV_MIN_W
lo, hi = app._side_bounds(1324)
chk('窗口 1324 时侧板上限 = 900', hi == 900, 'lo=%d hi=%d' % (lo, hi))
lo2, hi2 = app._side_bounds(1000)
chk('窗口 1000 时上限收缩到给画布留 320',
    hi2 == 1000 - pp_app.CV_X - 8 - pp_app.SPLIT_W - 320, 'lo=%d hi=%d' % (lo2, hi2))
prev, app._ed_w = app._ed_w, 900
cw_small = 1000
chk('窄窗下生效宽度被钳住、偏好不动',
    app._side_width(cw_small) == hi2 and app._ed_w == 900,
    'eff=%d pref=%d' % (app._side_width(cw_small), app._ed_w))
app._ed_w = prev

# 画布与侧板不重叠（分割条夹在中间）"""
assert s.count(old) == 1
s = s.replace(old, new)
io.open(p, 'w', encoding='utf-8', newline='').write(s)
print('pp_split_test.py 增加 3 项算法断言')
