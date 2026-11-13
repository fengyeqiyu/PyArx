import io
p = 'G:/PyArx/tests/mfc_xaml/pp_plugin.py'
s = io.open(p, encoding='utf-8').read()
s = s.replace("import importlib\n", "import importlib\nimport importlib.util\n")
s = s.replace("""    if modname in os.sys.modules:
        mod = importlib.reload(os.sys.modules[modname])
    else:
        spec = importlib.util.spec_from_file_location(modname, path)""",
"""    if modname in os.sys.modules and os.path.exists(path):
        mod = importlib.reload(os.sys.modules[modname])
    else:
        spec = importlib.util.spec_from_file_location(modname, path)""")
io.open(p, 'w', encoding='utf-8', newline='').write(s)
print('ok')
