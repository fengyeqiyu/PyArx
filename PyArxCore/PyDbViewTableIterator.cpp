#include "stdafx.h"
#include "PyErrorStatusException.h"
#include "PyDbViewTableIterator.h"
#include "PyDbViewTable.h"

PyDbViewTableIterator::PyDbViewTableIterator(PyDbViewTable& owner, bool atBeginning, bool skipDeleted) {
    if (!owner.impObj()) { reset(nullptr); return; }
    AcDbViewTableIterator* p = nullptr;
    if (!PyWarnIfError(owner.impObj()->newIterator(p, atBeginning, skipDeleted),
                       L"AcDbViewTable::newIterator")) {
        reset(nullptr);
        return;
    }
    reset(static_cast<AcDbAbstractViewTableIterator*>(p));
}

PyDbViewTableIterator::PyDbViewTableIterator(AcDbViewTableIterator* p)
    : PyDbAbstractViewTableIterator(static_cast<AcDbAbstractViewTableIterator*>(p)) {}

AcDbViewTableIterator* PyDbViewTableIterator::impObj() {
    return static_cast<AcDbViewTableIterator*>(PyDbAbstractViewTableIterator::impObj());
}
const AcDbViewTableIterator* PyDbViewTableIterator::impObj() const {
    return static_cast<const AcDbViewTableIterator*>(PyDbAbstractViewTableIterator::impObj());
}

void bind_PyDbViewTableIterator(pym& m) {
    py::class_<PyDbViewTableIterator, PyDbAbstractViewTableIterator>(m, "PyDbViewTableIterator")
        .def(py::init<PyDbViewTable&, bool, bool>(),
             py::arg("owner"), py::arg("atBeginning") = true, py::arg("skipDeleted") = true,
             "从 PyDbViewTable 创建迭代器")
        .def(py::init<AcDbViewTableIterator*>(), py::arg("p"), "指针接管构造（析构 delete）");
}
