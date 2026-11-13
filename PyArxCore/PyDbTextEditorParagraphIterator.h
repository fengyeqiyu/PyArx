#pragma once
#include "stdafx.h"

// SDK: dbtexteditor.h — class AcDbTextEditorParagraphIterator（无基类，独立类）
// ctor 需要 AcDbTextEditor&，源类在③档未封装，暂无公开创建路径。
// 当前元素 current() 返回未封装的 AcDbTextEditorParagraph*，以原始指针地址(uint64)透出。
class AcDbTextEditorParagraphIterator;

class PyDbTextEditorParagraphIterator {
	AcDbTextEditorParagraphIterator* m_p = nullptr;
public:
	// 指针接管：析构 delete。
	explicit PyDbTextEditorParagraphIterator(AcDbTextEditorParagraphIterator* p) : m_p(p) {}
	~PyDbTextEditorParagraphIterator();
	PyDbTextEditorParagraphIterator(const PyDbTextEditorParagraphIterator&) = delete;
	PyDbTextEditorParagraphIterator& operator=(const PyDbTextEditorParagraphIterator&) = delete;

	bool done();
	bool step();

public:
	AcDbTextEditorParagraphIterator* impObj() { return m_p; }
};

void bind_PyDbTextEditorParagraphIterator(pym& m);
