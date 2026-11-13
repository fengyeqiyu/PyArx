import io
p='G:/PyArx/PyMfcXaml/PyMfcProcs.cpp'
s=io.open(p,encoding='utf-8-sig').read()
old="""        CRect sclip;
        sclip.IntersectRect(sit->second.rc, rc);"""
new="""        CRect sclip;
        CRect src(sit->second.rc);              // RECT -> CRect，走 (const RECT&, const RECT&) 重载
        sclip.IntersectRect(src, rc);"""
assert s.count(old)==1
s=s.replace(old,new,1)
io.open(p,'w',encoding='utf-8-sig',newline='').write(s)
print('fixed IntersectRect')
