#include "stdafx.h"
#include "PyErrorStatusException.h"
#include "PyDbLayerStateManager.h"
#include "PyDbDatabase.h"

// AcDbLayerStateManager : public AcRxObject (dblstate.h:28)
// addReactor/removeReactor 跳过：AcDbLayerStateManagerReactor 未封装（P2 依赖）。

static inline uint64_t idToU64(const AcDbObjectId& id) {
	return id.isNull() ? 0ULL : static_cast<uint64_t>(id.asOldId());
}
static inline AcDbObjectId u64ToId(uint64_t v) {
	AcDbObjectId id;
	if (v != 0ULL) id.setFromOldId(static_cast<Adesk::UIntPtr>(v));
	return id;
}

void bind_PyDbLayerStateManager(pym& m) {
	py::class_<PyDbLayerStateManager, PyRxObject>(m, "PyDbLayerStateManager")
		.def(py::init<>())
		.def(py::init<AcDbLayerStateManager*>(), py::arg("p") = nullptr)
		.def(py::init([](PyDbDatabase& db) {
			return std::make_unique<PyDbLayerStateManager>(new AcDbLayerStateManager(db.impObj()), true).release();
		}), py::arg("db"))

		// ---- layer state layer list management ----
		.def("addLayerStateLayers", [](PyDbLayerStateManager& self, const AcString& name,
				const AcDbObjectIdArray& layerIds) {
				if (!self.impObj()) return false;
			return PyWarnIfError(self.impObj()->addLayerStateLayers(name.kwszPtr(), layerIds),
				L"AcDbLayerStateManager::addLayerStateLayers");
		}, py::arg("name"), py::arg("layerIds"))
		.def("removeLayerStateLayers", [](PyDbLayerStateManager& self, const AcString& name,
				const AcStringArray& layerNames) {
				if (!self.impObj()) return false;
			return PyWarnIfError(self.impObj()->removeLayerStateLayers(name.kwszPtr(), layerNames),
				L"AcDbLayerStateManager::removeLayerStateLayers");
		}, py::arg("name"), py::arg("layerNames"))
		.def("getLayerStateLayers", [](PyDbLayerStateManager& self, const AcString& name,
				bool invert) -> AcStringArray {
				if (!self.impObj()) return AcStringArray();
			AcStringArray arr;
			if (!PyWarnIfError(self.impObj()->getLayerStateLayers(arr, name.kwszPtr(), invert),
				L"AcDbLayerStateManager::getLayerStateLayers")) return AcStringArray();
			return arr;
		}, py::arg("name"), py::arg("invert") = false)

		// ---- description ----
		.def("getLayerStateDescription", [](PyDbLayerStateManager& self, const AcString& name) -> AcString {
		if (!self.impObj()) return AcString();
			ACHAR* pDesc = nullptr;
			if (!PyWarnIfError(self.impObj()->getLayerStateDescription(name.kwszPtr(), pDesc),
				L"AcDbLayerStateManager::getLayerStateDescription")) return AcString();
			return pDesc ? AcString(pDesc) : AcString();
		}, py::arg("name"))
		.def("setLayerStateDescription", [](PyDbLayerStateManager& self, const AcString& name,
				const AcString& desc) {
				if (!self.impObj()) return false;
			return PyWarnIfError(self.impObj()->setLayerStateDescription(name.kwszPtr(), desc.kwszPtr()),
				L"AcDbLayerStateManager::setLayerStateDescription");
		}, py::arg("name"), py::arg("desc"))

		// ---- mask ----
		.def("getLayerStateMask", [](PyDbLayerStateManager& self, const AcString& name) -> int {
		if (!self.impObj()) return 0;
			AcDbLayerStateManager::LayerStateMask mask;
			if (!PyWarnIfError(self.impObj()->getLayerStateMask(name.kwszPtr(), mask),
				L"AcDbLayerStateManager::getLayerStateMask")) return 0;
			return static_cast<int>(mask);
		}, py::arg("name"))
		.def("setLayerStateMask", [](PyDbLayerStateManager& self, const AcString& name, int mask) {
		if (!self.impObj()) return false;
			return PyWarnIfError(self.impObj()->setLayerStateMask(name.kwszPtr(),
				static_cast<AcDbLayerStateManager::LayerStateMask>(mask)),
				L"AcDbLayerStateManager::setLayerStateMask");
		}, py::arg("name"), py::arg("mask"))

		// ---- import / export ----
		.def("exportLayerState", [](PyDbLayerStateManager& self, const AcString& name,
				const AcString& filename) {
				if (!self.impObj()) return false;
			return PyWarnIfError(self.impObj()->exportLayerState(name.kwszPtr(), filename.kwszPtr()),
				L"AcDbLayerStateManager::exportLayerState");
		}, py::arg("name"), py::arg("filename"))
		.def("importLayerState", [](PyDbLayerStateManager& self, const AcString& filename) {
		if (!self.impObj()) return false;
			return PyWarnIfError(self.impObj()->importLayerState(filename.kwszPtr()),
				L"AcDbLayerStateManager::importLayerState");
		}, py::arg("filename"))
		.def("importLayerStateWithName", [](PyDbLayerStateManager& self, const AcString& filename)
				-> std::tuple<bool, AcString> {
			AcString sName;
			if (self.isNull()) return std::make_tuple(false, AcString());
			bool ok = PyWarnIfError(self.impObj()->importLayerState(filename.kwszPtr(), sName),
				L"AcDbLayerStateManager::importLayerState");
			return std::make_tuple(ok, ok ? sName : AcString());
		}, py::arg("filename"))
		.def("importLayerStateFromDb", [](PyDbLayerStateManager& self, const AcString& stateName,
				PyDbDatabase& db) {
				if (!self.impObj()) return false;
			return PyWarnIfError(self.impObj()->importLayerStateFromDb(stateName.kwszPtr(), db.impObj()),
				L"AcDbLayerStateManager::importLayerStateFromDb");
		}, py::arg("stateName"), py::arg("db"))

		// ---- query ----
		.def("getLastRestoredLayerState", [](PyDbLayerStateManager& self)
				-> std::tuple<AcString, uint64_t> {
				if (!self.impObj()) return std::make_tuple(AcString(), 0ULL);
			AcString sName;
			AcDbObjectId id;
			if (!PyWarnIfError(self.impObj()->getLastRestoredLayerState(sName, id),
				L"AcDbLayerStateManager::getLastRestoredLayerState"))
				return std::make_tuple(AcString(), 0ULL);
			return std::make_tuple(sName, idToU64(id));
		})
		.def("compareLayerStateToDb", [](PyDbLayerStateManager& self, const AcString& name,
				uint64_t vpId) {
				if (!self.impObj()) return false;
			return self.impObj()->compareLayerStateToDb(name.kwszPtr(), u64ToId(vpId));
		}, py::arg("name"), py::arg("vpId"))
		.def("layerStateHasViewportData", [](PyDbLayerStateManager& self, const AcString& name) {
		if (!self.impObj()) return false;
			return self.impObj()->layerStateHasViewportData(name.kwszPtr());
		}, py::arg("name"))
		.def("isDependentLayerState", [](PyDbLayerStateManager& self, const AcString& name) {
		if (!self.impObj()) return false;
			return self.impObj()->isDependentLayerState(name.kwszPtr());
		}, py::arg("name"))
		;
}
