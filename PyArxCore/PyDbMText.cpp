#include "stdafx.h"
#include "PyErrorStatusException.h"
#include "PyDbMText.h"

PyWrapper_Cpp(DbMText)
AcString PyDbMText::toString() const { return AcString(std::format("AcDbMText[wrapper]").c_str(), AcString::Utf8); }
AcGePoint3d PyDbMText::location() const { if (!impObj()) return AcGePoint3d(); return impObj()->location(); }

bool  PyDbMText::setLocation(const AcGePoint3d& p) { if (!impObj()) return false; return PyWarnIfError(impObj()->setLocation(p), L"AcDbMText::setLocation"); }

double PyDbMText::textHeight() const { if (!impObj()) return 0.0; return impObj()->textHeight(); }

bool  PyDbMText::setTextHeight(double h) { if (!impObj()) return false; return PyWarnIfError(impObj()->setTextHeight(h), L"AcDbMText::setTextHeight"); }

double PyDbMText::rotation() const { if (!impObj()) return 0.0; return impObj()->rotation(); }

bool  PyDbMText::setRotation(double r) { if (!impObj()) return false; return PyWarnIfError(impObj()->setRotation(r), L"AcDbMText::setRotation"); }

AcString PyDbMText::contents() const { if (!impObj()) return AcString(); AcString s; if(!PyWarnIfError(impObj()->contents(s), L"AcDbMText::contents")) return AcString(); return s; }

bool PyDbMText::setContents(const AcString& s) {
	if (!impObj()) return false;
	int rc = impObj()->setContents(s.kwszPtr());
	if (rc != 0) { acutPrintf(L"[PyArx] AcDbMText::setContents failed (e%d)\n", rc); return false; }
	return true;
}

