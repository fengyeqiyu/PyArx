# -*- coding: utf-8 -*-
import re, io

p = r'G:\PyArx\PyMfcXaml\PyMfcHost.cpp'
raw = open(p, encoding='utf-8-sig').read()

def insert_after_brace(src, sig, body):
    needle = sig + '\n{'
    i = src.find(needle)
    if i < 0:
        needle = sig + '\r\n{'
        i = src.find(needle)
    if i < 0:
        raise SystemExit('NOT FOUND: ' + sig)
    brace = src.find('{', i)
    nl = src.find('\n', brace)
    return src[:nl+1] + body + src[nl+1:]

fb_count = '        if (findNewTable(id)) return tableRowCount(id);\n'
fb_cellg = '        if (TableControl* nt = findNewTable(id)) return nt->GetText(row, col);\n'
fb_cells = '        if (TableControl* nt = findNewTable(id)) { nt->SetCell(row, col, text); return; }\n'
fb_selg  = '        if (TableControl* nt = findNewTable(id)) return nt->Selected();\n'
fb_sels  = '        if (TableControl* nt = findNewTable(id)) { nt->Select(row); return; }\n'
fb_rect  = '        if (TableControl* nt = findNewTable(id)) return tableCellRect(id, row, col);\n'
fb_clear = '        if (TableControl* nt = findNewTable(id)) { nt->Clear(); return; }\n'

src = raw
src = insert_after_brace(src, 'int PyMfcHost::getTableRowCount(int id) const', fb_count)
src = insert_after_brace(src, 'std::wstring PyMfcHost::getTableCell(int id, int row, int col) const', fb_cellg)
src = insert_after_brace(src, 'void PyMfcHost::setTableCell(int id, int row, int col, const std::wstring& text)', fb_cells)
src = insert_after_brace(src, 'int PyMfcHost::getTableSelectedRow(int id) const', fb_selg)
src = insert_after_brace(src, 'void PyMfcHost::setTableSelectedRow(int id, int row)', fb_sels)
src = insert_after_brace(src, 'py::object PyMfcHost::cellRect(int id, int row, int col)', fb_rect)
src = insert_after_brace(src, 'void PyMfcHost::clearTableRows(int id)', fb_clear)

# setTableRows：解析 rows 后，新表走 Clear + AppendRow
anchor = '        m_tblScroll[id] = 0;\n'
if anchor not in src:
    anchor = '        m_tblScroll[id] = 0;\r\n'
ins = anchor + '        if (TableControl* nt = findNewTable(id)) { nt->Clear(); for (const auto& rr : rows) nt->AppendRow(rr); return; }\n'
assert anchor in src
src = src.replace(anchor, ins, 1)

open(p, 'w', encoding='utf-8-sig', newline='').write(src)
print('OK')
