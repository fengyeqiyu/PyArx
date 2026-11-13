#include "stdafx.h"
#include "PyErrorStatusException.h"
#include "PyDbMLeader.h"
#include "PyDbMText.h"
#include "PyDbDatabase.h"

PyWrapper_Cpp(DbMLeader)
AcString PyDbMLeader::toString() const { return AcString(std::format("AcDbMLeader[wrapper]").c_str(), AcString::Utf8); }
int PyDbMLeader::numLeaderLines() const { return impObj()->numLeaderLines(); }

double PyDbMLeader::scale() const { return impObj()->scale(); }
bool  PyDbMLeader::setScale(double v) { return PyWarnIfError(impObj()->setScale(v), L"AcDbMLeader::setScale"); }
int PyDbMLeader::contentType() const { return static_cast<int>(impObj()->contentType()); }
bool  PyDbMLeader::setContentType(int v) { return PyWarnIfError(impObj()->setContentType(static_cast<AcDbMLeaderStyle::ContentType>(v)), L"AcDbMLeader::setContentType"); }
bool PyDbMLeader::enableAnnotationScale() const { return impObj()->enableAnnotationScale(); }
bool  PyDbMLeader::setEnableAnnotationScale(bool v) { return PyWarnIfError(impObj()->setEnableAnnotationScale(v), L"AcDbMLeader::setEnableAnnotationScale"); }
bool PyDbMLeader::extendLeaderToText() const { return impObj()->extendLeaderToText(); }
bool  PyDbMLeader::setExtendLeaderToText(bool v) { return PyWarnIfError(impObj()->setExtendLeaderToText(v), L"AcDbMLeader::setExtendLeaderToText"); }
int PyDbMLeader::textAttachmentDirection() const { return static_cast<int>(impObj()->textAttachmentDirection()); }
bool  PyDbMLeader::setTextAttachmentDirection(int v) { return PyWarnIfError(impObj()->setTextAttachmentDirection(static_cast<AcDbMLeaderStyle::TextAttachmentDirection>(v)), L"AcDbMLeader::setTextAttachmentDirection"); }
bool  PyDbMLeader::updateLeaderLinePosition() { return PyWarnIfError(impObj()->updateLeaderLinePosition(), L"AcDbMLeader::updateLeaderLinePosition"); }
int PyDbMLeader::numLeaders() const { return impObj()->numLeaders(); }

