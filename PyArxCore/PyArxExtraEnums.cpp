#include "stdafx.h"
#include "PyArxExtraEnums.h"

namespace py = pybind11;

namespace {
// 每个枚举一个独立 tag 类型：pybind11 的 py::enum_ 需要真实的 C++ enum 类型做标识，
// 这些枚举只是「PyRx 命名常量表」，没有对应的 PyArx 原生枚举，故用模板 tag 造类型。
template <int N> struct Tag { enum type : int { }; };
}

void bind_PyArxExtraEnums(py::module_& root)
{
    // 别名：PyRx 在 Db 下也放这两个枚举名，PyArx 侧类型已存在于 Gi
    {
        auto db = py::reinterpret_borrow<py::module_>(root.attr("Db"));
        auto gi = py::reinterpret_borrow<py::module_>(root.attr("Gi"));
        db.attr("ColorMethod") = gi.attr("AcCmColorMethod");
        db.attr("TransparencyMethod") = gi.attr("AcCmTransparencyMethod");
    }

	{
	auto m = py::reinterpret_borrow<py::module_>(root.attr("Db"));
	py::enum_<Tag<1>::type>(m, "ACIcolorMethod", "PyRx 兼容枚举（值取自 PyRx 桩）")
		.value("kACIbyBlock", static_cast<Tag<1>::type>(0))
		.value("kACIbyLayer", static_cast<Tag<1>::type>(256))
		.value("kACIclear", static_cast<Tag<1>::type>(0))
		.value("kACIforeground", static_cast<Tag<1>::type>(7))
		.value("kACIfrozenLayer", static_cast<Tag<1>::type>(-32700))
		.value("kACImaximum", static_cast<Tag<1>::type>(255))
		.value("kACIminimum", static_cast<Tag<1>::type>(-255))
		.value("kACInone", static_cast<Tag<1>::type>(257))
		.value("kACIstandard", static_cast<Tag<1>::type>(7));

	py::enum_<Tag<2>::type>(m, "AcCmDialogTabs", "PyRx 兼容枚举（值取自 PyRx 桩）")
		.value("kACITab", static_cast<Tag<2>::type>(1))
		.value("kColorBookTab", static_cast<Tag<2>::type>(4))
		.value("kTrueColorTab", static_cast<Tag<2>::type>(2));

	py::enum_<Tag<3>::type>(m, "AcGeoMapResolution", "PyRx 兼容枚举（值取自 PyRx 桩）")
		.value("kCoarse", static_cast<Tag<3>::type>(-1))
		.value("kFine", static_cast<Tag<3>::type>(1))
		.value("kFiner", static_cast<Tag<3>::type>(2))
		.value("kOptimal", static_cast<Tag<3>::type>(0));

	py::enum_<Tag<4>::type>(m, "AcGeoMapType", "PyRx 兼容枚举（值取自 PyRx 桩）")
		.value("kAerial", static_cast<Tag<4>::type>(1))
		.value("kBingAerial", static_cast<Tag<4>::type>(1))
		.value("kBingHybrid", static_cast<Tag<4>::type>(3))
		.value("kBingRoad", static_cast<Tag<4>::type>(2))
		.value("kEsriDarkGray", static_cast<Tag<4>::type>(8))
		.value("kEsriImagery", static_cast<Tag<4>::type>(4))
		.value("kEsriLightGray", static_cast<Tag<4>::type>(7))
		.value("kEsriOpenStreetMap", static_cast<Tag<4>::type>(5))
		.value("kEsriStreets", static_cast<Tag<4>::type>(6))
		.value("kHybrid", static_cast<Tag<4>::type>(3))
		.value("kNoMap", static_cast<Tag<4>::type>(0))
		.value("kRoad", static_cast<Tag<4>::type>(2));

	py::enum_<Tag<5>::type>(m, "AdskBoolean", "PyRx 兼容枚举（值取自 PyRx 桩）")
		.value("kFalse", static_cast<Tag<5>::type>(0))
		.value("kTrue", static_cast<Tag<5>::type>(1));

	py::enum_<Tag<6>::type>(m, "AnnoType", "PyRx 兼容枚举（值取自 PyRx 桩）")
		.value("kBlockRef", static_cast<Tag<6>::type>(2))
		.value("kFcf", static_cast<Tag<6>::type>(1))
		.value("kMText", static_cast<Tag<6>::type>(0))
		.value("kNoAnno", static_cast<Tag<6>::type>(3));

	py::enum_<Tag<7>::type>(m, "AnnotativeStates", "PyRx 兼容枚举（值取自 PyRx 桩）")
		.value("kFalse", static_cast<Tag<7>::type>(1))
		.value("kNotApplicable", static_cast<Tag<7>::type>(2))
		.value("kTrue", static_cast<Tag<7>::type>(0));

	py::enum_<Tag<8>::type>(m, "AssocConstraintType", "PyRx 兼容枚举（值取自 PyRx 桩）")
		.value("kAngle0AssocConstraintType", static_cast<Tag<8>::type>(4))
		.value("kAngle1AssocConstraintType", static_cast<Tag<8>::type>(5))
		.value("kAngle2AssocConstraintType", static_cast<Tag<8>::type>(6))
		.value("kAngle3AssocConstraintType", static_cast<Tag<8>::type>(7))
		.value("kDiameterAssocConstraintType", static_cast<Tag<8>::type>(9))
		.value("kDistanceAssocConstraintType", static_cast<Tag<8>::type>(1))
		.value("kHorizontalDistanceAssocConstraintType", static_cast<Tag<8>::type>(2))
		.value("kNoneAssocConstraintType", static_cast<Tag<8>::type>(0))
		.value("kRadiusAssocConstraintType", static_cast<Tag<8>::type>(8))
		.value("kVerticalDistanceAssocConstraintType", static_cast<Tag<8>::type>(3));

	py::enum_<Tag<9>::type>(m, "AssocDraggingState", "PyRx 兼容枚举（值取自 PyRx 桩）")
		.value("kFirstSampleAssocDraggingState", static_cast<Tag<9>::type>(1))
		.value("kIntermediateSampleAssocDraggingState", static_cast<Tag<9>::type>(2))
		.value("kLastSampleAssocDraggingState", static_cast<Tag<9>::type>(3))
		.value("kNotDraggingAssocDraggingState", static_cast<Tag<9>::type>(0));

	py::enum_<Tag<10>::type>(m, "AssocEvaluationMode", "PyRx 兼容枚举（值取自 PyRx 桩）")
		.value("kModifyActionAssocEvaluationMode", static_cast<Tag<10>::type>(1))
		.value("kModifyObjectsAssocEvaluationMode", static_cast<Tag<10>::type>(0));

	py::enum_<Tag<11>::type>(m, "AssocEvaluationPriority", "PyRx 兼容枚举（值取自 PyRx 桩）")
		.value("kCanBeEvaluatedAssocEvaluationPriority", static_cast<Tag<11>::type>(1000))
		.value("kCannotDermineAssocEvaluationPriority", static_cast<Tag<11>::type>(0))
		.value("kIsUpToDateAssocStatus", static_cast<Tag<11>::type>(-1000));

	py::enum_<Tag<12>::type>(m, "AssocFlags", "PyRx 兼容枚举（值取自 PyRx 桩）")
		.value("kFirstPointRef", static_cast<Tag<12>::type>(1))
		.value("kFourthPointRef", static_cast<Tag<12>::type>(8))
		.value("kSecondPointRef", static_cast<Tag<12>::type>(2))
		.value("kThirdPointRef", static_cast<Tag<12>::type>(4));

	py::enum_<Tag<13>::type>(m, "AssocStatus", "PyRx 兼容枚举（值取自 PyRx 桩）")
		.value("kChangedDirectlyAssocStatus", static_cast<Tag<13>::type>(1))
		.value("kChangedNoDifferenceAssocStatus", static_cast<Tag<13>::type>(3))
		.value("kChangedTransitivelyAssocStatus", static_cast<Tag<13>::type>(2))
		.value("kErasedAssocStatus", static_cast<Tag<13>::type>(5))
		.value("kFailedToEvaluateAssocStatus", static_cast<Tag<13>::type>(4))
		.value("kIsUpToDateAssocStatus", static_cast<Tag<13>::type>(0))
		.value("kSuppressedAssocStatus", static_cast<Tag<13>::type>(6))
		.value("kUnresolvedAssocStatus", static_cast<Tag<13>::type>(7));

	py::enum_<Tag<14>::type>(m, "AssocTransformationType", "PyRx 兼容枚举（值取自 PyRx 桩）")
		.value("kMove", static_cast<Tag<14>::type>(3))
		.value("kNotSpecified", static_cast<Tag<14>::type>(0))
		.value("kRotate", static_cast<Tag<14>::type>(2))
		.value("kStretch", static_cast<Tag<14>::type>(1));

	py::enum_<Tag<15>::type>(m, "BlockScaling", "PyRx 兼容枚举（值取自 PyRx 桩）")
		.value("kAny", static_cast<Tag<15>::type>(0))
		.value("kUniform", static_cast<Tag<15>::type>(1));

	py::enum_<Tag<16>::type>(m, "BoolOperType", "PyRx 兼容枚举（值取自 PyRx 桩）")
		.value("kBoolIntersect", static_cast<Tag<16>::type>(1))
		.value("kBoolSubtract", static_cast<Tag<16>::type>(2))
		.value("kBoolUnite", static_cast<Tag<16>::type>(0));

	py::enum_<Tag<17>::type>(m, "CellAlignment", "PyRx 兼容枚举（值取自 PyRx 桩）")
		.value("kBottomCenter", static_cast<Tag<17>::type>(8))
		.value("kBottomLeft", static_cast<Tag<17>::type>(7))
		.value("kBottomRight", static_cast<Tag<17>::type>(9))
		.value("kMiddleCenter", static_cast<Tag<17>::type>(5))
		.value("kMiddleLeft", static_cast<Tag<17>::type>(4))
		.value("kMiddleRight", static_cast<Tag<17>::type>(6))
		.value("kTopCenter", static_cast<Tag<17>::type>(2))
		.value("kTopLeft", static_cast<Tag<17>::type>(1))
		.value("kTopRight", static_cast<Tag<17>::type>(3));

	py::enum_<Tag<18>::type>(m, "CellContentLayout", "PyRx 兼容枚举（值取自 PyRx 桩）")
		.value("kCellContentLayoutStackedHorizontal", static_cast<Tag<18>::type>(2))
		.value("kCellContentLayoutStackedVertical", static_cast<Tag<18>::type>(4))
		.value("kCellStateNone", static_cast<Tag<18>::type>(1));

	py::enum_<Tag<19>::type>(m, "CellContentType", "PyRx 兼容枚举（值取自 PyRx 桩）")
		.value("kCellContentTypeBlock", static_cast<Tag<19>::type>(4))
		.value("kCellContentTypeField", static_cast<Tag<19>::type>(2))
		.value("kCellContentTypeUnknown", static_cast<Tag<19>::type>(0))
		.value("kCellContentTypeValue", static_cast<Tag<19>::type>(1));

	py::enum_<Tag<20>::type>(m, "CellEdgeMask", "PyRx 兼容枚举（值取自 PyRx 桩）")
		.value("kBottomMask", static_cast<Tag<20>::type>(4))
		.value("kLeftMask", static_cast<Tag<20>::type>(8))
		.value("kRightMask", static_cast<Tag<20>::type>(2))
		.value("kTopMask", static_cast<Tag<20>::type>(1));

	py::enum_<Tag<21>::type>(m, "CellMargin", "PyRx 兼容枚举（值取自 PyRx 桩）")
		.value("kCellMarginBottom", static_cast<Tag<21>::type>(4))
		.value("kCellMarginHorzSpacing", static_cast<Tag<21>::type>(16))
		.value("kCellMarginLeft", static_cast<Tag<21>::type>(2))
		.value("kCellMarginRight", static_cast<Tag<21>::type>(8))
		.value("kCellMarginTop", static_cast<Tag<21>::type>(1))
		.value("kCellMarginVertSpacing", static_cast<Tag<21>::type>(32));

	py::enum_<Tag<22>::type>(m, "CellOption", "PyRx 兼容枚举（值取自 PyRx 桩）")
		.value("kAerial", static_cast<Tag<22>::type>(1))
		.value("kNoMap", static_cast<Tag<22>::type>(0));

	py::enum_<Tag<23>::type>(m, "CellProperty", "PyRx 兼容枚举（值取自 PyRx 桩）")
		.value("kCellPropAlignment", static_cast<Tag<23>::type>(16))
		.value("kCellPropAll", static_cast<Tag<23>::type>(524287))
		.value("kCellPropAutoScale", static_cast<Tag<23>::type>(256))
		.value("kCellPropBackgroundColor", static_cast<Tag<23>::type>(512))
		.value("kCellPropBitProperties", static_cast<Tag<23>::type>(98560))
		.value("kCellPropContent", static_cast<Tag<23>::type>(495))
		.value("kCellPropContentColor", static_cast<Tag<23>::type>(32))
		.value("kCellPropContentLayout", static_cast<Tag<23>::type>(16384))
		.value("kCellPropDataFormat", static_cast<Tag<23>::type>(2))
		.value("kCellPropDataType", static_cast<Tag<23>::type>(1))
		.value("kCellPropDataTypeAndFormat", static_cast<Tag<23>::type>(3))
		.value("kCellPropFlowDirBtoT", static_cast<Tag<23>::type>(65536))
		.value("kCellPropInvalid", static_cast<Tag<23>::type>(0))
		.value("kCellPropMarginBottom", static_cast<Tag<23>::type>(8192))
		.value("kCellPropMarginHorzSpacing", static_cast<Tag<23>::type>(131072))
		.value("kCellPropMarginLeft", static_cast<Tag<23>::type>(1024))
		.value("kCellPropMarginRight", static_cast<Tag<23>::type>(4096))
		.value("kCellPropMarginTop", static_cast<Tag<23>::type>(2048))
		.value("kCellPropMarginVertSpacing", static_cast<Tag<23>::type>(262144))
		.value("kCellPropMergeAll", static_cast<Tag<23>::type>(32768))
		.value("kCellPropRotation", static_cast<Tag<23>::type>(4))
		.value("kCellPropScale", static_cast<Tag<23>::type>(8))
		.value("kCellPropTextHeight", static_cast<Tag<23>::type>(128))
		.value("kCellPropTextStyle", static_cast<Tag<23>::type>(64));

	py::enum_<Tag<24>::type>(m, "CellState", "PyRx 兼容枚举（值取自 PyRx 桩）")
		.value("kAllCellStates", static_cast<Tag<24>::type>(127))
		.value("kCellStateContentLocked", static_cast<Tag<24>::type>(1))
		.value("kCellStateContentModifiedAfterUpdate", static_cast<Tag<24>::type>(8))
		.value("kCellStateContentReadOnly", static_cast<Tag<24>::type>(2))
		.value("kCellStateFormatLocked", static_cast<Tag<24>::type>(16))
		.value("kCellStateFormatModifiedAfterUpdate", static_cast<Tag<24>::type>(64))
		.value("kCellStateFormatReadOnly", static_cast<Tag<24>::type>(32))
		.value("kCellStateLinked", static_cast<Tag<24>::type>(4))
		.value("kCellStateNone", static_cast<Tag<24>::type>(0));

	py::enum_<Tag<25>::type>(m, "CellType", "PyRx 兼容枚举（值取自 PyRx 桩）")
		.value("kBlockCell", static_cast<Tag<25>::type>(2))
		.value("kMultipleContentCell", static_cast<Tag<25>::type>(3))
		.value("kTextCell", static_cast<Tag<25>::type>(1))
		.value("kUnknownCell", static_cast<Tag<25>::type>(0));

	py::enum_<Tag<26>::type>(m, "CenterMarkType", "PyRx 兼容枚举（值取自 PyRx 桩）")
		.value("kLine", static_cast<Tag<26>::type>(1))
		.value("kMark", static_cast<Tag<26>::type>(0))
		.value("kNone", static_cast<Tag<26>::type>(2));

	py::enum_<Tag<27>::type>(m, "ClipBoundaryType", "PyRx 兼容枚举（值取自 PyRx 桩）")
		.value("kInvalid", static_cast<Tag<27>::type>(0))
		.value("kPoly", static_cast<Tag<27>::type>(2))
		.value("kRect", static_cast<Tag<27>::type>(1));

	py::enum_<Tag<28>::type>(m, "CollisionType", "PyRx 兼容枚举（值取自 PyRx 桩）")
		.value("kCollisionTypeNone", static_cast<Tag<28>::type>(0))
		.value("kCollisionTypeSolid", static_cast<Tag<28>::type>(1));

	py::enum_<Tag<29>::type>(m, "DatabaseOpenMode", "PyRx 兼容枚举（值取自 PyRx 桩）")
		.value("kForReadAndAllShare", static_cast<Tag<29>::type>(3))
		.value("kForReadAndReadShare", static_cast<Tag<29>::type>(1))
		.value("kForReadAndWriteNoShare", static_cast<Tag<29>::type>(2))
		.value("kTryForReadShare", static_cast<Tag<29>::type>(4));

	py::enum_<Tag<30>::type>(m, "DeepCloneType", "PyRx 兼容枚举（值取自 PyRx 桩）")
		.value("kDcBlock", static_cast<Tag<30>::type>(2))
		.value("kDcCopy", static_cast<Tag<30>::type>(0))
		.value("kDcExplode", static_cast<Tag<30>::type>(1))
		.value("kDcInsert", static_cast<Tag<30>::type>(6))
		.value("kDcInsertCopy", static_cast<Tag<30>::type>(10))
		.value("kDcObjects", static_cast<Tag<30>::type>(8))
		.value("kDcSymTableMerge", static_cast<Tag<30>::type>(4))
		.value("kDcWblkObjects", static_cast<Tag<30>::type>(11))
		.value("kDcWblock", static_cast<Tag<30>::type>(7))
		.value("kDcXrefBind", static_cast<Tag<30>::type>(3))
		.value("kDcXrefInsert", static_cast<Tag<30>::type>(9));

	py::enum_<Tag<31>::type>(m, "DimArrowFlags", "PyRx 兼容枚举（值取自 PyRx 桩）")
		.value("kAerial", static_cast<Tag<31>::type>(1))
		.value("kNoMap", static_cast<Tag<31>::type>(0));

	py::enum_<Tag<32>::type>(m, "DimAssocPointType", "PyRx 兼容枚举（值取自 PyRx 桩）")
		.value("kAngLineEnd", static_cast<Tag<32>::type>(3))
		.value("kAngLineStart", static_cast<Tag<32>::type>(2))
		.value("kCenterPoint", static_cast<Tag<32>::type>(1))
		.value("kChordPoint", static_cast<Tag<32>::type>(0))
		.value("kDefiningPoint", static_cast<Tag<32>::type>(1))
		.value("kFarChordPoint", static_cast<Tag<32>::type>(1))
		.value("kJogPoint", static_cast<Tag<32>::type>(3))
		.value("kLeaderPoint", static_cast<Tag<32>::type>(0))
		.value("kOriginPoint", static_cast<Tag<32>::type>(0))
		.value("kOverrideCenterPoint", static_cast<Tag<32>::type>(2))
		.value("kVertexPoint", static_cast<Tag<32>::type>(2))
		.value("kXline1End", static_cast<Tag<32>::type>(1))
		.value("kXline1Point", static_cast<Tag<32>::type>(0))
		.value("kXline1Start", static_cast<Tag<32>::type>(0))
		.value("kXline2End", static_cast<Tag<32>::type>(3))
		.value("kXline2Point", static_cast<Tag<32>::type>(1))
		.value("kXline2Start", static_cast<Tag<32>::type>(2));

	py::enum_<Tag<33>::type>(m, "DimInspect", "PyRx 兼容枚举（值取自 PyRx 桩）")
		.value("kShapeAngular", static_cast<Tag<33>::type>(2))
		.value("kShapeLabel", static_cast<Tag<33>::type>(16))
		.value("kShapeNone", static_cast<Tag<33>::type>(4))
		.value("kShapeRate", static_cast<Tag<33>::type>(32))
		.value("kShapeRemove", static_cast<Tag<33>::type>(0))
		.value("kShapeRound", static_cast<Tag<33>::type>(1));

	py::enum_<Tag<34>::type>(m, "DragStat", "PyRx 兼容枚举（值取自 PyRx 桩）")
		.value("kDragAbort", static_cast<Tag<34>::type>(2))
		.value("kDragEnd", static_cast<Tag<34>::type>(1))
		.value("kDragStart", static_cast<Tag<34>::type>(0));

	py::enum_<Tag<35>::type>(m, "DrawLeaderOrderType", "PyRx 兼容枚举（值取自 PyRx 桩）")
		.value("kDrawLeaderHeadFirst", static_cast<Tag<35>::type>(0))
		.value("kDrawLeaderTailFirst", static_cast<Tag<35>::type>(1));

	py::enum_<Tag<36>::type>(m, "DrawMLeaderOrderType", "PyRx 兼容枚举（值取自 PyRx 桩）")
		.value("kDrawContentFirst", static_cast<Tag<36>::type>(0))
		.value("kDrawLeaderFirst", static_cast<Tag<36>::type>(1));

	py::enum_<Tag<37>::type>(m, "DuplicateRecordCloning", "PyRx 兼容枚举（值取自 PyRx 桩）")
		.value("kDrcIgnore", static_cast<Tag<37>::type>(1))
		.value("kDrcMangleName", static_cast<Tag<37>::type>(4))
		.value("kDrcNotApplicable", static_cast<Tag<37>::type>(0))
		.value("kDrcReplace", static_cast<Tag<37>::type>(2))
		.value("kDrcUnmangleName", static_cast<Tag<37>::type>(5))
		.value("kDrcXrefMangleName", static_cast<Tag<37>::type>(3));

	py::enum_<Tag<38>::type>(m, "DwgDataType", "PyRx 兼容枚举（值取自 PyRx 桩）")
		.value("kDwg3Real", static_cast<Tag<38>::type>(12))
		.value("kDwgBChunk", static_cast<Tag<38>::type>(6))
		.value("kDwgHandle", static_cast<Tag<38>::type>(7))
		.value("kDwgHardOwnershipId", static_cast<Tag<38>::type>(8))
		.value("kDwgHardPointerId", static_cast<Tag<38>::type>(10))
		.value("kDwgInt16", static_cast<Tag<38>::type>(3))
		.value("kDwgInt32", static_cast<Tag<38>::type>(2))
		.value("kDwgInt64", static_cast<Tag<38>::type>(13))
		.value("kDwgInt8", static_cast<Tag<38>::type>(4))
		.value("kDwgNotRecognized", static_cast<Tag<38>::type>(19))
		.value("kDwgNull", static_cast<Tag<38>::type>(0))
		.value("kDwgReal", static_cast<Tag<38>::type>(1))
		.value("kDwgSoftOwnershipId", static_cast<Tag<38>::type>(9))
		.value("kDwgSoftPointerId", static_cast<Tag<38>::type>(11))
		.value("kDwgText", static_cast<Tag<38>::type>(5));

	py::enum_<Tag<39>::type>(m, "DwgVersion", "PyRx 兼容枚举（值取自 PyRx 桩）")
		.value("kDHL_1001", static_cast<Tag<39>::type>(8))
		.value("kDHL_1002", static_cast<Tag<39>::type>(9))
		.value("kDHL_1003", static_cast<Tag<39>::type>(10))
		.value("kDHL_1004", static_cast<Tag<39>::type>(11))
		.value("kDHL_1005", static_cast<Tag<39>::type>(12))
		.value("kDHL_1006", static_cast<Tag<39>::type>(13))
		.value("kDHL_1007", static_cast<Tag<39>::type>(14))
		.value("kDHL_1008", static_cast<Tag<39>::type>(15))
		.value("kDHL_1009", static_cast<Tag<39>::type>(16))
		.value("kDHL_1010", static_cast<Tag<39>::type>(17))
		.value("kDHL_1011", static_cast<Tag<39>::type>(18))
		.value("kDHL_1012", static_cast<Tag<39>::type>(19))
		.value("kDHL_1013", static_cast<Tag<39>::type>(20))
		.value("kDHL_1014", static_cast<Tag<39>::type>(21))
		.value("kDHL_1015", static_cast<Tag<39>::type>(23))
		.value("kDHL_1021", static_cast<Tag<39>::type>(27))
		.value("kDHL_1024", static_cast<Tag<39>::type>(29))
		.value("kDHL_1027", static_cast<Tag<39>::type>(31))
		.value("kDHL_1032", static_cast<Tag<39>::type>(33))
		.value("kDHL_1500", static_cast<Tag<39>::type>(22))
		.value("kDHL_1800", static_cast<Tag<39>::type>(25))
		.value("kDHL_1800a", static_cast<Tag<39>::type>(24))
		.value("kDHL_2100a", static_cast<Tag<39>::type>(26))
		.value("kDHL_2400a", static_cast<Tag<39>::type>(28))
		.value("kDHL_2700a", static_cast<Tag<39>::type>(30))
		.value("kDHL_3200a", static_cast<Tag<39>::type>(32))
		.value("kDHL_AC1_2", static_cast<Tag<39>::type>(1))
		.value("kDHL_AC1_40", static_cast<Tag<39>::type>(2))
		.value("kDHL_AC1_50", static_cast<Tag<39>::type>(3))
		.value("kDHL_AC2_10", static_cast<Tag<39>::type>(5))
		.value("kDHL_AC2_20", static_cast<Tag<39>::type>(4))
		.value("kDHL_AC2_21", static_cast<Tag<39>::type>(6))
		.value("kDHL_AC2_22", static_cast<Tag<39>::type>(7))
		.value("kDHL_CURRENT", static_cast<Tag<39>::type>(33))
		.value("kDHL_MC0_0", static_cast<Tag<39>::type>(0))
		.value("kDHL_Max", static_cast<Tag<39>::type>(255))
		.value("kDHL_Newest", static_cast<Tag<39>::type>(33))
		.value("kDHL_Unknown", static_cast<Tag<39>::type>(254));

	py::enum_<Tag<40>::type>(m, "DynUnitsType", "PyRx 兼容枚举（值取自 PyRx 桩）")
		.value("kAngular", static_cast<Tag<40>::type>(1))
		.value("kArea", static_cast<Tag<40>::type>(3))
		.value("kDistance", static_cast<Tag<40>::type>(2))
		.value("kNoUnits", static_cast<Tag<40>::type>(0));

	py::enum_<Tag<41>::type>(m, "EraseMask", "PyRx 兼容枚举（值取自 PyRx 桩）")
		.value("kAllEmptyObj", static_cast<Tag<41>::type>(-1))
		.value("kEmptyText", static_cast<Tag<41>::type>(2))
		.value("kZeroLengthCurve", static_cast<Tag<41>::type>(1));

	py::enum_<Tag<42>::type>(m, "FilerType", "PyRx 兼容枚举（值取自 PyRx 桩）")
		.value("kBagFiler", static_cast<Tag<42>::type>(3))
		.value("kCopyFiler", static_cast<Tag<42>::type>(1))
		.value("kDeepCloneFiler", static_cast<Tag<42>::type>(6))
		.value("kFileFiler", static_cast<Tag<42>::type>(0))
		.value("kIdFiler", static_cast<Tag<42>::type>(7))
		.value("kIdXlateFiler", static_cast<Tag<42>::type>(4))
		.value("kPageFiler", static_cast<Tag<42>::type>(5))
		.value("kPurgeFiler", static_cast<Tag<42>::type>(8))
		.value("kUndoFiler", static_cast<Tag<42>::type>(2))
		.value("kWblockCloneFiler", static_cast<Tag<42>::type>(9));

	py::enum_<Tag<43>::type>(m, "FindFileHint", "PyRx 兼容枚举（值取自 PyRx 桩）")
		.value("kARXApplication", static_cast<Tag<43>::type>(7))
		.value("kCloudOrProjectFile", static_cast<Tag<43>::type>(13))
		.value("kCompiledShapeFile", static_cast<Tag<43>::type>(2))
		.value("kDataLinkFile", static_cast<Tag<43>::type>(10))
		.value("kDefault", static_cast<Tag<43>::type>(0))
		.value("kEmbeddedImageFile", static_cast<Tag<43>::type>(4))
		.value("kFontFile", static_cast<Tag<43>::type>(1))
		.value("kFontMapFile", static_cast<Tag<43>::type>(8))
		.value("kMaterialMapFile", static_cast<Tag<43>::type>(12))
		.value("kPatternFile", static_cast<Tag<43>::type>(6))
		.value("kPhotometricWebFile", static_cast<Tag<43>::type>(11))
		.value("kTrueTypeFontFile", static_cast<Tag<43>::type>(3))
		.value("kUnderlayFile", static_cast<Tag<43>::type>(9))
		.value("kXRefDrawing", static_cast<Tag<43>::type>(5));

	py::enum_<Tag<44>::type>(m, "GeoCoordinateSystemProjectionCode", "PyRx 兼容枚举（值取自 PyRx 桩）")
		.value("kProjectionCodeAlber", static_cast<Tag<44>::type>(4))
		.value("kProjectionCodeAzede", static_cast<Tag<44>::type>(59))
		.value("kProjectionCodeAzmea", static_cast<Tag<44>::type>(11))
		.value("kProjectionCodeAzmed", static_cast<Tag<44>::type>(7))
		.value("kProjectionCodeBipolar", static_cast<Tag<44>::type>(31))
		.value("kProjectionCodeBonne", static_cast<Tag<44>::type>(24))
		.value("kProjectionCodeCassini", static_cast<Tag<44>::type>(22))
		.value("kProjectionCodeEckert4", static_cast<Tag<44>::type>(25))
		.value("kProjectionCodeEckert6", static_cast<Tag<44>::type>(26))
		.value("kProjectionCodeEdcnc", static_cast<Tag<44>::type>(12))
		.value("kProjectionCodeEdcyl", static_cast<Tag<44>::type>(20))
		.value("kProjectionCodeEdcylE", static_cast<Tag<44>::type>(67))
		.value("kProjectionCodeGaussK", static_cast<Tag<44>::type>(46))
		.value("kProjectionCodeGnomonic", static_cast<Tag<44>::type>(19))
		.value("kProjectionCodeGoode", static_cast<Tag<44>::type>(28))
		.value("kProjectionCodeHom1uv", static_cast<Tag<44>::type>(1281))
		.value("kProjectionCodeHom1xy", static_cast<Tag<44>::type>(1282))
		.value("kProjectionCodeHom2uv", static_cast<Tag<44>::type>(1283))
		.value("kProjectionCodeHom2xy", static_cast<Tag<44>::type>(1284))
		.value("kProjectionCodeKrovak", static_cast<Tag<44>::type>(47))
		.value("kProjectionCodeKrvk95", static_cast<Tag<44>::type>(51))
		.value("kProjectionCodeLL", static_cast<Tag<44>::type>(1))
		.value("kProjectionCodeLm1sp", static_cast<Tag<44>::type>(36))
		.value("kProjectionCodeLm2sp", static_cast<Tag<44>::type>(37))
		.value("kProjectionCodeLmblg", static_cast<Tag<44>::type>(38))
		.value("kProjectionCodeLmbrtaf", static_cast<Tag<44>::type>(65))
		.value("kProjectionCodeLmtan", static_cast<Tag<44>::type>(8))
		.value("kProjectionCodeMiller", static_cast<Tag<44>::type>(13))
		.value("kProjectionCodeMndotl", static_cast<Tag<44>::type>(41))
		.value("kProjectionCodeMndott", static_cast<Tag<44>::type>(42))
		.value("kProjectionCodeModpc", static_cast<Tag<44>::type>(10))
		.value("kProjectionCodeMollweid", static_cast<Tag<44>::type>(27))
		.value("kProjectionCodeMrcat", static_cast<Tag<44>::type>(6))
		.value("kProjectionCodeMrcatK", static_cast<Tag<44>::type>(49))
		.value("kProjectionCodeMstero", static_cast<Tag<44>::type>(15))
		.value("kProjectionCodeNeacyl", static_cast<Tag<44>::type>(29))
		.value("kProjectionCodeNerth", static_cast<Tag<44>::type>(55))
		.value("kProjectionCodeNrthsrt", static_cast<Tag<44>::type>(64))
		.value("kProjectionCodeNzealand", static_cast<Tag<44>::type>(16))
		.value("kProjectionCodeOblqM", static_cast<Tag<44>::type>(5))
		.value("kProjectionCodeObqcyl", static_cast<Tag<44>::type>(56))
		.value("kProjectionCodeOrtho", static_cast<Tag<44>::type>(18))
		.value("kProjectionCodeOstn02", static_cast<Tag<44>::type>(60))
		.value("kProjectionCodeOstn97", static_cast<Tag<44>::type>(58))
		.value("kProjectionCodeOstro", static_cast<Tag<44>::type>(34))
		.value("kProjectionCodePlateCarree", static_cast<Tag<44>::type>(68))
		.value("kProjectionCodePlycn", static_cast<Tag<44>::type>(9))
		.value("kProjectionCodePstro", static_cast<Tag<44>::type>(33))
		.value("kProjectionCodePstrosl", static_cast<Tag<44>::type>(53))
		.value("kProjectionCodePvMercator", static_cast<Tag<44>::type>(69))
		.value("kProjectionCodeRobinson", static_cast<Tag<44>::type>(23))
		.value("kProjectionCodeRskew", static_cast<Tag<44>::type>(1285))
		.value("kProjectionCodeRskewc", static_cast<Tag<44>::type>(1286))
		.value("kProjectionCodeRskewo", static_cast<Tag<44>::type>(1287))
		.value("kProjectionCodeSinus", static_cast<Tag<44>::type>(17))
		.value("kProjectionCodeSotrm", static_cast<Tag<44>::type>(43))
		.value("kProjectionCodeSstro", static_cast<Tag<44>::type>(35))
		.value("kProjectionCodeSwiss", static_cast<Tag<44>::type>(32))
		.value("kProjectionCodeSys34", static_cast<Tag<44>::type>(57))
		.value("kProjectionCodeSys34_01", static_cast<Tag<44>::type>(66))
		.value("kProjectionCodeSys34_99", static_cast<Tag<44>::type>(61))
		.value("kProjectionCodeTeacyl", static_cast<Tag<44>::type>(30))
		.value("kProjectionCodeTm", static_cast<Tag<44>::type>(3))
		.value("kProjectionCodeTrmeraf", static_cast<Tag<44>::type>(54))
		.value("kProjectionCodeTrmrkrg", static_cast<Tag<44>::type>(62))
		.value("kProjectionCodeTrmrs", static_cast<Tag<44>::type>(45))
		.value("kProjectionCodeUnknown", static_cast<Tag<44>::type>(0))
		.value("kProjectionCodeUtm", static_cast<Tag<44>::type>(44))
		.value("kProjectionCodeVdgrntn", static_cast<Tag<44>::type>(21))
		.value("kProjectionCodeWccsl", static_cast<Tag<44>::type>(39))
		.value("kProjectionCodeWccst", static_cast<Tag<44>::type>(40))
		.value("kProjectionCodeWinkl", static_cast<Tag<44>::type>(63));

	py::enum_<Tag<45>::type>(m, "GeoCoordinateSystemType", "PyRx 兼容枚举（值取自 PyRx 桩）")
		.value("kTypeArbitrary", static_cast<Tag<45>::type>(1))
		.value("kTypeGeographic", static_cast<Tag<45>::type>(2))
		.value("kTypeProjected", static_cast<Tag<45>::type>(3))
		.value("kTypeUnknown", static_cast<Tag<45>::type>(0));

	py::enum_<Tag<46>::type>(m, "GeoCoordinateSystemUnit", "PyRx 兼容枚举（值取自 PyRx 桩）")
		.value("kUnitBenoitChain", static_cast<Tag<46>::type>(26))
		.value("kUnitBenoitLink", static_cast<Tag<46>::type>(30))
		.value("kUnitBrealey", static_cast<Tag<46>::type>(38))
		.value("kUnitCaGrid", static_cast<Tag<46>::type>(23))
		.value("kUnitCapeFoot", static_cast<Tag<46>::type>(37))
		.value("kUnitCentimeter", static_cast<Tag<46>::type>(7))
		.value("kUnitCentisec", static_cast<Tag<46>::type>(1010))
		.value("kUnitClarkeChain", static_cast<Tag<46>::type>(24))
		.value("kUnitClarkeFoot", static_cast<Tag<46>::type>(5))
		.value("kUnitClarkeLink", static_cast<Tag<46>::type>(28))
		.value("kUnitDecameter", static_cast<Tag<46>::type>(48))
		.value("kUnitDecimeter", static_cast<Tag<46>::type>(18))
		.value("kUnitDecisec", static_cast<Tag<46>::type>(1009))
		.value("kUnitDegree", static_cast<Tag<46>::type>(1001))
		.value("kUnitDekameter", static_cast<Tag<46>::type>(20))
		.value("kUnitFoot", static_cast<Tag<46>::type>(2))
		.value("kUnitFurlong", static_cast<Tag<46>::type>(35))
		.value("kUnitGermanMeter", static_cast<Tag<46>::type>(22))
		.value("kUnitGoldCoastFoot", static_cast<Tag<46>::type>(40))
		.value("kUnitGrad", static_cast<Tag<46>::type>(1002))
		.value("kUnitGrade", static_cast<Tag<46>::type>(1003))
		.value("kUnitGunterChain", static_cast<Tag<46>::type>(25))
		.value("kUnitGunterLink", static_cast<Tag<46>::type>(29))
		.value("kUnitHectometer", static_cast<Tag<46>::type>(21))
		.value("kUnitIFoot", static_cast<Tag<46>::type>(4))
		.value("kUnitIInch", static_cast<Tag<46>::type>(6))
		.value("kUnitIMile", static_cast<Tag<46>::type>(13))
		.value("kUnitIYard", static_cast<Tag<46>::type>(12))
		.value("kUnitInch", static_cast<Tag<46>::type>(3))
		.value("kUnitIndianFoot", static_cast<Tag<46>::type>(43))
		.value("kUnitIndianFt37", static_cast<Tag<46>::type>(44))
		.value("kUnitIndianFt62", static_cast<Tag<46>::type>(45))
		.value("kUnitIndianFt75", static_cast<Tag<46>::type>(46))
		.value("kUnitIndianYard", static_cast<Tag<46>::type>(42))
		.value("kUnitIndianYd37", static_cast<Tag<46>::type>(47))
		.value("kUnitInternationalChain", static_cast<Tag<46>::type>(49))
		.value("kUnitInternationalLink", static_cast<Tag<46>::type>(50))
		.value("kUnitKilometer", static_cast<Tag<46>::type>(8))
		.value("kUnitKnot", static_cast<Tag<46>::type>(14))
		.value("kUnitLat66", static_cast<Tag<46>::type>(16))
		.value("kUnitLat83", static_cast<Tag<46>::type>(17))
		.value("kUnitMapInfo", static_cast<Tag<46>::type>(1004))
		.value("kUnitMeter", static_cast<Tag<46>::type>(1))
		.value("kUnitMicroInch", static_cast<Tag<46>::type>(41))
		.value("kUnitMil", static_cast<Tag<46>::type>(1005))
		.value("kUnitMile", static_cast<Tag<46>::type>(11))
		.value("kUnitMillimeter", static_cast<Tag<46>::type>(19))
		.value("kUnitMillisec", static_cast<Tag<46>::type>(1011))
		.value("kUnitMinute", static_cast<Tag<46>::type>(1006))
		.value("kUnitNautM", static_cast<Tag<46>::type>(15))
		.value("kUnitPerch", static_cast<Tag<46>::type>(33))
		.value("kUnitPole", static_cast<Tag<46>::type>(34))
		.value("kUnitRadian", static_cast<Tag<46>::type>(1007))
		.value("kUnitRod", static_cast<Tag<46>::type>(32))
		.value("kUnitRood", static_cast<Tag<46>::type>(36))
		.value("kUnitSearsChain", static_cast<Tag<46>::type>(27))
		.value("kUnitSearsFoot", static_cast<Tag<46>::type>(39))
		.value("kUnitSearsLink", static_cast<Tag<46>::type>(31))
		.value("kUnitSearsYard", static_cast<Tag<46>::type>(10))
		.value("kUnitSecond", static_cast<Tag<46>::type>(1008))
		.value("kUnitUnknown", static_cast<Tag<46>::type>(0))
		.value("kUnitYard", static_cast<Tag<46>::type>(9));

	py::enum_<Tag<47>::type>(m, "GeoScaleEstimationMethod", "PyRx 兼容枚举（值取自 PyRx 桩）")
		.value("kScaleEstMethodPrismoidal", static_cast<Tag<47>::type>(4))
		.value("kScaleEstMethodReferencePoint", static_cast<Tag<47>::type>(3))
		.value("kScaleEstMethodUnity", static_cast<Tag<47>::type>(1))
		.value("kScaleEstMethodUserDefined", static_cast<Tag<47>::type>(2));

	py::enum_<Tag<48>::type>(m, "GeoTextAlignmentType", "PyRx 兼容枚举（值取自 PyRx 桩）")
		.value("kCenterAlignment", static_cast<Tag<48>::type>(1))
		.value("kLeftAlignment", static_cast<Tag<48>::type>(0))
		.value("kRightAlignment", static_cast<Tag<48>::type>(2));

	py::enum_<Tag<49>::type>(m, "GeoTypeOfCoordinates", "PyRx 兼容枚举（值取自 PyRx 桩）")
		.value("kCoordTypGeographic", static_cast<Tag<49>::type>(3))
		.value("kCoordTypGrid", static_cast<Tag<49>::type>(2))
		.value("kCoordTypLocal", static_cast<Tag<49>::type>(1))
		.value("kCoordTypUnknown", static_cast<Tag<49>::type>(0));

	py::enum_<Tag<50>::type>(m, "GraphNodeFlags", "PyRx 兼容枚举（值取自 PyRx 桩）")
		.value("kAll", static_cast<Tag<50>::type>(47))
		.value("kFirstLevel", static_cast<Tag<50>::type>(16))
		.value("kInList", static_cast<Tag<50>::type>(8))
		.value("kListAll", static_cast<Tag<50>::type>(14))
		.value("kNone", static_cast<Tag<50>::type>(0))
		.value("kOutsideRefed", static_cast<Tag<50>::type>(2))
		.value("kSelected", static_cast<Tag<50>::type>(4))
		.value("kUnresTree", static_cast<Tag<50>::type>(32))
		.value("kVisited", static_cast<Tag<50>::type>(1));

	py::enum_<Tag<51>::type>(m, "GridLineStyle", "PyRx 兼容枚举（值取自 PyRx 桩）")
		.value("kGridLineStyleDouble", static_cast<Tag<51>::type>(2))
		.value("kGridLineStyleSingle", static_cast<Tag<51>::type>(1));

	py::enum_<Tag<52>::type>(m, "GridLineType", "PyRx 兼容枚举（值取自 PyRx 桩）")
		.value("kAllGridLineTypes", static_cast<Tag<52>::type>(63))
		.value("kHorzBottom", static_cast<Tag<52>::type>(4))
		.value("kHorzGridLineTypes", static_cast<Tag<52>::type>(7))
		.value("kHorzInside", static_cast<Tag<52>::type>(2))
		.value("kHorzTop", static_cast<Tag<52>::type>(1))
		.value("kInnerGridLineTypes", static_cast<Tag<52>::type>(18))
		.value("kInvalidGridLine", static_cast<Tag<52>::type>(0))
		.value("kOuterGridLineTypes", static_cast<Tag<52>::type>(45))
		.value("kVertGridLineTypes", static_cast<Tag<52>::type>(56))
		.value("kVertInside", static_cast<Tag<52>::type>(16))
		.value("kVertLeft", static_cast<Tag<52>::type>(8))
		.value("kVertRight", static_cast<Tag<52>::type>(32));

	py::enum_<Tag<53>::type>(m, "GridProperty", "PyRx 兼容枚举（值取自 PyRx 桩）")
		.value("kGridPropAll", static_cast<Tag<53>::type>(63))
		.value("kGridPropColor", static_cast<Tag<53>::type>(8))
		.value("kGridPropDoubleLineSpacing", static_cast<Tag<53>::type>(32))
		.value("kGridPropInvalid", static_cast<Tag<53>::type>(0))
		.value("kGridPropLineStyle", static_cast<Tag<53>::type>(1))
		.value("kGridPropLineWeight", static_cast<Tag<53>::type>(2))
		.value("kGridPropLinetype", static_cast<Tag<53>::type>(4))
		.value("kGridPropVisibility", static_cast<Tag<53>::type>(16));

	py::enum_<Tag<54>::type>(m, "GripStat", "PyRx 兼容枚举（值取自 PyRx 桩）")
		.value("kDimDataToBeDeleted", static_cast<Tag<54>::type>(2))
		.value("kGripsDone", static_cast<Tag<54>::type>(0))
		.value("kGripsToBeDeleted", static_cast<Tag<54>::type>(1));

	py::enum_<Tag<55>::type>(m, "HatchEdgeType", "PyRx 兼容枚举（值取自 PyRx 桩）")
		.value("kCirArc", static_cast<Tag<55>::type>(2))
		.value("kEllArc", static_cast<Tag<55>::type>(3))
		.value("kLine", static_cast<Tag<55>::type>(1))
		.value("kSpline", static_cast<Tag<55>::type>(4));

	py::enum_<Tag<56>::type>(m, "HatchGradientPatternType", "PyRx 兼容枚举（值取自 PyRx 桩）")
		.value("kPreDefinedGradient", static_cast<Tag<56>::type>(0))
		.value("kUserDefinedGradient", static_cast<Tag<56>::type>(1));

	py::enum_<Tag<57>::type>(m, "HatchLoopType", "PyRx 兼容枚举（值取自 PyRx 桩）")
		.value("kDefault", static_cast<Tag<57>::type>(0))
		.value("kDerived", static_cast<Tag<57>::type>(4))
		.value("kDoesNotSupportScale", static_cast<Tag<57>::type>(1024))
		.value("kDuplicate", static_cast<Tag<57>::type>(256))
		.value("kExternal", static_cast<Tag<57>::type>(1))
		.value("kForceAnnoAllVisible", static_cast<Tag<57>::type>(2048))
		.value("kIsAnnotative", static_cast<Tag<57>::type>(512))
		.value("kIsAnnotativeBlock", static_cast<Tag<57>::type>(8192))
		.value("kNotClosed", static_cast<Tag<57>::type>(32))
		.value("kOrientToPaper", static_cast<Tag<57>::type>(4096))
		.value("kOutermost", static_cast<Tag<57>::type>(16))
		.value("kPolyline", static_cast<Tag<57>::type>(2))
		.value("kSelfIntersecting", static_cast<Tag<57>::type>(64))
		.value("kTextIsland", static_cast<Tag<57>::type>(128))
		.value("kTextbox", static_cast<Tag<57>::type>(8));

	py::enum_<Tag<58>::type>(m, "HatchObjectType", "PyRx 兼容枚举（值取自 PyRx 桩）")
		.value("kGradientObject", static_cast<Tag<58>::type>(1))
		.value("kHatchObject", static_cast<Tag<58>::type>(0));

	py::enum_<Tag<59>::type>(m, "HatchPatternType", "PyRx 兼容枚举（值取自 PyRx 桩）")
		.value("kCustomDefined", static_cast<Tag<59>::type>(2))
		.value("kPreDefined", static_cast<Tag<59>::type>(1))
		.value("kUserDefined", static_cast<Tag<59>::type>(0));

	py::enum_<Tag<60>::type>(m, "HatchStyle", "PyRx 兼容枚举（值取自 PyRx 桩）")
		.value("kIgnore", static_cast<Tag<60>::type>(2))
		.value("kNormal", static_cast<Tag<60>::type>(0))
		.value("kOuter", static_cast<Tag<60>::type>(1));

	py::enum_<Tag<61>::type>(m, "HelixConstrainType", "PyRx 兼容枚举（值取自 PyRx 桩）")
		.value("kHeight", static_cast<Tag<61>::type>(2))
		.value("kTurnHeight", static_cast<Tag<61>::type>(0))
		.value("kTurns", static_cast<Tag<61>::type>(1));

	py::enum_<Tag<62>::type>(m, "ImageDisplayOpt", "PyRx 兼容枚举（值取自 PyRx 桩）")
		.value("kClip", static_cast<Tag<62>::type>(4))
		.value("kShow", static_cast<Tag<62>::type>(1))
		.value("kShowUnAligned", static_cast<Tag<62>::type>(2))
		.value("kTransparent", static_cast<Tag<62>::type>(8));

	py::enum_<Tag<63>::type>(m, "ImageUnits", "PyRx 兼容枚举（值取自 PyRx 桩）")
		.value("kAngstroms", static_cast<Tag<63>::type>(11))
		.value("kAstronomical", static_cast<Tag<63>::type>(18))
		.value("kCentimeter", static_cast<Tag<63>::type>(2))
		.value("kDecimeters", static_cast<Tag<63>::type>(14))
		.value("kDekameters", static_cast<Tag<63>::type>(15))
		.value("kFoot", static_cast<Tag<63>::type>(6))
		.value("kGigameters", static_cast<Tag<63>::type>(17))
		.value("kHectometers", static_cast<Tag<63>::type>(16))
		.value("kInch", static_cast<Tag<63>::type>(5))
		.value("kKilometer", static_cast<Tag<63>::type>(4))
		.value("kLightYears", static_cast<Tag<63>::type>(19))
		.value("kMeter", static_cast<Tag<63>::type>(3))
		.value("kMicroinches", static_cast<Tag<63>::type>(9))
		.value("kMicrons", static_cast<Tag<63>::type>(13))
		.value("kMile", static_cast<Tag<63>::type>(8))
		.value("kMillimeter", static_cast<Tag<63>::type>(1))
		.value("kMils", static_cast<Tag<63>::type>(10))
		.value("kNanometers", static_cast<Tag<63>::type>(12))
		.value("kNone", static_cast<Tag<63>::type>(0))
		.value("kParsecs", static_cast<Tag<63>::type>(20))
		.value("kYard", static_cast<Tag<63>::type>(7));

	py::enum_<Tag<64>::type>(m, "LayerStateMask", "PyRx 兼容枚举（值取自 PyRx 桩）")
		.value("kAll", static_cast<Tag<64>::type>(2047))
		.value("kColor", static_cast<Tag<64>::type>(32))
		.value("kCurrentViewport", static_cast<Tag<64>::type>(512))
		.value("kDecomposition", static_cast<Tag<64>::type>(133119))
		.value("kFrozen", static_cast<Tag<64>::type>(2))
		.value("kLastRestored", static_cast<Tag<64>::type>(65536))
		.value("kLineType", static_cast<Tag<64>::type>(64))
		.value("kLineWeight", static_cast<Tag<64>::type>(128))
		.value("kLocked", static_cast<Tag<64>::type>(4))
		.value("kNewViewport", static_cast<Tag<64>::type>(16))
		.value("kNone", static_cast<Tag<64>::type>(0))
		.value("kOn", static_cast<Tag<64>::type>(1))
		.value("kPlot", static_cast<Tag<64>::type>(8))
		.value("kPlotStyle", static_cast<Tag<64>::type>(256))
		.value("kStateIsHidden", static_cast<Tag<64>::type>(32768))
		.value("kTransparency", static_cast<Tag<64>::type>(1024));

	py::enum_<Tag<65>::type>(m, "LineSpacingStyle", "PyRx 兼容枚举（值取自 PyRx 桩）")
		.value("kAtLeast", static_cast<Tag<65>::type>(1))
		.value("kExactly", static_cast<Tag<65>::type>(2));

	py::enum_<Tag<66>::type>(m, "MLeaderBlockConnectionType", "PyRx 兼容枚举（值取自 PyRx 桩）")
		.value("kConnectBase", static_cast<Tag<66>::type>(1))
		.value("kConnectExtents", static_cast<Tag<66>::type>(0));

	py::enum_<Tag<67>::type>(m, "MLeaderContentType", "PyRx 兼容枚举（值取自 PyRx 桩）")
		.value("kBlockContent", static_cast<Tag<67>::type>(1))
		.value("kMTextContent", static_cast<Tag<67>::type>(2))
		.value("kNoneContent", static_cast<Tag<67>::type>(0))
		.value("kToleranceContent", static_cast<Tag<67>::type>(3));

	py::enum_<Tag<68>::type>(m, "MLeaderGsMarkType", "PyRx 兼容枚举（值取自 PyRx 桩）")
		.value("kArrowMark", static_cast<Tag<68>::type>(1))
		.value("kBlockAttribute", static_cast<Tag<68>::type>(15005))
		.value("kBlockMark", static_cast<Tag<68>::type>(15004))
		.value("kDoglegMark", static_cast<Tag<68>::type>(10001))
		.value("kLeaderLineMark", static_cast<Tag<68>::type>(5001))
		.value("kMTextMark", static_cast<Tag<68>::type>(15001))
		.value("kMTextUnderLineMark", static_cast<Tag<68>::type>(15002))
		.value("kNone", static_cast<Tag<68>::type>(0))
		.value("kToleranceMark", static_cast<Tag<68>::type>(15003));

	py::enum_<Tag<69>::type>(m, "MLeaderLeaderDirectionType", "PyRx 兼容枚举（值取自 PyRx 桩）")
		.value("kBottomLeader", static_cast<Tag<69>::type>(4))
		.value("kLeftLeader", static_cast<Tag<69>::type>(1))
		.value("kRightLeader", static_cast<Tag<69>::type>(2))
		.value("kTopLeader", static_cast<Tag<69>::type>(3))
		.value("kUnknownLeader", static_cast<Tag<69>::type>(0));

	py::enum_<Tag<70>::type>(m, "MLeaderLeaderType", "PyRx 兼容枚举（值取自 PyRx 桩）")
		.value("kInVisibleLeader", static_cast<Tag<70>::type>(0))
		.value("kSplineLeader", static_cast<Tag<70>::type>(2))
		.value("kStraightLeader", static_cast<Tag<70>::type>(1));

	py::enum_<Tag<71>::type>(m, "MLeaderMoveType", "PyRx 兼容枚举（值取自 PyRx 桩）")
		.value("kMoveAllExceptArrowHeaderPoints", static_cast<Tag<71>::type>(1))
		.value("kMoveAllPoints", static_cast<Tag<71>::type>(0))
		.value("kMoveContentAndDoglegPoints", static_cast<Tag<71>::type>(2));

	py::enum_<Tag<72>::type>(m, "MLeaderPropertyOverrideType", "PyRx 兼容枚举（值取自 PyRx 桩）")
		.value("kArrowSize", static_cast<Tag<72>::type>(9))
		.value("kArrowSymbolId", static_cast<Tag<72>::type>(8))
		.value("kBlockColor", static_cast<Tag<72>::type>(20))
		.value("kBlockConnectionType", static_cast<Tag<72>::type>(23))
		.value("kBlockId", static_cast<Tag<72>::type>(19))
		.value("kBlockRotation", static_cast<Tag<72>::type>(22))
		.value("kBlockScale", static_cast<Tag<72>::type>(21))
		.value("kContentType", static_cast<Tag<72>::type>(10))
		.value("kDefaultMText", static_cast<Tag<72>::type>(18))
		.value("kDoglegLength", static_cast<Tag<72>::type>(7))
		.value("kEnableDogleg", static_cast<Tag<72>::type>(6))
		.value("kEnableFrameText", static_cast<Tag<72>::type>(17))
		.value("kEnableLanding", static_cast<Tag<72>::type>(4))
		.value("kExtendLeaderToText", static_cast<Tag<72>::type>(30))
		.value("kLandingGap", static_cast<Tag<72>::type>(5))
		.value("kLeaderLineColor", static_cast<Tag<72>::type>(1))
		.value("kLeaderLineType", static_cast<Tag<72>::type>(0))
		.value("kLeaderLineTypeId", static_cast<Tag<72>::type>(2))
		.value("kLeaderLineWeight", static_cast<Tag<72>::type>(3))
		.value("kScale", static_cast<Tag<72>::type>(24))
		.value("kSize", static_cast<Tag<72>::type>(31))
		.value("kTextAlignmentType", static_cast<Tag<72>::type>(14))
		.value("kTextAngleType", static_cast<Tag<72>::type>(13))
		.value("kTextAttachmentDirection", static_cast<Tag<72>::type>(27))
		.value("kTextBottomAttachmentType", static_cast<Tag<72>::type>(29))
		.value("kTextColor", static_cast<Tag<72>::type>(15))
		.value("kTextHeight", static_cast<Tag<72>::type>(16))
		.value("kTextLeftAttachmentType", static_cast<Tag<72>::type>(12))
		.value("kTextRightAttachmentType", static_cast<Tag<72>::type>(25))
		.value("kTextStyleId", static_cast<Tag<72>::type>(11))
		.value("kTextSwitchAlignmentType", static_cast<Tag<72>::type>(26))
		.value("kTextTopAttachmentType", static_cast<Tag<72>::type>(28));

	py::enum_<Tag<73>::type>(m, "MLeaderSegmentAngleType", "PyRx 兼容枚举（值取自 PyRx 桩）")
		.value("k15", static_cast<Tag<73>::type>(1))
		.value("k30", static_cast<Tag<73>::type>(2))
		.value("k45", static_cast<Tag<73>::type>(3))
		.value("k60", static_cast<Tag<73>::type>(4))
		.value("k90", static_cast<Tag<73>::type>(6))
		.value("kAny", static_cast<Tag<73>::type>(0))
		.value("kHorz", static_cast<Tag<73>::type>(12));

	py::enum_<Tag<74>::type>(m, "MLeaderTextAlignmentType", "PyRx 兼容枚举（值取自 PyRx 桩）")
		.value("kCenterAlignment", static_cast<Tag<74>::type>(1))
		.value("kLeftAlignment", static_cast<Tag<74>::type>(0))
		.value("kRightAlignment", static_cast<Tag<74>::type>(2));

	py::enum_<Tag<75>::type>(m, "MLeaderTextAngleType", "PyRx 兼容枚举（值取自 PyRx 桩）")
		.value("kAlwaysRightReadingAngle", static_cast<Tag<75>::type>(2))
		.value("kHorizontalAngle", static_cast<Tag<75>::type>(1))
		.value("kInsertAngle", static_cast<Tag<75>::type>(0));

	py::enum_<Tag<76>::type>(m, "MLeaderTextAttachmentDirection", "PyRx 兼容枚举（值取自 PyRx 桩）")
		.value("kAttachmentHorizontal", static_cast<Tag<76>::type>(0))
		.value("kAttachmentVertical", static_cast<Tag<76>::type>(1));

	py::enum_<Tag<77>::type>(m, "MLeaderTextAttachmentType", "PyRx 兼容枚举（值取自 PyRx 桩）")
		.value("kAttachmentAllLine", static_cast<Tag<77>::type>(8))
		.value("kAttachmentBottomLine", static_cast<Tag<77>::type>(5))
		.value("kAttachmentBottomOfBottom", static_cast<Tag<77>::type>(4))
		.value("kAttachmentBottomOfTop", static_cast<Tag<77>::type>(7))
		.value("kAttachmentBottomOfTopLine", static_cast<Tag<77>::type>(6))
		.value("kAttachmentCenter", static_cast<Tag<77>::type>(9))
		.value("kAttachmentLinedCenter", static_cast<Tag<77>::type>(10))
		.value("kAttachmentMiddle", static_cast<Tag<77>::type>(2))
		.value("kAttachmentMiddleOfBottom", static_cast<Tag<77>::type>(3))
		.value("kAttachmentMiddleOfTop", static_cast<Tag<77>::type>(1))
		.value("kAttachmentTopOfTop", static_cast<Tag<77>::type>(0));

	py::enum_<Tag<78>::type>(m, "MPolygonloopDir", "PyRx 兼容枚举（值取自 PyRx 桩）")
		.value("kAnnotation", static_cast<Tag<78>::type>(2))
		.value("kExterior", static_cast<Tag<78>::type>(0))
		.value("kInterior", static_cast<Tag<78>::type>(1));

	py::enum_<Tag<79>::type>(m, "MTextAttachmentPoint", "PyRx 兼容枚举（值取自 PyRx 桩）")
		.value("kBaseAlign", static_cast<Tag<79>::type>(13))
		.value("kBaseCenter", static_cast<Tag<79>::type>(11))
		.value("kBaseFit", static_cast<Tag<79>::type>(17))
		.value("kBaseLeft", static_cast<Tag<79>::type>(10))
		.value("kBaseMid", static_cast<Tag<79>::type>(21))
		.value("kBaseRight", static_cast<Tag<79>::type>(12))
		.value("kBottomAlign", static_cast<Tag<79>::type>(14))
		.value("kBottomCenter", static_cast<Tag<79>::type>(8))
		.value("kBottomFit", static_cast<Tag<79>::type>(18))
		.value("kBottomLeft", static_cast<Tag<79>::type>(7))
		.value("kBottomMid", static_cast<Tag<79>::type>(22))
		.value("kBottomRight", static_cast<Tag<79>::type>(9))
		.value("kMiddleAlign", static_cast<Tag<79>::type>(15))
		.value("kMiddleCenter", static_cast<Tag<79>::type>(5))
		.value("kMiddleFit", static_cast<Tag<79>::type>(19))
		.value("kMiddleLeft", static_cast<Tag<79>::type>(4))
		.value("kMiddleMid", static_cast<Tag<79>::type>(23))
		.value("kMiddleRight", static_cast<Tag<79>::type>(6))
		.value("kTopAlign", static_cast<Tag<79>::type>(16))
		.value("kTopCenter", static_cast<Tag<79>::type>(2))
		.value("kTopFit", static_cast<Tag<79>::type>(20))
		.value("kTopLeft", static_cast<Tag<79>::type>(1))
		.value("kTopMid", static_cast<Tag<79>::type>(24))
		.value("kTopRight", static_cast<Tag<79>::type>(3));

	py::enum_<Tag<80>::type>(m, "MTextColumnType", "PyRx 兼容枚举（值取自 PyRx 桩）")
		.value("kDynamicColumns", static_cast<Tag<80>::type>(2))
		.value("kNoColumns", static_cast<Tag<80>::type>(0))
		.value("kStaticColumns", static_cast<Tag<80>::type>(1));

	py::enum_<Tag<81>::type>(m, "MTextFlowDirection", "PyRx 兼容枚举（值取自 PyRx 桩）")
		.value("kBtoT", static_cast<Tag<81>::type>(4))
		.value("kByStyle", static_cast<Tag<81>::type>(5))
		.value("kLtoR", static_cast<Tag<81>::type>(1))
		.value("kRtoL", static_cast<Tag<81>::type>(2))
		.value("kTtoB", static_cast<Tag<81>::type>(3));

	py::enum_<Tag<82>::type>(m, "MTextFragmentType", "PyRx 兼容枚举（值取自 PyRx 桩）")
		.value("kBigFont", static_cast<Tag<82>::type>(5))
		.value("kBold", static_cast<Tag<82>::type>(21))
		.value("kCapsHeight", static_cast<Tag<82>::type>(7))
		.value("kColor", static_cast<Tag<82>::type>(11))
		.value("kDirection", static_cast<Tag<82>::type>(2))
		.value("kEndFragmentTypes", static_cast<Tag<82>::type>(23))
		.value("kExtents", static_cast<Tag<82>::type>(6))
		.value("kFont", static_cast<Tag<82>::type>(4))
		.value("kFontname", static_cast<Tag<82>::type>(20))
		.value("kItalic", static_cast<Tag<82>::type>(22))
		.value("kLocation", static_cast<Tag<82>::type>(0))
		.value("kNormal", static_cast<Tag<82>::type>(1))
		.value("kObliqueAngle", static_cast<Tag<82>::type>(9))
		.value("kOverPoints", static_cast<Tag<82>::type>(18))
		.value("kOverlined", static_cast<Tag<82>::type>(15))
		.value("kStackBottom", static_cast<Tag<82>::type>(13))
		.value("kStackTop", static_cast<Tag<82>::type>(12))
		.value("kStrikePoints", static_cast<Tag<82>::type>(19))
		.value("kStrikethrough", static_cast<Tag<82>::type>(16))
		.value("kTextValue", static_cast<Tag<82>::type>(3))
		.value("kTrackingFactor", static_cast<Tag<82>::type>(10))
		.value("kUnderPoints", static_cast<Tag<82>::type>(17))
		.value("kUnderlined", static_cast<Tag<82>::type>(14))
		.value("kWidthFactor", static_cast<Tag<82>::type>(8));

	py::enum_<Tag<83>::type>(m, "MergeCellStyleOption", "PyRx 兼容枚举（值取自 PyRx 桩）")
		.value("kMergeCellStyleConvertDuplicatesToOverrides", static_cast<Tag<83>::type>(4))
		.value("kMergeCellStyleCopyDuplicates", static_cast<Tag<83>::type>(1))
		.value("kMergeCellStyleIgnoreNewStyles", static_cast<Tag<83>::type>(8))
		.value("kMergeCellStyleNone", static_cast<Tag<83>::type>(0))
		.value("kMergeCellStyleOverwriteDuplicates", static_cast<Tag<83>::type>(2));

	py::enum_<Tag<84>::type>(m, "MlineJustification", "PyRx 兼容枚举（值取自 PyRx 桩）")
		.value("kBottom", static_cast<Tag<84>::type>(2))
		.value("kClosed", static_cast<Tag<84>::type>(1))
		.value("kMerged", static_cast<Tag<84>::type>(2))
		.value("kOpen", static_cast<Tag<84>::type>(0))
		.value("kTop", static_cast<Tag<84>::type>(0))
		.value("kZero", static_cast<Tag<84>::type>(1));

	py::enum_<Tag<85>::type>(m, "OrthographicView", "PyRx 兼容枚举（值取自 PyRx 桩）")
		.value("kBackView", static_cast<Tag<85>::type>(4))
		.value("kBottomView", static_cast<Tag<85>::type>(2))
		.value("kFrontView", static_cast<Tag<85>::type>(3))
		.value("kLeftView", static_cast<Tag<85>::type>(5))
		.value("kNonOrthoView", static_cast<Tag<85>::type>(0))
		.value("kRightView", static_cast<Tag<85>::type>(6))
		.value("kTopView", static_cast<Tag<85>::type>(1));

	py::enum_<Tag<86>::type>(m, "OsnapMask", "PyRx 兼容枚举（值取自 PyRx 桩）")
		.value("kOsMaskAllowTan", static_cast<Tag<86>::type>(131072))
		.value("kOsMaskApint", static_cast<Tag<86>::type>(2048))
		.value("kOsMaskCen", static_cast<Tag<86>::type>(4))
		.value("kOsMaskCentroid", static_cast<Tag<86>::type>(1024))
		.value("kOsMaskDisablePerp", static_cast<Tag<86>::type>(262144))
		.value("kOsMaskEnd", static_cast<Tag<86>::type>(1))
		.value("kOsMaskImmediate", static_cast<Tag<86>::type>(65536))
		.value("kOsMaskIns", static_cast<Tag<86>::type>(64))
		.value("kOsMaskInt", static_cast<Tag<86>::type>(32))
		.value("kOsMaskMid", static_cast<Tag<86>::type>(2))
		.value("kOsMaskNear", static_cast<Tag<86>::type>(512))
		.value("kOsMaskNode", static_cast<Tag<86>::type>(8))
		.value("kOsMaskNoneOverride", static_cast<Tag<86>::type>(2097152))
		.value("kOsMaskPerp", static_cast<Tag<86>::type>(128))
		.value("kOsMaskQuad", static_cast<Tag<86>::type>(16))
		.value("kOsMaskQuick", static_cast<Tag<86>::type>(1024))
		.value("kOsMaskRelCartesian", static_cast<Tag<86>::type>(524288))
		.value("kOsMaskRelPolar", static_cast<Tag<86>::type>(1048576))
		.value("kOsMaskTan", static_cast<Tag<86>::type>(256));

	py::enum_<Tag<87>::type>(m, "OsnapMode", "PyRx 兼容枚举（值取自 PyRx 桩）")
		.value("kOsModeCen", static_cast<Tag<87>::type>(3))
		.value("kOsModeCentroid", static_cast<Tag<87>::type>(11))
		.value("kOsModeEnd", static_cast<Tag<87>::type>(1))
		.value("kOsModeIns", static_cast<Tag<87>::type>(7))
		.value("kOsModeMid", static_cast<Tag<87>::type>(2))
		.value("kOsModeNear", static_cast<Tag<87>::type>(10))
		.value("kOsModeNode", static_cast<Tag<87>::type>(4))
		.value("kOsModePerp", static_cast<Tag<87>::type>(8))
		.value("kOsModeQuad", static_cast<Tag<87>::type>(5))
		.value("kOsModeTan", static_cast<Tag<87>::type>(9));

	py::enum_<Tag<88>::type>(m, "OsnapType", "PyRx 兼容枚举（值取自 PyRx 桩）")
		.value("kOsnapApint", static_cast<Tag<88>::type>(11))
		.value("kOsnapCen", static_cast<Tag<88>::type>(3))
		.value("kOsnapCentroid", static_cast<Tag<88>::type>(14))
		.value("kOsnapEnd", static_cast<Tag<88>::type>(1))
		.value("kOsnapIns", static_cast<Tag<88>::type>(7))
		.value("kOsnapInt", static_cast<Tag<88>::type>(6))
		.value("kOsnapMid", static_cast<Tag<88>::type>(2))
		.value("kOsnapNear", static_cast<Tag<88>::type>(10))
		.value("kOsnapNode", static_cast<Tag<88>::type>(4))
		.value("kOsnapNone", static_cast<Tag<88>::type>(0))
		.value("kOsnapPerp", static_cast<Tag<88>::type>(8))
		.value("kOsnapQuad", static_cast<Tag<88>::type>(5))
		.value("kOsnapStart", static_cast<Tag<88>::type>(13))
		.value("kOsnapTan", static_cast<Tag<88>::type>(9));

	py::enum_<Tag<89>::type>(m, "Planarity", "PyRx 兼容枚举（值取自 PyRx 桩）")
		.value("kLinear", static_cast<Tag<89>::type>(2))
		.value("kNonPlanar", static_cast<Tag<89>::type>(0))
		.value("kPlanar", static_cast<Tag<89>::type>(1));

	py::enum_<Tag<90>::type>(m, "PlotPaperUnits", "PyRx 兼容枚举（值取自 PyRx 桩）")
		.value("kInches", static_cast<Tag<90>::type>(0))
		.value("kMillimeters", static_cast<Tag<90>::type>(1))
		.value("kPixels", static_cast<Tag<90>::type>(2));

	py::enum_<Tag<91>::type>(m, "PlotRotation", "PyRx 兼容枚举（值取自 PyRx 桩）")
		.value("k0degrees", static_cast<Tag<91>::type>(0))
		.value("k180degrees", static_cast<Tag<91>::type>(2))
		.value("k270degrees", static_cast<Tag<91>::type>(3))
		.value("k90degrees", static_cast<Tag<91>::type>(1));

	py::enum_<Tag<92>::type>(m, "PlotType", "PyRx 兼容枚举（值取自 PyRx 桩）")
		.value("kDisplay", static_cast<Tag<92>::type>(0))
		.value("kExtents", static_cast<Tag<92>::type>(1))
		.value("kLayout", static_cast<Tag<92>::type>(5))
		.value("kLimits", static_cast<Tag<92>::type>(2))
		.value("kView", static_cast<Tag<92>::type>(3))
		.value("kWindow", static_cast<Tag<92>::type>(4));

	py::enum_<Tag<93>::type>(m, "PointCloudCropType", "PyRx 兼容枚举（值取自 PyRx 桩）")
		.value("kCircular", static_cast<Tag<93>::type>(3))
		.value("kInvalid", static_cast<Tag<93>::type>(0))
		.value("kPolygonal", static_cast<Tag<93>::type>(2))
		.value("kRectangular", static_cast<Tag<93>::type>(1));

	py::enum_<Tag<94>::type>(m, "PointCloudDispOptionOutOfRange", "PyRx 兼容枚举（值取自 PyRx 桩）")
		.value("kHidePoints", static_cast<Tag<94>::type>(2))
		.value("kUseMinMaxColors", static_cast<Tag<94>::type>(0))
		.value("kUseRGBScanColors", static_cast<Tag<94>::type>(1));

	py::enum_<Tag<95>::type>(m, "PointCloudOSnapMode", "PyRx 兼容枚举（值取自 PyRx 桩）")
		.value("kOsModePCL", static_cast<Tag<95>::type>(6))
		.value("kOsModePCorner", static_cast<Tag<95>::type>(7))
		.value("kOsModePEdgeNea", static_cast<Tag<95>::type>(3))
		.value("kOsModePEdgePer", static_cast<Tag<95>::type>(4))
		.value("kOsModePInt", static_cast<Tag<95>::type>(5))
		.value("kOsModePNea", static_cast<Tag<95>::type>(1))
		.value("kOsModePNod", static_cast<Tag<95>::type>(0))
		.value("kOsModePPer", static_cast<Tag<95>::type>(2));

	py::enum_<Tag<96>::type>(m, "PointCloudProperty", "PyRx 兼容枚举（值取自 PyRx 桩）")
		.value("kClassification", static_cast<Tag<96>::type>(3))
		.value("kColor", static_cast<Tag<96>::type>(1))
		.value("kGeoGraphic", static_cast<Tag<96>::type>(6))
		.value("kIntensity", static_cast<Tag<96>::type>(2))
		.value("kNormal", static_cast<Tag<96>::type>(4))
		.value("kSegmentation", static_cast<Tag<96>::type>(5));

	py::enum_<Tag<97>::type>(m, "PointCloudPropertyState", "PyRx 兼容枚举（值取自 PyRx 桩）")
		.value("kAll", static_cast<Tag<97>::type>(1))
		.value("kNone", static_cast<Tag<97>::type>(-1))
		.value("kSome", static_cast<Tag<97>::type>(0));

	py::enum_<Tag<98>::type>(m, "PointCloudStylizationType", "PyRx 兼容枚举（值取自 PyRx 桩）")
		.value("kClassificationRamp", static_cast<Tag<98>::type>(6))
		.value("kHeightRamp", static_cast<Tag<98>::type>(4))
		.value("kIntensityRamp", static_cast<Tag<98>::type>(5))
		.value("kNormalRamp", static_cast<Tag<98>::type>(3))
		.value("kSingleColor", static_cast<Tag<98>::type>(2))
		.value("kTrueColor", static_cast<Tag<98>::type>(1));

	py::enum_<Tag<99>::type>(m, "Poly2dType", "PyRx 兼容枚举（值取自 PyRx 桩）")
		.value("k2dCubicSplinePoly", static_cast<Tag<99>::type>(3))
		.value("k2dFitCurvePoly", static_cast<Tag<99>::type>(1))
		.value("k2dQuadSplinePoly", static_cast<Tag<99>::type>(2))
		.value("k2dSimplePoly", static_cast<Tag<99>::type>(0));

	py::enum_<Tag<100>::type>(m, "Poly3dType", "PyRx 兼容枚举（值取自 PyRx 桩）")
		.value("k3dCubicSplinePoly", static_cast<Tag<100>::type>(2))
		.value("k3dQuadSplinePoly", static_cast<Tag<100>::type>(1))
		.value("k3dSimplePoly", static_cast<Tag<100>::type>(0));

	py::enum_<Tag<101>::type>(m, "RemapFileContext", "PyRx 兼容枚举（值取自 PyRx 桩）")
		.value("kAfterXrefResolution", static_cast<Tag<101>::type>(3))
		.value("kDrawingOpen", static_cast<Tag<101>::type>(0))
		.value("kRasterResolution", static_cast<Tag<101>::type>(2))
		.value("kXrefResolution", static_cast<Tag<101>::type>(1));

	py::enum_<Tag<102>::type>(m, "ReservedStringEnumType", "PyRx 兼容枚举（值取自 PyRx 桩）")
		.value("kByBlock", static_cast<Tag<102>::type>(1))
		.value("kByColor", static_cast<Tag<102>::type>(9))
		.value("kByLayer", static_cast<Tag<102>::type>(0))
		.value("kContinuous", static_cast<Tag<102>::type>(3))
		.value("kData", static_cast<Tag<102>::type>(19))
		.value("kDefPoints", static_cast<Tag<102>::type>(6))
		.value("kDefault", static_cast<Tag<102>::type>(7))
		.value("kGlobal", static_cast<Tag<102>::type>(11))
		.value("kHeader", static_cast<Tag<102>::type>(18))
		.value("kMissing", static_cast<Tag<102>::type>(8))
		.value("kModel", static_cast<Tag<102>::type>(10))
		.value("kNone", static_cast<Tag<102>::type>(2))
		.value("kNormal", static_cast<Tag<102>::type>(5))
		.value("kReservedStringCount", static_cast<Tag<102>::type>(25))
		.value("kStandard", static_cast<Tag<102>::type>(4))
		.value("kTitle", static_cast<Tag<102>::type>(17))
		.value("kVS2DWireframe", static_cast<Tag<102>::type>(12))
		.value("kVS3DHidden", static_cast<Tag<102>::type>(14))
		.value("kVS3DWireframe", static_cast<Tag<102>::type>(13))
		.value("kVSConceptual", static_cast<Tag<102>::type>(16))
		.value("kVSRealistic", static_cast<Tag<102>::type>(15))
		.value("kVSShaded", static_cast<Tag<102>::type>(24))
		.value("kVSShadedWithEdges", static_cast<Tag<102>::type>(23))
		.value("kVSShadesOfGray", static_cast<Tag<102>::type>(20))
		.value("kVSSketchy", static_cast<Tag<102>::type>(21))
		.value("kVSXRay", static_cast<Tag<102>::type>(22));

	py::enum_<Tag<103>::type>(m, "RotatedDimType", "PyRx 兼容枚举（值取自 PyRx 桩）")
		.value("kParallel", static_cast<Tag<103>::type>(1))
		.value("kPerpendicular", static_cast<Tag<103>::type>(2))
		.value("kUnknown", static_cast<Tag<103>::type>(0));

	py::enum_<Tag<104>::type>(m, "RotationAngle", "PyRx 兼容枚举（值取自 PyRx 桩）")
		.value("kDegrees000", static_cast<Tag<104>::type>(0))
		.value("kDegrees090", static_cast<Tag<104>::type>(1))
		.value("kDegrees180", static_cast<Tag<104>::type>(2))
		.value("kDegrees270", static_cast<Tag<104>::type>(3))
		.value("kDegreesUnknown", static_cast<Tag<104>::type>(-1));

	py::enum_<Tag<105>::type>(m, "RowType", "PyRx 兼容枚举（值取自 PyRx 桩）")
		.value("kAllRowTypes", static_cast<Tag<105>::type>(7))
		.value("kDataRow", static_cast<Tag<105>::type>(1))
		.value("kHeaderRow", static_cast<Tag<105>::type>(4))
		.value("kTitleRow", static_cast<Tag<105>::type>(2))
		.value("kUnknownRow", static_cast<Tag<105>::type>(0));

	py::enum_<Tag<106>::type>(m, "SectionGeneration", "PyRx 兼容枚举（值取自 PyRx 桩）")
		.value("kDestinationFile", static_cast<Tag<106>::type>(64))
		.value("kDestinationNewBlock", static_cast<Tag<106>::type>(16))
		.value("kDestinationReplaceBlock", static_cast<Tag<106>::type>(32))
		.value("kSourceAllObjects", static_cast<Tag<106>::type>(1))
		.value("kSourceSelectedObjects", static_cast<Tag<106>::type>(2));

	py::enum_<Tag<107>::type>(m, "SectionGeometry", "PyRx 兼容枚举（值取自 PyRx 桩）")
		.value("kBackgroundGeometry", static_cast<Tag<107>::type>(4))
		.value("kCurveTangencyLines", static_cast<Tag<107>::type>(16))
		.value("kForegroundGeometry", static_cast<Tag<107>::type>(8))
		.value("kIntersectionBoundary", static_cast<Tag<107>::type>(1))
		.value("kIntersectionFill", static_cast<Tag<107>::type>(2));

	py::enum_<Tag<108>::type>(m, "SectionHeight", "PyRx 兼容枚举（值取自 PyRx 桩）")
		.value("kHeightAboveSectionLine", static_cast<Tag<108>::type>(1))
		.value("kHeightBelowSectionLine", static_cast<Tag<108>::type>(2));

	py::enum_<Tag<109>::type>(m, "SectionState", "PyRx 兼容枚举（值取自 PyRx 桩）")
		.value("kBoundary", static_cast<Tag<109>::type>(2))
		.value("kPlane", static_cast<Tag<109>::type>(1))
		.value("kVolume", static_cast<Tag<109>::type>(4));

	py::enum_<Tag<110>::type>(m, "SectionSubItem", "PyRx 兼容枚举（值取自 PyRx 桩）")
		.value("kBackLine", static_cast<Tag<110>::type>(8))
		.value("kBackLineBottom", static_cast<Tag<110>::type>(32))
		.value("kBackLineTop", static_cast<Tag<110>::type>(16))
		.value("kNone", static_cast<Tag<110>::type>(0))
		.value("kSectionLine", static_cast<Tag<110>::type>(1))
		.value("kSectionLineBottom", static_cast<Tag<110>::type>(4))
		.value("kSectionLineTop", static_cast<Tag<110>::type>(2))
		.value("kVerticalLineBottom", static_cast<Tag<110>::type>(128))
		.value("kVerticalLineTop", static_cast<Tag<110>::type>(64));

	py::enum_<Tag<111>::type>(m, "SectionType", "PyRx 兼容枚举（值取自 PyRx 桩）")
		.value("k2dSection", static_cast<Tag<111>::type>(2))
		.value("k3dSection", static_cast<Tag<111>::type>(4))
		.value("kLiveSection", static_cast<Tag<111>::type>(1));

	py::enum_<Tag<112>::type>(m, "SegType", "PyRx 兼容枚举（值取自 PyRx 桩）")
		.value("kArc", static_cast<Tag<112>::type>(1))
		.value("kCoincident", static_cast<Tag<112>::type>(2))
		.value("kEmpty", static_cast<Tag<112>::type>(4))
		.value("kLine", static_cast<Tag<112>::type>(0))
		.value("kPoint", static_cast<Tag<112>::type>(3));

	py::enum_<Tag<113>::type>(m, "ShadePlotResLevel", "PyRx 兼容枚举（值取自 PyRx 桩）")
		.value("kAsDisplayed", static_cast<Tag<113>::type>(0))
		.value("kCustom", static_cast<Tag<113>::type>(5))
		.value("kMaximum", static_cast<Tag<113>::type>(4))
		.value("kNormal", static_cast<Tag<113>::type>(2))
		.value("kPresentation", static_cast<Tag<113>::type>(3))
		.value("kPreview", static_cast<Tag<113>::type>(1));

	py::enum_<Tag<114>::type>(m, "ShadePlotType", "PyRx 兼容枚举（值取自 PyRx 桩）")
		.value("kAsDisplayed", static_cast<Tag<114>::type>(0))
		.value("kHidden", static_cast<Tag<114>::type>(2))
		.value("kRenderPreset", static_cast<Tag<114>::type>(5))
		.value("kRendered", static_cast<Tag<114>::type>(3))
		.value("kVisualStyle", static_cast<Tag<114>::type>(4))
		.value("kWireframe", static_cast<Tag<114>::type>(1));

	py::enum_<Tag<115>::type>(m, "SplineType", "PyRx 兼容枚举（值取自 PyRx 桩）")
		.value("kControlPoints", static_cast<Tag<115>::type>(1))
		.value("kFitPoints", static_cast<Tag<115>::type>(0));

	py::enum_<Tag<116>::type>(m, "StandardScaleType", "PyRx 兼容枚举（值取自 PyRx 桩）")
		.value("k100_1", static_cast<Tag<116>::type>(18))
		.value("k10_1", static_cast<Tag<116>::type>(17))
		.value("k1_1", static_cast<Tag<116>::type>(2))
		.value("k1_10", static_cast<Tag<116>::type>(7))
		.value("k1_100", static_cast<Tag<116>::type>(13))
		.value("k1_128in_1ft", static_cast<Tag<116>::type>(19))
		.value("k1_16", static_cast<Tag<116>::type>(8))
		.value("k1_16in_1ft", static_cast<Tag<116>::type>(22))
		.value("k1_2", static_cast<Tag<116>::type>(3))
		.value("k1_20", static_cast<Tag<116>::type>(9))
		.value("k1_2in_1ft", static_cast<Tag<116>::type>(28))
		.value("k1_30", static_cast<Tag<116>::type>(10))
		.value("k1_32in_1ft", static_cast<Tag<116>::type>(21))
		.value("k1_4", static_cast<Tag<116>::type>(4))
		.value("k1_40", static_cast<Tag<116>::type>(11))
		.value("k1_4in_1ft", static_cast<Tag<116>::type>(26))
		.value("k1_5", static_cast<Tag<116>::type>(5))
		.value("k1_50", static_cast<Tag<116>::type>(12))
		.value("k1_64in_1ft", static_cast<Tag<116>::type>(20))
		.value("k1_8", static_cast<Tag<116>::type>(6))
		.value("k1_8in_1ft", static_cast<Tag<116>::type>(24))
		.value("k1and1_2in_1ft", static_cast<Tag<116>::type>(31))
		.value("k1ft_1ft", static_cast<Tag<116>::type>(34))
		.value("k1in_1ft", static_cast<Tag<116>::type>(30))
		.value("k2_1", static_cast<Tag<116>::type>(14))
		.value("k3_16in_1ft", static_cast<Tag<116>::type>(25))
		.value("k3_32in_1ft", static_cast<Tag<116>::type>(23))
		.value("k3_4in_1ft", static_cast<Tag<116>::type>(29))
		.value("k3_8in_1ft", static_cast<Tag<116>::type>(27))
		.value("k3in_1ft", static_cast<Tag<116>::type>(32))
		.value("k4_1", static_cast<Tag<116>::type>(15))
		.value("k6in_1ft", static_cast<Tag<116>::type>(33))
		.value("k8_1", static_cast<Tag<116>::type>(16))
		.value("kCustomScale", static_cast<Tag<116>::type>(1))
		.value("kScaleToFit", static_cast<Tag<116>::type>(0));

	py::enum_<Tag<117>::type>(m, "StdScaleType", "PyRx 兼容枚举（值取自 PyRx 桩）")
		.value("k1000_1", static_cast<Tag<117>::type>(33))
		.value("k100_1", static_cast<Tag<117>::type>(32))
		.value("k10_1", static_cast<Tag<117>::type>(31))
		.value("k1_1", static_cast<Tag<117>::type>(16))
		.value("k1_10", static_cast<Tag<117>::type>(21))
		.value("k1_100", static_cast<Tag<117>::type>(27))
		.value("k1_128in_1ft", static_cast<Tag<117>::type>(1))
		.value("k1_16", static_cast<Tag<117>::type>(22))
		.value("k1_16in_1ft", static_cast<Tag<117>::type>(4))
		.value("k1_2", static_cast<Tag<117>::type>(17))
		.value("k1_20", static_cast<Tag<117>::type>(23))
		.value("k1_2in_1ft", static_cast<Tag<117>::type>(10))
		.value("k1_30", static_cast<Tag<117>::type>(24))
		.value("k1_32in_1ft", static_cast<Tag<117>::type>(3))
		.value("k1_4", static_cast<Tag<117>::type>(18))
		.value("k1_40", static_cast<Tag<117>::type>(25))
		.value("k1_4in_1ft", static_cast<Tag<117>::type>(8))
		.value("k1_5", static_cast<Tag<117>::type>(19))
		.value("k1_50", static_cast<Tag<117>::type>(26))
		.value("k1_64in_1ft", static_cast<Tag<117>::type>(2))
		.value("k1_8", static_cast<Tag<117>::type>(20))
		.value("k1_8in_1ft", static_cast<Tag<117>::type>(6))
		.value("k1and1_2in_1ft", static_cast<Tag<117>::type>(34))
		.value("k1ft_1ft", static_cast<Tag<117>::type>(15))
		.value("k1in_1ft", static_cast<Tag<117>::type>(12))
		.value("k2_1", static_cast<Tag<117>::type>(28))
		.value("k3_16in_1ft", static_cast<Tag<117>::type>(7))
		.value("k3_32in_1ft", static_cast<Tag<117>::type>(5))
		.value("k3_4in_1ft", static_cast<Tag<117>::type>(11))
		.value("k3_8in_1ft", static_cast<Tag<117>::type>(9))
		.value("k3in_1ft", static_cast<Tag<117>::type>(13))
		.value("k4_1", static_cast<Tag<117>::type>(29))
		.value("k6in_1ft", static_cast<Tag<117>::type>(14))
		.value("k8_1", static_cast<Tag<117>::type>(30))
		.value("kScaleToFit", static_cast<Tag<117>::type>(0));

	py::enum_<Tag<118>::type>(m, "SubentType", "PyRx 兼容枚举（值取自 PyRx 桩）")
		.value("kAxisSubentType", static_cast<Tag<118>::type>(6))
		.value("kClassSubentType", static_cast<Tag<118>::type>(5))
		.value("kEdgeSubentType", static_cast<Tag<118>::type>(2))
		.value("kFaceSubentType", static_cast<Tag<118>::type>(1))
		.value("kMlineSubentCache", static_cast<Tag<118>::type>(4))
		.value("kNullSubentType", static_cast<Tag<118>::type>(0))
		.value("kSilhouetteSubentType", static_cast<Tag<118>::type>(7))
		.value("kVertexSubentType", static_cast<Tag<118>::type>(3));

	py::enum_<Tag<119>::type>(m, "SurfaceEdgeExtensionType", "PyRx 兼容枚举（值取自 PyRx 桩）")
		.value("kExtendEdge", static_cast<Tag<119>::type>(0))
		.value("kStretchEdge", static_cast<Tag<119>::type>(1));

	py::enum_<Tag<120>::type>(m, "SurfaceWireframeType", "PyRx 兼容枚举（值取自 PyRx 桩）")
		.value("kIsolines", static_cast<Tag<120>::type>(0))
		.value("kIsoparms", static_cast<Tag<120>::type>(1));

	py::enum_<Tag<121>::type>(m, "SweepAlignOption", "PyRx 兼容枚举（值取自 PyRx 桩）")
		.value("kAlignSweepEntityToPath", static_cast<Tag<121>::type>(1))
		.value("kNoAlignment", static_cast<Tag<121>::type>(0))
		.value("kTranslatePathToSweepEntity", static_cast<Tag<121>::type>(3))
		.value("kTranslateSweepEntityToPath", static_cast<Tag<121>::type>(2));

	py::enum_<Tag<122>::type>(m, "SweepMiterOption", "PyRx 兼容枚举（值取自 PyRx 桩）")
		.value("kBendMiter", static_cast<Tag<122>::type>(4))
		.value("kCrimpMiter", static_cast<Tag<122>::type>(3))
		.value("kDefaultMiter", static_cast<Tag<122>::type>(0))
		.value("kNewMiter", static_cast<Tag<122>::type>(2))
		.value("kOldMiter", static_cast<Tag<122>::type>(1));

	py::enum_<Tag<123>::type>(m, "TableBreakFlowDirection", "PyRx 兼容枚举（值取自 PyRx 桩）")
		.value("kTableBreakFlowDownOrUp", static_cast<Tag<123>::type>(2))
		.value("kTableBreakFlowLeft", static_cast<Tag<123>::type>(4))
		.value("kTableIteratorNone", static_cast<Tag<123>::type>(1));

	py::enum_<Tag<124>::type>(m, "TableBreakOption", "PyRx 兼容枚举（值取自 PyRx 桩）")
		.value("kTableBreakAllowManualHeights", static_cast<Tag<124>::type>(16))
		.value("kTableBreakAllowManualPositions", static_cast<Tag<124>::type>(8))
		.value("kTableBreakEnableBreaking", static_cast<Tag<124>::type>(1))
		.value("kTableBreakNone", static_cast<Tag<124>::type>(0))
		.value("kTableBreakRepeatBottomLabels", static_cast<Tag<124>::type>(4))
		.value("kTableBreakRepeatTopLabels", static_cast<Tag<124>::type>(2));

	py::enum_<Tag<125>::type>(m, "TableFlowDirection", "PyRx 兼容枚举（值取自 PyRx 桩）")
		.value("kExactly", static_cast<Tag<125>::type>(1))
		.value("kTtoB", static_cast<Tag<125>::type>(0));

	py::enum_<Tag<126>::type>(m, "TableHitItem", "PyRx 兼容枚举（值取自 PyRx 桩）")
		.value("kTableHitCell", static_cast<Tag<126>::type>(1))
		.value("kTableHitColumnIndicator", static_cast<Tag<126>::type>(4))
		.value("kTableHitGridLine", static_cast<Tag<126>::type>(8))
		.value("kTableHitNone", static_cast<Tag<126>::type>(0))
		.value("kTableHitRowIndicator", static_cast<Tag<126>::type>(2));

	py::enum_<Tag<127>::type>(m, "TableIteratorOption", "PyRx 兼容枚举（值取自 PyRx 桩）")
		.value("kTableIteratorIterateColumns", static_cast<Tag<127>::type>(4))
		.value("kTableIteratorIterateDataLinks", static_cast<Tag<127>::type>(128))
		.value("kTableIteratorIterateRows", static_cast<Tag<127>::type>(2))
		.value("kTableIteratorIterateSelection", static_cast<Tag<127>::type>(1))
		.value("kTableIteratorNone", static_cast<Tag<127>::type>(0))
		.value("kTableIteratorReverseOrder", static_cast<Tag<127>::type>(8))
		.value("kTableIteratorSkipMerged", static_cast<Tag<127>::type>(64))
		.value("kTableIteratorSkipReadOnlyContent", static_cast<Tag<127>::type>(16))
		.value("kTableIteratorSkipReadOnlyFormat", static_cast<Tag<127>::type>(32));

	py::enum_<Tag<128>::type>(m, "TextAlignment", "PyRx 兼容枚举（值取自 PyRx 桩）")
		.value("kTextAlignmentAligned", static_cast<Tag<128>::type>(3))
		.value("kTextAlignmentBottomCenter", static_cast<Tag<128>::type>(13))
		.value("kTextAlignmentBottomLeft", static_cast<Tag<128>::type>(12))
		.value("kTextAlignmentBottomRight", static_cast<Tag<128>::type>(14))
		.value("kTextAlignmentCenter", static_cast<Tag<128>::type>(1))
		.value("kTextAlignmentFit", static_cast<Tag<128>::type>(5))
		.value("kTextAlignmentLeft", static_cast<Tag<128>::type>(0))
		.value("kTextAlignmentMiddle", static_cast<Tag<128>::type>(4))
		.value("kTextAlignmentMiddleCenter", static_cast<Tag<128>::type>(10))
		.value("kTextAlignmentMiddleLeft", static_cast<Tag<128>::type>(9))
		.value("kTextAlignmentMiddleRight", static_cast<Tag<128>::type>(11))
		.value("kTextAlignmentRight", static_cast<Tag<128>::type>(2))
		.value("kTextAlignmentTopCenter", static_cast<Tag<128>::type>(7))
		.value("kTextAlignmentTopLeft", static_cast<Tag<128>::type>(6))
		.value("kTextAlignmentTopRight", static_cast<Tag<128>::type>(8));

	py::enum_<Tag<129>::type>(m, "TextHorzMode", "PyRx 兼容枚举（值取自 PyRx 桩）")
		.value("kTextAlign", static_cast<Tag<129>::type>(3))
		.value("kTextCenter", static_cast<Tag<129>::type>(1))
		.value("kTextFit", static_cast<Tag<129>::type>(5))
		.value("kTextLeft", static_cast<Tag<129>::type>(0))
		.value("kTextMid", static_cast<Tag<129>::type>(4))
		.value("kTextRight", static_cast<Tag<129>::type>(2));

	py::enum_<Tag<130>::type>(m, "TextVertMode", "PyRx 兼容枚举（值取自 PyRx 桩）")
		.value("kTextBase", static_cast<Tag<130>::type>(0))
		.value("kTextBottom", static_cast<Tag<130>::type>(1))
		.value("kTextTop", static_cast<Tag<130>::type>(3))
		.value("kTextVertMid", static_cast<Tag<130>::type>(2));

	py::enum_<Tag<131>::type>(m, "TimeZone", "PyRx 兼容枚举（值取自 PyRx 桩）")
		.value("kAbuDhabi", static_cast<Tag<131>::type>(4000))
		.value("kAdelaide", static_cast<Tag<131>::type>(9300))
		.value("kAlaska", static_cast<Tag<131>::type>(-9000))
		.value("kAlmaty", static_cast<Tag<131>::type>(6000))
		.value("kArizona", static_cast<Tag<131>::type>(-7001))
		.value("kAthens", static_cast<Tag<131>::type>(2000))
		.value("kAtlanticCanada", static_cast<Tag<131>::type>(-4000))
		.value("kAzores", static_cast<Tag<131>::type>(-1000))
		.value("kBaghdad", static_cast<Tag<131>::type>(3002))
		.value("kBangkok", static_cast<Tag<131>::type>(7000))
		.value("kBeijing", static_cast<Tag<131>::type>(8000))
		.value("kBerlin", static_cast<Tag<131>::type>(1000))
		.value("kBogota", static_cast<Tag<131>::type>(-5002))
		.value("kBrasilia", static_cast<Tag<131>::type>(-3000))
		.value("kBrisbane", static_cast<Tag<131>::type>(10002))
		.value("kBuenosAires", static_cast<Tag<131>::type>(-3001))
		.value("kCairo", static_cast<Tag<131>::type>(2002))
		.value("kCapeVerde", static_cast<Tag<131>::type>(-1001))
		.value("kCaracas", static_cast<Tag<131>::type>(-4001))
		.value("kCaucasus", static_cast<Tag<131>::type>(4001))
		.value("kCentral", static_cast<Tag<131>::type>(-6000))
		.value("kCentralAmerica", static_cast<Tag<131>::type>(-6001))
		.value("kDarwin", static_cast<Tag<131>::type>(9301))
		.value("kDhaka", static_cast<Tag<131>::type>(6001))
		.value("kEastAfrica", static_cast<Tag<131>::type>(3003))
		.value("kEastern", static_cast<Tag<131>::type>(-5000))
		.value("kEasternEurope", static_cast<Tag<131>::type>(2001))
		.value("kEkaterinburg", static_cast<Tag<131>::type>(5000))
		.value("kFiji", static_cast<Tag<131>::type>(12001))
		.value("kGMT", static_cast<Tag<131>::type>(1))
		.value("kGreenland", static_cast<Tag<131>::type>(-3002))
		.value("kGuam", static_cast<Tag<131>::type>(10001))
		.value("kHarare", static_cast<Tag<131>::type>(2003))
		.value("kHawaii", static_cast<Tag<131>::type>(-10000))
		.value("kHelsinki", static_cast<Tag<131>::type>(2004))
		.value("kHobart", static_cast<Tag<131>::type>(10003))
		.value("kIndiana", static_cast<Tag<131>::type>(-5001))
		.value("kInternationalDateLine", static_cast<Tag<131>::type>(-12000))
		.value("kIrkutsk", static_cast<Tag<131>::type>(8003))
		.value("kIslamabad", static_cast<Tag<131>::type>(5001))
		.value("kJerusalem", static_cast<Tag<131>::type>(2005))
		.value("kKabul", static_cast<Tag<131>::type>(4300))
		.value("kKathmandu", static_cast<Tag<131>::type>(5450))
		.value("kKolkata", static_cast<Tag<131>::type>(5300))
		.value("kKrasnoyarsk", static_cast<Tag<131>::type>(7001))
		.value("kMagadan", static_cast<Tag<131>::type>(11000))
		.value("kMazatlan", static_cast<Tag<131>::type>(-7002))
		.value("kMexicoCity", static_cast<Tag<131>::type>(-6002))
		.value("kMidAtlantic", static_cast<Tag<131>::type>(-2000))
		.value("kMidwayIsland", static_cast<Tag<131>::type>(-11000))
		.value("kMonrovia", static_cast<Tag<131>::type>(2))
		.value("kMoscow", static_cast<Tag<131>::type>(3000))
		.value("kMountain", static_cast<Tag<131>::type>(-7000))
		.value("kNewfoundland", static_cast<Tag<131>::type>(-3300))
		.value("kPacific", static_cast<Tag<131>::type>(-8000))
		.value("kParis", static_cast<Tag<131>::type>(1001))
		.value("kPerth", static_cast<Tag<131>::type>(8004))
		.value("kPrague", static_cast<Tag<131>::type>(1002))
		.value("kRangoon", static_cast<Tag<131>::type>(6300))
		.value("kRiyadh", static_cast<Tag<131>::type>(3001))
		.value("kSantiago", static_cast<Tag<131>::type>(-4002))
		.value("kSarajevo", static_cast<Tag<131>::type>(1003))
		.value("kSaskatchewan", static_cast<Tag<131>::type>(-6003))
		.value("kSeoul", static_cast<Tag<131>::type>(9001))
		.value("kSingapore", static_cast<Tag<131>::type>(8001))
		.value("kSriLanka", static_cast<Tag<131>::type>(6002))
		.value("kSydney", static_cast<Tag<131>::type>(10000))
		.value("kTaipei", static_cast<Tag<131>::type>(8002))
		.value("kTehran", static_cast<Tag<131>::type>(3300))
		.value("kTokyo", static_cast<Tag<131>::type>(9000))
		.value("kTonga", static_cast<Tag<131>::type>(13000))
		.value("kUTC", static_cast<Tag<131>::type>(0))
		.value("kVladivostock", static_cast<Tag<131>::type>(10004))
		.value("kWellington", static_cast<Tag<131>::type>(12000))
		.value("kWestCentralAfrica", static_cast<Tag<131>::type>(1004))
		.value("kYakutsk", static_cast<Tag<131>::type>(9002));

	py::enum_<Tag<132>::type>(m, "UserParameterType", "PyRx 兼容枚举（值取自 PyRx 桩）")
		.value("kAngle", static_cast<Tag<132>::type>(4))
		.value("kArea", static_cast<Tag<132>::type>(1))
		.value("kDistance", static_cast<Tag<132>::type>(0))
		.value("kReal", static_cast<Tag<132>::type>(3))
		.value("kString", static_cast<Tag<132>::type>(5))
		.value("kVolume", static_cast<Tag<132>::type>(2));

	py::enum_<Tag<133>::type>(m, "ValueDataType", "PyRx 兼容枚举（值取自 PyRx 桩）")
		.value("k3dPoint", static_cast<Tag<133>::type>(32))
		.value("kBuffer", static_cast<Tag<133>::type>(128))
		.value("kColor", static_cast<Tag<133>::type>(1024))
		.value("kDate", static_cast<Tag<133>::type>(8))
		.value("kDouble", static_cast<Tag<133>::type>(2))
		.value("kGeneral", static_cast<Tag<133>::type>(512))
		.value("kLong", static_cast<Tag<133>::type>(1))
		.value("kObjectId", static_cast<Tag<133>::type>(64))
		.value("kPoint", static_cast<Tag<133>::type>(16))
		.value("kResbuf", static_cast<Tag<133>::type>(256))
		.value("kString", static_cast<Tag<133>::type>(4))
		.value("kUnknown", static_cast<Tag<133>::type>(0));

	py::enum_<Tag<134>::type>(m, "ValueFormatOption", "PyRx 兼容枚举（值取自 PyRx 桩）")
		.value("kForEditing", static_cast<Tag<134>::type>(1))
		.value("kForExpression", static_cast<Tag<134>::type>(2))
		.value("kFormatOptionNone", static_cast<Tag<134>::type>(0))
		.value("kIgnoreMtextFormat", static_cast<Tag<134>::type>(8))
		.value("kUseMaximumPrecision", static_cast<Tag<134>::type>(4));

	py::enum_<Tag<135>::type>(m, "ValueParseOption", "PyRx 兼容枚举（值取自 PyRx 桩）")
		.value("kChangeDataType", static_cast<Tag<135>::type>(8))
		.value("kConvertTextToValue", static_cast<Tag<135>::type>(4))
		.value("kParseOptionNone", static_cast<Tag<135>::type>(0))
		.value("kParseTextForFieldCode", static_cast<Tag<135>::type>(16))
		.value("kPreserveMtextFormat", static_cast<Tag<135>::type>(2))
		.value("kSetDefaultFormat", static_cast<Tag<135>::type>(1));

	py::enum_<Tag<136>::type>(m, "ValueUnitType", "PyRx 兼容枚举（值取自 PyRx 桩）")
		.value("kAngle", static_cast<Tag<136>::type>(2))
		.value("kAngleNotTransformed", static_cast<Tag<136>::type>(65536))
		.value("kArea", static_cast<Tag<136>::type>(4))
		.value("kCurrency", static_cast<Tag<136>::type>(16))
		.value("kDistance", static_cast<Tag<136>::type>(1))
		.value("kPercentage", static_cast<Tag<136>::type>(32))
		.value("kUnitless", static_cast<Tag<136>::type>(0))
		.value("kVolume", static_cast<Tag<136>::type>(8));

	py::enum_<Tag<137>::type>(m, "Vertex2dType", "PyRx 兼容枚举（值取自 PyRx 桩）")
		.value("k2dCurveFitVertex", static_cast<Tag<137>::type>(3))
		.value("k2dSplineCtlVertex", static_cast<Tag<137>::type>(1))
		.value("k2dSplineFitVertex", static_cast<Tag<137>::type>(2))
		.value("k2dVertex", static_cast<Tag<137>::type>(0));

	py::enum_<Tag<138>::type>(m, "VpFreezeOps", "PyRx 兼容枚举（值取自 PyRx 桩）")
		.value("kFreeze", static_cast<Tag<138>::type>(0))
		.value("kReset", static_cast<Tag<138>::type>(2))
		.value("kThaw", static_cast<Tag<138>::type>(1));

	py::enum_<Tag<139>::type>(m, "VpShadePlotType", "PyRx 兼容枚举（值取自 PyRx 桩）")
		.value("kAsDisplayed", static_cast<Tag<139>::type>(0))
		.value("kHidden", static_cast<Tag<139>::type>(2))
		.value("kRenderPreset", static_cast<Tag<139>::type>(5))
		.value("kRendered", static_cast<Tag<139>::type>(3))
		.value("kVisualStyle", static_cast<Tag<139>::type>(4))
		.value("kWireframe", static_cast<Tag<139>::type>(1));

	py::enum_<Tag<140>::type>(m, "XRefLayerPropertyOverrideType", "PyRx 兼容枚举（值取自 PyRx 桩）")
		.value("kXrColor", static_cast<Tag<140>::type>(4))
		.value("kXrDescription", static_cast<Tag<140>::type>(10))
		.value("kXrFreeze", static_cast<Tag<140>::type>(1))
		.value("kXrLinetype", static_cast<Tag<140>::type>(5))
		.value("kXrLineweight", static_cast<Tag<140>::type>(6))
		.value("kXrLock", static_cast<Tag<140>::type>(2))
		.value("kXrNewVPFreeze", static_cast<Tag<140>::type>(9))
		.value("kXrOn", static_cast<Tag<140>::type>(0))
		.value("kXrPlot", static_cast<Tag<140>::type>(3))
		.value("kXrPlotStyle", static_cast<Tag<140>::type>(8))
		.value("kXrTransparency", static_cast<Tag<140>::type>(7));

	py::enum_<Tag<141>::type>(m, "XrefStatus", "PyRx 兼容枚举（值取自 PyRx 桩）")
		.value("kXrfFileNotFound", static_cast<Tag<141>::type>(4))
		.value("kXrfNotAnXref", static_cast<Tag<141>::type>(0))
		.value("kXrfResolved", static_cast<Tag<141>::type>(1))
		.value("kXrfUnloaded", static_cast<Tag<141>::type>(2))
		.value("kXrfUnreferenced", static_cast<Tag<141>::type>(3))
		.value("kXrfUnresolved", static_cast<Tag<141>::type>(5));
	}

	{
	auto m = py::reinterpret_borrow<py::module_>(root.attr("Ge"));
	py::enum_<Tag<142>::type>(m, "AcGeCSIConfig", "PyRx 兼容枚举（值取自 PyRx 桩）")
		.value("kXCoincident", static_cast<Tag<142>::type>(5))
		.value("kXCoincidentUnbounded", static_cast<Tag<142>::type>(6))
		.value("kXIn", static_cast<Tag<142>::type>(2))
		.value("kXOut", static_cast<Tag<142>::type>(1))
		.value("kXTanIn", static_cast<Tag<142>::type>(4))
		.value("kXTanOut", static_cast<Tag<142>::type>(3))
		.value("kXUnknown", static_cast<Tag<142>::type>(0));

	py::enum_<Tag<143>::type>(m, "AcGeError", "PyRx 兼容枚举（值取自 PyRx 桩）")
		.value("k0Arg1", static_cast<Tag<143>::type>(2))
		.value("k0Arg2", static_cast<Tag<143>::type>(3))
		.value("k0This", static_cast<Tag<143>::type>(1))
		.value("kArg1InsideThis", static_cast<Tag<143>::type>(11))
		.value("kArg1OnThis", static_cast<Tag<143>::type>(10))
		.value("kArg1TooBig", static_cast<Tag<143>::type>(9))
		.value("kEqualArg1Arg2", static_cast<Tag<143>::type>(5))
		.value("kEqualArg1Arg3", static_cast<Tag<143>::type>(6))
		.value("kEqualArg2Arg3", static_cast<Tag<143>::type>(7))
		.value("kLinearlyDependentArg1Arg2Arg3", static_cast<Tag<143>::type>(8))
		.value("kOk", static_cast<Tag<143>::type>(0))
		.value("kPerpendicularArg1Arg2", static_cast<Tag<143>::type>(4));

	py::enum_<Tag<144>::type>(m, "AcGeSSIConfig", "PyRx 兼容枚举（值取自 PyRx 桩）")
		.value("kSSICoincident", static_cast<Tag<144>::type>(3))
		.value("kSSIIn", static_cast<Tag<144>::type>(2))
		.value("kSSIOut", static_cast<Tag<144>::type>(1))
		.value("kSSIUnknown", static_cast<Tag<144>::type>(0));

	py::enum_<Tag<145>::type>(m, "AcGeSSIType", "PyRx 兼容枚举（值取自 PyRx 桩）")
		.value("kSSIAntiTangent", static_cast<Tag<145>::type>(2))
		.value("kSSITangent", static_cast<Tag<145>::type>(1))
		.value("kSSITransverse", static_cast<Tag<145>::type>(0));

	py::enum_<Tag<146>::type>(m, "AcGeXConfig", "PyRx 兼容枚举（值取自 PyRx 桩）")
		.value("kLeftLeft", static_cast<Tag<146>::type>(16))
		.value("kLeftOverlap", static_cast<Tag<146>::type>(256))
		.value("kLeftRight", static_cast<Tag<146>::type>(4))
		.value("kNotDefined", static_cast<Tag<146>::type>(1))
		.value("kOverlapEnd", static_cast<Tag<146>::type>(8192))
		.value("kOverlapLeft", static_cast<Tag<146>::type>(512))
		.value("kOverlapOverlap", static_cast<Tag<146>::type>(16384))
		.value("kOverlapRight", static_cast<Tag<146>::type>(2048))
		.value("kOverlapStart", static_cast<Tag<146>::type>(4096))
		.value("kPointLeft", static_cast<Tag<146>::type>(64))
		.value("kPointRight", static_cast<Tag<146>::type>(128))
		.value("kRightLeft", static_cast<Tag<146>::type>(8))
		.value("kRightOverlap", static_cast<Tag<146>::type>(1024))
		.value("kRightRight", static_cast<Tag<146>::type>(32))
		.value("kUnknown", static_cast<Tag<146>::type>(2));

	py::enum_<Tag<147>::type>(m, "CDTOpts", "PyRx 兼容枚举（值取自 PyRx 桩）")
		.value("KConformToEdges", static_cast<Tag<147>::type>(16))
		.value("KEraseSuperTriangle", static_cast<Tag<147>::type>(1))
		.value("KInsertEdges", static_cast<Tag<147>::type>(8))
		.value("kEraseOuterTriangles", static_cast<Tag<147>::type>(2))
		.value("kEraseOuterTrianglesAndHoles", static_cast<Tag<147>::type>(4))
		.value("kNone", static_cast<Tag<147>::type>(0));

	py::enum_<Tag<148>::type>(m, "ClipCondition", "PyRx 兼容枚举（值取自 PyRx 桩）")
		.value("kAllSegmentsInside", static_cast<Tag<148>::type>(1))
		.value("kAllSegmentsOutsideEvenWinds", static_cast<Tag<148>::type>(5))
		.value("kAllSegmentsOutsideOddWinds", static_cast<Tag<148>::type>(4))
		.value("kAllSegmentsOutsideZeroWinds", static_cast<Tag<148>::type>(3))
		.value("kInvalid", static_cast<Tag<148>::type>(0))
		.value("kSegmentsIntersect", static_cast<Tag<148>::type>(2));

	py::enum_<Tag<149>::type>(m, "EntityId", "PyRx 兼容枚举（值取自 PyRx 桩）")
		.value("Helix", static_cast<Tag<149>::type>(80))
		.value("kAugPolyline2d", static_cast<Tag<149>::type>(35))
		.value("kAugPolyline3d", static_cast<Tag<149>::type>(41))
		.value("kBezierCurve3d", static_cast<Tag<149>::type>(66))
		.value("kBoundBlock2d", static_cast<Tag<149>::type>(61))
		.value("kBoundBlock3d", static_cast<Tag<149>::type>(62))
		.value("kBoundedPlane", static_cast<Tag<149>::type>(9))
		.value("kCircArc2d", static_cast<Tag<149>::type>(10))
		.value("kCircArc3d", static_cast<Tag<149>::type>(11))
		.value("kClipBoundary2d", static_cast<Tag<149>::type>(76))
		.value("kCompositeCrv2d", static_cast<Tag<149>::type>(46))
		.value("kCompositeCrv3d", static_cast<Tag<149>::type>(47))
		.value("kCone", static_cast<Tag<149>::type>(32))
		.value("kConic2d", static_cast<Tag<149>::type>(12))
		.value("kConic3d", static_cast<Tag<149>::type>(13))
		.value("kCubicSplineCurve2d", static_cast<Tag<149>::type>(38))
		.value("kCubicSplineCurve3d", static_cast<Tag<149>::type>(44))
		.value("kCurve2d", static_cast<Tag<149>::type>(14))
		.value("kCurve3d", static_cast<Tag<149>::type>(15))
		.value("kCurveBoundedSurface", static_cast<Tag<149>::type>(53))
		.value("kCurveCurveInt2d", static_cast<Tag<149>::type>(59))
		.value("kCurveCurveInt3d", static_cast<Tag<149>::type>(60))
		.value("kCurveSampleData", static_cast<Tag<149>::type>(72))
		.value("kCurveSurfaceInt", static_cast<Tag<149>::type>(78))
		.value("kCylinder", static_cast<Tag<149>::type>(30))
		.value("kDSpline2d", static_cast<Tag<149>::type>(37))
		.value("kDSpline3d", static_cast<Tag<149>::type>(43))
		.value("kEllipArc2d", static_cast<Tag<149>::type>(16))
		.value("kEllipArc3d", static_cast<Tag<149>::type>(17))
		.value("kEllipCone", static_cast<Tag<149>::type>(73))
		.value("kEllipCylinder", static_cast<Tag<149>::type>(74))
		.value("kEntity2d", static_cast<Tag<149>::type>(0))
		.value("kEntity3d", static_cast<Tag<149>::type>(1))
		.value("kEnvelope2d", static_cast<Tag<149>::type>(52))
		.value("kExternalBoundedSurface", static_cast<Tag<149>::type>(58))
		.value("kExternalCurve2d", static_cast<Tag<149>::type>(55))
		.value("kExternalCurve3d", static_cast<Tag<149>::type>(54))
		.value("kExternalObject", static_cast<Tag<149>::type>(77))
		.value("kExternalSurface", static_cast<Tag<149>::type>(48))
		.value("kFitData3d", static_cast<Tag<149>::type>(68))
		.value("kHatch", static_cast<Tag<149>::type>(69))
		.value("kIntervalBoundBlock", static_cast<Tag<149>::type>(75))
		.value("kLine2d", static_cast<Tag<149>::type>(18))
		.value("kLine3d", static_cast<Tag<149>::type>(19))
		.value("kLineSeg2d", static_cast<Tag<149>::type>(22))
		.value("kLineSeg3d", static_cast<Tag<149>::type>(23))
		.value("kLinearEnt2d", static_cast<Tag<149>::type>(20))
		.value("kLinearEnt3d", static_cast<Tag<149>::type>(21))
		.value("kNurbCurve2d", static_cast<Tag<149>::type>(36))
		.value("kNurbCurve3d", static_cast<Tag<149>::type>(42))
		.value("kNurbSurface", static_cast<Tag<149>::type>(49))
		.value("kObject", static_cast<Tag<149>::type>(67))
		.value("kOffsetCurve2d", static_cast<Tag<149>::type>(63))
		.value("kOffsetCurve3d", static_cast<Tag<149>::type>(64))
		.value("kOffsetSurface", static_cast<Tag<149>::type>(51))
		.value("kPlanarEnt", static_cast<Tag<149>::type>(24))
		.value("kPlane", static_cast<Tag<149>::type>(25))
		.value("kPointEnt2d", static_cast<Tag<149>::type>(2))
		.value("kPointEnt3d", static_cast<Tag<149>::type>(3))
		.value("kPointOnCurve2d", static_cast<Tag<149>::type>(6))
		.value("kPointOnCurve3d", static_cast<Tag<149>::type>(7))
		.value("kPointOnSurface", static_cast<Tag<149>::type>(8))
		.value("kPolyline2d", static_cast<Tag<149>::type>(34))
		.value("kPolyline3d", static_cast<Tag<149>::type>(40))
		.value("kPolynomCurve3d", static_cast<Tag<149>::type>(65))
		.value("kPosition2d", static_cast<Tag<149>::type>(4))
		.value("kPosition3d", static_cast<Tag<149>::type>(5))
		.value("kRay2d", static_cast<Tag<149>::type>(26))
		.value("kRay3d", static_cast<Tag<149>::type>(27))
		.value("kSphere", static_cast<Tag<149>::type>(29))
		.value("kSplineEnt2d", static_cast<Tag<149>::type>(33))
		.value("kSplineEnt3d", static_cast<Tag<149>::type>(39))
		.value("kSurface", static_cast<Tag<149>::type>(28))
		.value("kSurfaceCurve2dTo3d", static_cast<Tag<149>::type>(56))
		.value("kSurfaceCurve3dTo2d", static_cast<Tag<149>::type>(57))
		.value("kSurfaceSurfaceInt", static_cast<Tag<149>::type>(79))
		.value("kTorus", static_cast<Tag<149>::type>(31))
		.value("kTrimmedCrv2d", static_cast<Tag<149>::type>(45))
		.value("kTrimmedCurve2d", static_cast<Tag<149>::type>(70))
		.value("kTrimmedCurve3d", static_cast<Tag<149>::type>(71))
		.value("kTrimmedSurface", static_cast<Tag<149>::type>(50));

	py::enum_<Tag<150>::type>(m, "ExternalEntityKind", "PyRx 兼容枚举（值取自 PyRx 桩）")
		.value("kAcisEntity", static_cast<Tag<150>::type>(0))
		.value("kExternalEntityUndefined", static_cast<Tag<150>::type>(1));

	py::enum_<Tag<151>::type>(m, "KnotParameterization", "PyRx 兼容枚举（值取自 PyRx 桩）")
		.value("kChord", static_cast<Tag<151>::type>(0))
		.value("kCustomParameterization", static_cast<Tag<151>::type>(15))
		.value("kNotDefinedKnotParam", static_cast<Tag<151>::type>(16))
		.value("kSqrtChord", static_cast<Tag<151>::type>(1))
		.value("kUniform", static_cast<Tag<151>::type>(2));

	py::enum_<Tag<152>::type>(m, "OffsetCrvExtType", "PyRx 兼容枚举（值取自 PyRx 桩）")
		.value("kChamfer", static_cast<Tag<152>::type>(1))
		.value("kExtend", static_cast<Tag<152>::type>(2))
		.value("kFillet", static_cast<Tag<152>::type>(0));

	py::enum_<Tag<153>::type>(m, "PointContainment", "PyRx 兼容枚举（值取自 PyRx 桩）")
		.value("kInside", static_cast<Tag<153>::type>(0))
		.value("kOnBoundary", static_cast<Tag<153>::type>(2))
		.value("kOutside", static_cast<Tag<153>::type>(1));
	}

	{
	auto m = py::reinterpret_borrow<py::module_>(root.attr("Ap"));
	py::enum_<Tag<154>::type>(m, "DocLockMode", "PyRx 兼容枚举（值取自 PyRx 桩）")
		.value("kAutoWrite", static_cast<Tag<154>::type>(1))
		.value("kNone", static_cast<Tag<154>::type>(0))
		.value("kNotLocked", static_cast<Tag<154>::type>(2))
		.value("kProtectedAutoWrite", static_cast<Tag<154>::type>(20))
		.value("kRead", static_cast<Tag<154>::type>(32))
		.value("kWrite", static_cast<Tag<154>::type>(4))
		.value("kXWrite", static_cast<Tag<154>::type>(64));

	py::enum_<Tag<155>::type>(m, "PaletteDockStyle", "PyRx 兼容枚举（值取自 PyRx 桩）")
		.value("kAny", static_cast<Tag<155>::type>(61440))
		.value("kBottom", static_cast<Tag<155>::type>(32768))
		.value("kLeft", static_cast<Tag<155>::type>(4096))
		.value("kNone", static_cast<Tag<155>::type>(0))
		.value("kRight", static_cast<Tag<155>::type>(16384))
		.value("kTop", static_cast<Tag<155>::type>(8192));

	py::enum_<Tag<156>::type>(m, "PaletteTitleBarLocation", "PyRx 兼容枚举（值取自 PyRx 桩）")
		.value("kLeft", static_cast<Tag<156>::type>(0))
		.value("kRight", static_cast<Tag<156>::type>(1));

	py::enum_<Tag<157>::type>(m, "PyRxTestFlags", "PyRx 兼容枚举（值取自 PyRx 桩）")
		.value("kNoOptimize", static_cast<Tag<157>::type>(1))
		.value("kPyReserved1", static_cast<Tag<157>::type>(2))
		.value("kPyReserved2", static_cast<Tag<157>::type>(4))
		.value("kPyReserved3", static_cast<Tag<157>::type>(8))
		.value("kPyReserved4", static_cast<Tag<157>::type>(16))
		.value("kPyReserved5", static_cast<Tag<157>::type>(32))
		.value("kPyReserved6", static_cast<Tag<157>::type>(64))
		.value("kPyReserved7", static_cast<Tag<157>::type>(128))
		.value("kWaitForDebug", static_cast<Tag<157>::type>(256));

	py::enum_<Tag<158>::type>(m, "SaveFormat", "PyRx 兼容枚举（值取自 PyRx 桩）")
		.value("k2000_Standard", static_cast<Tag<158>::type>(15))
		.value("k2000_Template", static_cast<Tag<158>::type>(14))
		.value("k2000_dwg", static_cast<Tag<158>::type>(12))
		.value("k2000_dxf", static_cast<Tag<158>::type>(13))
		.value("k2000_xml", static_cast<Tag<158>::type>(16))
		.value("k2004_Standard", static_cast<Tag<158>::type>(27))
		.value("k2004_Template", static_cast<Tag<158>::type>(26))
		.value("k2004_dwg", static_cast<Tag<158>::type>(24))
		.value("k2004_dxf", static_cast<Tag<158>::type>(25))
		.value("k2007_Standard", static_cast<Tag<158>::type>(39))
		.value("k2007_Template", static_cast<Tag<158>::type>(38))
		.value("k2007_dwg", static_cast<Tag<158>::type>(36))
		.value("k2007_dxf", static_cast<Tag<158>::type>(37))
		.value("k2010_Standard", static_cast<Tag<158>::type>(51))
		.value("k2010_Template", static_cast<Tag<158>::type>(50))
		.value("k2010_dwg", static_cast<Tag<158>::type>(48))
		.value("k2010_dxf", static_cast<Tag<158>::type>(49))
		.value("k2013_Standard", static_cast<Tag<158>::type>(63))
		.value("k2013_Template", static_cast<Tag<158>::type>(62))
		.value("k2013_dwg", static_cast<Tag<158>::type>(60))
		.value("k2013_dxf", static_cast<Tag<158>::type>(61))
		.value("k2018_Standard", static_cast<Tag<158>::type>(67))
		.value("k2018_Template", static_cast<Tag<158>::type>(66))
		.value("k2018_dwg", static_cast<Tag<158>::type>(64))
		.value("k2018_dxf", static_cast<Tag<158>::type>(65))
		.value("kNative", static_cast<Tag<158>::type>(64))
		.value("kNative_Template", static_cast<Tag<158>::type>(66))
		.value("kR12_dxf", static_cast<Tag<158>::type>(1))
		.value("kR13_dwg", static_cast<Tag<158>::type>(4))
		.value("kR13_dxf", static_cast<Tag<158>::type>(5))
		.value("kR14_dwg", static_cast<Tag<158>::type>(8))
		.value("kR14_dxf", static_cast<Tag<158>::type>(9))
		.value("kUnknown", static_cast<Tag<158>::type>(-1));
	}

	{
	auto m = py::reinterpret_borrow<py::module_>(root.attr("Gi"));
	py::enum_<Tag<159>::type>(m, "DefaultLightingType", "PyRx 兼容枚举（值取自 PyRx 桩）")
		.value("kBackLighting", static_cast<Tag<159>::type>(2))
		.value("kOneDistantLight", static_cast<Tag<159>::type>(0))
		.value("kTwoDistantLights", static_cast<Tag<159>::type>(1));

	py::enum_<Tag<160>::type>(m, "GiAttributesFlags", "PyRx 兼容枚举（值取自 PyRx 桩）")
		.value("kDrawableBlockDependentViewportDraw", static_cast<Tag<160>::type>(4096))
		.value("kDrawableHasAttributes", static_cast<Tag<160>::type>(32))
		.value("kDrawableIsAnEntity", static_cast<Tag<160>::type>(1))
		.value("kDrawableIsCompoundObject", static_cast<Tag<160>::type>(4))
		.value("kDrawableIsDimension", static_cast<Tag<160>::type>(133))
		.value("kDrawableIsExternalReference", static_cast<Tag<160>::type>(8192))
		.value("kDrawableIsInvisible", static_cast<Tag<160>::type>(16))
		.value("kDrawableMergeControlOff", static_cast<Tag<160>::type>(65536))
		.value("kDrawableNone", static_cast<Tag<160>::type>(0))
		.value("kDrawableNotAllowLCS", static_cast<Tag<160>::type>(32768))
		.value("kDrawableNotPlottable", static_cast<Tag<160>::type>(16384))
		.value("kDrawableRegenDraw", static_cast<Tag<160>::type>(256))
		.value("kDrawableRegenTypeDependentGeometry", static_cast<Tag<160>::type>(64))
		.value("kDrawableShadedDisplaySingleLOD", static_cast<Tag<160>::type>(1024))
		.value("kDrawableStandardDisplaySingleLOD", static_cast<Tag<160>::type>(512))
		.value("kDrawableUsesNesting", static_cast<Tag<160>::type>(2))
		.value("kDrawableViewDependentViewportDraw", static_cast<Tag<160>::type>(2048))
		.value("kDrawableViewIndependentViewportDraw", static_cast<Tag<160>::type>(8))
		.value("kThreadedWorldDrawViewportDraw", static_cast<Tag<160>::type>(131072));

	py::enum_<Tag<161>::type>(m, "GiDrawableType", "PyRx 兼容枚举（值取自 PyRx 桩）")
		.value("kAmbientLight", static_cast<Tag<161>::type>(4))
		.value("kDistantLight", static_cast<Tag<161>::type>(1))
		.value("kGeometry", static_cast<Tag<161>::type>(0))
		.value("kGradientBackground", static_cast<Tag<161>::type>(6))
		.value("kGroundPlaneBackground", static_cast<Tag<161>::type>(8))
		.value("kImageBackground", static_cast<Tag<161>::type>(7))
		.value("kImageBasedLightingBackground", static_cast<Tag<161>::type>(12))
		.value("kPointLight", static_cast<Tag<161>::type>(2))
		.value("kSkyBackground", static_cast<Tag<161>::type>(11))
		.value("kSolidBackground", static_cast<Tag<161>::type>(5))
		.value("kSpotLight", static_cast<Tag<161>::type>(3))
		.value("kViewport", static_cast<Tag<161>::type>(9))
		.value("kWebLight", static_cast<Tag<161>::type>(10));

	py::enum_<Tag<162>::type>(m, "HighlightStyle", "PyRx 兼容枚举（值取自 PyRx 桩）")
		.value("kAcGiHighlightCustom", static_cast<Tag<162>::type>(1))
		.value("kAcGiHighlightDashedAndThicken", static_cast<Tag<162>::type>(2))
		.value("kAcGiHighlightDim", static_cast<Tag<162>::type>(3))
		.value("kAcGiHighlightGlow", static_cast<Tag<162>::type>(5))
		.value("kAcGiHighlightNone", static_cast<Tag<162>::type>(0))
		.value("kAcGiHighlightThickDim", static_cast<Tag<162>::type>(4));
	}

	{
	auto m = py::reinterpret_borrow<py::module_>(root.attr("Ed"));
	py::enum_<Tag<163>::type>(m, "DrawOrderCmdType", "PyRx 兼容枚举（值取自 PyRx 桩）")
		.value("kDrawOrderAbove", static_cast<Tag<163>::type>(4))
		.value("kDrawOrderBelow", static_cast<Tag<163>::type>(3))
		.value("kDrawOrderBottom", static_cast<Tag<163>::type>(1))
		.value("kDrawOrderNone", static_cast<Tag<163>::type>(0))
		.value("kDrawOrderTop", static_cast<Tag<163>::type>(2));

	py::enum_<Tag<164>::type>(m, "PointHistory", "PyRx 兼容枚举（值取自 PyRx 桩）")
		.value("eAligned", static_cast<Tag<164>::type>(1024))
		.value("eAppFiltered", static_cast<Tag<164>::type>(2048))
		.value("eCartSnapped", static_cast<Tag<164>::type>(16))
		.value("eCoordPending", static_cast<Tag<164>::type>(458752))
		.value("eCyclingPt", static_cast<Tag<164>::type>(64))
		.value("eDidNotPick", static_cast<Tag<164>::type>(0))
		.value("eDirectDistance", static_cast<Tag<164>::type>(2097152))
		.value("eForcedPick", static_cast<Tag<164>::type>(4096))
		.value("eFromKeyboard", static_cast<Tag<164>::type>(524288))
		.value("eGizmoConstrainted", static_cast<Tag<164>::type>(4194304))
		.value("eGripped", static_cast<Tag<164>::type>(8))
		.value("eLastPt", static_cast<Tag<164>::type>(4))
		.value("eNotDigitizer", static_cast<Tag<164>::type>(2))
		.value("eNotInteractive", static_cast<Tag<164>::type>(1048576))
		.value("eOrtho", static_cast<Tag<164>::type>(32))
		.value("eOsnapped", static_cast<Tag<164>::type>(128))
		.value("ePickAborted", static_cast<Tag<164>::type>(32768))
		.value("ePickMask", static_cast<Tag<164>::type>(57344))
		.value("ePolarAngle", static_cast<Tag<164>::type>(256))
		.value("eTablet", static_cast<Tag<164>::type>(1))
		.value("eUsedOsnapBox", static_cast<Tag<164>::type>(16384))
		.value("eUsedPickBox", static_cast<Tag<164>::type>(8192))
		.value("eXPending", static_cast<Tag<164>::type>(65536))
		.value("eYPending", static_cast<Tag<164>::type>(131072))
		.value("eZPending", static_cast<Tag<164>::type>(262144));

	py::enum_<Tag<165>::type>(m, "PromptCondition", "PyRx 兼容枚举（值取自 PyRx 桩）")
		.value("eNoEmpty", static_cast<Tag<165>::type>(1))
		.value("eNoNegative", static_cast<Tag<165>::type>(2))
		.value("eNoZero", static_cast<Tag<165>::type>(1))
		.value("eNone", static_cast<Tag<165>::type>(0));
	}

	{
	auto m = py::reinterpret_borrow<py::module_>(root.attr("Rx"));
	py::enum_<Tag<166>::type>(m, "LispType", "PyRx 兼容枚举（值取自 PyRx 桩）")
		.value("kAngle", static_cast<Tag<166>::type>(5004))
		.value("kDottedPair", static_cast<Tag<166>::type>(5018))
		.value("kDouble", static_cast<Tag<166>::type>(5001))
		.value("kInt16", static_cast<Tag<166>::type>(5003))
		.value("kInt32", static_cast<Tag<166>::type>(5010))
		.value("kListBegin", static_cast<Tag<166>::type>(5016))
		.value("kListEnd", static_cast<Tag<166>::type>(5017))
		.value("kNil", static_cast<Tag<166>::type>(5019))
		.value("kNone", static_cast<Tag<166>::type>(5000))
		.value("kObjectId", static_cast<Tag<166>::type>(5006))
		.value("kOrientation", static_cast<Tag<166>::type>(5008))
		.value("kPoint2d", static_cast<Tag<166>::type>(5002))
		.value("kPoint3d", static_cast<Tag<166>::type>(5009))
		.value("kSelectionSet", static_cast<Tag<166>::type>(5007))
		.value("kT_atom", static_cast<Tag<166>::type>(5021))
		.value("kText", static_cast<Tag<166>::type>(5005))
		.value("kVector3d", static_cast<Tag<166>::type>(5008))
		.value("kVoid", static_cast<Tag<166>::type>(5014));

	py::enum_<Tag<167>::type>(m, "Ordering", "PyRx 兼容枚举（值取自 PyRx 桩）")
		.value("kEqual", static_cast<Tag<167>::type>(0))
		.value("kGreaterThan", static_cast<Tag<167>::type>(1))
		.value("kLessThan", static_cast<Tag<167>::type>(-1))
		.value("kNotOrderable", static_cast<Tag<167>::type>(2));
	}
}
