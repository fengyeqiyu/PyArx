#include "stdafx.h"
#include "PyErrorStatusException.h"
#include "PyDbCompositeFilteredBlockIterator.h"

PyDbCompositeFilteredBlockIterator::PyDbCompositeFilteredBlockIterator(AcDbCompositeFilteredBlockIterator* p)
    : m_pIter(p) {}
PyDbCompositeFilteredBlockIterator::~PyDbCompositeFilteredBlockIterator() { delete m_pIter; }

bool PyDbCompositeFilteredBlockIterator::start() {
    if (!m_pIter) return false;
    return PyWarnIfError(m_pIter->start(), L"AcDbCompositeFilteredBlockIterator::start");
}

uint64_t PyDbCompositeFilteredBlockIterator::currentId() const {
    if (!m_pIter) return 0ULL;
    AcDbObjectId id = m_pIter->id();
    if (id.isNull()) return 0ULL;
    return static_cast<uint64_t>(id.asOldId());
}

uint64_t PyDbCompositeFilteredBlockIterator::nextId() {
    if (!m_pIter) return 0ULL;
    AcDbObjectId id = m_pIter->next();
    if (id.isNull()) return 0ULL;
    return static_cast<uint64_t>(id.asOldId());
}

uint64_t PyDbCompositeFilteredBlockIterator::__next__() {
    if (!m_pIter) throw py::stop_iteration();
    AcDbObjectId id = m_pIter->next();
    if (id.isNull()) throw py::stop_iteration();
    return static_cast<uint64_t>(id.asOldId());
}

void bind_PyDbCompositeFilteredBlockIterator(pym& m) {
    py::class_<PyDbCompositeFilteredBlockIterator>(m, "PyDbCompositeFilteredBlockIterator")
        .def(py::init<AcDbCompositeFilteredBlockIterator*>(), py::arg("p"),
             "指针接管构造（析构 delete）；init() 需 AcDbFilter 数组，工程未封装")
        .def("start", &PyDbCompositeFilteredBlockIterator::start, "复位迭代（须先完成 init）")
        .def("currentId", &PyDbCompositeFilteredBlockIterator::currentId, "当前 ObjectId（uint64）；失败返回 0")
        .def("nextId", &PyDbCompositeFilteredBlockIterator::nextId, "前进并返回下一个 ObjectId（uint64）；结束返回 0")
        .def("__iter__", [](PyDbCompositeFilteredBlockIterator& it) -> PyDbCompositeFilteredBlockIterator& { it.start(); return it; })
        .def("__next__", &PyDbCompositeFilteredBlockIterator::__next__);
}
