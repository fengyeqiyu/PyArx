#include "stdafx.h"
#include "PyErrorStatusException.h"
#include "PyDbDimStyleTableRecord.h"

PyWrapper_Cpp(DbDimStyleTableRecord);

void bind_PyDbDimStyleTableRecord(pym& m) {
	py::class_<PyDbDimStyleTableRecord, PyDbSymbolTableRecord>(m, "PyDbDimStyleTableRecord")
		.def(py::init<>())
		.def(py::init<AcDbDimStyleTableRecord*>(), py::arg("p"))
		PyWrapper_BIND(DbDimStyleTableRecord)
		// ---- 尺寸变量 double ----
		.def("dimasz", [](PyDbDimStyleTableRecord& s){ if (!s.impObj()) return 0.0; return s.impObj()->dimasz(); })
		.def("setDimasz", [](PyDbDimStyleTableRecord& s, double v){ if (!s.impObj()) return false; return PyWarnIfError(s.impObj()->setDimasz(v), L"setDimasz"); }, py::arg("v"))
		.def("dimtxt", [](PyDbDimStyleTableRecord& s){ if (!s.impObj()) return 0.0; return s.impObj()->dimtxt(); })
		.def("setDimtxt", [](PyDbDimStyleTableRecord& s, double v){ if (!s.impObj()) return false; return PyWarnIfError(s.impObj()->setDimtxt(v), L"setDimtxt"); }, py::arg("v"))
		.def("dimscale", [](PyDbDimStyleTableRecord& s){ if (!s.impObj()) return 0.0; return s.impObj()->dimscale(); })
		.def("setDimscale", [](PyDbDimStyleTableRecord& s, double v){ if (!s.impObj()) return false; return PyWarnIfError(s.impObj()->setDimscale(v), L"setDimscale"); }, py::arg("v"))
		.def("dimexe", [](PyDbDimStyleTableRecord& s){ if (!s.impObj()) return 0.0; return s.impObj()->dimexe(); })
		.def("setDimexe", [](PyDbDimStyleTableRecord& s, double v){ if (!s.impObj()) return false; return PyWarnIfError(s.impObj()->setDimexe(v), L"setDimexe"); }, py::arg("v"))
		.def("dimexo", [](PyDbDimStyleTableRecord& s){ if (!s.impObj()) return 0.0; return s.impObj()->dimexo(); })
		.def("setDimexo", [](PyDbDimStyleTableRecord& s, double v){ if (!s.impObj()) return false; return PyWarnIfError(s.impObj()->setDimexo(v), L"setDimexo"); }, py::arg("v"))
		.def("dimgap", [](PyDbDimStyleTableRecord& s){ if (!s.impObj()) return 0.0; return s.impObj()->dimgap(); })
		.def("setDimgap", [](PyDbDimStyleTableRecord& s, double v){ if (!s.impObj()) return false; return PyWarnIfError(s.impObj()->setDimgap(v), L"setDimgap"); }, py::arg("v"))
		.def("dimdli", [](PyDbDimStyleTableRecord& s){ if (!s.impObj()) return 0.0; return s.impObj()->dimdli(); })
		.def("setDimdli", [](PyDbDimStyleTableRecord& s, double v){ if (!s.impObj()) return false; return PyWarnIfError(s.impObj()->setDimdli(v), L"setDimdli"); }, py::arg("v"))
		.def("dimdle", [](PyDbDimStyleTableRecord& s){ if (!s.impObj()) return 0.0; return s.impObj()->dimdle(); })
		.def("setDimdle", [](PyDbDimStyleTableRecord& s, double v){ if (!s.impObj()) return false; return PyWarnIfError(s.impObj()->setDimdle(v), L"setDimdle"); }, py::arg("v"))
		.def("dimrnd", [](PyDbDimStyleTableRecord& s){ if (!s.impObj()) return 0.0; return s.impObj()->dimrnd(); })
		.def("setDimrnd", [](PyDbDimStyleTableRecord& s, double v){ if (!s.impObj()) return false; return PyWarnIfError(s.impObj()->setDimrnd(v), L"setDimrnd"); }, py::arg("v"))
		.def("dimlfac", [](PyDbDimStyleTableRecord& s){ if (!s.impObj()) return 0.0; return s.impObj()->dimlfac(); })
		.def("setDimlfac", [](PyDbDimStyleTableRecord& s, double v){ if (!s.impObj()) return false; return PyWarnIfError(s.impObj()->setDimlfac(v), L"setDimlfac"); }, py::arg("v"))
		.def("dimtm", [](PyDbDimStyleTableRecord& s){ if (!s.impObj()) return 0.0; return s.impObj()->dimtm(); })
		.def("setDimtm", [](PyDbDimStyleTableRecord& s, double v){ if (!s.impObj()) return false; return PyWarnIfError(s.impObj()->setDimtm(v), L"setDimtm"); }, py::arg("v"))
		.def("dimtp", [](PyDbDimStyleTableRecord& s){ if (!s.impObj()) return 0.0; return s.impObj()->dimtp(); })
		.def("setDimtp", [](PyDbDimStyleTableRecord& s, double v){ if (!s.impObj()) return false; return PyWarnIfError(s.impObj()->setDimtp(v), L"setDimtp"); }, py::arg("v"))
		.def("dimtsz", [](PyDbDimStyleTableRecord& s){ if (!s.impObj()) return 0.0; return s.impObj()->dimtsz(); })
		.def("setDimtsz", [](PyDbDimStyleTableRecord& s, double v){ if (!s.impObj()) return false; return PyWarnIfError(s.impObj()->setDimtsz(v), L"setDimtsz"); }, py::arg("v"))
		// ---- int ----
		.def("dimdec", [](PyDbDimStyleTableRecord& s){ if (!s.impObj()) return 0; return s.impObj()->dimdec(); })
		.def("setDimdec", [](PyDbDimStyleTableRecord& s, int v){ if (!s.impObj()) return false; return PyWarnIfError(s.impObj()->setDimdec(v), L"setDimdec"); }, py::arg("v"))
		.def("dimtdec", [](PyDbDimStyleTableRecord& s){ if (!s.impObj()) return 0; return s.impObj()->dimtdec(); })
		.def("setDimtdec", [](PyDbDimStyleTableRecord& s, int v){ if (!s.impObj()) return false; return PyWarnIfError(s.impObj()->setDimtdec(v), L"setDimtdec"); }, py::arg("v"))
		.def("dimtad", [](PyDbDimStyleTableRecord& s){ if (!s.impObj()) return 0; return s.impObj()->dimtad(); })
		.def("setDimtad", [](PyDbDimStyleTableRecord& s, int v){ if (!s.impObj()) return false; return PyWarnIfError(s.impObj()->setDimtad(v), L"setDimtad"); }, py::arg("v"))
		.def("dimjust", [](PyDbDimStyleTableRecord& s){ if (!s.impObj()) return 0; return s.impObj()->dimjust(); })
		.def("setDimjust", [](PyDbDimStyleTableRecord& s, int v){ if (!s.impObj()) return false; return PyWarnIfError(s.impObj()->setDimjust(v), L"setDimjust"); }, py::arg("v"))
		.def("dimzin", [](PyDbDimStyleTableRecord& s){ if (!s.impObj()) return 0; return s.impObj()->dimzin(); })
		.def("setDimzin", [](PyDbDimStyleTableRecord& s, int v){ if (!s.impObj()) return false; return PyWarnIfError(s.impObj()->setDimzin(v), L"setDimzin"); }, py::arg("v"))
		.def("dimlunit", [](PyDbDimStyleTableRecord& s){ if (!s.impObj()) return 0; return s.impObj()->dimlunit(); })
		.def("setDimlunit", [](PyDbDimStyleTableRecord& s, int v){ if (!s.impObj()) return false; return PyWarnIfError(s.impObj()->setDimlunit(v), L"setDimlunit"); }, py::arg("v"))
		// ---- bool ----
		.def("dimtol", [](PyDbDimStyleTableRecord& s){ if (!s.impObj()) return false; return s.impObj()->dimtol(); })
		.def("setDimtol", [](PyDbDimStyleTableRecord& s, bool v){ if (!s.impObj()) return false; return PyWarnIfError(s.impObj()->setDimtol(v ? Adesk::kTrue : Adesk::kFalse), L"setDimtol"); }, py::arg("v"))
		.def("dimtih", [](PyDbDimStyleTableRecord& s){ if (!s.impObj()) return false; return s.impObj()->dimtih(); })
		.def("setDimtih", [](PyDbDimStyleTableRecord& s, bool v){ if (!s.impObj()) return false; return PyWarnIfError(s.impObj()->setDimtih(v ? Adesk::kTrue : Adesk::kFalse), L"setDimtih"); }, py::arg("v"))
		.def("dimtix", [](PyDbDimStyleTableRecord& s){ if (!s.impObj()) return false; return s.impObj()->dimtix(); })
		.def("setDimtix", [](PyDbDimStyleTableRecord& s, bool v){ if (!s.impObj()) return false; return PyWarnIfError(s.impObj()->setDimtix(v ? Adesk::kTrue : Adesk::kFalse), L"setDimtix"); }, py::arg("v"))
		// ---- ObjectId 透出 uint64 ----
		.def("dimtxstyId", [](PyDbDimStyleTableRecord& s) -> uint64_t {
			if (!s.impObj()) return 0ULL;
			auto id = s.impObj()->dimtxsty();
			return id.isNull() ? 0ULL : static_cast<uint64_t>(id.asOldId());
		}, "关联文字样式 ObjectId")
		.def("setDimtxsty", [](PyDbDimStyleTableRecord& s, uint64_t idv){
			if (!s.impObj()) return false;
			AcDbObjectId id; id.setFromOldId(static_cast<Adesk::UIntPtr>(idv));
			return PyWarnIfError(s.impObj()->setDimtxsty(id), L"setDimtxsty");
		}, py::arg("id"), "设置关联文字样式 ObjectId")
		// ---- 替代单位 dimalt 族 ----
		.def("dimalt", [](PyDbDimStyleTableRecord& s){ if (!s.impObj()) return false; return s.impObj()->dimalt(); })
		.def("setDimalt", [](PyDbDimStyleTableRecord& s, bool v){ if (!s.impObj()) return false; return PyWarnIfError(s.impObj()->setDimalt(v ? Adesk::kTrue : Adesk::kFalse), L"setDimalt"); }, py::arg("v"))
		.def("dimaltd", [](PyDbDimStyleTableRecord& s){ if (!s.impObj()) return 0; return s.impObj()->dimaltd(); })
		.def("setDimaltd", [](PyDbDimStyleTableRecord& s, int v){ if (!s.impObj()) return false; return PyWarnIfError(s.impObj()->setDimaltd(v), L"setDimaltd"); }, py::arg("v"))
		.def("dimaltf", [](PyDbDimStyleTableRecord& s){ if (!s.impObj()) return 0.0; return s.impObj()->dimaltf(); })
		.def("setDimaltf", [](PyDbDimStyleTableRecord& s, double v){ if (!s.impObj()) return false; return PyWarnIfError(s.impObj()->setDimaltf(v), L"setDimaltf"); }, py::arg("v"))
		.def("dimaltu", [](PyDbDimStyleTableRecord& s){ if (!s.impObj()) return 0; return s.impObj()->dimaltu(); })
		.def("setDimaltu", [](PyDbDimStyleTableRecord& s, int v){ if (!s.impObj()) return false; return PyWarnIfError(s.impObj()->setDimaltu(v), L"setDimaltu"); }, py::arg("v"))
		.def("dimalttd", [](PyDbDimStyleTableRecord& s){ if (!s.impObj()) return 0; return s.impObj()->dimalttd(); })
		.def("setDimalttd", [](PyDbDimStyleTableRecord& s, int v){ if (!s.impObj()) return false; return PyWarnIfError(s.impObj()->setDimalttd(v), L"setDimalttd"); }, py::arg("v"))
		.def("dimaltz", [](PyDbDimStyleTableRecord& s){ if (!s.impObj()) return 0; return s.impObj()->dimaltz(); })
		.def("setDimaltz", [](PyDbDimStyleTableRecord& s, int v){ if (!s.impObj()) return false; return PyWarnIfError(s.impObj()->setDimaltz(v), L"setDimaltz"); }, py::arg("v"))
		.def("dimaltrnd", [](PyDbDimStyleTableRecord& s){ if (!s.impObj()) return 0.0; return s.impObj()->dimaltrnd(); })
		.def("setDimaltrnd", [](PyDbDimStyleTableRecord& s, double v){ if (!s.impObj()) return false; return PyWarnIfError(s.impObj()->setDimaltrnd(v), L"setDimaltrnd"); }, py::arg("v"))
		// ---- 箭头块 ObjectId ----
		.def("dimblkId", [](PyDbDimStyleTableRecord& s) -> uint64_t {
			if (!s.impObj()) return 0ULL;
			auto id = s.impObj()->dimblk(); return id.isNull() ? 0ULL : (uint64_t)id.asOldId();
		}, "箭头块 ObjectId")
		.def("setDimblk", [](PyDbDimStyleTableRecord& s, uint64_t idv){
			if (!s.impObj()) return false;
			AcDbObjectId id; id.setFromOldId((Adesk::UIntPtr)idv);
			return PyWarnIfError(s.impObj()->setDimblk(id), L"setDimblk");
		}, py::arg("id"))
		.def("dimblk1Id", [](PyDbDimStyleTableRecord& s) -> uint64_t {
			if (!s.impObj()) return 0ULL;
			auto id = s.impObj()->dimblk1(); return id.isNull() ? 0ULL : (uint64_t)id.asOldId();
		})
		.def("dimblk2Id", [](PyDbDimStyleTableRecord& s) -> uint64_t {
			if (!s.impObj()) return 0ULL;
			auto id = s.impObj()->dimblk2(); return id.isNull() ? 0ULL : (uint64_t)id.asOldId();
		})
		// ---- arrowId(AcDb::DimArrowFlags) -> ObjectId 透出 uint64 ----
		.def("arrowId", [](PyDbDimStyleTableRecord& s, int whichArrow) -> uint64_t {
			if (!s.impObj()) return 0ULL;
			auto id = s.impObj()->arrowId(static_cast<AcDb::DimArrowFlags>(whichArrow));
			return id.isNull() ? 0ULL : static_cast<uint64_t>(id.asOldId());
		}, py::arg("whichArrow"), "按箭头标识取箭头块 ObjectId（AcDb::DimArrowFlags）")
		// ---- ACHAR*& 出参（内部缓存持有，调用方不释放）-> AcString ----
		.def("getDimapost", [](PyDbDimStyleTableRecord& s) -> AcString {
			if (!s.impObj()) return AcString();
			ACHAR* p = nullptr;
			if (!PyWarnIfError(s.impObj()->getDimapost(p), L"getDimapost")) return AcString();
			return p ? AcString(p) : AcString();
		})
		.def("getDimpost", [](PyDbDimStyleTableRecord& s) -> AcString {
			if (!s.impObj()) return AcString();
			ACHAR* p = nullptr;
			if (!PyWarnIfError(s.impObj()->getDimpost(p), L"getDimpost")) return AcString();
			return p ? AcString(p) : AcString();
		})
		.def("getDimblk", [](PyDbDimStyleTableRecord& s) -> AcString {
			if (!s.impObj()) return AcString();
			ACHAR* p = nullptr;
			if (!PyWarnIfError(s.impObj()->getDimblk(p), L"getDimblk")) return AcString();
			return p ? AcString(p) : AcString();
		})
		.def("getDimblk1", [](PyDbDimStyleTableRecord& s) -> AcString {
			if (!s.impObj()) return AcString();
			ACHAR* p = nullptr;
			if (!PyWarnIfError(s.impObj()->getDimblk1(p), L"getDimblk1")) return AcString();
			return p ? AcString(p) : AcString();
		})
		.def("getDimblk2", [](PyDbDimStyleTableRecord& s) -> AcString {
			if (!s.impObj()) return AcString();
			ACHAR* p = nullptr;
			if (!PyWarnIfError(s.impObj()->getDimblk2(p), L"getDimblk2")) return AcString();
			return p ? AcString(p) : AcString();
		})
		// ---- bool 查询 ----
		.def("isModifiedForRecompute", [](PyDbDimStyleTableRecord& s) {
			if (!s.impObj()) return false;
			return s.impObj()->isModifiedForRecompute();
		})
		// ---- setter -> bool ----
		.def("setDimfit", [](PyDbDimStyleTableRecord& s, int fit) {
			if (!s.impObj()) return false;
			return PyWarnIfError(s.impObj()->setDimfit(fit), L"setDimfit");
		}, py::arg("fit"))
		.def("setDimunit", [](PyDbDimStyleTableRecord& s, int unit) {
			if (!s.impObj()) return false;
			return PyWarnIfError(s.impObj()->setDimunit(unit), L"setDimunit");
		}, py::arg("unit"))
		;
}
