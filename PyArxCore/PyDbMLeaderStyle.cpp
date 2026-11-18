#include "stdafx.h"
#include "PyErrorStatusException.h"
#include "PyDbMLeaderStyle.h"
#include "PyDbMText.h"
#include "PyDbDatabase.h"
#include "PyDbTypeCaster.h"

PyWrapper_Cpp(DbMLeaderStyle)
AcString PyDbMLeaderStyle::toString() const { return AcString(std::format("AcDbMLeaderStyle[wrapper]").c_str(), AcString::Utf8); }

// ---- 名称 / 描述 ----
AcString PyDbMLeaderStyle::getName() const {
	if (!impObj()) return AcString();
	AcString s;
	if (!PyWarnIfError(impObj()->getName(s), L"AcDbMLeaderStyle::getName")) return AcString();
	return s;
}
bool PyDbMLeaderStyle::setName(const AcString& n) {
	if (!impObj()) return false;
	return PyWarnIfError(impObj()->setName(n.kwszPtr()), L"AcDbMLeaderStyle::setName");
}
AcString PyDbMLeaderStyle::description() const {
	if (!impObj()) return AcString();
	const ACHAR* p = impObj()->description();
	return p ? AcString(p) : AcString();
}
bool PyDbMLeaderStyle::setDescription(const AcString& d) {
	if (!impObj()) return false;
	return PyWarnIfError(impObj()->setDescription(d.kwszPtr()), L"AcDbMLeaderStyle::setDescription");
}

// ---- 内容类型 ----
int PyDbMLeaderStyle::contentType() const {
	if (!impObj()) return 0;
	return static_cast<int>(impObj()->contentType());
}
bool PyDbMLeaderStyle::setContentType(int v) {
	if (!impObj()) return false;
	return PyWarnIfError(impObj()->setContentType(static_cast<AcDbMLeaderStyle::ContentType>(v)), L"AcDbMLeaderStyle::setContentType");
}

// ---- 引线 ----
int PyDbMLeaderStyle::maxLeaderSegmentsPoints() const { if (!impObj()) return 0; return impObj()->maxLeaderSegmentsPoints(); }
bool PyDbMLeaderStyle::setMaxLeaderSegmentsPoints(int v) {
	if (!impObj()) return false;
	return PyWarnIfError(impObj()->setMaxLeaderSegmentsPoints(v), L"AcDbMLeaderStyle::setMaxLeaderSegmentsPoints");
}
int PyDbMLeaderStyle::firstSegmentAngleConstraint() const {
	if (!impObj()) return 0;
	return static_cast<int>(impObj()->firstSegmentAngleConstraint());
}
bool PyDbMLeaderStyle::setFirstSegmentAngleConstraint(int v) {
	if (!impObj()) return false;
	return PyWarnIfError(impObj()->setFirstSegmentAngleConstraint(static_cast<AcDbMLeaderStyle::SegmentAngleType>(v)), L"AcDbMLeaderStyle::setFirstSegmentAngleConstraint");
}
int PyDbMLeaderStyle::secondSegmentAngleConstraint() const {
	if (!impObj()) return 0;
	return static_cast<int>(impObj()->secondSegmentAngleConstraint());
}
bool PyDbMLeaderStyle::setSecondSegmentAngleConstraint(int v) {
	if (!impObj()) return false;
	return PyWarnIfError(impObj()->setSecondSegmentAngleConstraint(static_cast<AcDbMLeaderStyle::SegmentAngleType>(v)), L"AcDbMLeaderStyle::setSecondSegmentAngleConstraint");
}
uint64_t PyDbMLeaderStyle::leaderLineTypeId() const {
	if (!impObj()) return 0ULL;
	return static_cast<uint64_t>(impObj()->leaderLineTypeId().asOldId());
}
bool PyDbMLeaderStyle::setLeaderLineTypeId(uint64_t id) {
	if (!impObj()) return false;
	AcDbObjectId oid; oid.setFromOldId(static_cast<Adesk::UIntPtr>(id));
	return PyWarnIfError(impObj()->setLeaderLineTypeId(oid), L"AcDbMLeaderStyle::setLeaderLineTypeId");
}
bool PyDbMLeaderStyle::enableLanding() const { if (!impObj()) return false; return impObj()->enableLanding(); }
bool PyDbMLeaderStyle::setEnableLanding(bool v) {
	if (!impObj()) return false;
	return PyWarnIfError(impObj()->setEnableLanding(v), L"AcDbMLeaderStyle::setEnableLanding");
}
double PyDbMLeaderStyle::landingGap() const { if (!impObj()) return 0.0; return impObj()->landingGap(); }
bool PyDbMLeaderStyle::setLandingGap(double v) {
	if (!impObj()) return false;
	return PyWarnIfError(impObj()->setLandingGap(v), L"AcDbMLeaderStyle::setLandingGap");
}
bool PyDbMLeaderStyle::enableDogleg() const { if (!impObj()) return false; return impObj()->enableDogleg(); }
bool PyDbMLeaderStyle::setEnableDogleg(bool v) {
	if (!impObj()) return false;
	return PyWarnIfError(impObj()->setEnableDogleg(v), L"AcDbMLeaderStyle::setEnableDogleg");
}
double PyDbMLeaderStyle::doglegLength() const { if (!impObj()) return 0.0; return impObj()->doglegLength(); }
bool PyDbMLeaderStyle::setDoglegLength(double v) {
	if (!impObj()) return false;
	return PyWarnIfError(impObj()->setDoglegLength(v), L"AcDbMLeaderStyle::setDoglegLength");
}

