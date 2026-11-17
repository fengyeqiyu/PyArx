#include "stdafx.h"
#include "PyGiViewport.h"
#include "PyGiTypes.h"

#define GI_VP_CHECK() \
	if (!m_p) throw std::runtime_error("AcGiViewport 无效（不在视口绘制上下文中）")

AcGeMatrix3d PyGiViewport::modelToEyeTransform() const {
	GI_VP_CHECK();
	AcGeMatrix3d m = AcGeMatrix3d::kIdentity;
	m_p->getModelToEyeTransform(m);
	return m;
}

AcGeMatrix3d PyGiViewport::eyeToModelTransform() const {
	GI_VP_CHECK();
	AcGeMatrix3d m = AcGeMatrix3d::kIdentity;
	m_p->getEyeToModelTransform(m);
	return m;
}

AcGeMatrix3d PyGiViewport::worldToEyeTransform() const {
	GI_VP_CHECK();
	AcGeMatrix3d m = AcGeMatrix3d::kIdentity;
	m_p->getWorldToEyeTransform(m);
	return m;
}

AcGeMatrix3d PyGiViewport::eyeToWorldTransform() const {
	GI_VP_CHECK();
	AcGeMatrix3d m = AcGeMatrix3d::kIdentity;
	m_p->getEyeToWorldTransform(m);
	return m;
}

bool PyGiViewport::isPerspective() const {
	GI_VP_CHECK();
	return m_p->isPerspective() == Adesk::kTrue;
}

std::pair<bool, AcGePoint3d> PyGiViewport::doPerspective(const AcGePoint3d& pt) const {
	GI_VP_CHECK();
	AcGePoint3d out = pt;
	const bool ok = m_p->doPerspective(out) == Adesk::kTrue;
	return { ok, out };
}

std::pair<bool, AcGePoint3d> PyGiViewport::doInversePerspective(const AcGePoint3d& pt) const {
	GI_VP_CHECK();
	AcGePoint3d out = pt;
	const bool ok = m_p->doInversePerspective(out) == Adesk::kTrue;
	return { ok, out };
}

AcGePoint2d PyGiViewport::numPixelsInUnitSquare(const AcGePoint3d& worldPt, bool includePerspective) const {
	GI_VP_CHECK();
	AcGePoint2d area(0.0, 0.0);
	m_p->getNumPixelsInUnitSquare(worldPt, area, includePerspective);
	return area;
}

AcGePoint3d PyGiViewport::cameraLocation() const {
	GI_VP_CHECK();
	AcGePoint3d pt = AcGePoint3d::kOrigin;
	m_p->getCameraLocation(pt);
	return pt;
}

AcGePoint3d PyGiViewport::cameraTarget() const {
	GI_VP_CHECK();
	AcGePoint3d pt = AcGePoint3d::kOrigin;
	m_p->getCameraTarget(pt);
	return pt;
}

AcGeVector3d PyGiViewport::cameraUpVector() const {
	GI_VP_CHECK();
	AcGeVector3d v = AcGeVector3d::kZAxis;
	m_p->getCameraUpVector(v);
	return v;
}

unsigned long long PyGiViewport::viewportId() const {
	GI_VP_CHECK();
	return static_cast<unsigned long long>(m_p->viewportId());
}

int PyGiViewport::acadWindowId() const {
	GI_VP_CHECK();
	return static_cast<int>(m_p->acadWindowId());
}

std::pair<AcGePoint2d, AcGePoint2d> PyGiViewport::viewportDcCorners() const {
	GI_VP_CHECK();
	AcGePoint2d ll(0.0, 0.0), ur(0.0, 0.0);
	m_p->getViewportDcCorners(ll, ur);
	return { ll, ur };
}

py::tuple PyGiViewport::frontAndBackClipValues() const {
	GI_VP_CHECK();
	Adesk::Boolean front = Adesk::kFalse, back = Adesk::kFalse;
	double fz = 0.0, bz = 0.0;
	m_p->getFrontAndBackClipValues(front, back, fz, bz);
	return py::make_tuple(front == Adesk::kTrue, back == Adesk::kTrue, fz, bz);
}

double PyGiViewport::linetypeScaleMultiplier() const {
	GI_VP_CHECK();
	return m_p->linetypeScaleMultiplier();
}

double PyGiViewport::linetypeGenerationCriteria() const {
	GI_VP_CHECK();
	return m_p->linetypeGenerationCriteria();
}

bool PyGiViewport::layerVisible(const AcDbObjectId& layerId) const {
	GI_VP_CHECK();
	return m_p->layerVisible(layerId) == Adesk::kTrue;
}

AcGeVector3d PyGiViewport::viewDir() const {
	GI_VP_CHECK();
	return m_p->viewDir();
}

void bind_PyGiViewport(pym& m) {
	py::class_<PyGiViewport>(m, "AcGiViewport", "视口信息（AcGiViewportDraw::viewport() 取得）")
		.def("valid", &PyGiViewport::valid)
		// ---- 变换矩阵 ----
		.def("modelToEyeTransform", &PyGiViewport::modelToEyeTransform)
		.def("eyeToModelTransform", &PyGiViewport::eyeToModelTransform)
		.def("worldToEyeTransform", &PyGiViewport::worldToEyeTransform)
		.def("eyeToWorldTransform", &PyGiViewport::eyeToWorldTransform)
		// ---- 透视 ----
		.def("isPerspective", &PyGiViewport::isPerspective)
		.def("doPerspective", &PyGiViewport::doPerspective, py::arg("point"), "返回 (ok, 透视后点)")
		.def("doInversePerspective", &PyGiViewport::doInversePerspective, py::arg("point"), "返回 (ok, 反透视后点)")
		.def("getNumPixelsInUnitSquare", &PyGiViewport::numPixelsInUnitSquare,
			py::arg("world_point"), py::arg("include_perspective") = true)
		// ---- 相机 ----
		.def("getCameraLocation", &PyGiViewport::cameraLocation)
		.def("getCameraTarget", &PyGiViewport::cameraTarget)
		.def("getCameraUpVector", &PyGiViewport::cameraUpVector)
		.def("viewDir", &PyGiViewport::viewDir)
		// ---- 视口标识 / 范围 ----
		.def("viewportId", &PyGiViewport::viewportId)
		.def("acadWindowId", &PyGiViewport::acadWindowId)
		.def("getViewportDcCorners", &PyGiViewport::viewportDcCorners, "返回 (左下, 右上) 的 DC 坐标")
		.def("getFrontAndBackClipValues", &PyGiViewport::frontAndBackClipValues,
			"返回 (clip_front, clip_back, front_z, back_z)")
		// ---- 线型 ----
		.def("linetypeScaleMultiplier", &PyGiViewport::linetypeScaleMultiplier)
		.def("linetypeGenerationCriteria", &PyGiViewport::linetypeGenerationCriteria)
		.def("layerVisible", &PyGiViewport::layerVisible, py::arg("layer_id"));
}
