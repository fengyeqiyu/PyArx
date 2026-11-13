import io
p = r'G:\PyArx\PyMfcXaml\parts\02-host.inc'
lines = io.open(p, encoding='utf-8').read().split('\n')
for n in (1166, 1167, 1168, 1169, 2609, 2610, 2611, 3443, 3444, 3445, 3779, 3780, 3781, 4610, 4611, 4612, 5156, 5157, 5158):
    print('%6d |%s' % (n, lines[n - 1]))
