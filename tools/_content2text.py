import io
import re

p = r'G:\PyArx\examples\cad2d\cad_shell.xml'
s = io.open(p, encoding='utf-8').read()
s, n = re.subn(r'\bContent="', 'Text="', s)
io.open(p, 'w', encoding='utf-8', newline='').write(s)
print('Content->Text:', n)
