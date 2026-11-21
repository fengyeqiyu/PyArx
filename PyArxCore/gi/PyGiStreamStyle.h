#pragma once
// ===========================================================================
// PyGiStreamStyle.h —— 图形流 AcGiDrawStream（acgidrawstream.h:25）
//                    + 视觉样式 AcGiVisualStyle / AcGiVisualStyleTraits
//                      （acgivisualstyle.h:787 / 1394）
//
// AcGiDrawStream 可构造；bounds(AcDbExtents) 与 serializeIn/Out 分别依赖
// 未绑定的 Extents / IAcWriteStream，本轮不导出（用法上由 build 统一驱动）。
// ===========================================================================
#include "stdafx.h"

class GiDrawStreamHolder
{
public:
	explicit GiDrawStreamHolder(bool withOwnerPlaceholder = false);
	~GiDrawStreamHolder();

	AcGiDrawStream* stream() const { return m_p; }

	bool isValid() const;
	bool isPersistent() const;
	int drawableType() const;
	unsigned long long objectId() const;
	bool hasOwner() const;

	// AcGiDrawStream::build(AcArray, GraphicsUpdateProc)
	static bool build(const std::vector<GiDrawStreamHolder*>& streams, py::object callback);

private:
	AcGiDrawStream* m_p;
};

void bind_PyGiStreamStyle(pym& m);
