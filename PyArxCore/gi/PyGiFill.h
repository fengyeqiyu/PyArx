#pragma once
// ===========================================================================
// PyGiFill.h —— AcGi 填充对象族（SDK: acgi.h）。
//
//   AcGiFill            填充基类（deviation = 三角化偏差）
//   AcGiHatchPattern    图案填充（若干条 pattern line 定义）
//   AcGiGradientFill    渐变填充（角度/位移/等比/颜色/类型）
//
// 用途：AcGiSubEntityTraits::setFill(const AcGiFill*) —— 在自绘 hatch 类对象
// 时把填充样式交给图形系统。三者都是 AcRxObject 派生但不是持久对象，
// 这里用 new/delete 持有（与原 SDK 生命周期一致）。
// ===========================================================================
#include "stdafx.h"

class PyGiFill {
public:
	PyGiFill() : m_fill(new AcGiFill()) {}
	explicit PyGiFill(AcGiFill* f) : m_fill(f) {}
	virtual ~PyGiFill();

	AcGiFill* impObj() const { return m_fill; }
	virtual bool isHatchPattern() const { return false; }
	virtual bool isGradientFill() const { return false; }

	double deviation() const;
	void setDeviation(double dev);

protected:
	AcGiFill* m_fill = nullptr;
};

class PyGiHatchPattern : public PyGiFill {
public:
	// defs: [(angle, base_x, base_y, offset_x, offset_y, [dash...]) ...]
	explicit PyGiHatchPattern(const py::object& defs);
	~PyGiHatchPattern() override;

	bool isHatchPattern() const override { return true; }

	AcGiHatchPattern* impHatch() const { return static_cast<AcGiHatchPattern*>(m_fill); }
	py::list patternLines() const;
};

class PyGiGradientFill : public PyGiFill {
public:
	// colors: [(r, g, b) ...]
	PyGiGradientFill(double gradientAngle, double gradientShift, bool adjustAspect,
		const py::object& colors, int gradientType);
	~PyGiGradientFill() override;

	bool isGradientFill() const override { return true; }

	AcGiGradientFill* impGradient() const { return static_cast<AcGiGradientFill*>(m_fill); }
	double gradientAngle() const;
	double gradientShift() const;
	bool isAdjustAspect() const;
	int gradientType() const;
	py::list gradientColors() const;
};

void bind_PyGiFill(pym& m);
