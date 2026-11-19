#include "stdafx.h"
#include "PyErrorStatusException.h"
#include "PyDbColor.h"
// AcCmColor / AcCmEntityColor 定义在 dbcolor.h（已被 arxHeaders.h 拉入）

PyWrapper_Cpp(DbColor);

void bind_PyDbColor(pym& m) {
	py::class_<PyDbColor, PyDbObject>(m, "PyDbColor")
		.def(py::init<AcDbColor*>(), py::arg("p"))
		PyWrapper_BIND(DbColor)
		// ---- entityColor() -> (colorMethod, r, g, b) ----
		// AcCmEntityColor 未单独包装，提取基本数值返回。
		// colorMethod: 0xC0=ByLayer 0xC1=ByBlock 0xC2=ByColor 0xC3=ByACI ...（见 dbcolor.h）
		.def("entityColor", [](const PyDbColor& s) -> std::tuple<int, int, int, int> {
			if (!s.impObj()) return { 0, 0, 0, 0 };
			const AcCmEntityColor& ec = s.impObj()->entityColor();
			return { static_cast<int>(ec.colorMethod()),
				static_cast<int>(ec.red()),
				static_cast<int>(ec.green()),
				static_cast<int>(ec.blue()) };
		}, "返回 (colorMethod, r, g, b)")
		// ---- getColor() -> (r, g, b) ----
		// SDK: void getColor(AcCmColor& color) const;
		.def("getColorRGB", [](const PyDbColor& s) -> std::tuple<int, int, int> {
			if (!s.impObj()) return { 0, 0, 0 };
			AcCmColor c;
			s.impObj()->getColor(c);
			return { static_cast<int>(c.red()), static_cast<int>(c.green()), static_cast<int>(c.blue()) };
		}, "读取颜色 RGB (0-255)")
		// ---- setColorByRgb(r, g, b) ----
		// SDK: void setColor(const AcCmColor& color); 无返回值
		.def("setColorByRgb", [](PyDbColor& s, int r, int g, int b) {
			if (!s.impObj()) return false;
			auto clamp = [](int v) { return static_cast<Adesk::UInt8>(v < 0 ? 0 : (v > 255 ? 255 : v)); };
			AcCmColor c;
			c.setRGB(clamp(r), clamp(g), clamp(b));
			s.impObj()->setColor(c);
			return true;
		}, py::arg("r"), py::arg("g"), py::arg("b"), "按 RGB 设置颜色（分量夹紧 0-255）")
		;
}
