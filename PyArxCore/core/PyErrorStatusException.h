#pragma once
#include "stdafx.h"
#include "PyEngineCompat.h"

// ---------------------------------------------------------------------------
// Shared wrapper-layer error handling (used by PyDb*/PyGe*/PyEd* wrapper classes).
// Default rule (non-throwing): SDK failure is printed to the CAD command line and
// the wrapper returns a status / safe default; it must NOT throw into Python.
//   - success -> wrapper returns the real result value / true;
//   - failure -> PyWarnIfError prints "[PyArx] <text> (eNum) [Context]" to the
//                CAD command line and returns false; callers return safe defaults.
// PyErrorStatusException / PyThrowIfError are retained for optional explicit use
// but are NOT the default path.
// ---------------------------------------------------------------------------

class PyErrorStatusException : public std::runtime_error {
public:
	explicit PyErrorStatusException(Acad::ErrorStatus es, const AcString& ctx = AcString())
		: std::runtime_error(buildMessage(es, ctx)), m_es(es) {}

	Acad::ErrorStatus errorStatus() const noexcept { return m_es; }

private:
	static std::string buildMessage(Acad::ErrorStatus es, const AcString& ctx) {
		std::string msg = "PyErrorStatusException: ";
		AcString txt = acadErrorStatusText(es);
		const char* p = txt.utf8Ptr();
		if (p) msg += p;
		if (!ctx.isEmpty()) {
			msg += " [";
			if (const char* up = ctx.utf8Ptr()) msg += up; // UTF-8
			msg += "]";
		}
		return msg;
	}

	Acad::ErrorStatus m_es;
};

// Throws PyErrorStatusException when the SDK call failed; retained for optional
// explicit use. Default path is PyWarnIfError (non-throwing).
inline void PyThrowIfError(Acad::ErrorStatus es, const wchar_t* ctx = L"") {
	if (es != Acad::eOk) throw PyErrorStatusException(es, ctx ? ctx : L"");
}

// Default non-throwing path: eOk -> true; otherwise print to the CAD command line
// as "[PyArx] <acadErrorStatusText> (eNum) [Context]" and return false. Never throws.
// 同时把失败错误镜像写入 G:\PyArx\log\pyengine.log（GUI acad 命令行输出不可读）。
inline bool PyWarnIfError(Acad::ErrorStatus es, const wchar_t* ctx = L"") {
	if (es == Acad::eOk) return true;
	AcString txt = acadErrorStatusText(es);
	acutPrintf(L"[PyArx] %ls (%d) [%ls]\n", txt.constPtr(), static_cast<int>(es), ctx ? ctx : L"");
	PyEngine_DbgLog(L"[PyArx] %ls (%d) [%ls]", txt.constPtr(), static_cast<int>(es), ctx ? ctx : L"");
	return false;
}
