// PyDxfTypeValue.h
#pragma once
#pragma pack(push, 1)

#include "stdafx.h"
#include <cstdint>
#include <vector>
#include <array>
#include <string.h>
#include <cassert>
#include <algorithm>
#include <adsdef.h>
#include "PyAdsName.h"

// ========== 类型枚举 ==========
enum class DxfValueType : int8_t {
    kUnKnown = -1,
    kNull = 0,
    kInt8,
    kInt16,
    kInt32,
    kInt64,
    kDouble,
    kString,
    kPoint2d,
    kPoint3d,
    kBinary,
    kHandle,
    kAdsName,
};

class DxfValue;
class TypedValue;
class TVList;
using TV = TypedValue;

DxfValueType getDxfValueType(int code) noexcept;

// ===================== DxfValue 类 =====================
class DxfValue {
    DxfValueType m_type;
    void* m_data;

public:
    // ---------- 默认构造（null）----------
     static const DxfValue kNull;

    // ---------- 整数工厂 ----------
    static DxfValue fromInt8(int8_t v) {
        return DxfValue(DxfValueType::kInt8, new int8_t(v));
    }
    static DxfValue fromInt16(int16_t v) {
        return DxfValue(DxfValueType::kInt16, new int16_t(v));
    }
    static DxfValue fromInt32(int32_t v) {
        return DxfValue(DxfValueType::kInt32, new int32_t(v));
    }
    static DxfValue fromInt64(int64_t v) {
        return DxfValue(DxfValueType::kInt64, new int64_t(v));
    }

    // ---------- 浮点数工厂 ----------
    static DxfValue fromDouble(double v) {
        return DxfValue(DxfValueType::kDouble, new double(v));
    }

    // ---------- 点工厂 ----------
    static DxfValue fromPoint2d(double x, double y) {
        return DxfValue(DxfValueType::kPoint2d, new std::array<double, 3>{ x, y, 0.0 });
    }
    static DxfValue fromPoint2d(const std::array<double, 2>& v) {
        return fromPoint2d(v[0], v[1]);
    }
    static DxfValue fromPoint3d(double x, double y, double z) {
        return DxfValue(DxfValueType::kPoint3d, new std::array<double, 3>{ x, y, z });
    }
    static DxfValue fromPoint3d(const std::array<double, 3>& v) {
        return DxfValue(DxfValueType::kPoint3d, new std::array<double, 3>(v));
    }

    // ---------- 字符串工厂 ----------
    static DxfValue fromString(const wchar_t* str) {
        return DxfValue(DxfValueType::kString, new AcString(str));
    }
    static DxfValue fromString(const AcString& str) {
        return DxfValue(DxfValueType::kString, new AcString(str));
    }

    // ---------- 二进制工厂 ----------
    static DxfValue fromBinary(const char* data, size_t len) {
        return DxfValue(DxfValueType::kBinary, new std::vector<char>(data, data + len));
    }
    static DxfValue fromBinary(const std::vector<char>& data) {
        return DxfValue(DxfValueType::kBinary, new std::vector<char>(data));
    }
    static DxfValue fromBinary(const ads_binary& bin) {
        auto* vec = (bin.buf && bin.clen > 0)
            ? new std::vector<char>(bin.buf, bin.buf + bin.clen)
            : new std::vector<char>();
        return DxfValue(DxfValueType::kBinary, vec);
    }

    // ---------- 其他工厂 ----------
    static DxfValue fromHandle(const std::array<uint8_t, 8>& h) {
        return DxfValue(DxfValueType::kHandle, new std::array<uint8_t, 8>(h));
    }
    static DxfValue fromAdsName(const PyAdsName& name) {
        return DxfValue(DxfValueType::kAdsName, new PyAdsName(name));
    }

    // ---------- 拷贝 / 移动 ----------
    DxfValue(const DxfValue& other)
        : m_type(other.m_type), m_data(CloneData(other.m_type, other.m_data)) {
    }

    DxfValue(DxfValue&& other) noexcept
        : m_type(other.m_type), m_data(other.m_data) {
        other.m_type = DxfValueType::kNull;
        other.m_data = nullptr;
    }

