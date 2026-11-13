#include "stdafx.h"
#include "PyErrorStatusException.h"
#include "PyDbDimStyleTableIterator.h"
#include "PyDbDimStyleTable.h"
#include "PyDbDimStyleTableRecord.h"

PyDbDimStyleTableIterator::PyDbDimStyleTableIterator(PyDbDimStyleTable& owner, bool atBeginning, bool skipDeleted)
	: PyDbSymbolTableIterator(nullptr) {
	AcDbDimStyleTableIterator* p = nullptr;
	if (!PyWarnIfError(owner.impObj()->newIterator(p, atBeginning, skipDeleted), L"AcDbDimStyleTable::newIterator"))
		p = nullptr;
	m_pObj = p;
}

void bind_PyDbDimStyleTableIterator(pym& m) {
	py::class_<PyDbDimStyleTableIterator, PyDbSymbolTableIterator>(m, "PyDbDimStyleTableIterator")
		.def(py::init<PyDbDimStyleTable&, bool, bool>(),
			py::arg("owner"), py::arg("atBeginning") = true, py::arg("skipDeleted") = true,
			"从持有者 DimStyleTable 创建迭代器（接管指针，析构 delete）")
		.def(py::init<AcDbDimStyleTableIterator*>(), py::arg("p"), "接管裸指针")
		.def("__next__", [](PyDbDimStyleTableIterator& s) -> PyDbDimStyleTableRecord {
			auto* pIter = static_cast<AcDbDimStyleTableIterator*>(s.impObj());
			if (pIter->done()) throw py::stop_iteration();
			AcDbDimStyleTableRecord* pRec = nullptr;
			if (!PyWarnIfError(pIter->getRecord(pRec, AcDb::kForRead, false), L"AcDbDimStyleTableIterator::getRecord"))
				pRec = nullptr;
			pIter->step();
			return PyDbDimStyleTableRecord(pRec);
		}, "返回当前 DimStyleTableRecord 包装；失败返回空包装")
		;
}
