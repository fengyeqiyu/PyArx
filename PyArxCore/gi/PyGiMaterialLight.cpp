// ===========================================================================
// PyGiMaterialLight.cpp —— 材质 / 灯光 / 渲染环境绑定
// ===========================================================================
#include "stdafx.h"
#include "PyGiMaterialLight.h"
#include "acgimaterial.h"
#include "AcGiLightTraits.h"
#include "AcGiEnvironment.h"
#include "PyEnumArg.h"

// ===========================================================================
// 材质
// ===========================================================================
void bind_PyGiMaterial(pym& m)
{
	py::enum_<AcGiMaterialTraits::IlluminationModel>(m, "AcGiMaterialIlluminationModel")
		.value("kBlinnShader", AcGiMaterialTraits::kBlinnShader)
		.value("kMetalShader", AcGiMaterialTraits::kMetalShader);

	py::enum_<AcGiMaterialTraits::ChannelFlags>(m, "AcGiMaterialChannelFlags", py::arithmetic())
		.value("kNone", AcGiMaterialTraits::kNone)
		.value("kUseDiffuse", AcGiMaterialTraits::kUseDiffuse)
		.value("kUseSpecular", AcGiMaterialTraits::kUseSpecular)
		.value("kUseReflection", AcGiMaterialTraits::kUseReflection)
		.value("kUseOpacity", AcGiMaterialTraits::kUseOpacity)
		.value("kUseBump", AcGiMaterialTraits::kUseBump)
		.value("kUseRefraction", AcGiMaterialTraits::kUseRefraction)
		.value("kUseNormalMap", AcGiMaterialTraits::kUseNormalMap)
		.value("kUseAll", AcGiMaterialTraits::kUseAll);

	py::enum_<AcGiMaterialTraits::Mode>(m, "AcGiMaterialMode")
		.value("kRealistic", AcGiMaterialTraits::kRealistic)
		.value("kAdvanced", AcGiMaterialTraits::kAdvanced);

	py::enum_<AcGiMaterialTraits::LuminanceMode>(m, "AcGiMaterialLuminanceMode")
		.value("kSelfIllumination", AcGiMaterialTraits::kSelfIllumination)
		.value("kLuminance", AcGiMaterialTraits::kLuminance);

	py::enum_<AcGiMaterialTraits::NormalMapMethod>(m, "AcGiMaterialNormalMapMethod")
		.value("kTangentSpace", AcGiMaterialTraits::kTangentSpace);

	py::enum_<AcGiMaterialTraits::GlobalIlluminationMode>(m, "AcGiMaterialGlobalIlluminationMode")
		.value("kGlobalIlluminationNone", AcGiMaterialTraits::kGlobalIlluminationNone)
		.value("kGlobalIlluminationCast", AcGiMaterialTraits::kGlobalIlluminationCast)
		.value("kGlobalIlluminationReceive", AcGiMaterialTraits::kGlobalIlluminationReceive)
		.value("kGlobalIlluminationCastAndReceive", AcGiMaterialTraits::kGlobalIlluminationCastAndReceive);

	py::enum_<AcGiMaterialTraits::FinalGatherMode>(m, "AcGiMaterialFinalGatherMode")
		.value("kFinalGatherNone", AcGiMaterialTraits::kFinalGatherNone)
		.value("kFinalGatherCast", AcGiMaterialTraits::kFinalGatherCast)
		.value("kFinalGatherReceive", AcGiMaterialTraits::kFinalGatherReceive)
		.value("kFinalGatherCastAndReceive", AcGiMaterialTraits::kFinalGatherCastAndReceive);

	py::enum_<AcGiMaterialColor::Method>(m, "AcGiMaterialColorMethod")
		.value("kInherit", AcGiMaterialColor::kInherit)
		.value("kOverride", AcGiMaterialColor::kOverride);

	py::enum_<AcGiMaterialMap::Source>(m, "AcGiMaterialMapSource")
		.value("kScene", AcGiMaterialMap::kScene)
		.value("kFile", AcGiMaterialMap::kFile)
		.value("kProcedural", AcGiMaterialMap::kProcedural);

	py::enum_<AcGiMaterialMap::Filter>(m, "AcGiMaterialMapFilter")
		.value("kFilterDefault", AcGiMaterialMap::kFilterDefault)
		.value("kFilterNone", AcGiMaterialMap::kFilterNone);

	py::enum_<AcGiProceduralTexture::Type>(m, "AcGiProceduralTextureType")
		.value("kWood", AcGiProceduralTexture::kWood)
		.value("kMarble", AcGiProceduralTexture::kMarble)
		.value("kGeneric", AcGiProceduralTexture::kGeneric);

	// ---- 值类 ----
	py::class_<AcGiMaterialColor>(m, "AcGiMaterialColor", "材质分量颜色（method + color + factor）")
		.def(py::init<>())
		.def("set", [](AcGiMaterialColor& c, const AcGiMaterialColor& o) { c.set(o); })
		.def("setMethod", pyarx::enumSetter(&AcGiMaterialColor::setMethod), py::arg("method"))
		.def("method", &AcGiMaterialColor::method)
		.def("setFactor", &AcGiMaterialColor::setFactor)
		.def("factor", &AcGiMaterialColor::factor)
		.def("setColor", &AcGiMaterialColor::setColor)
		.def("color", &AcGiMaterialColor::color)
		.def("__eq__", [](const AcGiMaterialColor& a, const AcGiMaterialColor& b) { return a == b; },
			py::is_operator());

	py::class_<AcGiMaterialTexture>(m, "AcGiMaterialTexture", "材质贴图基类（实例由 CAD 提供）")
		.def("__eq__", [](const AcGiMaterialTexture& a, const AcGiMaterialTexture& b) { return a == b; },
			py::is_operator());

	py::class_<AcGiImageTexture, AcGiMaterialTexture>(m, "AcGiImageTexture", "图像贴图基类");

	py::class_<AcGiImageFileTexture, AcGiImageTexture>(m, "AcGiImageFileTexture", "图像文件贴图")
		.def(py::init<>())
		.def("setSourceFileName", [](AcGiImageFileTexture& t, const AcString& name) {
			t.setSourceFileName(name.kwszPtr());
		})
		.def("sourceFileName", [](const AcGiImageFileTexture& t) { return AcString(t.sourceFileName()); });

	py::class_<AcGiProceduralTexture, AcGiMaterialTexture>(m, "AcGiProceduralTexture", "程序化贴图基类");

	py::class_<AcGiGenericTexture, AcGiProceduralTexture>(m, "AcGiGenericTexture", "通用程序化贴图");
	py::class_<AcGiWoodTexture, AcGiProceduralTexture>(m, "AcGiWoodTexture", "木纹程序化贴图");
	py::class_<AcGiMarbleTexture, AcGiProceduralTexture>(m, "AcGiMarbleTexture", "大理石程序化贴图");

	py::class_<AcGiMaterialMap>(m, "AcGiMaterialMap", "材质分量贴图（source + texture + blend + mapper）")
		.def(py::init<>())
		.def("set", [](AcGiMaterialMap& mp, const AcGiMaterialMap& o) { mp.set(o); })
		.def("setSource", pyarx::enumSetter(&AcGiMaterialMap::setSource), py::arg("source"))
		.def("source", &AcGiMaterialMap::source)
		.def("setTexture", &AcGiMaterialMap::setTexture)
		.def("texture", &AcGiMaterialMap::texture, py::return_value_policy::reference)
		.def("setBlendFactor", &AcGiMaterialMap::setBlendFactor)
		.def("blendFactor", &AcGiMaterialMap::blendFactor)
		.def("setMapper", &AcGiMaterialMap::setMapper)
		.def("mapper", &AcGiMaterialMap::mapper)
		.def("setFilter", pyarx::enumSetter(&AcGiMaterialMap::setFilter), py::arg("filter"))
		.def("filter", &AcGiMaterialMap::filter)
		.def("__eq__", [](const AcGiMaterialMap& a, const AcGiMaterialMap& b) { return a == b; },
			py::is_operator());

	// ---- traits 接口（抽象，实例由 CAD 提供）----
	py::class_<AcGiMaterialTraits>(m, "AcGiMaterialTraits",
		"材质 traits 接口（抽象；实例由 CAD 材质对象提供）")
		.def("setAmbient", &AcGiMaterialTraits::setAmbient)
		.def("ambient", [](const AcGiMaterialTraits& t) { AcGiMaterialColor c; t.ambient(c); return c; })
		.def("setDiffuse", &AcGiMaterialTraits::setDiffuse)
		.def("diffuse", [](const AcGiMaterialTraits& t) {
			AcGiMaterialColor c; AcGiMaterialMap mp; t.diffuse(c, mp); return py::make_tuple(c, mp);
		})
		.def("setSpecular", &AcGiMaterialTraits::setSpecular)
		.def("specular", [](const AcGiMaterialTraits& t) {
			AcGiMaterialColor c; AcGiMaterialMap mp; double gloss = 0.0;
			t.specular(c, mp, gloss); return py::make_tuple(c, mp, gloss);
		})
		.def("setReflection", &AcGiMaterialTraits::setReflection)
		.def("reflection", [](const AcGiMaterialTraits& t) { AcGiMaterialMap mp; t.reflection(mp); return mp; })
		.def("setOpacity", &AcGiMaterialTraits::setOpacity)
		.def("opacity", [](const AcGiMaterialTraits& t) {
			double pct = 0.0; AcGiMaterialMap mp; t.opacity(pct, mp); return py::make_tuple(pct, mp);
		})
		.def("setBump", &AcGiMaterialTraits::setBump)
		.def("bump", [](const AcGiMaterialTraits& t) { AcGiMaterialMap mp; t.bump(mp); return mp; })
		.def("setRefraction", &AcGiMaterialTraits::setRefraction)
		.def("refraction", [](const AcGiMaterialTraits& t) {
			double idx = 0.0; AcGiMaterialMap mp; t.refraction(idx, mp); return py::make_tuple(idx, mp);
		})
		.def("setTranslucence", &AcGiMaterialTraits::setTranslucence)
		.def("translucence", &AcGiMaterialTraits::translucence)
		.def("setSelfIllumination", &AcGiMaterialTraits::setSelfIllumination)
		.def("selfIllumination", &AcGiMaterialTraits::selfIllumination)
		.def("setReflectivity", &AcGiMaterialTraits::setReflectivity)
		.def("reflectivity", &AcGiMaterialTraits::reflectivity)
		.def("setIlluminationModel", pyarx::enumSetter(&AcGiMaterialTraits::setIlluminationModel), py::arg("model"))
		.def("illuminationModel", &AcGiMaterialTraits::illuminationModel)
		.def("setChannelFlags", pyarx::enumSetter(&AcGiMaterialTraits::setChannelFlags), py::arg("flags"))
		.def("channelFlags", &AcGiMaterialTraits::channelFlags)
		.def("setMode", pyarx::enumSetter(&AcGiMaterialTraits::setMode), py::arg("value"))
		.def("mode", &AcGiMaterialTraits::mode);
}

