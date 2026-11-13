#include "stdafx.h"
#include "PyErrorStatusException.h"
#include "PyDbLayerTableIterator.h"
#include "PyDbLayerTable.h"
#include "PyDbLayerTableRecord.h"

PyDbLayerTableIterator::PyDbLayerTableIterator(PyDbLayerTable& owner, bool atBeginning, bool skipDeleted)
	: PyDbSymbolTableIterator(nullptr) {
	AcDbLayerTableIterator* p = nullptr;
	if (!PyWarnIfError(owner.impObj()->newIterator(p, atBeginning, skipDeleted), L"AcDbLayerTable::newIterator"))
		p = nullptr;
	m_pObj = p;
}

void bind_PyDbLayerTableIterator(pym& m) {
	py::class_<PyDbLayerTableIterator, PyDbSymbolTableIterator>(m, "PyDbLayerTableIterator")
		.def(py::init<PyDbLayerTable&, bool, bool>(),
			py::arg("owner"), py::arg("atBeginning") = true, py::arg("skipDeleted") = true,
			"从持有者 LayerTable 创建迭代器（接管指针，析构 delete）")
		.def(py::init<AcDbLayerTableIterator*>(), py::arg("p"), "接管裸指针")
		.def("__next__", [](PyDbLayerTableIterator& s) -> PyDbLayerTableRecord {
			auto* pIter = static_cast<AcDbLayerTableIterator*>(s.impObj());
			if (pIter->done()) throw py::stop_iteration();
			AcDbLayerTableRecord* pRec = nullptr;
			if (!PyWarnIfError(pIter->getRecord(pRec, AcDb::kForRead, false), L"AcDbLayerTableIterator::getRecord"))
				pRec = nullptr;
			pIter->step();
			return PyDbLayerTableRecord(pRec);
		}, "返回当前 LayerTableRecord 包装；失败返回空包装")
		;
}
