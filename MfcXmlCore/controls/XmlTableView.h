#pragma once
#include "XmlMfcControlBase.h"
#include "XmlContentParse.h"

// <TableView> 数据表格：Columns="名称	数值	状态"，Rows 每行用 "|" 分隔
//
// 属性走 XmlTypes.h 的属性表（S1）。注意：Name 是结构属性（由加载器/运行时消费），
// 不是控件属性，历史上把它写进 ownProperties 是错的，已在表中移除。
class XML_CORE_API XmlTableView : public XmlMfcControlBase
{

public:
    XmlTableView() : XmlMfcControlBase(L"tableview") {}

protected:
    void createNative(IXmlMfcHost* host, int id, int minValue, int maxValue, int x, int y, int w, int h) override;

    const XmlPropEntry* ownPropTable(size_t& count) const override
    {
        const std::vector<XmlPropEntry>& t = PropTableData();
        count = t.size();
        return t.data();
    }

private:
    static const std::vector<XmlPropEntry>& PropTableData();

    bool SetColumnsProp(const std::wstring& v);
    bool SetRowsProp(const std::wstring& v);
    bool SetSelectedRowProp(const std::wstring& v);
    std::wstring GetSelectedRowProp() const;
    std::wstring GetRowCountProp() const;
};
