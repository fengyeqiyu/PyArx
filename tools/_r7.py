import io,os,re,subprocess,sys
D=r'G:\PyArx\tests\mfc_xaml'
PY=r'D:\Program Files\Python314\python.exe'
def run():
    try:
        p=subprocess.run([PY,'pp_run.py'],cwd=D,capture_output=True,timeout=10)
        return p.returncode, p.stderr.decode('utf-8','ignore')
    except subprocess.TimeoutExpired:
        return None, ''      # 超时=窗口已经在跑，说明成功
def patch(old,new):
    n=0
    for fn in os.listdir(D):
        if not fn.endswith('.py'): continue
        q=os.path.join(D,fn); t=io.open(q,encoding='utf-8').read(); t2=t
        t2=re.sub(r'(?<![\w.])%s\b'%re.escape(old), new, t2)
        t2=re.sub(r'\.%s\b'%re.escape(old), '.'+new, t2)
        if t2!=t: io.open(q,'w',encoding='utf-8').write(t2); n+=1
    return n
for i in range(30):
    rc,err=run()
    if rc is None: print('OK 窗口已启动（第%d轮）'%i); break
    m=re.findall(r"has no attribute '([A-Za-z0-9_]+)'.*?Did you mean: '([A-Za-z0-9_]+)'",err,re.S)
    if not m:
        print('无法自动修复，剩余错误：'); print(err[-1200:]); break
    old,new=m[-1]
    n=patch(old,new)
    print('修正 %s -> %s（%d 个文件）'%(old,new,n))
