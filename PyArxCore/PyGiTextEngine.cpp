#include "stdafx.h"
#include "PyGiTextEngine.h"
#include "PyGiTextStyle.h"

namespace {

// Raw capture passed to the engine callback. a1 = number of polylines,
// a2[i] = vertex count of polyline i, pts = concatenated vertices in unit
// space (cap height = 1, baseline y = 0).
struct TextCapture {
    int                 callCount = 0;
    std::vector<int>    segCounts;
    std::vector<double> xs;
    std::vector<double> ys;
};

void TextPolylineCallback(int a1, int const* a2, AcGePoint3d const* pts, void* pVoid)
{
    TextCapture* cap = static_cast<TextCapture*>(pVoid);
    if (!cap) return;
    cap->callCount++;

    if (a1 > 0 && a1 <= 4096 && a2) {
        cap->segCounts.assign(a2, a2 + a1);
    }
    int total = 0;
    for (int n : cap->segCounts) total += n;
    if (total > 0 && total <= (1 << 20) && pts) {
        cap->xs.clear();
        cap->ys.clear();
        cap->xs.reserve(static_cast<size_t>(total));
        cap->ys.reserve(static_cast<size_t>(total));
        for (int i = 0; i < total; ++i) {
            cap->xs.push_back(pts[i].x);
            cap->ys.push_back(pts[i].y);
        }
    }
}

} // namespace

PyGiTextEngine::PyGiTextEngine()
{
    m_eng = AcGiTextEngine::create();
}

PyGiTextEngine::~PyGiTextEngine()
{
    if (m_eng) {
        delete m_eng;
        m_eng = nullptr;
    }
}

bool PyGiTextEngine::available()
{
    AcGiTextEngine* eng = AcGiTextEngine::create();
    bool ok = (eng != nullptr);
    if (eng) delete eng;
    return ok;
}

py::dict PyGiTextEngine::tessellate(PyGiTextStyle& style,
                                    const std::wstring& text,
                                    bool raw,
                                    double deviation) const
{
    py::dict out;
    if (!m_eng) { out["error"] = "AcGiTextEngine::create() returned null"; return out; }
    if (!style.ptr()) { out["error"] = "text style is null"; return out; }

    TextCapture cap;
    try {
        m_eng->tessellate(*style.ptr(),
                          text.c_str(),
                          static_cast<int>(text.size()),
                          raw,
                          deviation,
                          &cap,
                          &TextPolylineCallback);
    } catch (...) {
        out["error"] = "tessellate threw";
        return out;
    }

    py::list segs;
    int off = 0;
    const int have = static_cast<int>(cap.xs.size());
    for (int n : cap.segCounts) {
        if (n >= 2) {
            py::list pts;
            for (int k = 0; k < n && off + k < have; ++k) {
                pts.append(py::make_tuple(cap.xs[off + k], cap.ys[off + k]));
            }
            segs.append(pts);
        }
        off += n;
    }

    AcGePoint2d ext(0.0, 0.0);
    bool haveExt = false;
    try {
        m_eng->getExtents(*style.ptr(), text.c_str(), static_cast<int>(text.size()),
                          false /* bPenUp */, raw, ext);
        haveExt = true;
    } catch (...) {
        haveExt = false;
    }

    out["segments"]       = segs;
    out["segments_count"] = static_cast<int>(segs.size());
    out["points"]         = have;
    out["extents"]        = haveExt ? py::object(py::make_tuple(ext.x, ext.y)) : py::object(py::none());
    out["calls"]          = cap.callCount;
    return out;
}

py::tuple PyGiTextEngine::getExtents(PyGiTextStyle& style,
                                     const std::wstring& text,
                                     bool raw) const
{
    if (!m_eng || !style.ptr()) return py::make_tuple(0.0, 0.0);
    AcGePoint2d ext(0.0, 0.0);
    try {
        m_eng->getExtents(*style.ptr(), text.c_str(), static_cast<int>(text.size()),
                          false /* bPenUp */, raw, ext);
    } catch (...) {
        return py::make_tuple(0.0, 0.0);
    }
    return py::make_tuple(ext.x, ext.y);
}
