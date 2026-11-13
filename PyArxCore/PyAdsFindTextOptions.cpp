#include "stdafx.h"
#include "PyAdsFindTextOptions.h"
#include "PyDbDatabase.h" // for FindTextOptions
void bind_FindTextOptions(pym& m) {
	py::enum_<FindTextOptions>(m, "FindTextOptions")
		.value("kSearchBlock", FindTextOptions::kSearchBlock)
		.value("kSearchDimText", FindTextOptions::kSearchDimText)
		.value("kSearchText", FindTextOptions::kSearchText)
		.value("kSearchLinkDesc", FindTextOptions::kSearchLinkDesc)
		.value("kSearchLinkUrl", FindTextOptions::kSearchLinkUrl)
		.value("kSearchMatchCase", FindTextOptions::kSearchMatchCase)
		.value("kSearchWholeWord", FindTextOptions::kSearchWholeWord)
		.value("kSearchDefault", FindTextOptions::kSearchDefault)
		.export_values();
}
