# -*- coding: utf-8 -*-
hdl=open(r"G:\PyArx\examples\cad2d\dwg1018\hdl_full.bin","rb").read()
print("raw 2..60:",hdl[2:60].hex(' '))
# real locs by handle
real={1:2890,2:7614,3:98837,5:43053,0x12:363755,0x1b:134618,0x20:443689}
# encode MC 2802, 4724, 35439, ...
def mc_enc(v):
    neg=v<0; v=abs(v); b=[0]*5
    j=0
    for i in range(5): b[i]=(v>>j)&0x7f; j+=7
    top=4
    while top>0 and not b[top]: top-=1
    if b[top]&0x40 and top>0: top-=1
    for i in range(top): b[i]|=0x80
    if neg: b[top]|=0x40
    return bytes(reversed(b[:top+1]))
loc=0
for h,addr in sorted(real.items()):
    d=addr-loc
    print(hex(h),"addr",addr,"delta",d,"mc_enc",mc_enc(d).hex())
    loc=addr
