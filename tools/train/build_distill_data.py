# -*- coding: utf-8 -*-
"""把本项目的实测结论蒸馏成训练样本，并与工具调用样本合并。

三类数据：
  1. 行为样本：指令 → cad_* 工具调用（build_sft_data.py 产出）
  2. 事实样本：项目知识问答（本文件 FACTS，全部来自宿主实测）
  3. 约束样本：模型该拒绝/该换做法的情况（避免它学成"硬试"）

输出 sft_all.jsonl，训练脚本直接读这个。
"""

import json
import os

BASE = r"G:\PyArx\tools\train\data"
SRC_ACT = os.path.join(BASE, "sft_cad_tools.jsonl")
SRC_PREFIX = os.path.join(BASE, "sft_prefix.jsonl")
OUT = os.path.join(BASE, "sft_all.jsonl")

SYS = ("你是 CAD 绘图助手。只通过 cad_* 工具操作图形，不要自己写 PyArx 代码。"
       "用户要脚本或代码时照样输出 cad_* 工具调用，由系统生成 PyArx 代码，不要改成讲架构或手写代码。"
       "坐标单位 mm，角度用度，图层不存在会自动创建。"
       "需要等分时给 cad_rect 填 vdivide=N（竖线 N 等分）或 hdivide=N（横线 N 等分），等分线坐标由工具层算，不要自己算竖线 x 值。"
       "cad_layer 只用于新建图层；要查某层上已有哪些图元用 cad_query。"
       "一次事务全有或全无；工具返回 ok/count/handles/warnings/errors，"
       "失败时要读 warnings 自纠而不是重试同一调用。")

