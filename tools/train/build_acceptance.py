# -*- coding: utf-8 -*-
"""汇总矩阵/数据集/训练/评测/问答实测结果，落 acceptance.json。

轮次留档：
- v4(train4, 纯工具 SYS, adapter=v3 源)：eval 4/6。
- v5(train5, 混合 SYS 重训)：eval 1/6，QA 0/5，留档 v5-exp 备查。
- 第三轮：回滚到 v3 adapter，playground 加矩阵检索兜底；纯模型 0/5，带检索 0/5。
"""
import json
import os

BASE = r"G:\PyArx\tools\train\data"
TRAIN = r"G:\PyArx\tools\train"
import sys
sys.path.insert(0, TRAIN)
import build_matrix as bm

with open(os.path.join(BASE, "module_matrix.json"), encoding="utf-8") as f:
    matrix = json.load(f)

layers = {}
missing_classes = []
for m in matrix:
    L = layers.setdefault(m["layer"], {"modules": 0, "checks": 0})
    L["modules"] += 1
    L["checks"] += len(m["checks"])
    cats = set()
    for c in m["checks"]:
        cats |= bm.classify(c["name"], c["detail"])
    miss = [x for x in bm.CATS if x not in cats]
    if miss:
        missing_classes.append({"layer": m["layer"], "module": m["module"], "missing": miss})

rows = [json.loads(l) for l in open(os.path.join(BASE, "sft_all.jsonl"), encoding="utf-8") if l.strip()]
tools = [r for r in rows if r["meta"]["kind"] not in ("fact", "constraint")]
fc = [r for r in rows if r["meta"]["kind"] in ("fact", "constraint")]
fact_to_module = [[r["messages"][1]["content"][:20], r["meta"].get("module")]
                  for r in fc if r["meta"].get("module")]

# ---- 回滚后 eval v6：4/6（与 v4 基线一致）----
eval_v6 = {
    "hit": 4, "total": 6,
    "hits": ["cad_line", "cad_circle", "cad_text", "cad_query"],
    "misses": [
        ["统计一下当前图里有多少实体、分别在哪层", "cad_stats {}",
         "None；raw 为散文串接 cad_stats/类名/图层计数，未成形调用。"],
        ["把句柄 698 的那个图元删掉", "cad_erase {\"handles\":[\"698\"]}",
         "None；raw 裸 JSON 参数 handle 而非 handles，无 <tool_call> 包裹。"],
    ],
    "log": r"G:\PyArx\tools\train\logs\eval_v6.log",
}

