#pragma once
#include "stdafx.h"
#include "PyDbObject.h"

class PyDbPlotSettings : public PyDbObject {
public:
	PyWrapper_H(DbPlotSettings)
	PyDbPlotSettings(AcDbPlotSettings* ptr) : PyDbObject(ptr) {}
	PyDbPlotSettings() : PyDbPlotSettings(new AcDbPlotSettings()) {}

	AcString getPlotSettingsName() const;
	bool setPlotSettingsName(const AcString& name);
	AcString getCanonicalMediaName() const;
	int plotPaperUnits() const;
	bool plotViewportBorders() const;
	bool setPlotViewportBorders(bool on);
	bool plotTransparency() const;
	bool setPlotTransparency(bool on);
	bool plotPlotStyles() const;
	bool setPlotPlotStyles(bool on);
	bool showPlotStyles() const;
	bool setShowPlotStyles(bool on);
	int plotRotation() const;
	bool plotCentered() const;
	bool plotHidden() const;
	bool setPlotHidden(bool on);
	int shadePlot() const;
	bool setShadePlot(int type);
	int shadePlotResLevel() const;
	bool setShadePlotResLevel(int level);
	int shadePlotCustomDPI() const;
	bool setShadePlotCustomDPI(int dpi);
	int plotType() const;
	int stdScaleType() const;
	bool scaleLineweights() const;
	bool setScaleLineweights(bool on);
	bool printLineweights() const;
	bool setPrintLineweights(bool on);
	bool drawViewportsFirst() const;
	bool setDrawViewportsFirst(bool on);
	bool modelType() const;
	bool plotWireframe() const;
	bool plotAsRaster() const;

	// ---- 只读页面参数（out 参数包装为 tuple / str / double）----
	py::tuple getPlotPaperSize() const;
	py::tuple getPlotOrigin() const;
	py::tuple getCustomPrintScale() const;
	AcString getPlotViewName() const;
	AcString getCurrentStyleSheet() const;
	double getStdScale() const;
	bool useStandardScale() const;
	AcString getPlotCfgName() const;

	// ---- Skeleton (not bound; multi-out/object/stream types need review) ----
	// getPlotPaperMargins/WindowArea (4x double out), shadePlotId(ObjectId),
	// getStyleSheetReadStream(IAcReadStream*), media/plotType/rotation/centered setters,
	// copyFrom(AcDbPlotSettings*).
};

void bind_PyDbPlotSettings(pym& m);
