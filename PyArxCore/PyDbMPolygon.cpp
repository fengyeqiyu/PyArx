#include "stdafx.h"
#include "PyErrorStatusException.h"
#include "PyDbMPolygon.h"
#include "PyDbHatch.h"
// appendLoopFromBoundary 按运行时类型分派，需要完整类定义（dbMPolygon.h 仅前置声明）
// AcDbCircle / AcDb2dPolyline 均在 dbents.h；AcDbPolyline 在 dbpl.h
#include "dbents.h"
#include "dbpl.h"
#include "PyUtils.h"

double PyDbMPolygon::getArea() const { return static_cast<AcDbMPolygon*>(impObj())->getArea(); }

AcString PyDbMPolygon::toString() const { return AcString(std::format("AcDbMPolygon[wrapper]").c_str(), AcString::Utf8); }

void bind_PyDbMPolygon(pym& m) {
	py::class_<PyDbMPolygon, PyDbEntity>(m, "PyDbMPolygon")
		.def(py::init<>())
		.def(py::init<AcDbMPolygon*>(), py::arg("object"))
		.def("getArea", &PyDbMPolygon::getArea)
		// ---- hatch 成员 / 平面 ----
		.def("hatch", [](PyDbMPolygon& s) -> PyDbHatch {
			auto* h = static_cast<AcDbMPolygon*>(s.impObj())->hatch();
			return PyDbHatch(h);  // 内部成员，不接管内存
		}, "内部 AcDbHatch 对象")
		.def("elevation", [](const PyDbMPolygon& s){ return static_cast<AcDbMPolygon*>(s.impObj())->elevation(); })
		.def("setElevation", [](PyDbMPolygon& s, double v){ return PyWarnIfError(static_cast<AcDbMPolygon*>(s.impObj())->setElevation(v), L"setElevation"); }, py::arg("v"))
		.def("normal", [](const PyDbMPolygon& s){ return static_cast<AcDbMPolygon*>(s.impObj())->normal(); })
		.def("setNormal", [](PyDbMPolygon& s, const AcGeVector3d& n){ return PyWarnIfError(static_cast<AcDbMPolygon*>(s.impObj())->setNormal(n), L"setNormal"); }, py::arg("n"))
		// ---- 几何 ----
		.def("getPerimeter", [](const PyDbMPolygon& s){ return static_cast<AcDbMPolygon*>(s.impObj())->getPerimeter(); })
		.def("isBalanced", [](const PyDbMPolygon& s){ return static_cast<AcDbMPolygon*>(s.impObj())->isBalanced(); })
		.def("getOffsetVector", [](const PyDbMPolygon& s){ return static_cast<AcDbMPolygon*>(s.impObj())->getOffsetVector(); })
		.def("includesTouchingLoops", [](const PyDbMPolygon& s){ return static_cast<AcDbMPolygon*>(s.impObj())->includesTouchingLoops(); })
		// ---- 图案 ----
		.def("patternType", [](const PyDbMPolygon& s){ return static_cast<AcDbMPolygon*>(s.impObj())->patternType(); })
		.def("patternName", [](const PyDbMPolygon& s) -> AcString { return static_cast<AcDbMPolygon*>(s.impObj())->patternName(); })
		.def("setPattern", [](PyDbMPolygon& s, int type, const AcString& name){
			return PyWarnIfError(static_cast<AcDbMPolygon*>(s.impObj())->setPattern(static_cast<AcDbHatch::HatchPatternType>(type), name.kwszPtr()), L"setPattern");
		}, py::arg("type"), py::arg("name"))
		.def("patternAngle", [](const PyDbMPolygon& s){ return static_cast<AcDbMPolygon*>(s.impObj())->patternAngle(); })
		.def("setPatternAngle", [](PyDbMPolygon& s, double v){ return PyWarnIfError(static_cast<AcDbMPolygon*>(s.impObj())->setPatternAngle(v), L"setPatternAngle"); }, py::arg("v"))
		.def("patternSpace", [](const PyDbMPolygon& s){ return static_cast<AcDbMPolygon*>(s.impObj())->patternSpace(); })
		.def("setPatternSpace", [](PyDbMPolygon& s, double v){ return PyWarnIfError(static_cast<AcDbMPolygon*>(s.impObj())->setPatternSpace(v), L"setPatternSpace"); }, py::arg("v"))
		.def("patternScale", [](const PyDbMPolygon& s){ return static_cast<AcDbMPolygon*>(s.impObj())->patternScale(); })
		.def("setPatternScale", [](PyDbMPolygon& s, double v){ return PyWarnIfError(static_cast<AcDbMPolygon*>(s.impObj())->setPatternScale(v), L"setPatternScale"); }, py::arg("v"))
		.def("patternDouble", [](const PyDbMPolygon& s){ return static_cast<AcDbMPolygon*>(s.impObj())->patternDouble(); })
		.def("setPatternDouble", [](PyDbMPolygon& s, bool v){ return PyWarnIfError(static_cast<AcDbMPolygon*>(s.impObj())->setPatternDouble(v), L"setPatternDouble"); }, py::arg("v"))
		// ---- 环操作 ----
		.def("numLoops", [](const PyDbMPolygon& s){ return static_cast<AcDbMPolygon*>(s.impObj())->numMPolygonLoops(); }, "环数量")
		.def("appendLoop", [](PyDbMPolygon& s, const AcGePoint2dArray& verts, const AcGeDoubleArray& bulges){
			auto* p = static_cast<AcDbMPolygon*>(s.impObj());
			return PyWarnIfError(p->appendMPolygonLoop(verts, bulges), L"MPolygon.appendLoop");
		}, py::arg("vertices"), py::arg("bulges"), "追加一个环（顶点数组 + 凸度数组）")
		.def("removeLoopAt", [](PyDbMPolygon& s, int i){
			auto* p = static_cast<AcDbMPolygon*>(s.impObj());
			return PyWarnIfError(p->removeMPolygonLoopAt(i), L"MPolygon.removeLoopAt");
		}, py::arg("index"), "删除指定环")
		.def("balanceTree", [](PyDbMPolygon& s){
			auto* p = static_cast<AcDbMPolygon*>(s.impObj());
			return PyWarnIfError(p->balanceTree(), L"MPolygon.balanceTree");
		}, "平衡环树（建环后调用）")
		.def("closestLoopTo", [](const PyDbMPolygon& s, const AcGePoint3d& pt){
			return static_cast<AcDbMPolygon*>(s.impObj())->getClosestLoopTo(pt);
		}, py::arg("world_pt"), "离世界点最近的环索引")
		.def("parentLoop", [](const PyDbMPolygon& s, int cur){ return static_cast<AcDbMPolygon*>(s.impObj())->getParentLoop(cur); }, py::arg("current_loop"))
		.def("loopDirection", [](const PyDbMPolygon& s, int i) -> int {
			AcDbMPolygon::loopDir d = AcDbMPolygon::kExterior;
			if (!s.impObj()) return -1;
			Acad::ErrorStatus es = static_cast<AcDbMPolygon*>(s.impObj())->getLoopDirection(i, d);
			if (!PyWarnIfError(es, L"AcDbMPolygon::getLoopDirection")) return -1;
			return (int)d;
		}, py::arg("index"), "环方向 0=外/1=内/2=标注")
		.def("setLoopDirection", [](PyDbMPolygon& s, int i, int d){
			return PyWarnIfError(static_cast<AcDbMPolygon*>(s.impObj())->setLoopDirection(i, (AcDbMPolygon::loopDir)d), L"setLoopDirection");
		}, py::arg("index"), py::arg("dir"))
		.def("evaluateHatch", [](PyDbMPolygon& s){
			auto* p = static_cast<AcDbMPolygon*>(s.impObj());
			return PyWarnIfError(p->evaluateHatch(false), L"MPolygon.evaluateHatch");
		}, "重新计算填充路径")
		// ---- 图案定义 / 图案色 / 显示 ----
		.def("numPatternDefinitions", [](const PyDbMPolygon& s){
			return static_cast<AcDbMPolygon*>(s.impObj())->numPatternDefinitions();
		}, "图案定义数量")
		.def("getPatternDefinitionAt", [](const PyDbMPolygon& s, int index) -> std::tuple<double, double, double, double, double, AcGeDoubleArray> {
			double angle = 0, baseX = 0, baseY = 0, offX = 0, offY = 0;
			AcGeDoubleArray dashes;
			if (!s.impObj()) return { 0, 0, 0, 0, 0, AcGeDoubleArray() };
			Acad::ErrorStatus es = static_cast<AcDbMPolygon*>(s.impObj())->getPatternDefinitionAt(index, angle, baseX, baseY, offX, offY, dashes);
			if (!PyWarnIfError(es, L"AcDbMPolygon::getPatternDefinitionAt"))
				return { 0, 0, 0, 0, 0, AcGeDoubleArray() };
			return { angle, baseX, baseY, offX, offY, dashes };
		}, py::arg("index"), "读取图案定义: (angle, baseX, baseY, offsetX, offsetY, dashes)")
		.def("patternColor", [](const PyDbMPolygon& s) -> std::tuple<int, int, int> {
			AcCmColor c = static_cast<AcDbMPolygon*>(s.impObj())->patternColor();
			return { (int)c.red(), (int)c.green(), (int)c.blue() };
		}, "图案填充色 (r,g,b) 0-255")
		.def("setPatternColor", [](PyDbMPolygon& s, int r, int g, int b) -> bool {
			AcCmColor c;
			c.setRGB((Adesk::UInt8)r, (Adesk::UInt8)g, (Adesk::UInt8)b);
			return PyWarnIfError(static_cast<AcDbMPolygon*>(s.impObj())->setPatternColor(c), L"MPolygon.setPatternColor");
		}, py::arg("r"), py::arg("g"), py::arg("b"), "设置图案填充色 0-255")
		.def("balanceDisplay", [](PyDbMPolygon& s) -> bool {
			return PyWarnIfError(static_cast<AcDbMPolygon*>(s.impObj())->balanceDisplay(), L"MPolygon.balanceDisplay");
		}, "平衡显示")
		// ---- 环：边界生成 / 读取 / 插入 / 子环 / gsMarker ----
		.def("appendLoopFromBoundary", [](PyDbMPolygon& s, const PyDbEntity& boundary, bool excludeCrossing, double tol) -> bool {
			auto* p = static_cast<AcDbMPolygon*>(s.impObj());
			AcDbEntity* bo = boundary.impObj();
			if (!bo) {
				PyWarnIfError(Acad::eNullObjectPointer, L"MPolygon.appendLoopFromBoundary");
				return false;
			}
			AcRxClass* circleCls = AcDbCircle::desc();
			AcRxClass* plineCls = AcDbPolyline::desc();
			AcRxClass* p2dCls = AcDb2dPolyline::desc();
			Acad::ErrorStatus es = Acad::eInvalidInput;
			if (bo->isKindOf(circleCls))
				es = p->appendLoopFromBoundary(static_cast<const AcDbCircle*>(bo), excludeCrossing, tol);
			else if (bo->isKindOf(plineCls))
				es = p->appendLoopFromBoundary(static_cast<const AcDbPolyline*>(bo), excludeCrossing, tol);
			else if (bo->isKindOf(p2dCls))
				es = p->appendLoopFromBoundary(static_cast<const AcDb2dPolyline*>(bo), excludeCrossing, tol);
			else {
				acutPrintf(L"[PyArx] appendLoopFromBoundary: boundary must be Circle/Polyline/2dPolyline\n");
				return false;
			}
			return PyWarnIfError(es, L"MPolygon.appendLoopFromBoundary");
		}, py::arg("boundary"), py::arg("exclude_crossing") = true, py::arg("tol") = 1e-6, "由圆/多段线边界生成环")
		// ---- 空间查询 / 批量建环 ----
		.def("isPointInsideMPolygon", [](const PyDbMPolygon& s, const AcGePoint3d& worldPt, double tol) -> std::tuple<int, std::vector<int>> {
			auto* p = static_cast<AcDbMPolygon*>(s.impObj());
			AcGeIntArray loops;
			int cnt = p->isPointInsideMPolygon(worldPt, loops, tol);
			std::vector<int> out;
			out.reserve(loops.length());
			for (int i = 0; i < loops.length(); ++i) out.push_back(loops[i]);
			return { cnt, out };
		}, py::arg("world_pt"), py::arg("tol") = AcDbMPolygonCrossingFuzz, "点是否在多边形内；返回 (命中环数, 环索引列表)")
		.def("isPointOnLoopBoundary", [](const PyDbMPolygon& s, const AcGePoint3d& worldPt, int loop, double tol) -> bool {
			return static_cast<AcDbMPolygon*>(s.impObj())->isPointOnLoopBoundary(worldPt, loop, tol) != Adesk::kFalse;
		}, py::arg("world_pt"), py::arg("loop"), py::arg("tol") = AcDbMPolygonCrossingFuzz, "点是否在指定环边界上")
		.def("createLoopsFromBoundaries", [](PyDbMPolygon& s, const AcDbObjectIdArray& ids, bool excludeCrossing, double tol) -> std::tuple<bool, std::vector<int>> {
			auto* p = static_cast<AcDbMPolygon*>(s.impObj());
			AcDbIntArray rejected;
			Acad::ErrorStatus es = p->createLoopsFromBoundaries(ids, rejected, excludeCrossing, tol);
			std::vector<int> out;
			out.reserve(rejected.length());
			for (int i = 0; i < rejected.length(); ++i) out.push_back(rejected[i]);
			return { PyWarnIfError(es, L"MPolygon.createLoopsFromBoundaries"), out };
		}, py::arg("ids"), py::arg("exclude_crossing") = true, py::arg("tol") = AcDbMPolygonCrossingFuzz, "由边界 id 批量建环；返回 (是否成功, 被拒对象索引列表)")
		.def("createLoops", [](PyDbMPolygon& s, const py::list& vertices, const py::list& bulges, bool excludeCrossing, double tol) -> std::tuple<bool, std::vector<int>> {
			auto* p = static_cast<AcDbMPolygon*>(s.impObj());
			AcArray<AcGePoint2dArray> verts;
			AcArray<AcGeDoubleArray> bul;
			for (auto h : vertices) verts.append(py::cast<AcGePoint2dArray>(h.cast<py::list>()));
			for (auto h : bulges) bul.append(py::cast<AcGeDoubleArray>(h.cast<py::list>()));
			AcDbIntArray rejected;
			Acad::ErrorStatus es = p->createLoops(verts, bul, rejected, excludeCrossing, tol);
			std::vector<int> out;
			out.reserve(rejected.length());
			for (int i = 0; i < rejected.length(); ++i) out.push_back(rejected[i]);
			return { PyWarnIfError(es, L"MPolygon.createLoops"), out };
		}, py::arg("vertices"), py::arg("bulges"), py::arg("exclude_crossing") = true, py::arg("tol") = AcDbMPolygonCrossingFuzz, "批量点列建环；vertices=[[[x,y],...],...], bulges=[[d,...],...]；返回 (是否成功, 被拒环索引)")
		.def("getMPolygonLoopAt", [](const PyDbMPolygon& s, int index) -> std::tuple<AcGePoint2dArray, AcGeDoubleArray> {
			AcGePoint2dArray verts;
			AcGeDoubleArray bulges;
			if (!s.impObj()) return { AcGePoint2dArray(), AcGeDoubleArray() };
			Acad::ErrorStatus es = static_cast<AcDbMPolygon*>(s.impObj())->getMPolygonLoopAt(index, verts, bulges);
			if (!PyWarnIfError(es, L"AcDbMPolygon::getMPolygonLoopAt"))
				return { AcGePoint2dArray(), AcGeDoubleArray() };
			return { verts, bulges };
		}, py::arg("index"), "读取环: (顶点列, 凸度列)")
		.def("insertMPolygonLoopAt", [](PyDbMPolygon& s, int index, const AcGePoint2dArray& verts, const AcGeDoubleArray& bulges, bool excludeCrossing, double tol) -> bool {
			return PyWarnIfError(static_cast<AcDbMPolygon*>(s.impObj())->insertMPolygonLoopAt(index, verts, bulges, excludeCrossing, tol), L"MPolygon.insertMPolygonLoopAt");
		}, py::arg("index"), py::arg("vertices"), py::arg("bulges"), py::arg("exclude_crossing") = true, py::arg("tol") = 1e-6, "在指定索引插入环")
		.def("getChildLoops", [](const PyDbMPolygon& s, int curLoop) -> std::vector<int> {
			AcGeIntArray arr;
			static_cast<AcDbMPolygon*>(s.impObj())->getChildLoops(curLoop, arr);
			std::vector<int> out;
			out.reserve(arr.length());
			for (int i = 0; i < arr.length(); ++i) out.push_back(arr[i]);
			return out;
		}, py::arg("current_loop"), "获取子环索引列表")
		.def("getLoopAtGsMarker", [](const PyDbMPolygon& s, int gsMark) -> int {
			int loop = -1;
			if (!s.impObj()) return -1;
			Acad::ErrorStatus es = static_cast<AcDbMPolygon*>(s.impObj())->getLoopAtGsMarker(gsMark, loop);
			if (!PyWarnIfError(es, L"AcDbMPolygon::getLoopAtGsMarker")) return -1;
			return loop;
		}, py::arg("gs_marker"), "gsMarker 对应的环索引，失败返回 -1")
		// ---- 环树：扁平为 (loopIndex, parentLoopIndex)，遍历后内部 deleteMPolygonTree 释放 ----
		.def("getMPolygonTree", [](const PyDbMPolygon& s) -> std::vector<std::pair<int, int>> {
			auto* p = static_cast<AcDbMPolygon*>(s.impObj());
			AcDbMPolygonNode* root = nullptr;
			if (!p) return {};
			Acad::ErrorStatus es = p->getMPolygonTree(root);
			if (!PyWarnIfError(es, L"AcDbMPolygon::getMPolygonTree") || root == nullptr) return {};
			std::vector<std::pair<int, int>> out;
			std::function<void(AcDbMPolygonNode*, int)> walk = [&](AcDbMPolygonNode* node, int parent) {
				if (node == nullptr) return;
				out.push_back({ node->mLoopIndex, parent });
				for (int i = 0; i < node->mChildren.length(); ++i)
					walk(node->mChildren[i], node->mLoopIndex);
			};
			walk(root, -1);
			p->deleteMPolygonTree(root);
			return out;
		}, "环树: [(loopIndex, parentLoopIndex),...]，根父节点为 -1")
		// ---- 渐变 ----
		.def("setGradient", [](PyDbMPolygon& s, int gradType, const AcString& gradName) -> bool {
			return PyWarnIfError(static_cast<AcDbMPolygon*>(s.impObj())->setGradient((AcDbHatch::GradientPatternType)gradType, gradName.kwszPtr()), L"MPolygon.setGradient");
		}, py::arg("grad_type"), py::arg("grad_name"), "设置渐变图案")
		.def("setGradientAngle", [](PyDbMPolygon& s, double v){
			return PyWarnIfError(static_cast<AcDbMPolygon*>(s.impObj())->setGradientAngle(v), L"MPolygon.setGradientAngle");
		}, py::arg("angle"))
		.def("setGradientShift", [](PyDbMPolygon& s, float v){
			return PyWarnIfError(static_cast<AcDbMPolygon*>(s.impObj())->setGradientShift(v), L"MPolygon.setGradientShift");
		}, py::arg("shift"))
		.def("setGradientOneColorMode", [](PyDbMPolygon& s, bool v){
			return PyWarnIfError(static_cast<AcDbMPolygon*>(s.impObj())->setGradientOneColorMode(v ? Adesk::kTrue : Adesk::kFalse), L"MPolygon.setGradientOneColorMode");
		}, py::arg("one_color"))
		.def("setGradientColors", [](PyDbMPolygon& s, const std::vector<std::tuple<int, int, int>>& colors, const std::vector<float>& values) -> bool {
			std::vector<AcCmColor> cc(colors.size());
			for (size_t i = 0; i < colors.size(); ++i) {
				auto [r, g, b] = colors[i];
				cc[i].setRGB((Adesk::UInt8)r, (Adesk::UInt8)g, (Adesk::UInt8)b);
			}
			std::vector<float> vals(colors.size(), 0.0f);
			for (size_t i = 0; i < values.size() && i < colors.size(); ++i) vals[i] = values[i];
			return PyWarnIfError(static_cast<AcDbMPolygon*>(s.impObj())->setGradientColors((unsigned int)colors.size(), cc.data(), vals.data()), L"MPolygon.setGradientColors");
		}, py::arg("colors"), py::arg("values"), "渐变色 [(r,g,b),...] 与位置 [0..1]")
		.def("__repr__", &PyDbMPolygon::toString)
		;
}
