#include "stdafx.h"
#include "PyDbTextEditorParagraphIterator.h"
#include <dbtexteditor.h>

PyDbTextEditorParagraphIterator::~PyDbTextEditorParagraphIterator() {
	if (m_p) delete m_p;
	m_p = nullptr;
}

bool PyDbTextEditorParagraphIterator::done() {
	if (!m_p) return true;
	return m_p->done();
}

bool PyDbTextEditorParagraphIterator::step() {
	if (!m_p) return false;
	m_p->next();
	return !m_p->done();
}

void bind_PyDbTextEditorParagraphIterator(pym& m) {
	py::class_<PyDbTextEditorParagraphIterator>(m, "PyDbTextEditorParagraphIterator")
		.def(py::init<AcDbTextEditorParagraphIterator*>(), py::arg("p"),
			"接管已创建的迭代器指针（析构 delete）。暂无公开创建路径，待 AcDbTextEditor 封装后可用。")
		.def("done", &PyDbTextEditorParagraphIterator::done, "迭代是否到达末尾。")
		.def("step", &PyDbTextEditorParagraphIterator::step, "前进一位，返回是否仍有元素。")
		.def("__iter__", [](PyDbTextEditorParagraphIterator& self) -> PyDbTextEditorParagraphIterator& {
			return self;
		})
		.def("__next__", [](PyDbTextEditorParagraphIterator& self) -> uint64_t {
			auto* p = self.impObj();
			if (!p || p->done()) throw py::stop_iteration();
			AcDbTextEditorParagraph* cur = p->current();
			p->next();
			return cur == nullptr ? 0ULL : static_cast<uint64_t>(reinterpret_cast<uintptr_t>(cur));
		});
}
