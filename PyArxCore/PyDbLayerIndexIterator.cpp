#include "stdafx.h"
#include "PyErrorStatusException.h"
#include "PyDbLayerIndexIterator.h"
#include <dblyindx.h>

PyDbLayerIndexIterator::~PyDbLayerIndexIterator() {
	if (m_p) delete m_p;
	m_p = nullptr;
}

bool PyDbLayerIndexIterator::done() {
	if (!m_p) return true;
	return m_p->id().isNull();
}

bool PyDbLayerIndexIterator::step() {
	if (!m_p) return false;
	AcDbObjectId nxt = m_p->next();
	return !nxt.isNull();
}

void bind_PyDbLayerIndexIterator(pym& m) {
	py::class_<PyDbLayerIndexIterator>(m, "PyDbLayerIndexIterator")
		.def(py::init<AcDbLayerIndexIterator*>(), py::arg("p"),
			"接管已创建的迭代器指针（析构 delete）。暂无公开创建路径，待 AcDbLayerIndex/AcDbLayerFilter 封装后可用。")
		.def("done", &PyDbLayerIndexIterator::done, "迭代是否到达末尾（当前 id 为空）。")
		.def("step", &PyDbLayerIndexIterator::step, "前进一位，返回是否仍有元素。")
		.def("__iter__", [](PyDbLayerIndexIterator& self) -> PyDbLayerIndexIterator& {
			if (self.impObj()) PyWarnIfError(self.impObj()->start(), L"AcDbLayerIndexIterator::start");
			return self;
		})
		.def("__next__", [](PyDbLayerIndexIterator& self) -> uint64_t {
			auto* p = self.impObj();
			if (!p || p->id().isNull()) throw py::stop_iteration();
			AcDbObjectId id = p->id();
			p->next();
			return id.isNull() ? 0ULL : static_cast<uint64_t>(id.asOldId());
		});
}
