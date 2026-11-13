#include "stdafx.h"
#include "PyErrorStatusException.h"
#include "PyDbViewportTableIterator.h"
#include "PyDbViewportTable.h"

PyDbViewportTableIterator::PyDbViewportTableIterator(PyDbViewportTable& owner, bool atBeginning, bool skipDeleted) {
    if (!owner.impObj()) { reset(nullptr); return; }
    AcDbViewportTableIterator* p = nullptr;
    if (!PyWarnIfError(owner.impObj()->newIterator(p, atBeginning, skipDeleted),
                       L"AcDbViewportTable::newIterator")) {
        reset(nullptr);
        return;
    }
    reset(static_cast<AcDbAbstractViewTableIterator*>(p));
}

PyDbViewportTableIterator::PyDbViewportTableIterator(AcDbViewportTableIterator* p)
    : PyDbAbstractViewTableIterator(static_cast<AcDbAbstractViewTableIterator*>(p)) {}

AcDbViewportTableIterator* PyDbViewportTableIterator::impObj() {
    return static_cast<AcDbViewportTableIterator*>(PyDbAbstractViewTableIterator::impObj());
}
const AcDbViewportTableIterator* PyDbViewportTableIterator::impObj() const {
    return static_cast<const AcDbViewportTableIterator*>(PyDbAbstractViewTableIterator::impObj());
}

void bind_PyDbViewportTableIterator(pym& m) {
    py::class_<PyDbViewportTableIterator, PyDbAbstractViewTableIterator>(m, "PyDbViewportTableIterator")
        .def(py::init<PyDbViewportTable&, bool, bool>(),
             py::arg("owner"), py::arg("atBeginning") = true, py::arg("skipDeleted") = true,
             "从 PyDbViewportTable 创建迭代器")
        .def(py::init<AcDbViewportTableIterator*>(), py::arg("p"), "指针接管构造（析构 delete）");
}
