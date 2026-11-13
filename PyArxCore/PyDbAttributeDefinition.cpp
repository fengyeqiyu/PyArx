#include "stdafx.h"
#include "PyErrorStatusException.h"
#include "PyDbAttributeDefinition.h"
#include "PyDbMText.h"
#include "PyDbDatabase.h"

PyWrapper_Cpp(DbAttributeDefinition)
AcString PyDbAttributeDefinition::toString() const { return AcString(std::format("AcDbAttributeDefinition[wrapper]").c_str(), AcString::Utf8); }
AcString PyDbAttributeDefinition::tag() const { AcString s; if(!PyWarnIfError(impObj()->tag(s), L"AcDbAttributeDefinition::tag")) return AcString(); return s; }

bool  PyDbAttributeDefinition::setTag(const AcString& t) { return PyWarnIfError(impObj()->setTag(t.kwszPtr()), L"AcDbAttributeDefinition::setTag"); }

AcString PyDbAttributeDefinition::prompt() const { AcString s; if(!PyWarnIfError(impObj()->prompt(s), L"AcDbAttributeDefinition::prompt")) return AcString(); return s; }
bool  PyDbAttributeDefinition::setPrompt(const AcString& s) { return PyWarnIfError(impObj()->setPrompt(s.kwszPtr()), L"AcDbAttributeDefinition::setPrompt"); }
bool PyDbAttributeDefinition::isInvisible() const { return impObj()->isInvisible() != Adesk::kFalse; }
bool  PyDbAttributeDefinition::setInvisible(bool v) { return PyWarnIfError(impObj()->setInvisible(v ? Adesk::kTrue : Adesk::kFalse), L"AcDbAttributeDefinition::setInvisible"); }
bool PyDbAttributeDefinition::isConstant() const { return impObj()->isConstant() != Adesk::kFalse; }
bool  PyDbAttributeDefinition::setConstant(bool v) { return PyWarnIfError(impObj()->setConstant(v ? Adesk::kTrue : Adesk::kFalse), L"AcDbAttributeDefinition::setConstant"); }
bool PyDbAttributeDefinition::isVerifiable() const { return impObj()->isVerifiable() != Adesk::kFalse; }
bool  PyDbAttributeDefinition::setVerifiable(bool v) { return PyWarnIfError(impObj()->setVerifiable(v ? Adesk::kTrue : Adesk::kFalse), L"AcDbAttributeDefinition::setVerifiable"); }
bool PyDbAttributeDefinition::isPreset() const { return impObj()->isPreset() != Adesk::kFalse; }
bool  PyDbAttributeDefinition::setPreset(bool v) { return PyWarnIfError(impObj()->setPreset(v ? Adesk::kTrue : Adesk::kFalse), L"AcDbAttributeDefinition::setPreset"); }
int PyDbAttributeDefinition::fieldLength() const { return static_cast<int>(impObj()->fieldLength()); }
bool  PyDbAttributeDefinition::setFieldLength(int v) { return PyWarnIfError(impObj()->setFieldLength(static_cast<Adesk::UInt16>(v)), L"AcDbAttributeDefinition::setFieldLength"); }
bool PyDbAttributeDefinition::lockPositionInBlock() const { return impObj()->lockPositionInBlock(); }
bool  PyDbAttributeDefinition::setLockPositionInBlock(bool v) { return PyWarnIfError(impObj()->setLockPositionInBlock(v), L"AcDbAttributeDefinition::setLockPositionInBlock"); }

bool PyDbAttributeDefinition::adjustAlignment(const PyDbDatabase* pDb) { return PyWarnIfError(impObj()->adjustAlignment(pDb ? pDb->impObj() : nullptr), L"AcDbAttributeDefinition::adjustAlignment"); }
bool PyDbAttributeDefinition::convertIntoMTextAttributeDefinition(bool val) { return PyWarnIfError(impObj()->convertIntoMTextAttributeDefinition(val ? Adesk::kTrue : Adesk::kFalse), L"AcDbAttributeDefinition::convertIntoMTextAttributeDefinition"); }
bool PyDbAttributeDefinition::isMTextAttributeDefinition() const { return impObj()->isMTextAttributeDefinition(); }
bool PyDbAttributeDefinition::setMTextAttributeDefinition(PyDbMText* pMText) {
	if (!pMText) return false;
	return PyWarnIfError(impObj()->setMTextAttributeDefinition(pMText->impObj()), L"AcDbAttributeDefinition::setMTextAttributeDefinition");
}
bool PyDbAttributeDefinition::setMTextAttributeDefinitionConst(const PyDbMText* pMText) {
	if (!pMText) return false;
	return PyWarnIfError(impObj()->setMTextAttributeDefinitionConst(pMText->impObj()), L"AcDbAttributeDefinition::setMTextAttributeDefinitionConst");
}
bool PyDbAttributeDefinition::updateMTextAttributeDefinition() { return PyWarnIfError(impObj()->updateMTextAttributeDefinition(), L"AcDbAttributeDefinition::updateMTextAttributeDefinition"); }
PyDbMText PyDbAttributeDefinition::getMTextAttributeDefinition() const { return PyDbMText(impObj()->getMTextAttributeDefinition()); }
PyDbMText PyDbAttributeDefinition::getMTextAttributeDefinitionConst() const { return PyDbMText(const_cast<AcDbMText*>(impObj()->getMTextAttributeDefinitionConst())); }

