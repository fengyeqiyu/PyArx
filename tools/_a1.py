import io, re
def sub_once(path, old, new):
    s = io.open(path, encoding='utf-8-sig').read()
    n = s.count(old)
    if n < 1:
        print('FAIL', path, repr(old[:40])); return False
    s = s.replace(old, new, 1)
    io.open(path, 'w', encoding='utf-8-sig', newline='').write(s)
    print('OK', path); return True

sub_once('G:/PyArx/PyMfcXaml/PyMfcHost.h',
"void moveMany(py::list items);",
"""void moveMany(py::list items);

    // 父表面绘制的装饰矩形：没有自己的 HWND，由父窗口在 OnEraseBkgnd 里铺底色；
    // z 序由父表面统一保证（分割条不会被表格这类兄弟窗口盖掉），也省掉一次独立 WM_PAINT。
    struct SurfaceItem
    {
        RECT rc = { 0, 0, 0, 0 };
        COLORREF fill = RGB(255, 255, 255);
        bool visible = true;
    };
    std::map<int, SurfaceItem> m_surfaces;
    std::vector<int> m_surfaceOrder;
    void setSurface(int id, int x, int y, int w, int h, int fill);
    void moveSurfaces(py::list items);
    py::tuple getSurfaceRect(int id);""")
