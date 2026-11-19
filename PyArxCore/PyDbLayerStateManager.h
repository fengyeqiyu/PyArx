#pragma once
#include "stdafx.h"
#include "PyRxObject.h"
#include "dblstate.h"

// AcDbLayerStateManager : public AcRxObject (dblstate.h:28)
// 独立绑定（直接继承 PyRxObject）。
// addReactor/removeReactor 因 AcDbLayerStateManagerReactor 未封装（回调基类），
// 跳过（P2 依赖）。
class PyDbLayerStateManager : public PyRxObject {
public:
	PyDbLayerStateManager(AcDbLayerStateManager* p, bool autoDelete = true)
		: PyRxObject(p, autoDelete, false) {}
	PyDbLayerStateManager() : PyDbLayerStateManager(new AcDbLayerStateManager()) {}
	AcDbLayerStateManager* impObj() { return static_cast<AcDbLayerStateManager*>(m_ptr); }
	AcDbLayerStateManager* impObj() const { return static_cast<AcDbLayerStateManager*>(m_ptr); }
};

void bind_PyDbLayerStateManager(pym& m);
