#include "stdafx.h"
#include "PyAdsJigEnums.h"
#include "dbjig.h"
// JIG enums (AcEdJig / AcEdDragStyle)集中注册到 PyEnum; 不在 PyEd 类绑定内重复定义.
void bind_JigEnums(pym& m) {
	py::enum_<AcEdJig::DragStatus>(m, "DragStatus")
		.value("kModeless", AcEdJig::kModeless)
		.value("kNoChange", AcEdJig::kNoChange)
		.value("kCancel", AcEdJig::kCancel)
		.value("kOther", AcEdJig::kOther)
		.value("kNull", AcEdJig::kNull)
		.value("kNormal", AcEdJig::kNormal)
		.value("eNormal", AcEdJig::kNormal) // PyRx 同义名
		.value("eOk", AcEdJig::kNormal)     // PyRx 同义名
		.value("kKW1", AcEdJig::kKW1)
		.value("kKW2", AcEdJig::kKW2)
		.value("kKW3", AcEdJig::kKW3)
		.value("kKW4", AcEdJig::kKW4)
		.value("kKW5", AcEdJig::kKW5)
		.value("kKW6", AcEdJig::kKW6)
		.value("kKW7", AcEdJig::kKW7)
		.value("kKW8", AcEdJig::kKW8)
		.value("kKW9", AcEdJig::kKW9)
		.export_values();

	py::enum_<AcEdJig::UserInputControls>(m, "UserInputControls", py::arithmetic())
		.value("kGovernedByOrthoMode", AcEdJig::kGovernedByOrthoMode)
		.value("kNullResponseAccepted", AcEdJig::kNullResponseAccepted)
		.value("kDontEchoCancelForCtrlC", AcEdJig::kDontEchoCancelForCtrlC)
		.value("kDontUpdateLastPoint", AcEdJig::kDontUpdateLastPoint)
		.value("kNoDwgLimitsChecking", AcEdJig::kNoDwgLimitsChecking)
		.value("kNoZeroResponseAccepted", AcEdJig::kNoZeroResponseAccepted)
		.value("kNoNegativeResponseAccepted", AcEdJig::kNoNegativeResponseAccepted)
		.value("kAccept3dCoordinates", AcEdJig::kAccept3dCoordinates)
		.value("kAcceptMouseUpAsPoint", AcEdJig::kAcceptMouseUpAsPoint)
		.value("kAnyBlankTerminatesInput", AcEdJig::kAnyBlankTerminatesInput)
		.value("kInitialBlankTerminatesInput", AcEdJig::kInitialBlankTerminatesInput)
		.value("kAcceptOtherInputString", AcEdJig::kAcceptOtherInputString)
		.value("kGovernedByUCSDetect", AcEdJig::kGovernedByUCSDetect)
		.value("kNoZDirectionOrtho", AcEdJig::kNoZDirectionOrtho)
		.value("kImpliedFaceForUCSChange", AcEdJig::kImpliedFaceForUCSChange)
		.value("kUseBasePointElevation", AcEdJig::kUseBasePointElevation)
		.value("kDisableDirectDistanceInput", AcEdJig::kDisableDirectDistanceInput)
		.export_values();

	py::enum_<AcEdJig::CursorType>(m, "CursorType")
		.value("kNoSpecialCursor", AcEdJig::kNoSpecialCursor)
		.value("kCrosshair", AcEdJig::kCrosshair)
		.value("kRectCursor", AcEdJig::kRectCursor)
		.value("kRubberBand", AcEdJig::kRubberBand)
		.value("kNotRotated", AcEdJig::kNotRotated)
		.value("kTargetBox", AcEdJig::kTargetBox)
		.value("kRotatedCrosshair", AcEdJig::kRotatedCrosshair)
		.value("kCrosshairNoRotate", AcEdJig::kCrosshairNoRotate)
		.value("kInvisible", AcEdJig::kInvisible)
		.value("kEntitySelect", AcEdJig::kEntitySelect)
		.value("kParallelogram", AcEdJig::kParallelogram)
		.value("kEntitySelectNoPersp", AcEdJig::kEntitySelectNoPersp)
		.value("kPkfirstOrGrips", AcEdJig::kPkfirstOrGrips)
		.value("kCrosshairDashed", AcEdJig::kCrosshairDashed)
		.export_values();

	py::enum_<AcEdDragStyle::StyleType>(m, "DragStyleType")
		.value("kNone", AcEdDragStyle::kNone)
		.value("kHide", AcEdDragStyle::kHide)
		.value("kTransparent25", AcEdDragStyle::kTransparent25)
		.value("kTransparent75", AcEdDragStyle::kTransparent75)
		.value("kDeletedEffect", AcEdDragStyle::kDeletedEffect)
		.value("kHighlight", AcEdDragStyle::kHighlight)
		.value("kNotSet", AcEdDragStyle::kNotSet)
		.value("kOpacity", AcEdDragStyle::kOpacity)
		.export_values();
}