// ---- 箭头 ----
uint64_t PyDbMLeaderStyle::arrowSymbolId() const {
	if (!impObj()) return 0ULL;
	return static_cast<uint64_t>(impObj()->arrowSymbolId().asOldId());
}
bool PyDbMLeaderStyle::setArrowSymbolId(uint64_t id) {
	if (!impObj()) return false;
	AcDbObjectId oid; oid.setFromOldId(static_cast<Adesk::UIntPtr>(id));
	return PyWarnIfError(impObj()->setArrowSymbolId(oid), L"AcDbMLeaderStyle::setArrowSymbolId");
}
double PyDbMLeaderStyle::arrowSize() const { if (!impObj()) return 0.0; return impObj()->arrowSize(); }
bool PyDbMLeaderStyle::setArrowSize(double v) {
	if (!impObj()) return false;
	return PyWarnIfError(impObj()->setArrowSize(v), L"AcDbMLeaderStyle::setArrowSize");
}

// ---- 文字 ----
uint64_t PyDbMLeaderStyle::textStyleId() const {
	if (!impObj()) return 0ULL;
	return static_cast<uint64_t>(impObj()->textStyleId().asOldId());
}
bool PyDbMLeaderStyle::setTextStyleId(uint64_t id) {
	if (!impObj()) return false;
	AcDbObjectId oid; oid.setFromOldId(static_cast<Adesk::UIntPtr>(id));
	return PyWarnIfError(impObj()->setTextStyleId(oid), L"AcDbMLeaderStyle::setTextStyleId");
}
int PyDbMLeaderStyle::textAlignmentType() const {
	if (!impObj()) return 0;
	return static_cast<int>(impObj()->textAlignmentType());
}
bool PyDbMLeaderStyle::setTextAlignmentType(int v) {
	if (!impObj()) return false;
	return PyWarnIfError(impObj()->setTextAlignmentType(static_cast<AcDbMLeaderStyle::TextAlignmentType>(v)), L"AcDbMLeaderStyle::setTextAlignmentType");
}
bool PyDbMLeaderStyle::textAlignAlwaysLeft() const { if (!impObj()) return false; return impObj()->textAlignAlwaysLeft(); }
bool PyDbMLeaderStyle::setTextAlignAlwaysLeft(bool v) {
	if (!impObj()) return false;
	return PyWarnIfError(impObj()->setTextAlignAlwaysLeft(v), L"AcDbMLeaderStyle::setTextAlignAlwaysLeft");
}
double PyDbMLeaderStyle::textHeight() const { if (!impObj()) return 0.0; return impObj()->textHeight(); }
bool PyDbMLeaderStyle::setTextHeight(double v) {
	if (!impObj()) return false;
	return PyWarnIfError(impObj()->setTextHeight(v), L"AcDbMLeaderStyle::setTextHeight");
}
PyColor PyDbMLeaderStyle::textColor() const { if (!impObj()) return PyColor(); return PyColor(impObj()->textColor()); }
bool PyDbMLeaderStyle::setTextColor(const PyColor& c) {
	if (!impObj()) return false;
	return PyWarnIfError(impObj()->setTextColor(c.m_color), L"AcDbMLeaderStyle::setTextColor");
}
bool PyDbMLeaderStyle::enableFrameText() const { if (!impObj()) return false; return impObj()->enableFrameText(); }
bool PyDbMLeaderStyle::setEnableFrameText(bool v) {
	if (!impObj()) return false;
	return PyWarnIfError(impObj()->setEnableFrameText(v), L"AcDbMLeaderStyle::setEnableFrameText");
}
double PyDbMLeaderStyle::alignSpace() const { if (!impObj()) return 0.0; return impObj()->alignSpace(); }
bool PyDbMLeaderStyle::setAlignSpace(double v) {
	if (!impObj()) return false;
	return PyWarnIfError(impObj()->setAlignSpace(v), L"AcDbMLeaderStyle::setAlignSpace");
}
bool PyDbMLeaderStyle::setDefaultMText(uint64_t mtextId) {
	if (!impObj()) return false;
	AcDbObjectId oid; oid.setFromOldId(static_cast<Adesk::UIntPtr>(mtextId));
	AcDbMText* mt = nullptr;
	if (!oid.isNull()) {
		if (acdbOpenObject(mt, oid, AcDb::kForRead) != Acad::eOk) {
			acutPrintf(L"[PyArx] AcDbMLeaderStyle::setDefaultMText (e%d) [AcDbMText open]\n", static_cast<int>(Acad::eInvalidInput));
			return false;
		}
	}
	bool ok = PyWarnIfError(impObj()->setDefaultMText(mt), L"AcDbMLeaderStyle::setDefaultMText");
	if (mt) mt->close();
	return ok;
}
uint64_t PyDbMLeaderStyle::defaultMText() const {
	if (!impObj()) return 0ULL;
	AcDbMText* mt = impObj()->defaultMText(); // cloned; caller must delete
	if (!mt) return 0ULL;
	uint64_t id = static_cast<uint64_t>(mt->objectId().asOldId());
	delete mt;
	return id;
}

