# PyArxCore 能力矩阵（无头实跑验证）

图元样板 17/18 通过、几何运算 9/9 通过、选择集读取 3/3 通过。

## 可直接调用的图元构造（参数为图纸单位，角度用度）
- arc -> PyDbArc  bounds=[[100.0, 100.0], [150.0, 150.0]]
- circle -> PyDbCircle  bounds=[[20.0, 20.0], [80.0, 80.0]]
- ellipse -> PyDbEllipse  bounds=[[-1.0, -1.0], [1.0, 1.0]]   ← ⚠ 半径 setter 未生效（期望 ±40/±20，实际 ±1）
- hatch -> PyDbHatch  bounds=[[0.0, 0.0], [100.0, 60.0]]
- leader -> PyDbLeader  bounds=[[0.0, 0.0], [200.0, 80.0]]
- line -> PyDbLine  bounds=[[0.0, 0.0], [120.0, 0.0]]
- mline -> PyDbMline  bounds=[[1e+20, 1e+20], [-1e+20, -1e+20]]   ← ⚠ 已入库但几何未生效，先别用
- mtext -> PyDbMText  bounds=[[10.0, 3.408], [110.0, 10.0]]
- point -> PyDbPoint  bounds=[[40.0, 40.0], [40.0, 40.0]]
- polyline -> PyDbPolyline  bounds=[[0.0, 0.0], [100.0, 60.0]]
- ray -> PyDbRay  bounds=[[0.0, 0.0], [0.0, 0.0]]
- rotated_dim -> PyDbRotatedDimension  bounds=[[0.0, 0.0], [0.0, 0.0]]   ← ⚠ 已入库但几何未生效，先别用
- solid -> PyDbSolid  bounds=[[0.0, 0.0], [50.0, 40.0]]
- spline -> PyDbSpline  bounds=[[-24.19, -33.828], [195.402, 77.828]]
- table -> PyDbTable  bounds=[[0.0, 100.0], [0.0, 100.0]]   ← ⚠ 已入库但几何未生效，先别用
- text -> PyDbText  bounds=[[10.0, 9.318], [21.654, 15.757]]
- xline -> PyDbXline  bounds=[[0.0, 0.0], [0.0, 0.0]]

## 已验证的几何运算
- mat3_rotate：绕 Z 轴 90 度后 (1,0) -> (0,1) -> [2.6794896412773994e-08, 0.9999999999999997]
- mat3_translate：平移矩阵作用于点 -> [11.0, 22.0]
- seg3_length：线段长度 -> 5.0
- vec2_perp：二维向量垂直（旋转 90 度） -> [1.0, 0.0]
- vec3_angle：两向量夹角（度） -> 1.5707963267948966
- vec3_cross：叉积（z 分量） -> 1.0
- vec3_dot：点积 -> 12.0
- vec3_len：向量的模 -> 5.0
- vec3_normalize：归一化后长度=1 -> 1.0

## 不要使用（实跑已否定）
- PyDbEntity.getBoundingPoints —— 61 个方法里不存在；包围盒用 bounds() -> (min,max) 或 getGeomExtents()
- PyDbEntity.openObjectByIdString —— 不存在；按句柄读实物只能事务内 tr.openObject(eid,0) 遍历比对 handle()
- PyDbEntity.isA —— 实体上没有 isA；类名用 className()
- Ge.AcGePoint3dArray —— Ge 子模块未导出该数组类型；样条改用 setFitPointAt/insertFitPointAt
- PyDbWipeout.appendLoopPoints —— 11 个方法里只有 append；且按 list 传参仍 addEntity=False，遮挡图元构造未打通
- PyDbMline.appendSegments —— 真名是 appendSeg（单词，不是复数）；但只 appendSeg 后 bounds 为空盒
- PyDbPoint(点) —— 无带点构造；PyDbPoint() + setPosition(pt)
- PyDbSolid(四点) —— 无四参构造；PyDbSolid() + setPointAt(i, pt) 四次
- PyDbXline/Ray(点+方向) —— 无带参构造；() + setBasePoint + setUnitDir(AcGeVector3d)
- PyDbEllipse.setMajorAxis —— 不存在；用 setCenter/setMajorRadius/setRadiusRatio
- PyDbEllipse 半径 setter —— 入库前后各试一次，setMajorRadius/setRadiusRatio 均不生效，bounds 恒为默认单位圆 ±1 —— 疑绑定层 setter 缺陷，需查 C++ 侧 PyDbEllipse 的属性桥接

## 调用约定
- 开对象必须显式给模式：tr.openObject(eid, 0)=kForRead / 1=kForWrite，不许省略
- 图元/表/符号表默认写模式；纯查询传读模式
- 符号表在一个进程里只能开一次且必须 close()，否则带写锁存盘会崩
- addEntity 返回 bool，失败不抛异常——必须判返回值
- 新建实体先 addEntity 入模型空间再写属性（入库前未 setDatabaseDefaults）
- 图层要在事务外建：getLayerTable -> newRecord -> setName/setColor -> lt.add
