import io
P = r'G:\PyArx\tests\mfc_xaml\_pp_final_shot.py'
s = io.open(P, encoding='utf-8').read()
s = s.replace("app.win.fitPlanCanvas(pp_app.CV)", "app.fit_view()")
io.open(P,'w',encoding='utf-8').write(s)
print('ok')
