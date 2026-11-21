#include "dwg_reader/db/entity/registry.h"

#include "dwg_reader/db/entity/entities.h"

namespace dwr {

std::unique_ptr<Entity> makeEntity(int typeCode, uint64_t handle) {
    switch (typeCode) {
        case 19:  // kLine
            return std::make_unique<LineEntity>(handle, TypeCode::kLine);
        case 18:  // kCircle
            return std::make_unique<CircleEntity>(handle, TypeCode::kCircle);
        case 17:  // kArc
            return std::make_unique<ArcEntity>(handle, TypeCode::kArc);
        case 27:  // kPoint
            return std::make_unique<PointEntity>(handle, TypeCode::kPoint);
        case 35:  // kEllipse
            return std::make_unique<EllipseEntity>(handle, TypeCode::kEllipse);
        // ENDBLK / SEQEND：R13/R14 无几何段，仅公共段 + 句柄流，
        // 尾部未建模位由 Entity::rawTail 原样回放（dwg.spec: DWG_ENTITY (ENDBLK|SEQEND)）。
        // BLOCK(4) 带 block name 字段（BLOCK_NAME），留待下一轮单独实现。
        case 5:
            return std::make_unique<Entity>(handle, TypeCode::kEndblk);
        case 6:
            return std::make_unique<Entity>(handle, TypeCode::kSeqend);
        // BLOCK(4)：R13..R2000 数据段为 BLOCK_NAME(TV)，R2007+ 的 T 走 UCS-2。
        case 4:
            return std::make_unique<BlockEntity>(handle, TypeCode::kBlock);
        // INSERT(7)：ins_pt/scale/rotation/extrusion/has_attribs + 块头与 attrib 链句柄。
        case 7:
            return std::make_unique<InsertEntity>(handle, TypeCode::kInsert);
        // TEXT(1)：单行文字（R13/R14 分支已实现，R2000+ 的 dataflags 变体待补）。
        case 1:
            return std::make_unique<TextEntity>(handle, TypeCode::kText);
        // ATTRIB(2)：AcDbText 字段 + tag/field_length/flags。
        case 2:
            return std::make_unique<AttribEntity>(handle, TypeCode::kAttrib);
        // VERTEX_2D(10)：flag/point/宽线起止宽/bulge/tangent。
        case 10:
            return std::make_unique<Vertex2dEntity>(handle, TypeCode::kVertex);
        // ATTDEF(3)：AcDbText 字段 + tag/field_length/flags/prompt。
        case 3:
            return std::make_unique<AttdefEntity>(handle, TypeCode::kAttdef);
        // DICTIONARY(42)：非实体对象，BL numitems + BS cloning + RC is_hardowner + TV 表 + 句柄向量。
        case 42:
            return std::make_unique<DictionaryObject>(handle, TypeCode::kDictionary);
        // LAYER(51)：表对象，name/xref/flag0/color + plotstyle/ltype 句柄。
        case 51:
            return std::make_unique<LayerObject>(handle, TypeCode::kLayer);
        // BLOCK_HEADER(49)：表对象，数据段回放 + block_entity/first/last 句柄。
        case 49:
            return std::make_unique<BlockHeaderObject>(handle, TypeCode::kBlockHeader);
        // LWPOLYLINE(77)：AcDbPolyline，R2000 数据段按 flag 条件字段 + 点/凸度/宽度数组。
        case 77:
            return std::make_unique<LwPolylineObject>(handle, TypeCode::kLwPolyline);
        // STYLE(53)：AcDbTextStyleTableRecord —— 字段口径（name/xref/font 串）尚未定清，
        // 暂由整段保真承载，类落点保留在 TextStyleTableRecord。
        case 53:
            return std::make_unique<GenericObject>(handle, static_cast<TypeCode>(typeCode));
        // 符号表控制（AcDbSymbolTable 派生）+ 表记录（AcDbSymbolTableRecord 派生）：
        // 精简/稳定表格，先整段位串保真，保证逐字节零差异。
        case 48:
            return std::make_unique<BlockTable>(handle, static_cast<TypeCode>(typeCode));
        case 50:
            return std::make_unique<LayerTable>(handle, static_cast<TypeCode>(typeCode));
        case 52:
            return std::make_unique<TextStyleTable>(handle, static_cast<TypeCode>(typeCode));
        case 56:
            return std::make_unique<LinetypeTable>(handle, static_cast<TypeCode>(typeCode));
        case 58:
            return std::make_unique<GenericObject>(handle, static_cast<TypeCode>(typeCode));
        case 60:
            return std::make_unique<ViewTable>(handle, static_cast<TypeCode>(typeCode));
        case 62:
            return std::make_unique<UcsTable>(handle, static_cast<TypeCode>(typeCode));
        case 64:
            return std::make_unique<ViewportTable>(handle, static_cast<TypeCode>(typeCode));
        case 66:
            return std::make_unique<RegAppTable>(handle, static_cast<TypeCode>(typeCode));
        case 68:
            return std::make_unique<DimStyleTable>(handle, static_cast<TypeCode>(typeCode));
        case 57:
            return std::make_unique<LinetypeTableRecord>(handle, static_cast<TypeCode>(typeCode));
        case 61:
            return std::make_unique<ViewTableRecord>(handle, static_cast<TypeCode>(typeCode));
        case 63:
            return std::make_unique<UcsTableRecord>(handle, static_cast<TypeCode>(typeCode));
        case 65:
            return std::make_unique<ViewportTableRecord>(handle, static_cast<TypeCode>(typeCode));
        case 67:
            return std::make_unique<RegAppTableRecord>(handle, static_cast<TypeCode>(typeCode));
        case 69:
            return std::make_unique<DimStyleTableRecord>(handle, static_cast<TypeCode>(typeCode));
        default:
            return nullptr;
    }
}

}  // namespace dwr
