#include "stdafx.h"
#include "PyGiTextStyle.h"

PyGiTextStyle::PyGiTextStyle(const std::wstring& fontFile,
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
                             bool  strikethrough)
{
    m_ts = new AcGiTextStyle(
        fontFile.empty() ? nullptr : fontFile.c_str(),
        bigFont.empty()  ? nullptr : bigFont.c_str(),
        textSize,
        xScale,
        obliqueAngle,
        trackingPercent,
        backward     ? Adesk::kTrue : Adesk::kFalse,
        upsideDown   ? Adesk::kTrue : Adesk::kFalse,
        vertical     ? Adesk::kTrue : Adesk::kFalse,
        overlined    ? Adesk::kTrue : Adesk::kFalse,
        underlined   ? Adesk::kTrue : Adesk::kFalse,
        strikethrough? Adesk::kTrue : Adesk::kFalse);
}

PyGiTextStyle::~PyGiTextStyle()
{
    if (m_ts) {
        delete m_ts;
        m_ts = nullptr;
    }
}
