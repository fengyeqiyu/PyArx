#include "stdafx.h"
#include "PyErrorStatusException.h"
#include "PyDbDeepCloneFiler.h"
#include "PyDbTypeCaster.h"

// AcDbDeepCloneFiler : public AcDbDwgFiler (dbcfilrs.h:30)
// 独立绑定（无 PyDbDwgFiler 包装）。addReferences(AcDbIdRefQueue&) 因
// AcDbIdRefQueue 未封装且为 AutoCAD 内部深拷贝回调，跳过（P2 依赖）。

static inline uint64_t idToU64(const AcDbObjectId& id) {
	return id.isNull() ? 0ULL : static_cast<uint64_t>(id.asOldId());
}
static inline AcDbObjectId u64ToId(uint64_t v) {
	AcDbObjectId id;
	if (v != 0ULL) id.setFromOldId(static_cast<Adesk::UIntPtr>(v));
	return id;
}

void bind_PyDbDeepCloneFiler(pym& m) {
	py::class_<PyDbDeepCloneFiler, PyRxObject>(m, "PyDbDeepCloneFiler")
		.def(py::init<AcDbDeepCloneFiler*, bool>(), py::arg("p"), py::arg("autoDelete") = true)

		// ---- status / type ----
		.def("filerStatus", [](PyDbDeepCloneFiler& self) {
			if (self.isNull()) return static_cast<int>(Acad::eInvalidInput);
			return static_cast<int>(self.impObj()->filerStatus());
		})
		.def("setFilerStatus", [](PyDbDeepCloneFiler& self, int es) {
			if (self.isNull()) return false;
			self.impObj()->setFilerStatus(static_cast<Acad::ErrorStatus>(es));
			return true;
		}, py::arg("es"))
		.def("resetFilerStatus", [](PyDbDeepCloneFiler& self) {
			if (self.isNull()) return;
			self.impObj()->resetFilerStatus();
		})
		.def("filerType", [](PyDbDeepCloneFiler& self) {
			if (self.isNull()) return 0;
			return static_cast<int>(self.impObj()->filerType());
		})

		// ---- position ----
		.def("seek", [](PyDbDeepCloneFiler& self, int64_t offset, int method) {
			if (self.isNull()) return false;
			return PyWarnIfError(self.impObj()->seek(static_cast<Adesk::Int64>(offset), method),
				L"AcDbDeepCloneFiler::seek");
		}, py::arg("offset"), py::arg("method"))
		.def("tell", [](PyDbDeepCloneFiler& self) -> int64_t {
			if (self.isNull()) return 0;
			return static_cast<int64_t>(self.impObj()->tell());
		})

		// ---- owned-object traversal ----
		.def("moreOwnedObjects", [](PyDbDeepCloneFiler& self) {
			if (self.isNull()) return false;
			return self.impObj()->moreOwnedObjects();
		})
		.def("getNextOwnedObject", [](PyDbDeepCloneFiler& self) -> std::tuple<bool, uint64_t> {
			if (self.isNull()) return std::make_tuple(false, 0ULL);
			AcDbObjectId id;
			bool has = self.impObj()->getNextOwnedObject(id);
			return std::make_tuple(has, idToU64(id));
		})

		// ---- references ----
		// addReferences(AcDbIdRefQueue&) 跳过：AcDbIdRefQueue 未封装（P2 依赖）
		.def("usesReferences", [](PyDbDeepCloneFiler& self) {
			if (self.isNull()) return false;
			return self.impObj()->usesReferences();
		})

		// ---- read: integers ----
		.def("readInt8", [](PyDbDeepCloneFiler& self) -> int {
			Adesk::Int8 v = 0;
			if (!PyWarnIfError(self.impObj()->readInt8(&v), L"AcDbDeepCloneFiler::readInt8")) return 0;
			return static_cast<int>(v);
		})
		.def("readInt16", [](PyDbDeepCloneFiler& self) -> int {
			Adesk::Int16 v = 0;
			if (!PyWarnIfError(self.impObj()->readInt16(&v), L"AcDbDeepCloneFiler::readInt16")) return 0;
			return static_cast<int>(v);
		})
		.def("readInt32", [](PyDbDeepCloneFiler& self) -> int {
			Adesk::Int32 v = 0;
			if (!PyWarnIfError(self.impObj()->readInt32(&v), L"AcDbDeepCloneFiler::readInt32")) return 0;
			return static_cast<int>(v);
		})
		.def("readInt64", [](PyDbDeepCloneFiler& self) -> int64_t {
			Adesk::Int64 v = 0;
			if (!PyWarnIfError(self.impObj()->readInt64(&v), L"AcDbDeepCloneFiler::readInt64")) return 0;
			return static_cast<int64_t>(v);
		})
		.def("readUInt8", [](PyDbDeepCloneFiler& self) -> unsigned int {
			Adesk::UInt8 v = 0;
			if (!PyWarnIfError(self.impObj()->readUInt8(&v), L"AcDbDeepCloneFiler::readUInt8")) return 0;
			return static_cast<unsigned int>(v);
		})
		.def("readUInt16", [](PyDbDeepCloneFiler& self) -> unsigned int {
			Adesk::UInt16 v = 0;
			if (!PyWarnIfError(self.impObj()->readUInt16(&v), L"AcDbDeepCloneFiler::readUInt16")) return 0;
			return static_cast<unsigned int>(v);
		})
		.def("readUInt32", [](PyDbDeepCloneFiler& self) -> unsigned int {
			Adesk::UInt32 v = 0;
			if (!PyWarnIfError(self.impObj()->readUInt32(&v), L"AcDbDeepCloneFiler::readUInt32")) return 0;
			return static_cast<unsigned int>(v);
		})
		.def("readUInt64", [](PyDbDeepCloneFiler& self) -> uint64_t {
			Adesk::UInt64 v = 0;
			if (!PyWarnIfError(self.impObj()->readUInt64(&v), L"AcDbDeepCloneFiler::readUInt64")) return 0;
			return static_cast<uint64_t>(v);
		})

		// ---- read: float / bool ----
		.def("readDouble", [](PyDbDeepCloneFiler& self) -> double {
			double v = 0.0;
			if (!PyWarnIfError(self.impObj()->readDouble(&v), L"AcDbDeepCloneFiler::readDouble")) return 0.0;
			return v;
		})
		.def("readBool", [](PyDbDeepCloneFiler& self) -> bool {
			bool v = false;
			if (!PyWarnIfError(self.impObj()->readBool(&v), L"AcDbDeepCloneFiler::readBool")) return false;
			return v;
		})
		.def("readBoolean", [](PyDbDeepCloneFiler& self) -> bool {
			Adesk::Boolean v = Adesk::kFalse;
			if (!PyWarnIfError(self.impObj()->readBoolean(&v), L"AcDbDeepCloneFiler::readBoolean")) return false;
			return v != Adesk::kFalse;
		})

		// ---- read: geometry ----
		.def("readPoint2d", [](PyDbDeepCloneFiler& self) -> AcGePoint2d {
			AcGePoint2d v;
			if (!PyWarnIfError(self.impObj()->readPoint2d(&v), L"AcDbDeepCloneFiler::readPoint2d")) return AcGePoint2d();
			return v;
		})
		.def("readPoint3d", [](PyDbDeepCloneFiler& self) -> AcGePoint3d {
			AcGePoint3d v;
			if (!PyWarnIfError(self.impObj()->readPoint3d(&v), L"AcDbDeepCloneFiler::readPoint3d")) return AcGePoint3d();
			return v;
		})
		.def("readVector2d", [](PyDbDeepCloneFiler& self) -> AcGeVector2d {
			AcGeVector2d v;
			if (!PyWarnIfError(self.impObj()->readVector2d(&v), L"AcDbDeepCloneFiler::readVector2d")) return AcGeVector2d();
			return v;
		})
		.def("readVector3d", [](PyDbDeepCloneFiler& self) -> AcGeVector3d {
			AcGeVector3d v;
			if (!PyWarnIfError(self.impObj()->readVector3d(&v), L"AcDbDeepCloneFiler::readVector3d")) return AcGeVector3d();
			return v;
		})
		.def("readScale3d", [](PyDbDeepCloneFiler& self) -> AcGeScale3d {
			AcGeScale3d v;
			if (!PyWarnIfError(self.impObj()->readScale3d(&v), L"AcDbDeepCloneFiler::readScale3d")) return AcGeScale3d();
			return v;
		})

		// ---- read: ids ----
		.def("readHardOwnershipId", [](PyDbDeepCloneFiler& self) -> uint64_t {
			AcDbHardOwnershipId id;
			if (!PyWarnIfError(self.impObj()->readHardOwnershipId(&id), L"AcDbDeepCloneFiler::readHardOwnershipId")) return 0ULL;
			return idToU64(id);
		})
		.def("readSoftOwnershipId", [](PyDbDeepCloneFiler& self) -> uint64_t {
			AcDbSoftOwnershipId id;
			if (!PyWarnIfError(self.impObj()->readSoftOwnershipId(&id), L"AcDbDeepCloneFiler::readSoftOwnershipId")) return 0ULL;
			return idToU64(id);
		})
		.def("readHardPointerId", [](PyDbDeepCloneFiler& self) -> uint64_t {
			AcDbHardPointerId id;
			if (!PyWarnIfError(self.impObj()->readHardPointerId(&id), L"AcDbDeepCloneFiler::readHardPointerId")) return 0ULL;
			return idToU64(id);
		})
		.def("readSoftPointerId", [](PyDbDeepCloneFiler& self) -> uint64_t {
			AcDbSoftPointerId id;
			if (!PyWarnIfError(self.impObj()->readSoftPointerId(&id), L"AcDbDeepCloneFiler::readSoftPointerId")) return 0ULL;
			return idToU64(id);
		})

		// ---- read: string / handle / address ----
		.def("readString", [](PyDbDeepCloneFiler& self) -> AcString {
			AcString s;
			if (!PyWarnIfError(self.impObj()->readString(s), L"AcDbDeepCloneFiler::readString")) return AcString();
			return s;
		})
		.def("readAcDbHandle", [](PyDbDeepCloneFiler& self) -> AcDbHandle {
			AcDbHandle h;
			if (!PyWarnIfError(self.impObj()->readAcDbHandle(&h), L"AcDbDeepCloneFiler::readAcDbHandle")) return AcDbHandle();
			return h;
		})
		.def("readAddress", [](PyDbDeepCloneFiler& self) -> uintptr_t {
			void* p = nullptr;
			if (!PyWarnIfError(self.impObj()->readAddress(&p), L"AcDbDeepCloneFiler::readAddress")) return 0;
			return reinterpret_cast<uintptr_t>(p);
		})

		// ---- write: integers ----
		.def("writeInt8", [](PyDbDeepCloneFiler& self, int v) {
			return PyWarnIfError(self.impObj()->writeInt8(static_cast<Adesk::Int8>(v)), L"AcDbDeepCloneFiler::writeInt8");
		}, py::arg("v"))
		.def("writeInt16", [](PyDbDeepCloneFiler& self, int v) {
			return PyWarnIfError(self.impObj()->writeInt16(static_cast<Adesk::Int16>(v)), L"AcDbDeepCloneFiler::writeInt16");
		}, py::arg("v"))
		.def("writeInt32", [](PyDbDeepCloneFiler& self, int v) {
			return PyWarnIfError(self.impObj()->writeInt32(static_cast<Adesk::Int32>(v)), L"AcDbDeepCloneFiler::writeInt32");
		}, py::arg("v"))
		.def("writeInt64", [](PyDbDeepCloneFiler& self, int64_t v) {
			return PyWarnIfError(self.impObj()->writeInt64(static_cast<Adesk::Int64>(v)), L"AcDbDeepCloneFiler::writeInt64");
		}, py::arg("v"))
		.def("writeUInt8", [](PyDbDeepCloneFiler& self, unsigned int v) {
			return PyWarnIfError(self.impObj()->writeUInt8(static_cast<Adesk::UInt8>(v)), L"AcDbDeepCloneFiler::writeUInt8");
		}, py::arg("v"))
		.def("writeUInt16", [](PyDbDeepCloneFiler& self, unsigned int v) {
			return PyWarnIfError(self.impObj()->writeUInt16(static_cast<Adesk::UInt16>(v)), L"AcDbDeepCloneFiler::writeUInt16");
		}, py::arg("v"))
		.def("writeUInt32", [](PyDbDeepCloneFiler& self, unsigned int v) {
			return PyWarnIfError(self.impObj()->writeUInt32(static_cast<Adesk::UInt32>(v)), L"AcDbDeepCloneFiler::writeUInt32");
		}, py::arg("v"))
		.def("writeUInt64", [](PyDbDeepCloneFiler& self, uint64_t v) {
			return PyWarnIfError(self.impObj()->writeUInt64(static_cast<Adesk::UInt64>(v)), L"AcDbDeepCloneFiler::writeUInt64");
		}, py::arg("v"))

		// ---- write: float / bool ----
		.def("writeDouble", [](PyDbDeepCloneFiler& self, double v) {
			return PyWarnIfError(self.impObj()->writeDouble(v), L"AcDbDeepCloneFiler::writeDouble");
		}, py::arg("v"))
		.def("writeBool", [](PyDbDeepCloneFiler& self, bool v) {
			return PyWarnIfError(self.impObj()->writeBool(v), L"AcDbDeepCloneFiler::writeBool");
		}, py::arg("v"))
		.def("writeBoolean", [](PyDbDeepCloneFiler& self, bool v) {
			return PyWarnIfError(self.impObj()->writeBoolean(v ? Adesk::kTrue : Adesk::kFalse), L"AcDbDeepCloneFiler::writeBoolean");
		}, py::arg("v"))

		// ---- write: geometry ----
		.def("writePoint2d", [](PyDbDeepCloneFiler& self, const AcGePoint2d& v) {
			return PyWarnIfError(self.impObj()->writePoint2d(v), L"AcDbDeepCloneFiler::writePoint2d");
		}, py::arg("v"))
		.def("writePoint3d", [](PyDbDeepCloneFiler& self, const AcGePoint3d& v) {
			return PyWarnIfError(self.impObj()->writePoint3d(v), L"AcDbDeepCloneFiler::writePoint3d");
		}, py::arg("v"))
		.def("writeVector2d", [](PyDbDeepCloneFiler& self, const AcGeVector2d& v) {
			return PyWarnIfError(self.impObj()->writeVector2d(v), L"AcDbDeepCloneFiler::writeVector2d");
		}, py::arg("v"))
		.def("writeVector3d", [](PyDbDeepCloneFiler& self, const AcGeVector3d& v) {
			return PyWarnIfError(self.impObj()->writeVector3d(v), L"AcDbDeepCloneFiler::writeVector3d");
		}, py::arg("v"))
		.def("writeScale3d", [](PyDbDeepCloneFiler& self, const AcGeScale3d& v) {
			return PyWarnIfError(self.impObj()->writeScale3d(v), L"AcDbDeepCloneFiler::writeScale3d");
		}, py::arg("v"))

		// ---- write: ids ----
		.def("writeHardOwnershipId", [](PyDbDeepCloneFiler& self, uint64_t v) {
			AcDbHardOwnershipId id(u64ToId(v));
			return PyWarnIfError(self.impObj()->writeHardOwnershipId(id), L"AcDbDeepCloneFiler::writeHardOwnershipId");
		}, py::arg("v"))
		.def("writeSoftOwnershipId", [](PyDbDeepCloneFiler& self, uint64_t v) {
			AcDbSoftOwnershipId id(u64ToId(v));
			return PyWarnIfError(self.impObj()->writeSoftOwnershipId(id), L"AcDbDeepCloneFiler::writeSoftOwnershipId");
		}, py::arg("v"))
		.def("writeHardPointerId", [](PyDbDeepCloneFiler& self, uint64_t v) {
			AcDbHardPointerId id(u64ToId(v));
			return PyWarnIfError(self.impObj()->writeHardPointerId(id), L"AcDbDeepCloneFiler::writeHardPointerId");
		}, py::arg("v"))
		.def("writeSoftPointerId", [](PyDbDeepCloneFiler& self, uint64_t v) {
			AcDbSoftPointerId id(u64ToId(v));
			return PyWarnIfError(self.impObj()->writeSoftPointerId(id), L"AcDbDeepCloneFiler::writeSoftPointerId");
		}, py::arg("v"))

		// ---- write: string / handle / address / bchunk ----
		.def("writeString", [](PyDbDeepCloneFiler& self, const AcString& v) {
			return PyWarnIfError(self.impObj()->writeString(v.kwszPtr()), L"AcDbDeepCloneFiler::writeString");
		}, py::arg("v"))
		.def("writeAcDbHandle", [](PyDbDeepCloneFiler& self, const AcDbHandle& h) {
			return PyWarnIfError(self.impObj()->writeAcDbHandle(h), L"AcDbDeepCloneFiler::writeAcDbHandle");
		}, py::arg("h"))
		.def("writeAddress", [](PyDbDeepCloneFiler& self, uintptr_t p) {
			return PyWarnIfError(self.impObj()->writeAddress(reinterpret_cast<const void*>(p)), L"AcDbDeepCloneFiler::writeAddress");
		}, py::arg("p"))
		.def("writeBChunk", [](PyDbDeepCloneFiler& self, const std::vector<uint8_t>& data) {
			ads_binary bin;
			bin.buf = const_cast<char*>(reinterpret_cast<const char*>(data.data()));
			bin.clen = data.size();
			return PyWarnIfError(self.impObj()->writeBChunk(bin), L"AcDbDeepCloneFiler::writeBChunk");
		}, py::arg("data"))
		;
}
