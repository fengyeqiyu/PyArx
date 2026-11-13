#include "stdafx.h"
#include "PyErrorStatusException.h"
#include "PyDbSymbolTableIterator.h"
#include "PyDbSymbolTableRecord.h"

void bind_PyDbSymbolTableIterator(pym& m) {
	py::class_<PyDbSymbolTableIterator>(m, "PyDbSymbolTableIterator")
		.def(py::init<AcDbSymbolTableIterator*>(), py::arg("p"), "接管已 new 出来的 AcDbSymbolTableIterator 指针（析构 delete）")
		.def("done", [](const PyDbSymbolTableIterator& s) { return s.impObj()->done(); }, "是否已迭代到末尾")
		.def("step", [](PyDbSymbolTableIterator& s, bool forward, bool skipDeleted) {
			s.impObj()->step(forward, skipDeleted);
			return true;
		}, py::arg("forward") = true, py::arg("skipDeleted") = true, "前进一步，恒返回 true")
		.def("getRecordId", [](const PyDbSymbolTableIterator& s) -> uint64_t {
			AcDbObjectId id;
			if (!PyWarnIfError(s.impObj()->getRecordId(id), L"AcDbSymbolTableIterator::getRecordId")) return 0ULL;
			return id.isNull() ? 0ULL : static_cast<uint64_t>(id.asOldId());
		}, "当前记录的 ObjectId（uint64）；失败返回 0")
		.def("__iter__", [](PyDbSymbolTableIterator& s) -> PyDbSymbolTableIterator& { return s; })
		// 优先 getRecord 打开记录返回 PyDbSymbolTableRecord；失败返回空包装（isNull() 为真），不抛（StopIteration 除外）
		.def("__next__", [](PyDbSymbolTableIterator& s) -> PyDbSymbolTableRecord {
			if (s.impObj()->done()) throw py::stop_iteration();
			AcDbSymbolTableRecord* pRec = nullptr;
			if (!PyWarnIfError(s.impObj()->getRecord(pRec, AcDb::kForRead, false), L"AcDbSymbolTableIterator::getRecord"))
				pRec = nullptr;
			s.impObj()->step();
			return PyDbSymbolTableRecord(pRec);
		})
		;
}
