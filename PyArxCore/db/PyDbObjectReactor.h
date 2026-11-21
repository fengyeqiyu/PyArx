#pragma once
#include "stdafx.h"
#include "PyRxObject.h"

// AcDbObjectReactor 的 Python 封装（trampoline 模式）
// Python 子类继承 PyDbObjectReactor，重写对应虚函数即可接收事件回调
class PyDbObjectReactor : public AcDbObjectReactor
{
public:
    using AcDbObjectReactor::AcDbObjectReactor;

    // trampoline：转发到 Python 重写
    void cancelled(const AcDbObject* pObj) override;
    void copied(const AcDbObject* pOld, const AcDbObject* pNew) override;
    void erased(const AcDbObject* pObj, bool bErasing) override;
    void goodbye(const AcDbObject* pObj) override;
    void openedForModify(const AcDbObject* pObj) override;
    void modified(const AcDbObject* pObj) override;
    void subObjModified(const AcDbObject* pObj, const AcDbObject* pSub) override;
    void modifyUndone(const AcDbObject* pObj) override;
    void modifiedXData(const AcDbObject* pObj) override;
    void unappended(const AcDbObject* pObj) override;
    void reappended(const AcDbObject* pObj) override;
    void objectClosed(const AcDbObjectId objId) override;
};

void bind_PyDbObjectReactor(pym& m);
