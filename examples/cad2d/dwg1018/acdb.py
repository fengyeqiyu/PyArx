# -*- coding: utf-8 -*-
"""离线实体类体系（仿 PyArx SDK）。

机制：
- 基类 AcDbEntity 统一持有 handle/common/geom；公共属性走 SDK 风格 getter。
- 每个 DXF 类型一个子类，readGeom(b, h) 自读主流几何段；结果写入 self.geom
  （dict，兼容下游 render/cmp），关键字段同时挂为具名属性。
- 注册表 BY_CODE 按对象 type code 自动实例化，decode_entity 不再走函数字典。
"""


class AcDbEntity:
    dxfName = "ENTITY"
    arxClass = "AcDbEntity"
    typeCodes = ()

    def __init__(self, handle, common):
        self.handle = handle
        self.common = common
        self.geom = None

    def readGeom(self, b, h):
        return None

    # ---- 公共属性（仿 SDK getter）----
    @property
    def colorIndex(self):
        return self.common.get("color", 256)

    @property
    def ownerId(self):
        return self.common["handles"]["owner"]

    @property
    def layerId(self):
        return self.common["handles"]["layer"]


class AcDbLine(AcDbEntity):
    dxfName = "LINE"
    arxClass = "AcDbLine"
    typeCodes = (0x13,)

    def readGeom(self, b, h):
        z0 = b.read_b()
        sx = b.read_rd(); ex = b.read_dd(sx)
        sy = b.read_rd(); ey = b.read_dd(sy)
        if z0:
            sz = ez = 0.0
        else:
            sz = b.read_rd(); ez = b.read_dd(sz)
        self.geom = {"sp": [sx, sy, sz], "ep": [ex, ey, ez]}
        self.startPoint = self.geom["sp"]
        self.endPoint = self.geom["ep"]
        return self.geom


class AcDbCircle(AcDbEntity):
    dxfName = "CIRCLE"
    arxClass = "AcDbCircle"
    typeCodes = (0x12,)

    def readGeom(self, b, h):
        cx, cy, cz = b.read_3bd()
        radius = b.read_bd()
        b.read_bt()                                  # thickness
        nx, ny, nz = b.read_be()                     # extrusion
        self.geom = {"center": [cx, cy, cz], "radius": radius, "normal": [nx, ny, nz]}
        self.center = self.geom["center"]
        self.radius = radius
        return self.geom


class AcDbArc(AcDbEntity):
    dxfName = "ARC"
    arxClass = "AcDbArc"
    typeCodes = (0x11,)

    def readGeom(self, b, h):
        cx, cy, cz = b.read_3bd()
        radius = b.read_bd()
        b.read_bt()
        nx, ny, nz = b.read_be()
        sa = b.read_bd(); ea = b.read_bd()
        self.geom = {"center": [cx, cy, cz], "radius": radius,
                     "startAngle": sa, "endAngle": ea, "normal": [nx, ny, nz]}
        self.center = self.geom["center"]
        self.radius = radius
        self.startAngle = sa
        self.endAngle = ea
        return self.geom


class AcDbPoint(AcDbEntity):
    dxfName = "POINT"
    arxClass = "AcDbPoint"
    typeCodes = (0x1b,)

    def readGeom(self, b, h):
        x = b.read_bd(); y = b.read_bd(); z = b.read_bd()
        b.read_bt()
        nx, ny, nz = b.read_be()
        ang = b.read_bd()
        self.geom = {"pos": [x, y, z], "normal": [nx, ny, nz], "ecsRotation": ang}
        self.position = self.geom["pos"]
        return self.geom


class AcDbEllipse(AcDbEntity):
    dxfName = "ELLIPSE"
    arxClass = "AcDbEllipse"
    typeCodes = (0x23,)

    def readGeom(self, b, h):
        cx, cy, cz = b.read_3bd()
        mx, my, mz = b.read_3bd()                    # sm_axis
        nx, ny, nz = b.read_3bd()                    # extrusion
        ratio = b.read_bd()
        sa = b.read_bd(); ea = b.read_bd()
        self.geom = {"center": [cx, cy, cz], "majorAxis": [mx, my, mz],
                     "normal": [nx, ny, nz], "radiusRatio": ratio,
                     "startAngle": sa, "endAngle": ea}
        self.center = self.geom["center"]
        return self.geom


