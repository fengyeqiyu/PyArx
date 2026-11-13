import io
P = r'G:\PyArx\PyMfcXaml\PyMfcXaml.cpp'
t = io.open(P, encoding='utf-8-sig').read()
bad = '''        .def("getPlanView", &PyMfcWindow::getPlanView,
        .def("planProbe", &PyMfcWindow::planProbe, py::arg("id"),
             "画布鼠标诊断：(lastMsg, upX, upY, wasDrag, moved, lastHit)")
             "视口参数 (zoom, ox, oy)")'''
good = '''        .def("getPlanView", &PyMfcWindow::getPlanView,
             "视口参数 (zoom, ox, oy)")
        .def("planProbe", &PyMfcWindow::planProbe, py::arg("id"),
             "画布鼠标诊断：(lastMsg, upX, upY, wasDrag, moved, lastHit)")'''
assert t.count(bad) == 1
t = t.replace(bad, good)
io.open(P, 'w', encoding='utf-8-sig', newline='').write(t)
print('def 位置修正')
