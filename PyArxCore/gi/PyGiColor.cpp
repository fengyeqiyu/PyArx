#include "stdafx.h"
#include "PyGiColor.h"
#include "PyGiTypes.h"
#include "PyErrorStatusException.h"
#include "PyEnumArg.h"

// ===========================================================================
// AcCmEntityColor —— 子实体真彩色
// SDK: dbcolor.h。四字节 RGBM：低 24 位 RGB，高字节为 colorMethod。
// ===========================================================================
void bind_PyCmEntityColor(pym& m) {
	py::enum_<AcCmEntityColor::ColorMethod>(m, "AcCmColorMethod")
		.value("kByLayer", AcCmEntityColor::kByLayer)
		.value("kByBlock", AcCmEntityColor::kByBlock)
		.value("kByColor", AcCmEntityColor::kByColor)
		.value("kByACI", AcCmEntityColor::kByACI)
		.value("kByPen", AcCmEntityColor::kByPen)
		.value("kForeground", AcCmEntityColor::kForeground)
		.value("kLayerOff", AcCmEntityColor::kLayerOff)
		.value("kLayerFrozen", AcCmEntityColor::kLayerFrozen)
		.value("kNone", AcCmEntityColor::kNone);

	py::class_<AcCmEntityColor>(m, "AcCmEntityColor", "子实体真彩色（RGB + colorMethod）")
		.def(py::init<>())
		.def(py::init([](int r, int g, int b) {
			return AcCmEntityColor(static_cast<Adesk::UInt8>(r), static_cast<Adesk::UInt8>(g), static_cast<Adesk::UInt8>(b));
		}), py::arg("red"), py::arg("green"), py::arg("blue"))
		// ---- 静态构造 ----
		.def_static("none", &AcCmEntityColor::None)
		.def_static("byBlock", &AcCmEntityColor::ByBlock)
		.def_static("byLayer", &AcCmEntityColor::ByLayer)
		.def_static("foreground", &AcCmEntityColor::Foreground)
		.def_static("white", &AcCmEntityColor::white)
		.def_static("black", &AcCmEntityColor::black)
		// ---- 直接设定方法 ----
		.def("setNone", &AcCmEntityColor::setNone)
		.def("setByBlock", &AcCmEntityColor::setByBlock)
		.def("setByLayer", &AcCmEntityColor::setByLayer)
		.def("setForeground", &AcCmEntityColor::setForeground)
		.def("setLayerOff", &AcCmEntityColor::setLayerOff)
		.def("colorMethod", &AcCmEntityColor::colorMethod)
		// ---- ACI / 层 / 笔号 ----
		.def("setColorIndex", [](AcCmEntityColor& c, int idx) {
			return PyWarnIfError(c.setColorIndex(static_cast<Adesk::Int16>(idx)), L"AcCmEntityColor::setColorIndex");
		}, py::arg("index"))
		.def("colorIndex", [](const AcCmEntityColor& c) { return static_cast<int>(c.colorIndex()); })
		.def("setLayerIndex", [](AcCmEntityColor& c, int idx) {
			return PyWarnIfError(c.setLayerIndex(static_cast<Adesk::Int32>(idx)), L"AcCmEntityColor::setLayerIndex");
		}, py::arg("index"))
		.def("layerIndex", [](const AcCmEntityColor& c) { return static_cast<int>(c.layerIndex()); })
		.def("setPenIndex", [](AcCmEntityColor& c, int idx) {
			return PyWarnIfError(c.setPenIndex(static_cast<Adesk::UInt16>(idx)), L"AcCmEntityColor::setPenIndex");
		}, py::arg("index"))
		.def("penIndex", [](const AcCmEntityColor& c) { return static_cast<int>(c.penIndex()); })
		// ---- RGB ----
		.def("setRGB", [](AcCmEntityColor& c, int r, int g, int b) {
			return PyWarnIfError(c.setRGB(static_cast<Adesk::UInt8>(r), static_cast<Adesk::UInt8>(g), static_cast<Adesk::UInt8>(b)), L"AcCmEntityColor::setRGB");
		}, py::arg("red"), py::arg("green"), py::arg("blue"))
		.def("red", [](const AcCmEntityColor& c) { return static_cast<int>(c.red()); })
		.def("green", [](const AcCmEntityColor& c) { return static_cast<int>(c.green()); })
		.def("blue", [](const AcCmEntityColor& c) { return static_cast<int>(c.blue()); })
		.def("rgb", [](const AcCmEntityColor& c) {
			return py::make_tuple(static_cast<int>(c.red()), static_cast<int>(c.green()), static_cast<int>(c.blue()));
		}, "返回 (r, g, b)")
		.def("setCOLORREF", [](AcCmEntityColor& c, unsigned int cref) {
			return PyWarnIfError(c.setCOLORREF(static_cast<Adesk::ColorRef>(cref)), L"AcCmEntityColor::setCOLORREF");
		}, py::arg("colorref"))
		.def("getCOLORREF", [](const AcCmEntityColor& c) { return static_cast<unsigned int>(c.getCOLORREF()); })
		.def("setRGBM", [](AcCmEntityColor& c, unsigned int rgbm) {
			return PyWarnIfError(c.setRGBM(static_cast<Adesk::UInt32>(rgbm)), L"AcCmEntityColor::setRGBM");
		}, py::arg("rgbm"))
		.def("getRGBM", [](const AcCmEntityColor& c) { return static_cast<unsigned int>(c.getRGBM()); })
		// ---- 方法判定 / 解析 ----
		.def("isByColor", &AcCmEntityColor::isByColor)
		.def("isByLayer", &AcCmEntityColor::isByLayer)
		.def("isByBlock", &AcCmEntityColor::isByBlock)
		.def("isByACI", &AcCmEntityColor::isByACI)
		.def("isByPen", &AcCmEntityColor::isByPen)
		.def("isForeground", &AcCmEntityColor::isForeground)
		.def("isLayerOff", &AcCmEntityColor::isLayerOff)
		.def("isLayerFrozen", &AcCmEntityColor::isLayerFrozen)
		.def("isNone", &AcCmEntityColor::isNone)
		.def("isLayerFrozenOrOff", &AcCmEntityColor::isLayerFrozenOrOff)
		.def("canResolveRGB", &AcCmEntityColor::canResolveRGB)
		.def("makeRGB", [](const AcCmEntityColor& c, unsigned int bg) { return c.makeRGB(static_cast<Adesk::ColorRef>(bg)); }, py::arg("background_rgb") = 0u)
		.def("forceRGB", [](const AcCmEntityColor& c, unsigned int bg) { return c.forceRGB(static_cast<Adesk::ColorRef>(bg)); }, py::arg("background_rgb") = 0u)
		.def("makeTrueColor", &AcCmEntityColor::makeTrueColor)
		.def(py::self == py::self)
		.def(py::self != py::self)
		.def("__repr__", [](const AcCmEntityColor& c) {
			return std::format("AcCmEntityColor(method={}, rgb=({}, {}, {}))",
				static_cast<int>(c.colorMethod()),
				static_cast<int>(c.red()), static_cast<int>(c.green()), static_cast<int>(c.blue()));
		});
}

