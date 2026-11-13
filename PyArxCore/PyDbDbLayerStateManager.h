#pragma once
#include "stdafx.h"
#include "dblstate.h"

class PyDbDatabase;

// AcDbLayerStateManager pointer wrapper (dblstate.h; engine-owned via
// AcDbDatabase::getLayerStateManager(), NOT auto-deleted here).
// LayerStateMask bits are exposed to Python as plain int flags; see the
// kOn/kFrozen/.../kAll constants registered in bind_PyDbDbLayerStateManager.
class PyDbDbLayerStateManager {
	AcDbLayerStateManager* m_ptr = nullptr;
public:
	explicit PyDbDbLayerStateManager(AcDbLayerStateManager* p = nullptr) : m_ptr(p) {}
	AcDbLayerStateManager* impObj() const { return m_ptr; }
	bool isNull() const { return m_ptr == nullptr; }

	uint64_t layerStatesDictionaryId(bool createIfNotPresent = false) const;
	bool hasLayerState(const AcString& name) const;
	bool saveLayerState(const AcString& name, int mask);
	bool restoreLayerState(const AcString& name);
	bool deleteLayerState(const AcString& name);
	bool renameLayerState(const AcString& oldName, const AcString& newName);
	std::vector<AcString> getLayerStateNames(bool includeHidden = true, bool includeXref = true) const;
	PyDbDatabase getDatabase() const;

	AcString toString() const;
};

void bind_PyDbDbLayerStateManager(pym& m);
