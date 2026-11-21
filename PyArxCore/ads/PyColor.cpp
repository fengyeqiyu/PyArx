#include "stdafx.h"
#include "PyColor.h"
#include "PyErrorStatusException.h"
#include <aced.h>

// ACI(1-255) -> RGB：acedGetRGB 返回 COLORREF(0x00bbggrr)；0/256/257 无实体色，返回 false
static bool aciToRGB(int aci, int& r, int& g, int& b) {
	if (aci < 1 || aci > 255) return false;
	Adesk::ColorRef cr = ::acedGetRGB(aci);
	r = static_cast<int>(cr & 0xFF);
	g = static_cast<int>((cr >> 8) & 0xFF);
	b = static_cast<int>((cr >> 16) & 0xFF);
	return true;
}

// RGB 分量越界一律夹紧到 0-255，避免 static_cast<UInt8> 静默回绕（300 -> 44）
static Adesk::UInt8 clamp255(int v) {
	return static_cast<Adesk::UInt8>(v < 0 ? 0 : (v > 255 ? 255 : v));
}

// ---- 工厂 ----
PyColor PyColor::fromRGB(int r, int g, int b) {
	PyColor c;
	c.setColorByRgb(r, g, b);   // 已夹紧，setColorByRgb 内部不会失败
	return c;
}

PyColor PyColor::fromACI(int index) {
	PyColor c;
	c.setColorIndex(index);
	return c;
}

PyColor PyColor::byLayer() { PyColor c; c.m_color.setByLayer(); return c; }
PyColor PyColor::byBlock() { PyColor c; c.m_color.setByBlock(); return c; }
PyColor PyColor::none()    { PyColor c; c.m_color.setNone();    return c; }

// ---- RGB ----
// ACI 色（非真彩色）时 AcCmColor 的 RGB 分量恒为 0，这里按 ACI 索引查真实 RGB，
// 保证 fromACI(...).red/green/blue 与 fromRGB(...) 语义一致。
int  PyColor::red() const {
	int r = 0, g = 0, b = 0;
	if (!m_color.isByColor() && aciToRGB(static_cast<int>(m_color.colorIndex()), r, g, b)) return r;
	return m_color.red();
}
int  PyColor::green() const {
	int r = 0, g = 0, b = 0;
	if (!m_color.isByColor() && aciToRGB(static_cast<int>(m_color.colorIndex()), r, g, b)) return g;
	return m_color.green();
}
int  PyColor::blue() const {
	int r = 0, g = 0, b = 0;
	if (!m_color.isByColor() && aciToRGB(static_cast<int>(m_color.colorIndex()), r, g, b)) return b;
	return m_color.blue();
}
bool PyColor::setColorByRgb(int r, int g, int b) {
	return PyWarnIfError(m_color.setRGB(clamp255(r), clamp255(g), clamp255(b)),
		L"AcCmColor::setRGB");
}

// ---- ACI ----
int  PyColor::colorIndex() const      { return m_color.colorIndex(); }
bool PyColor::setColorIndex(int idx) {
	// 0-255 是 ACI 实体色（0=ByBlock）；256/257 是 SDK 的方法码，必须走专用 setter，
	// 否则 setColorIndex(256) 会被当成 ByBlock。其余越界按 Python 语义对 256 取模归一。
	if (idx == 256) { m_color.setByLayer(); return true; }
	if (idx == 257) { m_color.setNone();    return true; }
	int aci = ((idx % 256) + 256) % 256;
	return PyWarnIfError(m_color.setColorIndex(static_cast<Adesk::UInt16>(aci)),
		L"AcCmColor::setColorIndex");
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
	if (m_color.getDescription(desc) == Acad::eOk && !desc.isEmpty()) {
		return desc;
	}
	// SDK 描述为空时兜底，避免 __repr__/__str__ 返回空串
	AcString fb;
	if (m_color.isByLayer())      fb = L"ByLayer";
	else if (m_color.isByBlock()) fb = L"ByBlock";
	else if (m_color.isNone())    fb = L"None";
	else if (m_color.isByColor()) fb.format(L"RGB(%d,%d,%d)", red(), green(), blue());
	else                          fb.format(L"ACI %d", static_cast<int>(m_color.colorIndex()));
	return fb;
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
			"按 RGB 构造真彩色（分量越界夹紧到 0-255）")
		.def_static("fromACI", &PyColor::fromACI,
			py::arg("index"), "按 ACI 颜色号构造（0=ByBlock, 1-255, 256=ByLayer, 257=None）")
		.def_static("byLayer", &PyColor::byLayer, "构造 ByLayer 颜色")
		.def_static("byBlock", &PyColor::byBlock, "构造 ByBlock 颜色")
		.def_static("none", &PyColor::none, "构造 None（无颜色）")

		// RGB 读写
		.def_property_readonly("red", &PyColor::red, "红分量 (0-255)；ACI 色按 ACI 调色板解析真实 RGB")
		.def_property_readonly("green", &PyColor::green, "绿分量 (0-255)；ACI 色按 ACI 调色板解析真实 RGB")
		.def_property_readonly("blue", &PyColor::blue, "蓝分量 (0-255)；ACI 色按 ACI 调色板解析真实 RGB")
		.def("setColorByRgb", &PyColor::setColorByRgb,
			py::arg("r"), py::arg("g"), py::arg("b"),
			"按 RGB 设置真彩色（分量越界夹紧到 0-255），返回 bool")

		// ACI
		.def_property_readonly("colorIndex", &PyColor::colorIndex, "ACI 颜色号 (0=ByBlock, 1-255, 256=ByLayer, 257=None)")
		.def("setColorIndex", &PyColor::setColorIndex, py::arg("index"), "设置 ACI 颜色号（0=ByBlock, 1-255 实体色, 256=ByLayer, 257=None, 其余对 256 取模），返回 bool")

		// 判定
		.def("isByLayer", &PyColor::isByLayer)
		.def("isByBlock", &PyColor::isByBlock)
		.def("isByColor", &PyColor::isByColor, "是否真彩色（由 setColorByRgb/fromRGB 设置）")
		.def("isNone", &PyColor::isNone)

		// 命名
		.def_property_readonly("colorName", &PyColor::colorName, "颜色名（命名色簿色，可能为空）")
		.def_property_readonly("bookName", &PyColor::bookName, "色簿名（可能为空）")

		// 字符串
		.def("describe", &PyColor::describe, "可读颜色描述")
		.def("__repr__", &PyColor::describe)
		.def("__str__", &PyColor::describe)
		.def("__eq__", [](const PyColor& a, const PyColor& b) { return a == b; }, py::is_operator())
		.def("__ne__", [](const PyColor& a, const PyColor& b) { return a != b; }, py::is_operator());
}
