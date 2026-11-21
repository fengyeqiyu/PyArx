#pragma once
// ===========================================================================
// PyGiTransient.h —— AcGi 瞬态图形（SDK: acgitransient.h）+ Python 自绘桥。
//
// AcGi 的 worldDraw / viewportDraw 上下文只会在“图形系统回调某个 drawable”
// 时出现，Python 侧本来无从获得。补齐两件事让 AcGi 真正可用：
//   1) GiPythonDrawable —— 由 Python 继承、实现 worldDraw/viewportDraw 的
//      AcGiDrawable 子类（内部走 AcGiDrawable 的保护虚函数 subXxx）；
//   2) transient 管理 —— AcGiTransientManager 的 add/erase/update 系列，
//      把自绘对象挂进图形系统（kAcGiMain / kAcGiHighlight / ...）。
//
// 生命周期：加入瞬态的 Python drawable 会被登记在模块级 _transient_refs
// 字典里保活，erase 时移除；Python 侧对象被回收前必须先 erase。
// ===========================================================================
#include "stdafx.h"

class PyGiWorldDraw;
class PyGiViewportDraw;
class PyGiDrawableTraits;

// AcGiDrawable 的 Python 桥：子类在 Python 里实现
//   worldDraw(self, wd) -> bool
//   viewportDraw(self, vd) -> None
//   setAttributes(self, traits) -> int (AcGiDrawable::SetAttributesFlags)
class PyGiPythonDrawable : public AcGiDrawable {
public:
	PyGiPythonDrawable() = default;
	~PyGiPythonDrawable() override = default;

	Adesk::Boolean isPersistent() const override { return Adesk::kFalse; }
	AcDbObjectId id() const override { return AcDbObjectId::kNull; }
	AcGiDrawable::DrawableType drawableType() const override { return AcGiDrawable::kGeometry; }

	// Python 侧可选实现：bounds(self) -> (min: (x,y,z), max: (x,y,z)) | None
	bool bounds(AcDbExtents& bounds) const override;

protected:
	Adesk::UInt32 subSetAttributes(AcGiDrawableTraits* traits) override;
	Adesk::Boolean subWorldDraw(AcGiWorldDraw* wd) override;
	void subViewportDraw(AcGiViewportDraw* vd) override;
	Adesk::UInt32 subViewportDrawLogicalFlags(AcGiViewportDraw* vd) override;
};

void bind_PyGiTransient(pym& m);
