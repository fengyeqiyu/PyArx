# -*- coding: utf-8 -*-
"""先热替换 PyArxCore，再跑最小容器协议探针。"""
exec(open(r"G:\PyArx\tools\hotbuild\reload.py", encoding="utf-8").read())
exec(open(r"G:\PyArx\tests\cad\pyarx_minimal_probe2.py", encoding="utf-8").read())
