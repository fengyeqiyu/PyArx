#include "stdafx.h"
#include "PyErrorStatusException.h"
#include "PyDbRegAppTableIterator.h"
#include "PyDbRegAppTable.h"
#include "PyDbRegAppTableRecord.h"

PyDbRegAppTableIterator::PyDbRegAppTableIterator(PyDbRegAppTable& owner, bool atBeginning, bool skipDeleted)
	: PyDbSymbolTableIterator(nullptr) {
	AcDbRegAppTableIterator* p = nullptr;
	if (!PyWarnIfError(owner.impObj()->newIterator(p, atBeginning, skipDeleted), L"AcDbRegAppTable::newIterator"))
		p = nullptr;
	m_pObj = p;
}

void bind_PyDbRegAppTableIterator(pym& m) {
	py::class_<PyDbRegAppTableIterator, PyDbSymbolTableIterator>(m, "PyDbRegAppTableIterator")
		.def(py::init<PyDbRegAppTable&, bool, bool>(),
			py::arg("owner"), py::arg("atBeginning") = true, py::arg("skipDeleted") = true,
			"从持有者 RegAppTable 创建迭代器（接管指针，析构 delete）")
		.def(py::init<AcDbRegAppTableIterator*>(), py::arg("p"), "接管裸指针")
		.def("__next__", [](PyDbRegAppTableIterator& s) -> PyDbRegAppTableRecord {
			auto* pIter = static_cast<AcDbRegAppTableIterator*>(s.impObj());
			if (pIter->done()) throw py::stop_iteration();
			AcDbRegAppTableRecord* pRec = nullptr;
			if (!PyWarnIfError(pIter->getRecord(pRec, AcDb::kForRead, false), L"AcDbRegAppTableIterator::getRecord"))
				pRec = nullptr;
			pIter->step();
			return PyDbRegAppTableRecord(pRec);
		}, "返回当前 RegAppTableRecord 包装；失败返回空包装")
		;
}
