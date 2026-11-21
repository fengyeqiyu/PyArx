#pragma once
#include "stdafx.h"
#include <dbid.h>	// AcDbObjectId / AcDbHandle 完整定义（防御性；stdafx.h 经 arxHeaders.h 已间接提供）

// =============================================================================
// 原生实体标识类型 <-> Python int 的 pybind11 type_caster
// -----------------------------------------------------------------------------
// 架构决策（2026-10-22）：移除 PyDbObjectId / PyDbHandle 两个封装类及其绑定，
// 实体标识在 Python 侧统一表现为 int：
//   AcDbObjectId <-> int ：asOldId() / setFromOldId()
//   AcDbHandle   <-> int ：get64BitVal() / set64BitVal()
// 现有 Python 脚本传/收的本来就是 int oldId，因此零改动。
// 注意：type_caster 特化必须在调用 pybind 绑定（py::class_ / .def）的翻译单元
// 可见才会对绑定参数/返回值生效，故所有绑定 .cpp 均须 #include 本头。
// =============================================================================
namespace pybind11 {
namespace detail {

template <>
struct type_caster<AcDbObjectId> {
public:
	PYBIND11_TYPE_CASTER(AcDbObjectId, const_name("int"));

	// Python int（oldId）-> AcDbObjectId
	bool load(handle src, bool) {
		if (!py::isinstance<py::int_>(src)) return false;
		value.setFromOldId(static_cast<Adesk::IntDbId>(py::cast<long long>(src)));
		return true;
	}

	// AcDbObjectId -> Python int（asOldId；null 亦为 0）
	static handle cast(const AcDbObjectId& src, return_value_policy, handle) {
		return py::int_((long long)src.asOldId()).release();
	}
};

template <>
struct type_caster<AcDbHandle> {
public:
	PYBIND11_TYPE_CASTER(AcDbHandle, const_name("int"));

	// Python int（64 位数值）-> AcDbHandle
	bool load(handle src, bool) {
		if (!py::isinstance<py::int_>(src)) return false;
		value.set64BitVal((Adesk::UInt64)py::cast<unsigned long long>(src));
		return true;
	}

	// AcDbHandle -> Python int（get64BitVal）
	static handle cast(const AcDbHandle& src, return_value_policy, handle) {
		return py::int_((unsigned long long)src.get64BitVal()).release();
	}
};

}}
