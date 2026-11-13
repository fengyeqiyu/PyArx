#include "stdafx.h"
#include "PyErrorStatusException.h"
#include "PyDbObjectIterator.h"

AcDbObjectIterator* PyDbObjectIterator::impObj() { return static_cast<AcDbObjectIterator*>(m_ptr); }
AcDbObjectIterator* PyDbObjectIterator::impObj() const { return static_cast<AcDbObjectIterator*>(m_ptr); }

void bind_PyDbObjectIterator(pym& m) {
	py::class_<PyDbObjectIterator>(m, "PyDbObjectIterator")
		.def(py::init<AcDbObjectIterator*>(), py::arg("p"), "接管已 new 出来的 AcDbObjectIterator 指针（析构 delete）")
		.def("done", [](PyDbObjectIterator& s) { return s.impObj()->done(); }, "是否已迭代到末尾")
		.def("step", [](PyDbObjectIterator& s, bool backwards, bool skipDeleted) {
			s.impObj()->step(backwards, skipDeleted);
			return true;
		}, py::arg("backwards") = false, py::arg("skipDeleted") = true, "前进一步，恒返回 true")
		.def("objectId", [](PyDbObjectIterator& s) -> uint64_t {
			AcDbObjectId id = s.impObj()->objectId();
			return id.isNull() ? 0ULL : static_cast<uint64_t>(id.asOldId());
		}, "当前位置实体的 ObjectId（uint64）；空位置返回 0")
		.def("__iter__", [](PyDbObjectIterator& s) -> PyDbObjectIterator& { return s; })
		// 库驻留实体 entity() 返回 NULL，故 __next__ 统一返回 objectId(uint64)
		.def("__next__", [](PyDbObjectIterator& s) -> uint64_t {
			if (s.impObj()->done()) throw py::stop_iteration();
			AcDbObjectId id = s.impObj()->objectId();
			uint64_t out = id.isNull() ? 0ULL : static_cast<uint64_t>(id.asOldId());
			s.impObj()->step();
			return out;
		})
		;
}