// ===========================================================================
// AcCmTransparency —— 子实体透明度
// SDK: dbcolor.h。低字节 alpha，高字节 method（0=ByLayer 1=ByBlock 2=ByAlpha）。
// ===========================================================================
void bind_PyCmTransparency(pym& m) {
	py::enum_<AcCmTransparency::transparencyMethod>(m, "AcCmTransparencyMethod")
		.value("kByLayer", AcCmTransparency::kByLayer)
		.value("kByBlock", AcCmTransparency::kByBlock)
		.value("kByAlpha", AcCmTransparency::kByAlpha)
		.value("kErrorValue", AcCmTransparency::kErrorValue);

	py::class_<AcCmTransparency>(m, "AcCmTransparency", "子实体透明度（alpha 0-255 / percent 0-1）")
		.def(py::init<>())
		.def(py::init([](int alpha) { return AcCmTransparency(static_cast<Adesk::UInt8>(alpha)); }), py::arg("alpha"))
		.def(py::init([](double percent) { return AcCmTransparency(percent); }), py::arg("alpha_percent"))
		.def("setAlpha", [](AcCmTransparency& t, int alpha) { t.setAlpha(static_cast<Adesk::UInt8>(alpha)); }, py::arg("alpha"))
		.def("setAlphaPercent", &AcCmTransparency::setAlphaPercent, py::arg("alpha_percent"))
		.def("setMethod", pyarx::enumSetter(&AcCmTransparency::setMethod), py::arg("method"))
		.def("alpha", [](const AcCmTransparency& t) { return static_cast<int>(t.alpha()); })
		.def("alphaPercent", &AcCmTransparency::alphaPercent)
		.def("isByAlpha", &AcCmTransparency::isByAlpha)
		.def("isByBlock", &AcCmTransparency::isByBlock)
		.def("isByLayer", &AcCmTransparency::isByLayer)
		.def("isInvalid", &AcCmTransparency::isInvalid)
		.def("isClear", &AcCmTransparency::isClear)
		.def("isSolid", &AcCmTransparency::isSolid)
		.def("serializeOut", [](const AcCmTransparency& t) { return static_cast<unsigned int>(t.serializeOut()); })
		.def("serializeIn", [](AcCmTransparency& t, unsigned int v) { t.serializeIn(static_cast<Adesk::UInt32>(v)); }, py::arg("value"))
		.def(py::self == py::self)
		.def(py::self != py::self)
		.def("__repr__", [](const AcCmTransparency& t) {
			return std::format("AcCmTransparency(alpha={}, percent={:.4f})",
				static_cast<int>(t.alpha()), t.alphaPercent());
		});
}

