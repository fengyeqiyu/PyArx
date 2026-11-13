#include "stdafx.h"
#include "PyErrorStatusException.h"
#include "PyDbLongTransWorkSetIterator.h"
#include <dbltrans.h>

PyDbLongTransWorkSetIterator::~PyDbLongTransWorkSetIterator() {
	if (m_p) delete m_p;
	m_p = nullptr;
}

bool PyDbLongTransWorkSetIterator::done() {
	if (!m_p) return true;
	return m_p->done();
}

bool PyDbLongTransWorkSetIterator::step() {
	if (!m_p) return false;
	m_p->step();
	return !m_p->done();
}

void bind_PyDbLongTransWorkSetIterator(pym& m) {
	py::class_<PyDbLongTransWorkSetIterator>(m, "PyDbLongTransWorkSetIterator")
		.def(py::init<AcDbLongTransWorkSetIterator*>(), py::arg("p"),
			"接管已创建的迭代器指针（析构 delete）。暂无公开创建路径，待 PyDbLongTransaction 封装后可用。")
		.def("done", &PyDbLongTransWorkSetIterator::done, "迭代是否到达末尾。")
		.def("step", &PyDbLongTransWorkSetIterator::step, "前进一位，返回是否仍有元素。")
		.def("__iter__", [](PyDbLongTransWorkSetIterator& self) -> PyDbLongTransWorkSetIterator& {
			if (self.impObj()) self.impObj()->start(false, false);
			return self;
		})
		.def("__next__", [](PyDbLongTransWorkSetIterator& self) -> uint64_t {
			auto* p = self.impObj();
			if (!p || p->done()) throw py::stop_iteration();
			auto id = p->objectId();
			p->step();
			return id.isNull() ? 0ULL : static_cast<uint64_t>(id.asOldId());
		});
}
