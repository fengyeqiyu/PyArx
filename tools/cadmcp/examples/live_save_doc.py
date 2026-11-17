# -*- coding: utf-8 -*-
"""已命名文档走 PyArx 的 document.save()（内部 db->saveAs 当前文件名）。"""

import os

import PyArxCore as P

PATH = r"C:\temp\pyarx_save_verify.dwg"

doc = P.Ap.PyApp.documentManager.curDocument
st = str(doc.save())
DB = P.Db.curDb()

result = {
    "status": st,
    "exists": os.path.exists(PATH),
    "size": os.path.getsize(PATH) if os.path.exists(PATH) else 0,
    "entities": len(DB.objectIds("AcDbEntity")),
}
