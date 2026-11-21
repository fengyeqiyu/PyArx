#pragma once

#include <string>

#include "dwg_reader/db/entity/entity.h"
#include "dwg_reader/ge/point3d.h"
#include "dwg_reader/ge/vector3d.h"

namespace dwr {

class BitStream;
class BitWriter;

/// 直线（蓝本 AcDbLine / AcDbLine）。
class LineEntity : public Entity {
public:
    Point3d startPoint;
    Point3d endPoint;

    using Entity::Entity;
    void readGeom(BitStream& b, BitStream& h, BitStream* s) override;
    void writeGeom(BitWriter& w) const override;
};

/// 圆（蓝本 AcDbCircle）。
class CircleEntity : public Entity {
public:
    Point3d center;
    double radius = 0.0;
    Vector3d normal;

    using Entity::Entity;
    void readGeom(BitStream& b, BitStream& h, BitStream* s) override;
    void writeGeom(BitWriter& w) const override;
};

/// 圆弧（蓝本 AcDbArc）。
class ArcEntity : public Entity {
public:
    Point3d center;
    double radius = 0.0;
    Vector3d normal;
    double startAngle = 0.0;
    double endAngle = 0.0;

    using Entity::Entity;
    void readGeom(BitStream& b, BitStream& h, BitStream* s) override;
    void writeGeom(BitWriter& w) const override;
};

/// 块参照实体头（蓝本 AcDbBlockBegin）。R13..R2000 数据段仅 BLOCK_NAME(TV)，
/// 句柄流之后接 block header 的引用（dwg.spec: DWG_ENTITY (BLOCK) 的 DWG 分支）。
class BlockEntity : public Entity {
public:
    std::string name;

    using Entity::Entity;
    void readGeom(BitStream& b, BitStream& h, BitStream* s) override;
    void writeGeom(BitWriter& w) const override;
};

/// 块参照插入（蓝本 AcDbBlockReference / insert.spec）。
/// 数据段 R13..R14：3BD ins_pt + 3BD scale + BD rotation + 3BD extrusion + B has_attribs；
/// 句柄流公共段之后：block_header，has_attribs 时 first_attrib/last_attrib(R13..R2000) + seqend。
class InsertEntity : public Entity {
public:
    Point3d insertionPoint;
    Vector3d scale{1.0, 1.0, 1.0};
    double rotation = 0.0;
    Vector3d blockExtrusion{0.0, 0.0, 1.0};
    int hasAttribs = 0;
    HandleRef blockHeader;
    HandleRef firstAttrib;
    HandleRef lastAttrib;
    HandleRef seqend;

    using Entity::Entity;
    void readGeom(BitStream& b, BitStream& h, BitStream* s) override;
    void writeGeom(BitWriter& w) const override;
    void readExtraHandles(BitStream& h) override;
    void writeExtraHandles(BitWriter& w) const override;
};

/// 单行文字（蓝本 AcDbText / dwg.spec DWG_ENTITY (TEXT) 的 R13/R14 分支）。
/// 数据段：BD elevation + 2RD ins_pt + 2RD alignment_pt + 3BD extrusion + BD0 thickness
/// + BD0 oblique + BD0 rotation + BD height + BD width_factor + TV text_value
/// + BS generation/horiz/vert；句柄流公共段之后追加 style。
class TextEntity : public Entity {
public:
    double elevation = 0.0;
    Point3d insertionPoint;
    double alignmentX = 0.0;
    double alignmentY = 0.0;
    Vector3d textExtrusion{0.0, 0.0, 1.0};
    double thicknessValue = 0.0;
    double obliqueAngle = 0.0;
    double rotation = 0.0;
    double height = 0.0;
    double widthFactor = 1.0;
    std::string textValue;
    int generation = 0;
    int horizAlignment = 0;
    int vertAlignment = 0;
    int dataFlags = 0xFF;  // R2000+：bit=1 表示该字段取默认值（dwg.spec SINCE(R_2000b) 块）
    HandleRef style;

    using Entity::Entity;
    void readGeom(BitStream& b, BitStream& h, BitStream* s) override;
    void writeGeom(BitWriter& w) const override;
    void readExtraHandles(BitStream& h) override;
    void writeExtraHandles(BitWriter& w) const override;
};

/// 属性值（蓝本 AcDbAttribute）：字段布局 = AcDbText 全部字段 + tag/field_length/flags。
/// 句柄流：公共段之后 style（ATTRIB 的 style 在公共段之后，TEXT 出现在数据段之后同位置）。
class AttribEntity : public TextEntity {
public:
    std::string tag;
    int fieldLength = 0;
    int flags = 0;  // 1 invisible, 2 constant, 4 verify, 8 preset

