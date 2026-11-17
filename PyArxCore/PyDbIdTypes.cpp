#include "PyDbIdTypes.h"
#include "dbid.h"
#include "dbhandle.h"
#include "PyDbDatabase.h"
#include "PyRxClass.h"

// 把 ObjectId / int（含 __index__ 对象）统一取成 oldId，供比较与运算复用
static bool toOldId(const py::object& o, Adesk::IntDbId& out)
{
	if (py::isinstance<AcDbObjectId>(o))
	{
		out = o.cast<AcDbObjectId>().asOldId();
		return true;
	}
	if (PyLong_Check(o.ptr()) || PyIndex_Check(o.ptr()))
	{
		if (PyErr_Occurred())
			PyErr_Clear();
		PyObject* idx = PyNumber_Index(o.ptr());
		if (!idx)
		{
			PyErr_Clear();
			return false;
		}
		py::object hold = py::reinterpret_steal<py::object>(idx);
		out = static_cast<Adesk::IntDbId>(py::cast<long long>(hold));
		return true;
	}
	return false;
}

void bind_PyDbIdTypes(pym& m)
{
	// ---------------- Handle：直接绑定 AcDbHandle ----------------
	if (!py::hasattr(m, "PyDbHandle"))
	{
		py::class_<AcDbHandle>(m, "PyDbHandle", "AcDbHandle 原生绑定（对齐 PyRx Db.Handle）")
			.def(py::init([]() { return AcDbHandle(0, 0); }))
			.def(py::init([](int lo, int hi) { return AcDbHandle(lo, hi); }),
				py::arg("low"), py::arg("high"))
			.def(py::init([](const std::string& s) {
				AcString t(s.c_str());
				return AcDbHandle(t.constPtr());
				}), py::arg("strVal"))
			.def(py::init([](Adesk::UInt64 v) { return AcDbHandle(v); }), py::arg("int64Val"))
			.def("toString", [](const AcDbHandle& h) {
				ACHAR buf[20] = { 0 };
				h.getIntoAsciiBuffer(buf, 19);
				AcString t(buf);
				const char* p = t.utf8Ptr();
				return std::string(p ? p : "");
				})
			.def("low", &AcDbHandle::low)
			.def("high", &AcDbHandle::high)
			.def("setLow", &AcDbHandle::setLow, py::arg("val"))
			.def("setHigh", &AcDbHandle::setHigh, py::arg("val"))
			.def("value", [](const AcDbHandle& h) { return static_cast<Adesk::UInt64>(h); })
			.def("setValue", [](AcDbHandle& h, Adesk::UInt64 v) { h = AcDbHandle(v); }, py::arg("val"))
			.def("isNull", [](const AcDbHandle& h) { return h == AcDbHandle::kNull; })
			.def("isOne", [](const AcDbHandle& h) { return static_cast<Adesk::UInt64>(h) == 1; })
			.def("setNull", [](AcDbHandle& h) { h = AcDbHandle(0, 0); })
			.def("increment", [](AcDbHandle& h) { h = h + AcDbHandle(1, 0); })
			.def("decrement", [](AcDbHandle& h) { h = h - AcDbHandle(1, 0); })
			.def("__hash__", [](const AcDbHandle& h) {
				return static_cast<size_t>(static_cast<Adesk::UInt64>(h));
				})
			.def("__eq__", [](const AcDbHandle& a, const AcDbHandle& b) { return a == b; },
				py::is_operator())
			.def("__repr__", [](const AcDbHandle& h) {
				ACHAR buf[20] = { 0 };
				h.getIntoAsciiBuffer(buf, 19);
				AcString t(buf);
				const char* p = t.utf8Ptr();
				return "Handle('" + std::string(p ? p : "") + "')";
				});
	}

	// ---------------- ObjectId：直接绑定 AcDbObjectId ----------------
	if (!py::hasattr(m, "PyDbObjectId"))
	{
		py::class_<AcDbObjectId>(m, "PyDbObjectId", "AcDbObjectId 原生绑定（对齐 PyRx Db.ObjectId）")
			.def(py::init<>())
			.def(py::init([](Adesk::IntDbId v) {
				AcDbObjectId id(AcDbObjectId::kNull);
				id.setFromOldId(v);
				return id;
				}), py::arg("oldId"), "由旧 id（asOldId 编码）构造，与 PyArx 既有 uint64 口径互通")
			.def("asOldId", &AcDbObjectId::asOldId)
			.def("setFromOldId", [](AcDbObjectId& id, Adesk::IntDbId v) { id.setFromOldId(v); },
				py::arg("oldId"))
			.def_static("fromOldId", [](Adesk::IntDbId v) {
				AcDbObjectId id(AcDbObjectId::kNull);
				id.setFromOldId(v);
				return id;
				}, py::arg("oldId"))
			.def("isNull", &AcDbObjectId::isNull)
			.def("setNull", [](AcDbObjectId& id) { id.setNull(); })
			.def("isErased", &AcDbObjectId::isErased)
			.def("isEffectivelyErased", &AcDbObjectId::isEffectivelyErased)
			.def("isResident", &AcDbObjectId::isResident)
			.def("isValid", &AcDbObjectId::isValid)
			.def("isWellBehaved", &AcDbObjectId::isWellBehaved)
			.def("objectLeftOnDisk", &AcDbObjectId::objectLeftOnDisk)
			.def("convertToRedirectedId", [](AcDbObjectId& id) { return id.convertToRedirectedId(); })
			.def("database", [](const AcDbObjectId& id) {
				AcDbDatabase* p = id.isNull() ? nullptr : id.database();
				return PyDbDatabase(p);
				})
			.def("originalDatabase", [](const AcDbObjectId& id) {
				AcDbDatabase* p = id.isNull() ? nullptr : id.originalDatabase();
				return PyDbDatabase(p);
				})
			.def("objectClass", [](const AcDbObjectId& id) { return PyRxClass(id.objectClass()); })
			.def("isDerivedFrom", [](const AcDbObjectId& id, const PyRxClass& desc) {
				AcRxClass* k = id.objectClass();
				return k ? k->isDerivedFrom(desc.impObj()) : false;
				}, py::arg("desc"))
			.def("handle", [](const AcDbObjectId& id) { return id.handle(); })
			.def("nonForwardedHandle", [](const AcDbObjectId& id) { return id.nonForwardedHandle(); })
			.def("__int__", [](const AcDbObjectId& id) { return py::int_(id.asOldId()); })
			// __index__ 让 ObjectId 可直接传给所有 uint64_t / int 入参
			// （pybind11 整型 caster 在 CPython>=3.8 走 PyLong_AsLongLong -> __index__）
			.def("__index__", [](const AcDbObjectId& id) { return py::int_(id.asOldId()); })
			.def("__hash__", [](const AcDbObjectId& id) {
				return static_cast<size_t>(id.asOldId());
				})
			.def("__eq__", [](const AcDbObjectId& a, py::object b) {
				Adesk::IntDbId v = 0;
				return toOldId(b, v) && a.asOldId() == v;
				}, py::is_operator(), "支持与 ObjectId 或 int 比较")
			.def("__ne__", [](const AcDbObjectId& a, py::object b) {
				Adesk::IntDbId v = 0;
				if (!toOldId(b, v))
					return true;
				return a.asOldId() != v;
				}, py::is_operator(), "支持与 ObjectId 或 int 比较")
			.def("__lt__", [](const AcDbObjectId& a, const AcDbObjectId& b) {
				return a.asOldId() < b.asOldId();
				}, py::is_operator())
			.def("__le__", [](const AcDbObjectId& a, const AcDbObjectId& b) {
				return a.asOldId() <= b.asOldId();
				}, py::is_operator())
			.def("__gt__", [](const AcDbObjectId& a, const AcDbObjectId& b) {
				return a.asOldId() > b.asOldId();
				}, py::is_operator())
			.def("__ge__", [](const AcDbObjectId& a, const AcDbObjectId& b) {
				return a.asOldId() >= b.asOldId();
				}, py::is_operator())
			.def("__repr__", [](const AcDbObjectId& id) {
				return "ObjectId(" + std::to_string(static_cast<long long>(id.asOldId())) + ")";
				});
	}
}
