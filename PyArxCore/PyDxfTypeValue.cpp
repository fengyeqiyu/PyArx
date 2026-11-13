// PyDxfTypeValue.cpp

#include "stdafx.h"
#include "PyDxfTypeValue.h"
#include "PyAdsName.h"


const DxfValue DxfValue::kNull(DxfValueType::kNull, nullptr);

// ============================================================
// DxfValue::Release
// ============================================================
void DxfValue::Release() noexcept {
    if (!m_data) return;
    switch (m_type) {
    case DxfValueType::kInt8:     delete static_cast<int8_t*>(m_data);   break;
    case DxfValueType::kInt16:    delete static_cast<int16_t*>(m_data);  break;
    case DxfValueType::kInt32:    delete static_cast<int32_t*>(m_data);  break;
    case DxfValueType::kInt64:    delete static_cast<int64_t*>(m_data);  break;
    case DxfValueType::kDouble:   delete static_cast<double*>(m_data);   break;
    case DxfValueType::kString:   delete static_cast<AcString*>(m_data); break;
    case DxfValueType::kPoint2d:
    case DxfValueType::kPoint3d:  delete static_cast<std::array<double, 3>*>(m_data); break;
    case DxfValueType::kBinary:   delete static_cast<std::vector<char>*>(m_data);     break;
    case DxfValueType::kHandle:   delete static_cast<std::array<uint8_t, 8>*>(m_data); break;
    case DxfValueType::kAdsName:  delete static_cast<PyAdsName*>(m_data);             break;
    default: break;
    }
    m_data = nullptr;
}

// ============================================================
// DxfValue::CloneData
// ============================================================
void* DxfValue::CloneData(DxfValueType type, const void* src) {
    if (!src) return nullptr;
    switch (type) {
    case DxfValueType::kInt8:     return new int8_t(*static_cast<const int8_t*>(src));
    case DxfValueType::kInt16:    return new int16_t(*static_cast<const int16_t*>(src));
    case DxfValueType::kInt32:    return new int32_t(*static_cast<const int32_t*>(src));
    case DxfValueType::kInt64:    return new int64_t(*static_cast<const int64_t*>(src));
    case DxfValueType::kDouble:   return new double(*static_cast<const double*>(src));
    case DxfValueType::kString:   return new AcString(*static_cast<const AcString*>(src));
    case DxfValueType::kPoint2d:
    case DxfValueType::kPoint3d:  return new std::array<double, 3>(*static_cast<const std::array<double, 3>*>(src));
    case DxfValueType::kBinary:   return new std::vector<char>(*static_cast<const std::vector<char>*>(src));
    case DxfValueType::kHandle:   return new std::array<uint8_t, 8>(*static_cast<const std::array<uint8_t, 8>*>(src));
    case DxfValueType::kAdsName:  return new PyAdsName(*static_cast<const PyAdsName*>(src));
    default: return nullptr;
    }
}

// ============================================================
// DxfValue::toString
// ============================================================
AcString DxfValue::toString() const {
    switch (m_type) {
    case DxfValueType::kNull:     return AcString();
    case DxfValueType::kInt8:     return AcString(std::format(L"{}", asInt8()));
    case DxfValueType::kInt16:    return AcString(std::format(L"{}", asInt16()));
    case DxfValueType::kInt32:    return AcString(std::format(L"{}", asInt32()));
    case DxfValueType::kInt64:    return AcString(std::format(L"{}", asInt64()));
    case DxfValueType::kDouble:   return AcString(std::format(L"{}", asDouble()));
    case DxfValueType::kString:   return asString();
    case DxfValueType::kPoint2d: {
        auto p = asPoint2d();
        return AcString(std::format(L"({:.3f},{:.3f})", p[0], p[1]));
    }
    case DxfValueType::kPoint3d: {
        auto& p = asPoint3d();
        return AcString(std::format(L"({:.3f},{:.3f},{:.3f})", p[0], p[1], p[2]));
    }
    case DxfValueType::kBinary: {
        auto& bin = asBinary();
        AcString acs;
        acs.format(L"bin[%d] ", static_cast<int>(bin.size()));
        for (auto& c : bin) {
            AcString hex;
            hex.format(L"%02X", static_cast<unsigned char>(c));
            acs += hex;
        }
        return acs;
    }
    case DxfValueType::kHandle: {
        auto& handle = asHandle();
        AcString acs = L"handle [8]: ";
        for (auto& c : handle) {
            AcString hex;
            hex.format(L"%02X", static_cast<unsigned char>(c));
            acs += hex;
        }
        return acs;
    }
    case DxfValueType::kAdsName: {
        PyAdsName name = asAdsName();
        AcString acs;
        acs.format(L"<adsname %x>", static_cast<int64_t>(name[0]));
        return acs;
    }
    default:
        return L"null";
    }
}

