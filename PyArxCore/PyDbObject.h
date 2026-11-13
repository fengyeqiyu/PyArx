#pragma once
#include "stdafx.h"
#include <dbObject.h>
#include "PyRxObject.h"
#include "PyRxClass.h"

#include "PyGiDrawable.h"
class PyDbDatabase;
class PyDbDwgFiler;
class PyDbDxfFiler;

class AcDbDwgFiler;


class PyDbObject :public PyGiDrawable {
	PyWrapper_H(DbObject)
public:
	PyDbObject(AcDbObject* ptr) :PyGiDrawable(ptr, true, true) {}
	virtual ~PyDbObject(void)override = default;
	virtual enum Acad::ErrorStatus applyPartialUndo(const PyDbDwgFiler&,const PyRxClass&);
	void assertNotifyEnabled(void)const;
	void assertReadEnabled(void)const;
	void assertWriteEnabled(bool autoUndo, bool recordModified);
	virtual enum Acad::ErrorStatus audit(AcDbAuditInfo*);
	enum Acad::ErrorStatus cancel(void);
	virtual void cancelled(const PyDbObject& obj);
	virtual  PyRxObject clone(void)const;
	enum Acad::ErrorStatus close(void);
	enum Acad::ErrorStatus closeAndPage(bool);
	virtual void copied(const PyDbObject &, const PyDbObject &);
	Acad::ErrorStatus copyFrom(const PyDbObject& dbobj);
	enum Acad::ErrorStatus createExtensionDictionary(void);
	PyDbDatabase database(void)const;
	virtual enum Acad::ErrorStatus decomposeForSave(enum AcDb::AcDbDwgVersion, PyDbObject*&, AcDbObjectId&, bool&);
	virtual enum Acad::ErrorStatus deepClone(PyDbObject*, PyDbObject*&, AcDbIdMapping&, bool)const;
	bool disableUndoRecording(bool);
	enum Acad::ErrorStatus downgradeOpen(void);
	enum Acad::ErrorStatus downgradeToNotify(bool);
	virtual  AcGiDrawable* drawable(void);
	enum Acad::ErrorStatus dwgIn(PyDbDwgFiler&);
	virtual enum Acad::ErrorStatus dwgInFields(PyDbDwgFiler&);
	enum Acad::ErrorStatus dwgOut(PyDbDwgFiler&)const;
	virtual enum Acad::ErrorStatus dwgOutFields(PyDbDwgFiler&)const;
	enum Acad::ErrorStatus dxfIn(AcDbDxfFiler*);
	virtual enum Acad::ErrorStatus dxfInFields(AcDbDxfFiler*);
	enum Acad::ErrorStatus dxfOut(AcDbDxfFiler*, bool, AcDbObjectId, const std::vector<AcDbObjectId>& ids)const;
	virtual enum Acad::ErrorStatus dxfOutFields(AcDbDxfFiler*)const;
	enum Acad::ErrorStatus erase(bool);
	virtual void erased(PyDbObject const*, bool);
	AcDbObjectId extensionDictionary(void)const;
	void getAcDbHandle(AcDbHandle&)const;
	Adesk::UInt64 handle(void)const;
	enum Acad::ErrorStatus getBinaryData(wchar_t const*, int&, char*&)const;
	virtual enum Acad::ErrorStatus getClassID(struct _GUID*)const;
	enum Acad::ErrorStatus getField(wchar_t const*, AcDbField*&, enum AcDb::OpenMode)const;
	enum Acad::ErrorStatus getField(wchar_t const*, AcDbObjectId&)const;
	AcDbObjectId getFieldDictionary(void)const;
	enum Acad::ErrorStatus getFieldDictionary(AcDbDictionary*&, enum AcDb::OpenMode)const;
	enum Acad::ErrorStatus getObjectBirthVersion(enum AcDb::AcDbDwgVersion&, enum AcDb::MaintenanceReleaseVersion&);
	virtual enum Acad::ErrorStatus getObjectSaveVersion(AcDbDwgFiler const*, enum AcDb::AcDbDwgVersion&, enum AcDb::MaintenanceReleaseVersion&);
	virtual enum Acad::ErrorStatus getObjectSaveVersion(AcDbDxfFiler const*, enum AcDb::AcDbDwgVersion&, enum AcDb::MaintenanceReleaseVersion&);
	virtual void goodbye(PyDbObject const*);
	enum Acad::ErrorStatus handOverTo(PyDbObject*, bool, bool);
	bool hasFields(void)const;
	bool hasPersistentReactor(AcDbObjectId)const;
	bool hasSaveVersionOverride(void);
	virtual  AcDbObjectId id(void)const;

