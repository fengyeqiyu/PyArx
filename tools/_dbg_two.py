import io
lines = io.open(r'G:\PyArx\docs\_backup_PyMfcHost.h', encoding='utf-8').read().split('\n')
for key in ('getCanvasCommands', 'setCanvasCommands', 'caretBlinkStates'):
    print('==== %s ====' % key)
    for i, l in enumerate(lines):
        if key in l:
            for j in range(max(0, i - 2), min(len(lines), i + 8)):
                print('%5d |%s' % (j + 1, lines[j]))
            print('   ...')