AcGeVector3d PyDbMText::normal() const { if (!impObj()) return AcGeVector3d(); return impObj()->normal(); }
bool  PyDbMText::setNormal(const AcGeVector3d& n) { if (!impObj()) return false; return PyWarnIfError(impObj()->setNormal(n), L"AcDbMText::setNormal"); }
AcGeVector3d PyDbMText::direction() const { if (!impObj()) return AcGeVector3d(); return impObj()->direction(); }
bool  PyDbMText::setDirection(const AcGeVector3d& v) { if (!impObj()) return false; return PyWarnIfError(impObj()->setDirection(v), L"AcDbMText::setDirection"); }
double PyDbMText::width() const { if (!impObj()) return 0.0; return impObj()->width(); }
bool  PyDbMText::setWidth(double v) { if (!impObj()) return false; return PyWarnIfError(impObj()->setWidth(v), L"AcDbMText::setWidth"); }
double PyDbMText::ascent() const { if (!impObj()) return 0.0; return impObj()->ascent(); }
double PyDbMText::descent() const { if (!impObj()) return 0.0; return impObj()->descent(); }
int PyDbMText::attachment() const { if (!impObj()) return 0; return static_cast<int>(impObj()->attachment()); }
bool  PyDbMText::setAttachment(int v) { if (!impObj()) return false; return PyWarnIfError(impObj()->setAttachment(static_cast<AcDbMText::AttachmentPoint>(v)), L"AcDbMText::setAttachment"); }
int PyDbMText::flowDirection() const { if (!impObj()) return 0; return static_cast<int>(impObj()->flowDirection()); }
bool  PyDbMText::setFlowDirection(int v) { if (!impObj()) return false; return PyWarnIfError(impObj()->setFlowDirection(static_cast<AcDbMText::FlowDirection>(v)), L"AcDbMText::setFlowDirection"); }
AcString PyDbMText::contentsRTF() const { if (!impObj()) return AcString(); AcString s; if(!PyWarnIfError(impObj()->contentsRTF(s), L"AcDbMText::contentsRTF")) return AcString(); return s; }
bool PyDbMText::setContentsRTF(const AcString& s) {
	if (!impObj()) return false;
	int rc = impObj()->setContentsRTF(s.kwszPtr());
	if (rc != 0) { acutPrintf(L"[PyArx] AcDbMText::setContentsRTF failed (e%d)\n", rc); return false; }
	return true;
}
AcString PyDbMText::text() const { if (!impObj()) return AcString(); AcString s; if(!PyWarnIfError(impObj()->text(s), L"AcDbMText::text")) return AcString(); return s; }
double PyDbMText::actualHeight() const { if (!impObj()) return 0.0; return impObj()->actualHeight(); }
double PyDbMText::actualWidth() const { if (!impObj()) return 0.0; return impObj()->actualWidth(); }
int PyDbMText::lineSpacingStyle() const { if (!impObj()) return 0; return static_cast<int>(impObj()->lineSpacingStyle()); }
bool  PyDbMText::setLineSpacingStyle(int v) { if (!impObj()) return false; return PyWarnIfError(impObj()->setLineSpacingStyle(static_cast<AcDb::LineSpacingStyle>(v)), L"AcDbMText::setLineSpacingStyle"); }
double PyDbMText::lineSpacingFactor() const { if (!impObj()) return 0.0; return impObj()->lineSpacingFactor(); }
bool  PyDbMText::setLineSpacingFactor(double v) { if (!impObj()) return false; return PyWarnIfError(impObj()->setLineSpacingFactor(v), L"AcDbMText::setLineSpacingFactor"); }
bool PyDbMText::showBorders() const { if (!impObj()) return false; return impObj()->showBorders(); }
bool  PyDbMText::setShowBorders(bool v) { if (!impObj()) return false; return PyWarnIfError(impObj()->setShowBorders(v), L"AcDbMText::setShowBorders"); }
bool PyDbMText::backgroundFillOn() const { if (!impObj()) return false; return impObj()->backgroundFillOn(); }
bool  PyDbMText::setBackgroundFill(bool v) { if (!impObj()) return false; return PyWarnIfError(impObj()->setBackgroundFill(v), L"AcDbMText::setBackgroundFill"); }
PyColor PyDbMText::backgroundFillColor() const {
	AcCmColor c;
	PyWarnIfError(impObj()->getBackgroundFillColor(c), L"AcDbMText::getBackgroundFillColor");
	return PyColor(c);
}
bool  PyDbMText::setBackgroundFillColor(const PyColor& color) {
	if (!impObj()) return false;
	return PyWarnIfError(impObj()->setBackgroundFillColor(color.m_color), L"AcDbMText::setBackgroundFillColor");
}
bool PyDbMText::useBackgroundColorOn() const { if (!impObj()) return false; return impObj()->useBackgroundColorOn(); }
bool  PyDbMText::setUseBackgroundColor(bool v) { if (!impObj()) return false; return PyWarnIfError(impObj()->setUseBackgroundColor(v), L"AcDbMText::setUseBackgroundColor"); }
bool PyDbMText::useWordBreak() const { if (!impObj()) return false; return impObj()->useWordBreak(); }
bool  PyDbMText::setUseWordBreak(bool v) { if (!impObj()) return false; return PyWarnIfError(impObj()->setUseWordBreak(v), L"AcDbMText::setUseWordBreak"); }
bool PyDbMText::hitTest(const AcGePoint3d& pt) const { if (!impObj()) return false; return impObj()->hitTest(pt); }
int PyDbMText::correctSpelling() { if (!impObj()) return 0; return impObj()->correctSpelling(); }

double PyDbMText::height() const { if (!impObj()) return 0.0; return impObj()->height(); }
bool  PyDbMText::setHeight(double h) { if (!impObj()) return false; return PyWarnIfError(impObj()->setHeight(h), L"AcDbMText::setHeight"); }

