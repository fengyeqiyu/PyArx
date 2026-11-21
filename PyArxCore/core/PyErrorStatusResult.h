#pragma once
#include "stdafx.h"

template<typename T>
struct PyErrorStatusResult {
	Acad::ErrorStatus es;
	T value;

	PyErrorStatusResult(Acad::ErrorStatus status, T result) :
		es(status), value(result) {
	}

	bool isOk() const {
		return es == Acad::eOk;
	}

	AcString  StatusMessage() const {
		return acadErrorStatusText(es);
	}
	__declspec(property(get = isOk))  bool Ok;
	__declspec(property(get = StatusMessage))  AcString  StatusMessage;
};



