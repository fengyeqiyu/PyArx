#pragma once
#include "stdafx.h"
#include <dbobjptr2.h>
#include <dbents.h>    // AcDbText
#include <dbmtext.h>   // AcDbMText
#include <dbid.h>      // AcDbObjectId::isClassOf<T>()
#include <map>
#include <vector>

// QTextFind — 批量文字索引（AcDbText / AcDbMText，大小写无关）。
// 构造一次后可在同一批 id 上反复查询，避免每次查询都重新打开图元。
// 打开失败 / 已删除 / 空 id 一律跳过，不抛异常：索引尽量全，不做部分失败中断。
class QTextFind {
private:
    std::map<AcDbObjectId, AcString> pairs;

    void add(const AcDbObjectId& objid, const ACHAR* text) {
        if (!text) return;
        AcString acs(text);
        acs.makeUpper();
        pairs.emplace(objid, acs);
    }

public:
    template<typename Container>
    explicit QTextFind(const Container& ids) {
        for (const AcDbObjectId& objid : ids) {
            if (objid.isNull()) continue;
            if (objid.isClassOf<AcDbText>()) {
                AcDbObjectPointer<AcDbText> p(objid, AcDb::kForRead);
                if (p.openStatus() != Acad::eOk || p.object() == nullptr) continue;
                add(objid, p->textString());
            } else if (objid.isClassOf<AcDbMText>()) {
                AcDbObjectPointer<AcDbMText> p(objid, AcDb::kForRead);
                if (p.openStatus() != Acad::eOk || p.object() == nullptr) continue;
                add(objid, p->contents());
            }
        }
    }

    size_t size() const { return pairs.size(); }

    std::vector<AcDbObjectId> matchStart(const AcString& input) const {
        std::vector<AcDbObjectId> result;
        AcString kw(input);
        kw.makeUpper();
        for (auto& [objid, acs] : pairs) {
            if (acs.matchPrefix(kw, true)) result.push_back(objid);
        }
        return result;
    }

    std::vector<AcDbObjectId> matchAnyPos(const AcString& input) const {
        std::vector<AcDbObjectId> result;
        AcString kw(input);
        kw.makeUpper();
        for (auto& [objid, acs] : pairs) {
            if (acs.matchAnyPos(kw, true)) result.push_back(objid);
        }
        return result;
    }

    std::vector<AcDbObjectId> matchFull(const AcString& input) const {
        std::vector<AcDbObjectId> result;
        for (auto& [objid, acs] : pairs) {
            if (AcString::equalsNoCase(acs, input)) result.push_back(objid);
        }
        return result;
    }
};
