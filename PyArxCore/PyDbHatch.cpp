#include "stdafx.h"
#include "PyErrorStatusException.h"
#include "PyDbHatch.h"

PyWrapper_Cpp(DbHatch)
AcString PyDbHatch::toString() const { return AcString(std::format("AcDbHatch[wrapper]").c_str(), AcString::Utf8); }
bool PyDbHatch::associative() const { return impObj()->associative(); }

bool  PyDbHatch::setAssociative(bool a) { return PyWarnIfError(impObj()->setAssociative(a), L"AcDbHatch::setAssociative"); }

int PyDbHatch::numLoops() const { return impObj()->numLoops(); }
int PyDbHatch::loopTypeAt(int i) const { return static_cast<int>(impObj()->loopTypeAt(i)); }
double PyDbHatch::elevation() const { return impObj()->elevation(); }
bool  PyDbHatch::setElevation(double v) { return PyWarnIfError(impObj()->setElevation(v), L"AcDbHatch::setElevation"); }

AcGeVector3d PyDbHatch::normal() const { return impObj()->normal(); }
bool  PyDbHatch::setNormal(const AcGeVector3d& v) { return PyWarnIfError(impObj()->setNormal(v), L"AcDbHatch::setNormal"); }
int PyDbHatch::hatchObjectType() const { return static_cast<int>(impObj()->hatchObjectType()); }
bool  PyDbHatch::setHatchObjectType(int v) { return PyWarnIfError(impObj()->setHatchObjectType(static_cast<AcDbHatch::HatchObjectType>(v)), L"AcDbHatch::setHatchObjectType"); }
bool PyDbHatch::isGradient() const { return impObj()->isGradient() != Adesk::kFalse; }
bool PyDbHatch::isHatch() const { return impObj()->isHatch() != Adesk::kFalse; }
bool PyDbHatch::isSolidFill() const { return impObj()->isSolidFill(); }
AcGePoint2d PyDbHatch::originPoint() const { return impObj()->originPoint(); }
bool  PyDbHatch::setOriginPoint(const AcGePoint2d& p) { return PyWarnIfError(impObj()->setOriginPoint(p), L"AcDbHatch::setOriginPoint"); }
int PyDbHatch::patternType() const { return static_cast<int>(impObj()->patternType()); }
AcString PyDbHatch::patternName() const { return impObj()->patternName(); }
bool  PyDbHatch::setPattern(int t, const AcString& n) { return PyWarnIfError(impObj()->setPattern(static_cast<AcDbHatch::HatchPatternType>(t), n.kwszPtr()), L"AcDbHatch::setPattern"); }
double PyDbHatch::patternAngle() const { return impObj()->patternAngle(); }
bool  PyDbHatch::setPatternAngle(double v) { return PyWarnIfError(impObj()->setPatternAngle(v), L"AcDbHatch::setPatternAngle"); }
double PyDbHatch::patternSpace() const { return impObj()->patternSpace(); }
bool  PyDbHatch::setPatternSpace(double v) { return PyWarnIfError(impObj()->setPatternSpace(v), L"AcDbHatch::setPatternSpace"); }
double PyDbHatch::patternScale() const { return impObj()->patternScale(); }
bool  PyDbHatch::setPatternScale(double v) { return PyWarnIfError(impObj()->setPatternScale(v), L"AcDbHatch::setPatternScale"); }
bool PyDbHatch::patternDouble() const { return impObj()->patternDouble(); }
bool  PyDbHatch::setPatternDouble(bool v) { return PyWarnIfError(impObj()->setPatternDouble(v), L"AcDbHatch::setPatternDouble"); }
int PyDbHatch::numPatternDefinitions() const { return impObj()->numPatternDefinitions(); }
std::tuple<double, double, double, double, double, AcGeDoubleArray> PyDbHatch::getPatternDefinitionAt(int index) const {
	double angle = 0.0, baseX = 0.0, baseY = 0.0, offX = 0.0, offY = 0.0;
	AcGeDoubleArray dashes;
	if (impObj()->getPatternDefinitionAt(index, angle, baseX, baseY, offX, offY, dashes) != Acad::eOk)
		return { 0.0, 0.0, 0.0, 0.0, 0.0, AcGeDoubleArray() };
	return { angle, baseX, baseY, offX, offY, dashes };
}
int PyDbHatch::hatchStyle() const { return static_cast<int>(impObj()->hatchStyle()); }
bool  PyDbHatch::setHatchStyle(int v) { return PyWarnIfError(impObj()->setHatchStyle(static_cast<AcDbHatch::HatchStyle>(v)), L"AcDbHatch::setHatchStyle"); }
int PyDbHatch::numHatchLines() const { return impObj()->numHatchLines(); }
double PyDbHatch::area() const { double a = 0.0; return (impObj()->getArea(a) == Acad::eOk) ? a : 0.0; }
bool  PyDbHatch::evaluateHatch(bool u) { return PyWarnIfError(impObj()->evaluateHatch(u), L"AcDbHatch::evaluateHatch"); }
bool  PyDbHatch::evaluateHatchAllScales(bool u) { return PyWarnIfError(impObj()->evaluateHatchAllScales(u), L"AcDbHatch::evaluateHatchAllScales"); }
bool PyDbHatch::lineGenerationEnabled() const { return impObj()->lineGenerationEnabled(); }
bool PyDbHatch::setLineGenerationEnabled(bool v) { return impObj()->setLineGenerationEnabled(v); }
int PyDbHatch::gradientType() const { return static_cast<int>(impObj()->gradientType()); }
AcString PyDbHatch::gradientName() const { return impObj()->gradientName(); }
double PyDbHatch::gradientAngle() const { return impObj()->gradientAngle(); }
bool  PyDbHatch::setGradientAngle(double v) { return PyWarnIfError(impObj()->setGradientAngle(v), L"AcDbHatch::setGradientAngle"); }
bool PyDbHatch::gradientOneColorMode() const { return impObj()->getGradientOneColorMode() != Adesk::kFalse; }
bool  PyDbHatch::setGradientOneColorMode(bool v) { return PyWarnIfError(impObj()->setGradientOneColorMode(v ? Adesk::kTrue : Adesk::kFalse), L"AcDbHatch::setGradientOneColorMode"); }
float PyDbHatch::shadeTintValue() const { return impObj()->getShadeTintValue(); }
bool  PyDbHatch::setShadeTintValue(float v) { return PyWarnIfError(impObj()->setShadeTintValue(v), L"AcDbHatch::setShadeTintValue"); }
float PyDbHatch::gradientShift() const { return impObj()->gradientShift(); }
bool  PyDbHatch::setGradientShift(float v) { return PyWarnIfError(impObj()->setGradientShift(v), L"AcDbHatch::setGradientShift"); }

