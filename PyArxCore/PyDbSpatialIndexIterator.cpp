#include "stdafx.h"
#include "PyErrorStatusException.h"
#include "PyDbSpatialIndexIterator.h"
#include <dbspindx.h>

PyDbSpatialIndexIterator::~PyDbSpatialIndexIterator() {
	if (m_p) delete m_p;
	m_p = nullptr;
}

bool PyDbSpatialIndexIterator::done() {
	if (!m_p) return true;
	return m_p->id().isNull();
}

bool PyDbSpatialIndexIterator::step() {
	if (!m_p) return false;
	AcDbObjectId nxt = m_p->next();
	return !nxt.isNull();
}

void bind_PyDbSpatialIndexIterator(pym& m) {
	py::class_<PyDbSpatialIndexIterator>(m, "PyDbSpatialIndexIterator")
		.def(py::init<AcDbSpatialIndexIterator*>(), py::arg("p"),
			"接管已创建的迭代器指针（析构 delete）。暂无公开创建路径，待 AcDbSpatialIndex/AcDbSpatialFilter 封装后可用。")
		.def("done", &PyDbSpatialIndexIterator::done, "迭代是否到达末尾（当前 id 为空）。")
		.def("step", &PyDbSpatialIndexIterator::step, "前进一位，返回是否仍有元素。")
		.def("__iter__", [](PyDbSpatialIndexIterator& self) -> PyDbSpatialIndexIterator& {
			if (self.impObj()) PyWarnIfError(self.impObj()->start(), L"AcDbSpatialIndexIterator::start");
			return self;
		})
		.def("__next__", [](PyDbSpatialIndexIterator& self) -> uint64_t {
			auto* p = self.impObj();
			if (!p || p->id().isNull()) throw py::stop_iteration();
			AcDbObjectId id = p->id();
			p->next();
			return id.isNull() ? 0ULL : static_cast<uint64_t>(id.asOldId());
		});
}