# ---------------------------------------------------------------- 事实样本
FACTS = [
    ("cad_* 一共有哪些工具？",
     "17 个：cad_line 直线、cad_polyline 多段线、cad_rect 矩形、cad_text 单行文字、"
     "cad_mtext 多行文字、cad_circle 圆、cad_arc 圆弧、cad_hatch 填充、cad_table 表格、"
     "cad_block_ref 块参照、cad_layer 建图层、cad_offset 偏移、cad_batch 批量、"
     "cad_save 保存、cad_stats 统计、cad_query 查询、cad_erase 删除。"),
    ("画矩形要自己算四个角吗？",
     "不用。cad_rect 只要两个对角点 p1、p2，四个角由工具层算。"),
    ("怎么画一条和已有线平行的线？",
     "用 cad_offset，给源图元句柄和距离 dist。几何由 CAD 自己算，不用推坐标；"
     "新线继承源图元的图层。"),
    ("设置图层失败怎么办？",
     "不要反复重试。setLayer(字符串) 恒返回 False；setClayer 只收 layerId:int，传字符串必 TypeError；"
     "要设图层用 setLayerId(图层 objectId)，id 走层表 getAt(图层名).objectId()。"
     "图层创建路径是通的，不是能力缺失。"),
    ("实体画完但句柄是 0 是什么情况？",
     "说明 addEntity 被拒、实体没入库。addEntity 返回 bool 且失败不抛异常，"
     "常见原因是引用了 xref 派生块（名字含 $0$）。工具层会把这种情况记进 warnings 且不计入 count。"),
    ("块参照为什么引用不了？",
     "xref 派生块（名字含 $0$）会被 appendAcDbEntity 拒绝；而且块定义 API 没打通"
     "（PyDbBlockTable.add 事务外会断言/挂死），所以只能引用当前图已有的普通块。"),
    ("批量画图失败了会画一半吗？",
     "不会。cad_batch 一次事务全有或全无：失败时 handles 清空、rolled_back=true、"
     "failed_index 指出第几步，你先读 errors 再改。"),
    ("保存图形会不会把 CAD 搞崩？",
     "不会，已经修过。历史上两个崩溃点：saveAs 传 nullptr SecurityParams 在 acdb24+0x6f414 解引用空指针；"
     "建层后图层表没 close 就存盘。现在工具层都兜住了。"),
    ("未命名图形能直接保存吗？",
     "不能。document.save() 对未命名图形返回 eInvalidInput，必须先 cad_save 给 path 做 saveAs。"),
    ("cad_erase 的句柄参数是 handles 还是 handle？",
     'handles，而且是数组：{"handles": ["732"]}。handle 单数是 cad_offset 用的，两者别混，混了 schema 会判无效。'),
    ("cad_erase 怎么清空全图？",
     '必须同时给 all=true 和 confirm="ERASE_ALL"，只给 all 会被拒。'),
    ("统计和查询有什么区别？",
     "cad_stats 出全图概览（总数、按类、按层）；cad_query 按图层或类过滤，返回句柄列表。"),
    ("坐标单位是什么？",
     "mm。角度用度。字高也是 mm。"),
    ("一个图层不存在时画图会怎样？",
     "会自动创建。cad_batch 在事务外建层，颜色默认黄色，也可以传 layer_rgb 指定。"),
    ("句柄什么时候能拿到？",
     "事务提交之后。事务内 handle() 还是 0，工具层是在提交后才统一取句柄的。"),
    ("为什么不让模型自己写 PyArx 脚本？",
     "宿主有一串怪癖：事务退出异常会覆盖真实错误、addEntity 静默失败、块表写锁、"
     "图层名编码、saveAs 空指针、符号表没关会崩。这些都固化在工具层里了，"
     "模型只负责选工具和填参数，出错概率低得多。"
     "但用户要具体代码时照样输出 cad_* 工具调用，宿主会据此生成真实 PyArx 代码，不要推辞。"),
    ("cad_offset 支持哪些图元？",
     "直线、圆、圆弧、多段线这类曲线。块参照等非曲线不支持，会返回明确错误。"),
    ("画错了怎么改？",
     "先用 cad_query 找到句柄，再 cad_erase 删掉，然后重画；不要试图去改已有实体的属性。"),
    # ---- 以下来自 module_matrix.json 实测蒸馏，三元组末位是对应 layer.module ----
    ("PyDbCircle 怎么改半径？",
     "先 PyDbCircle() 空构造，再 setRadius(r: float)，radius() 回读同值（实测 33.25→33.2500）；"
     "构造签名是 (cntr: AcGePoint3d, nrm: AcGeVector3d, radius: float)，第二参数是法向 nrm，不是半径。",
     "Db.PyDbCircle"),
    ("PyDbPolyline 怎么加顶点？",
     "addVertexAt(index: int, pt: AcGePoint2d, bulge: float = 0.0, ...)；顶点回读是 AcGePoint2d，numVerts 是 int；"
     "越界索引返回 False，点传字符串抛 TypeError。",
     "Db.PyDbPolyline"),
    ("PyDbViewport 入库后读回还是视口吗？",
     "不是。提交后 openObject 读回降级成 PyDbObject，不是 PyDbViewport；setHeight/setLensLength 只收 float。",
     "Db.PyDbViewport"),
    ("PyDbTrans 没 enter 就 addEntity 会怎样？",
     "返回 True，但这是静默接受，不代表事务成功；全新事务直接 commit 返回 None；openObject 传非法 id 返回 None。"
     "必须先 enter 再 addEntity。",
     "Db.PyDbTrans"),
    ("PyDbEntity/PyDbObject/PyDbCurve 能空构造吗？",
     "不能。三者只有引用构造 (object: AcDbEntity/AcDbObject/AcDbCurve)，包装字符串抛 TypeError；"
     "MText 包成 PyDbCurve 也抛 TypeError；基类成员由具体 wrapper 继承。",
     "Db.PyDbEntity"),
    ("AcGePoint3d 怎么被矩阵变换？",
     "点调 p.transformBy(m)；矩阵对象本身没有 transformBy。distanceTo(AcGePoint3d) 返回 float；"
     "ctor(x,y,z)/无参/拷贝，kOrigin=[0,0,0]；平移矩阵 inverse() 可闭环回初值。",
     "Ge.AcGePoint3d"),
    ("AcGeVector3d 零长度 normalize 会报错吗？",
     "不会。零向量 normalize 静默返回零向量，不抛异常（2d/3d 同病，AcGeVector2d(0,0) 同样返回零向量）；"
     "正常向量 length/lengthSqrd、dotProduct/crossProduct、negate 可用，kXAxis/kYAxis/kZAxis 是单位轴常量。",
     "Ge.AcGeVector3d"),
    ("AcGeMatrix3d 怎么平移/缩放？奇异矩阵 inverse 会报错吗？",
     "无参构造是单位阵 det=1；setToTranslation(x,y,z) 平移；setToScaling(factor, center=AcGePoint3d)，"
     "中心形参是 Point3d 不是 Vector3d；inverse() 无参或带 AcGeTol；奇异矩阵 inverse 不抛异常，返回 det=1 的矩阵；"
     "矩阵没有 transformBy。",
     "Ge.AcGeMatrix3d"),
    ("PyGiColor 这个类存在吗？怎么设颜色？",
     "PyGiColor 不存在。颜色走 traits.setColor(color: int) 或 setTrueColor(AcCmEntityColor)，"
     "也可用 PyDbEntity.colorRGB；Gi 内真实颜色类是 AcCmEntityColor、AcGiColorRGB、AcGiColorRGBA。"
     "PyGiPolyline、PyGiContextGdi 也不存在，真实为 AcGiPolyline、AcGiContext、AcGiGdiDrawObject。",
     "Gi.PyGiColor"),
    ("PyEdEnt/PyEdFunc/PyEdSel 这些类怎么用？",
     "都不存在。真实对应：jig 是 AcEdJig；选集是 SelectionSet + ssget/ssadd/sslength/ssname/ssmemb；"
     "实体名走 AdsName/entget/entmake/handent/entlast；handent(句柄) 无头返回 None（状态 -5001 归一）；"
     "工具函数 acutDistance/acdbAngToS/acdbRToS/acutPolar/acutWcMatch 挂在 P.Ed 顶层；"
     "UCS/视口是 acedGetCurrentUCS/acedActiveViewportId/acedVports。",
     "Ed.PyEdEnt"),
    ("ErrorStatus/DxfCode 这类枚举在哪？",
     "在 P.Enum：ErrorStatus（eOk=0、eInvalidInput=3、eInvalidAdsName=31）、"
     "DxfCode（kDxfInvalid=-9999、kDxfStart=0）、PromptStatus（eNone=5000、eNormal=5100、eError=-5001）、"
     "AdsReturn（kNorm=5100、kError=-5001）；PyAdsErrorStatus/PyAdsDxfCode/PyAdsPromptStatus 不存在。",
     "Ads.Enum.ErrorStatus"),
    ("PyApp.acadApplication 为什么取不到？",
     "无头环境抛 RuntimeError：acadApplication 不可用；mainWindow=0(int)、nonInPlaceMainWindow=0、statusBar=None "
     "是常态；documentManager 可用；preferences/infoCenter/menuBar/menuGroups 降级 TypeError，C++ 侧无等价物。",
     "App.PyApp"),
    ("PYRUNFILE 是哪个项目注册的？",
     "PyArxEngine。命令组 'PyArxEngine' 注册清单含 PYLOAD、PYRUN、PYEXEC、PIPM、PYDEBUG、PYARXDEPLOY、PYRUNFILE；"
     "arxload PyArxEngine.crx 后 (command \"PYRUNFILE\" 脚本路径) 执行；Python 侧无命令名 lookup，不能二次调起。",
     "Engine.PYRUNFILE.command_chain"),
    ("PyGiTextStyle 的字高和入库文字能对上吗？",
     "能。setTextSize(8.0) 后建 PyDbText('配电箱', height=8.0) 入库，读回 height=8.0000，尺寸语义闭环；"
     "xScale/obliquingAngle/trackingPercent(比例非百分数)/styleName 可往返；非法字体文件 setFileName 返回 None。",
     "Gi.PyGiTextStyle"),
    ("PaletteSet/Palette 能直接 new 吗？",
     "不能，两者都是 No constructor defined!。实例走 P.Ap.getPalette(name) 或 PaletteSet.create/addPage；"
     "create/show/addPage/hwnd/title 是 MFC 宿主侧 GUI 操作，无头 console 无宿主不触发。",
     "App.PaletteSet"),
    ("圆弧/平面/向量还有哪些静默坑？",
     "AcGeCircArc3d.length(-1,-1) 超区间参数返回 0.0 不抛异常；负半径静默取绝对值（读回 radius=1.000）；"
     "length(fromParam,toParam) 无参 length() 不存在。AcGePlane(0,0,0,0) 全零系数静默产出 NaN 平面；"
     "AcGeVector2d(0,0).normalize() 静默返回零向量。",
     "Ge.AcGeCircArc3d"),
]

