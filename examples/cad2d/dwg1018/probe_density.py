# -*- coding: utf-8 -*-
import sys
sys.path.insert(0, r"G:\PyArx\examples\cad2d\dwg1018")
import render_svg

el, n = render_svg.collect()
ys, xs = [], []
def add(x, y):
    xs.append(x); ys.append(y)
for x1,y1,x2,y2,_ in el["line"]: add(x1,y1); add(x2,y2)
for cx,cy,_,_ in el["circle"]: add(cx,cy)
for cx,cy,_,_,_,_ in el["arc"]: add(cx,cy)
for pts,_,_ in el["poly"]:
    for x,y in pts: add(x,y)
for pts,_ in el["spline"]:
    for x,y in pts: add(x,y)
for x,y,_,_,_,_ in el["text"]: add(x,y)

xs.sort(); ys.sort()
N=len(xs)
print("n", N)
for q in (0.01,0.05,0.1,0.25,0.5,0.75,0.9,0.95,0.99):
    i=int(q*N)
    print(f"x {q}: {xs[i]:.0f}  y {q}: {ys[i]:.0f}")

# y 直方图（20 桶）
lo,hi=ys[0],ys[-1]; step=(hi-lo)/20
import collections
h=collections.Counter(int((y-lo)/step) for y in ys)
for b in range(20):
    print(f"y[{lo+b*step:.0f},{lo+(b+1)*step:.0f}] {'#'*(h[b]//200)} {h[b]}")
