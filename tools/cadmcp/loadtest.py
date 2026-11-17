import ctypes, os, sys
os.environ["PATH"] = r"G:\PyArx\X64\Release;" + os.environ.get("PATH", "")
p = r"G:\PyArx\X64\Release\PyArxEngine.arx"
try:
    h = ctypes.WinDLL(p)
    print("LOADS_OK handle=%s" % h._handle)
except OSError as e:
    print("LOADFAIL winerror=%s msg=%s" % (getattr(e, "winerror", None), e))
