// Ttf2Line.cpp - text -> geometry for both SHX (AcGiTextEngine) and TTF (GDI outlines).
//
// SHX path : AcGiTextEngine::tessellate (unit coords, cap height = 1, baseline y = 0)
// TTF path : GetGlyphOutlineW(GGO_NATIVE) + quadratic flattening, normalised the same way
//
// Only usable inside an ARX host (acad.exe), because it creates AcDb entities.

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <windows.h>   // must come before the AcDb headers (packing)

#include "adesk.h"
#include "adslib.h"
#include "acdb.h"
#include "acgi.h"
#include "textengine.h"
#include "AdAChar.h"
#include "dbents.h"
#include "dbsymtb.h"
#include "dbapserv.h"
#include "dbmain.h"

#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

namespace py = pybind11;

namespace {

struct Shape {
    std::vector<int>    counts;      // points per polyline
    std::vector<double> xs, ys;      // vertices
    double advances = 0.0;           // total pen advance in normalised units
};

AcDbDatabase* curDb() { return acdbHostApplicationServices()->workingDatabase(); }

// ---------------------------------------------------------------- SHX engine

Shape shapeFromEngine(const std::wstring& font, const std::wstring& big,
                      const std::wstring& text, double deviation)
{
    Shape s;
    AcGiTextEngine* eng = AcGiTextEngine::create();
    if (!eng) return s;

    AcGiTextStyle ts(font.c_str(), big.empty() ? nullptr : big.c_str(),
                     1.0, 1.0, 0.0, 1.0,
                     Adesk::kFalse, Adesk::kFalse, Adesk::kFalse,
                     Adesk::kFalse, Adesk::kFalse, Adesk::kFalse);

    struct C {
        Shape* s;
        static void cb(int n, int const* ints, AcGePoint3d const* pts, void* v)
        {
            C* c = static_cast<C*>(v);
            if (!c || !c->s) return;
            int total = 0;
            for (int i = 0; i < n; ++i) { c->s->counts.push_back(ints[i]); total += ints[i]; }
            for (int i = 0; i < total; ++i) {
                c->s->xs.push_back(pts[i].x);
                c->s->ys.push_back(pts[i].y);
            }
        }
    };
    C ctx; ctx.s = &s;
    eng->tessellate(ts, text.c_str(), static_cast<int>(text.size()),
                    false, deviation, &ctx, &C::cb);
    delete eng;
    return s;
}

// ---------------------------------------------------------------- TTF outlines

double fixedToDouble(const FIXED& f) { return f.value + f.fract / 65536.0; }

void flattenQuad(std::vector<std::pair<double,double>>& pts,
                 double ax, double ay, double bx, double by,
                 double cx, double cy, double tol)
{
    double d = std::hypot(ax - 2.0 * bx + cx, ay - 2.0 * by + cy);
    int n = static_cast<int>(std::ceil(std::sqrt(d / (8.0 * tol))));
    if (n < 1) n = 1;
    if (n > 96) n = 96;
    for (int i = 1; i <= n; ++i) {
        double t = static_cast<double>(i) / n, u = 1.0 - t;
        pts.push_back({u * u * ax + 2 * u * t * bx + t * t * cx,
                       u * u * ay + 2 * u * t * by + t * t * cy});
    }
}

bool glyphContours(HDC hdc, wchar_t ch, double tol,
                   std::vector<std::vector<std::pair<double,double>>>& out)
{
    MAT2 mat;
    mat.eM11.value = 1; mat.eM11.fract = 0;
    mat.eM12.value = 0; mat.eM12.fract = 0;
    mat.eM21.value = 0; mat.eM21.fract = 0;
    mat.eM22.value = 1; mat.eM22.fract = 0;

    GLYPHMETRICS gm = {};
    DWORD sz = ::GetGlyphOutlineW(hdc, ch, GGO_NATIVE, &gm, 0, nullptr, &mat);
    if (sz == GDI_ERROR || sz == 0) return false;

    std::vector<BYTE> buf(sz);
    if (::GetGlyphOutlineW(hdc, ch, GGO_NATIVE, &gm, sz, buf.data(), &mat) == GDI_ERROR)
        return false;

    BYTE* p = buf.data();
    BYTE* end = p + sz;
    while (p + sizeof(TTPOLYGONHEADER) <= end) {
        TTPOLYGONHEADER* ph = reinterpret_cast<TTPOLYGONHEADER*>(p);
        if (ph->cb == 0) break;

        std::vector<std::pair<double,double>> pts;
        double curx = fixedToDouble(ph->pfxStart.x);
        double cury = fixedToDouble(ph->pfxStart.y);
        pts.push_back({curx, cury});

        BYTE* q = p + sizeof(TTPOLYGONHEADER);
        BYTE* qEnd = p + ph->cb;
        while (q + sizeof(TTPOLYCURVE) <= qEnd) {
            TTPOLYCURVE* cv = reinterpret_cast<TTPOLYCURVE*>(q);
            if (cv->wType == TT_PRIM_LINE) {
                for (int i = 0; i < cv->cpfx; ++i) {
                    curx = fixedToDouble(cv->apfx[i].x);
                    cury = fixedToDouble(cv->apfx[i].y);
                    pts.push_back({curx, cury});
                }
            } else if (cv->wType == TT_PRIM_QSPLINE) {
                for (int i = 0; i < cv->cpfx - 1; ++i) {
                    double bx = fixedToDouble(cv->apfx[i].x);
                    double by = fixedToDouble(cv->apfx[i].y);
                    double cx, cy;
                    if (i == cv->cpfx - 2) {
                        cx = fixedToDouble(cv->apfx[i + 1].x);
                        cy = fixedToDouble(cv->apfx[i + 1].y);
                    } else {
                        cx = (fixedToDouble(cv->apfx[i].x) + fixedToDouble(cv->apfx[i + 1].x)) * 0.5;
                        cy = (fixedToDouble(cv->apfx[i].y) + fixedToDouble(cv->apfx[i + 1].y)) * 0.5;
                    }
                    flattenQuad(pts, curx, cury, bx, by, cx, cy, tol);
                    curx = cx; cury = cy;
                }
            }
            q += sizeof(TTPOLYCURVE) + (cv->cpfx - 1) * sizeof(POINTFX);
        }

        if (pts.size() >= 3) out.push_back(pts);
        p += ph->cb;
    }
    return true;
}

// Build a Shape for a string drawn with a TrueType face.
// em = 1000 units, y up positive, baseline at 0; then normalised so cap height = 1.
Shape shapeFromTtf(const std::wstring& face, const std::wstring& text, double deviation)
{
    Shape s;
    HDC hdc = ::CreateCompatibleDC(nullptr);
    if (!hdc) return s;

    LOGFONTW lf = {};
    lf.lfHeight = -1000;                       // 1 em = 1000 units
    lf.lfCharSet = DEFAULT_CHARSET;
    lf.lfQuality = DEFAULT_QUALITY;
    wcsncpy_s(lf.lfFaceName, face.c_str(), LF_FACESIZE - 1);

    HFONT hf = ::CreateFontIndirectW(&lf);
    if (!hf) { ::DeleteDC(hdc); return s; }
    HGDIOBJ old = ::SelectObject(hdc, hf);
    ::SetMapMode(hdc, MM_TEXT);

    // cap height reference from 'H'
    double capH = 0.0;
    {
        MAT2 mat;
        mat.eM11.value = 1; mat.eM11.fract = 0;
        mat.eM12.value = 0; mat.eM12.fract = 0;
        mat.eM21.value = 0; mat.eM21.fract = 0;
        mat.eM22.value = 1; mat.eM22.fract = 0;
        GLYPHMETRICS gm = {};
        if (::GetGlyphOutlineW(hdc, L'H', GGO_METRICS, &gm, 0, nullptr, &mat) != GDI_ERROR)
            capH = gm.gmBlackBoxY;
        if (capH <= 0.0) capH = 700.0;
    }
    const double k = 1000.0 / capH;            // em units -> cap-height-1 units
    const double tolPx = deviation * capH;     // tolerance in em units

    double penX = 0.0;
    for (wchar_t ch : text) {
        std::vector<std::vector<std::pair<double,double>>> contours;
        if (ch == L' ') {
            MAT2 mat;
            mat.eM11.value = 1; mat.eM11.fract = 0;
            mat.eM12.value = 0; mat.eM12.fract = 0;
            mat.eM21.value = 0; mat.eM21.fract = 0;
            mat.eM22.value = 1; mat.eM22.fract = 0;
            GLYPHMETRICS gm = {};
            if (::GetGlyphOutlineW(hdc, ch, GGO_NATIVE, &gm, 0, nullptr, &mat) != GDI_ERROR)
                penX += gm.gmCellIncX * k;
            continue;
        }
        if (!glyphContours(hdc, ch, tolPx, contours)) continue;

        for (auto& c : contours) {
            if (c.size() < 2) continue;
            s.counts.push_back(static_cast<int>(c.size()));
            for (auto& p : c) {
                s.xs.push_back(penX + p.first * k);
                s.ys.push_back(p.second * k);
            }
        }

        MAT2 mat;
        mat.eM11.value = 1; mat.eM11.fract = 0;
        mat.eM12.value = 0; mat.eM12.fract = 0;
        mat.eM21.value = 0; mat.eM21.fract = 0;
        mat.eM22.value = 1; mat.eM22.fract = 0;
        GLYPHMETRICS gm = {};
        if (::GetGlyphOutlineW(hdc, ch, GGO_NATIVE, &gm, 0, nullptr, &mat) != GDI_ERROR)
            penX += gm.gmCellIncX * k;
    }

    ::SelectObject(hdc, old);
    ::DeleteObject(hf);
    ::DeleteDC(hdc);
    return s;
}

// ---------------------------------------------------------------- entity helpers

bool isTtfName(const std::wstring& f)
{
    std::wstring low;
    for (wchar_t c : f) low.push_back((c >= L'A' && c <= L'Z') ? static_cast<wchar_t>(c + 32) : c);
    if (low.size() < 4) return false;
    const std::wstring tail4 = low.substr(low.size() - 4);
    return tail4 == L".ttf" || tail4 == L".otf" || tail4 == L".ttc";
}

std::wstring faceFromFileName(const std::wstring& file)
{
    std::wstring s = file;
    size_t slash = s.find_last_of(L"\\/");
    if (slash != std::wstring::npos) s = s.substr(slash + 1);
    size_t dot = s.find_last_of(L'.');
    if (dot != std::wstring::npos) s = s.substr(0, dot);
    return s;
}

AcGePoint2d toWorld(double ux, double uy, double h, double wf, double ob,
                    double rot, const AcGePoint3d& pos)
{
    double x = ux * h * wf;
    double y = uy * h;
    x += y * std::tan(ob);
    double c = std::cos(rot), s = std::sin(rot);
    return AcGePoint2d(pos.x + x * c - y * s, pos.y + x * s + y * c);
}

} // namespace

