#include "stdafx.h"
#include "PyErrorStatusException.h"
#include "PyDbSummaryInfo.h"

AcString PyDbDatabaseSummaryInfo::title() const {
	if (!impObj()) return AcString();
	AcString s;
	if (!PyWarnIfError(impObj()->getTitle(s), L"SummaryInfo.getTitle")) return AcString();
	return s;
}
bool PyDbDatabaseSummaryInfo::setTitle(const AcString& v) {
	if (!impObj()) return false;
	return impObj() ? PyWarnIfError(impObj()->setTitle(v.kwszPtr()), L"SummaryInfo.setTitle") : false;
}

AcString PyDbDatabaseSummaryInfo::subject() const {
	if (!impObj()) return AcString();
	AcString s;
	if (!PyWarnIfError(impObj()->getSubject(s), L"SummaryInfo.getSubject")) return AcString();
	return s;
}
bool PyDbDatabaseSummaryInfo::setSubject(const AcString& v) {
	if (!impObj()) return false;
	return impObj() ? PyWarnIfError(impObj()->setSubject(v.kwszPtr()), L"SummaryInfo.setSubject") : false;
}

AcString PyDbDatabaseSummaryInfo::author() const {
	if (!impObj()) return AcString();
	AcString s;
	if (!PyWarnIfError(impObj()->getAuthor(s), L"SummaryInfo.getAuthor")) return AcString();
	return s;
}
bool PyDbDatabaseSummaryInfo::setAuthor(const AcString& v) {
	if (!impObj()) return false;
	return impObj() ? PyWarnIfError(impObj()->setAuthor(v.kwszPtr()), L"SummaryInfo.setAuthor") : false;
}

AcString PyDbDatabaseSummaryInfo::keywords() const {
	if (!impObj()) return AcString();
	AcString s;
	if (!PyWarnIfError(impObj()->getKeywords(s), L"SummaryInfo.getKeywords")) return AcString();
	return s;
}
bool PyDbDatabaseSummaryInfo::setKeywords(const AcString& v) {
	if (!impObj()) return false;
	return impObj() ? PyWarnIfError(impObj()->setKeywords(v.kwszPtr()), L"SummaryInfo.setKeywords") : false;
}

AcString PyDbDatabaseSummaryInfo::comments() const {
	if (!impObj()) return AcString();
	AcString s;
	if (!PyWarnIfError(impObj()->getComments(s), L"SummaryInfo.getComments")) return AcString();
	return s;
}
bool PyDbDatabaseSummaryInfo::setComments(const AcString& v) {
	if (!impObj()) return false;
	return impObj() ? PyWarnIfError(impObj()->setComments(v.kwszPtr()), L"SummaryInfo.setComments") : false;
}

AcString PyDbDatabaseSummaryInfo::lastSavedBy() const {
	if (!impObj()) return AcString();
	AcString s;
	if (!PyWarnIfError(impObj()->getLastSavedBy(s), L"SummaryInfo.getLastSavedBy")) return AcString();
	return s;
}
bool PyDbDatabaseSummaryInfo::setLastSavedBy(const AcString& v) {
	if (!impObj()) return false;
	return impObj() ? PyWarnIfError(impObj()->setLastSavedBy(v.kwszPtr()), L"SummaryInfo.setLastSavedBy") : false;
}

AcString PyDbDatabaseSummaryInfo::revisionNumber() const {
	if (!impObj()) return AcString();
	AcString s;
	if (!PyWarnIfError(impObj()->getRevisionNumber(s), L"SummaryInfo.getRevisionNumber")) return AcString();
	return s;
}
bool PyDbDatabaseSummaryInfo::setRevisionNumber(const AcString& v) {
	if (!impObj()) return false;
	return impObj() ? PyWarnIfError(impObj()->setRevisionNumber(v.kwszPtr()), L"SummaryInfo.setRevisionNumber") : false;
}

AcString PyDbDatabaseSummaryInfo::hyperlinkBase() const {
	if (!impObj()) return AcString();
	AcString s;
	if (!PyWarnIfError(impObj()->getHyperlinkBase(s), L"SummaryInfo.getHyperlinkBase")) return AcString();
	return s;
}
bool PyDbDatabaseSummaryInfo::setHyperlinkBase(const AcString& v) {
	if (!impObj()) return false;
	return impObj() ? PyWarnIfError(impObj()->setHyperlinkBase(v.kwszPtr()), L"SummaryInfo.setHyperlinkBase") : false;
}

int PyDbDatabaseSummaryInfo::numCustomInfo() const {
	if (!impObj()) return 0;
	return impObj() ? impObj()->numCustomInfo() : 0;
}

bool PyDbDatabaseSummaryInfo::addCustomInfo(const AcString& key, const AcString& value) {
	if (!impObj()) return false;
	return impObj() ? PyWarnIfError(impObj()->addCustomSummaryInfo(key.kwszPtr(), value.kwszPtr()), L"SummaryInfo.addCustomInfo") : false;
}

bool PyDbDatabaseSummaryInfo::deleteCustomInfoAt(int index) {
	if (!impObj()) return false;
	return impObj() ? PyWarnIfError(impObj()->deleteCustomSummaryInfo(index), L"SummaryInfo.deleteCustomInfoAt") : false;
}

