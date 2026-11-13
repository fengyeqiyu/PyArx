#include "stdafx.h"
#include "PyErrorStatusException.h"
#include "PyDbTextStyleTableIterator.h"
#include "PyDbTextStyleTable.h"
#include "PyDbTextStyleTableRecord.h"

PyDbTextStyleTableIterator::PyDbTextStyleTableIterator(PyDbTextStyleTable& owner, bool atBeginning, bool skipDeleted)
	: PyDbSymbolTableIterator(nullptr) {
	AcDbTextStyleTableIterator* p = nullptr;
	if (!PyWarnIfError(owner.impObj()->newIterator(p, atBeginning, skipDeleted), L"AcDbTextStyleTable::newIterator"))
		p = nullptr;
	m_pObj = p;
}

void bind_PyDbTextStyleTableIterator(pym& m) {
	py::class_<PyDbTextStyleTableIterator, PyDbSymbolTableIterator>(m, "PyDbTextStyleTableIterator")
		.def(py::init<PyDbTextStyleTable&, bool, bool>(),
			py::arg("owner"), py::arg("atBeginning") = true, py::arg("skipDeleted") = true,
			"从持有者 TextStyleTable 创建迭代器（接管指针，析构 delete）")
		.def(py::init<AcDbTextStyleTableIterator*>(), py::arg("p"), "接管裸指针")
		.def("__next__", [](PyDbTextStyleTableIterator& s) -> PyDbTextStyleTableRecord {
			auto* pIter = static_cast<AcDbTextStyleTableIterator*>(s.impObj());
			if (pIter->done()) throw py::stop_iteration();
			AcDbTextStyleTableRecord* pRec = nullptr;
			if (!PyWarnIfError(pIter->getRecord(pRec, AcDb::kForRead, false), L"AcDbTextStyleTableIterator::getRecord"))
				pRec = nullptr;
			pIter->step();
			return PyDbTextStyleTableRecord(pRec);
		}, "返回当前 TextStyleTableRecord 包装；失败返回空包装")
		;
}
