#include "stdafx.h"
#include "PyErrorStatusException.h"
#include "PyDbAttribute.h"
#include "PyDbMText.h"
#include "PyDbAttributeDefinition.h"

PyWrapper_Cpp(DbAttribute)
AcString PyDbAttribute::toString() const { return AcString(std::format("AcDbAttribute[wrapper]").c_str(), AcString::Utf8); }
AcString PyDbAttribute::tag() const { if (!impObj()) return AcString(); AcString s; if(!PyWarnIfError(impObj()->tag(s), L"AcDbAttribute::tag")) return AcString(); return s; }

bool  PyDbAttribute::setTag(const AcString& t) { if (!impObj()) return false; return PyWarnIfError(impObj()->setTag(t.kwszPtr()), L"AcDbAttribute::setTag"); }

bool PyDbAttribute::isInvisible() const { if (!impObj()) return false; return impObj()->isInvisible() == Adesk::kTrue; }

bool  PyDbAttribute::setInvisible(bool v) { if (!impObj()) return false; return PyWarnIfError(impObj()->setInvisible(v ? Adesk::kTrue : Adesk::kFalse), L"AcDbAttribute::setInvisible"); }

bool PyDbAttribute::isConstant() const { if (!impObj()) return false; return impObj()->isConstant() != Adesk::kFalse; }
bool PyDbAttribute::isVerifiable() const { if (!impObj()) return false; return impObj()->isVerifiable() != Adesk::kFalse; }
bool PyDbAttribute::isPreset() const { if (!impObj()) return false; return impObj()->isPreset() != Adesk::kFalse; }
int PyDbAttribute::fieldLength() const { if (!impObj()) return 0; return static_cast<int>(impObj()->fieldLength()); }
bool  PyDbAttribute::setFieldLength(int v) { if (!impObj()) return false; return PyWarnIfError(impObj()->setFieldLength(static_cast<Adesk::UInt16>(v)), L"AcDbAttribute::setFieldLength"); }
bool PyDbAttribute::lockPositionInBlock() const { if (!impObj()) return false; return impObj()->lockPositionInBlock(); }
bool  PyDbAttribute::setLockPositionInBlock(bool v) { if (!impObj()) return false; return PyWarnIfError(impObj()->setLockPositionInBlock(v), L"AcDbAttribute::setLockPositionInBlock"); }
bool PyDbAttribute::isReallyLocked() const { if (!impObj()) return false; return impObj()->isReallyLocked(); }

bool PyDbAttribute::convertIntoMTextAttribute(bool val) { if (!impObj()) return false; return PyWarnIfError(impObj()->convertIntoMTextAttribute(val ? Adesk::kTrue : Adesk::kFalse), L"AcDbAttribute::convertIntoMTextAttribute"); }
bool PyDbAttribute::isMTextAttribute() const { if (!impObj()) return false; return impObj()->isMTextAttribute(); }
bool PyDbAttribute::setAttributeFromBlock(const AcGeMatrix3d& blkXform) { if (!impObj()) return false; return PyWarnIfError(impObj()->setAttributeFromBlock(blkXform), L"AcDbAttribute::setAttributeFromBlock"); }
bool PyDbAttribute::setAttributeFromBlock(const PyDbAttributeDefinition* pAttdef, const AcGeMatrix3d& blkXform) {
	if (!impObj()) return false;
	if (!pAttdef) return false;
	return PyWarnIfError(impObj()->setAttributeFromBlock(pAttdef->impObj(), blkXform), L"AcDbAttribute::setAttributeFromBlock");
}
bool PyDbAttribute::setMTextAttribute(PyDbMText* pMText) {
	if (!impObj()) return false;
	if (!pMText) return false;
	return PyWarnIfError(impObj()->setMTextAttribute(pMText->impObj()), L"AcDbAttribute::setMTextAttribute");
}
bool PyDbAttribute::setMTextAttributeConst(const PyDbMText* pMText) {
	if (!impObj()) return false;
	if (!pMText) return false;
	return PyWarnIfError(impObj()->setMTextAttributeConst(pMText->impObj()), L"AcDbAttribute::setMTextAttributeConst");
}
bool PyDbAttribute::updateMTextAttribute() { if (!impObj()) return false; return PyWarnIfError(impObj()->updateMTextAttribute(), L"AcDbAttribute::updateMTextAttribute"); }
PyDbMText PyDbAttribute::getMTextAttribute() const { return PyDbMText(impObj()->getMTextAttribute()); }
PyDbMText PyDbAttribute::getMTextAttributeConst() const { return PyDbMText(const_cast<AcDbMText*>(impObj()->getMTextAttributeConst())); }