void bind_PyDbAttributeDefinition(pym& m) {
	py::class_<PyDbAttributeDefinition, PyDbText>(m, "PyDbAttributeDefinition")
		.def(py::init<>())
		.def(py::init<const AcGePoint3d&, const AcString&, const AcString&, const AcString&>(),
			py::arg("position"), py::arg("text"), py::arg("tag"), py::arg("prompt"))
		.def(py::init<AcDbAttributeDefinition*>(), py::arg("object"))
		.def("tag", &PyDbAttributeDefinition::tag)
		.def("setTag", &PyDbAttributeDefinition::setTag, py::arg("tag"))
		.def("prompt", &PyDbAttributeDefinition::prompt)
		.def("setPrompt", &PyDbAttributeDefinition::setPrompt, py::arg("prompt"))
		.def("isInvisible", &PyDbAttributeDefinition::isInvisible)
		.def("setInvisible", &PyDbAttributeDefinition::setInvisible, py::arg("v"))
		.def("isConstant", &PyDbAttributeDefinition::isConstant)
		.def("setConstant", &PyDbAttributeDefinition::setConstant, py::arg("v"))
		.def("isVerifiable", &PyDbAttributeDefinition::isVerifiable)
		.def("setVerifiable", &PyDbAttributeDefinition::setVerifiable, py::arg("v"))
		.def("isPreset", &PyDbAttributeDefinition::isPreset)
		.def("setPreset", &PyDbAttributeDefinition::setPreset, py::arg("v"))
		.def("fieldLength", &PyDbAttributeDefinition::fieldLength)
		.def("setFieldLength", &PyDbAttributeDefinition::setFieldLength, py::arg("v"))
		.def("lockPositionInBlock", &PyDbAttributeDefinition::lockPositionInBlock)
		.def("setLockPositionInBlock", &PyDbAttributeDefinition::setLockPositionInBlock, py::arg("v"))
		.def("adjustAlignment", &PyDbAttributeDefinition::adjustAlignment, py::arg("db"), "对齐调整，可传入数据库（不传则继承自 PyDbText 的无参版本）")
		.def("convertIntoMTextAttributeDefinition", &PyDbAttributeDefinition::convertIntoMTextAttributeDefinition, py::arg("val") = true, "转换为多行属性定义")
		.def("isMTextAttributeDefinition", &PyDbAttributeDefinition::isMTextAttributeDefinition, "是否为多行属性定义")
		.def("setMTextAttributeDefinition", &PyDbAttributeDefinition::setMTextAttributeDefinition, py::arg("pMText"), "替换内部多行文本（成功后由属性定义接管所有权）")
		.def("setMTextAttributeDefinitionConst", &PyDbAttributeDefinition::setMTextAttributeDefinitionConst, py::arg("pMText"), "替换内部多行文本（const 版本，成功后由属性定义接管所有权）")
		.def("updateMTextAttributeDefinition", &PyDbAttributeDefinition::updateMTextAttributeDefinition, "从属性定义基本属性刷新内部多行文本")
		.def("getMTextAttributeDefinition", &PyDbAttributeDefinition::getMTextAttributeDefinition, "获取内部多行文本的克隆（调用方负责释放）")
		.def("getMTextAttributeDefinitionConst", &PyDbAttributeDefinition::getMTextAttributeDefinitionConst, "获取内部多行文本的 const 克隆（调用方负责释放）")
		PyWrapper_BIND(DbAttributeDefinition)
		.def("__repr__", &PyDbAttributeDefinition::toString)
	;
}

