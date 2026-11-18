#include "stdafx.h"
#include "PyErrorStatusException.h"
#include <dbplotsettings.h>
#include "PyDbPlotSettings.h"

PyWrapper_Cpp(DbPlotSettings);

AcString PyDbPlotSettings::getPlotSettingsName() const {
	if (!impObj()) return AcString();
	const ACHAR* p = nullptr;
	if(!PyWarnIfError(impObj()->getPlotSettingsName(p), L"AcDbPlotSettings::getPlotSettingsName")) return AcString();
	return p ? AcString(p) : AcString();
}
bool PyDbPlotSettings::setPlotSettingsName(const AcString& name) { if (!impObj()) return false; return PyWarnIfError(impObj()->setPlotSettingsName(name.kwszPtr()), L"AcDbPlotSettings::setPlotSettingsName"); }
AcString PyDbPlotSettings::getCanonicalMediaName() const {
	if (!impObj()) return AcString();
	AcString s; if(!PyWarnIfError(impObj()->getCanonicalMediaName(s), L"AcDbPlotSettings::getCanonicalMediaName")) return AcString(); return s;
}
int PyDbPlotSettings::plotPaperUnits() const { if (!impObj()) return 0; return static_cast<int>(impObj()->plotPaperUnits()); }
bool PyDbPlotSettings::plotViewportBorders() const { if (!impObj()) return false; return impObj()->plotViewportBorders() != Adesk::kFalse; }
bool PyDbPlotSettings::setPlotViewportBorders(bool on) { if (!impObj()) return false; impObj()->setPlotViewportBorders(on ? Adesk::kTrue : Adesk::kFalse); return true; }
bool PyDbPlotSettings::plotTransparency() const { if (!impObj()) return false; return impObj()->plotTransparency() != Adesk::kFalse; }
bool PyDbPlotSettings::setPlotTransparency(bool on) { if (!impObj()) return false; impObj()->setPlotTransparency(on ? Adesk::kTrue : Adesk::kFalse); return true; }
bool PyDbPlotSettings::plotPlotStyles() const { if (!impObj()) return false; return impObj()->plotPlotStyles() != Adesk::kFalse; }
bool PyDbPlotSettings::setPlotPlotStyles(bool on) { if (!impObj()) return false; impObj()->setPlotPlotStyles(on ? Adesk::kTrue : Adesk::kFalse); return true; }
bool PyDbPlotSettings::showPlotStyles() const { if (!impObj()) return false; return impObj()->showPlotStyles() != Adesk::kFalse; }
bool PyDbPlotSettings::setShowPlotStyles(bool on) { if (!impObj()) return false; impObj()->setShowPlotStyles(on ? Adesk::kTrue : Adesk::kFalse); return true; }
int PyDbPlotSettings::plotRotation() const { if (!impObj()) return 0; return static_cast<int>(impObj()->plotRotation()); }
bool PyDbPlotSettings::plotCentered() const { if (!impObj()) return false; return impObj()->plotCentered() != Adesk::kFalse; }
bool PyDbPlotSettings::plotHidden() const { if (!impObj()) return false; return impObj()->plotHidden() != Adesk::kFalse; }
bool PyDbPlotSettings::setPlotHidden(bool on) { if (!impObj()) return false; impObj()->setPlotHidden(on ? Adesk::kTrue : Adesk::kFalse); return true; }
int PyDbPlotSettings::shadePlot() const { if (!impObj()) return 0; return static_cast<int>(impObj()->shadePlot()); }
bool PyDbPlotSettings::setShadePlot(int type) { if (!impObj()) return false; return PyWarnIfError(impObj()->setShadePlot(static_cast<AcDbPlotSettings::ShadePlotType>(type)), L"AcDbPlotSettings::setShadePlot"); }
int PyDbPlotSettings::shadePlotResLevel() const { if (!impObj()) return 0; return static_cast<int>(impObj()->shadePlotResLevel()); }
bool PyDbPlotSettings::setShadePlotResLevel(int level) { if (!impObj()) return false; return PyWarnIfError(impObj()->setShadePlotResLevel(static_cast<AcDbPlotSettings::ShadePlotResLevel>(level)), L"AcDbPlotSettings::setShadePlotResLevel"); }
int PyDbPlotSettings::shadePlotCustomDPI() const { if (!impObj()) return 0; return static_cast<int>(impObj()->shadePlotCustomDPI()); }
bool PyDbPlotSettings::setShadePlotCustomDPI(int dpi) { if (!impObj()) return false; return PyWarnIfError(impObj()->setShadePlotCustomDPI(static_cast<Adesk::Int16>(dpi)), L"AcDbPlotSettings::setShadePlotCustomDPI"); }
int PyDbPlotSettings::plotType() const { if (!impObj()) return 0; return static_cast<int>(impObj()->plotType()); }
int PyDbPlotSettings::stdScaleType() const { if (!impObj()) return 0; return static_cast<int>(impObj()->stdScaleType()); }
bool PyDbPlotSettings::scaleLineweights() const { if (!impObj()) return false; return impObj()->scaleLineweights() != Adesk::kFalse; }
bool PyDbPlotSettings::setScaleLineweights(bool on) { if (!impObj()) return false; impObj()->setScaleLineweights(on ? Adesk::kTrue : Adesk::kFalse); return true; }
bool PyDbPlotSettings::printLineweights() const { if (!impObj()) return false; return impObj()->printLineweights() != Adesk::kFalse; }
bool PyDbPlotSettings::setPrintLineweights(bool on) { if (!impObj()) return false; impObj()->setPrintLineweights(on ? Adesk::kTrue : Adesk::kFalse); return true; }
bool PyDbPlotSettings::drawViewportsFirst() const { if (!impObj()) return false; return impObj()->drawViewportsFirst() != Adesk::kFalse; }
bool PyDbPlotSettings::setDrawViewportsFirst(bool on) { if (!impObj()) return false; impObj()->setDrawViewportsFirst(on ? Adesk::kTrue : Adesk::kFalse); return true; }
bool PyDbPlotSettings::modelType() const { if (!impObj()) return false; return impObj()->modelType() != Adesk::kFalse; }
bool PyDbPlotSettings::plotWireframe() const { if (!impObj()) return false; return impObj()->plotWireframe(); }
bool PyDbPlotSettings::plotAsRaster() const { if (!impObj()) return false; return impObj()->plotAsRaster(); }

