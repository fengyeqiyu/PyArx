#pragma once
// ===========================================================================
// PyGiImage.h —— AcGiImageBGRA32 / AcGiPixelBGRA32（SDK: acgi.h）。
//
// AcGiGeometry::image() 的入参：自带像素缓冲的位图（BGRA32，行优先）。
// SDK 只持裸指针 + 宽高，因此包装类自己保管像素数组（std::vector），
// Python 侧只给 numpy uint8 (H, W, 4) 数组（BGRA 顺序），构造时拷贝一份，
// 之后 Python 侧改动原数组不会影响已构造的对象。
// ===========================================================================
#include "stdafx.h"

class PyGiImageBGRA32 {
public:
	explicit PyGiImageBGRA32(const py::object& pixels);

	AcGiImageBGRA32& impObj() { return m_img; }
	const AcGiImageBGRA32& impObj() const { return m_img; }

	unsigned int width() const;
	unsigned int height() const;
	// 返回一份 BGRA uint8 ndarray 副本（(H, W, 4)）
	py::array data() const;

private:
	std::vector<AcGiPixelBGRA32> m_pixels;
	AcGiImageBGRA32              m_img;
};

void bind_PyGiImage(pym& m);
