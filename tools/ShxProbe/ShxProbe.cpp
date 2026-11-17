// ShxProbe.cpp - minimal pyd that calls AcGiTextEngine::tessellate inside an ARX process.
//
// Goals:
//   1) verify AcGiTextEngine::create() is usable in the host process
//   2) find out what the 2nd parameter (int const*) of PolylineCallback really means
//   3) dump real polylines of txt.shx (unit coords: cap height = 1, baseline y = 0, +X advance)
//
// Usage (only inside CAD / a host with acdb initialized):
//   import ShxProbe
//   r = ShxProbe.tessellate(r"C:\...\Fonts\txt.shx", "", "AOB0g", 1.0, 0.05)
//   r["int_arg"], r["ints_head"], r["point_count"]

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "adesk.h"
#include "adslib.h"   // ads_point / ads_real, needed by gemat3d.h
#include "acdb.h"
#include "acgi.h"
#include "textengine.h"
#include "AdAChar.h"
#include "dbents.h"
#include "dbsymtb.h"
#include "dbapserv.h"   // acdbHostApplicationServices()
#include "dbmain.h"

#include <cmath>
#include <string>
#include <vector>

namespace py = pybind11;

namespace {

struct Capture {
    int                 callCount = 0;
    int                 nArg      = -1;
    int                 nArgMax   = -1;
    int                 nArgsAll[8] = {0,0,0,0,0,0,0,0};
    unsigned long long  a2addr    = 0;
    unsigned long long  ptAddr    = 0;
    std::vector<int>    ints;
    int                 intsLen   = 0;
    std::vector<double> xs, ys, zs;
    int                 pointTotal = 0;
    bool                readData  = false;   // stage 0 = metadata only (100% safe)
};

// Record raw arguments only; interpretation is left to the Python side.
void ProbeCallback(int a1, int const* a2, AcGePoint3d const* pts, void* pVoid)
{
    Capture* cap = static_cast<Capture*>(pVoid);
    if (!cap) return;
    if (cap->callCount == 0) {
        cap->nArg  = a1;
        cap->a2addr = reinterpret_cast<unsigned long long>(a2);
        cap->ptAddr = reinterpret_cast<unsigned long long>(pts);
    }
    if (cap->callCount < 8) cap->nArgsAll[cap->callCount] = a1;
    if (a1 > cap->nArgMax) cap->nArgMax = a1;
    cap->callCount++;
    if (!cap->readData) return;

    // Hypothesis: a1 = number of polylines, a2[i] = point count of polyline i,
    // pts = concatenated vertices. Read both, clamped so we never run wild.
    int nInts = (a1 > 0 && a1 <= 4096) ? a1 : 0;
    if (nInts > 0 && a2) {
        cap->ints.reserve(nInts);
        for (int i = 0; i < nInts; ++i) cap->ints.push_back(a2[i]);
        cap->intsLen = nInts;
    }

    int total = 0;
    for (int i = 0; i < cap->intsLen; ++i) total += cap->ints[i];
    if (total > 0 && total <= 4096 && pts) {
        cap->xs.reserve(total);
        cap->ys.reserve(total);
        cap->zs.reserve(total);
        for (int i = 0; i < total; ++i) {
            cap->xs.push_back(pts[i].x);
            cap->ys.push_back(pts[i].y);
            cap->zs.push_back(pts[i].z);
        }
    }
    cap->pointTotal = total;
}

} // namespace