# ---------------------------------------------------------------- 约束样本
CONSTRAINTS = [
    # 工具层覆盖不到 —— 必须承认，不许编
    ("帮我设计一个配电系统图",
     "超出工具层能力：cad_* 只会画图元，不会做系统设计和选型。这类需求要交给能做规划的模型或人。"),
    ("这张图的线为什么画歪了",
     "我看不到图面，也读不到实体坐标。先 cad_query 查出句柄看几何数据，别凭感觉判断。"),
    ("PyArx 的 C++ 源码在哪、怎么改",
     "我不知道，也不该猜。源码位置和改法要查项目文档或问维护者。"),
    ("把这张图导出成 PDF",
     "工具层没有导出 PDF 的能力，只有 cad_save 存 dwg。做不到就直说。"),
    ("cad_array 这个工具怎么用",
     "目前没有 cad_array。现在只有 17 个 cad_* 工具，阵列要么逐条画，要么用 pyarx_run。"),
    ("帮我把全图图层改成蓝色",
     "工具层没有批量改已有实体属性的能力，做不到就直说，不要编一个调用出来。"),
    ("1000 乘以 3 等于多少",
     "算术我不算，容易错，交给代码或计算工具。"),
    ("这条线的长度是多少",
     "我不知道。先 cad_query 拿到句柄，让 CAD 算长度，不要口算坐标。"),
    # 参数必须合法 —— 缺东西就要，不许脑补
    ("把线画到 A 点",
     '坐标必须是数字，A 不是合法坐标。给 (x,y)，例如 [0,0]。'),
    ("图层名设成 3 号",
     '图层名必须是字符串，例如 "3号"，不能是裸数字。'),
    ("删掉刚才那条线",
     "删除必须给句柄，我没法猜“刚才那条”是哪条。先 cad_query 查出来再删。"),
    ("把这条线偏移 10",
     "偏移要源图元句柄。先 cad_query 定位，再把 handle 传给 cad_offset。"),
    # 设计取舍 —— 不是操作指令，别硬套工具
    ("为什么不让模型自己写 PyArx 脚本",
     "因为宿主有一串怪癖：事务退出异常会覆盖真实错误、addEntity 静默失败、块表写锁、"
     "图层名编码、saveAs 空指针、符号表没关会崩。这些已固化在工具层，模型只负责选工具填参数。"),
    ("cad_save 保存会崩吗",
     "不会，已修过。历史崩溃点是 saveAs 传空 SecurityParams 和图层表没 close，现在都兜住了。"),
    ("没训练的模型答项目问题会怎样",
     "会照通用编程常识编一段，比如把 cad_save 的坑说成“路径错误、权限问题”。"
     "看着通顺但全错，所以项目知识要靠事实样本喂，不能靠它自己想。"),
    # ---- 以下来自 module_matrix.json 实测蒸馏，三元组末位是对应 layer.module ----
    ("帮我用 PyGeMatrix3d 把点平移",
     "没有 PyGeMatrix3d 这个名字。要用 AcGeMatrix3d.setToTranslation(v) 建平移矩阵，再点调 p.transformBy(m)；"
     "矩阵对象上没有 transformBy，也不要在矩阵上找点变换方法。",
     "Ge.AcGeMatrix3d"),
    ("给 traits 传字符串设图层",
     "setLayer 只收 layerId: int；传字符串会被拒。要图层号就先查层表拿 int，别用图层名字符串。",
     "Gi.PyGiDrawableTraits"),
    ("我想实例化一个 PyEdResult",
     "PyEdResult 无构造器（No constructor defined），不能自己 new；结果由 jig/交互返回，取不到就是无结果/None。",
     "Ed.PyEdResult"),
    ("直接在脚本里 acedSetVar 改系统变量",
     "别调，无头彻底走不通：直传 int 抛 TypeError（要 (sym: str, TypedValue)）；"
     "Db.TypedValue(3/7, int) 抛 RuntimeError（int 无法转 AcString）；TypedValue(70, int) 送入直接崩进程 0xC0000409。"
     "只读用 acedGetVar('CLAYER')（返回 str '0'）；command(int) 入参同样会崩，command 只走已验证的 str/AdsName/point 路径。",
     "Ed.acedSetVar"),
    ("重复 acedArxLoad('PyArxEngine')",
     "别重复加载。acedArxLoad('PyArxEngine')=False（状态 -5001，已加载/名字不匹配）；"
     "acedArxLoaded() 清单里是小写 pyarxengine.crx。",
     "Engine.PyArxEngine.loaded"),
    ("新库调 approxNumObjects 数实体",
     "别调。全新 PyDbDatabase 的 approxNumObjects() 抛 RuntimeError；未命名新库 save() 返回 False；"
     "要存盘先 saveAs 给路径。",
     "Db.PyDbDatabase"),
    ("事务还没 enter 就 addEntity",
     "必须先 enter。未 enter 调 addEntity 返回 True 是静默接受，不代表入库成功；事务内提交后才拿得到句柄。",
     "Db.PyDbTrans"),
    ("2d 平移矩阵为什么作用不到点？",
     "绑定侧实测 bug：AcGeMatrix2d 存下 translation=(10,0)，点 transformBy 后坐标不变；"
     "且 AcGePoint2d.transformBy 只收 AcGeMatrix2d，传 AcGeMatrix3d 直接 TypeError。"
     "2d 平移要在 C++ 侧补偿或改用 3d 点/3d 矩阵，别指望 Matrix2d 平移作用到点。",
     "Ge.AcGePoint2d"),
    ("找 PyAdsErrorStatus 拿错误码",
     "名字不存在。错误状态走 P.Enum.ErrorStatus，DXF 组码走 P.Enum.DxfCode，提示状态走 P.Enum.PromptStatus；"
     "别在 Ed/Db 下找 PyAds 前缀类。",
     "Ads.Enum.ErrorStatus"),
    # ---- 现状前缀约束（前缀拼在 user 消息最前面，是当时的图纸快照）----
    ("[现状 ... 图层:LIVE_TEST,0 ...] 在 LIVE_TEST 层画一条线",
     "现状里 LIVE_TEST 已经存在，不要再调 cad_layer 新建；直接画，layer 参数填现有层名 LIVE_TEST。"),
    ("[现状 ... 实体6 ...] 把句柄 999 删掉",
     "现状里看不到句柄 999 对应的图元。先 cad_query 查一遍，查不到就答复“图上无此句柄，未删除”，"
     "不要直接 cad_erase。"),
    ("[现状 ... 取图失败 src=none 340ms] 图上现在有几个实体",
     "现状取图失败、src=none，不能假装知道实体总数或层分布；需要时用 cad_stats 只读统计，别凭印象报数字。"),
    ("[现状 ... 图层:0,轴线,设备 ...] 新建轴线层",
     "cad_layer 只用于现状里不存在的新层名；轴线已在现状图层清单里，直接在该层画，不要重复建层。"),
    ("[现状 17:31 实体6 层:0=3,LIVE_TEST=3 ...] 全图一共多少个实体",
     "前缀给出的总数和分层数量与现状一致时，直接引用前缀数字回答，不必再调 cad_stats。"),
    ("[现状 09:12 实体14 层:轴线=6 ...] 在轴线层画圆",
     "先把指令里的层名/句柄和前缀对一遍：层名在现状清单里就是已有层，直接用；"
     "句柄不在现状命中里就先查询，前缀只是当时快照，不能当成新图元的来源。"),
]


