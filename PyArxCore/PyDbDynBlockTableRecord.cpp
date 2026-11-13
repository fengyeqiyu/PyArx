#include "stdafx.h"
#include "PyDbDynBlockTableRecord.h"
#include "PyErrorStatusException.h"

PyDbDynBlockTableRecord::PyDbDynBlockTableRecord(uint64_t btrOldId) {
	AcDbObjectId id;
	id.setFromOldId(static_cast<Adesk::UIntPtr>(btrOldId));
	m_pObj = new AcDbDynBlockTableRecord(id);
}

PyDbDynBlockTableRecord::~PyDbDynBlockTableRecord() {
	delete m_pObj;
	m_pObj = nullptr;
}

void bind_PyDbDynBlockTableRecord(pym& m) {
	py::class_<PyDbDynBlockTableRecord>(m, "DynBlockTableRecord")
		.def(py::init<uint64_t>(), py::arg("btr_old_id"))
		// isDynamicBlock
		.def("isDynamicBlock", [](const PyDbDynBlockTableRecord& d) -> bool {
			return d.impObj()->isDynamicBlock();
		})
		// blockTableRecordId -> uint64_t
		.def("blockTableRecordId", [](const PyDbDynBlockTableRecord& d) -> uint64_t {
			auto id = d.impObj()->blockTableRecordId();
			return id.isNull() ? 0ULL : static_cast<uint64_t>(id.asOldId());
		})
		// getAnonymousBlockIds -> list[uint64]（失败返回空列表）
		.def("getAnonymousBlockIds", [](const PyDbDynBlockTableRecord& d) -> std::vector<uint64_t> {
			AcDbObjectIdArray arr;
			if (!PyWarnIfError(d.impObj()->getAnonymousBlockIds(arr), L"AcDbDynBlockTableRecord::getAnonymousBlockIds")) {
				return {};
			}
			std::vector<uint64_t> out;
			out.reserve(arr.length());
			for (int i = 0; i < arr.length(); ++i) {
				const AcDbObjectId& id = arr[i];
				out.push_back(id.isNull() ? 0ULL : static_cast<uint64_t>(id.asOldId()));
			}
			return out;
		})
		// updateAnonymousBlocks -> bool
		.def("updateAnonymousBlocks", [](const PyDbDynBlockTableRecord& d) -> bool {
			return PyWarnIfError(d.impObj()->updateAnonymousBlocks(), L"AcDbDynBlockTableRecord::updateAnonymousBlocks");
		})
		;
}
