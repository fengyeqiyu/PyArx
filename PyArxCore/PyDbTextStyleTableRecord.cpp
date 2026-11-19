#include "stdafx.h"
#include "PyErrorStatusException.h"
#include "PyDbTextStyleTableRecord.h"

PyWrapper_Cpp(DbTextStyleTableRecord);

void bind_PyDbTextStyleTableRecord(pym& m) {
	py::class_<PyDbTextStyleTableRecord, PyDbSymbolTableRecord>(m, "PyDbTextStyleTableRecord")
		.def(py::init<>())
		.def(py::init<AcDbTextStyleTableRecord*>(), py::arg("p"))
		PyWrapper_BIND(DbTextStyleTableRecord)
		// ---- isShapeFile ----
		.def("isShapeFile", [](PyDbTextStyleTableRecord& s) { return s.impObj()->isShapeFile() == Adesk::kTrue; })
		.def("setIsShapeFile", [](PyDbTextStyleTableRecord& s, bool v) {
			s.impObj()->setIsShapeFile(v ? Adesk::kTrue : Adesk::kFalse); return true;
		}, py::arg("v"))
		// ---- isVertical ----
		.def("isVertical", [](PyDbTextStyleTableRecord& s) { return s.impObj()->isVertical() == Adesk::kTrue; })
		.def("setIsVertical", [](PyDbTextStyleTableRecord& s, bool v) {
			s.impObj()->setIsVertical(v ? Adesk::kTrue : Adesk::kFalse); return true;
		}, py::arg("v"))
		// ---- textSize ----
		.def("textSize", [](PyDbTextStyleTableRecord& s) { return s.impObj()->textSize(); })
		.def("setTextSize", [](PyDbTextStyleTableRecord& s, double v) {
			return PyWarnIfError(s.impObj()->setTextSize(v), L"AcDbTextStyleTableRecord::setTextSize");
		}, py::arg("v"))
		// ---- xScale ----
		.def("xScale", [](PyDbTextStyleTableRecord& s) { return s.impObj()->xScale(); })
		.def("setXScale", [](PyDbTextStyleTableRecord& s, double v) {
			return PyWarnIfError(s.impObj()->setXScale(v), L"AcDbTextStyleTableRecord::setXScale");
		}, py::arg("v"))
		// ---- obliquingAngle ----
		.def("obliquingAngle", [](PyDbTextStyleTableRecord& s) { return s.impObj()->obliquingAngle(); })
		.def("setObliquingAngle", [](PyDbTextStyleTableRecord& s, double v) {
			return PyWarnIfError(s.impObj()->setObliquingAngle(v), L"AcDbTextStyleTableRecord::setObliquingAngle");
		}, py::arg("v"))
		// ---- flagBits（Adesk::UInt8 以 int 暴露） ----
		.def("flagBits", [](PyDbTextStyleTableRecord& s) { return static_cast<int>(s.impObj()->flagBits()); })
		.def("setFlagBits", [](PyDbTextStyleTableRecord& s, int v) {
			s.impObj()->setFlagBits(static_cast<Adesk::UInt8>(v)); return true;
		}, py::arg("v"))
		// ---- priorSize ----
		.def("priorSize", [](PyDbTextStyleTableRecord& s) { return s.impObj()->priorSize(); })
		.def("setPriorSize", [](PyDbTextStyleTableRecord& s, double v) {
			return PyWarnIfError(s.impObj()->setPriorSize(v), L"AcDbTextStyleTableRecord::setPriorSize");
		}, py::arg("v"))
		// ---- fileName ----
		.def("fileName", [](PyDbTextStyleTableRecord& s) -> AcString {
			AcString sOut;
			if (!PyWarnIfError(s.impObj()->fileName(sOut), L"AcDbTextStyleTableRecord::fileName")) return AcString();
			return sOut;
		})
		.def("setFileName", [](PyDbTextStyleTableRecord& s, const AcString& v) {
			return PyWarnIfError(s.impObj()->setFileName(v.kwszPtr()), L"AcDbTextStyleTableRecord::setFileName");
		}, py::arg("v"))
		// ---- bigFontFileName ----
		.def("bigFontFileName", [](PyDbTextStyleTableRecord& s) -> AcString {
			AcString sOut;
			if (!PyWarnIfError(s.impObj()->bigFontFileName(sOut), L"AcDbTextStyleTableRecord::bigFontFileName")) return AcString();
			return sOut;
		})
		.def("setBigFontFileName", [](PyDbTextStyleTableRecord& s, const AcString& v) {
			return PyWarnIfError(s.impObj()->setBigFontFileName(v.kwszPtr()), L"AcDbTextStyleTableRecord::setBigFontFileName");
		}, py::arg("v"))
		// ---- setFont（PAL FontUtils 枚举以 int 传入，内部 static_cast） ----
		.def("setFont", [](PyDbTextStyleTableRecord& s, const AcString& typeface, bool bold, bool italic,
				int charset, int pitch, int family, bool bAllowMissingFont) {
			return PyWarnIfError(s.impObj()->setFont(typeface.kwszPtr(),
				bold ? Adesk::kTrue : Adesk::kFalse,
				italic ? Adesk::kTrue : Adesk::kFalse,
				static_cast<Charset>(charset),
				static_cast<Autodesk::AutoCAD::PAL::FontUtils::FontPitch>(pitch),
				static_cast<Autodesk::AutoCAD::PAL::FontUtils::FontFamily>(family),
				bAllowMissingFont), L"AcDbTextStyleTableRecord::setFont");
		}, py::arg("typeface"), py::arg("bold"), py::arg("italic"),
			py::arg("charset"), py::arg("pitch"), py::arg("family"),
			py::arg("bAllowMissingFont") = false,
			"typeface: 字体名; charset: Charset 枚举整数值; pitch: 0=kDefault 1=kFixed 2=kVariable; family: 0=DoNotCare 16=Roman 32=Swiss 48=Modern 64=Script 80=Decorative")
		// ---- font（getter，出参打包为 tuple） ----
		.def("font", [](PyDbTextStyleTableRecord& s) -> std::tuple<AcString, bool, bool, int, int, int> {
			AcString sTypeface; bool bold = false, italic = false;
			Charset charset = kUndefinedCharset;
			Autodesk::AutoCAD::PAL::FontUtils::FontPitch pitch = Autodesk::AutoCAD::PAL::FontUtils::FontPitch::kDefault;
			Autodesk::AutoCAD::PAL::FontUtils::FontFamily family = Autodesk::AutoCAD::PAL::FontUtils::FontFamily::kDoNotCare;
			if (!PyWarnIfError(s.impObj()->font(sTypeface, bold, italic, charset, pitch, family), L"AcDbTextStyleTableRecord::font")) {
				return std::make_tuple(AcString(), false, false, 0, 0, 0);
			}
			return std::make_tuple(sTypeface, bold, italic,
				static_cast<int>(charset), static_cast<int>(pitch), static_cast<int>(family));
		}, "返回 (typeface, bold, italic, charset, pitch, family) 元组")
		;
}