	bool isAProxy(void)const;
	bool isAcDbObjectIdsInFlux(void)const;
	bool isCancelling(void)const;
	bool isEraseStatusToggled(void)const;
	bool isErased(void)const;
	bool isModified(void)const;
	bool isModifiedGraphics(void)const;
	bool isModifiedXData(void)const;
	bool isNewObject(void)const;
	bool isNotifyEnabled(void)const;
	bool isNotifying(void)const;
	virtual bool isPersistent(void)const;
	bool isReadEnabled(void)const;;
	bool isReallyClosing(void)const;
	bool isTransactionResident(void)const;
	bool isUndoRecordingDisabled(void)const;
	bool isUndoing(void)const;
	bool isWriteEnabled(void)const;
	virtual enum AcDb::DuplicateRecordCloning mergeStyle(void)const;
	virtual void modified(PyDbObject const*);
	virtual void modifiedGraphics(AcDbEntity const*);
	virtual void modifiedXData(PyDbObject const*);
	virtual void modifyUndone(PyDbObject const*);
	virtual void objectClosed(AcDbObjectId);
	AcDbObjectId objectId(void)const;
	virtual void openedForModify(PyDbObject const*);
	AcDbObjectId ownerId(void)const;
	AcArray<void*, AcArrayMemCopyReallocator<void*> > const* reactors(void)const;
	virtual void reappended(PyDbObject const*);
	virtual void recvPropagateModify(PyDbObject const*);
	enum Acad::ErrorStatus releaseExtensionDictionary(void);
	virtual enum Acad::ErrorStatus removeField(wchar_t const*);
	virtual enum Acad::ErrorStatus removeField(wchar_t const*, AcDbObjectId&);
	virtual enum Acad::ErrorStatus removeField(AcDbObjectId);
	void setAcDbObjectIdsInFlux(void);
	enum Acad::ErrorStatus setBinaryData(wchar_t const*, int, char const*);
	virtual enum Acad::ErrorStatus setField(wchar_t const*, AcDbField*, AcDbObjectId&);
	void setHasSaveVersionOverride(bool);
	virtual enum Acad::ErrorStatus setOwnerId(AcDbObjectId);
	virtual enum Acad::ErrorStatus setXData(struct resbuf const*);
	virtual enum Acad::ErrorStatus subCancel(void);
	virtual enum Acad::ErrorStatus subClose(void);
	virtual enum Acad::ErrorStatus subDeepClone(PyDbObject*, PyDbObject*&, AcDbIdMapping&, bool)const;
	virtual enum Acad::ErrorStatus subErase(bool);
	virtual enum Acad::ErrorStatus subGetClassID(struct _GUID*)const;
	virtual enum Acad::ErrorStatus subHandOverTo(PyDbObject*);
	virtual void subObjModified(PyDbObject const*, PyDbObject const*);
	virtual enum Acad::ErrorStatus subOpen(enum AcDb::OpenMode);
	virtual unsigned int subSetAttributes(AcGiDrawableTraits*);
	virtual enum Acad::ErrorStatus subSwapIdWith(AcDbObjectId, bool, bool);
	virtual void subViewportDraw(AcGiViewportDraw*);
	virtual enum Acad::ErrorStatus subWblockClone(AcRxObject*, PyDbObject*&, AcDbIdMapping&, bool)const;
	virtual bool subWorldDraw(AcGiWorldDraw*);
	enum Acad::ErrorStatus swapIdWith(AcDbObjectId, bool, bool);
	virtual enum Acad::ErrorStatus swapReferences(AcDbIdMapping const&);
	virtual void unappended(PyDbObject const*);
	PyDbDwgFiler& undoFiler(void);
	enum Acad::ErrorStatus upgradeFromNotify(bool&);
	enum Acad::ErrorStatus upgradeOpen(void);
	virtual enum Acad::ErrorStatus wblockClone(AcRxObject*, PyDbObject*&, AcDbIdMapping&, bool)const;
	virtual struct resbuf* xData(wchar_t const*)const;
	enum Acad::ErrorStatus xDataTransformBy(AcGeMatrix3d const&);
	virtual void xmitPropagateModify(void)const;

};


void bind_PyDbObject(pym& m);