#pragma once
#include "stdafx.h"
#include "textengine.h"

class PyGiTextStyle;

// PyGiTextEngine -- wrapper of AcGiTextEngine (SHX / TTF text vectorization).
// Returns unit-space polylines: cap height = 1, baseline y = 0, +X advance.
// Segments are grouped by the engine's per-polyline point counts, so callers
// get real stroke polylines instead of one concatenated point soup.
class PyGiTextEngine {
public:
    PyGiTextEngine();
    ~PyGiTextEngine();

    PyGiTextEngine(const PyGiTextEngine&) = delete;
    PyGiTextEngine& operator=(const PyGiTextEngine&) = delete;

    // True when AcGiTextEngine::create() succeeds in this host process.
    static bool available();

    bool valid() const { return m_eng != nullptr; }

    // {segments: [[(x, y), ...], ...], extents: (w, h), points: n, segments_count: n}
    py::dict tessellate(PyGiTextStyle& style,
                        const std::wstring& text,
                        bool raw,
                        double deviation) const;

    // (width, height) of the laid-out text in the same unit space.
    py::tuple getExtents(PyGiTextStyle& style,
                         const std::wstring& text,
                         bool raw) const;

private:
    AcGiTextEngine* m_eng = nullptr;
};
