#include "stdafx.h"
#include "PyErrorStatusException.h"
#include "PyDbFilteredBlockIterator.h"

PyDbFilteredBlockIterator::PyDbFilteredBlockIterator(AcDbFilteredBlockIterator* p) : m_pIter(p) {}
PyDbFilteredBlockIterator::~PyDbFilteredBlockIterator() { delete m_pIter; }

bool PyDbFilteredBlockIterator::start() {
    if (!m_pIter) return false;
    return PyWarnIfError(m_pIter->start(), L"AcDbFilteredBlockIterator::start");
}

uint64_t PyDbFilteredBlockIterator::currentId() const {
    if (!m_pIter) return 0ULL;
    AcDbObjectId id = m_pIter->id();
    if (id.isNull()) return 0ULL;
    return static_cast<uint64_t>(id.asOldId());
}

uint64_t PyDbFilteredBlockIterator::nextId() {
    if (!m_pIter) return 0ULL;
    AcDbObjectId id = m_pIter->next();
    if (id.isNull()) return 0ULL;
    return static_cast<uint64_t>(id.asOldId());
}

uint64_t PyDbFilteredBlockIterator::__next__() {
    if (!m_pIter) throw py::stop_iteration();
    AcDbObjectId id = m_pIter->next();
    if (id.isNull()) throw py::stop_iteration();
    return static_cast<uint64_t>(id.asOldId());
}

void bind_PyDbFilteredBlockIterator(pym& m) {
    py::class_<PyDbFilteredBlockIterator>(m, "PyDbFilteredBlockIterator")
        .def(py::init<AcDbFilteredBlockIterator*>(), py::arg("p"),
             "指针接管构造（析构 delete）；构造需 AcDbFilter 过滤条件，工程未封装")
        .def("start", &PyDbFilteredBlockIterator::start, "复位迭代")
        .def("currentId", &PyDbFilteredBlockIterator::currentId, "当前 ObjectId（uint64）；失败返回 0")
        .def("nextId", &PyDbFilteredBlockIterator::nextId, "前进并返回下一个 ObjectId（uint64）；结束返回 0")
        .def("__iter__", [](PyDbFilteredBlockIterator& it) -> PyDbFilteredBlockIterator& { it.start(); return it; })
        .def("__next__", &PyDbFilteredBlockIterator::__next__);
}
