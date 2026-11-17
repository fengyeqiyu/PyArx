# -*- coding: utf-8 -*-
"""在重启后的 acad.exe 内确认：新 pyd 是否带 setLayerId。"""

import PyArxCore as P

result = {
    "pyd": str(P.__file__),
    "has_setLayerId": hasattr(P.Db.PyDbEntity, "setLayerId"),
    "setLayerId_doc": (P.Db.PyDbEntity.setLayerId.__doc__ or "").strip()[:80],
}
