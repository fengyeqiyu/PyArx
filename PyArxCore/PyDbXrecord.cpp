#include "stdafx.h"
#include "PyErrorStatusException.h"
#include "PyDbXrecord.h"
#include "PyResbuf.h"

PyWrapper_Cpp(DbXrecord);

bool PyDbXrecord::isXlateReferences() const {
	if (!impObj()) return false;
	return impObj()->isXlateReferences();
}

bool PyDbXrecord::setXlateReferences(bool translate) {
	if (!impObj()) return false;
	impObj()->setXlateReferences(translate);
	return true;
}

AcString PyDbXrecord::toString() const {
	return AcString(std::format("PyDbXrecord[xlateRef={}]", isXlateReferences()).c_str(), AcString::Utf8);
}

static PyResbuf xrecChain(const PyDbXrecord& self) {
	if (!self.impObj()) return PyResbuf(nullptr);
	resbuf* rb = nullptr;
	if (self.impObj()->rbChain(&rb, self.impObj()->database()) != Acad::eOk) rb = nullptr;
	return PyResbuf(rb);
}

static py::list xrecTags(const PyDbXrecord& self) {
	py::list out;
	for (const auto& tv : xrecChain(self).toList()) out.append(py::cast(tv));
	return out;
}
void bind_PyDbXrecord(pym& m) {
	py::class_<PyDbXrecord, PyDbObject>(m, "PyDbXrecord")
		.def(py::init<>())
		.def(py::init<AcDbXrecord*>(), py::arg("object"))
		.def("isXlateReferences", &PyDbXrecord::isXlateReferences)
		.def("setXlateReferences", &PyDbXrecord::setXlateReferences, py::arg("translate"))
		// resbuf 链读写：rbChain 输出链由 PyResbuf 接管释放；setFromRbChain 入参 Resbuf。
		.def("rbChain", [](const PyDbXrecord& self) -> PyResbuf {
			if (!self.impObj()) return PyResbuf(nullptr);
			resbuf* rb = nullptr;
			if (self.impObj()->rbChain(&rb, self.impObj()->database()) != Acad::eOk) rb = nullptr;
			return PyResbuf(rb);
		}, "读取 xrecord 内容为 Resbuf 链（空则 isNull()=True）")
		.def("setFromRbChain", [](PyDbXrecord& self, const PyResbuf& rb) {
			if (!self.impObj() || rb.isNull()) return false;
			return PyWarnIfError(self.impObj()->setFromRbChain(*rb.data(), self.impObj()->database()), L"AcDbXrecord::setFromRbChain");
		}, py::arg("chain"), "由 Resbuf 链设置 xrecord 内容")
		.def("appendRbChain", [](PyDbXrecord& self, const PyResbuf& rb) -> bool {
			if (!self.impObj() || rb.isNull()) return false;
			return PyWarnIfError(self.impObj()->appendRbChain(*rb.data()), L"AcDbXrecord::appendRbChain");
		}, py::arg("chain"), "在现有 resbuf 链尾追加数据（不读取/回写整条链，效率更高）")
		.def("mergeStyle", [](const PyDbXrecord& self) -> int {
			if (!self.impObj()) return 0;
			return static_cast<int>(self.impObj()->mergeStyle());
		}, "返回 AcDb::DuplicateRecordCloning 整数")
		.def("setMergeStyle", [](PyDbXrecord& self, int style) {
			if (!self.impObj()) return;
			self.impObj()->setMergeStyle(static_cast<AcDb::DuplicateRecordCloning>(style));
		}, py::arg("style"), "设置 AcDb::DuplicateRecordCloning")
		// ---- 遍历 + 索引：AcDbXrecordIterator 折叠进 Xrecord 本体 ----
		.def("tags", &xrecTags, "把 xrecord 内容展开为 [TypedValue] 列表")
		.def("__len__", [](const PyDbXrecord& self) { return (size_t)xrecChain(self).toList().size(); }, "len(xrec)")
		.def("__getitem__", [](const PyDbXrecord& self, int i) {
			auto list = xrecTags(self);
			const int n = (int)list.size();
			if (i < 0) i += n;
			if (i < 0 || i >= n) throw py::index_error("Xrecord 索引越界");
			return list[i];
		}, py::arg("index"), "xrec[i] 取第 i 个组码对（支持负索引）")
		.def("__iter__", [](const PyDbXrecord& self) {
			return xrecTags(self).attr("__iter__")();
		}, "for tv in xrec：遍历组码对")		PyWrapper_BIND(DbXrecord)
		.def("__repr__", &PyDbXrecord::toString);
}
