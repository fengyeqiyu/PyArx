#include "stdafx.h"
#include "PyErrorStatusException.h"
#include "PyDbWblockCloneFiler.h"

// AcDbWblockCloneFiler : public AcDbDeepCloneFiler (dbcfilrs.h:142)
// addReferences(AcDbIdRefQueue&) 跳过：AcDbIdRefQueue 未封装（P2 依赖，同基类）。

static inline uint64_t idToU64(const AcDbObjectId& id) {
	return id.isNull() ? 0ULL : static_cast<uint64_t>(id.asOldId());
}

void bind_PyDbWblockCloneFiler(pym& m) {
	py::class_<PyDbWblockCloneFiler, PyDbDeepCloneFiler>(m, "PyDbWblockCloneFiler")
		.def(py::init<>())
		.def(py::init<AcDbWblockCloneFiler*, bool>(), py::arg("p"), py::arg("autoDelete") = true)

		// filerType override
		.def("filerType", [](PyDbWblockCloneFiler& self) {
			if (self.isNull()) return 0;
			return static_cast<int>(self.impObj()->filerType());
		})

		// hard-object traversal
		.def("moreHardObjects", [](PyDbWblockCloneFiler& self) {
			if (self.isNull()) return false;
			return self.impObj()->moreHardObjects();
		})
		.def("getNextHardObject", [](PyDbWblockCloneFiler& self) -> std::tuple<bool, uint64_t> {
			if (self.isNull()) return std::make_tuple(false, 0ULL);
			AcDbObjectId id;
			bool has = self.impObj()->getNextHardObject(id);
			return std::make_tuple(has, idToU64(id));
		})

		// usesReferences override
		.def("usesReferences", [](PyDbWblockCloneFiler& self) {
			if (self.isNull()) return false;
			return self.impObj()->usesReferences();
		})
		;
}
