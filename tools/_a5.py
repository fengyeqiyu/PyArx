import io
p='G:/PyArx/PyMfcXaml/PyMfcProcs.cpp'
s=io.open(p,encoding='utf-8-sig').read()
old="static D2DDcCanvas* canvasPtr = new D2DDcCanvas();  // 故意不析构"
new="""// 画布（含 D2D 渲染目标与笔刷/文字格式缓存）按"控件 id"复用。
    // 原来全进程只有一个，十几尺寸不同的控件轮流用：每次尺寸一变就 releaseTarget()，
    // 把目标 + 所有笔刷 + 文字格式重建一遍，一块纯色矩形也要 16 ms 就是这么来的。
    static std::map<int, D2DDcCanvas*> s_ctlCanvases;
    D2DDcCanvas*& canvasPtr = s_ctlCanvases[::GetDlgCtrlID(child)];
    if (canvasPtr == nullptr) canvasPtr = new D2DDcCanvas();"""
assert s.count(old)==1
s=s.replace(old,new,1)
io.open(p,'w',encoding='utf-8-sig',newline='').write(s)
print('per-control canvas cache')
