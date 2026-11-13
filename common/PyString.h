#pragma once

#include "stdafx.h"

// ---------------------------------------------------------------------------
// pybind11 type_caster<AcString>：让 Python str 与 AcString 自动转换，
// 用法与 std::wstring / std::string 完全一致（绑定函数参数/返回值直接写
// AcString 即可，Python 侧一律是 str）。
// 转换方向：Python str(Unicode) → AcString(宽字符)；AcString → Python str。
// 归属：命令相关公共类型转换（与 PyCommandFlags.h 同置于 common）。
// 编码：UTF-8 带 BOM。
// ---------------------------------------------------------------------------
namespace pybind11 {
namespace detail {

template <>
struct type_caster<AcString> {
public:
	PYBIND11_TYPE_CASTER(AcString, const_name("str"));

	// Python str → AcString（宽字符直连：Windows 上 wchar_t 即 UTF-16，与 AcString 同宽）
	bool load(handle src, bool) {
		if (!src || !PyUnicode_Check(src.ptr())) return false;
		Py_ssize_t len = 0;
		const wchar_t* w = PyUnicode_AsWideCharString(src.ptr(), &len);
		if (!w) {
			PyErr_Clear();
			return false;
		}
		value = AcString(w, static_cast<unsigned int>(len));
		PyMem_Free(const_cast<wchar_t*>(w));
		return true;
	}

	// AcString → Python str（宽字符直连：用 AcString 自带 kwszPtr() 取 UTF-16）
	static handle cast(const AcString& src, return_value_policy, handle) {
		if (src.isEmpty()) {
			return PyUnicode_FromWideChar(L"", 0);
		}
		return PyUnicode_FromWideChar(src.kwszPtr(), (Py_ssize_t)src.length());
	}
};

}
}
