#include "stdafx.h"
#include "PyAdsLineEnums.h"
#include "acdb.h"
void bind_PyLineWeight(pym& m)
{
	py::enum_<AcDb::LineWeight>(m, "LineWeight")
		.value("kLnWt000", AcDb::LineWeight::kLnWt000)
		.value("kLnWt005", AcDb::LineWeight::kLnWt005)
		.value("kLnWt009", AcDb::LineWeight::kLnWt009)
		.value("kLnWt013", AcDb::LineWeight::kLnWt013)
		.value("kLnWt015", AcDb::LineWeight::kLnWt015)
		.value("kLnWt018", AcDb::LineWeight::kLnWt018)
		.value("kLnWt020", AcDb::LineWeight::kLnWt020)
		.value("kLnWt025", AcDb::LineWeight::kLnWt025)
		.value("kLnWt030", AcDb::LineWeight::kLnWt030)
		.value("kLnWt035", AcDb::LineWeight::kLnWt035)
		.value("kLnWt040", AcDb::LineWeight::kLnWt040)
		.value("kLnWt050", AcDb::LineWeight::kLnWt050)
		.value("kLnWt053", AcDb::LineWeight::kLnWt053)
		.value("kLnWt060", AcDb::LineWeight::kLnWt060)
		.value("kLnWt070", AcDb::LineWeight::kLnWt070)
		.value("kLnWt080", AcDb::LineWeight::kLnWt080)
		.value("kLnWt090", AcDb::LineWeight::kLnWt090)
		.value("kLnWt100", AcDb::LineWeight::kLnWt100)
		.value("kLnWt106", AcDb::LineWeight::kLnWt106)
		.value("kLnWt120", AcDb::LineWeight::kLnWt120)
		.value("kLnWt140", AcDb::LineWeight::kLnWt140)
		.value("kLnWt158", AcDb::LineWeight::kLnWt158)
		.value("kLnWt200", AcDb::LineWeight::kLnWt200)
		.value("kLnWt211", AcDb::LineWeight::kLnWt211)
		.value("kLnWtByLayer", AcDb::LineWeight::kLnWtByLayer)
		.value("kLnWtByBlock", AcDb::LineWeight::kLnWtByBlock)
		.value("kLnWtByLwDefault", AcDb::LineWeight::kLnWtByLwDefault)
		.export_values();
}


void bind_PyPlotStyleNameType(pym& m) {
	py::enum_<AcDb::PlotStyleNameType>(m, "PlotStyleNameType")
		.value("kPlotStyleNameByLayer", AcDb::PlotStyleNameType::kPlotStyleNameByLayer)
		.value("kPlotStyleNameByBlock", AcDb::PlotStyleNameType::kPlotStyleNameByBlock)
		.value("kPlotStyleNameIsDictDefault", AcDb::PlotStyleNameType::kPlotStyleNameIsDictDefault)
		.value("kPlotStyleNameById", AcDb::PlotStyleNameType::kPlotStyleNameById)
		.export_values();
}


void bind_PyEndCaps(pym& m) {
	py::enum_<AcDb::EndCaps>(m, "EndCaps")
		.value("kEndCapNone", AcDb::EndCaps::kEndCapNone)
		.value("kEndCapRound", AcDb::EndCaps::kEndCapRound)
		.value("kEndCapAngle", AcDb::EndCaps::kEndCapAngle)
		.value("kEndCapSquare", AcDb::EndCaps::kEndCapSquare)
		.export_values();
}


void bind_PyUnitsValue(pym& m) {
	py::enum_<AcDb::UnitsValue>(m, "UnitsValue")
		.value("kUnitsUndefined", AcDb::UnitsValue::kUnitsUndefined)
		.value("kUnitsInches", AcDb::UnitsValue::kUnitsInches)
		.value("kUnitsFeet", AcDb::UnitsValue::kUnitsFeet)
		.value("kUnitsMiles", AcDb::UnitsValue::kUnitsMiles)
		.value("kUnitsMillimeters", AcDb::UnitsValue::kUnitsMillimeters)
		.value("kUnitsCentimeters", AcDb::UnitsValue::kUnitsCentimeters)
		.value("kUnitsMeters", AcDb::UnitsValue::kUnitsMeters)
		.value("kUnitsKilometers", AcDb::UnitsValue::kUnitsKilometers)
		.value("kUnitsMicroinches", AcDb::UnitsValue::kUnitsMicroinches)
		.value("kUnitsMils", AcDb::UnitsValue::kUnitsMils)
		.value("kUnitsYards", AcDb::UnitsValue::kUnitsYards)
		.value("kUnitsAngstroms", AcDb::UnitsValue::kUnitsAngstroms)
		.value("kUnitsNanometers", AcDb::UnitsValue::kUnitsNanometers)
		.value("kUnitsMicrons", AcDb::UnitsValue::kUnitsMicrons)
		.value("kUnitsDecimeters", AcDb::UnitsValue::kUnitsDecimeters)
		.value("kUnitsDekameters", AcDb::UnitsValue::kUnitsDekameters)
		.value("kUnitsHectometers", AcDb::UnitsValue::kUnitsHectometers)
		.value("kUnitsGigameters", AcDb::UnitsValue::kUnitsGigameters)
		.value("kUnitsAstronomical", AcDb::UnitsValue::kUnitsAstronomical)
		.value("kUnitsLightYears", AcDb::UnitsValue::kUnitsLightYears)
		.value("kUnitsParsecs", AcDb::UnitsValue::kUnitsParsecs)
		.value("kUnitsUSSurveyFeet", AcDb::UnitsValue::kUnitsUSSurveyFeet)
		.value("kUnitsUSSurveyInch", AcDb::UnitsValue::kUnitsUSSurveyInch)
		.value("kUnitsUSSurveyYard", AcDb::UnitsValue::kUnitsUSSurveyYard)
		.value("kUnitsUSSurveyMile", AcDb::UnitsValue::kUnitsUSSurveyMile)
		.value("kUnitsMax", AcDb::UnitsValue::kUnitsMax)
		.export_values();
}


void bind_PyJoinStyle(pym& m) {
	py::enum_<AcDb::JoinStyle>(m, "JoinStyle")
		.value("kJnStylNone", AcDb::JoinStyle::kJnStylNone)
		.value("kJnStylRound", AcDb::JoinStyle::kJnStylRound)
		.value("kJnStylAngle", AcDb::JoinStyle::kJnStylAngle)
		.value("kJnStylFlat", AcDb::JoinStyle::kJnStylFlat)
		.export_values();
}


void bind_PyMeasurementValue(pym& m) {
	py::enum_<AcDb::MeasurementValue>(m, "MeasurementValue")
		.value("kEnglish", AcDb::MeasurementValue::kEnglish)
		.value("kMetric", AcDb::MeasurementValue::kMetric)
		.export_values()
		;
}