PYBIND11_MODULE(Ttf2Line, m)
{
    m.doc() = "text -> polylines: SHX via AcGiTextEngine, TTF via GDI outlines";

    m.def("probe",
        [](std::wstring font, std::wstring text, double deviation) -> py::dict
        {
            py::dict out;
            Shape s = isTtfName(font)
                          ? shapeFromTtf(faceFromFileName(font), text, deviation)
                          : shapeFromEngine(font, L"", text, deviation);
            out["counts"]  = s.counts;
            out["xs"]      = s.xs;
            out["ys"]      = s.ys;
            out["glyphs"]  = static_cast<int>(s.counts.size());
            out["is_ttf"]  = isTtfName(font);
            return out;
        },
        py::arg("font"), py::arg("text"), py::arg("deviation") = 0.02);

    m.def("explodeTexts",
        [](double deviation, bool eraseText) -> py::dict
        {
            py::dict out;
            int texts = 0, polys = 0, verts = 0, skipped = 0, ttf = 0, shx = 0;

            std::vector<AcDbObjectId> ids;
            {
                AcDbBlockTable* pBt = nullptr;
                if (curDb()->getBlockTable(pBt, AcDb::kForRead) != Acad::eOk) { out["error"] = "block table"; return out; }
                AcDbBlockTableRecord* pMs = nullptr;
                pBt->getAt(ACDB_MODEL_SPACE, pMs, AcDb::kForRead);
                pBt->close();
                AcDbBlockTableRecordIterator* it = nullptr;
                pMs->newIterator(it);
                for (; !it->done(); it->step()) {
                    AcDbEntity* pEnt = nullptr;
                    if (it->getEntity(pEnt, AcDb::kForRead) != Acad::eOk) continue;
                    if (pEnt->isKindOf(AcDbText::desc())) ids.push_back(pEnt->objectId());
                    pEnt->close();
                }
                delete it;
                pMs->close();
            }

            AcDbBlockTable* pBt = nullptr;
            curDb()->getBlockTable(pBt, AcDb::kForRead);
            AcDbBlockTableRecord* pMsW = nullptr;
            pBt->getAt(ACDB_MODEL_SPACE, pMsW, AcDb::kForWrite);
            pBt->close();

            for (AcDbObjectId id : ids) {
                AcDbText* pT = nullptr;
                if (acdbOpenObject(pT, id, AcDb::kForRead) != Acad::eOk) continue;

                AcString txt;
                pT->textString(txt);
                std::wstring text(txt.kwszPtr());
                double h = pT->height();
                double wf = pT->widthFactor();
                double ob = pT->oblique();
                double rot = pT->rotation();
                AcGePoint3d pos = pT->position();

                std::wstring font(L"txt.shx"), big;
                AcDbTextStyleTableRecord* pRec = nullptr;
                AcDbObjectId styleId = pT->textStyle();
                if (!styleId.isNull() && acdbOpenObject(pRec, styleId, AcDb::kForRead) == Acad::eOk) {
                    AcString fn, bn;
                    pRec->fileName(fn);
                    pRec->bigFontFileName(bn);
                    if (!fn.isEmpty()) font = std::wstring(fn.kwszPtr());
                    if (!bn.isEmpty()) big = std::wstring(bn.kwszPtr());
                    pRec->close();
                }

                Shape s;
                if (isTtfName(font)) {
                    s = shapeFromTtf(faceFromFileName(font), text, deviation);
                    ++ttf;
                } else {
                    s = shapeFromEngine(font, big, text, deviation);
                    ++shx;
                }

                if (s.counts.empty()) {
                    ++skipped;
                    pT->close();
                    continue;
                }

                int off = 0;
                for (size_t pi = 0; pi < s.counts.size(); ++pi) {
                    int n = s.counts[pi];
                    bool closed = (n >= 2 &&
                                   std::fabs(s.xs[off] - s.xs[off + n - 1]) < 1e-9 &&
                                   std::fabs(s.ys[off] - s.ys[off + n - 1]) < 1e-9);
                    int keep = closed ? n - 1 : n;
                    if (keep >= 2) {
                        AcDbPolyline* pl = new AcDbPolyline(keep);
                        for (int k = 0; k < keep; ++k) {
                            AcGePoint2d p = toWorld(s.xs[off + k], s.ys[off + k], h, wf, ob, rot, pos);
                            pl->addVertexAt(k, p);
                            ++verts;
                        }
                        if (closed) pl->setClosed(true);
                        AcDbObjectId pid;
                        if (pMsW->appendAcDbEntity(pid, pl) == Acad::eOk) { ++polys; pl->close(); }
                        else delete pl;
                    }
                    off += n;
                }

                ++texts;
                if (eraseText) { pT->upgradeOpen(); pT->erase(); }
                pT->close();
            }
            pMsW->close();

            out["texts"]     = texts;
            out["polylines"] = polys;
            out["vertices"]  = verts;
            out["skipped"]   = skipped;
            out["ttf"]       = ttf;
            out["shx"]       = shx;
            return out;
        },
        py::arg("deviation") = 0.02,
        py::arg("erase_text") = false);

    m.def("version", []() { return std::string("Ttf2Line 1.0"); });
}