// ---- 构建族（T0 补齐 2026-10-21）：引线 / 引线直线 / 顶点 / 内容 / 样式 ----
bool PyDbMLeader::addLeader(int& leaderIndex) {
	Acad::ErrorStatus es = impObj()->addLeader(leaderIndex);
	return PyWarnIfError(es, L"AcDbMLeader::addLeader");
}
bool PyDbMLeader::removeLeader(int leaderIndex) {
	return PyWarnIfError(impObj()->removeLeader(leaderIndex), L"AcDbMLeader::removeLeader");
}
bool PyDbMLeader::addLeaderLine(int leaderIndex, int& leaderLineIndex) {
	Acad::ErrorStatus es = impObj()->addLeaderLine(leaderIndex, leaderLineIndex);
	return PyWarnIfError(es, L"AcDbMLeader::addLeaderLine");
}
bool PyDbMLeader::addLeaderLineAtPoint(const AcGePoint3d& point, int& leaderLineIndex) {
	Acad::ErrorStatus es = impObj()->addLeaderLine(point, leaderLineIndex);
	return PyWarnIfError(es, L"AcDbMLeader::addLeaderLine(point)");
}
bool PyDbMLeader::removeLeaderLine(int leaderLineIndex) {
	return PyWarnIfError(impObj()->removeLeaderLine(leaderLineIndex), L"AcDbMLeader::removeLeaderLine");
}
bool PyDbMLeader::addFirstVertex(int leaderLineIndex, const AcGePoint3d& point) {
	return PyWarnIfError(impObj()->addFirstVertex(leaderLineIndex, point), L"AcDbMLeader::addFirstVertex");
}
bool PyDbMLeader::addLastVertex(int leaderLineIndex, const AcGePoint3d& point) {
	return PyWarnIfError(impObj()->addLastVertex(leaderLineIndex, point), L"AcDbMLeader::addLastVertex");
}
bool PyDbMLeader::numVertices(int leaderLineIndex, int& num) const {
	Acad::ErrorStatus es = impObj()->numVertices(leaderLineIndex, num);
	return PyWarnIfError(es, L"AcDbMLeader::numVertices");
}
bool PyDbMLeader::setVertex(int leaderLineIndex, int index, const AcGePoint3d& point) {
	return PyWarnIfError(impObj()->setVertex(leaderLineIndex, index, point), L"AcDbMLeader::setVertex");
}
bool PyDbMLeader::getVertex(int leaderLineIndex, int index, AcGePoint3d& point) const {
	Acad::ErrorStatus es = impObj()->getVertex(leaderLineIndex, index, point);
	return PyWarnIfError(es, L"AcDbMLeader::getVertex");
}
bool PyDbMLeader::setMText(const AcDbMText* pMText) {
	return PyWarnIfError(impObj()->setMText(pMText), L"AcDbMLeader::setMText");
}
bool PyDbMLeader::setBlockContentId(AcDbObjectId blockId) {
	return PyWarnIfError(impObj()->setBlockContentId(blockId), L"AcDbMLeader::setBlockContentId");
}
AcDbObjectId PyDbMLeader::blockContentId() const { return impObj()->blockContentId(); }
bool PyDbMLeader::setMLeaderStyle(AcDbObjectId newStyleId) {
	return PyWarnIfError(impObj()->setMLeaderStyle(newStyleId), L"AcDbMLeader::setMLeaderStyle");
}
AcDbObjectId PyDbMLeader::MLeaderStyle() const { return impObj()->MLeaderStyle(); }
bool PyDbMLeader::setTextLocation(const AcGePoint3d& location) {
	return PyWarnIfError(impObj()->setTextLocation(location), L"AcDbMLeader::setTextLocation");
}
bool PyDbMLeader::getTextLocation(AcGePoint3d& location) const {
	Acad::ErrorStatus es = impObj()->getTextLocation(location);
	return PyWarnIfError(es, L"AcDbMLeader::getTextLocation");
}
bool PyDbMLeader::postMLeaderToDb(AcDbDatabase* pDb) {
	return PyWarnIfError(impObj()->postMLeaderToDb(pDb), L"AcDbMLeader::postMLeaderToDb");
}
AcGeVector3d PyDbMLeader::normal() const { return impObj()->normal(); }
bool PyDbMLeader::hasContent() const { return impObj()->hasContent(); }
double PyDbMLeader::landingGap() const { return impObj()->landingGap(); }
bool  PyDbMLeader::setLandingGap(double v) { return PyWarnIfError(impObj()->setLandingGap(v), L"AcDbMLeader::setLandingGap"); }
bool PyDbMLeader::enableLanding() const { return impObj()->enableLanding(); }
bool  PyDbMLeader::setEnableLanding(bool v) { return PyWarnIfError(impObj()->setEnableLanding(v), L"AcDbMLeader::setEnableLanding"); }
bool PyDbMLeader::enableDogleg() const { return impObj()->enableDogleg(); }
bool  PyDbMLeader::setEnableDogleg(bool v) { return PyWarnIfError(impObj()->setEnableDogleg(v), L"AcDbMLeader::setEnableDogleg"); }
double PyDbMLeader::arrowSize() const { return impObj()->arrowSize(); }
bool  PyDbMLeader::setArrowSize(double v) { return PyWarnIfError(impObj()->setArrowSize(v), L"AcDbMLeader::setArrowSize"); }
double PyDbMLeader::textHeight() const { return impObj()->textHeight(); }
bool  PyDbMLeader::setTextHeight(double v) { return PyWarnIfError(impObj()->setTextHeight(v), L"AcDbMLeader::setTextHeight"); }
double PyDbMLeader::blockRotation() const { return impObj()->blockRotation(); }
bool  PyDbMLeader::setBlockRotation(double v) { return PyWarnIfError(impObj()->setBlockRotation(v), L"AcDbMLeader::setBlockRotation"); }

