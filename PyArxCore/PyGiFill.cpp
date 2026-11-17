#include "stdafx.h"
#include "PyGiFill.h"

PyGiFill::~PyGiFill() {
	if (m_fill) {
		delete m_fill;
		m_fill = nullptr;
	}
}

double PyGiFill::deviation() const { return m_fill->deviation(); }
void PyGiFill::setDeviation(double dev) { m_fill->setDeviation(dev); }

// ===========================================================================
// AcGiHatchPattern
// ===========================================================================
PyGiHatchPattern::PyGiHatchPattern(const py::object& defs) : PyGiFill(nullptr) {
	AcArray<AcGiHatchPatternDefinition> lines;
	const py::sequence seq = py::cast<py::sequence>(defs);
	const size_t n = static_cast<size_t>(py::len(seq));
	lines.setLogicalLength(static_cast<int>(n));
	for (size_t i = 0; i < n; ++i) {
		const py::sequence item = py::cast<py::sequence>(seq[static_cast<py::ssize_t>(i)]);
		const size_t m = static_cast<size_t>(py::len(item));
		if (m < 5)
			throw py::value_error("pattern line 至少需要 (angle, baseX, baseY, offsetX, offsetY[, dash...])");
		const double angle = py::cast<double>(item[0]);
		const double baseX = py::cast<double>(item[1]);
		const double baseY = py::cast<double>(item[2]);
		const double offX = py::cast<double>(item[3]);
		const double offY = py::cast<double>(item[4]);
		// dash 段两种写法都收：平铺 (…, 5, -3)、或末尾给一个 [5, -3] 列表。
		double dashValues[64];
		int dashCount = 0;
		py::object last = py::reinterpret_borrow<py::object>(item[static_cast<py::ssize_t>(m - 1)]);
		const bool nested = (m > 5) && PySequence_Check(last.ptr()) && !PyUnicode_Check(last.ptr());
		if (nested) {
			const py::sequence ds = py::cast<py::sequence>(last);
			const size_t dn = static_cast<size_t>(py::len(ds));
			for (size_t k = 0; k < dn && dashCount < 64; ++k)
				dashValues[dashCount++] = py::cast<double>(ds[static_cast<py::ssize_t>(k)]);
		} else {
			for (size_t k = 5; k < m && dashCount < 64; ++k)
				dashValues[dashCount++] = py::cast<double>(item[static_cast<py::ssize_t>(k)]);
		}
		AcGeDoubleArray dashes;
		dashes.setLogicalLength(dashCount);
		for (int k = 0; k < dashCount; ++k)
			dashes[k] = dashValues[k];
		lines[i] = AcGiHatchPatternDefinition(angle, baseX, baseY, offX, offY, dashes);
	}
	m_fill = new AcGiHatchPattern(lines);
}

PyGiHatchPattern::~PyGiHatchPattern() = default;

py::list PyGiHatchPattern::patternLines() const {
	py::list out;
	const AcArray<AcGiHatchPatternDefinition>& lines = impHatch()->patternLines();
	for (const auto& def : lines) {
		py::list dashes;
		const AcGeDoubleArray& dl = def.dashList();
		for (int i = 0; i < dl.length(); ++i)
			dashes.append(dl[i]);
		out.append(py::make_tuple(def.angle(), def.baseX(), def.baseY(), def.offsetX(), def.offsetY(), dashes));
	}
	return out;
}

