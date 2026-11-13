# -*- coding: utf-8 -*-
import csv, math, os
DOC = r"G:\PyArx\docs"
H = ["起点","回路","终点","型号规格","电压等级","电缆根数","水平长度","垂直长度","余量","合计","功率"]
def row(s,l,e,spec,v,n,h,vv,sp,p):
    return [s,l,e,spec,v,n,h,vv,sp,int(math.ceil(h+vv+sp)),p]
rows=[row("","WL-GRID","GRID","-","10kV 中压",1,0,0,0,0)]
for i in range(1,21):
    rows.append(row("GRID","WL-TR%d"%i,"TR%d"%i,"YJV22-4×95","10kV/380V 三相",1,3,2,2,1.5))
for i in range(1,21):
    for j in range(1,11):
        rows.append(row("TR%d"%i,"WL-AT%d-%02d"%(i,j),"AT%d-%d"%(i,j),"YJV22-4×35","380V 三相",1,5+(i+j)%6,2,2,round(1.5*j,1)))
for i in range(1,21):
    for j in range(1,11):
        for k in range(1,41):
            rows.append(row("AT%d-%d"%(i,j),"WL-AT%d-%d-%02d"%(i,j,k),"AT%d-%d-%d"%(i,j,k),
                            "BV-3×4" if k%3==0 else "YJV22-4×25",
                            "220V 单相" if k%2 else "380V 三相",1,4+(i+j+k)%9,2,2,round(1.0*k%5+1,1)))
p=os.path.join(DOC,"_tree3_20_10_40_allstart.csv")
with open(p,"w",newline="",encoding="utf-8") as f:
    w=csv.writer(f); w.writerow(H); w.writerows(rows)
print(p, len(rows), "行")
