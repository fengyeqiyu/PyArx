#include "stdafx.h"
#include "PyDbIndexUpdateDataIterator.h"
#include <dbindex.h>

PyDbIndexUpdateDataIterator::~PyDbIndexUpdateDataIterator() {
	if (m_p) delete m_p;
	m_p = nullptr;
}

bool PyDbIndexUpdateDataIterator::done() {
	if (!m_p) return true;
	return m_p->done();
}

bool PyDbIndexUpdateDataIterator::step() {
	if (!m_p) return false;
	m_p->next();
	return !m_p->done();
}

void bind_PyDbIndexUpdateDataIterator(pym& m) {
	py::class_<PyDbIndexUpdateDataIterator>(m, "PyDbIndexUpdateDataIterator")
		.def(py::init<AcDbIndexUpdateDataIterator*>(), py::arg("p"),
			"接管已创建的迭代器指针（析构 delete）。暂无公开创建路径，待 AcDbIndexUpdateData 封装后可用。")
		.def("done", &PyDbIndexUpdateDataIterator::done, "迭代是否到达末尾。")
		.def("step", &PyDbIndexUpdateDataIterator::step, "前进一位，返回是否仍有元素。")
		.def("__iter__", [](PyDbIndexUpdateDataIterator& self) -> PyDbIndexUpdateDataIterator& {
			if (self.impObj()) self.impObj()->start();
			return self;
		})
		.def("__next__", [](PyDbIndexUpdateDataIterator& self) -> uint64_t {
			auto* p = self.impObj();
			if (!p || p->done()) throw py::stop_iteration();
			AcDbObjectId id = AcDbObjectId::kNull;
			Adesk::UInt8 flags = 0;
			Adesk::ULongPtr data = 0;
			if (p->currentData(id, flags, data) != Acad::eOk) return 0ULL;
			p->next();
			return id.isNull() ? 0ULL : static_cast<uint64_t>(id.asOldId());
		});
}