// ===========================================================================
// AcGiGradientFill
// ===========================================================================
PyGiGradientFill::PyGiGradientFill(double gradientAngle, double gradientShift, bool adjustAspect,
	const py::object& colors, int gradientType) : PyGiFill(nullptr) {
	AcArray<AcCmColor> clrs;
	const py::sequence seq = py::cast<py::sequence>(colors);
	const size_t n = static_cast<size_t>(py::len(seq));
	clrs.setLogicalLength(static_cast<int>(n));
	for (size_t i = 0; i < n; ++i) {
		const py::sequence rgb = py::cast<py::sequence>(seq[static_cast<py::ssize_t>(i)]);
		if (static_cast<size_t>(py::len(rgb)) < 3)
			throw py::value_error("渐变色需要 (r, g, b)");
		AcCmColor c;
		c.setRGB(static_cast<Adesk::UInt8>(py::cast<int>(rgb[0])),
			static_cast<Adesk::UInt8>(py::cast<int>(rgb[1])),
			static_cast<Adesk::UInt8>(py::cast<int>(rgb[2])));
		clrs[i] = c;
	}
	m_fill = new AcGiGradientFill(gradientAngle, gradientShift, adjustAspect, clrs,
		static_cast<AcGiGradientFill::GradientType>(gradientType));
}

PyGiGradientFill::~PyGiGradientFill() = default;

double PyGiGradientFill::gradientAngle() const { return impGradient()->gradientAngle(); }
double PyGiGradientFill::gradientShift() const { return impGradient()->gradientShift(); }
bool PyGiGradientFill::isAdjustAspect() const { return impGradient()->isAdjustAspect(); }
int PyGiGradientFill::gradientType() const { return static_cast<int>(impGradient()->gradientType()); }

py::list PyGiGradientFill::gradientColors() const {
	py::list out;
	const AcArray<AcCmColor>& clrs = impGradient()->gradientColors();
	for (const auto& c : clrs)
		out.append(py::make_tuple(static_cast<int>(c.red()), static_cast<int>(c.green()), static_cast<int>(c.blue())));
	return out;
}

void bind_PyGiFill(pym& m) {
	py::enum_<AcGiGradientFill::GradientType>(m, "AcGiGradientType")
		.value("kLinear", AcGiGradientFill::kLinear)
		.value("kCylinder", AcGiGradientFill::kCylinder)
		.value("kInvCylinder", AcGiGradientFill::kInvCylinder)
		.value("kSpherical", AcGiGradientFill::kSpherical)
		.value("kHemispherical", AcGiGradientFill::kHemispherical)
		.value("kCurved", AcGiGradientFill::kCurved)
		.value("kInvSpherical", AcGiGradientFill::kInvSpherical)
		.value("kInvHemispherical", AcGiGradientFill::kInvHemispherical)
		.value("kInvCurved", AcGiGradientFill::kInvCurved);

	py::class_<PyGiFill>(m, "AcGiFill", "AcGi 填充基类（三角化偏差）")
		.def(py::init<>())
		.def("deviation", &PyGiFill::deviation)
		.def("setDeviation", &PyGiFill::setDeviation, py::arg("deviation"))
		.def("isHatchPattern", &PyGiFill::isHatchPattern)
		.def("isGradientFill", &PyGiFill::isGradientFill);

	py::class_<PyGiHatchPattern, PyGiFill>(m, "AcGiHatchPattern", "图案填充")
		.def(py::init([](py::object lines) { return std::make_unique<PyGiHatchPattern>(lines); }),
			py::arg("pattern_lines"),
			"pattern_lines: [(angle, base_x, base_y, offset_x, offset_y, [dash...]) ...]")
		.def("patternLines", &PyGiHatchPattern::patternLines);

	py::class_<PyGiGradientFill, PyGiFill>(m, "AcGiGradientFill", "渐变填充")
		.def(py::init([](double angle, double shift, bool adjust, py::object colors, int type) {
			return std::make_unique<PyGiGradientFill>(angle, shift, adjust, colors, type);
		}), py::arg("gradient_angle"), py::arg("gradient_shift"), py::arg("adjust_aspect"),
			py::arg("colors"), py::arg("gradient_type"))
		.def("gradientAngle", &PyGiGradientFill::gradientAngle)
		.def("gradientShift", &PyGiGradientFill::gradientShift)
		.def("isAdjustAspect", &PyGiGradientFill::isAdjustAspect)
		.def("gradientType", &PyGiGradientFill::gradientType)
		.def("gradientColors", &PyGiGradientFill::gradientColors);
}
