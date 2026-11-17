import io

p = r'G:\PyArx\examples\cad2d\cad_shell.xml'
s = io.open(p, encoding='utf-8').read()
old = '  <XStackPanel Padding="0" Margin="0" Spacing="0">\n\n'
assert old in s, 'wrapper not found'
s = s.replace(old, '\n', 1)
tail = '  </XStackPanel>\n</XWindow>'
assert tail in s, 'tail not found'
s = s.replace(tail, '</XWindow>', 1)
io.open(p, 'w', encoding='utf-8', newline='\n').write(s)
print('ok')