class AcDbPolyline(AcDbEntity):
    dxfName = "LWPOLYLINE"
    arxClass = "AcDbPolyline"
    typeCodes = (0x4d,)

    def readGeom(self, b, h):
        flag = b.read_bs()
        const_w = None; elevation = None; thickness = None
        nx = ny = nz = 0.0
        if flag & 4: const_w = b.read_bd()
        if flag & 8: elevation = b.read_bd()
        if flag & 2: thickness = b.read_bd()
        if flag & 1: nx, ny, nz = b.read_3bd()
        n = b.read_bl()
        n_bul = b.read_bl() if flag & 16 else 0
        n_wid = b.read_bl() if flag & 32 else 0
        pts = []
        px = py = None
        for i in range(n):
            if i == 0:
                x, y = b.read_2rd()
            else:
                x = b.read_dd(px); y = b.read_dd(py)
            pts.append([x, y, 0.0]); px, py = x, y
        bulges = [b.read_bd() for _ in range(n_bul)]
        widths = []
        for _ in range(n_wid):
            widths.append([b.read_bd(), b.read_bd()])
        self.geom = {"verts": [{"pt": p, "bulge": bulges[i] if i < n_bul else 0.0,
                                "widths": widths[i] if i < n_wid else None}
                               for i, p in enumerate(pts)],
                     "elevation": elevation, "thickness": thickness,
                     "normal": [nx, ny, nz], "constWidth": const_w}
        self.numVerts = n
        return self.geom


class AcDbBlockReference(AcDbEntity):
    dxfName = "INSERT"
    arxClass = "AcDbBlockReference"
    typeCodes = (0x07,)

    def readGeom(self, b, h):
        px, py, pz = b.read_3bd()                   # ins_pt
        flag = b.read_bb()                           # scale_flag
        if flag == 3:
            sx = sy = sz = 1.0
        elif flag == 1:
            sx = 1.0; sy = b.read_dd(1.0); sz = b.read_dd(1.0)
        elif flag == 2:
            sx = b.read_rd(); sy = sz = sx
        else:
            sx = b.read_rd(); sy = b.read_dd(sx); sz = b.read_dd(sx)
        rot = b.read_bd()                            # rotation
        nx, ny, nz = b.read_3bd()                    # extrusion
        has_attribs = b.read_b()
        num_owned = b.read_bl() if has_attribs else 0
        from entity import read_h
        _, _, block_header = read_h(h)
        attribs = []
        if has_attribs:
            attribs = [read_h(h)[2] for _ in range(num_owned)]
            _, _, _seqend = read_h(h)
        self.geom = {"pos": [px, py, pz], "rotation": rot,
                     "scale": [sx, sy, sz], "normal": [nx, ny, nz],
                     "block": block_header, "has_attribs": has_attribs,
                     "attribs": attribs}
        self.position = self.geom["pos"]
        self.blockRecord = block_header
        return self.geom


class AcDbText(AcDbEntity):
    dxfName = "TEXT"
    arxClass = "AcDbText"
    typeCodes = (0x01, 0x02, 0x03)               # TEXT / ATTRIB / ATTDEF

    def readGeom(self, b, h):
        flags = b.read_rc()
        elev = 0.0
        if not (flags & 1):
            elev = b.read_rd()
        ix, iy = b.read_2rd()
        ax = ay = None
        if not (flags & 2):
            ax = b.read_dd(ix); ay = b.read_dd(iy)
        nx, ny, nz = b.read_be()
        b.read_bt()                                  # thickness
        oblique = 0.0
        if not (flags & 4):
            oblique = b.read_rd()
        rotation = 0.0
        if not (flags & 8):
            rotation = b.read_rd()
        height = b.read_rd()
        wf = 1.0
        if not (flags & 0x10):
            wf = b.read_rd()
        tv = b.read_tv()
        halign = valign = 0
        if not (flags & 0x20):
            b.read_bs()                              # generation
        if not (flags & 0x40):
            halign = b.read_bs()
        if not (flags & 0x80):
            valign = b.read_bs()
        self.geom = {"pos": [ix, iy, elev], "alignPt": [ax, ay, 0.0], "height": height,
                     "rotation": rotation, "widthFactor": wf, "oblique": oblique,
                     "text": tv, "horizMode": halign, "vertMode": valign,
                     "normal": [nx, ny, nz]}
        self.position = self.geom["pos"]
        self.textString = tv
        self.height = height
        return self.geom