// ============================================================
// TypedValue::checkIsValid
// ============================================================
bool TypedValue::checkIsValid() const {
    auto codetype = getDxfValueType(code);
    return codetype == value.getType() && codetype != DxfValueType::kUnKnown;
}

// ============================================================
// TypedValue::toString
// ============================================================
AcString TypedValue::toString() const {
    AcString codeSTR;
    switch (code) {
    case RTNONE:     codeSTR = L"RTNONE";     break;
    case RTREAL:     codeSTR = L"RTREAL";     break;
    case RTPOINT:    codeSTR = L"RTPOINT";    break;
    case RTSHORT:    codeSTR = L"RTSHORT";    break;
    case RTANG:      codeSTR = L"RTANG";      break;
    case RTSTR:      codeSTR = L"RTSTR";      break;
    case RTENAME:    codeSTR = L"RTENAME";    break;
    case RTPICKS:    codeSTR = L"RTPICKS";    break;
    case RTORINT:    codeSTR = L"RTORINT";    break;
    case RT3DPOINT:  codeSTR = L"RT3DPOINT";  break;
    case RTLONG:     codeSTR = L"RTLONG";     break;
    case RTVOID:     codeSTR = L"RTVOID";     break;
    case RTLB:       codeSTR = L"RTLB";       break;
    case RTLE:       codeSTR = L"RTLE";       break;
    case RTDOTE:     codeSTR = L"RTDOTE";     break;
    case RTNIL:      codeSTR = L"RTNIL";      break;
    case RTDXF0:     codeSTR = L"RTDXF0";     break;
    case RTT:        codeSTR = L"RTT";        break;
    case RTRESBUF:   codeSTR = L"RTRESBUF";   break;
    case RTMODELESS: codeSTR = L"RTMODELESS"; break;
    default:
        codeSTR.format(L"%d", static_cast<int>(code));
        break;
    }

    AcString valStr = value.toString();
    if (valStr.isEmpty()) {
        return codeSTR;
    }
    AcString acs;
    acs.format(L"(%s . %s)", codeSTR, valStr);
    return acs;
}

// ============================================================
// TVList::toString
// ============================================================
AcString TVList::toString() const {
    AcString result;
    for (size_t i = 0; i < size(); ++i) {
        result += (*this)[i].toString();
        if (i + 1 < size()) result += L"\n";
    }
    return result;
}

// ============================================================
// 比较函数
// ============================================================
TVList PyEq(const TypedValue& cond) { return SymboleEQ & cond; }
TVList PyNe(const TypedValue& cond) { return SymboleNE & cond; }
TVList PyGt(const TypedValue& cond) { return SymboleGT & cond; }
TVList PyLt(const TypedValue& cond) { return SymboleLT & cond; }
TVList PyGe(const TypedValue& cond) { return SymboleGE & cond; }
TVList PyLe(const TypedValue& cond) { return SymboleLE & cond; }

TVList PyEq(short code, const DxfValue& cond) { return PyEq(TypedValue(code, cond)); }
TVList PyNe(short code, const DxfValue& cond) { return PyNe(TypedValue(code, cond)); }
TVList PyLt(short code, const DxfValue& cond) { return PyLt(TypedValue(code, cond)); }
TVList PyLe(short code, const DxfValue& cond) { return PyLe(TypedValue(code, cond)); }
TVList PyGt(short code, const DxfValue& cond) { return PyGt(TypedValue(code, cond)); }
TVList PyGe(short code, const DxfValue& cond) { return PyGe(TypedValue(code, cond)); }

// ============================================================
// 逻辑运算符
// ============================================================
TVList operator~(const TypedValue& cond) { return { NOT_START, cond, NOT_END }; }

TVList operator~(const TVList& f) {
    TVList res;
    res.reserve(f.size() + 2);
    res.push_back(NOT_START);
    res.insert(res.end(), f.begin(), f.end());
    res.push_back(NOT_END);
    return res;
}

TVList operator&(const TypedValue& lhs, const TypedValue& rhs) { return { AND_START, lhs, rhs, AND_END }; }

TVList operator&(const TypedValue& lhs, const TVList& rhs) {
    TVList res{ AND_START, lhs };
    res.insert(res.end(), rhs.begin(), rhs.end());
    res.push_back(AND_END);
    return res;
}

TVList operator&(const TVList& lhs, const TypedValue& rhs) {
    TVList res{ AND_START };
    res.insert(res.end(), lhs.begin(), lhs.end());
    res.push_back(rhs);
    res.push_back(AND_END);
    return res;
}