    using TextEntity::TextEntity;
    void readGeom(BitStream& b, BitStream& h, BitStream* s) override;
    void writeGeom(BitWriter& w) const override;
};

/// 多段线顶点（蓝本 AcDb2dVertex / dwg.spec DWG_ENTITY (VERTEX_2D) 的 SINCE(R_13b1) 分支）。
/// 数据段：RC flag + 3BD point + BD start_width（起止宽度相等时按 libredwg 口径写负值压缩）
/// + 可选 BD end_width + BD bulge + BD tangent_dir。
class Vertex2dEntity : public Entity {
public:
    int flag = 0;
    Point3d point;
    double startWidth = 0.0;
    double endWidth = 0.0;
    double bulge = 0.0;
    double tangentDir = 0.0;

    using Entity::Entity;
    void readGeom(BitStream& b, BitStream& h, BitStream* s) override;
    void writeGeom(BitWriter& w) const override;
};

/// 属性定义（蓝本 AcDbAttributeDefinition）：AcDbText 字段（文本值位换成 default_value）
/// + tag(T)/field_length(BS)/flags(RC)/prompt(T)；句柄流沿用 style 追加。
class AttdefEntity : public TextEntity {
public:
    std::string tag;
    int fieldLength = 0;
    int flags = 0;
    std::string prompt;

    using TextEntity::TextEntity;
    void readGeom(BitStream& b, BitStream& h, BitStream* s) override;
    void writeGeom(BitWriter& w) const override;
};

/// 点（蓝本 AcDbPoint）。
class PointEntity : public Entity {
public:
    Point3d position;
    Vector3d normal;
    double ecsRotation = 0.0;

    using Entity::Entity;
    void readGeom(BitStream& b, BitStream& h, BitStream* s) override;
    void writeGeom(BitWriter& w) const override;
};

/// 字典（蓝本 AcDbDictionary，非实体对象）：数据段 num_items(BL) + cloning(BS)
/// + is_hardowner(RC) + texts(TV×num_items)；句柄流尾部追加 itemhandles。
class DictionaryObject : public Entity {
public:
    int32_t numItems = 0;
    int cloning = 0;
    int isHardOwner = 0;
    std::vector<std::string> texts;
    std::vector<HandleRef> itemHandles;

    using Entity::Entity;
    void readGeom(BitStream& b, BitStream& h, BitStream* s) override;
    void writeGeom(BitWriter& w) const override;
    void readExtraHandles(BitStream& h) override;
    void writeExtraHandles(BitWriter& w) const override;
};

/// 图层（蓝本 AcDbLayerTableRecord，表对象）：数据段 name(TV) + is_xref_ref(B)
/// + is_xref_resolved(BS) + is_xref_dep(B) + xref(H，写在数据段) + flag0(BS)
/// + color(CMC 索引)；句柄流追加 plotstyle / ltype。
class LayerObject : public Entity {
public:
    std::string name;
    int isXrefRef = 0;
    int isXrefResolved = 0;
    int isXrefDep = 0;
    HandleRef xref;
    int flag0 = 0;
    int colorIndex = 0;
    HandleRef plotstyle;
    HandleRef ltype;

    using Entity::Entity;
    void readGeom(BitStream& b, BitStream& h, BitStream* s) override;
    void writeGeom(BitWriter& w) const override;
    void readExtraHandles(BitStream& h) override;
    void writeExtraHandles(BitWriter& w) const override;
};

/// 块头（蓝本 AcDbBlockTableRecord，表对象）：数据段（name/base_pt/flags/xref/
/// num_inserts/description/preview）按 rawData 位串回放；句柄流追加
/// block_entity(code3) + first_entity(code4) + last_entity(code4)。
class BlockHeaderObject : public Entity {
public:
    HandleRef blockEntity;
    HandleRef firstEntity;
    HandleRef lastEntity;

