#include "stdafx.h"
#include "PyErrorStatusException.h"
#include "PyDbBlockTableRecordIterator.h"
#include "PyDbBlockTableRecord.h"
#include "PyDbEntity.h"
#include "PyDbOpenCache.h"

PyDbBlockTableRecordIterator::PyDbBlockTableRecordIterator(PyDbBlockTableRecord& owner, bool atBeginning, bool skipDeleted) {
    if (!owner.impObj()) { m_pIter = nullptr; return; }
    AcDbBlockTableRecordIterator* p = nullptr;
    if (!PyWarnIfError(owner.impObj()->newIterator(p, atBeginning, skipDeleted),
                       L"AcDbBlockTableRecord::newIterator")) {
        m_pIter = nullptr;
        return;
    }
    m_pIter = p;
}

PyDbBlockTableRecordIterator::PyDbBlockTableRecordIterator(AcDbBlockTableRecordIterator* p) : m_pIter(p) {}
PyDbBlockTableRecordIterator::~PyDbBlockTableRecordIterator() { delete m_pIter; }

bool PyDbBlockTableRecordIterator::done() const {
    if (!m_pIter) return true;
    return m_pIter->done();
}

bool PyDbBlockTableRecordIterator::step(bool forward, bool skipDeleted) {
    if (!m_pIter) return false;
    m_pIter->step(forward, skipDeleted);
    return true;
}

uint64_t PyDbBlockTableRecordIterator::currentId() const {
    if (!m_pIter) return 0ULL;
    AcDbObjectId id;
    if (m_pIter->getEntityId(id) != Acad::eOk || id.isNull()) return 0ULL;
    return static_cast<uint64_t>(id.asOldId());
}

PyDbEntity PyDbBlockTableRecordIterator::getEntity(int mode, bool openErasedEntity) const {
    if (!m_pIter) return PyDbEntity(nullptr);
    AcDbEntity* pEntity = nullptr;
    if (m_pIter->getEntity(pEntity, static_cast<AcDb::OpenMode>(mode), openErasedEntity) != Acad::eOk)
        pEntity = nullptr;
    return PyDbEntity(pEntity);
}

uint64_t PyDbBlockTableRecordIterator::__next__() {
    if (!m_pIter || m_pIter->done()) throw py::stop_iteration();
    uint64_t id = currentId();
    m_pIter->step(true, true);
    return id;
}

PyDbBlockTableRecordEntityIter::PyDbBlockTableRecordEntityIter(PyDbBlockTableRecord& owner, int mode, bool skipDeleted) : m_mode(mode) {
	if (!owner.impObj()) return;
	AcDbBlockTableRecordIterator* p = nullptr;
	if (!PyWarnIfError(owner.impObj()->newIterator(p, true, skipDeleted),
	                   L"AcDbBlockTableRecord::newIterator")) return;
	m_pIter = p;
}

PyDbBlockTableRecordEntityIter::~PyDbBlockTableRecordEntityIter() {
	if (m_pEnt) { PyDbOpenCache::Close(m_pEnt); m_pEnt->close(); }
	delete m_pIter;
}

PyDbEntity PyDbBlockTableRecordEntityIter::__next__() {
	if (!m_pIter) throw py::stop_iteration();
	// 上一个遍历对象先关闭再取下一个，避免 AcDb 同一对象被连续打开两次。
	if (m_pEnt) { PyDbOpenCache::Close(m_pEnt); m_pEnt->close(); m_pEnt = nullptr; }
	// 打不开的单个对象只跳过，不能终止整个 for 循环；
	// 请求写模式但对象已被别处写打开时，降级为只读产出，保证遍历不丢元素。
	while (!m_pIter->done()) {
		AcDbEntity* pEnt = nullptr;
		Acad::ErrorStatus es = m_pIter->getEntity(pEnt, static_cast<AcDb::OpenMode>(m_mode), false);
		if ((es != Acad::eOk || !pEnt) && m_mode != 0) {
			pEnt = nullptr;
			es = m_pIter->getEntity(pEnt, AcDb::kForRead, false);
			if (es == Acad::eOk && pEnt) {
				m_pIter->step(true, true);
				PyDbOpenCache::Track(pEnt, AcDb::kForRead);
				m_pEnt = pEnt;
				return PyDbEntity(pEnt);
			}
		}
		m_pIter->step(true, true);
		if (es != Acad::eOk || !pEnt) continue;
		PyDbOpenCache::Track(pEnt, static_cast<AcDb::OpenMode>(m_mode));
		m_pEnt = pEnt;
		return PyDbEntity(pEnt);
	}
	throw py::stop_iteration();
}

void bind_PyDbBlockTableRecordIterator(pym& m) {
    // 公开迭代器类已退役：遍历只挂块表记录本体（for ent in btr）。
    // PyDbBlockTableRecordIterator 与 PyDbBlockTableRecordEntityIter 保留为内部实现，
    // EntityIter 仅以私有绑定名暴露给 BTR.__iter__ 返回值类型使用。
    py::class_<PyDbBlockTableRecordEntityIter>(m, "_PyDbBlockTableRecordEntityIter")
        .def(py::init<PyDbBlockTableRecord&, int, bool>(),
             py::arg("owner"), py::arg("mode") = 1, py::arg("skipDeleted") = true)
        .def("__iter__", [](PyDbBlockTableRecordEntityIter& it) -> PyDbBlockTableRecordEntityIter& { return it; })
        .def("__next__", &PyDbBlockTableRecordEntityIter::__next__);
}