void bind_PyDbHatch(pym& m) {
	py::class_<PyDbHatch, PyDbEntity>(m, "PyDbHatch")
		.def(py::init<>())
		.def(py::init<AcDbHatch*>(), py::arg("object"))
		.def("associative", &PyDbHatch::associative)
		.def("setAssociative", &PyDbHatch::setAssociative, py::arg("a"))
		.def("numLoops", &PyDbHatch::numLoops)
		.def("loopTypeAt", &PyDbHatch::loopTypeAt, py::arg("loopIndex"))
		.def("elevation", &PyDbHatch::elevation)
		.def("setElevation", &PyDbHatch::setElevation, py::arg("v"))
		.def("normal", &PyDbHatch::normal)
		.def("setNormal", &PyDbHatch::setNormal, py::arg("v"))
		.def("hatchObjectType", &PyDbHatch::hatchObjectType)
		.def("setHatchObjectType", &PyDbHatch::setHatchObjectType, py::arg("v"))
		.def("isGradient", &PyDbHatch::isGradient)
		.def("isHatch", &PyDbHatch::isHatch)
		.def("isSolidFill", &PyDbHatch::isSolidFill)
		.def("originPoint", &PyDbHatch::originPoint)
		.def("setOriginPoint", &PyDbHatch::setOriginPoint, py::arg("p"))
		.def("patternType", &PyDbHatch::patternType)
		.def("patternName", &PyDbHatch::patternName)
		.def("setPattern", &PyDbHatch::setPattern, py::arg("patType"), py::arg("patName"))
		.def("patternAngle", &PyDbHatch::patternAngle)
		.def("setPatternAngle", &PyDbHatch::setPatternAngle, py::arg("v"))
		.def("patternSpace", &PyDbHatch::patternSpace)
		.def("setPatternSpace", &PyDbHatch::setPatternSpace, py::arg("v"))
		.def("patternScale", &PyDbHatch::patternScale)
		.def("setPatternScale", &PyDbHatch::setPatternScale, py::arg("v"))
		.def("patternDouble", &PyDbHatch::patternDouble)
		.def("setPatternDouble", &PyDbHatch::setPatternDouble, py::arg("v"))
		.def("numPatternDefinitions", &PyDbHatch::numPatternDefinitions, "图案定义数量")
		.def("getPatternDefinitionAt", &PyDbHatch::getPatternDefinitionAt, py::arg("index"), "读取图案定义: (angle, baseX, baseY, offsetX, offsetY, dashes)")
		.def("hatchStyle", &PyDbHatch::hatchStyle)
		.def("setHatchStyle", &PyDbHatch::setHatchStyle, py::arg("v"))
		.def("numHatchLines", &PyDbHatch::numHatchLines)
		.def("area", &PyDbHatch::area)
		.def("evaluateHatch", &PyDbHatch::evaluateHatch, py::arg("underestimate") = false)
		.def("evaluateHatchAllScales", &PyDbHatch::evaluateHatchAllScales, py::arg("underestimate") = false)
		.def("lineGenerationEnabled", &PyDbHatch::lineGenerationEnabled)
		.def("setLineGenerationEnabled", &PyDbHatch::setLineGenerationEnabled, py::arg("v"))
		.def("gradientType", &PyDbHatch::gradientType)
		.def("gradientName", &PyDbHatch::gradientName)
		.def("gradientAngle", &PyDbHatch::gradientAngle)
		.def("setGradientAngle", &PyDbHatch::setGradientAngle, py::arg("v"))
		.def("gradientOneColorMode", &PyDbHatch::gradientOneColorMode)
		.def("setGradientOneColorMode", &PyDbHatch::setGradientOneColorMode, py::arg("v"))
		.def("shadeTintValue", &PyDbHatch::shadeTintValue)
		.def("setShadeTintValue", &PyDbHatch::setShadeTintValue, py::arg("v"))
		.def("gradientShift", &PyDbHatch::gradientShift)
		.def("setGradientShift", &PyDbHatch::setGradientShift, py::arg("v"))
		// ---- 边界环构建：点列环 / 实体关联环 / 删环 ----
		.def("appendLoopPoints", [](PyDbHatch& self, int loopType, const AcGePoint2dArray& pts, const AcGeDoubleArray& bulges) {
			return PyWarnIfError(self.impObj()->appendLoop(loopType, pts, bulges), L"appendLoopPoints");
		}, py::arg("loop_type"), py::arg("points"), py::arg("bulges") = AcGeDoubleArray(), "按 Point2d 点列+凸度追加边界环")
		.def("appendLoopEntities", [](PyDbHatch& self, int loopType, const AcDbObjectIdArray& ids) {
			return PyWarnIfError(self.impObj()->appendLoop(loopType, ids), L"appendLoopEntities");
		}, py::arg("loop_type"), py::arg("entity_ids"), "按实体 id 数组追加关联边界环")
		.def("removeLoopAt", [](PyDbHatch& self, int idx) {
			return PyWarnIfError(self.impObj()->removeLoopAt(idx), L"removeLoopAt");
		}, py::arg("index"), "删除指定索引的边界环")
		// ---- 背景色（AcCmColor 未单独封装，按 RGB 安全出入参包装）----
		.def("backgroundColor", [](const PyDbHatch& self) -> std::tuple<int, int, int> {
			AcCmColor c = self.impObj()->backgroundColor();
			return { (int)c.red(), (int)c.green(), (int)c.blue() };
		}, "返回背景色 (r,g,b) 0-255")
		.def("setBackgroundColor", [](PyDbHatch& self, int r, int g, int b) -> bool {
			AcCmColor c;
			c.setRGB((Adesk::UInt8)r, (Adesk::UInt8)g, (Adesk::UInt8)b);
			return PyWarnIfError(self.impObj()->setBackgroundColor(c), L"AcDbHatch::setBackgroundColor");
		}, py::arg("r"), py::arg("g"), py::arg("b"), "设置背景色 0-255")
		// ---- 关联边界对象 ID（按工程 oldId(uint64) 约定透出）----
		.def("getAssocObjIds", [](const PyDbHatch& self) -> std::vector<uint64_t> {
			AcDbObjectIdArray ids;
			if (self.impObj()->getAssocObjIds(ids) != Acad::eOk) return {};
			std::vector<uint64_t> out;
			out.reserve(ids.length());
			for (int i = 0; i < ids.length(); ++i) out.push_back((uint64_t)ids[i].asOldId());
			return out;
		}, "所有关联边界对象 ID 列表")
		.def("getAssocObjIdsAt", [](const PyDbHatch& self, int loopIndex) -> std::vector<uint64_t> {
			AcDbObjectIdArray ids;
			if (self.impObj()->getAssocObjIdsAt(loopIndex, ids) != Acad::eOk) return {};
			std::vector<uint64_t> out;
			out.reserve(ids.length());
			for (int i = 0; i < ids.length(); ++i) out.push_back((uint64_t)ids[i].asOldId());
			return out;
		}, py::arg("loop_index"), "指定环的关联边界对象 ID 列表")
		.def("removeAssocObjIds", [](PyDbHatch& self) -> bool {
			return PyWarnIfError(self.impObj()->removeAssocObjIds(), L"AcDbHatch::removeAssocObjIds");
		}, "解除所有边界对象关联")
		// ---- 环读取/插入（仅绑定点列与实体 id 重载；AcGeVoidPointerArray 裸边重载跳过）----
		.def("getLoopAt", [](const PyDbHatch& self, int loopIndex) -> std::tuple<int, AcGePoint2dArray, AcGeDoubleArray> {
			Adesk::Int32 loopType = 0;
			AcGePoint2dArray verts;
			AcGeDoubleArray bulges;
			if (self.impObj()->getLoopAt(loopIndex, loopType, verts, bulges) != Acad::eOk)
				return { 0, AcGePoint2dArray(), AcGeDoubleArray() };
			return { (int)loopType, verts, bulges };
		}, py::arg("loop_index"), "读取环: 返回 (loopType, 顶点列, 凸度列)")
		.def("insertLoopAtPoints", [](PyDbHatch& self, int loopIndex, int loopType, const AcGePoint2dArray& verts, const AcGeDoubleArray& bulges) -> bool {
			return PyWarnIfError(self.impObj()->insertLoopAt(loopIndex, (Adesk::Int32)loopType, verts, bulges), L"AcDbHatch::insertLoopAt");
		}, py::arg("loop_index"), py::arg("loop_type"), py::arg("vertices"), py::arg("bulges"), "在指定索引插入点列环")
		.def("insertLoopAtEntities", [](PyDbHatch& self, int loopIndex, int loopType, const AcDbObjectIdArray& ids) -> bool {
			return PyWarnIfError(self.impObj()->insertLoopAt(loopIndex, (Adesk::Int32)loopType, ids), L"AcDbHatch::insertLoopAt");
		}, py::arg("loop_index"), py::arg("loop_type"), py::arg("entity_ids"), "在指定索引插入实体关联环")
		// ---- 渐变 ----
		.def("setGradient", [](PyDbHatch& self, int gradType, const AcString& gradName) -> bool {
			return PyWarnIfError(self.impObj()->setGradient((AcDbHatch::GradientPatternType)gradType, gradName.kwszPtr()), L"AcDbHatch::setGradient");
		}, py::arg("grad_type"), py::arg("grad_name"), "设置渐变填充图案")
		.def("setGradientColors", [](PyDbHatch& self, const std::vector<std::tuple<int, int, int>>& colors, const std::vector<float>& values) -> bool {
			std::vector<AcCmColor> cc(colors.size());
			for (size_t i = 0; i < colors.size(); ++i) {
				auto [r, g, b] = colors[i];
				cc[i].setRGB((Adesk::UInt8)r, (Adesk::UInt8)g, (Adesk::UInt8)b);
			}
			std::vector<float> vals(colors.size(), 0.0f);
			for (size_t i = 0; i < values.size() && i < colors.size(); ++i) vals[i] = values[i];
			return PyWarnIfError(self.impObj()->setGradientColors((unsigned int)colors.size(), cc.data(), vals.data()), L"AcDbHatch::setGradientColors");
		}, py::arg("colors"), py::arg("values"), "设置渐变色 [(r,g,b),...] 与归一化位置 [0..1]")
		.def("evaluateGradientColorAt", [](PyDbHatch& self, float value) -> std::tuple<int, int, int> {
			AcCmColor c;
			if (self.impObj()->evaluateGradientColorAt(value, c) != Acad::eOk) return { 0, 0, 0 };
			return { (int)c.red(), (int)c.green(), (int)c.blue() };
		}, py::arg("value"), "评估渐变在 value[0..1] 处的颜色 (r,g,b)")
		.def("setShadeTintValueAndColor2", [](PyDbHatch& self, float value) -> bool {
			return PyWarnIfError(self.impObj()->setShadeTintValueAndColor2(value), L"AcDbHatch::setShadeTintValueAndColor2");
		}, py::arg("value"), "单色模式下设置亮度并据此计算第二渐变颜色")
		.def("getGradientColors", [](PyDbHatch& self) -> std::tuple<std::vector<std::tuple<int, int, int>>, std::vector<float>> {
			unsigned int count = 0;
			AcCmColor* colors = nullptr;
			float* values = nullptr;
			if (self.impObj()->getGradientColors(count, colors, values) != Acad::eOk) return { {}, {} };
			std::vector<std::tuple<int, int, int>> cc;
			std::vector<float> vals;
			cc.reserve(count);
			vals.reserve(count);
			for (unsigned int i = 0; i < count; ++i)
				cc.push_back({ (int)colors[i].red(), (int)colors[i].green(), (int)colors[i].blue() });
			for (unsigned int i = 0; i < count; ++i)
				vals.push_back(values[i]);
			return { cc, vals };
		}, "读取渐变色 [(r,g,b),...] 与位置 [0..1]")
		PyWrapper_BIND(DbHatch)
		.def("__repr__", &PyDbHatch::toString)
		;
}