py::tuple PyDbPlotSettings::getPlotPaperSize() const {
	if (!impObj()) return py::tuple();
	double w = 0.0, h = 0.0;
	PyWarnIfError(impObj()->getPlotPaperSize(w, h), L"AcDbPlotSettings::getPlotPaperSize");
	return py::make_tuple(w, h);
}

py::tuple PyDbPlotSettings::getPlotOrigin() const {
	if (!impObj()) return py::tuple();
	double x = 0.0, y = 0.0;
	PyWarnIfError(impObj()->getPlotOrigin(x, y), L"AcDbPlotSettings::getPlotOrigin");
	return py::make_tuple(x, y);
}

py::tuple PyDbPlotSettings::getCustomPrintScale() const {
	if (!impObj()) return py::tuple();
	double num = 0.0, den = 0.0;
	PyWarnIfError(impObj()->getCustomPrintScale(num, den), L"AcDbPlotSettings::getCustomPrintScale");
	return py::make_tuple(num, den);
}

AcString PyDbPlotSettings::getPlotViewName() const {
	if (!impObj()) return AcString();
	const ACHAR* p = nullptr;
	if (!PyWarnIfError(impObj()->getPlotViewName(p), L"AcDbPlotSettings::getPlotViewName")) return AcString();
	return p ? AcString(p) : AcString();
}

AcString PyDbPlotSettings::getCurrentStyleSheet() const {
	if (!impObj()) return AcString();
	const ACHAR* p = nullptr;
	if (!PyWarnIfError(impObj()->getCurrentStyleSheet(p), L"AcDbPlotSettings::getCurrentStyleSheet")) return AcString();
	return p ? AcString(p) : AcString();
}

double PyDbPlotSettings::getStdScale() const {
	if (!impObj()) return 0.0;
	double s = 1.0;
	PyWarnIfError(impObj()->getStdScale(s), L"AcDbPlotSettings::getStdScale");
	return s;
}

bool PyDbPlotSettings::useStandardScale() const { if (!impObj()) return false; return impObj()->useStandardScale(); }

AcString PyDbPlotSettings::getPlotCfgName() const {
	if (!impObj()) return AcString();
	const ACHAR* p = nullptr;
	if (!PyWarnIfError(impObj()->getPlotCfgName(p), L"AcDbPlotSettings::getPlotCfgName")) return AcString();
	return p ? AcString(p) : AcString();
}