uint64_t PyDbMText::textStyle() const { if (!impObj()) return 0ULL; return static_cast<uint64_t>(impObj()->textStyle().asOldId()); }
bool  PyDbMText::setTextStyle(uint64_t id) {
	if (!impObj()) return false;
	AcDbObjectId styleId; styleId.setFromOldId(static_cast<Adesk::IntDbId>(id));
	return PyWarnIfError(impObj()->setTextStyle(styleId), L"AcDbMText::setTextStyle");
}

std::vector<AcGePoint3d> PyDbMText::getBoundingPoints() const {
	AcGePoint3dArray arr;
	impObj()->getBoundingPoints(arr);
	std::vector<AcGePoint3d> out;
	out.reserve(arr.length());
	for (int i = 0; i < arr.length(); ++i) out.push_back(arr[i]);
	return out;
}

std::vector<std::vector<AcGePoint3d>> PyDbMText::getBoundingPointsByLine() const {
	AcArray<AcGePoint3dArray> lines;
	impObj()->getBoundingPointsByLine(lines);
	std::vector<std::vector<AcGePoint3d>> out;
	out.reserve(lines.length());
	for (int i = 0; i < lines.length(); ++i) {
		const AcGePoint3dArray& arr = lines[i];
		std::vector<AcGePoint3d> row;
		row.reserve(arr.length());
		for (int j = 0; j < arr.length(); ++j) row.push_back(arr[j]);
		out.push_back(std::move(row));
	}
	return out;
}

bool  PyDbMText::setAttachmentMovingLocation(int v) { if (!impObj()) return false; return PyWarnIfError(impObj()->setAttachmentMovingLocation(static_cast<AcDbMText::AttachmentPoint>(v)), L"AcDbMText::setAttachmentMovingLocation"); }

py::tuple PyDbMText::getPlane() const {
	if (!impObj()) return py::tuple();
	AcGePlane plane; AcDb::Planarity type;
	if (!PyWarnIfError(impObj()->getPlane(plane, type), L"AcDbMText::getPlane"))
		return py::make_tuple(AcGePlane(), 0);
	return py::make_tuple(plane, static_cast<int>(type));
}

bool PyDbMText::isPlanar() const { if (!impObj()) return false; return impObj()->isPlanar() != Adesk::kFalse; }

// ---- explodeFragments: C enumerator callback bridged to a Python callable ----
namespace {
	struct PyDbMTextEnumCtx {
		py::object cb;
	};

	int PyDbMTextFragmentTrampoline(AcDbMTextFragment* frag, void* param) {
		if (!frag || !param) return 0;
		auto* ctx = static_cast<PyDbMTextEnumCtx*>(param);
		int rc = 1;
		try {
			py::gil_scoped_acquire gil;
			PyDbMTextFragmentView v;
			v.location = frag->location;
			v.normal = frag->normal;
			v.direction = frag->direction;
			v.text = frag->msText;
			v.font = frag->msFont;
			v.bigFont = frag->msBigFont;
			v.extents = frag->extents;
			v.capsHeight = frag->capsHeight;
			v.widthFactor = frag->widthFactor;
			v.obliqueAngle = frag->obliqueAngle;
			v.trackingFactor = frag->trackingFactor;
			v.colorRGB = std::make_tuple((int)frag->color.red(), (int)frag->color.green(), (int)frag->color.blue());
			v.stackTop = frag->stackTop != 0;
			v.stackBottom = frag->stackBottom != 0;
			v.underlined = frag->underlined != 0;
			v.overlined = frag->overlined != 0;
			v.strikethrough = frag->strikethrough != 0;
			v.underPoint1 = frag->underPoints[0];
			v.underPoint2 = frag->underPoints[1];
			v.overPoint1 = frag->overPoints[0];
			v.overPoint2 = frag->overPoints[1];
			v.strikePoint1 = frag->strikePoints[0];
			v.strikePoint2 = frag->strikePoints[1];
			v.fontName = frag->msFontName;
			v.bold = frag->bold;
			v.italic = frag->italic;
			py::object ret = ctx->cb(v);
			if (!ret.is_none()) {
				try { rc = ret.cast<int>(); }
				catch (...) { rc = 1; } // non-int return value: keep enumerating
			}
		} catch (const py::error_already_set&) {
			if (PyErr_Occurred()) PyErr_Clear();
			rc = 0; // stop on Python exception, swallow it (never into ObjectARX)
		} catch (...) {
			if (PyErr_Occurred()) PyErr_Clear();
			rc = 0;
		}
		return rc;
	}
}

