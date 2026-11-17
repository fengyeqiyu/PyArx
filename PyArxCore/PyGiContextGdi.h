#pragma once
// ===========================================================================
// PyGiContextGdi.h —— 绘制上下文 AcGiContext（acgi.h:725）
//                  + GDI 自绘对象 AcGiGdiDrawObject（acgi.h:1603）
//
// AcGiContext 的实例只能由图形系统提供（AcGiCommonDraw::context()），
// 因此这里用非持有型引用 GiContextRef 包装，不参与生命周期。
//
// AcGiGdiDrawObject 是纯虚抽象类，HDC 在 SDK 里是原生句柄；Python 侧用
// int 传（ACD 的窗口 DC 由 CAD 给出），子类实现：
//     draw(self, hdc, x, y, w, h) -> bool
//     width(self) -> int
//     height(self) -> int
//     addRef(self) / delRef(self)      # 可选：缺省空实现
// ===========================================================================
#include "stdafx.h"

class PyDbDatabase;

// 非持有型 AcGiContext 引用
class GiContextRef
{
public:
	explicit GiContextRef(AcGiContext* p = nullptr) : m_p(p) {}

	AcGiContext* get() const { return m_p; }
	bool valid() const { return m_p != nullptr; }

	bool isPsOut() const;
	bool isPlotGeneration() const;
	bool isBoundaryClipping() const;
	bool isNesting() const;
	bool supportsTrueTypeText() const;
	bool supportsOwnerDraw() const;
	bool supportsFillTrait() const;
	bool supportsFastRenderBuffer() const;
	bool supportsNativePointCloudRender() const;
	void disableFastMoveDrag() const;
	AcCmEntityColor effectiveColor() const;
	int byBlockLineWeight() const;
	unsigned long long byBlockPlotStyleNameId() const;
	unsigned long long byBlockMaterialId() const;
	PyDbDatabase* database() const;

private:
	AcGiContext* m_p;
};


// Python 自绘 GDI 对象（AcGiGdiDrawObject 的 Python 桥）
class AcGiGdiDrawObjectPy : public AcGiGdiDrawObject
{
public:
	AcGiGdiDrawObjectPy() = default;
	~AcGiGdiDrawObjectPy() override = default;

	Adesk::Boolean draw(HDC hDC, int x, int y, int w, int h) override;
	int width() const override;
	int height() const override;
	void addRef() override;
	void delRef() override;
};

void bind_PyGiContextGdi(pym& m);