// ---- 块 ----
uint64_t PyDbMLeaderStyle::blockId() const {
	if (!impObj()) return 0ULL;
	return static_cast<uint64_t>(impObj()->blockId().asOldId());
}
bool PyDbMLeaderStyle::setBlockId(uint64_t id) {
	if (!impObj()) return false;
	AcDbObjectId oid; oid.setFromOldId(static_cast<Adesk::UIntPtr>(id));
	return PyWarnIfError(impObj()->setBlockId(oid), L"AcDbMLeaderStyle::setBlockId");
}
PyColor PyDbMLeaderStyle::blockColor() const { if (!impObj()) return PyColor(); return PyColor(impObj()->blockColor()); }
bool PyDbMLeaderStyle::setBlockColor(const PyColor& c) {
	if (!impObj()) return false;
	return PyWarnIfError(impObj()->setBlockColor(c.m_color), L"AcDbMLeaderStyle::setBlockColor");
}
bool PyDbMLeaderStyle::enableBlockScale() const { if (!impObj()) return false; return impObj()->enableBlockScale(); }
bool PyDbMLeaderStyle::setEnableBlockScale(bool v) {
	if (!impObj()) return false;
	return PyWarnIfError(impObj()->setEnableBlockScale(v), L"AcDbMLeaderStyle::setEnableBlockScale");
}
double PyDbMLeaderStyle::blockRotation() const { if (!impObj()) return 0.0; return impObj()->blockRotation(); }
bool PyDbMLeaderStyle::setBlockRotation(double v) {
	if (!impObj()) return false;
	return PyWarnIfError(impObj()->setBlockRotation(v), L"AcDbMLeaderStyle::setBlockRotation");
}
bool PyDbMLeaderStyle::enableBlockRotation() const { if (!impObj()) return false; return impObj()->enableBlockRotation(); }
bool PyDbMLeaderStyle::setEnableBlockRotation(bool v) {
	if (!impObj()) return false;
	return PyWarnIfError(impObj()->setEnableBlockRotation(v), L"AcDbMLeaderStyle::setEnableBlockRotation");
}

