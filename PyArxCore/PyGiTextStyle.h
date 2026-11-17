#pragma once
#include "stdafx.h"
#include "textengine.h"

// PyGiTextStyle -- value wrapper of AcGiTextStyle.
// Construct once with the same parameter order AcGiTextStyle expects, then
// pass it to PyGiTextEngine::tessellate / getExtents.
class PyGiTextStyle {
public:
    PyGiTextStyle(const std::wstring& fontFile,
                  const std::wstring& bigFont,
                  double textSize,
                  double xScale,
                  double obliqueAngle,
                  double trackingPercent,
                  bool  backward,
                  bool  upsideDown,
                  bool  vertical,
                  bool  overlined,
                  bool  underlined,
                  bool  strikethrough);
    ~PyGiTextStyle();

    PyGiTextStyle(const PyGiTextStyle&) = delete;
    PyGiTextStyle& operator=(const PyGiTextStyle&) = delete;

    AcGiTextStyle* ptr() const { return m_ts; }
    bool valid() const { return m_ts != nullptr; }

private:
    AcGiTextStyle* m_ts = nullptr;
};