// ===========================================================================
// AcGiVariant —— 变体值（AcGiDrawableTraits::setHighlightProperty 用）
// 只绑基础类型（bool/int/double/string）+ 表访问；AcCmColor 分支留空（AcCmColor
// 尚未绑定，Variant 里的颜色值可用 getRGBM 语义从调用方自己换算）。
// ===========================================================================
void bind_PyGiVariant(pym& m) {
	py::enum_<AcGiVariant::VariantType>(m, "AcGiVariantType")
		.value("kUndefined", AcGiVariant::kUndefined)
		.value("kBoolean", AcGiVariant::kBoolean)
		.value("kInt", AcGiVariant::kInt)
		.value("kDouble", AcGiVariant::kDouble)
		.value("kColor", AcGiVariant::kColor)
		.value("kString", AcGiVariant::kString)
		.value("kTable", AcGiVariant::kTable);

	py::class_<AcGiVariant>(m, "AcGiVariant", "AcGi 变体值")
		.def(py::init<>())
		.def(py::init([](bool v) { return AcGiVariant(v); }), py::arg("value"))
		.def(py::init([](int v) { return AcGiVariant(static_cast<Adesk::Int32>(v)); }), py::arg("value"))
		.def(py::init([](double v) { return AcGiVariant(v); }), py::arg("value"))
		.def(py::init([](const AcString& v) { return AcGiVariant(v.constPtr()); }), py::arg("value"))
		.def("type", &AcGiVariant::type)
		.def("setBool", [](AcGiVariant& v, bool x) { v.set(x); }, py::arg("value"))
		.def("setInt", [](AcGiVariant& v, int x) { v.set(static_cast<Adesk::Int32>(x)); }, py::arg("value"))
		.def("setDouble", [](AcGiVariant& v, double x) { v.set(x); }, py::arg("value"))
		.def("setString", [](AcGiVariant& v, const AcString& x) { v.set(x.constPtr()); }, py::arg("value"))
		.def("asBoolean", &AcGiVariant::asBoolean)
		.def("asInt", &AcGiVariant::asInt)
		.def("asDouble", &AcGiVariant::asDouble)
		.def("asString", &AcGiVariant::asString)
		.def("getElemCount", &AcGiVariant::getElemCount)
		.def("getElem", [](const AcGiVariant& v, const AcString& key) {
			AcGiVariant out;
			PyWarnIfError(v.getElem(key.constPtr(), out), L"AcGiVariant::getElem");
			return out;
		}, py::arg("key"))
		.def("setElem", [](AcGiVariant& v, const AcString& key, const AcGiVariant& value) {
			v.setElem(key.constPtr(), value);
		}, py::arg("key"), py::arg("value"))
		.def("deleteElem", [](AcGiVariant& v, const AcString& key) { v.deleteElem(key.constPtr()); }, py::arg("key"));
}

// ===========================================================================
// AcGiMapper —— 材质映射（AcGiSubEntityTraits::setMapper / AcGiFaceData::setMappers）
// ===========================================================================
void bind_PyGiMapper(pym& m) {
	py::enum_<AcGiMapper::Projection>(m, "AcGiMapperProjection")
		.value("kInheritProjection", AcGiMapper::kInheritProjection)
		.value("kPlanar", AcGiMapper::kPlanar)
		.value("kBox", AcGiMapper::kBox)
		.value("kCylinder", AcGiMapper::kCylinder)
		.value("kSphere", AcGiMapper::kSphere);
	py::enum_<AcGiMapper::Tiling>(m, "AcGiMapperTiling")
		.value("kInheritTiling", AcGiMapper::kInheritTiling)
		.value("kTile", AcGiMapper::kTile)
		.value("kCrop", AcGiMapper::kCrop)
		.value("kClamp", AcGiMapper::kClamp)
		.value("kMirror", AcGiMapper::kMirror);
	py::enum_<AcGiMapper::AutoTransform>(m, "AcGiMapperAutoTransform")
		.value("kInheritAutoTransform", AcGiMapper::kInheritAutoTransform)
		.value("kNone", AcGiMapper::kNone)
		.value("kObject", AcGiMapper::kObject)
		.value("kModel", AcGiMapper::kModel);

	py::class_<AcGiMapper>(m, "AcGiMapper", "材质映射（投影 / 平铺 / 变换）")
		.def(py::init<>())
		.def(py::init<const AcGiMapper&>(), py::arg("src"))
		.def("setProjection", pyarx::enumSetter(&AcGiMapper::setProjection), py::arg("projection"))
		.def("projection", &AcGiMapper::projection)
		.def("setAutoTransform", pyarx::enumSetter(&AcGiMapper::setAutoTransform), py::arg("auto_transform"))
		.def("autoTransform", &AcGiMapper::autoTransform)
		.def("setTransform", [](AcGiMapper& mp, py::object mat) { mp.setTransform(AsMatrix3d(mat)); }, py::arg("matrix"))
		.def("transform", [](const AcGiMapper& mp) { return mp.transform(); })
		.def("setUTiling", pyarx::enumSetter(&AcGiMapper::setUTiling), py::arg("tiling"))
		.def("setVTiling", pyarx::enumSetter(&AcGiMapper::setVTiling), py::arg("tiling"))
		.def("uTiling", &AcGiMapper::uTiling)
		.def("vTiling", &AcGiMapper::vTiling)
		.def(py::self == py::self)
		.def(py::self != py::self);
}
