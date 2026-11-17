import io
p = r'G:\PyArx\UIDesign\DesignerPaneSurface.cpp'
s = io.open(p, encoding='utf-8', newline='').read()
old_lf = "    if (!cv->begin(client.Width(), client.Height()))\n        return;"
new_lf = ("    if (!cv->begin(client.Width(), client.Height()))\n"
          "    {\n"
          '        diagLog("[paneSurface] begin FAIL %dx%d hdc=%p\\n", client.Width(), client.Height(), (void*)hdc);\n'
          "        return;")
if old_lf in s:
    s = s.replace(old_lf, new_lf, 1)
    io.open(p, 'w', encoding='utf-8', newline='').write(s)
    print('PATCHED lf')
else:
    old = old_lf.replace("\n", "\r\n")
    new = new_lf.replace("\n", "\r\n")
    if old in s:
        io.open(p, 'w', encoding='utf-8', newline='').write(s.replace(old, new, 1))
        print('PATCHED crlf')
    else:
        print('NOT FOUND')