    DxfValue& operator=(const DxfValue& other) {
        if (this != &other) {
            Release();
            m_type = other.m_type;
            m_data = CloneData(other.m_type, other.m_data);
        }
        return *this;
    }

    DxfValue& operator=(DxfValue&& other) noexcept {
        if (this != &other) {
            Release();
            m_type = other.m_type;
            m_data = other.m_data;
            other.m_type = DxfValueType::kNull;
            other.m_data = nullptr;
        }
        return *this;
    }

    ~DxfValue() { Release(); }

    // ---------- 查询 ----------
    DxfValueType getType() const noexcept { return m_type; }
    bool hasValue() const noexcept { return m_type != DxfValueType::kNull && m_data != nullptr; }

    // ---------- 取值（带类型检查）----------
    double  asDouble() const { return castChecked<double>(DxfValueType::kDouble); }
    int8_t  asInt8()   const { return castChecked<int8_t >(DxfValueType::kInt8); }
    int16_t asInt16()  const { return castChecked<int16_t>(DxfValueType::kInt16); }
    int32_t asInt32()  const { return castChecked<int32_t>(DxfValueType::kInt32); }
    int64_t asInt64()  const { return castChecked<int64_t>(DxfValueType::kInt64); }

    const AcString& asString()  const { return castChecked<AcString>(DxfValueType::kString); }
    const PyAdsName& asAdsName() const { return castChecked<PyAdsName>(DxfValueType::kAdsName); }
    const std::vector<char>& asBinary()  const { return castChecked<std::vector<char>>(DxfValueType::kBinary); }
    const std::array<uint8_t, 8>& asHandle()  const { return castChecked<std::array<uint8_t, 8>>(DxfValueType::kHandle); }
    const std::array<double, 3>& asPoint3d() const { return castChecked<std::array<double, 3>>(DxfValueType::kPoint3d); }

    std::array<double, 2> asPoint2d() const {
        const auto& p = castChecked<std::array<double, 3>>(DxfValueType::kPoint2d);
        return { p[0], p[1] };
    }

    ads_binary asAdsBinary() const {
        auto& vec = asBinary();
        ads_binary bin{};
        const size_t n = std::min<size_t>(vec.size(), sizeof(bin.buf));
        bin.clen = static_cast<short>(n);
        if (n > 0) std::memcpy(bin.buf, vec.data(), n);
        return bin;
    }

    AcString toString() const;

    // 转为 Python 对象（自动根据类型转换）
    pybind11::object toPy() const;

    // 从 Python 对象构造（按组码类型推断）
    static DxfValue fromPy(const pybind11::object& v, int code);

private:
    DxfValue(DxfValueType type, void* data) : m_type(type), m_data(data) {}

    void Release() noexcept;
    static void* CloneData(DxfValueType type, const void* src);

    template<typename T>
    const T& castChecked(DxfValueType expected) const {
        assert(m_data != nullptr && m_type == expected);
        if (m_data == nullptr || m_type != expected) {
            static const T kDummy{};
            return kDummy;
        }
        return *static_cast<const T*>(m_data);
    }
};

// ===================== 过滤器 DSL =====================
class TypedValue {
public:
    short    code;
    DxfValue value;

    bool checkIsValid() const;

    TypedValue(short c, const DxfValue& v) : code(c), value(v) {}
    TypedValue(short c, const AcString& s) : code(c), value(DxfValue::fromString(s)) {}
    TypedValue(short c, int8_t  v) : code(c), value(DxfValue::fromInt8(v)) {}
    TypedValue(short c, int16_t v) : code(c), value(DxfValue::fromInt16(v)) {}
    TypedValue(short c, int32_t v) : code(c), value(DxfValue::fromInt32(v)) {}
    TypedValue(short c, int64_t v) : code(c), value(DxfValue::fromInt64(v)) {}
    TypedValue(short c, double  v) : code(c), value(DxfValue::fromDouble(v)) {}
    TypedValue(short c, const std::array<double, 2>& pt) : code(c), value(DxfValue::fromPoint2d(pt)) {}
    TypedValue(short c, const std::array<double, 3>& pt) : code(c), value(DxfValue::fromPoint3d(pt)) {}
    TypedValue(short c, const PyAdsName& name) : code(c), value(DxfValue::fromAdsName(name)) {}
    TypedValue(short c, const std::vector<char>& bin) : code(c), value(DxfValue::fromBinary(bin)) {}
    TypedValue(short c, const std::array<uint8_t, 8>& handle) : code(c), value(DxfValue::fromHandle(handle)) {}
    TypedValue(short c, const ads_binary& bin) : code(c), value(DxfValue::fromBinary(bin)) {}

