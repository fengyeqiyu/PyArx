import io,re,shutil,os
p='G:/PyArx/tests/mfc_xaml/pp_app.py'
shutil.copyfile(p,'G:/PyArx/docs/_bk4_pp_app.py')
s=io.open(p,encoding='utf-8').read()

def camel(name):
    lead=''
    while name.startswith('_'): lead+='_'; name=name[1:]
    parts=name.split('_')
    if len(parts)==1: return lead+name
    out=parts[0]+''.join(w[:1].upper()+w[1:] for w in parts[1:] if w)
    return lead+out

ID=re.compile(r'[A-Za-z_][A-Za-z0-9_]*')
changed={}
def repl(m):
    w=m.group(0)
    if '_' not in w: return w
    if w.startswith('__') or w.endswith('__'): return w
    if w.upper()==w: return w          # 常量保持原样
    c=camel(w)
    if c!=w: changed[w]=c
    return c
s2=ID.sub(repl,s)
io.open(p,'w',encoding='utf-8').write(s2)
print('pp_app 重命名 %d 个标识符' % len(changed))
# 其他同目录脚本里引用这些名字的同步替换
for fn in os.listdir('G:/PyArx/tests/mfc_xaml'):
    if not fn.endswith('.py') or fn=='pp_app.py': continue
    q='G:/PyArx/tests/mfc_xaml/'+fn
    t=io.open(q,encoding='utf-8').read()
    t2=ID.sub(lambda m: changed.get(m.group(0),m.group(0)), t)
    if t2!=t:
        io.open(q,'w',encoding='utf-8').write(t2)
        print('同步',fn)
