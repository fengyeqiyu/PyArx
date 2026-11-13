#include "stdafx.h"
#include "PyErrorStatusException.h"
#include "PyDbMaterial.h"

PyWrapper_Cpp(DbMaterial);

int PyDbMaterial::channelFlags() const { return (int)impObj()->channelFlags(); }
void PyDbMaterial::clearAdskMaterialCache() { impObj()->clearAdskMaterialCache(); }
double PyDbMaterial::colorBleedScale() const { return impObj()->colorBleedScale(); }
int PyDbMaterial::finalGather() const { return (int)impObj()->finalGather(); }
int PyDbMaterial::getAdskMaterialFlags() const {
	AcDbMaterial::MaterialFlags f = AcDbMaterial::kNone;
	PyWarnIfError(impObj()->getAdskMaterialFlags(f), L"AcDbMaterial::getAdskMaterialFlags");
	return (int)f;
}
int PyDbMaterial::globalIllumination() const { return (int)impObj()->globalIllumination(); }
int PyDbMaterial::illuminationModel() const { return (int)impObj()->illuminationModel(); }
double PyDbMaterial::indirectBumpScale() const { return impObj()->indirectBumpScale(); }
bool PyDbMaterial::isAnonymous() const { return impObj()->isAnonymous() != Adesk::kFalse; }
// isRenamable: SDK2024 未导出该符号，暂不封装
double PyDbMaterial::luminance() const { return impObj()->luminance(); }
int PyDbMaterial::luminanceMode() const { return (int)impObj()->luminanceMode(); }
int PyDbMaterial::mode() const { return (int)impObj()->mode(); }
double PyDbMaterial::reflectanceScale() const { return impObj()->reflectanceScale(); }
double PyDbMaterial::reflectivity() const { return impObj()->reflectivity(); }
double PyDbMaterial::selfIllumination() const { return impObj()->selfIllumination(); }
double PyDbMaterial::shininess() const { return impObj()->shininess(); }
double PyDbMaterial::translucence() const { return impObj()->translucence(); }
double PyDbMaterial::transmittanceScale() const { return impObj()->transmittanceScale(); }
bool PyDbMaterial::twoSided() const { return impObj()->twoSided() != Adesk::kFalse; }

bool PyDbMaterial::setChannelFlags(int v) {
	return PyWarnIfError(impObj()->setChannelFlags(static_cast<AcGiMaterialTraits::ChannelFlags>(v)), L"AcDbMaterial::setChannelFlags");
}
bool PyDbMaterial::setColorBleedScale(double v) {
	return PyWarnIfError(impObj()->setColorBleedScale(v), L"AcDbMaterial::setColorBleedScale");
}
bool PyDbMaterial::setFinalGather(int v) {
	impObj()->setFinalGather(static_cast<AcGiMaterialTraits::FinalGatherMode>(v));
	return true;
}
bool PyDbMaterial::setAdskMaterialFlags(int v) {
	return PyWarnIfError(impObj()->setAdskMaterialFlags(static_cast<AcDbMaterial::MaterialFlags>(v)), L"AcDbMaterial::setAdskMaterialFlags");
}
bool PyDbMaterial::setGlobalIllumination(int v) {
	impObj()->setGlobalIllumination(static_cast<AcGiMaterialTraits::GlobalIlluminationMode>(v));
	return true;
}
bool PyDbMaterial::setIlluminationModel(int v) {
	return PyWarnIfError(impObj()->setIlluminationModel(static_cast<AcGiMaterialTraits::IlluminationModel>(v)), L"AcDbMaterial::setIlluminationModel");
}
bool PyDbMaterial::setIndirectBumpScale(double v) {
	return PyWarnIfError(impObj()->setIndirectBumpScale(v), L"AcDbMaterial::setIndirectBumpScale");
}
bool PyDbMaterial::setAnonymous(bool v) {
	return PyWarnIfError(impObj()->setAnonymous(v ? Adesk::kTrue : Adesk::kFalse), L"AcDbMaterial::setAnonymous");
}
bool PyDbMaterial::setLuminance(double v) {
	return PyWarnIfError(impObj()->setLuminance(v), L"AcDbMaterial::setLuminance");
}
bool PyDbMaterial::setLuminanceMode(int v) {
	return PyWarnIfError(impObj()->setLuminanceMode(static_cast<AcGiMaterialTraits::LuminanceMode>(v)), L"AcDbMaterial::setLuminanceMode");
}
bool PyDbMaterial::setMode(int v) {
	return PyWarnIfError(impObj()->setMode(static_cast<AcGiMaterialTraits::Mode>(v)), L"AcDbMaterial::setMode");
}
bool PyDbMaterial::setName(const AcString& v) {
	return PyWarnIfError(impObj()->setName(v.kwszPtr()), L"AcDbMaterial::setName");
}
bool PyDbMaterial::setDescription(const AcString& v) {
	return PyWarnIfError(impObj()->setDescription(v.kwszPtr()), L"AcDbMaterial::setDescription");
}
bool PyDbMaterial::setReflectanceScale(double v) {
	return PyWarnIfError(impObj()->setReflectanceScale(v), L"AcDbMaterial::setReflectanceScale");
}
bool PyDbMaterial::setReflectivity(double v) {
	return PyWarnIfError(impObj()->setReflectivity(v), L"AcDbMaterial::setReflectivity");
}
bool PyDbMaterial::setSelfIllumination(double v) {
	return PyWarnIfError(impObj()->setSelfIllumination(v), L"AcDbMaterial::setSelfIllumination");
}
bool PyDbMaterial::setShininess(double v) {
	return PyWarnIfError(impObj()->setShininess(v), L"AcDbMaterial::setShininess");
}
bool PyDbMaterial::setTranslucence(double v) {
	return PyWarnIfError(impObj()->setTranslucence(v), L"AcDbMaterial::setTranslucence");
}
bool PyDbMaterial::setTransmittanceScale(double v) {
	return PyWarnIfError(impObj()->setTransmittanceScale(v), L"AcDbMaterial::setTransmittanceScale");
}
bool PyDbMaterial::setTwoSided(bool v) {
	return PyWarnIfError(impObj()->setTwoSided(v ? Adesk::kTrue : Adesk::kFalse), L"AcDbMaterial::setTwoSided");
}