class AcDbMText(AcDbEntity):
    dxfName = "MTEXT"
    arxClass = "AcDbMText"
    typeCodes = (0x2c,)

    def readGeom(self, b, h):
        ix, iy, iz = b.read_3bd()
        nx, ny, nz = b.read_3bd()
        dx, dy, dz = b.read_3bd()
        rw = b.read_bd()
        th = b.read_bd()
        attach = b.read_bs()
        flow = b.read_bd()
        b.read_bd()                                  # extents_height
        b.read_bd()                                  # extents_width
        tv = b.read_tv()
        b.read_bs()                                  # linespace_style
        b.read_bd()                                  # linespace_factor
        bg = b.read_bl()
        if bg & 1:
            b.read_bd(); b.read_mc(); b.read_bl()
        self.geom = {"loc": [ix, iy, iz], "direction": [dx, dy, dz], "width": rw,
                     "height": th, "attachment": attach, "flowDir": flow,
                     "contents": tv, "normal": [nx, ny, nz]}
        self.location = self.geom["loc"]
        self.contents = tv
        return self.geom


class AcDbSpline(AcDbEntity):
    dxfName = "SPLINE"
    arxClass = "AcDbSpline"
    typeCodes = (0x24,)

    def readGeom(self, b, h):
        scenario = b.read_bl()
        degree = b.read_bl()
        if scenario & 1:
            rational = b.read_b(); closed = b.read_b(); periodic = b.read_b()
            b.read_bd(); b.read_bd()                 # knot_tol, ctrl_tol
            n_knots = b.read_bl(); n_ctrl = b.read_bl()
            weighted = b.read_b()
            knots = [b.read_bd() for _ in range(n_knots)]
            ctrl = []; weights = []
            for _ in range(n_ctrl):
                x, y, z = b.read_3bd()
                ctrl.append([x, y, z])
                weights.append(b.read_bd() if weighted else 1.0)
            self.geom = {"degree": degree, "rational": bool(rational),
                         "closed": bool(closed), "periodic": bool(periodic),
                         "ctrl": ctrl, "weights": weights, "fit": [], "fitTol": None,
                         "knots": knots}
            return self.geom
        fit_tol = b.read_bd()
        b.read_3bd(); b.read_3bd()
        n_fit = b.read_bl()
        fit = [list(b.read_3bd()) for _ in range(n_fit)]
        self.geom = {"degree": degree, "rational": False, "ctrl": [], "weights": [],
                     "fit": fit, "fitTol": fit_tol}
        return self.geom


class AcDbHatch(AcDbEntity):
    dxfName = "HATCH"
    arxClass = "AcDbHatch"
    typeCodes = (0x4e,)

    @staticmethod
    def _seg(b):
        ct = b.read_rc()
        if ct == 1:
            x1, y1 = b.read_2rd(); x2, y2 = b.read_2rd()
            return {"type": 1, "p1": [x1, y1], "p2": [x2, y2]}
        if ct == 2:
            cx, cy = b.read_2rd(); r = b.read_bd(); sa = b.read_bd(); ea = b.read_bd()
            ccw = b.read_b()
            return {"type": 2, "center": [cx, cy], "radius": r, "sa": sa, "ea": ea, "ccw": ccw}
        if ct == 3:
            cx, cy = b.read_2rd(); ex, ey = b.read_2rd(); ratio = b.read_bd()
            sa = b.read_bd(); ea = b.read_bd(); ccw = b.read_b()
            return {"type": 3, "center": [cx, cy], "end": [ex, ey], "ratio": ratio,
                    "sa": sa, "ea": ea, "ccw": ccw}
        if ct == 4:
            degree = b.read_bl(); rational = b.read_b(); periodic = b.read_b()
            nk = b.read_bl(); nc = b.read_bl()
            knots = [b.read_bd() for _ in range(nk)]
            ctrl = []
            for _ in range(nc):
                x, y = b.read_2rd()
                ctrl.append([x, y] + ([b.read_bd()] if rational else []))
            return {"type": 4, "degree": degree, "rational": rational,
                    "periodic": periodic, "knots": knots, "ctrl": ctrl}
        return {"type": ct}

    def readGeom(self, b, h):
        from entity import read_h
        b.read_bl(); b.read_bl()
        b.read_bd(); b.read_bd()
        b.read_bl(); b.read_bd()
        ncol = b.read_bl()
        for _ in range(ncol):
            b.read_bd(); b.read_cmc()
        b.read_tv()
        b.read_bd()
        nx, ny, nz = b.read_3bd()
        name = b.read_tv()
        solid = b.read_b(); associative = b.read_b()
        npath = b.read_bl()
        loops = []
        derived = 0
        for _ in range(npath):
            flag = b.read_bl()
            derived |= (flag & 4)
            if flag & 2:
                bulg = b.read_b(); closed = b.read_b()
                n = b.read_bl()
                pts = []; bulges = []
                for _ in range(n):
                    x, y = b.read_2rd(); pts.append([x, y, 0.0])
                    if bulg: bulges.append(b.read_bd())
                loops.append({"type": 2, "pts": pts, "bulges": bulges, "closed": closed})
            else:
                nseg = b.read_bl()
                segs = [self._seg(b) for _ in range(nseg)]
                pts = []
                for s in segs:
                    if s["type"] == 1:
                        if not pts: pts.append(s["p1"])
                        pts.append(s["p2"])
                loops.append({"type": 0, "segs": segs, "pts": pts})
            nb = b.read_bl()
            bh = []
            if 0 < nb <= 100:
                bh = [read_h(h)[2] for _ in range(nb)]
            loops[-1]["boundaryHandles"] = bh
        style = b.read_bs(); pattern_type = b.read_bs()
        angle = spacing = 0.0; double = 0
        if not solid:
            angle = b.read_bd(); spacing = b.read_bd(); double = b.read_b()
            nd = b.read_bs()
            for _ in range(nd):
                b.read_bd(); b.read_2bd(); b.read_2bd()
                ndash = b.read_bs()
                [b.read_bd() for _ in range(ndash)]
        if derived:
            b.read_bd()
        nseed = b.read_bl()
        seeds = [list(b.read_2rd()) for _ in range(nseed)]
        self.geom = {"loops": loops, "patternName": name, "patternType": pattern_type,
                     "patternAngle": angle, "patternSpace": spacing, "hatchStyle": style,
                     "normal": [nx, ny, nz], "solid": solid}
        return self.geom


