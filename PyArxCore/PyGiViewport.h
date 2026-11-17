#pragma once
// ===========================================================================
// PyGiViewport.h —— AcGiViewport 封装（SDK: acgi.h）。
//
// AcGiViewportDraw::viewport() 返回的只读视口信息对象：相机、投影矩阵、
// 像素/单位换算、前后裁剪、线型缩放等。原封装里完全没有，AcGiViewportDraw
// 也被迫只暴露 geometry()；此处补齐后 viewportDraw 里才算“能读到视口”。
//
// 对象由 SDK 传入（AcGiViewportDraw::viewport() 取得），不归 Python 侧创建，
// 因此不提供 __init__，也不负责释放。
// ===========================================================================
#include "stdafx.h"

class PyGiViewport {
public:
	explicit PyGiViewport(AcGiViewport* p = nullptr) : m_p(p) {}
	AcGiViewport* impObj() const { return m_p; }
	bool valid() const { return m_p != nullptr; }

	AcGeMatrix3d modelToEyeTransform() const;
	AcGeMatrix3d eyeToModelTransform() const;
	AcGeMatrix3d worldToEyeTransform() const;
	AcGeMatrix3d eyeToWorldTransform() const;

	bool isPerspective() const;
	std::pair<bool, AcGePoint3d> doPerspective(const AcGePoint3d& pt) const;
	std::pair<bool, AcGePoint3d> doInversePerspective(const AcGePoint3d& pt) const;

	AcGePoint2d numPixelsInUnitSquare(const AcGePoint3d& worldPt, bool includePerspective) const;

	AcGePoint3d cameraLocation() const;
	AcGePoint3d cameraTarget() const;
	AcGeVector3d cameraUpVector() const;

	unsigned long long viewportId() const;
	int acadWindowId() const;
	std::pair<AcGePoint2d, AcGePoint2d> viewportDcCorners() const;
	py::tuple frontAndBackClipValues() const;

	double linetypeScaleMultiplier() const;
	double linetypeGenerationCriteria() const;
	bool layerVisible(const AcDbObjectId& layerId) const;
	AcGeVector3d viewDir() const;

private:
	AcGiViewport* m_p = nullptr;
};

void bind_PyGiViewport(pym& m);
