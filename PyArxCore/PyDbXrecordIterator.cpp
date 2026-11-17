#include "stdafx.h"
#include "PyErrorStatusException.h"
#include "PyDbXrecordIterator.h"
#include "PyDbXrecord.h"
#include <dbxrecrd.h>

PyDbXrecordIterator::~PyDbXrecordIterator() {
	if (m_p) delete m_p;
	m_p = nullptr;
}

PyDbXrecordIterator::PyDbXrecordIterator(AcDbXrecord* pOwner)
	: m_p(pOwner ? new AcDbXrecordIterator(pOwner) : nullptr) {}

bool PyDbXrecordIterator::done() {
	if (!m_p) return true;
	return m_p->done();
}

bool PyDbXrecordIterator::step() {
	if (!m_p) return false;
	return PyWarnIfError(m_p->next(), L"AcDbXrecordIterator::next");
}

void bind_PyDbXrecordIterator(pym&) {
	// 公开迭代器已退役；Xrecord 遍历折叠进 PyDbXrecord 本体字典协议（内部类保留）。
}