# ---- 纯模型（无检索）----
# ---- 检索直答 grounded（抽取式，事实不经过生成）----
qa_grounded = [
    {"q": "PyDbCircle 怎么改半径？", "matched": ["Db.PyDbCircle"],
     "expected": ["setRadius", "radius() 往返", "(cntr,nrm,radius) 构造"],
     "found": ["setRadius（check 名 setRadius(0) [probe]）",
               "radius 33.25->33.2500 往返（prop_roundtrip_radius_center）",
               "构造 (cntr: AcGePoint3d, nrm: AcGeVector3d, radius: float)（ctor 负半径/传字符串 [probe] 完整签名）"],
     "missing": [],
     "verdict": "hit",
     "grounded_excerpt": "radius 33.25->33.2500；ctor 签名 cntr: AcGePoint3d, nrm: AcGeVector3d, radius: float；setRadius(0) [probe]"},
    {"q": "PyGeMatrix3d 怎么平移？", "matched": ["Ge.AcGeMatrix3d", "Ge.AcGePoint3d"],
     "expected": ["setToTranslation", "transformBy"],
     "found": ["setToTranslation(10,0,0) translation=[10.0,0.0,0.0]（AcGeMatrix3d.setToTranslation）",
               "p.transformBy(matrix)（跨模块带出 Ge.AcGePoint3d.transformBy_translation：(1,2,3).transformBy(translate(10,0,0)) -> (+11,+2,+3)）",
               "指引：矩阵本身没有 transformBy"],
     "missing": [],
     "verdict": "hit",
     "grounded_excerpt": "setToTranslation(10,0,0)；矩阵本身无 transformBy；点.transformBy(translate(10,0,0)) -> (+11.000,+2.000,+3.000)"},
    {"q": "PYRUNFILE 是哪个项目注册的？", "matched": ["Engine.PYRUNFILE.command_chain"],
     "expected": ["PyArxEngine", "PYRUNFILE"],
     "found": ["组 'PyArxEngine' 注册清单含 PYRUNFILE（registered_executed）",
               "arxload PyArxEngine.crx -> command PYRUNFILE 运行证据"],
     "missing": [],
     "verdict": "hit",
     "grounded_excerpt": "addCommand 组 'PyArxEngine' 注册清单=[PYLOAD,PYRUN,PYEXEC,PIPM,PYDEBUG,PYARXDEPLOY,PYRUNFILE]"},
    {"q": "PyEdEnt 这个类怎么用？", "matched": ["Ed.PyEdEnt"],
     "expected": ["不存在", "AcEdJig", "SelectionSet"],
     "found": ["hasattr(P.Ed.PyEdEnt)=False（不存在，name_trap_missing）",
               "Ed.AcEdJig（对应陷阱名 PyEdJig）",
               "Ed.SelectionSet + ssget/ssadd（对应 PyEdSel/PyEdSelectionSets）",
               "AdsName/entget/entmake/handent/entlast/EntityValue/SubEntityValue"],
     "missing": [],
     "verdict": "hit",
     "grounded_excerpt": "hasattr=False；真实对应 Ed.AcEdJig、Ed.SelectionSet+ssget/ssadd；AdsName/entget/entmake/handent/entlast"},
    {"q": "PyGiColor 怎么设颜色？", "matched": ["Gi.PyGiColor"],
     "expected": ["不存在", "AcCmEntityColor", "setTrueColor"],
     "found": ["hasattr=False 不存在（exists_missing）",
               "setTrueColor(AcCmEntityColor) / setColor(int) 替代",
               "真实类 AcCmEntityColor、AcGiColorRGB、AcGiColorRGBA"],
     "missing": [],
     "verdict": "hit",
     "grounded_excerpt": "不存在；替代 setColor(int)/setTrueColor(AcCmEntityColor)；真实类 AcCmEntityColor、AcGiColorRGB、AcGiColorRGBA"},
]

qa_no_retrieval = [
    {"q": "PyDbCircle 怎么改半径？", "matched": [], "verdict": "fail",
     "answer_excerpt": "圆的中心是 (0,0)，半径是 100000000000000…（编造超大半径）",
     "reason": "未提 setRadius/radius 往返与法向构造，走基座几何先验。"},
    {"q": "PyGeMatrix3d 怎么平移？", "matched": [], "verdict": "fail",
     "answer_excerpt": "m = PyGeMatrix3d(1,2,3) * PyGeVector3d(4,5,6)…行列式为 0 不平移…",
     "reason": "编造 PyGeMatrix3d/PyGeVector3d 构造与乘法；未答 setToTranslation/p.transformBy，未说不存在。"},
    {"q": "PYRUNFILE 是哪个项目注册的？", "matched": [], "verdict": "fail",
     "answer_excerpt": "不是 PyArxCAD，是 PyArxCore 里的一组函数，比如 py_runfile 执行文件脚本…",
     "reason": "未答 PyArxEngine；归到 PyArxCore/py_runfile，命令组/crx 未提。"},
    {"q": "PyEdEnt 这个类怎么用？", "matched": [], "verdict": "fail",
     "answer_excerpt": "PyEdEnt 是 PyArxEngine 的辅助工具类…getEntityByPath/getEntityByHandle…",
     "reason": "未说不存在；编造辅助类与 getEntityByPath/Handle，未给 AcEdJig/SelectionSet/ssget/entget。"},
    {"q": "PyGiColor 怎么设颜色？", "matched": [], "verdict": "fail",
     "answer_excerpt": "PyGiColor 不能设置颜色，只能取色表…setRGB(255,0,0)…setHSL(…)",
     "reason": "未说不存在；围绕未证实 setRGB/setHSL，未给 AcCmEntityColor/setTrueColor/setColor(int)。"},
]

