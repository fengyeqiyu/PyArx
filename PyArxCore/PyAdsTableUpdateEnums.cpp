#include "stdafx.h"
#include "PyAdsTableUpdateEnums.h"
#include "acdb.h"
void bind_PyUpdateOption(pym& m) {
	py::enum_<AcDb::UpdateOption>(m, "UpdateOption")
		.value("kUpdateOptionNone", AcDb::UpdateOption::kUpdateOptionNone)
		.value("kUpdateOptionSkipFormat", AcDb::UpdateOption::kUpdateOptionSkipFormat)
		.value("kUpdateOptionUpdateRowHeight", AcDb::UpdateOption::kUpdateOptionUpdateRowHeight)
		.value("kUpdateOptionUpdateColumnWidth", AcDb::UpdateOption::kUpdateOptionUpdateColumnWidth)
		.value("kUpdateOptionAllowSourceUpdate", AcDb::UpdateOption::kUpdateOptionAllowSourceUpdate)
		.value("kUpdateOptionForceFullSourceUpdate", AcDb::UpdateOption::kUpdateOptionForceFullSourceUpdate)
		.value("kUpdateOptionOverwriteContentModifiedAfterUpdate", AcDb::UpdateOption::kUpdateOptionOverwriteContentModifiedAfterUpdate)
		.value("kUpdateOptionOverwriteFormatModifiedAfterUpdate", AcDb::UpdateOption::kUpdateOptionOverwriteFormatModifiedAfterUpdate)
		.value("kUpdateOptionForPreview", AcDb::UpdateOption::kUpdateOptionForPreview)
		.value("kUpdateOptionIncludeXrefs", AcDb::UpdateOption::kUpdateOptionIncludeXrefs)
		.value("kSkipFormatAfterFirstUpdate", AcDb::UpdateOption::kSkipFormatAfterFirstUpdate)
		.export_values()
		;
}


void bind_PyUpdateDirection(pym& m) {
	py::enum_<AcDb::UpdateDirection>(m,"UpdateDirection")
		.value("kUpdateDirSourceToData", AcDb::UpdateDirection::kUpdateDirSourceToData)
		.value("kUpdateDirDataToSource", AcDb::UpdateDirection::kUpdateDirDataToSource)
		.export_values()
		;
}
