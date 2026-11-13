import io
P = r'G:\PyArx\PyMfcXaml\PyMfcXaml.cpp'
t = io.open(P, encoding='utf-8-sig').read()
old = """static LRESULT CALLBACK PyMfcOwnerDrawProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    // 配电画布先接管鼠标：它要自己处理滚轮缩放与拖动平移，不能再转给 Static 的类过程"""
new = """static LRESULT CALLBACK PyMfcOwnerDrawProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    // Static / SysLink 的类过程默认把 WM_NCHITTEST 回成 HTTRANSPARENT —— 意思是
    // "这点我透明，把点击交给下面的窗口"。我们用 SS_OWNERDRAW 自绘后没覆盖这条，
    // 于是真实鼠标全部落到父窗：卡片点不中、滚轮也滚不到；而 SendMessage 直接投递
    // 绕过命中链，所以模拟点击一直是好的（这个反差就是根因线索）。
    // 自绘控件的交互都自己处理，一律声明可点的客户区。
    if (msg == WM_NCHITTEST)
        return HTCLIENT;

    // 配电画布先接管鼠标：它要自己处理滚轮缩放与拖动平移，不能再转给 Static 的类过程"""
assert t.count(old) == 1
t = t.replace(old, new)
io.open(P, 'w', encoding='utf-8-sig', newline='').write(t)
print('WM_NCHITTEST -> HTCLIENT')
