import io,re,os
mp={'cur_sel':'curSel','cv_h':'cvH','cv_w':'cvW','scene_text':'sceneText','show_cable':'showCable','title_text':'titleText'}
p='G:/PyArx/tests/mfc_xaml/pp_app.py'
s=io.open(p,encoding='utf-8').read()
for k,v in mp.items():
    s=re.sub(r'(?<![\w.])%s\b'%k, v, s)
    s=re.sub(r'\b(self|cls)\.%s\b'%k, r'\1.'+v, s)
io.open(p,'w',encoding='utf-8').write(s)
for fn in os.listdir('G:/PyArx/tests/mfc_xaml'):
    if not fn.endswith('.py') or fn=='pp_app.py': continue
    q='G:/PyArx/tests/mfc_xaml/'+fn
    t=io.open(q,encoding='utf-8').read(); t2=t
    for k,v in mp.items():
        t2=re.sub(r'(?<![\w.])%s\b'%k, v, t2); t2=re.sub(r'\.%s\b'%k, '.'+v, t2)
    if t2!=t: io.open(q,'w',encoding='utf-8').write(t2)
print('属性去下划线完成')
