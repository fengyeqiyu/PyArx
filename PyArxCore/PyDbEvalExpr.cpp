#include "stdafx.h"
#include "PyErrorStatusException.h"
#include "PyDbEvalExpr.h"
#include "PyDbEvalVariant.h"

PyWrapper_Cpp(DbEvalExpr);

void bind_PyDbEvalExpr(pym& m) {
	py::class_<PyDbEvalExpr, PyDbObject>(m, "PyDbEvalExpr")
		.def(py::init<AcDbEvalExpr*>(), py::arg("p"))
		.def(py::init([]() { return std::make_unique<PyDbEvalExpr>(new AcDbEvalExpr()).release(); }), "无参构造（new 默认 AcDbEvalExpr）")
		PyWrapper_BIND(DbEvalExpr)
		// ---- nodeId() -> uint32 (AcDbEvalNodeId = Adesk::UInt32，kNullNodeId=0) ----
		.def("nodeId", [](PyDbEvalExpr& s) -> uint32_t {
			return static_cast<uint32_t>(s.impObj()->nodeId());
		})
		// ---- value() -> PyDbEvalVariant（按值返回表达式值） ----
		.def("value", [](const PyDbEvalExpr& s) {
			return PyDbEvalVariant(s.impObj()->value());
		}, "返回表达式求值后的 AcDbEvalVariant 值（默认 kNone）。")
		// ---- getGraph(AcDbEvalGraph** pGraph, OpenMode) -> PyDbObject（基类包装，调用方用完 close） ----
		.def("getGraph", [](PyDbEvalExpr& s, int mode) -> PyDbObject {
			AcDbEvalGraph* pGraph = nullptr;
			if (!PyWarnIfError(s.impObj()->getGraph(&pGraph, static_cast<AcDb::OpenMode>(mode)), L"AcDbEvalExpr::getGraph"))
				return PyDbObject(nullptr);
			return PyDbObject(pGraph);
		}, py::arg("mode") = 0, "打开拥有本节点的图；返回基类 PyDbObject（db 驻留，析构时自动 close）。待 PyDbEvalGraph 包装后可细化。")
		;
	// 跳过（均为图回调虚函数，依赖未封装类型 / Python 子类化 trampoline，P2 依赖）：
	//   addedToGraph / removedFromGraph / adjacentNodeRemoved / adjacentEdgeRemoved /
	//   adjacentEdgeAdded / movedFromGraph / movedIntoGraph / remappedNodeIds /
	//   copiedIntoGraph / isActivatable / activated / graphEvalStart / graphEvalEnd /
	//   graphEvalAbort / evaluate / equals / postInDatabase。
	// 原因：入参为 AcDbEvalGraph* / AcDbEvalNodeIdArray / AcDbEvalContext*（无 Python 包装），
	//       且这些是图内部回调钩子，Python 子类化需 GIL 感知 trampoline；待 PyDbEvalGraph /
	//       PyDbEvalContext 封装后再补。addGraph：CHM 无页面，疑误提取，跳过。
}
