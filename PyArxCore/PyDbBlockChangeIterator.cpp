#include "stdafx.h"
#include "PyDbBlockChangeIterator.h"
#include <dbindex.h>

// SDK 析构为 private（ctor 也私有，由引擎内部创建/销毁）。本包装不接管生命周期、不 delete，
// 仅持有非拥有指针；析构仅置空，避免调用不可访问的私有 ~AcDbBlockChangeIterator。
PyDbBlockChangeIterator::~PyDbBlockChangeIterator() {
	m_p = nullptr;
}

bool PyDbBlockChangeIterator::done() {
	if (!m_p) return true;
	return m_p->done();
}

bool PyDbBlockChangeIterator::step() {
	if (!m_p) return false;
	m_p->next();
	return !m_p->done();
}

void bind_PyDbBlockChangeIterator(pym& m) {
	py::class_<PyDbBlockChangeIterator>(m, "PyDbBlockChangeIterator")
		.def(py::init<AcDbBlockChangeIterator*>(), py::arg("p"),
			"接管已创建的迭代器指针（析构 delete）。暂无公开创建路径，待源类封装后可用。")
		.def("done", &PyDbBlockChangeIterator::done, "迭代是否到达末尾。")
		.def("step", &PyDbBlockChangeIterator::step, "前进一位，返回是否仍有元素。")
		.def("__iter__", [](PyDbBlockChangeIterator& self) -> PyDbBlockChangeIterator& {
			if (self.impObj()) self.impObj()->start();
			return self;
		})
		.def("__next__", [](PyDbBlockChangeIterator& self) -> uint64_t {
			auto* p = self.impObj();
			if (!p || p->done()) throw py::stop_iteration();
			AcDbObjectId id = p->id();
			p->next();
			return id.isNull() ? 0ULL : static_cast<uint64_t>(id.asOldId());
		});
}
