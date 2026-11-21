#include "stdafx.h"
#include "PyGiImage.h"

PyGiImageBGRA32::PyGiImageBGRA32(const py::object& pixels) {
	auto arr = py::array_t<Adesk::UInt8, py::array::c_style | py::array::forcecast>::ensure(pixels);
	if (!arr || arr.ndim() != 3 || arr.shape(2) != 4)
		throw py::value_error("位图需要 numpy uint8 数组，形状 (height, width, 4)（BGRA）");

	const size_t h = static_cast<size_t>(arr.shape(0));
	const size_t w = static_cast<size_t>(arr.shape(1));
	if (h == 0 || w == 0)
		throw py::value_error("位图宽高不能为 0");

	m_pixels.resize(h * w);
	const auto raw = arr.unchecked<3>();
	for (size_t y = 0; y < h; ++y) {
		for (size_t x = 0; x < w; ++x) {
			AcGiPixelBGRA32 px;
			px.setBGRA(static_cast<Adesk::UInt8>(raw(y, x, 0)),
				static_cast<Adesk::UInt8>(raw(y, x, 1)),
				static_cast<Adesk::UInt8>(raw(y, x, 2)),
				static_cast<Adesk::UInt8>(raw(y, x, 3)));
			m_pixels[y * w + x] = px;
		}
	}
	m_img.setImage(static_cast<Adesk::UInt32>(w), static_cast<Adesk::UInt32>(h), m_pixels.data());
}

unsigned int PyGiImageBGRA32::width() const {
	return static_cast<unsigned int>(m_img.width());
}

unsigned int PyGiImageBGRA32::height() const {
	return static_cast<unsigned int>(m_img.height());
}

py::array PyGiImageBGRA32::data() const {
	const size_t h = static_cast<size_t>(m_img.height());
	const size_t w = static_cast<size_t>(m_img.width());
	py::array_t<Adesk::UInt8> out({ static_cast<py::ssize_t>(h), static_cast<py::ssize_t>(w), py::ssize_t(4) });
	auto dst = out.mutable_unchecked<3>();
	for (size_t y = 0; y < h; ++y) {
		for (size_t x = 0; x < w; ++x) {
			const AcGiPixelBGRA32& px = m_pixels[y * w + x];
			dst(static_cast<py::ssize_t>(y), static_cast<py::ssize_t>(x), 0) = px.blue();
			dst(static_cast<py::ssize_t>(y), static_cast<py::ssize_t>(x), 1) = px.green();
			dst(static_cast<py::ssize_t>(y), static_cast<py::ssize_t>(x), 2) = px.red();
			dst(static_cast<py::ssize_t>(y), static_cast<py::ssize_t>(x), 3) = px.alpha();
		}
	}
	return out;
}

void bind_PyGiImage(pym& m) {
	py::class_<AcGiPixelBGRA32>(m, "AcGiPixelBGRA32", "BGRA32 像素")
		.def(py::init<>())
		.def(py::init([](int blue, int green, int red, int alpha) {
			AcGiPixelBGRA32 px;
			px.setBGRA(static_cast<Adesk::UInt8>(blue), static_cast<Adesk::UInt8>(green),
				static_cast<Adesk::UInt8>(red), static_cast<Adesk::UInt8>(alpha));
			return px;
		}), py::arg("blue"), py::arg("green"), py::arg("red"), py::arg("alpha") = 255)
		.def("getBGRA", [](const AcGiPixelBGRA32& p) { return static_cast<unsigned int>(p.getBGRA()); })
		.def("getRGBA", [](const AcGiPixelBGRA32& p) { return static_cast<unsigned int>(p.getRGBA()); })
		.def("setBGRA", [](AcGiPixelBGRA32& p, unsigned int v) { p.setBGRA(static_cast<Adesk::UInt32>(v)); }, py::arg("bgra"))
		.def("setRGBA", [](AcGiPixelBGRA32& p, unsigned int v) { p.setRGBA(static_cast<Adesk::UInt32>(v)); }, py::arg("rgba"))
		.def("blue", [](const AcGiPixelBGRA32& p) { return static_cast<int>(p.blue()); })
		.def("green", [](const AcGiPixelBGRA32& p) { return static_cast<int>(p.green()); })
		.def("red", [](const AcGiPixelBGRA32& p) { return static_cast<int>(p.red()); })
		.def("alpha", [](const AcGiPixelBGRA32& p) { return static_cast<int>(p.alpha()); })
		.def("setBlue", [](AcGiPixelBGRA32& p, int v) { p.setBlue(static_cast<Adesk::UInt8>(v)); }, py::arg("value"))
		.def("setGreen", [](AcGiPixelBGRA32& p, int v) { p.setGreen(static_cast<Adesk::UInt8>(v)); }, py::arg("value"))
		.def("setRed", [](AcGiPixelBGRA32& p, int v) { p.setRed(static_cast<Adesk::UInt8>(v)); }, py::arg("value"))
		.def("setAlpha", [](AcGiPixelBGRA32& p, int v) { p.setAlpha(static_cast<Adesk::UInt8>(v)); }, py::arg("value"));

	py::class_<PyGiImageBGRA32>(m, "AcGiImageBGRA32", "BGRA32 位图（numpy uint8 (H, W, 4) 构造）")
		.def(py::init<const py::object&>(), py::arg("pixels"))
		.def("width", &PyGiImageBGRA32::width)
		.def("height", &PyGiImageBGRA32::height)
		.def("data", &PyGiImageBGRA32::data);
}