namespace {

AcDbDatabase* curDb()
{
    return acdbHostApplicationServices()->workingDatabase();
}

// Create (or reuse) a text style record that points at the given SHX files.
AcDbObjectId ensureShxStyle(const std::wstring& font, const std::wstring& big)
{
    const ACHAR* kName = L"PYARXSHX";
    AcDbTextStyleTable* pTab = nullptr;
    if (curDb()->getSymbolTable(pTab, AcDb::kForRead) != Acad::eOk)
        return AcDbObjectId::kNull;

    AcDbObjectId id;
    if (pTab->has(kName) && pTab->getAt(kName, id) == Acad::eOk) {
        pTab->close();
        return id;
    }

    pTab->upgradeOpen();
    AcDbTextStyleTableRecord* pRec = new AcDbTextStyleTableRecord();
    pRec->setName(kName);
    if (!font.empty()) pRec->setFileName(font.c_str());
    if (!big.empty())  pRec->setBigFontFileName(big.c_str());
    pRec->setTextSize(0.0);
    pRec->setXScale(1.0);

    Acad::ErrorStatus es = pTab->add(id, pRec);
    pRec->close();
    pTab->close();
    return (es == Acad::eOk) ? id : AcDbObjectId::kNull;
}

bool addOneText(const std::wstring& text, double height, double x, double y,
                double rotDeg, double widthFactor,
                const std::wstring& font, const std::wstring& big)
{
    AcDbObjectId styleId = ensureShxStyle(font, big);
    if (styleId.isNull()) return false;

    AcDbBlockTable* pBt = nullptr;
    if (curDb()->getBlockTable(pBt, AcDb::kForRead) != Acad::eOk) return false;
    AcDbBlockTableRecord* pMs = nullptr;
    if (pBt->getAt(ACDB_MODEL_SPACE, pMs, AcDb::kForWrite) != Acad::eOk) {
        pBt->close();
        return false;
    }
    pBt->close();

    AcDbText* pTxt = new AcDbText();
    pTxt->setPosition(AcGePoint3d(x, y, 0.0));
    pTxt->setTextString(text.c_str());
    pTxt->setHeight(height);
    pTxt->setRotation(rotDeg * 3.14159265358979 / 180.0);
    pTxt->setWidthFactor(widthFactor);
    pTxt->setTextStyle(styleId);
    pTxt->setHorizontalMode(AcDb::kTextLeft);
    pTxt->setVerticalMode(AcDb::kTextBase);

    AcDbObjectId id;
    Acad::ErrorStatus es = pMs->appendAcDbEntity(id, pTxt);
    pTxt->close();
    pMs->close();
    return (es == Acad::eOk);
}

// Unit-coordinate vertex -> world point for a given text entity.
AcGePoint2d toWorld(double ux, double uy, double height, double widthFactor,
                    double oblique, double rotation, const AcGePoint3d& pos)
{
    double x = ux * height * widthFactor;
    double y = uy * height;
    x += y * std::tan(oblique);
    double c = std::cos(rotation), s = std::sin(rotation);
    return AcGePoint2d(pos.x + x * c - y * s, pos.y + x * s + y * c);
}

} // namespace

