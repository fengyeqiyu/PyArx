#pragma once
#include "stdafx.h"


class AcDbHandle;
class PySelectionSet;
class PyAdsName
{
private:
	ads_name m_data;
public:
	// ---------- 构造 / 析构 ----------
	PyAdsName();
	PyAdsName(const PyAdsName& other);
	PyAdsName(const ads_name& other);
	~PyAdsName();


	PyAdsName& operator=(const PyAdsName& other);
	// 从 AcDbObjectId 构造
	explicit PyAdsName(const AcDbObjectId& objId);

	// 静态工厂：错误处理更明确
	static bool fromObjectId(const AcDbObjectId& objId, PyAdsName& out);

	// ---------- 核心：暴露底层数组 ----------
	ads_name& data() { return m_data; }
	const ads_name& data() const { return m_data; }
	// 数组式访问
	int64_t& operator[](int i) { return m_data[i]; }
	const int64_t& operator[](int i) const { return m_data[i]; }
	// ---------- 扩展 API ----------
	bool isEmpty() const;
	void clear();

	AcString toString() const;

	// 反向转换：转回 AcDbObjectId
	AcDbObjectId toObjectId() const;

	// 是否有效（能成功转回非空 ObjectId）
	bool isValid() const;
	// ---------- 比较运算符 ----------
	bool operator==(const PyAdsName& other) const;
	bool operator!=(const PyAdsName& other) const;
	bool operator<(const PyAdsName& other) const;
};