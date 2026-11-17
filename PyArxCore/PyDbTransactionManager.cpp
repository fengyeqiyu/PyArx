#include "stdafx.h"
#include "PyErrorStatusException.h"
#include "PyDbTransactionManager.h"
#include "PyDbObject.h"
#include "PyDbTypeCaster.h"

int PyDbTransactionManager::numActiveTransactions() const {
	return m_ptr ? m_ptr->numActiveTransactions() : 0;
}

int PyDbTransactionManager::numOpenedObjects() const {
	return m_ptr ? m_ptr->numOpenedObjects() : 0;
}

bool PyDbTransactionManager::endTransaction() {
	return m_ptr ? PyWarnIfError(m_ptr->endTransaction(), L"TransactionManager.endTransaction") : false;
}

bool PyDbTransactionManager::abortTransaction() {
	return m_ptr ? PyWarnIfError(m_ptr->abortTransaction(), L"TransactionManager.abortTransaction") : false;
}

bool PyDbTransactionManager::queueForGraphicsFlush() {
	return m_ptr ? PyWarnIfError(m_ptr->queueForGraphicsFlush(), L"TransactionManager.queueForGraphicsFlush") : false;
}

bool PyDbTransactionManager::addNewlyCreatedDBRObject(PyDbObject& obj, bool add) {
	return m_ptr ? PyWarnIfError(m_ptr->addNewlyCreatedDBRObject(obj.impObj(), add),
		L"TransactionManager.addNewlyCreatedDBRObject") : false;
}

std::vector<PyDbObject> PyDbTransactionManager::getAllObjects() {
	std::vector<PyDbObject> result;
	if (!m_ptr) return result;
	AcArray<AcDbObject*> objs;
	if (PyWarnIfError(m_ptr->getAllObjects(objs), L"TransactionManager.getAllObjects")) {
		for (int i = 0; i < objs.length(); ++i)
			result.emplace_back(objs[i]);
	}
	return result;
}

PyDbObject PyDbTransactionManager::getObject(uint64_t objId, int mode, bool openErased) {
	if (!m_ptr) return PyDbObject(nullptr);
	if (objId == 0) {
		PyWarnIfError(Acad::eNullObjectPointer, L"TransactionManager.getObject(null objectId)");
		return PyDbObject(nullptr);
	}
	if (mode < 0 || mode > 2) {
		PyWarnIfError(Acad::eInvalidInput, L"TransactionManager.getObject(mode must be 0/1/2)");
		return PyDbObject(nullptr);
	}
	AcDbObjectId id; id.setFromOldId(objId);
	AcDbObject* pObj = nullptr;
	if (!PyWarnIfError(m_ptr->getObject(pObj, id, static_cast<AcDb::OpenMode>(mode), openErased),
		L"TransactionManager.getObject"))
		return PyDbObject(nullptr);
	return PyDbObject(pObj);
}

bool PyDbTransactionManager::markObjectPageable(PyDbObject& obj) {
	return m_ptr ? PyWarnIfError(m_ptr->markObjectPageable(obj.impObj()),
		L"TransactionManager.markObjectPageable") : false;
}

AcString PyDbTransactionManager::toString() const {
	return AcString(std::format("PyDbTransactionManager[active={},opened={}]",
		numActiveTransactions(), numOpenedObjects()).c_str(), AcString::Utf8);
}

void bind_PyDbTransactionManager(pym& m) {
	py::class_<PyDbTransactionManager>(m, "PyDbTransactionManager")
		.def(py::init<AcDbTransactionManager*>(), py::arg("object") = nullptr)
		.def("isNull", &PyDbTransactionManager::isNull)
		.def("numActiveTransactions", &PyDbTransactionManager::numActiveTransactions)
		.def("numOpenedObjects", &PyDbTransactionManager::numOpenedObjects)
		.def("endTransaction", &PyDbTransactionManager::endTransaction)
		.def("abortTransaction", &PyDbTransactionManager::abortTransaction)
		.def("queueForGraphicsFlush", &PyDbTransactionManager::queueForGraphicsFlush)
		// ---- 事务托管 DB 对象 ----
		.def("addNewlyCreatedDBRObject", &PyDbTransactionManager::addNewlyCreatedDBRObject,
			py::arg("obj"), py::arg("add") = true, "把新建 DB 对象纳入事务托管")
		.def("getAllObjects", &PyDbTransactionManager::getAllObjects,
			"返回当前事务托管的所有对象列表")
		.def("getObject", &PyDbTransactionManager::getObject,
			py::arg("objId"), py::arg("mode") = 1, py::arg("openErased") = false,
			"按 ObjectId 打开对象（事务托管）；mode 0读/1写/2通知")
		.def("markObjectPageable", &PyDbTransactionManager::markObjectPageable,
			py::arg("obj"), "标记对象可分页换出")
		.def("__repr__", &PyDbTransactionManager::toString);
}
