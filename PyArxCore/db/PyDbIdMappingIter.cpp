#include "stdafx.h"
#include "PyDbIdMappingIter.h"
#include "PyDbIdMapping.h"
#include <dbidmap.h>

PyDbIdMappingIter::~PyDbIdMappingIter() {
	if (m_p) delete m_p;
	m_p = nullptr;
}

PyDbIdMappingIter::PyDbIdMappingIter(const AcDbIdMapping* pOwner)
	: m_p(pOwner ? new AcDbIdMappingIter(*pOwner) : nullptr) {}

bool PyDbIdMappingIter::done() {
	if (!m_p) return true;
	return m_p->done();
}

bool PyDbIdMappingIter::step() {
	if (!m_p) return false;
	return m_p->next();
}

void bind_PyDbIdMappingIter(pym& m) {
	py::class_<PyDbIdMappingIter>(m, "PyDbIdMappingIter")
		.def(py::init([](PyDbIdMapping& owner) {
			return std::make_unique<PyDbIdMappingIter>(owner.impObj()).release();
		}), py::arg("owner"),
			"从 PyDbIdMapping 创建迭代器（本包装接管并析构 delete）。")
		.def(py::init<AcDbIdMappingIter*>(), py::arg("p"),
			"接管已创建的迭代器指针（析构 delete）。")
		.def("done", &PyDbIdMappingIter::done, "迭代是否到达末尾。")
		.def("step", &PyDbIdMappingIter::step, "前进一位，返回是否仍有元素。")
		.def("__iter__", [](PyDbIdMappingIter& self) -> PyDbIdMappingIter& {
			if (self.impObj()) self.impObj()->start();
			return self;
		})
		.def("__next__", [](PyDbIdMappingIter& self) -> std::tuple<uint64_t, uint64_t> {
			auto* p = self.impObj();
			if (!p || p->done()) throw py::stop_iteration();
			AcDbIdPair pair;
			uint64_t key = 0ULL, val = 0ULL;
			if (p->getMap(pair)) {
				key = pair.key().isNull() ? 0ULL : static_cast<uint64_t>(pair.key().asOldId());
				val = pair.value().isNull() ? 0ULL : static_cast<uint64_t>(pair.value().asOldId());
			}
			p->next();
			return { key, val };
		});
}