TVList operator&(const TVList& lhs, const TVList& rhs) {
    TVList res{ AND_START };
    res.insert(res.end(), lhs.begin(), lhs.end());
    res.insert(res.end(), rhs.begin(), rhs.end());
    res.push_back(AND_END);
    return res;
}

TVList operator|(const TypedValue& lhs, const TypedValue& rhs) { return { OR_START, lhs, rhs, OR_END }; }

TVList operator|(const TypedValue& lhs, const TVList& rhs) {
    TVList res{ OR_START, lhs };
    res.insert(res.end(), rhs.begin(), rhs.end());
    res.push_back(OR_END);
    return res;
}

TVList operator|(const TVList& lhs, const TypedValue& rhs) {
    TVList res{ OR_START };
    res.insert(res.end(), lhs.begin(), lhs.end());
    res.push_back(rhs);
    res.push_back(OR_END);
    return res;
}

TVList operator|(const TVList& lhs, const TVList& rhs) {
    TVList res{ OR_START };
    res.insert(res.end(), lhs.begin(), lhs.end());
    res.insert(res.end(), rhs.begin(), rhs.end());
    res.push_back(OR_END);
    return res;
}

// ---------- 替代记号 ----------
TVList operator not(const TypedValue& cond) { return ~cond; }
TVList operator not(const TVList& f) { return ~f; }

TVList operator and(const TypedValue& lhs, const TypedValue& rhs) { return lhs & rhs; }
TVList operator and(const TypedValue& lhs, const TVList& rhs) { return lhs & rhs; }
TVList operator and(const TVList& lhs, const TypedValue& rhs) { return lhs & rhs; }
TVList operator and(const TVList& lhs, const TVList& rhs) { return lhs & rhs; }

TVList operator or (const TypedValue& lhs, const TypedValue& rhs) { return lhs | rhs; }
TVList operator or (const TypedValue& lhs, const TVList& rhs) { return lhs | rhs; }
TVList operator or (const TVList& lhs, const TypedValue& rhs) { return lhs | rhs; }
TVList operator or (const TVList& lhs, const TVList& rhs) { return lhs | rhs; }

// ============================================================
// 类型映射
// ============================================================
DxfValueType getDxfValueType(int code) noexcept {
    switch (code) {
    case -2: case -1:
    case 330: case 331: case 332: case 333: case 334: case 335: case 336: case 337: case 338: case 339:
    case 340: case 341: case 342: case 343: case 344: case 345: case 346: case 347: case 348: case 349:
    case 350: case 351: case 352: case 353: case 354: case 355: case 356: case 357: case 358: case 359:
    case 360: case 361: case 362: case 363: case 364: case 365: case 366: case 367: case 368: case 369:
    case 390: case 391: case 392: case 393: case 394: case 395: case 396: case 397: case 398: case 399:
    case 5006: case 5007:
        return DxfValueType::kAdsName;

    case 310: case 311: case 312: case 313: case 314: case 315: case 316: case 317: case 318: case 319:
    case 1004:
        return DxfValueType::kBinary;

    case 38: case 39:
    case 40: case 41: case 42: case 43: case 44: case 45: case 46: case 47: case 48: case 49:
    case 50: case 51: case 52: case 53: case 54: case 55: case 56: case 57: case 58: case 59:
    case 140: case 141: case 142: case 143: case 144: case 145: case 146: case 147: case 148: case 149:
    case 460: case 461: case 462: case 463: case 464: case 465: case 466: case 467: case 468: case 469:
    case 1040: case 1041: case 1042:
    case 5001: case 5004:
        return DxfValueType::kDouble;

    case 60: case 61: case 62: case 63: case 64: case 65: case 66: case 67: case 68: case 69:
    case 70: case 71: case 72: case 73: case 74: case 75: case 76: case 77: case 78: case 79:
    case 170: case 171: case 172: case 173: case 174: case 175: case 176: case 177: case 178: case 179:
    case 270: case 271: case 272: case 273: case 274: case 275: case 276: case 277: case 278: case 279:
    case 370: case 371: case 372: case 373: case 374: case 375: case 376: case 377: case 378: case 379:
    case 380: case 381: case 382: case 383: case 384: case 385: case 386: case 387: case 388: case 389:
    case 400: case 401: case 402: case 403: case 404: case 405: case 406: case 407: case 408: case 409:
    case 1070:
    case 5003:
        return DxfValueType::kInt16;

    case 90: case 91: case 92: case 93: case 94: case 95: case 96: case 97: case 98: case 99:
    case 420: case 421: case 422: case 423: case 424: case 425: case 426: case 427: case 428: case 429:
    case 440: case 441: case 442: case 443: case 444: case 445: case 446: case 447: case 448: case 449:
    case 450: case 451: case 452: case 453: case 454: case 455: case 456: case 457: case 458: case 459:
    case 1071:
    case 5010:
        return DxfValueType::kInt32;

    case 5030: case 5031:
        return DxfValueType::kInt64;

    case 280: case 281: case 282: case 283: case 284: case 285: case 286: case 287: case 288: case 289:
    case 290: case 291: case 292: case 293: case 294: case 295: case 296: case 297: case 298: case 299:
        return DxfValueType::kInt8;

    case 5000: case 5014: case 5016: case 5017: case 5018: case 5019: case 5020: case 5021:
        return DxfValueType::kNull;

    case 5002:
        return DxfValueType::kPoint2d;

    case 10: case 11: case 12: case 13: case 14: case 15: case 16: case 17:
    case 110: case 111: case 112:
    case 210: case 211: case 212: case 213: case 214: case 215: case 216: case 217: case 218: case 219:
    case 1010: case 1011: case 1012: case 1013:
    case 5008: case 5009:
        return DxfValueType::kPoint3d;

    case -4:
    case 0: case 1: case 2: case 3: case 4: case 5: case 6: case 7: case 8: case 9:
    case 100: case 101: case 102: case 105:
    case 300: case 301: case 302: case 303: case 304: case 305: case 306: case 307: case 308: case 309:
    case 320: case 321: case 322: case 323: case 324: case 325: case 326: case 327: case 328: case 329:
    case 410: case 411: case 412: case 413: case 414: case 415: case 416: case 417: case 418: case 419:
    case 430: case 431: case 432: case 433: case 434: case 435: case 436: case 437: case 438: case 439:
    case 470: case 471: case 472: case 473: case 474: case 475: case 476: case 477: case 478: case 479:
    case 999: case 1000: case 1001: case 1002: case 1003: case 1005:
    case 5005:
        return DxfValueType::kString;

    default:
        return DxfValueType::kUnKnown;
    }
}