bool PyDbDatabaseSummaryInfo::deleteCustomInfoByKey(const AcString& key) {
	if (!impObj()) return false;
	return impObj() ? PyWarnIfError(impObj()->deleteCustomSummaryInfo(key.kwszPtr()), L"SummaryInfo.deleteCustomInfoByKey") : false;
}

py::tuple PyDbDatabaseSummaryInfo::getCustomInfoAt(int index) const {
	if (!impObj()) return py::make_tuple(AcString(), AcString());
	AcString key, value;
	if (!PyWarnIfError(impObj()->getCustomSummaryInfo(index, key, value), L"SummaryInfo.getCustomInfoAt"))
		return py::make_tuple(AcString(), AcString());
	return py::make_tuple(key, value);
}

bool PyDbDatabaseSummaryInfo::setCustomInfoAt(int index, const AcString& key, const AcString& value) {
	if (!impObj()) return false;
	return impObj() ? PyWarnIfError(impObj()->setCustomSummaryInfo(index, key.kwszPtr(), value.kwszPtr()), L"SummaryInfo.setCustomInfoAt") : false;
}

AcString PyDbDatabaseSummaryInfo::getCustomInfoByKey(const AcString& key) const {
	if (!impObj()) return AcString();
	AcString value;
	if (!PyWarnIfError(impObj()->getCustomSummaryInfo(key.kwszPtr(), value), L"SummaryInfo.getCustomInfoByKey")) return AcString();
	return value;
}

bool PyDbDatabaseSummaryInfo::setCustomInfoByKey(const AcString& key, const AcString& value) {
	if (!impObj()) return false;
	return impObj() ? PyWarnIfError(impObj()->setCustomSummaryInfo(key.kwszPtr(), value.kwszPtr()), L"SummaryInfo.setCustomInfoByKey") : false;
}

AcString PyDbDatabaseSummaryInfo::toString() const {
	return AcString(std::format("PyDbDatabaseSummaryInfo[custom={}]", numCustomInfo()).c_str(), AcString::Utf8);
}

void bind_PyDbDatabaseSummaryInfo(pym& m) {
	py::class_<PyDbDatabaseSummaryInfo>(m, "PyDbDatabaseSummaryInfo")
		.def(py::init<AcDbDatabaseSummaryInfo*>(), py::arg("object"))
		.def("title", &PyDbDatabaseSummaryInfo::title)
		.def("setTitle", &PyDbDatabaseSummaryInfo::setTitle, py::arg("v"))
		.def("subject", &PyDbDatabaseSummaryInfo::subject)
		.def("setSubject", &PyDbDatabaseSummaryInfo::setSubject, py::arg("v"))
		.def("author", &PyDbDatabaseSummaryInfo::author)
		.def("setAuthor", &PyDbDatabaseSummaryInfo::setAuthor, py::arg("v"))
		.def("keywords", &PyDbDatabaseSummaryInfo::keywords)
		.def("setKeywords", &PyDbDatabaseSummaryInfo::setKeywords, py::arg("v"))
		.def("comments", &PyDbDatabaseSummaryInfo::comments)
		.def("setComments", &PyDbDatabaseSummaryInfo::setComments, py::arg("v"))
		.def("lastSavedBy", &PyDbDatabaseSummaryInfo::lastSavedBy)
		.def("setLastSavedBy", &PyDbDatabaseSummaryInfo::setLastSavedBy, py::arg("v"))
		.def("revisionNumber", &PyDbDatabaseSummaryInfo::revisionNumber)
		.def("setRevisionNumber", &PyDbDatabaseSummaryInfo::setRevisionNumber, py::arg("v"))
		.def("hyperlinkBase", &PyDbDatabaseSummaryInfo::hyperlinkBase)
		.def("setHyperlinkBase", &PyDbDatabaseSummaryInfo::setHyperlinkBase, py::arg("v"))
		.def("numCustomInfo", &PyDbDatabaseSummaryInfo::numCustomInfo)
		.def("addCustomInfo", &PyDbDatabaseSummaryInfo::addCustomInfo, py::arg("key"), py::arg("value"))
		.def("deleteCustomInfoAt", &PyDbDatabaseSummaryInfo::deleteCustomInfoAt, py::arg("index"))
		.def("deleteCustomInfoByKey", &PyDbDatabaseSummaryInfo::deleteCustomInfoByKey, py::arg("key"))
		.def("getCustomInfoAt", &PyDbDatabaseSummaryInfo::getCustomInfoAt, py::arg("index"))
		.def("setCustomInfoAt", &PyDbDatabaseSummaryInfo::setCustomInfoAt, py::arg("index"), py::arg("key"), py::arg("value"))
		.def("getCustomInfoByKey", &PyDbDatabaseSummaryInfo::getCustomInfoByKey, py::arg("key"))
		.def("setCustomInfoByKey", &PyDbDatabaseSummaryInfo::setCustomInfoByKey, py::arg("key"), py::arg("value"))
		.def("__repr__", &PyDbDatabaseSummaryInfo::toString);
}
