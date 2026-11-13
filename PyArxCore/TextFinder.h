#pragma once
#include "stdafx.h"
#include <dbobjptr2.h>

class QTextFind {
private:
	std::map<AcDbObjectId, AcString> pairs;
public:
	template<typename Container>
	QTextFind(const Container ids) {
		for (const AcDbObjectId& objid : ids) {
			if (objid.isClassOf<AcDbText>()) {
				auto acs = AcString(AcDbObjectPointer<AcDbText>(objid)->textString()).makeUpper();
				pairs.emplace(objid, acs);
			}
			if (objid.isClassOf<AcDbMText>()) {
				auto acs = AcString(AcDbObjectPointer<AcDbMText>(objid)->contents()).makeUpper();
				pairs.emplace(objid, acs);
			}
		}
	}

	std::vector<AcDbObjectId> matchStart(const AcString& input) const {
		std::vector<AcDbObjectId> result;
		for (auto& [objid, acs] : pairs) {
			if (acs.matchPrefix(input, true)) {
				result.push_back(objid);
			}
		}
		return result;
	}

	std::vector<AcDbObjectId> matchAnyPos(const AcString& input) const {
		std::vector<AcDbObjectId> result;
		for (auto& [objid, acs] : pairs) {
			if (acs.matchAnyPos(input,true)) {
				result.push_back(objid);
			}
		}
		return result;
	}

	std::vector<AcDbObjectId> matchFull(const AcString& input) const {
		std::vector<AcDbObjectId> result;
		for (auto& [objid, acs] : pairs) {
			if (AcString::equalsNoCase(acs, input)) {
				result.push_back(objid);
			}
		}
		return result;
	}
};