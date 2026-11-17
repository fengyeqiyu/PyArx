import ctypes, os
acad = r"C:\Program Files\Autodesk\AutoCAD 2024"
os.environ["PATH"] = r"G:\PyArx\X64\Release;" + acad + ";" + os.environ.get("PATH", "")
os.add_dll_directory(acad)
os.add_dll_directory(r"G:\PyArx\X64\Release")
p = r"G:\PyArx\X64\Release\PyArxEngine.arx"
try:
    h = ctypes.WinDLL(p)
    print("LOADS_OK handle=%s" % h._handle)
except OSError as e:
    print("LOADFAIL winerror=%s msg=%s" % (getattr(e, "winerror", None), e))
