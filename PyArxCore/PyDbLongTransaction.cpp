#include "stdafx.h"
#include "PyErrorStatusException.h"
#include "PyDbLongTransaction.h"

PyWrapper_Cpp(DbLongTransaction);

void bind_PyDbLongTransaction(pym& m) {
	py::class_<PyDbLongTransaction, PyDbObject>(m, "PyDbLongTransaction")
		.def(py::init<AcDbLongTransaction*>(), py::arg("p"))
		PyWrapper_BIND(DbLongTransaction)
		// ---- type() -> int (kSameDb=0/kXrefDb=1/kUnrelatedDb=2) ----
		.def("type", [](PyDbLongTransaction& s) {
			if (!s.impObj()) return -1;
			return s.impObj()->type();
		})
		// ---- originBlock() / destinationBlock() -> uint64 ObjectId ----
		.def("originBlock", [](PyDbLongTransaction& s) -> uint64_t {
			if (!s.impObj()) return 0ULL;
			auto id = s.impObj()->originBlock();
			return id.isNull() ? 0ULL : static_cast<uint64_t>(id.asOldId());
		})
		.def("destinationBlock", [](PyDbLongTransaction& s) -> uint64_t {
			if (!s.impObj()) return 0ULL;
			auto id = s.impObj()->destinationBlock();
			return id.isNull() ? 0ULL : static_cast<uint64_t>(id.asOldId());
		})
		// ---- getLongTransactionName(AcString&) ----
		.def("getLongTransactionName", [](PyDbLongTransaction& s) -> AcString {
			if (!s.impObj()) return AcString();
			AcString out;
			if (!PyWarnIfError(s.impObj()->getLongTransactionName(out), L"AcDbLongTransaction::getLongTransactionName"))
				return AcString();
			return out;
		})
		// ---- addToWorkSet / removeFromWorkSet / syncWorkSet -> bool ----
		.def("addToWorkSet", [](PyDbLongTransaction& s, uint64_t idv) {
			if (!s.impObj()) return false;
			AcDbObjectId id; id.setFromOldId(static_cast<Adesk::UIntPtr>(idv));
			return PyWarnIfError(s.impObj()->addToWorkSet(id), L"AcDbLongTransaction::addToWorkSet");
		}, py::arg("id"))
		.def("removeFromWorkSet", [](PyDbLongTransaction& s, uint64_t idv) {
			if (!s.impObj()) return false;
			AcDbObjectId id; id.setFromOldId(static_cast<Adesk::UIntPtr>(idv));
			return PyWarnIfError(s.impObj()->removeFromWorkSet(id), L"AcDbLongTransaction::removeFromWorkSet");
		}, py::arg("id"))
		.def("syncWorkSet", [](PyDbLongTransaction& s) {
			if (!s.impObj()) return false;
			return PyWarnIfError(s.impObj()->syncWorkSet(), L"AcDbLongTransaction::syncWorkSet");
		})
		// ---- originObject(AcDbObjectId) -> uint64 ----
		.def("originObject", [](PyDbLongTransaction& s, uint64_t idv) -> uint64_t {
			if (!s.impObj()) return 0ULL;
			AcDbObjectId id; id.setFromOldId(static_cast<Adesk::UIntPtr>(idv));
			auto r = s.impObj()->originObject(id);
			return r.isNull() ? 0ULL : static_cast<uint64_t>(r.asOldId());
		}, py::arg("id"))
		// ---- workSetHas(AcDbObjectId, bool) -> bool ----
		.def("workSetHas", [](PyDbLongTransaction& s, uint64_t idv, bool incErased) {
			if (!s.impObj()) return false;
			AcDbObjectId id; id.setFromOldId(static_cast<Adesk::UIntPtr>(idv));
			return s.impObj()->workSetHas(id, incErased);
		}, py::arg("id"), py::arg("incErased") = false)
		// ---- regenWorkSetWithDrawOrder -> void ----
		.def("regenWorkSetWithDrawOrder", [](PyDbLongTransaction& s) {
			if (!s.impObj()) return;
			s.impObj()->regenWorkSetWithDrawOrder();
		})
		// ---- disallowDrawOrder -> bool ----
		.def("disallowDrawOrder", [](PyDbLongTransaction& s) {
			if (!s.impObj()) return false;
			return s.impObj()->disallowDrawOrder();
		})
		;
	// 跳过：
	//  - newWorkSetIterator(AcDbLongTransWorkSetIterator*&, bool, bool)：依赖 PyDbLongTransWorkSetIterator（另批封装），P2 依赖。
	//  - activeIdMap() -> AcDbIdMapping*：工程无 AcDbIdMapping 的 Python 包装，P2 依赖。
	//  - getLongTransactionName(ACHAR*&)：deprecated 重载（调用方负责 free），不绑。
	//  - pack：CHM 无页面，疑误提取，跳过。
}
