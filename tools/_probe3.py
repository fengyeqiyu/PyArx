import io
P = r'G:\PyArx\PyMfcXaml\PyMfcXaml.cpp'
t = io.open(P, encoding='utf-8-sig').read()
print('A', t.count("    void setCurSel(int id, int index) { m_host.setCurSel(id, index); }"))
print('B', t.count("    void setCurSel(int id, int index) { m_host->setCurSel(id, index); }"))
