import io
# 头：两处 addTableRow 后各插一行（PyMfcWindow / PyMfcPanel）
p = 'G:/PyArx/PyMfcXaml/PyMfcWrap.h'
s = io.open(p, encoding='utf-8').read()
old = "void addTableRow(int id, const std::wstring& s);"
new = """void addTableRow(int id, const std::wstring& s);
void setTableRows(int id, const std::wstring& blob);"""
assert s.count(old) == 2, s.count(old)
s = s.replace(old, new)
io.open(p, 'w', encoding='utf-8', newline='').write(s)

# 实现：两处转发
p = 'G:/PyArx/PyMfcXaml/PyMfcWrap.cpp'
s = io.open(p, encoding='utf-8-sig').read()
old1 = """void PyMfcWindow::addTableRow(int id, const std::wstring& s)
{ m_host.addTableRow(id, s); }"""
new1 = """void PyMfcWindow::addTableRow(int id, const std::wstring& s)
{ m_host.addTableRow(id, s); }

void PyMfcWindow::setTableRows(int id, const std::wstring& blob)
{ m_host.setTableRows(id, blob); }"""
assert s.count(old1) == 1
s = s.replace(old1, new1)

old2 = """void PyMfcPanel::addTableRow(int id, const std::wstring& s)
{ m_host->addTableRow(id, s); }"""
new2 = """void PyMfcPanel::addTableRow(int id, const std::wstring& s)
{ m_host->addTableRow(id, s); }

void PyMfcPanel::setTableRows(int id, const std::wstring& blob)
{ m_host->setTableRows(id, blob); }"""
assert s.count(old2) == 1
s = s.replace(old2, new2)
io.open(p, 'w', encoding='utf-8-sig', newline='').write(s)

# 绑定：两处 addTableRow .def 后各加一条
p = 'G:/PyArx/PyMfcXaml/PyMfcBind.cpp'
s = io.open(p, encoding='utf-8').read()
old3 = '.def("addTableRow", &PyMfcWindow::addTableRow)'
new3 = """.def("addTableRow", &PyMfcWindow::addTableRow)
            .def("setTableRows", &PyMfcWindow::setTableRows,
                 "整表一次灌入：行间\\\\n、格间\\\\t")"""
assert s.count(old3) == 1
s = s.replace(old3, new3)
old4 = '.def("addTableRow", &PyMfcPanel::addTableRow)'
new4 = """.def("addTableRow", &PyMfcPanel::addTableRow)
            .def("setTableRows", &PyMfcPanel::setTableRows,
                 "整表一次灌入：行间\\\\n、格间\\\\t")"""
assert s.count(old4) == 1
s = s.replace(old4, new4)
io.open(p, 'w', encoding='utf-8', newline='').write(s)
print('wrap + bind 已加 setTableRows')
