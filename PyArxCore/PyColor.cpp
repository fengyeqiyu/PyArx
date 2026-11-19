#include "stdafx.h"
#include "PyColor.h"

// ---- 工厂 ----
PyColor PyColor::fromRGB(int r, int g, int b) {
	PyColor c;
	c.m_color.setRGB(static_cast<Adesk::UInt8>(r),
		static_cast<Adesk::UInt8>(g),
		static_cast<Adesk::UInt8>(b));
	return c;
}

PyColor PyColor::fromACI(int index) {
	PyColor c;
	c.setColorIndex(index);
	return c;
}

PyColor PyColor::byLayer() { PyColor c; c.m_color.setByLayer(); return c; }
PyColor PyColor::byBlock() { PyColor c; c.m_color.setByBlock(); return c; }

// ---- RGB ----
int  PyColor::red() const   { return m_color.red(); }
int  PyColor::green() const { return m_color.green(); }
int  PyColor::blue() const  { return m_color.blue(); }
void PyColor::setRGB(int r, int g, int b) {
	m_color.setRGB(static_cast<Adesk::UInt8>(r),
		static_cast<Adesk::UInt8>(g),
		static_cast<Adesk::UInt8>(b));
}

// ---- ACI ----
int  PyColor::colorIndex() const      { return m_color.colorIndex(); }
void PyColor::setColorIndex(int idx) {
	// 0-255 是 ACI 实体色（0=ByBlock）；256/257 是 SDK 的方法码，必须走专用 setter，
	// 否则 setColorIndex(256) 会被当成 ByBlock。其余越界按 Python 语义对 256 取模归一。
	if (idx == 256) { m_color.setByLayer(); return; }
	if (idx == 257) { m_color.setNone();    return; }
	int aci = ((idx % 256) + 256) % 256;
	m_color.setColorIndex(static_cast<Adesk::UInt16>(aci));
}

// ---- 判定 ----
bool PyColor::isByLayer() const { return m_color.isByLayer(); }
bool PyColor::isByBlock() const { return m_color.isByBlock(); }
bool PyColor::isByColor() const { return m_color.isByColor(); }
bool PyColor::isNone() const    { return m_color.isNone(); }

// ---- 命名 ----
AcString PyColor::colorName() const {
	const ACHAR* n = m_color.colorName();
	return n ? AcString(n) : AcString();
}
AcString PyColor::bookName() const {
	const ACHAR* n = m_color.bookName();
	return n ? AcString(n) : AcString();
}

AcString PyColor::describe() const {
	AcString desc;
	if (m_color.getDescription(desc) == Acad::eOk) {
		return desc;
	}
	return AcString();
}

// ---- pybind 绑定 ----
void bind_PyColor(pym& m) {
	py::class_<PyColor>(m, "Color",
		"AcCmColor 颜色值类（非数据库对象，可自由构造）")
		.def(py::init<>(), "默认构造（ByLayer）")
		.def(py::init<const PyColor&>(), py::arg("other"), "拷贝构造")

		// 工厂方法
		.def_static("fromRGB", &PyColor::fromRGB,
			py::arg("r"), py::arg("g"), py::arg("b"),
			"按 RGB (0-255) 构造真彩色")
		.def_static("fromACI", &PyColor::fromACI,
			py::arg("index"), "按 ACI 颜色号 (0-255) 构造")
		.def_static("byLayer", &PyColor::byLayer, "构造 ByLayer 颜色")
		.def_static("byBlock", &PyColor::byBlock, "构造 ByBlock 颜色")

		// RGB 读写
		.def_property_readonly("red", &PyColor::red, "红分量 (0-255)")
		.def_property_readonly("green", &PyColor::green, "绿分量 (0-255)")
		.def_property_readonly("blue", &PyColor::blue, "蓝分量 (0-255)")
		.def("setRGB", &PyColor::setRGB,
			py::arg("r"), py::arg("g"), py::arg("b"), "按 0-255 设置 RGB")

		// ACI
		.def_property_readonly("colorIndex", &PyColor::colorIndex, "ACI 颜色号 (0=ByBlock, 1-255)")
		.def("setColorIndex", &PyColor::setColorIndex, py::arg("index"), "设置 ACI 颜色号（0=ByBlock, 1-255 实体色, 256=ByLayer, 257=None, 其余对 256 取模）")

		// 判定
		.def("isByLayer", &PyColor::isByLayer)
		.def("isByBlock", &PyColor::isByBlock)
		.def("isByColor", &PyColor::isByColor, "是否为真彩色/ACI 显式色")
		.def("isNone", &PyColor::isNone)

		// 命名
		.def_property_readonly("colorName", &PyColor::colorName, "颜色名（命名色簿色，可能为空）")
		.def_property_readonly("bookName", &PyColor::bookName, "色簿名（可能为空）")

		// 字符串
		.def("describe", &PyColor::describe, "可读颜色描述")
		.def("__repr__", &PyColor::describe)
		.def("__str__", &PyColor::describe);
}
