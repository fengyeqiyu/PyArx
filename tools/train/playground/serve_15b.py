# -*- coding: utf-8 -*-
"""1.5B adapter 的独立 playground 实例（端口 8766）。

不改 app.py、不重启 8765 现役服务：用 importlib 加载 app.py，改掉
BASE_MODEL / ADAPTER / PORT 三个模块级常量后另起一个进程。
"""
import importlib.util
import os
import sys
from http.server import ThreadingHTTPServer

HERE = os.path.dirname(os.path.abspath(__file__))
APP = os.path.join(HERE, "app.py")

if HERE not in sys.path:
    sys.path.insert(0, HERE)

spec = importlib.util.spec_from_file_location("pg_app", APP)
m = importlib.util.module_from_spec(spec)
spec.loader.exec_module(m)

m.BASE_MODEL = r"D:\models\qwen2.5-1.5b"
m.ADAPTER = os.path.join(m.ROOT, "out", "qwen1.5b-cad-lora-api318")
m.PORT = 8766

if __name__ == "__main__":
    m._load(lora=True)
    srv = ThreadingHTTPServer(("127.0.0.1", m.PORT), m.Handler)
    print("PyArx playground 1.5B -> http://127.0.0.1:%d" % m.PORT, flush=True)
    srv.serve_forever()