PYBIND11_MODULE(ShxProbe, m)
{
    m.doc() = "AcGiTextEngine::tessellate probe (import inside an ARX process)";

    m.def("create", []() -> bool {
        AcGiTextEngine* eng = AcGiTextEngine::create();
        bool ok = (eng != nullptr);
        delete eng;
        return ok;
    }, "check whether AcGiTextEngine::create() works");

    m.def("tessellate",
        [](std::wstring fontFile, std::wstring bigFont, std::wstring text,
           double textSize, double deviation, bool raw, int stage) -> py::dict
        {
            py::dict out;
            std::string step = "create";
            try {
                AcGiTextEngine* eng = AcGiTextEngine::create();
                if (!eng) {
                    out["error"] = "create() returned null";
                    return out;
                }

                step = "cstyle";
                AcGiTextStyle ts(
                    fontFile.empty() ? nullptr : fontFile.c_str(),
                    bigFont.empty()  ? nullptr : bigFont.c_str(),
                    textSize,
                    1.0,            // xScale
                    0.0,            // obliqueAngle
                    1.0,            // trackingPercent
                    Adesk::kFalse,  // backward
                    Adesk::kFalse,  // upsideDown
                    Adesk::kFalse,  // vertical
                    Adesk::kFalse,  // overlined
                    Adesk::kFalse,  // underlined
                    Adesk::kFalse   // strikethrough
                );

                step = "extents";
                AcGePoint2d ext(0.0, 0.0);
                bool haveExt = false;
                try {
                    eng->getExtents(ts, text.c_str(), static_cast<int>(text.size()),
                                    false, raw, ext);
                    haveExt = true;
                } catch (...) {
                    haveExt = false;
                }

                step = "tessellate";
                Capture cap;
                cap.readData = (stage >= 1);
                try {
                    eng->tessellate(ts, text.c_str(), static_cast<int>(text.size()),
                                    raw, deviation, &cap, &ProbeCallback);
                } catch (...) {
                    out["failed_at"] = step;
                    out["note"]      = "tessellate threw";
                    delete eng;
                    return out;
                }

                int nonZero = 0;
                for (int i : cap.ints) if (i != 0) ++nonZero;

                out["stage"]       = stage;
                out["call_count"]  = cap.callCount;
                out["int_arg"]     = cap.nArg;
                out["int_arg_max"] = cap.nArgMax;
                out["int_args"]    = std::vector<int>(cap.nArgsAll, cap.nArgsAll + 8);
                out["a2_addr"]     = cap.a2addr;
                out["pts_addr"]    = cap.ptAddr;
                out["ints_head"]   = cap.ints;
                out["ints_len"]    = cap.intsLen;
                out["non_zero_ints"] = nonZero;
                out["point_count"] = static_cast<int>(cap.xs.size());
                out["point_total"] = cap.pointTotal;
                out["x"] = cap.xs;
                out["y"] = cap.ys;
                out["z"] = cap.zs;
                if (haveExt) {
                    out["extents"] = py::make_tuple(ext.x, ext.y);
                } else {
                    out["extents"] = py::none();
                }
                delete eng;
                return out;
            } catch (...) {
                out["failed_at"] = step;
                out["note"]      = "exception at step above";
                return out;
            }
        },
        py::arg("font_file"),
        py::arg("big_font")  = "",
        py::arg("text"),
        py::arg("text_size") = 1.0,
        py::arg("deviation") = 0.05,
        py::arg("raw")       = false,
        py::arg("stage")     = 0);

    // Create a TEXT entity in model space (style PYARXSHX -> txt.shx + gbcbig.shx by default).
    m.def("addText",
        [](std::wstring text, double height, double x, double y,
           double rotDeg, double widthFactor,
           std::wstring font, std::wstring big) -> bool
        {
            return addOneText(text, height, x, y, rotDeg, widthFactor, font, big);
        },
        py::arg("text"),
        py::arg("height")       = 5.0,
        py::arg("x")            = 0.0,
        py::arg("y")            = 0.0,
        py::arg("rotation_deg") = 0.0,
        py::arg("width_factor") = 1.0,
        py::arg("font")         = "txt.shx",
        py::arg("big_font")     = "gbcbig.shx");

    // Convert every AcDbText in model space into AcDbPolylines (text -> geometry).
    m.def("explodeTexts",
        [](double deviation, bool eraseText, bool keepDuplicateEnd) -> py::dict
        {
            py::dict out;
            int texts = 0, polys = 0, verts = 0;
            std::vector<std::wstring> log;

            AcGiTextEngine* eng = AcGiTextEngine::create();
            if (!eng) {
                out["error"] = "create() returned null";
                return out;
            }

            // collect text ids first
            std::vector<AcDbObjectId> ids;
            {
                AcDbBlockTable* pBt = nullptr;
                if (curDb()->getBlockTable(pBt, AcDb::kForRead) != Acad::eOk) { delete eng; out["error"] = "block table"; return out; }
                AcDbBlockTableRecord* pMs = nullptr;
                if (pBt->getAt(ACDB_MODEL_SPACE, pMs, AcDb::kForRead) != Acad::eOk) { pBt->close(); delete eng; out["error"] = "model space"; return out; }
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

                AcString txtStr;
                pT->textString(txtStr);          // avoids the allocating deprecated overload
                std::wstring txt(txtStr.kwszPtr());
                double h  = pT->height();
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

                AcGiTextStyle ts(
                    font.c_str(), big.empty() ? nullptr : big.c_str(),
                    1.0, 1.0, 0.0, 1.0,
                    Adesk::kFalse, Adesk::kFalse, Adesk::kFalse,
                    Adesk::kFalse, Adesk::kFalse, Adesk::kFalse);

                Capture cap;
                cap.readData = true;
                bool okCall = true;
                try {
                    eng->tessellate(ts, txt.c_str(), static_cast<int>(txt.size()),
                                    false, deviation, &cap, &ProbeCallback);
                } catch (...) {
                    okCall = false;
                }
                if (!okCall || cap.ints.empty()) {
                    log.push_back(L"skip: " + txt);
                    pT->close();
                    continue;
                }

                int off = 0;
                for (size_t pi = 0; pi < cap.ints.size(); ++pi) {
                    int n = cap.ints[pi];
                    bool closed = false;
                    if (n >= 2 && off + n <= static_cast<int>(cap.xs.size())) {
                        closed = (std::fabs(cap.xs[off] - cap.xs[off + n - 1]) < 1e-9 &&
                                  std::fabs(cap.ys[off] - cap.ys[off + n - 1]) < 1e-9);
                    }
                    if (n >= 2) {
                        int keep = (closed && !keepDuplicateEnd) ? n - 1 : n;
                        AcDbPolyline* pl = new AcDbPolyline(keep);
                        for (int k = 0; k < keep; ++k) {
                            AcGePoint2d p = toWorld(cap.xs[off + k], cap.ys[off + k],
                                                    h, wf, ob, rot, pos);
                            pl->addVertexAt(k, p);
                            ++verts;
                        }
                        if (closed) pl->setClosed(true);
                        AcDbObjectId pid;
                        if (pMsW->appendAcDbEntity(pid, pl) == Acad::eOk) { ++polys; pl->close(); }
                        else { delete pl; }
                    }
                    off += n;
                }
                ++texts;
                if (eraseText) {
                    pT->upgradeOpen();
                    pT->erase();
                }
                pT->close();
            }
            pMsW->close();
            delete eng;

            out["texts"]     = texts;
            out["polylines"] = polys;
            out["vertices"]  = verts;
            out["skipped"]   = static_cast<int>(log.size());
            return out;
        },
        py::arg("deviation")           = 0.1,
        py::arg("erase_text")          = false,
        py::arg("keep_duplicate_end")  = false);

    m.def("version", []() { return std::string("ShxProbe 1.1"); });
}