// ===========================================================================
// 灯光
// ===========================================================================
void bind_PyGiLight(pym& m)
{
	// ---- 颜色值类型 ----
	py::class_<AcGiColorRGB>(m, "AcGiColorRGB", "线性 RGB 颜色（0..1）")
		.def(py::init<>())
		.def(py::init<double, double, double>(), py::arg("r"), py::arg("g"), py::arg("b"))
		.def_readwrite("red", &AcGiColorRGB::red)
		.def_readwrite("green", &AcGiColorRGB::green)
		.def_readwrite("blue", &AcGiColorRGB::blue)
		.def("__repr__", [](const AcGiColorRGB& c) {
			return "AcGiColorRGB(" + std::to_string(c.red) + ", " + std::to_string(c.green) + ", " +
				std::to_string(c.blue) + ")";
		});

	py::class_<AcGiColorRGBA>(m, "AcGiColorRGBA", "线性 RGBA 颜色（0..1）")
		.def(py::init<>())
		.def(py::init<double, double, double, double>(), py::arg("r"), py::arg("g"), py::arg("b"), py::arg("a"))
		.def_readwrite("red", &AcGiColorRGBA::red)
		.def_readwrite("green", &AcGiColorRGBA::green)
		.def_readwrite("blue", &AcGiColorRGBA::blue)
		.def_readwrite("alpha", &AcGiColorRGBA::alpha);

	// ---- 阴影参数 ----
	py::enum_<AcGiShadowParameters::ShadowType>(m, "AcGiShadowType")
		.value("kShadowsRayTraced", AcGiShadowParameters::kShadowsRayTraced)
		.value("kShadowMaps", AcGiShadowParameters::kShadowMaps)
		.value("kAreaSampled", AcGiShadowParameters::kAreaSampled);

	py::enum_<AcGiShadowParameters::ExtendedLightShape>(m, "AcGiExtendedLightShape")
		.value("kLinear", AcGiShadowParameters::kLinear)
		.value("kRectangle", AcGiShadowParameters::kRectangle)
		.value("kDisk", AcGiShadowParameters::kDisk)
		.value("kCylinder", AcGiShadowParameters::kCylinder)
		.value("kSphere", AcGiShadowParameters::kSphere);

	py::class_<AcGiShadowParameters>(m, "AcGiShadowParameters", "阴影参数")
		.def(py::init<>())
		.def("setShadowsOn", &AcGiShadowParameters::setShadowsOn)
		.def("shadowsOn", &AcGiShadowParameters::shadowsOn)
		.def("setShadowType", pyarx::enumSetter(&AcGiShadowParameters::setShadowType), py::arg("typ"))
		.def("shadowType", &AcGiShadowParameters::shadowType)
		.def("setShadowMapSize", &AcGiShadowParameters::setShadowMapSize)
		.def("shadowMapSize", &AcGiShadowParameters::shadowMapSize)
		.def("setShadowMapSoftness", &AcGiShadowParameters::setShadowMapSoftness)
		.def("shadowMapSoftness", &AcGiShadowParameters::shadowMapSoftness)
		.def("setShadowSamples", &AcGiShadowParameters::setShadowSamples)
		.def("shadowSamples", &AcGiShadowParameters::shadowSamples)
		.def("setShapeVisibility", &AcGiShadowParameters::setShapeVisibility)
		.def("shapeVisibility", &AcGiShadowParameters::shapeVisibility)
		.def("setExtendedLightShape", pyarx::enumSetter(&AcGiShadowParameters::setExtendedLightShape), py::arg("shape"))
		.def("extendedLightShape", &AcGiShadowParameters::extendedLightShape)
		.def("setExtendedLightLength", &AcGiShadowParameters::setExtendedLightLength)
		.def("extendedLightLength", &AcGiShadowParameters::extendedLightLength)
		.def("setExtendedLightWidth", &AcGiShadowParameters::setExtendedLightWidth)
		.def("extendedLightWidth", &AcGiShadowParameters::extendedLightWidth)
		.def("setExtendedLightRadius", &AcGiShadowParameters::setExtendedLightRadius)
		.def("extendedLightRadius", &AcGiShadowParameters::extendedLightRadius);

	// ---- 衰减 ----
	py::enum_<AcGiLightAttenuation::AttenuationType>(m, "AcGiLightAttenuationType")
		.value("kNone", AcGiLightAttenuation::kNone)
		.value("kInverseLinear", AcGiLightAttenuation::kInverseLinear)
		.value("kInverseSquare", AcGiLightAttenuation::kInverseSquare);

	py::class_<AcGiLightAttenuation>(m, "AcGiLightAttenuation", "灯光衰减参数")
		.def(py::init<>())
		.def("setAttenuationType", pyarx::enumSetter(&AcGiLightAttenuation::setAttenuationType), py::arg("typ"))
		.def("attenuationType", &AcGiLightAttenuation::attenuationType)
		.def("setUseLimits", &AcGiLightAttenuation::setUseLimits)
		.def("useLimits", &AcGiLightAttenuation::useLimits)
		.def("setLimits", &AcGiLightAttenuation::setLimits)
		.def("startLimit", &AcGiLightAttenuation::startLimit)
		.def("endLimit", &AcGiLightAttenuation::endLimit);

	// ---- 天空参数 ----
	// AcGiSkyParameters 在 SDK 里**没有导出符号**（setXxxImp / isA 全部未声明 ACDBCORE2D_PORT，
	// 实测链接失败 LNK2001），因此无法构造、无法调用，只能跳过。需要天空参数时走
	// AcGiSkyBackgroundTraits（CAD 侧实例），本模块不提供该值类型。

	// ---- 灯光 traits（抽象）----
	py::class_<AcGiLightTraits>(m, "AcGiLightTraits", "灯光 traits 基类（抽象）")
		.def("setOn", &AcGiLightTraits::setOn)
		.def("isOn", &AcGiLightTraits::isOn);

	py::class_<AcGiStandardLightTraits, AcGiLightTraits>(m, "AcGiStandardLightTraits",
		"标准灯光 traits（抽象）")
		.def("setIntensity", &AcGiStandardLightTraits::setIntensity)
		.def("intensity", &AcGiStandardLightTraits::intensity)
		.def("setLightColor", &AcGiStandardLightTraits::setLightColor)
		.def("lightColor", &AcGiStandardLightTraits::lightColor)
		.def("setShadowParameters", &AcGiStandardLightTraits::setShadowParameters)
		.def("shadowParameters", [](const AcGiStandardLightTraits& t) {
			AcGiShadowParameters p; t.shadowParameters(p); return p;
		});

	py::class_<AcGiPointLightTraits, AcGiStandardLightTraits>(m, "AcGiPointLightTraits",
		"点光源 traits（抽象）")
		.def("setPosition", &AcGiPointLightTraits::setPosition)
		.def("position", &AcGiPointLightTraits::position)
		.def("lightAttenuation", [](const AcGiPointLightTraits& t) {
			AcGiLightAttenuation a; t.lightAttenuation(a); return a;
		})
		.def("setAttenuation", &AcGiPointLightTraits::setAttenuation)
		.def("setPhysicalIntensity", &AcGiPointLightTraits::setPhysicalIntensity)
		.def("physicalIntensity", &AcGiPointLightTraits::physicalIntensity)
		.def("setLampColor", &AcGiPointLightTraits::setLampColor)
		.def("lampColor", [](const AcGiPointLightTraits& t) {
			AcGiColorRGB c; t.lampColor(c); return c;
		})
		.def("setHasTarget", &AcGiPointLightTraits::setHasTarget)
		.def("hasTarget", &AcGiPointLightTraits::hasTarget)
		.def("setTargetLocation", &AcGiPointLightTraits::setTargetLocation)
		.def("targetLocation", &AcGiPointLightTraits::targetLocation);

	py::class_<AcGiSpotLightTraits, AcGiStandardLightTraits>(m, "AcGiSpotLightTraits",
		"聚光灯 traits（抽象）")
		.def("setHotspotAndFalloff", &AcGiSpotLightTraits::setHotspotAndFalloff)
		.def("hotspot", &AcGiSpotLightTraits::hotspot)
		.def("falloff", &AcGiSpotLightTraits::falloff);

	py::class_<AcGiDistantLightTraits, AcGiStandardLightTraits>(m, "AcGiDistantLightTraits",
		"平行光 traits（抽象）")
		.def("setLightDirection", &AcGiDistantLightTraits::setLightDirection)
		.def("lightDirection", &AcGiDistantLightTraits::lightDirection)
		.def("setIsSunlight", &AcGiDistantLightTraits::setIsSunlight)
		.def("isSunlight", &AcGiDistantLightTraits::isSunlight);
		// setSkyParameters/skyParameters 依赖未导出的 AcGiSkyParameters，故不导出。

	py::class_<AcGiWebLightTraits, AcGiPointLightTraits>(m, "AcGiWebLightTraits",
		"光域网灯光 traits（抽象）")
		.def("setPhysicalIntensity", &AcGiWebLightTraits::setPhysicalIntensity)
		.def("physicalIntensity", &AcGiWebLightTraits::physicalIntensity)
		.def("setLampColor", &AcGiWebLightTraits::setLampColor)
		.def("lampColor", [](const AcGiWebLightTraits& t) { AcGiColorRGB c; t.lampColor(c); return c; })
		.def("setWebFile", [](AcGiWebLightTraits& t, const AcString& f) { t.setWebFile(f.kwszPtr()); })
		.def("webFile", [](const AcGiWebLightTraits& t) {
			AcString s; t.webFile(s); return s;
		});
}