def build_facts():
    rows = []
    for item in FACTS:
        q, a = item[0], item[1]
        meta = {"kind": "fact"}
        if len(item) == 3:
            meta["module"] = item[2]
        rows.append({"messages": [
            {"role": "system", "content": SYS},
            {"role": "user", "content": q},
            {"role": "assistant", "content": a},
        ], "meta": meta})
    for item in CONSTRAINTS:
        q, a = item[0], item[1]
        meta = {"kind": "constraint"}
        if len(item) == 3:
            meta["module"] = item[2]
        rows.append({"messages": [
            {"role": "system", "content": SYS},
            {"role": "user", "content": q},
            {"role": "assistant", "content": a},
        ], "meta": meta})
    return rows


def main():
    rows = []
    if os.path.isfile(SRC_ACT):
        with open(SRC_ACT, encoding="utf-8") as f:
            for line in f:
                line = line.strip()
                if line:
                    rows.append(json.loads(line))
        print("action rows=%d" % len(rows))

    prefix_rows = []
    if os.path.isfile(SRC_PREFIX):
        with open(SRC_PREFIX, encoding="utf-8") as f:
            for line in f:
                line = line.strip()
                if line:
                    prefix_rows.append(json.loads(line))
        print("prefix rows=%d" % len(prefix_rows))
    rows.extend(prefix_rows)

    facts = build_facts()
    print("fact rows=%d" % len(facts))
    rows.extend(facts)

    os.makedirs(BASE, exist_ok=True)
    with open(OUT, "w", encoding="utf-8") as f:
        for r in rows:
            f.write(json.dumps(r, ensure_ascii=False) + "\n")
    print("total=%d -> %s" % (len(rows), OUT))


if __name__ == "__main__":
    main()
