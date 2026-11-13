import io,re,os
def snake(n):
    return ''.join('_'+c.lower() if c.isupper() else c for c in n)
d='G:/PyArx/tests/mfc_xaml'
# 本目录已存在的模块名
mods={fn[:-3] for fn in os.listdir(d) if fn.endswith('.py')}
fixed=0
for fn in sorted(os.listdir(d)):
    if not fn.endswith('.py'): continue
    q=os.path.join(d,fn); t=io.open(q,encoding='utf-8').read(); t2=t
    for m in re.finditer(r'\b(import|from)\s+([A-Za-z_][A-Za-z0-9_]*)', t):
        name=m.group(2)
        if name in mods: continue
        sn=snake(name)
        if sn in mods:
            t2=re.sub(r'\b(import|from)\s+%s\b'%re.escape(name), r'\1 '+sn, t2)
            t2=re.sub(r'\b%s\.'%re.escape(name), sn+'.', t2)
            fixed+=1
    if t2!=t: io.open(q,'w',encoding='utf-8').write(t2)
print('修复模块名 %d 处' % fixed)