void bind_PyDbMaterial(pym& m) {
	py::class_<PyDbMaterial, PyDbObject>(m, "PyDbMaterial")
		.def(py::init<AcDbMaterial*>(), py::arg("p"))
		.def(py::init([]() { return std::make_unique<PyDbMaterial>(new AcDbMaterial()).release(); }), "无参构造（new 默认 AcDbMaterial）")
		PyWrapper_BIND(DbMaterial)
		.def("channelFlags", &PyDbMaterial::channelFlags)
		.def("clearAdskMaterialCache", &PyDbMaterial::clearAdskMaterialCache)
		.def("colorBleedScale", &PyDbMaterial::colorBleedScale)
		.def("finalGather", &PyDbMaterial::finalGather)
		.def("getAdskMaterialFlags", &PyDbMaterial::getAdskMaterialFlags)
		.def("globalIllumination", &PyDbMaterial::globalIllumination)
		.def("illuminationModel", &PyDbMaterial::illuminationModel)
		.def("indirectBumpScale", &PyDbMaterial::indirectBumpScale)
		.def("isAnonymous", &PyDbMaterial::isAnonymous)
		// isRenamable: SDK2024 未导出该符号，暂不封装
		.def("luminance", &PyDbMaterial::luminance)
		.def("luminanceMode", &PyDbMaterial::luminanceMode)
		.def("mode", &PyDbMaterial::mode)
		.def("reflectanceScale", &PyDbMaterial::reflectanceScale)
		.def("reflectivity", &PyDbMaterial::reflectivity)
		.def("selfIllumination", &PyDbMaterial::selfIllumination)
		.def("shininess", &PyDbMaterial::shininess)
		.def("translucence", &PyDbMaterial::translucence)
		.def("transmittanceScale", &PyDbMaterial::transmittanceScale)
		.def("twoSided", &PyDbMaterial::twoSided)
		.def("setChannelFlags", &PyDbMaterial::setChannelFlags, py::arg("v"))
		.def("setColorBleedScale", &PyDbMaterial::setColorBleedScale, py::arg("v"))
		.def("setFinalGather", &PyDbMaterial::setFinalGather, py::arg("v"))
		.def("setAdskMaterialFlags", &PyDbMaterial::setAdskMaterialFlags, py::arg("v"))
		.def("setGlobalIllumination", &PyDbMaterial::setGlobalIllumination, py::arg("v"))
		.def("setIlluminationModel", &PyDbMaterial::setIlluminationModel, py::arg("v"))
		.def("setIndirectBumpScale", &PyDbMaterial::setIndirectBumpScale, py::arg("v"))
		.def("setAnonymous", &PyDbMaterial::setAnonymous, py::arg("v"))
		.def("setLuminance", &PyDbMaterial::setLuminance, py::arg("v"))
		.def("setLuminanceMode", &PyDbMaterial::setLuminanceMode, py::arg("v"))
		.def("setMode", &PyDbMaterial::setMode, py::arg("v"))
		.def("setName", &PyDbMaterial::setName, py::arg("v"))
		.def("setDescription", &PyDbMaterial::setDescription, py::arg("v"))
		.def("setReflectanceScale", &PyDbMaterial::setReflectanceScale, py::arg("v"))
		.def("setReflectivity", &PyDbMaterial::setReflectivity, py::arg("v"))
		.def("setSelfIllumination", &PyDbMaterial::setSelfIllumination, py::arg("v"))
		.def("setShininess", &PyDbMaterial::setShininess, py::arg("v"))
		.def("setTranslucence", &PyDbMaterial::setTranslucence, py::arg("v"))
		.def("setTransmittanceScale", &PyDbMaterial::setTransmittanceScale, py::arg("v"))
		.def("setTwoSided", &PyDbMaterial::setTwoSided, py::arg("v"))
		// 未绑（依赖工程未封装类型 AcGiMaterialColor / AcGiMaterialMap / AcGiMaterialTraits 嵌套值对象）：
		//   ambient/bump/diffuse/specular/reflection/refraction/opacity/normalMap 及对应 setter；
		//   setAdskMaterialData(const void*,...) 不透明指针；dwg/dxf in/outFields、subGetClassID 为 AcDbObject 覆写。
		;
}
