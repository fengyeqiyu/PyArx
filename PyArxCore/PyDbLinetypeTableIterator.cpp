#include "stdafx.h"
#include "PyErrorStatusException.h"
#include "PyDbLinetypeTableIterator.h"
#include "PyDbLinetypeTable.h"
#include "PyDbLinetypeTableRecord.h"

PyDbLinetypeTableIterator::PyDbLinetypeTableIterator(PyDbLinetypeTable& owner, bool atBeginning, bool skipDeleted)
	: PyDbSymbolTableIterator(nullptr) {
	AcDbLinetypeTableIterator* p = nullptr;
	if (!PyWarnIfError(owner.impObj()->newIterator(p, atBeginning, skipDeleted), L"AcDbLinetypeTable::newIterator"))
		p = nullptr;
	m_pObj = p;
}

void bind_PyDbLinetypeTableIterator(pym& m) {
	py::class_<PyDbLinetypeTableIterator, PyDbSymbolTableIterator>(m, "PyDbLinetypeTableIterator")
		.def(py::init<PyDbLinetypeTable&, bool, bool>(),
			py::arg("owner"), py::arg("atBeginning") = true, py::arg("skipDeleted") = true,
			"从持有者 LinetypeTable 创建迭代器（接管指针，析构 delete）")
		.def(py::init<AcDbLinetypeTableIterator*>(), py::arg("p"), "接管裸指针")
		.def("__next__", [](PyDbLinetypeTableIterator& s) -> PyDbLinetypeTableRecord {
			auto* pIter = static_cast<AcDbLinetypeTableIterator*>(s.impObj());
			if (pIter->done()) throw py::stop_iteration();
			AcDbLinetypeTableRecord* pRec = nullptr;
			if (!PyWarnIfError(pIter->getRecord(pRec, AcDb::kForRead, false), L"AcDbLinetypeTableIterator::getRecord"))
				pRec = nullptr;
			pIter->step();
			return PyDbLinetypeTableRecord(pRec);
		}, "返回当前 LinetypeTableRecord 包装；失败返回空包装")
		;
}
