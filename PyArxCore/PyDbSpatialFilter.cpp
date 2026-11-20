#include "stdafx.h"
#include "PyErrorStatusException.h"
#include "PyDbSpatialFilter.h"
#include "PyDbExtents.h"

PyWrapper_Cpp(DbSpatialFilter);

void bind_PyDbSpatialFilter(pym& m) {
	py::class_<PyDbSpatialFilter, PyDbFilter>(m, "PyDbSpatialFilter")
		.def(py::init<>())
		.def(py::init<AcDbSpatialFilter*>(), py::arg("p"))
		PyWrapper_BIND(DbSpatialFilter)
		// ---- clipVolumeIntersectsExtents(ext) -> bool ----
		.def("clipVolumeIntersectsExtents", [](const PyDbSpatialFilter& s, const PyDbExtents3d& ext) -> bool {
			if (!s.impObj()) return false;
			return s.impObj()->clipVolumeIntersectsExtents(ext.impObj()) != Adesk::kFalse;
		}, py::arg("ext"), "裁剪体是否与给定包围盒相交")
		// ---- getDefinition() -> (pts2d, normal, elevation, frontClip, backClip, enabled) ----
		.def("getDefinition", [](const PyDbSpatialFilter& s)
			-> std::tuple<AcGePoint2dArray, AcGeVector3d, double, double, double, bool> {
			AcGePoint2dArray pts;
			AcGeVector3d normal;
			double elevation = 0, frontClip = 0, backClip = 0;
			Adesk::Boolean enabled = Adesk::kFalse;
			if (!s.impObj()) return { AcGePoint2dArray(), AcGeVector3d::kZAxis, 0, 0, 0, false };
			if (!PyWarnIfError(s.impObj()->getDefinition(pts, normal, elevation, frontClip, backClip, enabled),
				L"AcDbSpatialFilter::getDefinition"))
				return { AcGePoint2dArray(), AcGeVector3d::kZAxis, 0, 0, 0, false };
			return { pts, normal, elevation, frontClip, backClip, enabled != Adesk::kFalse };
		}, "读取裁剪多边形定义")
		// ---- setDefinition(pts, normal, elevation, frontClip, backClip, enabled) -> bool ----
		.def("setDefinition", [](PyDbSpatialFilter& s, const AcGePoint2dArray& pts,
			const AcGeVector3d& normal, double elevation, double frontClip, double backClip, bool enabled) -> bool {
			if (!s.impObj()) return false;
			return PyWarnIfError(
				s.impObj()->setDefinition(pts, normal, elevation, frontClip, backClip,
					enabled ? Adesk::kTrue : Adesk::kFalse),
				L"AcDbSpatialFilter::setDefinition");
		}, py::arg("pts"), py::arg("normal"), py::arg("elevation"),
			py::arg("front_clip"), py::arg("back_clip"), py::arg("enabled"),
			"设置裁剪多边形定义")
		// ---- getVolume() -> (fromPt, toPt, upDir, viewField) ----
		.def("getVolume", [](const PyDbSpatialFilter& s)
			-> std::tuple<AcGePoint3d, AcGePoint3d, AcGeVector3d, AcGeVector2d> {
			AcGePoint3d fromPt, toPt;
			AcGeVector3d upDir;
			AcGeVector2d viewField;
			if (!s.impObj()) return { AcGePoint3d(), AcGePoint3d(), AcGeVector3d::kZAxis, AcGeVector2d() };
			if (!PyWarnIfError(s.impObj()->getVolume(fromPt, toPt, upDir, viewField),
				L"AcDbSpatialFilter::getVolume"))
				return { AcGePoint3d::kOrigin, AcGePoint3d::kOrigin,
					AcGeVector3d::kZAxis, AcGeVector2d() };
			return { fromPt, toPt, upDir, viewField };
		}, "返回视图体 (相机点, 目标点, up, viewField)")
		// ---- hasPerspectiveCamera() -> bool ----
		.def("hasPerspectiveCamera", [](const PyDbSpatialFilter& s) -> bool {
			if (s.isNull()) return false;
			return s.impObj()->hasPerspectiveCamera() != Adesk::kFalse;
		}, "是否使用透视相机")
		// ---- isInverted() / setInverted ----
		.def("isInverted", [](const PyDbSpatialFilter& s) -> bool {
			if (s.isNull()) return false;
			return s.impObj()->isInverted();
		}, "裁剪是否反转")
		.def("setInverted", [](PyDbSpatialFilter& s, bool b) -> bool {
			if (s.isNull()) return false;
			return PyWarnIfError(s.impObj()->setInverted(b), L"AcDbSpatialFilter::setInverted");
		}, py::arg("inverted"), "设置裁剪反转")
		// ---- queryBounds(ext)：SDK 中带 pRefBlkRef 的重载标注 To Be Obsoleted，这里绑无参版本 ----
		.def("queryBounds", [](const PyDbSpatialFilter& s, PyDbExtents3d& ext) {
			// 直接写入用户提供的 AcDbExtents（m_isEmpty 由用户自行管理）
			if (s.isNull() || !ext.impObj()) return;
			s.impObj()->queryBounds(ext.impObj());
		}, py::arg("ext"), "计算裁剪后包围盒（写入 ext.impObj()）")
		// ---- setPerspectiveCamera(fromPt) -> bool ----
		.def("setPerspectiveCamera", [](PyDbSpatialFilter& s, const AcGePoint3d& fromPt) -> bool {
			if (s.isNull()) return false;
			return PyWarnIfError(s.impObj()->setPerspectiveCamera(fromPt),
				L"AcDbSpatialFilter::setPerspectiveCamera");
		}, py::arg("from_pt"), "设置透视相机位置")
		;
}
