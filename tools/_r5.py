import io,re,os,importlib
mods=['os','sys','json','csv','math','time','re','datetime','subprocess','pathlib','struct',
      'ctypes','shutil','glob','random','itertools','functools','collections','urllib',
      'hashlib','logging','traceback','copy','io','tempfile','string','os.path']
def snake(n):  # camel -> snake
    s=''.join('_'+c.lower() if c.isupper() else c for c in n)
    return s
fixed_files=0; fixed_names=0
d='G:/PyArx/tests/mfc_xaml'
for fn in os.listdir(d):
    if not fn.endswith('.py'): continue
    q=os.path.join(d,fn)
    t=io.open(q,encoding='utf-8').read()
    t2=t
    for mod in mods:
        pat=r'\b%s\.([A-Za-z_][A-Za-z0-9_]*)' % re.escape(mod)
        def fix(m):
            global fixed_names
            name=m.group(1)
            try:
                obj=importlib.import_module(mod.replace('.path','')) if mod!='os.path' else __import__('os').path
            except Exception:
                return m.group(0)
            if not hasattr(obj,name) and hasattr(obj,snake(name)):
                fixed_names+=1
                return mod+'.'+snake(name)
            return m.group(0)
        t2=re.sub(pat,fix,t2)
    if t2!=t:
        io.open(q,'w',encoding='utf-8').write(t2); fixed_files+=1
print('修复文件 %d 个，修正外部属性 %d 处' % (fixed_files,fixed_names))