class AcDbDimension(AcDbEntity):
    dxfName = "DIMENSION"
    arxClass = "AcDbDimension"
    typeCodes = (0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a)

    def readGeom(self, b, h):
        nx, ny, nz = b.read_3bd()
        mx, my = b.read_2rd()
        elev = b.read_bd()
        b.read_rc()
        b.read_tv()
        trot = b.read_bd()
        hdir = b.read_bd()
        b.read_3bd(); b.read_bd()
        b.read_bs(); b.read_bs(); b.read_bd()
        meas = b.read_bd()
        b.read_2rd()
        b.read_3bd(); b.read_3bd(); b.read_3bd(); b.read_bd()
        self.geom = {"textPos": [mx, my, elev], "measurement": meas, "normal": [nx, ny, nz],
                     "elevation": elev, "textRotation": trot, "horizRotation": hdir}
        return self.geom


class AcDbTable(AcDbEntity):
    dxfName = "TABLE"
    arxClass = "AcDbTable"
    typeCodes = (0x330,)

    def readGeom(self, b, h):
        from entity import read_h
        ins = b.read_3bd()
        sf = b.read_bb()
        if sf == 0:
            b.read_dd(1.0); b.read_dd(1.0)
        elif sf == 1:
            b.read_dd(1.0); b.read_dd(1.0)
        elif sf == 2:
            b.read_rd()
        rot = b.read_bd()
        extr = b.read_be()
        ha = b.read_b()
        no = b.read_bl() if ha else 0
        read_h(b)
        if ha:
            for _ in range(no):
                read_h(b)
            read_h(b)
        read_h(b)
        b.read_bs()
        horiz = b.read_3bd()
        nc = b.read_bl(); nr = b.read_bl()
        self.geom = {"insPt": list(ins), "rotation": rot, "extrusion": list(extr),
                     "horiz": list(horiz), "numCols": nc, "numRows": nr}
        return self.geom


# ---- 注册表：type code -> 实体类 ----
_REGISTRY = (
    AcDbLine, AcDbCircle, AcDbArc, AcDbPoint, AcDbEllipse, AcDbPolyline,
    AcDbBlockReference, AcDbText, AcDbMText, AcDbSpline, AcDbHatch,
    AcDbDimension, AcDbTable,
)
BY_CODE = {code: cls for cls in _REGISTRY for code in cls.typeCodes}
BY_NAME = {cls.dxfName: cls for cls in _REGISTRY}
# 文字族别名：ATTRIB / ATTDEF 与 TEXT 共用同一解码类
BY_NAME["ATTRIB"] = AcDbText
BY_NAME["ATTDEF"] = AcDbText