    using Entity::Entity;
    void readGeom(BitStream& b, BitStream& h, BitStream* s) override;
    void writeGeom(BitWriter& w) const override;
    void readExtraHandles(BitStream& h) override;
    void writeExtraHandles(BitWriter& w) const override;
};

/// AcDbEllipse / ELLIPSE（35）：center(3BD) → sm_axis(3BD) → extrusion(3BD)
/// → axis_ratio(BD) → start_angle(BD) → end_angle(BD)（dwg.spec 2444-2457）。
class EllipseEntity : public Entity {
public:
    using Entity::Entity;
    Vector3d center;
    Vector3d smAxis;
    Vector3d normal{0.0, 0.0, 1.0};
    double axisRatio = 1.0;
    double startAngle = 0.0;
    double endAngle = 0.0;
    void readGeom(BitStream& b, BitStream& h, BitStream* s) override;
    void writeGeom(BitWriter& w) const override;
};

/// AcDbPolyline / LWPOLYLINE（77）：R2000 数据段口径
/// flag(BS) → 条件 const_width/elevation/thickness/extrusion → num_points(BL)
/// → 条件 num_bulges/num_widths(BL) → points（首点 2RD，其余 相对前点 DD）
/// → bulges(BD[]) → widths(BD,BD[])。
class LwPolylineObject : public Entity {
public:
    struct Pt {
        double x = 0.0;
        double y = 0.0;
    };
    using Entity::Entity;
    int flag = 0;
    double constWidth = 0.0;
    double elevation = 0.0;
    double thickness = 0.0;
    double ex = 0.0, ey = 0.0, ez = 0.0;
    int numPoints = 0;
    int numBulges = 0;
    int numWidths = 0;
    std::vector<Pt> points;
    std::vector<double> bulges;
    std::vector<Pt> widths;  // start/end
    void readGeom(BitStream& b, BitStream& h, BitStream* s) override;
    void writeGeom(BitWriter& w) const override;
};

/// ARX 口径 AcDbSymbolTableRecord 的公共字段（libredwg COMMON_TABLE_FLAGS，R13b1+）：
/// name(TV) → is_xref_ref(B)/is_xref_resolved(BS)/is_xref_dep(B) → xref(H, code5)。
class SymbolTableRecord : public Entity {
public:
    using Entity::Entity;
    std::string name;
    int flag = 0;
    int isXrefRef = 0;
    int isXrefResolved = 0;
    int isXrefDep = 0;
    uint64_t xref = 0;
    void readGeom(BitStream& b, BitStream& h, BitStream* s) override;
    void writeGeom(BitWriter& w) const override;
};

/// AcDbTextStyleTableRecord（STYLE 53/T3）。
class TextStyleTableRecord : public SymbolTableRecord {
public:
    using SymbolTableRecord::SymbolTableRecord;
    int isShape = 0;
    int isVertical = 0;
    double textSize = 0.0;
    double widthFactor = 1.0;
    double obliqueAngle = 0.0;
    int generation = 0;
    double lastHeight = 0.0;
    std::string fontFile;
    std::string bigFontFile;
    void readGeom(BitStream& b, BitStream& h, BitStream* s) override;
    void writeGeom(BitWriter& w) const override;
};

/// 通用表记录/控制对象（STYLE/LTYPE/APPID/DIMSTYLE/VIEW/UCS/VPORT 及其 *_CONTROL）：
/// 数据段与句柄流都按整段位串保真回放（语义字段尚未逐个建模），字节级对齐优先。
class GenericObject : public Entity {
public:
    using Entity::Entity;
    void readGeom(BitStream& b, BitStream& h, BitStream* s) override;
    void writeGeom(BitWriter& w) const override;
};

// ---- ARX 符号表 / 表记录类落点（AcDbSymbolTable 与 AcDbSymbolTableRecord 的派生）----
// 语义字段尚未逐一建模者先以整段保真承载；类名与 ARX acdb 模块一一对应。
class BlockTable : public GenericObject { using GenericObject::GenericObject; };            // 48
class LayerTable : public GenericObject { using GenericObject::GenericObject; };            // 50
class TextStyleTable : public GenericObject { using GenericObject::GenericObject; };        // 52
class LinetypeTable : public GenericObject { using GenericObject::GenericObject; };         // 56
class ViewTable : public GenericObject { using GenericObject::GenericObject; };             // 60 VIEW_CONTROL
class UcsTable : public GenericObject { using GenericObject::GenericObject; };              // 62 UCS_CONTROL
class ViewportTable : public GenericObject { using GenericObject::GenericObject; };         // 64 VPORT_CONTROL
class RegAppTable : public GenericObject { using GenericObject::GenericObject; };           // 66
class DimStyleTable : public GenericObject { using GenericObject::GenericObject; };         // 68
class LinetypeTableRecord : public GenericObject { using GenericObject::GenericObject; };   // 57
class ViewTableRecord : public GenericObject { using GenericObject::GenericObject; };       // 61
class UcsTableRecord : public GenericObject { using GenericObject::GenericObject; };        // 63
class ViewportTableRecord : public GenericObject { using GenericObject::GenericObject; };   // 65
class RegAppTableRecord : public GenericObject { using GenericObject::GenericObject; };     // 67
class DimStyleTableRecord : public GenericObject { using GenericObject::GenericObject; };   // 69

// ARX 命名别名：已语义建模的两类与 ARX acdb 类名对齐。
using BlockTableRecord = BlockHeaderObject;   // 49
using LayerTableRecord = LayerObject;         // 51

}  // namespace dwr