// ===========================================================================
// 渲染环境
// ===========================================================================
void bind_PyGiEnvironment(pym& m)
{
	py::class_<AcGiSolidBackgroundTraits>(m, "AcGiSolidBackgroundTraits", "纯色背景 traits（抽象）")
		.def("setColorSolid", &AcGiSolidBackgroundTraits::setColorSolid)
		.def("colorSolid", &AcGiSolidBackgroundTraits::colorSolid);

	py::class_<AcGiGradientBackgroundTraits>(m, "AcGiGradientBackgroundTraits", "渐变背景 traits（抽象）")
		.def("setColorTop", &AcGiGradientBackgroundTraits::setColorTop)
		.def("colorTop", &AcGiGradientBackgroundTraits::colorTop)
		.def("setColorMiddle", &AcGiGradientBackgroundTraits::setColorMiddle)
		.def("colorMiddle", &AcGiGradientBackgroundTraits::colorMiddle)
		.def("setColorBottom", &AcGiGradientBackgroundTraits::setColorBottom)
		.def("colorBottom", &AcGiGradientBackgroundTraits::colorBottom)
		.def("setHorizon", &AcGiGradientBackgroundTraits::setHorizon)
		.def("horizon", &AcGiGradientBackgroundTraits::horizon)
		.def("setHeight", &AcGiGradientBackgroundTraits::setHeight)
		.def("height", &AcGiGradientBackgroundTraits::height)
		.def("setRotation", &AcGiGradientBackgroundTraits::setRotation)
		.def("rotation", &AcGiGradientBackgroundTraits::rotation);

	py::class_<AcGiImageBackgroundTraits>(m, "AcGiImageBackgroundTraits", "图像背景 traits（抽象）")
		.def("setImageFilename", [](AcGiImageBackgroundTraits& t, const AcString& n) {
			t.setImageFilename(n.kwszPtr());
		})
		.def("imageFilename", [](const AcGiImageBackgroundTraits& t) { return AcString(t.imageFilename()); })
		.def("setFitToScreen", &AcGiImageBackgroundTraits::setFitToScreen)
		.def("fitToScreen", &AcGiImageBackgroundTraits::fitToScreen)
		.def("setMaintainAspectRatio", &AcGiImageBackgroundTraits::setMaintainAspectRatio)
		.def("maintainAspectRatio", &AcGiImageBackgroundTraits::maintainAspectRatio)
		.def("setUseTiling", &AcGiImageBackgroundTraits::setUseTiling)
		.def("useTiling", &AcGiImageBackgroundTraits::useTiling)
		.def("setXOffset", &AcGiImageBackgroundTraits::setXOffset)
		.def("xOffset", &AcGiImageBackgroundTraits::xOffset)
		.def("setYOffset", &AcGiImageBackgroundTraits::setYOffset)
		.def("yOffset", &AcGiImageBackgroundTraits::yOffset)
		.def("setXScale", &AcGiImageBackgroundTraits::setXScale)
		.def("xScale", &AcGiImageBackgroundTraits::xScale)
		.def("setYScale", &AcGiImageBackgroundTraits::setYScale)
		.def("yScale", &AcGiImageBackgroundTraits::yScale);

	py::class_<AcGiGroundPlaneBackgroundTraits>(m, "AcGiGroundPlaneBackgroundTraits",
		"地面背景 traits（抽象）")
		.def("setColorSkyZenith", &AcGiGroundPlaneBackgroundTraits::setColorSkyZenith)
		.def("colorSkyZenith", &AcGiGroundPlaneBackgroundTraits::colorSkyZenith)
		.def("setColorSkyHorizon", &AcGiGroundPlaneBackgroundTraits::setColorSkyHorizon)
		.def("colorSkyHorizon", &AcGiGroundPlaneBackgroundTraits::colorSkyHorizon)
		.def("setColorUndergroundHorizon", &AcGiGroundPlaneBackgroundTraits::setColorUndergroundHorizon)
		.def("colorUndergroundHorizon", &AcGiGroundPlaneBackgroundTraits::colorUndergroundHorizon)
		.def("setColorUndergroundAzimuth", &AcGiGroundPlaneBackgroundTraits::setColorUndergroundAzimuth)
		.def("colorUndergroundAzimuth", &AcGiGroundPlaneBackgroundTraits::colorUndergroundAzimuth)
		.def("setColorGroundPlaneNear", &AcGiGroundPlaneBackgroundTraits::setColorGroundPlaneNear)
		.def("colorGroundPlaneNear", &AcGiGroundPlaneBackgroundTraits::colorGroundPlaneNear)
		.def("setColorGroundPlaneFar", &AcGiGroundPlaneBackgroundTraits::setColorGroundPlaneFar)
		.def("colorGroundPlaneFar", &AcGiGroundPlaneBackgroundTraits::colorGroundPlaneFar);

	py::class_<AcGiSkyBackgroundTraits>(m, "AcGiSkyBackgroundTraits", "天空背景 traits（抽象）");
	// setSkyParameters/skyParameters 依赖未导出的 AcGiSkyParameters，故不导出。

	py::class_<AcGiIBLBackgroundTraits>(m, "AcGiIBLBackgroundTraits", "IBL 背景 traits（抽象）")
		.def("setEnable", &AcGiIBLBackgroundTraits::setEnable)
		.def("enable", &AcGiIBLBackgroundTraits::enable)
		.def("setIBLImageName", &AcGiIBLBackgroundTraits::setIBLImageName)
		.def("IBLImageName", &AcGiIBLBackgroundTraits::IBLImageName);

	// 渲染环境 / 渲染设置：纯接口，成员由 CAD 渲染侧实现，这里只导出类型。
	py::class_<AcGiRenderEnvironmentTraits>(m, "AcGiRenderEnvironmentTraits",
		"渲染环境 traits（抽象；实例由 CAD 渲染管线提供）")
		.def("__repr__", [](const AcGiRenderEnvironmentTraits&) { return "AcGiRenderEnvironmentTraits"; });

	py::class_<AcGiRenderSettingsTraits>(m, "AcGiRenderSettingsTraits",
		"渲染设置 traits（抽象；实例由 CAD 渲染管线提供）");

	py::class_<AcGiMentalRayRenderSettingsTraits, AcGiRenderSettingsTraits>(
		m, "AcGiMentalRayRenderSettingsTraits", "mental ray 渲染设置 traits（抽象）");

	py::class_<AcGiRapidRTRenderSettingsTraits>(m, "AcGiRapidRTRenderSettingsTraits",
		"RapidRT 渲染设置 traits（抽象）");

	// ---- 色调 / 曝光参数（可构造）----
	py::class_<AcGiToneOperatorParameters>(m, "AcGiToneOperatorParameters", "色调操作参数")
		.def(py::init<>())
		.def("setIsActive", &AcGiToneOperatorParameters::setIsActive)
		.def("isActive", &AcGiToneOperatorParameters::isActive)
		.def("setChromaticAdaptation", &AcGiToneOperatorParameters::setChromaticAdaptation)
		.def("chromaticAdaptation", &AcGiToneOperatorParameters::chromaticAdaptation)
		.def("setColorDifferentiation", &AcGiToneOperatorParameters::setColorDifferentiation)
		.def("colorDifferentiation", &AcGiToneOperatorParameters::colorDifferentiation)
		.def("setWhiteColor", &AcGiToneOperatorParameters::setWhiteColor)
		.def("setProcessBackground", &AcGiToneOperatorParameters::setProcessBackground);

	py::class_<AcGiPhotographicExposureParameters, AcGiToneOperatorParameters>(
		m, "AcGiPhotographicExposureParameters", "摄影曝光参数")
		.def(py::init<>())
		.def("setExposure", &AcGiPhotographicExposureParameters::setExposure)
		.def("exposure", &AcGiPhotographicExposureParameters::exposure)
		.def("setWhitePoint", &AcGiPhotographicExposureParameters::setWhitePoint)
		.def("whitePoint", &AcGiPhotographicExposureParameters::whitePoint)
		.def_static("convertExposureToBrightness",
			&AcGiPhotographicExposureParameters::convertExposureToBrightness)
		.def_static("convertBrightnessToExposure",
			&AcGiPhotographicExposureParameters::convertBrightnessToExposure);
}