void bind_PyDbAttribute(pym& m) {
	py::class_<PyDbAttribute, PyDbText>(m, "PyDbAttribute")
		.def(py::init<>())
		.def(py::init<const AcGePoint3d&, const AcString&, const AcString&>(),
			py::arg("position"), py::arg("text"), py::arg("tag"))
		.def(py::init<AcDbAttribute*>(), py::arg("object"))
		.def("tag", &PyDbAttribute::tag)
		.def("setTag", &PyDbAttribute::setTag, py::arg("tag"))
		.def("isInvisible", &PyDbAttribute::isInvisible)
		.def("setInvisible", &PyDbAttribute::setInvisible, py::arg("v"))
		.def("isConstant", &PyDbAttribute::isConstant)
		.def("isVerifiable", &PyDbAttribute::isVerifiable)
		.def("isPreset", &PyDbAttribute::isPreset)
		.def("fieldLength", &PyDbAttribute::fieldLength)
		.def("setFieldLength", &PyDbAttribute::setFieldLength, py::arg("v"))
		.def("lockPositionInBlock", &PyDbAttribute::lockPositionInBlock)
		.def("setLockPositionInBlock", &PyDbAttribute::setLockPositionInBlock, py::arg("v"))
		.def("isReallyLocked", &PyDbAttribute::isReallyLocked)
		.def("convertIntoMTextAttribute", &PyDbAttribute::convertIntoMTextAttribute, py::arg("val") = true, "转换为多行属性")
		.def("isMTextAttribute", &PyDbAttribute::isMTextAttribute, "是否为多行属性")
		.def("setAttributeFromBlock", [](PyDbAttribute& self, const AcGeMatrix3d& blkXform) {
			return self.setAttributeFromBlock(blkXform);
		}, py::arg("blkXform"), "按块变换矩阵更新属性")
		.def("setAttributeFromBlock", [](PyDbAttribute& self, const PyDbAttributeDefinition* pAttdef, const AcGeMatrix3d& blkXform) {
			return self.setAttributeFromBlock(pAttdef, blkXform);
		}, py::arg("pAttdef"), py::arg("blkXform"), "按属性定义与块变换矩阵更新属性")
		.def("setMTextAttribute", &PyDbAttribute::setMTextAttribute, py::arg("pMText"), "替换内部多行文本（成功后由属性接管所有权）")
		.def("setMTextAttributeConst", &PyDbAttribute::setMTextAttributeConst, py::arg("pMText"), "替换内部多行文本（const 版本，成功后由属性接管所有权）")
		.def("updateMTextAttribute", &PyDbAttribute::updateMTextAttribute, "从属性基本属性刷新内部多行文本")
		.def("getMTextAttribute", &PyDbAttribute::getMTextAttribute, "获取内部多行文本的克隆（调用方负责释放）")
		.def("getMTextAttributeConst", &PyDbAttribute::getMTextAttributeConst, "获取内部多行文本的 const 克隆（调用方负责释放）")
		PyWrapper_BIND(DbAttribute)
		.def("__repr__", &PyDbAttribute::toString)
	;
}