void bind_PyDbPlotSettings(pym& m) {
	py::class_<PyDbPlotSettings, PyDbObject>(m, "PyDbPlotSettings")
		.def(py::init<AcDbPlotSettings*>(), py::arg("p"))
		.def(py::init([]() { return std::make_unique<PyDbPlotSettings>(new AcDbPlotSettings()).release(); }), "无参构造（new 默认 AcDbPlotSettings）")
		PyWrapper_BIND(DbPlotSettings)
		.def("getPlotSettingsName", &PyDbPlotSettings::getPlotSettingsName)
		.def("setPlotSettingsName", &PyDbPlotSettings::setPlotSettingsName, py::arg("name"))
		.def("getCanonicalMediaName", &PyDbPlotSettings::getCanonicalMediaName)
		.def("plotPaperUnits", &PyDbPlotSettings::plotPaperUnits)
		.def("plotViewportBorders", &PyDbPlotSettings::plotViewportBorders)
		.def("setPlotViewportBorders", &PyDbPlotSettings::setPlotViewportBorders, py::arg("on"))
		.def("plotTransparency", &PyDbPlotSettings::plotTransparency)
		.def("setPlotTransparency", &PyDbPlotSettings::setPlotTransparency, py::arg("on"))
		.def("plotPlotStyles", &PyDbPlotSettings::plotPlotStyles)
		.def("setPlotPlotStyles", &PyDbPlotSettings::setPlotPlotStyles, py::arg("on"))
		.def("showPlotStyles", &PyDbPlotSettings::showPlotStyles)
		.def("setShowPlotStyles", &PyDbPlotSettings::setShowPlotStyles, py::arg("on"))
		.def("plotRotation", &PyDbPlotSettings::plotRotation)
		.def("plotCentered", &PyDbPlotSettings::plotCentered)
		.def("plotHidden", &PyDbPlotSettings::plotHidden)
		.def("setPlotHidden", &PyDbPlotSettings::setPlotHidden, py::arg("on"))
		.def("shadePlot", &PyDbPlotSettings::shadePlot)
		.def("setShadePlot", &PyDbPlotSettings::setShadePlot, py::arg("type"))
		.def("shadePlotResLevel", &PyDbPlotSettings::shadePlotResLevel)
		.def("setShadePlotResLevel", &PyDbPlotSettings::setShadePlotResLevel, py::arg("level"))
		.def("shadePlotCustomDPI", &PyDbPlotSettings::shadePlotCustomDPI)
		.def("setShadePlotCustomDPI", &PyDbPlotSettings::setShadePlotCustomDPI, py::arg("dpi"))
		.def("plotType", &PyDbPlotSettings::plotType)
		.def("stdScaleType", &PyDbPlotSettings::stdScaleType)
		.def("scaleLineweights", &PyDbPlotSettings::scaleLineweights)
		.def("setScaleLineweights", &PyDbPlotSettings::setScaleLineweights, py::arg("on"))
		.def("printLineweights", &PyDbPlotSettings::printLineweights)
		.def("setPrintLineweights", &PyDbPlotSettings::setPrintLineweights, py::arg("on"))
		.def("drawViewportsFirst", &PyDbPlotSettings::drawViewportsFirst)
		.def("setDrawViewportsFirst", &PyDbPlotSettings::setDrawViewportsFirst, py::arg("on"))
		.def("modelType", &PyDbPlotSettings::modelType)
		.def("plotWireframe", &PyDbPlotSettings::plotWireframe)
		.def("plotAsRaster", &PyDbPlotSettings::plotAsRaster)
		.def("getPlotPaperSize", &PyDbPlotSettings::getPlotPaperSize)
		.def("getPlotOrigin", &PyDbPlotSettings::getPlotOrigin)
		.def("getCustomPrintScale", &PyDbPlotSettings::getCustomPrintScale)
		.def("getPlotViewName", &PyDbPlotSettings::getPlotViewName)
		.def("getCurrentStyleSheet", &PyDbPlotSettings::getCurrentStyleSheet)
		.def("getStdScale", &PyDbPlotSettings::getStdScale)
		.def("useStandardScale", &PyDbPlotSettings::useStandardScale)
		.def("getPlotCfgName", &PyDbPlotSettings::getPlotCfgName);
}
