import io
P = r'G:\PyArx\PyMfcXaml\PyMfcXaml.cpp'
lines = io.open(P, encoding='utf-8-sig').read().split('\n')

i_g = next(i for i, l in enumerate(lines) if 'm.def("guiload", &Guiload,' in l)
i_end = i_g
while ');' not in lines[i_end]:
    i_end += 1
block = lines[i_g:i_end + 1]
print('guiload def 原在 %d..%d，剪切' % (i_g + 1, i_end + 1))
del lines[i_g:i_end + 1]

i_x = next(i for i, l in enumerate(lines) if 'm.def("xamlLayout"' in l)
lines[i_x:i_x] = block
io.open(P, 'w', encoding='utf-8-sig', newline='').write('\n'.join(lines))
print('已移到模块内 xamlLayout def 之前')
