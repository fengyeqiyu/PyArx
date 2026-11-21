#include "stdafx.h"
#include "PyErrorStatusException.h"
#include "PyDbPlotSettingsValidator.h"
#include "PyDbPlotSettings.h"
#include "dbapserv.h"

// AcDbPlotSettingsValidator: pure abstract, no base class (dbplotsetval.h:34).
// 所有方法为纯虚实现，由 AutoCAD 提供。

PyDbPlotSettingsValidator PyDbPlotSettingsValidator::get() {
	auto* hs = acdbHostApplicationServices();
	return PyDbPlotSettingsValidator(hs ? hs->plotSettingsValidator() : nullptr);
}

static inline std::vector<AcString> AcStrArrToVec(const AcArray<const ACHAR*>& arr) {
	std::vector<AcString> out;
	out.reserve(arr.length());
	for (int i = 0; i < arr.length(); ++i)
		out.push_back(arr[i] ? AcString(arr[i]) : AcString());
	return out;
}

void bind_PyDbPlotSettingsValidator(pym& m) {
	py::class_<PyDbPlotSettingsValidator>(m, "PyDbPlotSettingsValidator")
		.def(py::init<AcDbPlotSettingsValidator*>(), py::arg("p") = nullptr)
		.def_static("get", &PyDbPlotSettingsValidator::get)
		.def("isNull", &PyDbPlotSettingsValidator::isNull)

		// ---- lists ----
		.def("plotDeviceList", [](PyDbPlotSettingsValidator& self) -> std::vector<AcString> {
			if (!self.impObj()) return {};
			AcArray<const ACHAR*> arr;
			if (!PyWarnIfError(self.impObj()->plotDeviceList(arr), L"AcDbPlotSettingsValidator::plotDeviceList"))
				return {};
			return AcStrArrToVec(arr);
		})
		.def("plotStyleSheetList", [](PyDbPlotSettingsValidator& self) -> std::vector<AcString> {
			if (!self.impObj()) return {};
			AcArray<const ACHAR*> arr;
			if (!PyWarnIfError(self.impObj()->plotStyleSheetList(arr), L"AcDbPlotSettingsValidator::plotStyleSheetList"))
				return {};
			return AcStrArrToVec(arr);
		})
		.def("canonicalMediaNameList", [](PyDbPlotSettingsValidator& self, PyDbPlotSettings& ps)
				-> std::vector<AcString> {
			if (!self.impObj() || !ps.impObj()) return {};
			AcArray<const ACHAR*> arr;
			if (!PyWarnIfError(self.impObj()->canonicalMediaNameList(ps.impObj(), arr),
				L"AcDbPlotSettingsValidator::canonicalMediaNameList")) return {};
			return AcStrArrToVec(arr);
		}, py::arg("ps"))
		.def("refreshLists", [](PyDbPlotSettingsValidator& self, PyDbPlotSettings& ps) {
			if (!self.impObj() || !ps.impObj()) return;
			self.impObj()->refreshLists(ps.impObj());
		}, py::arg("ps"))

		// ---- getLocaleMediaName (2 overloads) ----
		.def("getLocaleMediaName", [](PyDbPlotSettingsValidator& self, PyDbPlotSettings& ps,
				const AcString& canonicalName) -> AcString {
			if (!self.impObj() || !ps.impObj()) return AcString();
			const ACHAR* pName = nullptr;
			if (!PyWarnIfError(self.impObj()->getLocaleMediaName(ps.impObj(), canonicalName.kwszPtr(), pName),
				L"AcDbPlotSettingsValidator::getLocaleMediaName")) return AcString();
			return pName ? AcString(pName) : AcString();
		}, py::arg("ps"), py::arg("canonicalName"))
		.def("getLocaleMediaNameByIndex", [](PyDbPlotSettingsValidator& self, PyDbPlotSettings& ps,
				int index) -> AcString {
			if (!self.impObj() || !ps.impObj()) return AcString();
			const ACHAR* pName = nullptr;
			if (!PyWarnIfError(self.impObj()->getLocaleMediaName(ps.impObj(), index, pName),
				L"AcDbPlotSettingsValidator::getLocaleMediaName(index)")) return AcString();
			return pName ? AcString(pName) : AcString();
		}, py::arg("ps"), py::arg("index"))

		// ---- setters (all return ErrorStatus -> bool) ----
		.def("setPlotCfgName", [](PyDbPlotSettingsValidator& self, PyDbPlotSettings& ps,
				const AcString& deviceName, const AcString& mediaName) {
			if (!self.impObj() || !ps.impObj()) return false;
			return PyWarnIfError(self.impObj()->setPlotCfgName(ps.impObj(), deviceName.kwszPtr(),
				mediaName.kwszPtr()), L"AcDbPlotSettingsValidator::setPlotCfgName");
		}, py::arg("ps"), py::arg("deviceName"), py::arg("mediaName") = L"")
		.def("setCanonicalMediaName", [](PyDbPlotSettingsValidator& self, PyDbPlotSettings& ps,
				const AcString& mediaName) {
			if (!self.impObj() || !ps.impObj()) return false;
			return PyWarnIfError(self.impObj()->setCanonicalMediaName(ps.impObj(), mediaName.kwszPtr()),
				L"AcDbPlotSettingsValidator::setCanonicalMediaName");
		}, py::arg("ps"), py::arg("mediaName"))
		.def("setClosestMediaName", [](PyDbPlotSettingsValidator& self, PyDbPlotSettings& ps,
				double paperWidth, double paperHeight, int units, bool matchPrintableArea) {
			if (!self.impObj() || !ps.impObj()) return false;
			return PyWarnIfError(self.impObj()->setClosestMediaName(ps.impObj(), paperWidth, paperHeight,
				static_cast<PlotPaperUnits>(units), matchPrintableArea ? Adesk::kTrue : Adesk::kFalse),
				L"AcDbPlotSettingsValidator::setClosestMediaName");
		}, py::arg("ps"), py::arg("paperWidth"), py::arg("paperHeight"), py::arg("units"),
			py::arg("matchPrintableArea"))
		.def("setPlotOrigin", [](PyDbPlotSettingsValidator& self, PyDbPlotSettings& ps,
				double x, double y) {
			if (!self.impObj() || !ps.impObj()) return false;
			return PyWarnIfError(self.impObj()->setPlotOrigin(ps.impObj(), x, y),
				L"AcDbPlotSettingsValidator::setPlotOrigin");
		}, py::arg("ps"), py::arg("x"), py::arg("y"))
		.def("setPlotPaperUnits", [](PyDbPlotSettingsValidator& self, PyDbPlotSettings& ps, int units) {
			if (!self.impObj() || !ps.impObj()) return false;
			return PyWarnIfError(self.impObj()->setPlotPaperUnits(ps.impObj(), static_cast<PlotPaperUnits>(units)),
				L"AcDbPlotSettingsValidator::setPlotPaperUnits");
		}, py::arg("ps"), py::arg("units"))
		.def("setPlotRotation", [](PyDbPlotSettingsValidator& self, PyDbPlotSettings& ps, int rotation) {
			if (!self.impObj() || !ps.impObj()) return false;
			return PyWarnIfError(self.impObj()->setPlotRotation(ps.impObj(), static_cast<PlotRotation>(rotation)),
				L"AcDbPlotSettingsValidator::setPlotRotation");
		}, py::arg("ps"), py::arg("rotation"))
		.def("setPlotCentered", [](PyDbPlotSettingsValidator& self, PyDbPlotSettings& ps, bool centered) {
			if (!self.impObj() || !ps.impObj()) return false;
			return PyWarnIfError(self.impObj()->setPlotCentered(ps.impObj(),
				centered ? Adesk::kTrue : Adesk::kFalse), L"AcDbPlotSettingsValidator::setPlotCentered");
		}, py::arg("ps"), py::arg("centered"))
		.def("setPlotType", [](PyDbPlotSettingsValidator& self, PyDbPlotSettings& ps, int plotType) {
			if (!self.impObj() || !ps.impObj()) return false;
			return PyWarnIfError(self.impObj()->setPlotType(ps.impObj(), static_cast<PlotType>(plotType)),
				L"AcDbPlotSettingsValidator::setPlotType");
		}, py::arg("ps"), py::arg("plotType"))
		.def("setPlotWindowArea", [](PyDbPlotSettingsValidator& self, PyDbPlotSettings& ps,
				double xmin, double ymin, double xmax, double ymax) {
			if (!self.impObj() || !ps.impObj()) return false;
			return PyWarnIfError(self.impObj()->setPlotWindowArea(ps.impObj(), xmin, ymin, xmax, ymax),
				L"AcDbPlotSettingsValidator::setPlotWindowArea");
		}, py::arg("ps"), py::arg("xmin"), py::arg("ymin"), py::arg("xmax"), py::arg("ymax"))
		.def("setPlotViewName", [](PyDbPlotSettingsValidator& self, PyDbPlotSettings& ps,
				const AcString& viewName) {
			if (!self.impObj() || !ps.impObj()) return false;
			return PyWarnIfError(self.impObj()->setPlotViewName(ps.impObj(), viewName.kwszPtr()),
				L"AcDbPlotSettingsValidator::setPlotViewName");
		}, py::arg("ps"), py::arg("viewName"))
		.def("setUseStandardScale", [](PyDbPlotSettingsValidator& self, PyDbPlotSettings& ps, bool useStandard) {
			if (!self.impObj() || !ps.impObj()) return false;
			return PyWarnIfError(self.impObj()->setUseStandardScale(ps.impObj(),
				useStandard ? Adesk::kTrue : Adesk::kFalse), L"AcDbPlotSettingsValidator::setUseStandardScale");
		}, py::arg("ps"), py::arg("useStandard"))
		.def("setCustomPrintScale", [](PyDbPlotSettingsValidator& self, PyDbPlotSettings& ps,
				double numerator, double denominator) {
			if (!self.impObj() || !ps.impObj()) return false;
			return PyWarnIfError(self.impObj()->setCustomPrintScale(ps.impObj(), numerator, denominator),
				L"AcDbPlotSettingsValidator::setCustomPrintScale");
		}, py::arg("ps"), py::arg("numerator"), py::arg("denominator"))
		.def("setCurrentStyleSheet", [](PyDbPlotSettingsValidator& self, PyDbPlotSettings& ps,
				const AcString& styleSheetName) {
			if (!self.impObj() || !ps.impObj()) return false;
			return PyWarnIfError(self.impObj()->setCurrentStyleSheet(ps.impObj(), styleSheetName.kwszPtr()),
				L"AcDbPlotSettingsValidator::setCurrentStyleSheet");
		}, py::arg("ps"), py::arg("styleSheetName"))
		.def("setStdScaleType", [](PyDbPlotSettingsValidator& self, PyDbPlotSettings& ps, int scaleType) {
			if (!self.impObj() || !ps.impObj()) return false;
			return PyWarnIfError(self.impObj()->setStdScaleType(ps.impObj(), static_cast<StdScaleType>(scaleType)),
				L"AcDbPlotSettingsValidator::setStdScaleType");
		}, py::arg("ps"), py::arg("scaleType"))
		.def("setStdScale", [](PyDbPlotSettingsValidator& self, PyDbPlotSettings& ps, double scale) {
			if (!self.impObj() || !ps.impObj()) return false;
			return PyWarnIfError(self.impObj()->setStdScale(ps.impObj(), scale),
				L"AcDbPlotSettingsValidator::setStdScale");
		}, py::arg("ps"), py::arg("scale"))
		.def("setDefaultPlotConfig", [](PyDbPlotSettingsValidator& self, PyDbPlotSettings& ps) {
			if (!self.impObj() || !ps.impObj()) return false;
			return PyWarnIfError(self.impObj()->setDefaultPlotConfig(ps.impObj()),
				L"AcDbPlotSettingsValidator::setDefaultPlotConfig");
		}, py::arg("ps"))
		.def("setZoomToPaperOnUpdate", [](PyDbPlotSettingsValidator& self, PyDbPlotSettings& ps, bool doZoom) {
			if (!self.impObj() || !ps.impObj()) return;
			self.impObj()->setZoomToPaperOnUpdate(ps.impObj(), doZoom ? Adesk::kTrue : Adesk::kFalse);
		}, py::arg("ps"), py::arg("doZoom"))
		;
}
