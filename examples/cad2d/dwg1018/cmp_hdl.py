# -*- coding: utf-8 -*-
full=open(r"G:\PyArx\examples\cad2d\dwg1018\hdl_full.bin","rb").read()
ref=open(r"G:\PyArx\examples\cad2d\dwg1018\hdl_ref.bin","rb").read()
print("full len",len(full),"ref len",len(ref))
n=min(len(full),len(ref))
diff=[i for i in range(n) if full[i]!=ref[i]]
print("diff count",len(diff),"first",diff[:10])
if diff:
    i=diff[0]
    print("ctx full:",full[i-4:i+12].hex(' '))
    print("ctx ref :",ref[i-4:i+12].hex(' '))
