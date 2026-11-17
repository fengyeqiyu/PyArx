import io, os

TARGETS = [
    (r'G:\PyArx\UIDesign\designer_layout.xml',
     [('Title="PyMfc UI Designer"', 'Title="PyArx"'),
      ('<ShellTitle Name="winTitle" Text="PyMfc Studio"/>', '<ShellTitle Name="winTitle" Text="PyArx"/>'),
      ('Header="关于 PyMfc UI Designer"', 'Header="关于 PyArx"'),
      ('<XLabel Name="statusBrand" Text="PyMfc Studio" Width="112"/>',
       '<XLabel Name="statusBrand" Text="PyArx" Width="64"/>')]),
    (r'G:\PyArx\UIDesign\DesignerShell.cpp',
     [('CreateFrame(_T("PyMfc UI Designer"), rect)', 'CreateFrame(_T("PyArx"), rect)')]),
    (r'G:\PyArx\UIDesign\DesignerPaneFeature.cpp',
     [('SetWindowText(_T("PyMfc UI Designer - 未命名"))', 'SetWindowText(_T("PyArx - 未命名"))')]),
    (r'G:\PyArx\UIDesign\DesignerPaneView.cpp',
     [('_T("PyMfc UI Designer - %s")', '_T("PyArx - %s")'),
      ('_T("PyMfc UI Designer\\n基于 PyMfcXml', '_T("PyArx\\n基于 PyMfcXml'),
      ('_T("PyMfc UI Designer · %s")', '_T("PyArx · %s")'),
      ('_T("PyMfc UI Designer - %s · %s")', '_T("PyArx - %s · %s")')]),
    (r'G:\PyArx\examples\ui\cad_start.xml',
     [('Title="PyMfc Studio — 起始页"', 'Title="PyArx — 起始页"')]),
]

for path, pairs in TARGETS:
    raw = open(path, 'rb').read()
    bom = raw.startswith(b'\xef\xbb\xbf')
    text = raw.decode('utf-8-sig' if bom else 'utf-8')
    hits = 0
    for old, new in pairs:
        n = text.count(old)
        hits += n
        text = text.replace(old, new)
    enc = 'utf-8-sig' if bom else 'utf-8'
    io.open(path, 'w', encoding=enc, newline='').write(text)
    print(os.path.basename(path), 'replacements:', hits)