void bind_PyDbMLeader(pym& m) {
	py::class_<PyDbMLeader, PyDbEntity>(m, "PyDbMLeader")
		.def(py::init<>())
		.def(py::init<AcDbMLeader*>(), py::arg("object"))
		.def("numLeaderLines", &PyDbMLeader::numLeaderLines)
		.def("scale", &PyDbMLeader::scale)
		.def("setScale", &PyDbMLeader::setScale, py::arg("v"))
		.def("contentType", &PyDbMLeader::contentType)
		.def("setContentType", &PyDbMLeader::setContentType, py::arg("v"))
		.def("enableAnnotationScale", &PyDbMLeader::enableAnnotationScale)
		.def("setEnableAnnotationScale", &PyDbMLeader::setEnableAnnotationScale, py::arg("v"))
		.def("extendLeaderToText", &PyDbMLeader::extendLeaderToText)
		.def("setExtendLeaderToText", &PyDbMLeader::setExtendLeaderToText, py::arg("v"))
		.def("textAttachmentDirection", &PyDbMLeader::textAttachmentDirection)
		.def("setTextAttachmentDirection", &PyDbMLeader::setTextAttachmentDirection, py::arg("v"))
		.def("numLeaders", &PyDbMLeader::numLeaders)
		.def("normal", &PyDbMLeader::normal)
		.def("hasContent", &PyDbMLeader::hasContent)
		.def("landingGap", &PyDbMLeader::landingGap)
		.def("setLandingGap", &PyDbMLeader::setLandingGap, py::arg("v"))
		.def("enableLanding", &PyDbMLeader::enableLanding)
		.def("setEnableLanding", &PyDbMLeader::setEnableLanding, py::arg("v"))
		.def("enableDogleg", &PyDbMLeader::enableDogleg)
		.def("setEnableDogleg", &PyDbMLeader::setEnableDogleg, py::arg("v"))
		.def("arrowSize", &PyDbMLeader::arrowSize)
		.def("setArrowSize", &PyDbMLeader::setArrowSize, py::arg("v"))
		.def("textHeight", &PyDbMLeader::textHeight)
		.def("setTextHeight", &PyDbMLeader::setTextHeight, py::arg("v"))
		.def("blockRotation", &PyDbMLeader::blockRotation)
		.def("setBlockRotation", &PyDbMLeader::setBlockRotation, py::arg("v"))
		.def("updateLeaderLinePosition", &PyDbMLeader::updateLeaderLinePosition)
		// ---- 构建族（T0 补齐 2026-10-21）----
		.def("addLeader", [](PyDbMLeader& self) -> std::pair<int, bool> {
			int idx = -1;
			bool ok = self.addLeader(idx);
			return { idx, ok };
		}, "新增一条引线，返回 (leaderIndex, 是否成功)")
		.def("removeLeader", &PyDbMLeader::removeLeader, py::arg("leader_index"), "删除一条引线；成功返回 true")
		.def("addLeaderLine", [](PyDbMLeader& self, int leaderIndex) -> std::pair<int, bool> {
			int idx = -1;
			bool ok = self.addLeaderLine(leaderIndex, idx);
			return { idx, ok };
		}, py::arg("leader_index"), "在指定引线下新增引线直线，返回 (leaderLineIndex, 是否成功)")
		.def("addLeaderLineAtPoint", [](PyDbMLeader& self, const AcGePoint3d& pt) -> std::pair<int, bool> {
			int idx = -1;
			bool ok = self.addLeaderLineAtPoint(pt, idx);
			return { idx, ok };
		}, py::arg("point"), "按点新增引线直线，返回 (leaderLineIndex, 是否成功)")
		.def("removeLeaderLine", &PyDbMLeader::removeLeaderLine, py::arg("leader_line_index"), "删除引线直线；成功返回 true")
		.def("addFirstVertex", &PyDbMLeader::addFirstVertex, py::arg("leader_line_index"), py::arg("point"), "在引线直线首端插入顶点；成功返回 true")
		.def("addLastVertex", &PyDbMLeader::addLastVertex, py::arg("leader_line_index"), py::arg("point"), "在引线直线末端追加顶点；成功返回 true")
		.def("numVertices", [](const PyDbMLeader& self, int leaderLineIndex) -> std::pair<int, bool> {
			int num = -1;
			bool ok = self.numVertices(leaderLineIndex, num);
			return { num, ok };
		}, py::arg("leader_line_index"), "引线直线顶点数，返回 (num, 是否成功)")
		.def("setVertex", &PyDbMLeader::setVertex, py::arg("leader_line_index"), py::arg("index"), py::arg("point"), "设置顶点；成功返回 true")
		.def("getVertex", [](const PyDbMLeader& self, int leaderLineIndex, int index) -> std::pair<AcGePoint3d, bool> {
			AcGePoint3d pt;
			bool ok = self.getVertex(leaderLineIndex, index, pt);
			return { pt, ok };
		}, py::arg("leader_line_index"), py::arg("index"), "读取顶点，返回 (point, 是否成功)")
		.def("setMText", [](PyDbMLeader& self, const PyDbMText& mt) -> bool {
			return self.setMText(mt.impObj());
		}, py::arg("mtext"), "设置多行文字内容（contentType=kMTextContent 时）；成功返回 true")
		.def("setBlockContentId", [](PyDbMLeader& self, uint64_t blockId) -> bool {
			AcDbObjectId id;
			id.setFromOldId(static_cast<Adesk::UIntPtr>(blockId));
			return self.setBlockContentId(id);
		}, py::arg("block_id"), "设置块注释内容 ID(by oldId)；成功返回 true")
		.def("blockContentId", [](const PyDbMLeader& self) -> uint64_t {
			return static_cast<uint64_t>(self.blockContentId().asOldId());
		}, "返回块注释内容 ObjectId(uint64)")
		.def("setMLeaderStyle", [](PyDbMLeader& self, uint64_t styleId) -> bool {
			AcDbObjectId id;
			id.setFromOldId(static_cast<Adesk::UIntPtr>(styleId));
			return self.setMLeaderStyle(id);
		}, py::arg("style_id"), "设置引线样式 ID(by oldId)；成功返回 true")
		.def("mleaderStyle", [](const PyDbMLeader& self) -> uint64_t {
			return static_cast<uint64_t>(self.MLeaderStyle().asOldId());
		}, "返回引线样式 ObjectId(uint64)")
		.def("setTextLocation", &PyDbMLeader::setTextLocation, py::arg("location"), "设置文字位置；成功返回 true")
		.def("getTextLocation", [](const PyDbMLeader& self) -> std::pair<AcGePoint3d, bool> {
			AcGePoint3d loc;
			bool ok = self.getTextLocation(loc);
			return { loc, ok };
		}, "读取文字位置，返回 (location, 是否成功)")
		.def("postMLeaderToDb", [](PyDbMLeader& self, PyDbDatabase& db) -> bool {
			return self.postMLeaderToDb(db.impObj());
		}, py::arg("database"), "程序化构建后入库；成功返回 true")
		PyWrapper_BIND(DbMLeader)
		.def("__repr__", &PyDbMLeader::toString)
		;
}