// ---- 比例 / 标注性 / 其他 ----
double PyDbMLeaderStyle::scale() const { if (!impObj()) return 0.0; return impObj()->scale(); }
bool PyDbMLeaderStyle::setScale(double v) {
	if (!impObj()) return false;
	return PyWarnIfError(impObj()->setScale(v), L"AcDbMLeaderStyle::setScale");
}
bool PyDbMLeaderStyle::overwritePropChanged() const { if (!impObj()) return false; return impObj()->overwritePropChanged(); }
bool PyDbMLeaderStyle::annotative() const { if (!impObj()) return false; return impObj()->annotative(); }
bool PyDbMLeaderStyle::setAnnotative(bool v) {
	if (!impObj()) return false;
	return PyWarnIfError(impObj()->setAnnotative(v), L"AcDbMLeaderStyle::setAnnotative");
}
double PyDbMLeaderStyle::breakSize() const { if (!impObj()) return 0.0; return impObj()->breakSize(); }
bool PyDbMLeaderStyle::setBreakSize(double v) {
	if (!impObj()) return false;
	return PyWarnIfError(impObj()->setBreakSize(v), L"AcDbMLeaderStyle::setBreakSize");
}
int PyDbMLeaderStyle::textAttachmentDirection() const {
	if (!impObj()) return 0;
	return static_cast<int>(impObj()->textAttachmentDirection());
}
bool PyDbMLeaderStyle::setTextAttachmentDirection(int v) {
	if (!impObj()) return false;
	return PyWarnIfError(impObj()->setTextAttachmentDirection(static_cast<AcDbMLeaderStyle::TextAttachmentDirection>(v)), L"AcDbMLeaderStyle::setTextAttachmentDirection");
}
bool PyDbMLeaderStyle::extendLeaderToText() const { if (!impObj()) return false; return impObj()->extendLeaderToText(); }
bool PyDbMLeaderStyle::setExtendLeaderToText(bool v) {
	if (!impObj()) return false;
	return PyWarnIfError(impObj()->setExtendLeaderToText(v), L"AcDbMLeaderStyle::setExtendLeaderToText");
}

// ---- 入库 ----
std::tuple<bool, uint64_t> PyDbMLeaderStyle::postMLeaderStyleToDb(PyDbDatabase& db, const AcString& styleName) {
	if (!impObj()) return std::make_tuple(false, 0ULL);
	AcDbObjectId styleId;
	Acad::ErrorStatus es = impObj()->postMLeaderStyleToDb(db.impObj(), styleName.kwszPtr(), styleId);
	if (!PyWarnIfError(es, L"AcDbMLeaderStyle::postMLeaderStyleToDb"))
		return std::make_tuple(false, 0ULL);
	return std::make_tuple(true, styleId.isNull() ? 0ULL : static_cast<uint64_t>(styleId.asOldId()));
}

