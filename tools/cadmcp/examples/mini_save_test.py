# -*- coding: utf-8 -*-
"""最小 saveAs 验证：只保存，不建层、不画图元。"""

import os

import PyArxCore as P

PATH = r"C:\temp\pyarx_save_verify.dwg"

doc = P.Ap.PyApp.documentManager.curDocument
st = str(doc.database.saveAs(PATH))

result = {
    "status": st,
    "exists": os.path.exists(PATH),
    "size": os.path.getsize(PATH) if os.path.exists(PATH) else 0,
    "pid": os.getpid(),
}
