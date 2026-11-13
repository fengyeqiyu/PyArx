#pragma once
#include "stdafx.h"
#include "AcCmColor.h"

// ===========================================================================
// PyColor —— AcCmColor 颜色值类包装（非 AcDbObject / 非 AcRxObject）
//
// AcCmColor 继承自 AcCmColorBase，是可默认构造、可拷贝的纯值类，
// 内部持有 AcCmEntityColor + 颜色名/色书名。脚本侧直接栈构造、按值传递。
// 与 PyDbColor（AcDbColor 数据库对象包装）的区别：
//   - PyDbColor 是数据库里的实体颜色记录（指针包装、需 close()）；
//   - PyColor 是一次性颜色对象，可自由构造，赋给实体前的临时值。
// ===========================================================================
class PyColor {
public:
	AcCmColor m_color;

	PyColor() {}
	PyColor(const AcCmColor& c) : m_color(c) {}

	// 工厂
	static PyColor fromRGB(int r, int g, int b);
	static PyColor fromACI(int index);
	static PyColor byLayer();
	static PyColor byBlock();

	// RGB 读写
	int  red() const;
	int  green() const;
	int  blue() const;
	void setRGB(int r, int g, int b);

	// ACI
	int  colorIndex() const;
	void setColorIndex(int index);

	// 判定
	bool isByLayer() const;
	bool isByBlock() const;
	bool isByColor() const;
	bool isNone() const;

	// 命名
	AcString colorName() const;
	AcString bookName() const;

	// 可读描述
	AcString describe() const;
};

void bind_PyColor(pym& m);
