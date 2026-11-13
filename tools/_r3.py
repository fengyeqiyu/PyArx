import io,re,shutil
p='G:/PyArx/tests/mfc_xaml/pp_app.py'
shutil.copyfile(p,'G:/PyArx/docs/_bk5_pp_app.py')
s=io.open(p,encoding='utf-8').read()

def camel(name):
    lead=''
    while name.startswith('_'): lead+='_'; name=name[1:]
    parts=[x for x in name.split('_') if x]
    if len(parts)<=1: return lead+name
    return lead+parts[0]+''.join(w[:1].upper()+w[1:] for w in parts[1:])

# 关键字实参名字（外部 API 的 kwarg）不改
kwargs=set(re.findall(r'([A-Za-z_][A-Za-z0-9_]*)\s*=(?!=)', s))

changed={}
def repl(m):
    w=m.group(0)
    if '_' not in w or w.startswith('__') or w.endswith('__'): return w
    if w.upper()==w: return w              # 常量保持
    if w in kwargs: return w               # kwarg 保持
    c=camel(w).lstrip('_') if camel(w).startswith('_') else camel(w)
    if c!=w: changed[w]=c
    return c

# 不动前面是点的外部属性（但 self./cls. 要改）
def sub_all(text):
    # 先处理 self./cls. 之后的名字
    text=re.sub(r'\b(self|cls)\.([A-Za-z_][A-Za-z0-9_]*)',
                lambda m: m.group(1)+'.'+repl(type('X',(),{'group':lambda s,i,w=m.group(2):w})()  if False else m),
                text)
    return text

# 简化：分两步
s1=re.sub(r'\b(self|cls)\.([A-Za-z_][A-Za-z0-9_]*)',
          lambda m: m.group(1)+'.'+ (camel(m.group(2)).lstrip('_') if ('_' in m.group(2) and m.group(2).upper()!=m.group(2) and not m.group(2).startswith('__') and m.group(2) not in kwargs) else m.group(2)),
          s)
changed.update({k:camel(k).lstrip('_') for k in re.findall(r'\b(?:self|cls)\.([A-Za-z_][A-Za-z0-9_]*)', s) if '_' in k and k.upper()!=k and not k.startswith('__') and k not in kwargs})
s2=re.sub(r'(?<![\w.])[A-Za-z_][A-Za-z0-9_]*', repl, s1)
io.open(p,'w',encoding='utf-8').write(s2)
print('pp_app 去下划线 %d 个' % len(changed))
print(sorted(changed.items())[:8])
