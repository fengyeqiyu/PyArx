#include "stdafx.h"
#include "PyErrorStatusException.h"
#include "PyDbBlockTableIterator.h"
#include "PyDbBlockTable.h"
#include "PyDbBlockTableRecord.h"

PyDbBlockTableIterator::PyDbBlockTableIterator(PyDbBlockTable& owner, bool atBeginning, bool skipDeleted)
	: PyDbSymbolTableIterator(nullptr) {
	AcDbBlockTableIterator* p = nullptr;
	if (!PyWarnIfError(owner.impObj()->newIterator(p, atBeginning, skipDeleted), L"AcDbBlockTable::newIterator"))
		p = nullptr;
	m_pObj = p;
}

void bind_PyDbBlockTableIterator(pym& m) {
	py::class_<PyDbBlockTableIterator, PyDbSymbolTableIterator>(m, "PyDbBlockTableIterator")
		.def(py::init<PyDbBlockTable&, bool, bool>(),
			py::arg("owner"), py::arg("atBeginning") = true, py::arg("skipDeleted") = true,
			"从持有者 BlockTable 创建迭代器（接管指针，析构 delete）")
		.def(py::init<AcDbBlockTableIterator*>(), py::arg("p"), "接管裸指针")
		.def("__next__", [](PyDbBlockTableIterator& s) -> PyDbBlockTableRecord {
			auto* pIter = static_cast<AcDbBlockTableIterator*>(s.impObj());
			if (pIter->done()) throw py::stop_iteration();
			AcDbBlockTableRecord* pRec = nullptr;
			if (!PyWarnIfError(pIter->getRecord(pRec, AcDb::kForRead, false), L"AcDbBlockTableIterator::getRecord"))
				pRec = nullptr;
			pIter->step();
			return PyDbBlockTableRecord(pRec);
		}, "返回当前 BlockTableRecord 包装；失败返回空包装")
		;
}