void PyDbMText::explodeFragments(const py::object& callback) {
	if (!impObj()) return;
	auto* ctx = new PyDbMTextEnumCtx{callback};
	struct CtxGuard { PyDbMTextEnumCtx* c; ~CtxGuard() { delete c; } } guard{ctx};
	impObj()->explodeFragments(&PyDbMTextFragmentTrampoline, ctx, nullptr);
}

void bind_PyDbMText(pym& m) {
	py::class_<PyDbMTextFragmentView>(m, "PyDbMTextFragment")
		.def_readonly("location", &PyDbMTextFragmentView::location)
		.def_readonly("normal", &PyDbMTextFragmentView::normal)
		.def_readonly("direction", &PyDbMTextFragmentView::direction)
		.def_readonly("text", &PyDbMTextFragmentView::text)
		.def_readonly("font", &PyDbMTextFragmentView::font)
		.def_readonly("bigFont", &PyDbMTextFragmentView::bigFont)
		.def_readonly("extents", &PyDbMTextFragmentView::extents)
		.def_readonly("capsHeight", &PyDbMTextFragmentView::capsHeight)
		.def_readonly("widthFactor", &PyDbMTextFragmentView::widthFactor)
		.def_readonly("obliqueAngle", &PyDbMTextFragmentView::obliqueAngle)
		.def_readonly("trackingFactor", &PyDbMTextFragmentView::trackingFactor)
		.def_readonly("colorRGB", &PyDbMTextFragmentView::colorRGB)
		.def_readonly("stackTop", &PyDbMTextFragmentView::stackTop)
		.def_readonly("stackBottom", &PyDbMTextFragmentView::stackBottom)
		.def_readonly("underlined", &PyDbMTextFragmentView::underlined)
		.def_readonly("overlined", &PyDbMTextFragmentView::overlined)
		.def_readonly("strikethrough", &PyDbMTextFragmentView::strikethrough)
		.def_readonly("underPoint1", &PyDbMTextFragmentView::underPoint1)
		.def_readonly("underPoint2", &PyDbMTextFragmentView::underPoint2)
		.def_readonly("overPoint1", &PyDbMTextFragmentView::overPoint1)
		.def_readonly("overPoint2", &PyDbMTextFragmentView::overPoint2)
		.def_readonly("strikePoint1", &PyDbMTextFragmentView::strikePoint1)
		.def_readonly("strikePoint2", &PyDbMTextFragmentView::strikePoint2)
		.def_readonly("fontName", &PyDbMTextFragmentView::fontName)
		.def_readonly("bold", &PyDbMTextFragmentView::bold)
		.def_readonly("italic", &PyDbMTextFragmentView::italic)
		;

	py::class_<PyDbMText, PyDbEntity>(m, "PyDbMText")
		.def(py::init<>())
		.def(py::init<AcDbMText*>(), py::arg("object"))
		.def("location", &PyDbMText::location)
		.def("setLocation", &PyDbMText::setLocation, py::arg("p"))
		.def("normal", &PyDbMText::normal)
		.def("setNormal", &PyDbMText::setNormal, py::arg("n"))
		.def("direction", &PyDbMText::direction)
		.def("setDirection", &PyDbMText::setDirection, py::arg("v"))
		.def("rotation", &PyDbMText::rotation)
		.def("setRotation", &PyDbMText::setRotation, py::arg("r"))
		.def("width", &PyDbMText::width)
		.def("setWidth", &PyDbMText::setWidth, py::arg("v"))
		.def("ascent", &PyDbMText::ascent)
		.def("descent", &PyDbMText::descent)
		.def("textHeight", &PyDbMText::textHeight)
		.def("setTextHeight", &PyDbMText::setTextHeight, py::arg("h"))
		.def("attachment", &PyDbMText::attachment)
		.def("setAttachment", &PyDbMText::setAttachment, py::arg("v"))
		.def("flowDirection", &PyDbMText::flowDirection)
		.def("setFlowDirection", &PyDbMText::setFlowDirection, py::arg("v"))
		.def("contents", &PyDbMText::contents)
		.def("setContents", &PyDbMText::setContents, py::arg("text"))
		.def("contentsRTF", &PyDbMText::contentsRTF)
		.def("setContentsRTF", &PyDbMText::setContentsRTF, py::arg("rtf"))
		.def("text", &PyDbMText::text)
		.def("actualHeight", &PyDbMText::actualHeight)
		.def("actualWidth", &PyDbMText::actualWidth)
		.def("lineSpacingStyle", &PyDbMText::lineSpacingStyle)
		.def("setLineSpacingStyle", &PyDbMText::setLineSpacingStyle, py::arg("v"))
		.def("lineSpacingFactor", &PyDbMText::lineSpacingFactor)
		.def("setLineSpacingFactor", &PyDbMText::setLineSpacingFactor, py::arg("v"))
		.def("showBorders", &PyDbMText::showBorders)
		.def("setShowBorders", &PyDbMText::setShowBorders, py::arg("v"))
		.def("backgroundFillOn", &PyDbMText::backgroundFillOn)
		.def("setBackgroundFill", &PyDbMText::setBackgroundFill, py::arg("v"))
		.def("backgroundFillColor", &PyDbMText::backgroundFillColor, "背景填充颜色（Color 值对象）")
		.def("setBackgroundFillColor", &PyDbMText::setBackgroundFillColor, py::arg("color"), "设置背景填充颜色")
		.def("useBackgroundColorOn", &PyDbMText::useBackgroundColorOn)
		.def("setUseBackgroundColor", &PyDbMText::setUseBackgroundColor, py::arg("v"))
		.def("useWordBreak", &PyDbMText::useWordBreak)
		.def("setUseWordBreak", &PyDbMText::setUseWordBreak, py::arg("v"))
		.def("hitTest", &PyDbMText::hitTest, py::arg("pt"))
		.def("correctSpelling", &PyDbMText::correctSpelling)
		.def("height", &PyDbMText::height)
		.def("setHeight", &PyDbMText::setHeight, py::arg("h"))
		.def("textStyle", &PyDbMText::textStyle)
		.def("setTextStyle", &PyDbMText::setTextStyle, py::arg("styleId"))
		.def("getBoundingPoints", &PyDbMText::getBoundingPoints)
		.def("getBoundingPointsByLine", &PyDbMText::getBoundingPointsByLine)
		.def("setAttachmentMovingLocation", &PyDbMText::setAttachmentMovingLocation, py::arg("v"))
		.def("getPlane", &PyDbMText::getPlane)
		.def("isPlanar", &PyDbMText::isPlanar)
		.def("explodeFragments", &PyDbMText::explodeFragments, py::arg("callback"),
			"Enumerate text fragments; callback receives a PyDbMTextFragment and may return False to stop.")
		.def_static("nonBreakSpace", []() -> AcString { return AcString(AcDbMText::nonBreakSpace()); })
		.def_static("overlineOn", []() -> AcString { return AcString(AcDbMText::overlineOn()); })
		.def_static("overlineOff", []() -> AcString { return AcString(AcDbMText::overlineOff()); })
		.def_static("underlineOn", []() -> AcString { return AcString(AcDbMText::underlineOn()); })
		.def_static("underlineOff", []() -> AcString { return AcString(AcDbMText::underlineOff()); })
		PyWrapper_BIND(DbMText)
		.def("__repr__", &PyDbMText::toString)
		;
}

