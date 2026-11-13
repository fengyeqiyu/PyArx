import io,re,shutil,os
bk='G:/PyArx/docs/_bk4_pp_app.py'
p='G:/PyArx/tests/mfc_xaml/pp_app.py'
shutil.copyfile(bk,p)
s=io.open(p,encoding='utf-8').read()

def camel(name):
    lead=''
    while name.startswith('_'): lead+='_'; name=name[1:]
    parts=name.split('_')
    if len(parts)==1: return lead+name
    return lead+parts[0]+''.join(w[:1].upper()+w[1:] for w in parts[1:] if w)

# 只改本文件里定义的名字：def 名 + 类属性/方法引用
names=set()
for m in re.finditer(r'\bdef ([A-Za-z_][A-Za-z0-9_]*)\s*\(', s):
    n=m.group(1)
    if '_' in n and not n.startswith('__'): names.add(n)
mp={n:camel(n) for n in names}
mp={k:v for k,v in mp.items() if k!=v}

s2=s
for k,v in sorted(mp.items(), key=lambda kv:-len(kv[0])):
    # def 名、self./cls./实例属性 引用，以及裸调用；不碰 os.xxx / 外部模块属性
    s2=re.sub(r'(?<![\w.])%s\b' % re.escape(k), v, s2)
    s2=re.sub(r'\b(self|cls)\.%s\b' % re.escape(k), r'\1.'+v, s2)
io.open(p,'w',encoding='utf-8').write(s2)
print('重命名 def 名 %d 个' % len(mp))

# 同目录其他脚本：只替换 .method 引用与裸调用
for fn in os.listdir('G:/PyArx/tests/mfc_xaml'):
    if not fn.endswith('.py') or fn=='pp_app.py': continue
    q='G:/PyArx/tests/mfc_xaml/'+fn
    t=io.open(q,encoding='utf-8').read()
    t2=t
    for k,v in mp.items():
        t2=re.sub(r'(?<![\w.])%s\b' % re.escape(k), v, t2)
        t2=re.sub(r'\.%s\b' % re.escape(k), '.'+v, t2)
    if t2!=t:
        io.open(q,'w',encoding='utf-8').write(t2)
print('同步完成')
