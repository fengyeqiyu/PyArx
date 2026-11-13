import io
p='G:/PyArx/PyMfcXaml/PyMfcCommon.h'
s=io.open(p,encoding='utf-8-sig').read()
old="""        std::vector<xamlgeom::Point2> pts;
        xamlgeom::bezierPoints(xamlgeom::Point2(l.bez[0], l.bez[1]),
                               xamlgeom::Point2(l.bez[2], l.bez[3]),
                               xamlgeom::Point2(l.bez[4], l.bez[5]),
                               xamlgeom::Point2(l.bez[6], l.bez[7]), 16, pts);
        std::vector<POINT> poly;
        poly.reserve(pts.size());
        for (const xamlgeom::Point2& p : pts)
            poly.push_back(POINT{ X(p.x), Y(p.y) });"""
new="""        // 用户要求：回路线由"曲线"改成"折线"。布局阶段给出的 4 个点
        // (x1,y1) / (mx,y2 or y1) / (mx,y2) / (x2,y2) 本身就是一条正交折线的
        // 拐点序列，直接拿它们连折线即可，不再做贝塞尔采样（不再抹圆角）。
        std::vector<POINT> poly;
        const double raw[4][2] = { { l.bez[0], l.bez[1] }, { l.bez[2], l.bez[3] },
                                   { l.bez[4], l.bez[5] }, { l.bez[6], l.bez[7] } };
        for (int i = 0; i < 4; ++i)
        {
            const POINT pt = { X(raw[i][0]), Y(raw[i][1]) };
            if (!poly.empty() && poly.back().x == pt.x && poly.back().y == pt.y)
                continue;                          // 控制点重合时去掉重复点
            poly.push_back(pt);
        }"""
assert s.count(old)==1
s=s.replace(old,new,1)
io.open(p,'w',encoding='utf-8-sig',newline='').write(s)
print('回路线改折线')