// ============================================================
// DxfValue::toPy — 自动转换为 Python 对象
// ============================================================
pybind11::object DxfValue::toPy() const {
    using namespace pybind11;
    switch (m_type) {
    case DxfValueType::kNull:   return none();
    case DxfValueType::kInt8:   return int_(asInt8());
    case DxfValueType::kInt16:  return int_(asInt16());
    case DxfValueType::kInt32:  return int_(asInt32());
    case DxfValueType::kInt64:  return int_(asInt64());
    case DxfValueType::kDouble: return float_(asDouble());
    case DxfValueType::kString: return cast(asString());
    case DxfValueType::kPoint2d: {
        auto p = asPoint2d();
        return make_tuple(p[0], p[1]);
    }
    case DxfValueType::kPoint3d: {
        auto p = asPoint3d();
        return make_tuple(p[0], p[1], p[2]);
    }
    case DxfValueType::kAdsName: {
        auto& n = asAdsName();
        return make_tuple((uint64_t)n[0], (uint64_t)n[1]);
    }
    case DxfValueType::kBinary:
        return bytes(std::string(asBinary().data(), asBinary().size()));
    case DxfValueType::kHandle: {
        auto h = asHandle();
        return bytes(std::string(reinterpret_cast<const char*>(h.data()), h.size()));
    }
    default: return none();
    }
}

// ============================================================
// DxfValue::fromPy — 从 Python 对象构造（按组码类型推断）
// ============================================================
DxfValue DxfValue::fromPy(const pybind11::object& v, int code) {
    using namespace pybind11;
    DxfValueType ty = getDxfValueType(code);
    switch (ty) {
    case DxfValueType::kInt8:
    case DxfValueType::kInt16:
    case DxfValueType::kInt32:
    case DxfValueType::kInt64:
        return DxfValue::fromInt64(v.cast<int64_t>());
    case DxfValueType::kDouble:
        return DxfValue::fromDouble(v.cast<double>());
    case DxfValueType::kString:
        return DxfValue::fromString(v.cast<AcString>());
    case DxfValueType::kPoint2d: {
        auto t = v.cast<py::tuple>();
        return DxfValue::fromPoint2d(t[0].cast<double>(), t[1].cast<double>());
    }
    case DxfValueType::kPoint3d: {
        auto t = v.cast<py::tuple>();
        return DxfValue::fromPoint3d(
            t[0].cast<double>(), t[1].cast<double>(), t[2].cast<double>());
    }
    case DxfValueType::kBinary: {
        std::string s = v.cast<std::string>();
        return DxfValue::fromBinary(std::vector<char>(s.begin(), s.end()));
    }
    default:
        return DxfValue::fromInt32(0);
    }
}