import io
p = r'G:\PyArx\UIDesign\DesignerPaneView.cpp'
s = io.open(p, encoding='utf-8', newline='').read()
nl = '\r\n' if '\r\n' in s else '\n'

start = s.index('    diagLog("[panePaint] full %dx%d back=%dx%d\\n"')
end = s.index('    dc.BitBlt(0, 0, client.Width(), client.Height(), &m_backDc, 0, 0, SRCCOPY);')
end += len('    dc.BitBlt(0, 0, client.Width(), client.Height(), &m_backDc, 0, 0, SRCCOPY);')

new = nl.join([
'    diagLog("[panePaint] full %dx%d back=%dx%d\\n", client.Width(), client.Height(),',
'            m_back.width(), m_back.height());',
'',
'    // 离屏缓冲由控件库负责（32bpp DIB、尺寸复用、BitBlt 上屏）：',
'    // D2D 只能可靠绑 DIB Section，Display 兼容位图在部分驱动上 BindDC 直接失败，',
'    // 表现就是"位图全黑、只剩真子窗口可见"。这条约束收在 XmlBackBuffer 里，',
'    // 任何新建的宿主/预览器直接用它，不需要再记这个坑。',
'    HDC mem = m_back.ensure(dc.GetSafeHdc(), client.Width(), client.Height());',
'    if (mem == nullptr)',
'        return;',
'',
'    paintSurface(mem);',
'',
'    m_back.present(dc.GetSafeHdc(), client.Width(), client.Height());',
])

s = s[:start] + new + s[end:]
io.open(p, 'w', encoding='utf-8', newline='').write(s)
print('OnPaint rewired')
