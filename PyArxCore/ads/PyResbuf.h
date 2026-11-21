#pragma once 

#include "stdafx.h"
#include <vector>
#include <array>
#include <string>
#include <memory>
#include "PyDxfTypeValue.h"


class PyResbuf {
	resbuf* m_pRb = nullptr;   // 裸指针，独占所有权 
	static resbuf* buildRb(const TypedValue& pair);
	static resbuf* buildRb(const TVList& pylist);
	// ---- 所有权转移 ----
	resbuf* releaseOwner() noexcept;

public:
	// ---- 构造函数 ----
	PyResbuf() = default;
	explicit PyResbuf(resbuf* rb) : m_pRb(rb) {}
	PyResbuf(const TypedValue& pair) : m_pRb(buildRb(pair)) {}
	PyResbuf(const TVList& tvs) : m_pRb(buildRb(tvs)) {}
	PyResbuf(const std::vector<AcGePoint3d>& pts) {
		TVList tvs;
		for (auto& pt : pts) {
			tvs.emplace_back(RTPOINT, pt.toArray());
		}
		m_pRb = buildRb(tvs);
	}
	~PyResbuf() {
		if (m_pRb) acutRelRb(m_pRb);
	}

	// ---- 拷贝：深度克隆 ----
	PyResbuf(const PyResbuf& other) {
		if (other.m_pRb) {
			TVList list = other.toList();      // 提取数据 
			m_pRb = buildRb(list);             // 重建链表 
		}
		else {
			m_pRb = nullptr;
		}
	}

	PyResbuf& operator=(const PyResbuf& other) {
		if (this != &other) {
			reset();                           // 释放当前资源 
			if (other.m_pRb) {
				TVList list = other.toList();
				m_pRb = buildRb(list);
			}
		}
		return *this;
	}

	// ---- 移动：转移所有权 ----
	PyResbuf(PyResbuf&& other) noexcept : m_pRb(other.releaseOwner()) {}

	PyResbuf& operator=(PyResbuf&& other) noexcept;

	// ---- 访问裸指针 ----

	resbuf* data() { return m_pRb; }
	const resbuf* data() const { return m_pRb; }
	bool isNull() const { return m_pRb == nullptr; }
	void reset(resbuf* rb = nullptr) noexcept;

	// ---- 功能函数 ----
	TVList toList() const;
	AcString toString() const;
	void print() const;
};

