#include "stdafx.h"
#include "PyDbEvalContextIterator.h"
#include <dbeval.h>

PyDbEvalContextIterator::~PyDbEvalContextIterator() {
	if (m_p) delete m_p;
	m_p = nullptr;
}

bool PyDbEvalContextIterator::done() {
	if (!m_p) return true;
	return m_p->done();
}

bool PyDbEvalContextIterator::step() {
	if (!m_p) return false;
	return m_p->next();
}

void bind_PyDbEvalContextIterator(pym& m) {
	py::class_<PyDbEvalContextIterator>(m, "PyDbEvalContextIterator")
		.def(py::init<AcDbEvalContextIterator*>(), py::arg("p"),
			"接管已创建的迭代器指针（析构 delete）。暂无公开创建路径，待 AcDbEvalContext 封装后可用。")
		.def("done", &PyDbEvalContextIterator::done, "迭代是否到达末尾。")
		.def("step", &PyDbEvalContextIterator::step, "前进一位，返回是否仍有元素。")
		.def("__iter__", [](PyDbEvalContextIterator& self) -> PyDbEvalContextIterator& {
			if (self.impObj()) self.impObj()->start();
			return self;
		})
		.def("__next__", [](PyDbEvalContextIterator& self) -> AcString {
			auto* p = self.impObj();
			if (!p || p->done()) throw py::stop_iteration();
			AcDbEvalContextPair pair = p->contextPair();
			const ACHAR* key = pair.key();
			p->next();
			return key ? AcString(key) : AcString();
		});
}
