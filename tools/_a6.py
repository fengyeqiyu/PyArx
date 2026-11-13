import io
p='G:/PyArx/PyMfcXaml/PyMfcProcs.cpp'
s=io.open(p,encoding='utf-8-sig').read()
old="""    static D2DDcCanvas* d2dCanvasPtr = new D2DDcCanvas();  // 故意不析构
    D2DDcCanvas& d2dCanvas = *d2dCanvasPtr;"""
new="""    // 与 L1103 同一个根因：全进程一个画布、控件尺寸轮流变 -> 每帧 releaseTarget()
    // 重建目标与全部笔刷/文字格式。这里按控件 id 各持一个，尺寸不变时只 BindDC。
    static std::map<int, D2DDcCanvas*> s_d2dCanvases;
    D2DDcCanvas*& d2dCanvasPtr = s_d2dCanvases[cid];
    if (d2dCanvasPtr == nullptr) d2dCanvasPtr = new D2DDcCanvas();
    D2DDcCanvas& d2dCanvas = *d2dCanvasPtr;"""
assert s.count(old)==1
s=s.replace(old,new,1)
io.open(p,'w',encoding='utf-8-sig',newline='').write(s)
print('owner-draw 画布也按控件缓存')
