#include "stdafx.h"
#include "PyResbuf.h"
#include "stdafx.h"
#include "PyAdsName.h"
// ==================== 类型映射（纯 switch，无字典） ====================


PyResbuf& PyResbuf::operator=(PyResbuf&& other) noexcept {
	if (this != &other) {
		reset(other.releaseOwner());            // 释放旧资源，接管新资源 
	}
	return *this;
}

void PyResbuf::reset(resbuf* rb) noexcept {
	if (m_pRb) acutRelRb(m_pRb);
	m_pRb = rb;
}


TVList PyResbuf::toList() const {
	TVList result;
	if (!m_pRb) return{};
	resbuf* cur = m_pRb;
	while (cur) {
		DxfValueType T = getDxfValueType(cur->restype);
		switch (T) {
		case DxfValueType::kNull:		result.emplace_back(cur->restype, DxfValue::kNull); break;
		case DxfValueType::kInt8:		result.emplace_back(cur->restype, static_cast<int8_t> (cur->resval.rbyte)); break;
		case DxfValueType::kInt16:		result.emplace_back(cur->restype, static_cast<int16_t>(cur->resval.rint)); break;
		case DxfValueType::kInt32:		result.emplace_back(cur->restype, static_cast<int32_t>(cur->resval.rlong)); break;
		case DxfValueType::kInt64:		result.emplace_back(cur->restype, static_cast<int64_t>(cur->resval.mnInt64)); break;
		case DxfValueType::kDouble:	result.emplace_back(cur->restype, static_cast<double>(cur->resval.rreal)); break;
		case DxfValueType::kString:	result.emplace_back(cur->restype, AcString(cur->resval.rstring)); break;
		case DxfValueType::kPoint2d:	result.emplace_back(cur->restype, std::array<double, 2>{ cur->resval.x, cur->resval.y }); break;
		case DxfValueType::kPoint3d:	result.emplace_back(cur->restype, std::array<double, 3>{ cur->resval.x, cur->resval.y, cur->resval.z }); break;
		case DxfValueType::kBinary:	result.emplace_back(cur->restype, cur->resval.rbinary); break;
		case DxfValueType::kHandle:	result.emplace_back(cur->restype, cur->resval.mHandle); break;
		case DxfValueType::kAdsName:	result.emplace_back(cur->restype, PyAdsName(cur->resval.rlname)); break;
		default:
			acutPrintf(L"error type code =%d, dwgtype = %d ", cur->restype, acdbGroupCodeToType(cur->restype)); break;
		}
		cur = cur->rbnext;
	}
	return result;
}
resbuf* PyResbuf::buildRb(const TypedValue& pair)
{
	short code = pair.code;
	DxfValueType T = getDxfValueType(code);
	resbuf* pNode = acutNewRb(pair.code);
	pNode->rbnext = nullptr;
	pNode->restype = code;

	switch (T) {
	case DxfValueType::kNull:			pNode->resval.mnInt64 = 0; break;
	case DxfValueType::kInt8:			pNode->resval.mnInt64 = pair.value.asInt8(); break;
	case DxfValueType::kInt16:			pNode->resval.mnInt64 = pair.value.asInt16(); break;
	case DxfValueType::kInt32:			pNode->resval.mnInt64 = pair.value.asInt32(); break;
	case DxfValueType::kInt64:			pNode->resval.mnInt64 = pair.value.asInt64(); break;
	case DxfValueType::kDouble:		pNode->resval.rreal = pair.value.asDouble(); break;
	case DxfValueType::kString: {
		auto& acs = pair.value.asString();
		acutNewString(acs, pNode->resval.rstring);
		break;
	}
	case DxfValueType::kPoint2d: {
		auto pnt = pair.value.asPoint2d();
		pNode->resval.x = pnt[0];
		pNode->resval.y = pnt[1];
		pNode->resval.z = 0;
		break;
	}
	case DxfValueType::kPoint3d: {
		auto& pnt = pair.value.asPoint3d();
		pNode->resval.x = pnt[0];
		pNode->resval.y = pnt[1];
		pNode->resval.z = pnt[2];
		break;
	}
	case DxfValueType::kBinary: {
		auto& vec = pair.value.asBinary();
		pNode->resval.rbinary.clen = vec.size();
		pNode->resval.rbinary.buf = static_cast<char*>(malloc(vec.size()));
		if (pNode->resval.rbinary.buf) {
			std::memcpy(pNode->resval.rbinary.buf, vec.data(), vec.size());
		}
		break;
	}
	case DxfValueType::kHandle:
		pNode->resval.mHandle = pair.value.asHandle(); break;

	case DxfValueType::kAdsName:
		pNode->resval.rlname[0] = pair.value.asAdsName()[0];
		pNode->resval.rlname[1] = pair.value.asAdsName()[1];
	}


	return pNode;
}



resbuf* PyResbuf::buildRb(const TVList& tvs)
{
	if (tvs.empty()) return nullptr;

	resbuf* pHead = nullptr;
	resbuf* pCur = nullptr;

	for (auto& pair : tvs) {
		resbuf* pNode = buildRb(pair);   // 临时拥有一个节点 

		if (pHead == nullptr) {
			pHead = pNode;
			pCur = pHead;
		}
		else {
			pCur->rbnext = pNode;
			pCur = pNode;
		}
	}

	return pHead;
}

resbuf* PyResbuf::releaseOwner() noexcept {
	resbuf* tmp = m_pRb;
	m_pRb = nullptr;
	return tmp;
}




void PyResbuf::print() const
{
	acutPrintf(toString());

}

AcString PyResbuf::toString() const
{
	if (!m_pRb) return L"nullptr";
	return toList().toString();
}
