import io
P = r'G:\PyArx\tests\mfc_xaml\_diag_canvas.py'
s = io.open(P, encoding='utf-8').read()
s = s.replace("app.win.show()\nM.pump(100)", "app.win.show()\ntime.sleep(0.7)\nM.pump(120)")
s = s.replace("print('host=%d cv=%d zoom=%.3f' % (host, cv, app.win.getPlanZoom(pp_app.CV)))",
              "print('host=%s cv=%s zoom=%.3f' % (host, cv, app.win.getPlanZoom(pp_app.CV)))")
io.open(P, 'w', encoding='utf-8').write(s)
print('ok')
