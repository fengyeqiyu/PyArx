import sys
d = open(r'G:\PyArx\samples\dwg_versions\v_2000.dwg','rb').read()
class R:
    def __init__(s,a,p=0): s.a=a; s.p=p
    def b(s,n=1):
        v=0
        for _ in range(n):
            v=(v<<1)|((d[s.a+s.p//8]>>(7-s.p%8))&1); s.p+=1
        return v
    def rc(s): return s.b(8)
    def rs(s):
        lo=s.rc(); return (s.rc()<<8)|lo
    def bs(s):
        t=s.b(2)
        return s.rs() if t==0 else s.b(8) if t==1 else 0 if t==2 else 256
for a in (12938 and 0,):
    pass
def dump(a):
    r=R(a); size=r.rs(); t=r.bs(); bs=r.bs()  # bs approximates rl for small
    r.rb = None
    print(f'addr={a} size={size} type={t} bitsize={bs}')
    # restart to parse properly
    r=R(a); size=r.rs(); t=r.bs(); bsv=r.rs()|(r.rs()<<16)
    c=r.rc(); n=c&15; hv=0
    for _ in range(n): hv=(hv<<8)|r.rc()
    eed=r.bs()
    print(f'  eed={eed} @{r.p}')
    # common
    pv=r.b(); em=r.b(2); nr=r.bs(); nol=r.b(); col=r.bs(); lt=r.b(2);
    if lt==0:
        r.rs(); r.rs()
    lf=r.b(2); pf=r.b(2); inv=r.bs(); lw=r.rc()
    print(f'  common end={r.p} pv={pv} em={em} nr={nr} nol={nol} col={col} ltflag={lt} lf={lf} pf={pf} inv={inv} lw={lw}')
    gs=r.p
    if t==4:
        ln=r.bs()
        s=''.join(chr(r.rc()) for _ in range(ln))
        print(f'  name len={ln} "{s}" geomEnd={r.p} geomBits={r.p-gs} bitsize={bsv} hdlStart={16+bsv}')
    else:
        print(f'  geomStart={gs} bitsize={bsv} hdlStart={16+bsv}')
dump(14194985); dump(14368480); dump(130938)