void bind_PyDbMLeaderStyle(pym& m) {
	py::class_<PyDbMLeaderStyle, PyDbObject>(m, "PyDbMLeaderStyle",
		"多重引线样式（AcDbMLeaderStyle，直接继承 AcDbObject）")
		.def(py::init<>(), "默认构造")
		.def(py::init<AcDbMLeaderStyle*>(), py::arg("object"), "按裸指针构造")
		// ---- 名称 / 描述 ----
		.def("name", &PyDbMLeaderStyle::getName, "样式名")
		.def("setName", &PyDbMLeaderStyle::setName, py::arg("name"), "设置样式名；成功返回 true")
		.def("description", &PyDbMLeaderStyle::description, "描述文本")
		.def("setDescription", &PyDbMLeaderStyle::setDescription, py::arg("desc"), "设置描述；成功返回 true")
		// ---- 内容类型 ----
		.def("contentType", &PyDbMLeaderStyle::contentType, "内容类型：0=多行文字 1=块")
		.def("setContentType", &PyDbMLeaderStyle::setContentType, py::arg("v"), "设置内容类型；成功返回 true")
		// ---- 引线 ----
		.def("maxLeaderSegmentsPoints", &PyDbMLeaderStyle::maxLeaderSegmentsPoints)
		.def("setMaxLeaderSegmentsPoints", &PyDbMLeaderStyle::setMaxLeaderSegmentsPoints, py::arg("v"))
		.def("firstSegmentAngleConstraint", &PyDbMLeaderStyle::firstSegmentAngleConstraint)
		.def("setFirstSegmentAngleConstraint", &PyDbMLeaderStyle::setFirstSegmentAngleConstraint, py::arg("v"))
		.def("secondSegmentAngleConstraint", &PyDbMLeaderStyle::secondSegmentAngleConstraint)
		.def("setSecondSegmentAngleConstraint", &PyDbMLeaderStyle::setSecondSegmentAngleConstraint, py::arg("v"))
		.def("leaderLineTypeId", &PyDbMLeaderStyle::leaderLineTypeId, "引线线型 ObjectId(uint64)")
		.def("setLeaderLineTypeId", &PyDbMLeaderStyle::setLeaderLineTypeId, py::arg("id"))
		.def("enableLanding", &PyDbMLeaderStyle::enableLanding)
		.def("setEnableLanding", &PyDbMLeaderStyle::setEnableLanding, py::arg("v"))
		.def("landingGap", &PyDbMLeaderStyle::landingGap)
		.def("setLandingGap", &PyDbMLeaderStyle::setLandingGap, py::arg("v"))
		.def("enableDogleg", &PyDbMLeaderStyle::enableDogleg)
		.def("setEnableDogleg", &PyDbMLeaderStyle::setEnableDogleg, py::arg("v"))
		.def("doglegLength", &PyDbMLeaderStyle::doglegLength)
		.def("setDoglegLength", &PyDbMLeaderStyle::setDoglegLength, py::arg("v"))
		// ---- 箭头 ----
		.def("arrowSymbolId", &PyDbMLeaderStyle::arrowSymbolId, "箭头符号 ObjectId(uint64)")
		.def("setArrowSymbolId", &PyDbMLeaderStyle::setArrowSymbolId, py::arg("id"))
		.def("arrowSize", &PyDbMLeaderStyle::arrowSize)
		.def("setArrowSize", &PyDbMLeaderStyle::setArrowSize, py::arg("v"))
		// ---- 文字 ----
		.def("textStyleId", &PyDbMLeaderStyle::textStyleId, "文字样式 ObjectId(uint64)")
		.def("setTextStyleId", &PyDbMLeaderStyle::setTextStyleId, py::arg("id"))
		.def("textAlignmentType", &PyDbMLeaderStyle::textAlignmentType)
		.def("setTextAlignmentType", &PyDbMLeaderStyle::setTextAlignmentType, py::arg("v"))
		.def("textAlignAlwaysLeft", &PyDbMLeaderStyle::textAlignAlwaysLeft)
		.def("setTextAlignAlwaysLeft", &PyDbMLeaderStyle::setTextAlignAlwaysLeft, py::arg("v"))
		.def("textHeight", &PyDbMLeaderStyle::textHeight)
		.def("setTextHeight", &PyDbMLeaderStyle::setTextHeight, py::arg("v"))
		.def("textColor", &PyDbMLeaderStyle::textColor, "文字颜色 (Color)")
		.def("setTextColor", &PyDbMLeaderStyle::setTextColor, py::arg("color"), "设置文字颜色 (Color)")
		.def("enableFrameText", &PyDbMLeaderStyle::enableFrameText)
		.def("setEnableFrameText", &PyDbMLeaderStyle::setEnableFrameText, py::arg("v"))
		.def("alignSpace", &PyDbMLeaderStyle::alignSpace)
		.def("setAlignSpace", &PyDbMLeaderStyle::setAlignSpace, py::arg("v"))
		.def("setDefaultMText", &PyDbMLeaderStyle::setDefaultMText, py::arg("mtext_id"), "设置默认多行文字 (by ObjectId)；成功返回 true")
		.def("defaultMText", &PyDbMLeaderStyle::defaultMText, "默认多行文字 ObjectId(uint64)")
		// ---- 块 ----
		.def("blockId", &PyDbMLeaderStyle::blockId, "块 ObjectId(uint64)")
		.def("setBlockId", &PyDbMLeaderStyle::setBlockId, py::arg("id"))
		.def("blockColor", &PyDbMLeaderStyle::blockColor, "块颜色 (Color)")
		.def("setBlockColor", &PyDbMLeaderStyle::setBlockColor, py::arg("color"))
		.def("enableBlockScale", &PyDbMLeaderStyle::enableBlockScale)
		.def("setEnableBlockScale", &PyDbMLeaderStyle::setEnableBlockScale, py::arg("v"))
		.def("blockRotation", &PyDbMLeaderStyle::blockRotation)
		.def("setBlockRotation", &PyDbMLeaderStyle::setBlockRotation, py::arg("v"))
		.def("enableBlockRotation", &PyDbMLeaderStyle::enableBlockRotation)
		.def("setEnableBlockRotation", &PyDbMLeaderStyle::setEnableBlockRotation, py::arg("v"))
		// ---- 比例 / 标注性 / 其他 ----
		.def("scale", &PyDbMLeaderStyle::scale)
		.def("setScale", &PyDbMLeaderStyle::setScale, py::arg("v"))
		.def("overwritePropChanged", &PyDbMLeaderStyle::overwritePropChanged)
		.def("annotative", &PyDbMLeaderStyle::annotative, "是否注释性")
		.def("setAnnotative", &PyDbMLeaderStyle::setAnnotative, py::arg("v"))
		.def("breakSize", &PyDbMLeaderStyle::breakSize, "断开尺寸")
		.def("setBreakSize", &PyDbMLeaderStyle::setBreakSize, py::arg("v"))
		.def("textAttachmentDirection", &PyDbMLeaderStyle::textAttachmentDirection)
		.def("setTextAttachmentDirection", &PyDbMLeaderStyle::setTextAttachmentDirection, py::arg("v"))
		.def("extendLeaderToText", &PyDbMLeaderStyle::extendLeaderToText)
		.def("setExtendLeaderToText", &PyDbMLeaderStyle::setExtendLeaderToText, py::arg("v"))
		// ---- 入库 ----
		.def("postMLeaderStyleToDb", &PyDbMLeaderStyle::postMLeaderStyleToDb,
			py::arg("database"), py::arg("style_name"),
			"把样式入库到数据库，返回 (是否成功, styleObjectId)")
		PyWrapper_BIND(DbMLeaderStyle)
		.def("__repr__", &PyDbMLeaderStyle::toString)
		;
}
