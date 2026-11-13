import io, re
p='G:/PyArx/PyMfcXaml/PyMfcProcs.cpp'
s=io.open(p,encoding='utf-8-sig').read()
# 找到"尺寸变化就重建 DIB"的那个判断，改成只增不减
pat = re.compile(r"if \(s_memDc == nullptr \|\| (?:nw != s_memW \|\| nh != s_memH|s_memW != nw \|\| s_memH != nh|nw != s_memW \|\| s_memH != nh|s_memW != nw \|\| nh != s_memH)\)")
m = pat.search(s)
if m:
    s = s[:m.start()] + "if (s_memDc == nullptr || nw > s_memW || nh > s_memH)   // 只增不减：拖动时尺寸每帧在变，重建 DIB+DC 就是那 13 ms" + s[m.end():]
    io.open(p,'w',encoding='utf-8-sig',newline='').write(s)
    print('OK 改为只增不减', m.group(0))
else:
    i = s.find('s_memW')
    print('FAIL 没命中，附近原文：')
    print(s[max(0,i-420):i+120])
    raise SystemExit(1)