    AcString toString() const;
};

class TVList : public std::vector<TypedValue> {
public:
    TVList() = default;
    TVList(const std::vector<TypedValue>& data) : std::vector<TypedValue>(data) {}
    TVList(std::initializer_list<TypedValue> data) : std::vector<TypedValue>(data) {}
    TVList(const TVList&) = default;
    TVList(TVList&&) = default;
    TVList& operator=(const TVList&) = default;
    TVList& operator=(TVList&&) = default;

    AcString toString() const;
};

static inline const TypedValue AND_START{ -4, L"<AND" };
static inline const TypedValue AND_END{ -4, L"AND>" };
static inline const TypedValue OR_START{ -4, L"<OR" };
static inline const TypedValue OR_END{ -4, L"OR>" };
static inline const TypedValue NOT_START{ -4, L"<NOT" };
static inline const TypedValue NOT_END{ -4, L"NOT>" };
static inline const TypedValue SymboleEQ{ -4, L"=" };
static inline const TypedValue SymboleNE{ -4, L"!=" };
static inline const TypedValue SymboleLT{ -4, L"<" };
static inline const TypedValue SymboleLE{ -4, L"<=" };
static inline const TypedValue SymboleGT{ -4, L">" };
static inline const TypedValue SymboleGE{ -4, L">=" };

// ---------- 组码逻辑运算符 ----------
TVList operator~(const TypedValue& cond);
TVList operator~(const TVList& f);
TVList operator&(const TypedValue& lhs, const TypedValue& rhs);
TVList operator&(const TypedValue& lhs, const TVList& rhs);
TVList operator&(const TVList& lhs, const TypedValue& rhs);
TVList operator&(const TVList& lhs, const TVList& rhs);
TVList operator|(const TypedValue& lhs, const TypedValue& rhs);
TVList operator|(const TypedValue& lhs, const TVList& rhs);
TVList operator|(const TVList& lhs, const TypedValue& rhs);
TVList operator|(const TVList& lhs, const TVList& rhs);

// 替代记号版本（not/and/or 是 C++ 替代记号，等价 !/&&/||）
TVList operator not(const TypedValue& cond);
TVList operator not(const TVList& f);
TVList operator and(const TypedValue& lhs, const TypedValue& rhs);
TVList operator and(const TypedValue& lhs, const TVList& rhs);
TVList operator and(const TVList& lhs, const TypedValue& rhs);
TVList operator and(const TVList& lhs, const TVList& rhs);
TVList operator or (const TypedValue& lhs, const TypedValue& rhs);
TVList operator or (const TypedValue& lhs, const TVList& rhs);
TVList operator or (const TVList& lhs, const TypedValue& rhs);
TVList operator or (const TVList& lhs, const TVList& rhs);

// ---------- 组码比较运算符 ----------
TVList PyEq(const TypedValue& cond);
TVList PyNe(const TypedValue& cond);
TVList PyLt(const TypedValue& cond);
TVList PyLe(const TypedValue& cond);
TVList PyGt(const TypedValue& cond);
TVList PyGe(const TypedValue& cond);

TVList PyEq(short code, const DxfValue& cond);
TVList PyNe(short code, const DxfValue& cond);
TVList PyLt(short code, const DxfValue& cond);
TVList PyLe(short code, const DxfValue& cond);
TVList PyGt(short code, const DxfValue& cond);
TVList PyGe(short code, const DxfValue& cond);

#pragma pack(pop)