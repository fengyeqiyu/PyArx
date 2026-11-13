#include "stdafx.h"
#include "PyErrorStatusException.h"
#include "PyDbEvalVariant.h"

PyDbEvalVariant::PyDbEvalVariant() : m_val() {}
PyDbEvalVariant::PyDbEvalVariant(double v) : m_val(v) {}
PyDbEvalVariant::PyDbEvalVariant(short v) : m_val(v) {}
PyDbEvalVariant::PyDbEvalVariant(Adesk::Int32 v) : m_val(v) {}
PyDbEvalVariant::PyDbEvalVariant(const AcString& v) : m_val(v.kwszPtr()) {}
PyDbEvalVariant::PyDbEvalVariant(uint64_t id) {
	AcDbObjectId oid; oid.setFromOldId(static_cast<Adesk::UIntPtr>(id));
	m_val = oid;
}
PyDbEvalVariant::PyDbEvalVariant(const AcGePoint2d& pt) : m_val(pt) {}
PyDbEvalVariant::PyDbEvalVariant(const AcGePoint3d& pt) : m_val(pt) {}
PyDbEvalVariant::PyDbEvalVariant(const AcDbEvalVariant& other) : m_val(other) {}

AcDbEvalVariant& PyDbEvalVariant::impObj() { return m_val; }
const AcDbEvalVariant& PyDbEvalVariant::impObj() const { return m_val; }

void bind_PyDbEvalVariant(pym& m) {
	py::class_<PyDbEvalVariant>(m, "PyDbEvalVariant")
		.def(py::init<>(), "默认构造（kNone）。")
		.def(py::init<double>(), py::arg("value"), "由 double 构造（kDouble）。")
		.def(py::init<short>(), py::arg("value"), "由 short 构造（kShort）。")
		.def(py::init<Adesk::Int32>(), py::arg("value"), "由 long/int32 构造（kLong）。")
		.def(py::init([](const AcString& s) { return PyDbEvalVariant(s); }), py::arg("value"), "由字符串构造（kString）。")
		.def(py::init([](uint64_t id) { return PyDbEvalVariant(id); }), py::arg("objectId"), "由 ObjectId（uint64）构造（kOldId）。")
		.def(py::init<const AcGePoint2d&>(), py::arg("point"), "由 2D 点构造（kPoint2d）。")
		.def(py::init<const AcGePoint3d&>(), py::arg("point"), "由 3D 点构造（kPoint3d）。")

		// ---- getType() -> int (AcDb::DwgDataType) ----
		.def("getType", [](const PyDbEvalVariant& v) {
			return static_cast<int>(v.impObj().getType());
		}, "返回当前值的类型（AcDb::DwgDataType 整数）。")
		// ---- clear() ----
		.def("clear", [](PyDbEvalVariant& v) {
			v.impObj().clear();
		}, "清空内容并释放内存，类型置为 kNone。")

		// ---- setValue(groupCode, value) -> bool ----
		// 以下重载在 SDK2024 lib 中未导出符号，暂不封装：
		// setValue(double/short/Int32/ObjectId/Point2d/Point3d)
		.def("setValueString", [](PyDbEvalVariant& v, int gc, const AcString& s) {
			return PyWarnIfError(v.impObj().setValue(static_cast<AcDb::DxfCode>(gc), s.kwszPtr()), L"AcDbEvalVariant::setValue(string)");
		}, py::arg("groupCode"), py::arg("value"))

		// ---- getValue -> 各类型（失败返回安全默认值） ----
		.def("getDouble", [](const PyDbEvalVariant& v) -> double {
			double x = 0.0;
			if (!PyWarnIfError(v.impObj().getValue(x), L"AcDbEvalVariant::getValue(double)")) return 0.0;
			return x;
		})
		.def("getShort", [](const PyDbEvalVariant& v) -> short {
			short x = 0;
			if (!PyWarnIfError(v.impObj().getValue(x), L"AcDbEvalVariant::getValue(short)")) return 0;
			return x;
		})
		.def("getLong", [](const PyDbEvalVariant& v) -> Adesk::Int32 {
			Adesk::Int32 x = 0;
			if (!PyWarnIfError(v.impObj().getValue(x), L"AcDbEvalVariant::getValue(Int32)")) return 0;
			return x;
		})
		.def("getString", [](const PyDbEvalVariant& v) -> AcString {
			AcString s;
			if (!PyWarnIfError(v.impObj().getValue(s), L"AcDbEvalVariant::getValue(string)")) return AcString();
			return s;
		})
		// getObjectId / getPoint2d: SDK2024 未导出对应符号，暂不封装
		.def("getPoint3d", [](const PyDbEvalVariant& v) -> AcGePoint3d {
			AcGePoint3d pt;
			if (!PyWarnIfError(v.impObj().getValue(pt), L"AcDbEvalVariant::getValue(Point3d)")) return AcGePoint3d();
			return pt;
		})
		;
	// 跳过：
	//  - toAcRxValue / fromAcRxValue：依赖 AcRxValue / AcRxValueType 的 Python 包装，P2 依赖。
	//  - copyFrom(const AcRxObject*)：AcRxObject 赋值语义不适合直接透出，暂不绑。
	//  - 比较运算符：仅数值/字符串类型可用，意义有限，暂不绑。
	//  - acutDelString / acutRelRb：resbuf 内部释放工具，非本类方法，不绑。
}