# ---- 带检索：命中模块 + 注入记录 + 模型回答 + 判定 ----
qa_with_retrieval = [
    {"q": "PyDbCircle 怎么改半径？", "matched": ["Db.PyDbCircle"], "verdict": "fail",
     "injected": "prop_roundtrip_radius_center: radius 33.25->33.2500 center->(10.000,20.000,0.000) 回读类型=AcGePoint3d；exists+construct: 空构造 members=164",
     "answer_excerpt": "半径是 33.25，所以 radius=33.2500 不行。半径必须小数点后两位，不能是浮点数…循环复述 33.2500=33.25",
     "reason": "引用到实测 33.25→33.2500 碎片，但未给出 setRadius 方法名与构造第二参数法向；把回读精度误说成“两位小数约束”。"},
    {"q": "PyGeMatrix3d 怎么平移？", "matched": ["Ge.AcGeMatrix3d"], "verdict": "fail",
     "injected": "setToTranslation: setToTranslation(10,0,0) translation=[10.0,0.0,0.0] (1,2,3)->(+11.000,+2.000,+3.000)；setToScaling 中心 Point3d；exists+construct 无参 det=1",
     "answer_excerpt": "PyGeAcGeMatrix3d 是抽象基类，没有 setToTranslation/setToScaling；要平移先构造 GeAcGeMatrix3d，再调 setToTranslation 或 setToScaling…",
     "reason": "出现真实方法名 setToTranslation，但类名被拼坏（PyGeAcGeMatrix3d/GeAcGeMatrix3d），未说 PyGeMatrix3d 不存在，未给点 p.transformBy(m)。"},
    {"q": "PYRUNFILE 是哪个项目注册的？", "matched": ["Engine.PYRUNFILE.command_chain"], "verdict": "fail",
     "injected": "registered_executed: 源码 acrxEntryPoint.cpp kInitAppMsg addCommand 组 'PyArxEngine' 注册清单=[PYLOAD,PYRUN,PYEXEC,PIPM,PYDEBUG,PYARXDEPLOY,PYRUNFILE]；arxload PyArxEngine.crx",
     "answer_excerpt": "Acryl Engine 项目注册了 PYRUNFILE…2.0 已注册到 acrxEntryPoint.cpp kInitAppMsg，1.0 未注册…",
     "reason": "引用到 acrxEntryPoint/kInitAppMsg 碎片，但项目名被误读为“Acryl Engine”，未给出 PyArxEngine 与 PyArxEngine.crx。"},
    {"q": "PyEdEnt 这个类怎么用？", "matched": ["Ed.PyEdEnt"], "verdict": "fail",
     "injected": "name_trap_missing: hasattr(P.Ed.PyEdEnt)=False；真实存在对应物 EntityValue/SubEntityValue/AdsName/entget/entmake/handent/entlast",
     "answer_excerpt": "PyEdEnt 本质是 PyArxEngine 的子类…没有 getEntityValue()，只有 getEntity()/makeEntity()，返回 None 表示实体不存在…",
     "reason": "未吸收“hasattr=False”结论；仍按存在子类处理，编造 getEntity/makeEntity，未列真实替代 AdsName/entget/entmake/handent/entlast。"},
    {"q": "PyGiColor 怎么设颜色？", "matched": ["Gi.PyGiColor"], "verdict": "fail",
     "injected": "exists_missing: hasattr=False 不存在；替代 PyGiDrawableTraits.setColor(int)/setTrueColor(AcCmEntityColor) 或 PyDbEntity.colorRGB；真实类 AcCmEntityColor/AcGiColorRGB/AcGiColorRGBA",
     "answer_excerpt": "别问这个…setTrueColor(0xFF0000FF) 就是瞎扯…AcGiColorRGB(255,0,0) 就是瞎扯…getTrueColor/setTrueColor…",
     "reason": "把真实替代 setTrueColor/AcGiColorRGB 当成“瞎扯”否定掉；未说 PyGiColor 不存在，未给 AcCmEntityColor/setColor(int)。"},
]

