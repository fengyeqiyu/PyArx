#include "stdafx.h"
#include "PyErrorStatusException.h"
#include "PyDbXrecordIterator.h"
#include "PyDbXrecord.h"
#include <dbxrecrd.h>

PyDbXrecordIterator::~PyDbXrecordIterator() {
	if (m_p) delete m_p;
	m_p = nullptr;
}

PyDbXrecordIterator::PyDbXrecordIterator(AcDbXrecord* pOwner)
	: m_p(pOwner ? new AcDbXrecordIterator(pOwner) : nullptr) {}

bool PyDbXrecordIterator::done() {
	if (!m_p) return true;
	return m_p->done();
}

bool PyDbXrecordIterator::step() {
	if (!m_p) return false;
	return PyWarnIfError(m_p->next(), L"AcDbXrecordIterator::next");
}

void bind_PyDbXrecordIterator(pym& m) {
	py::class_<PyDbXrecordIterator>(m, "PyDbXrecordIterator")
		.def(py::init([](PyDbXrecord& owner, bool /*atBeginning*/) {
			return std::make_unique<PyDbXrecordIterator>(owner.impObj()).release();
		}), py::arg("owner"), py::arg("atBeginning") = true,
			"从 PyDbXrecord 创建迭代器（本包装接管并析构 delete）。")
		.def(py::init<AcDbXrecordIterator*>(), py::arg("p"),
			"接管已创建的迭代器指针（析构 delete）。")
		.def("done", &PyDbXrecordIterator::done, "迭代是否到达末尾。")
		.def("step", &PyDbXrecordIterator::step, "前进一位，返回是否仍有元素。")
		.def("__iter__", [](PyDbXrecordIterator& self) -> PyDbXrecordIterator& {
			if (self.impObj()) self.impObj()->start();
			return self;
		})
		.def("__next__", [](PyDbXrecordIterator& self) -> int {
			auto* p = self.impObj();
			if (!p || p->done()) throw py::stop_iteration();
			int restype = p->curRestype();
			p->next();
			return restype;
		});
}
