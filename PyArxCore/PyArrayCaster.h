#pragma once
// PyArrayCaster.h - 通用 AcArray<T> 的 pybind11 type_caster（方式一：自动值转换）。
// 约定：C++ 函数签名直接写 const AcArray<T>& / AcArray<T>，Python 侧传普通 list，
// 进入 C++ 自动拆装，返回自动变回 list；Python 不持有数组本体（值拷贝）。
// 元素 T 本身可 py::cast（double/int/AcString/已注册的几何类）即由主模板覆盖；
// AcDbObjectIdArray 按工程 oldId(uint64) 约定单独全特化。
// 非抛异常：元素类型不符时 load 返回 false，由 pybind 报参数类型错误，不崩溃。
#include "stdafx.h"

namespace pybind11 {
namespace detail {

// ---------------------------------------------------------------------------
// 主模板：元素可直接 cast<T> 的 AcArray<T>（double/int/AcString/几何类等）。
// ---------------------------------------------------------------------------
template <typename T>
struct type_caster<AcArray<T>> {
    PYBIND11_TYPE_CASTER(AcArray<T>, const_name("list"));

    // Python list/sequence -> AcArray<T>
    bool load(handle src, bool) {
        if (!isinstance<sequence>(src)) return false;
        sequence seq = reinterpret_borrow<sequence>(src);
        value = AcArray<T>();
        for (auto h : seq) {
            try {
                value.append(h.cast<T>());
            }
            catch (...) {
                return false;
            }
        }
        return true;
    }

    // AcArray<T> -> Python list
    static handle cast(const AcArray<T>& src, return_value_policy /*pol*/, handle /*parent*/) {
        list out;
        for (int i = 0; i < src.length(); ++i)
            out.append(py::cast(src[i]));
        return out.release();
    }
};

// ---------------------------------------------------------------------------
// 特化 AcDbObjectIdArray：Python 侧用 oldId(uint64) 表示对象 ID（与 ListToObjectIdArray
// / AcArrayToList 现行约定一致），无需 PyDbObjectId 包装类。
// ---------------------------------------------------------------------------
template <>
struct type_caster<AcDbObjectIdArray> {
    PYBIND11_TYPE_CASTER(AcDbObjectIdArray, const_name("list"));

    bool load(handle src, bool) {
        if (!isinstance<sequence>(src)) return false;
        sequence seq = reinterpret_borrow<sequence>(src);
        value = AcDbObjectIdArray();
        for (auto h : seq) {
            try {
                AcDbObjectId id;
                id.setFromOldId(h.cast<uint64_t>());
                value.append(id);
            }
            catch (...) {
                return false;
            }
        }
        return true;
    }

    static handle cast(const AcDbObjectIdArray& src, return_value_policy /*pol*/, handle /*parent*/) {
        list out;
        for (int i = 0; i < src.length(); ++i) {
            const AcDbObjectId& id = src[i];
            out.append(py::int_(id.isNull() ? (uint64_t)0 : (uint64_t)id.asOldId()));
        }
        return out.release();
    }
};

} // namespace detail
} // namespace pybind11

// AcGePoint3d 不提供自定义 tuple caster：它已通过 py::class_ 注册，Python 侧直接
// 传 Point3d 对象即可由内置 class_ caster 透传；这里不再叠加 sequence 构造，避免
// type_caster_base 与 PYBIND11_TYPE_CASTER 的 value 存储冲突。