acceptance = {
    "matrix": {
        "layers": layers,
        "missing_classes": missing_classes,
        "note": "Db 含旧矩阵回填的 PyDbCircle（9 条）；extra_scan 不计入模块。七组均 >=3 模块；缺四类断言的模块已逐条列出，未伪造补齐。",
    },
    "dataset": {
        "total": len(rows), "tools": len(tools), "facts_constraints": len(fc),
        "ratio": round(len(fc) / len(rows), 4), "fact_to_module": fact_to_module,
    },
    "adapter_rollback": {
        "current_adapter": r"G:\PyArx\tools\train\out\qwen0.5b-cad-lora",
        "source": r"G:\PyArx\tools\train\out\qwen0.5b-cad-lora-v3",
        "note": "train5 产物留档 out\\qwen0.5b-cad-lora-v5-exp 备查；v1/v2/v3 保留未删。回滚后 eval 4/6 验证通过。",
        "retained": [
            r"G:\PyArx\tools\train\out\qwen0.5b-cad-lora-v1",
            r"G:\PyArx\tools\train\out\qwen0.5b-cad-lora-v2",
            r"G:\PyArx\tools\train\out\qwen0.5b-cad-lora-v3",
            r"G:\PyArx\tools\train\out\qwen0.5b-cad-lora-v5-exp",
        ],
    },
    "eval": eval_v6,
    "qa_model_only": {"hit": 0, "total": 5, "note": "强制 retrieval=false，模型纯生成；grounded 为空；与回滚基线一致 0/5。"},
    "qa_no_retrieval": {"hit": 0, "total": 5, "items": qa_no_retrieval},
    "qa_with_retrieval": {"hit": 0, "total": 5, "items": qa_with_retrieval},
    "qa_grounded": {
        "full_match": 5, "partial": 0, "hit_any_real_name": 5, "total": 5,
        "note": "grounded 为抽取式直答，非模型答对；5 条期望类/方法全部在矩阵 detail 中出现。",
        "items": qa_grounded,
    },
    "answer_path": ("命中 >=1 矩阵模块即走抽取式直答：按模板把模块 check 的 name+detail（截 320 字）原样拼出，"
                    "不改数值与异常原文；几何变换族（AcGeMatrix3d/2d/Scale3d）额外带出 Ge.AcGePoint3d 的 transformBy check；"
                    "模型生成结果（raw）并列保留作补充，事实层不经过生成；无命中不注入、不生成 grounded。"),
    "retrieval_design": ("启动/首调读入 module->checks 内存索引；问题抽 [A-Za-z][A-Za-z0-9_]{3,} 词，"
                         "大小写不敏感子串匹配，PyXxx 未命中时试 AcXxx 别名；前 3 个模块的前 3 条断言"
                         "（detail 截 200 字）拼到 user 消息尾部，system 不动；无命中不注入。"),
    "sys_mismatch_note": ("训练 build_distill_data.py 的 SYS 已改回纯工具口径（v3 即在此口径训出）；"
                          "playground/app.py 保留混合口径（允许回答模块 API，产品体验需要）。训练/推理 SYS 不同源，如实记录。"),
    "root_cause": ("0.5B 容量不足以内化 78 个模块的类名/方法/签名；无检索时退回基座先验编造。"
                   "检索兜底能把矩阵碎片带进上下文，但模型只能复述/拼坏碎片（33.2500、setToTranslation、"
                   "kInitAppMsg、setTrueColor/AcGiColorRGB 均出现过），组织不出干净、无矛盾的 API 回答；"
                   "陷阱名“不存在+真实替代”的否定结论也未被稳定吸收。"),
    "next_step": [
        "要真正靠模型内化：事实行按问答×3 变体渲染，陷阱名单独补“不存在→真实替代”QA 对；"
        "同时补 15-20 条工具样本，把事实约束占比压到 30% 以下，先保住工具格式。",
        "或换更大基座（需用户拍板；本轮未升基座，仍为 D:\\models\\qwen2.5-0.5b）。",
        "检索侧可再加强：把命中断言按关键词排序、每条陷阱名直接带“不存在”结论句；当前为子串匹配，未做向量检索。",
    ],
    "limitations": [
        "未改 pyd/crx/源码、未碰 tests/*.dwg、未升基座；除授权终止的 14684、32864 外未 kill 其它进程。",
        "playground 当前 PID 28272；status: ready=true, lora=true, adapter_files=6, matrix_modules=78, err 空。",
    ],
}

with open(os.path.join(BASE, "acceptance.json"), "w", encoding="utf-8") as f:
    json.dump(acceptance, f, ensure_ascii=False, indent=2)
print("dataset", acceptance["dataset"]["total"], acceptance["dataset"]["tools"],
      acceptance["dataset"]["facts_constraints"], acceptance["dataset"]["ratio"])
print("eval", eval_v6["hit"], "/", eval_v6["total"])
print("qa no-retrieval", acceptance["qa_no_retrieval"]["hit"], "/5",
      "with-retrieval", acceptance["qa_with_retrieval"]["hit"], "/5")
