# -*- coding: utf-8 -*-
import csv, math, os
DOC = r"G:\PyArx\docs"
H = ["起点","回路","终点","型号规格","电压等级","电缆根数","水平长度","垂直长度","余量","合计","功率"]
def row(s,l,e,spec,v,n,h,vv,sp,p):
    return [s,l,e,spec,v,n,h,vv,sp,int(math.ceil(h+vv+sp)),p]
def build(n1, m2, k3, path):
    rows=[row("","WL-GRID","GRID","-","10kV 中压",1,0,0,0,0)]
    for i in range(1,n1+1):
        rows.append(row("GRID","WL-TR%d"%i,"TR%d"%i,"YJV22-4×95","10kV/380V 三相",1,3,2,2,1.5))
    for i in range(1,n1+1):
        for j in range(1,m2+1):
            rows.append(row("TR%d"%i,"WL-AT%d-%02d"%(i,j),"AT%d-%d"%(i,j),"YJV22-4×35","380V 三相",1,5+(i+j)%6,2,2,round(1.5*j,1)))
    for i in range(1,n1+1):
        for j in range(1,m2+1):
            for k in range(1,k3+1):
                rows.append(row("AT%d-%d"%(i,j),"WL-AT%d-%d-%02d"%(i,j,k),"AT%d-%d-%d"%(i,j,k),
                                "BV-3×4" if k%3==0 else "YJV22-4×25",
                                "220V 单相" if k%2 else "380V 三相",1,4+(i+j+k)%9,2,2,round(1.0*k%5+1,1)))
    with open(path,"w",newline="",encoding="utf-8") as f:
        w=csv.writer(f); w.writerow(H); w.writerows(rows)
    print(path, len(rows), "行，空起点行",
          sum(1 for r in rows if not r[0]))
build(20,10,50, os.path.join(DOC,"_big_20_10_50_allstart.csv"))
