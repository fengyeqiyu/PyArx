#pragma once
#include "stdafx.h"
#include "PyDxfTypeValue.h"
class PyAdsName;
class PyResbuf;

// AutoCAD 选择集拥有体（RAII）：
//   - 持有一个 ads_name 选择集句柄，m_owned=true 时析构自动 acedSSFree；
//   - 默认构造为空（不持有、不释放）；从 ads_name/(a,b) 构造即接管所有权；
//   - 禁拷贝（共享句柄会导致析构二次释放），仅允许 move 转移所有权。
class PyEdSelectionSets {
private:
	ads_name m_data{};        // 选择集名称（ads_name）
	bool m_owned = false;     // 是否由本对象持有并负责释放
public:
	// 默认构造：空集，不持有任何选择集
	PyEdSelectionSets() {
		m_data[0] = 0;
		m_data[1] = 0;
		m_owned = false;
	}
	// 从 ads_name 构造：接管所有权，析构释放
	explicit PyEdSelectionSets(const ads_name& ss) {
		std::memcpy(m_data, ss, sizeof(m_data));
		m_owned = true;
	}
	// 从 int64_t 数组构造：接管所有权，析构释放
	explicit PyEdSelectionSets(const int64_t* ss) {
		std::memcpy(m_data, ss, sizeof(m_data));
		m_owned = true;
	}

	// 禁拷贝：同一选择集句柄不能被两个对象同时持有（否则二次释放）
	PyEdSelectionSets(const PyEdSelectionSets&) = delete;
	PyEdSelectionSets& operator=(const PyEdSelectionSets&) = delete;
	// move：转移所有权，源对象置空不再释放
	PyEdSelectionSets(PyEdSelectionSets&& o) noexcept {
		std::memcpy(m_data, o.m_data, sizeof(m_data));
		m_owned = o.m_owned;
		o.m_data[0] = o.m_data[1] = 0;
		o.m_owned = false;
	}
	PyEdSelectionSets& operator=(PyEdSelectionSets&& o) noexcept {
		if (this != &o) {
			if (m_owned) ::acedSSFree(m_data);
			std::memcpy(m_data, o.m_data, sizeof(m_data));
			m_owned = o.m_owned;
			o.m_data[0] = o.m_data[1] = 0;
			o.m_owned = false;
		}
		return *this;
	}

	// 析构：仅持有有效选择集时释放（自析构）
	~PyEdSelectionSets() {
		if (m_owned) ::acedSSFree(m_data);
		m_data[0] = 0;
		m_data[1] = 0;
		m_owned = false;
	}

	// ---------- 所有权管理 ----------
	// 外部 SDK（acedSSGet/acedSSAdd）已在 m_data 建集，声明由本对象负责释放
	void attach() { m_owned = true; }
	// 放弃所有权（转交给别处），析构不再释放
	void detach() { m_owned = false; }
	bool owned() const { return m_owned; }

	// ---------- 底层访问 ----------
	ads_name& ads() { return m_data; }
	const ads_name& ads() const { return m_data; }
	// 兼容旧接口：指向内部句柄的指针
	int64_t* data() { return m_data; }
	const int64_t* data() const { return m_data; }

	// ---------- 操作 ----------
	int clear();
	bool isEmpty() const;
	size_t count() const;
	PyAdsName getEntNameAt(size_t index) const;
	PyResbuf getResbufAt(size_t index) const;
	AcDbObjectId getObjectIdAt(size_t index) const;
	size_t subentCount(size_t index) const;
	void add(const AcDbObjectId& objId);
	void remove(const AcDbObjectId& objId);
	bool hasMember(const AcDbObjectId& objId) const;
	AcDbFullSubentPath subentName(size_t entIndex, size_t subentIndex) const;
	TVList subentNameX(size_t entIndex, size_t subentIndex, int flags) const;
	bool ssSetFirst() const;
	Acad::PromptStatus ssXform(const AcGeMatrix3d& xform) const;
	// idx = -1 时表示遍历所有选择对象结果
	TVList ssNameX(int idx = 0) const;
	std::vector<AcDbObjectId> objectIds() const;
	AcString toString() const
	{
		AcString acs;
		acs.format(L"<SelectionSet (%08X,%08X) owned=%d>",
			static_cast<unsigned>(m_data[1]),
			static_cast<unsigned>(m_data[0]),
			m_owned ? 1 : 0);
		return acs;
	}
};
