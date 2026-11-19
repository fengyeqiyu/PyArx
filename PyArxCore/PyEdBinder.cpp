// PyEd.cpp - PyArx.Ed sub-module binder.
// After the PyEdFuncs split, every wrapper definition lives in a per-family
// slice (PyEdFunc / PyEdUcsVp / PyEdInput / PyEdSel / PyEdEnt / PyEdUtil /
// PyEdPrompt); bind_PyEd 为 PyArx.Ed 子模块注册函数（由 PyArxBinder.cpp 调用），
// registers all functions/classes under their unchanged Python names.
// First line must be stdafx.h.
#include "stdafx.h"
#include "PyEdBinder.h"
#include "PyErrorStatusException.h"
#include "PyResbuf.h"
#include "PyUtils.h"
#include "PyEdJig.h"
#include "PyAdsName.h"
#include "PyEdSelectionSets.h"
#include "PyDbTypeCaster.h"
#include "PyEdEnt.h"
#include "PyEdSel.h"
#include <acedCmdNF.h>

namespace py = pybind11;

//-----------------------------------------------------------------------------
// PyAdsName / SelectionSet 对象封装绑定（PyAdsName.h / PyEdSelectionSets.h）
//-----------------------------------------------------------------------------
namespace {

// DxfValue → Python 对象（TVList 转 list[(code, value)] 用）
py::object DxfValueToPy(const DxfValue& v) {
	switch (v.getType()) {
		case DxfValueType::kNull:   return py::none();
		case DxfValueType::kInt8:   return py::int_(v.asInt8());
		case DxfValueType::kInt16:  return py::int_(v.asInt16());
		case DxfValueType::kInt32:  return py::int_(v.asInt32());
		case DxfValueType::kInt64:  return py::int_(v.asInt64());
		case DxfValueType::kDouble: return py::float_(v.asDouble());
		case DxfValueType::kString: return py::cast(v.asString());
		case DxfValueType::kPoint2d: { auto p = v.asPoint2d(); return py::make_tuple(p[0], p[1]); }
		case DxfValueType::kPoint3d: { auto p = v.asPoint3d(); return py::make_tuple(p[0], p[1], p[2]); }
		case DxfValueType::kAdsName: return py::cast(v.asAdsName());
		case DxfValueType::kBinary:
			return py::bytes(std::string(v.asBinary().data(), v.asBinary().size()));
		case DxfValueType::kHandle: {
			auto h = v.asHandle();
			return py::bytes(std::string(reinterpret_cast<const char*>(h.data()), h.size()));
		}
		default: return py::none();
	}
}

py::list TVListToPyList(const TVList& tvs) {
	py::list out;
	for (const auto& tv : tvs)
		out.append(py::make_tuple(tv.code, DxfValueToPy(tv.value)));
	return out;
}

} // namespace

void bind_PyAdsName(py::module& m) {
	py::class_<PyAdsName>(m, "AdsName",
		"ads_name 实体名封装（AutoCAD 实体选择名，即 (int64, int64) 对）")
		.def(py::init<>(), "空名称 (0,0)")
		.def(py::init<const PyAdsName&>(), "拷贝构造")
		.def(py::init([](uint64_t a, uint64_t b) {
			PyAdsName n; n[0] = (int64_t)a; n[1] = (int64_t)b; return n;
		}), py::arg("a"), py::arg("b"), "从 (a, b) 两个整数构造")
		.def(py::init([](const AcDbObjectId& id) {
			return PyAdsName(id);
		}), py::arg("object_id"), "从 AcDbObjectId 构造（失败时为空名称）")
		.def_static("fromObjectId", [](const AcDbObjectId& id) {
			PyAdsName out;
			bool ok = PyAdsName::fromObjectId(id, out);
			return py::make_tuple(ok, out);
		}, py::arg("object_id"), "静态工厂：(是否成功, AdsName)")
		.def("data", [](const PyAdsName& n) {
			return py::make_tuple((uint64_t)n[0], (uint64_t)n[1]);
		}, "底层 (a, b) 两整数")
		.def("__getitem__", [](const PyAdsName& n, int i) { return (uint64_t)n[i]; }, py::arg("i"))
		.def("__setitem__", [](PyAdsName& n, int i, uint64_t v) { n[i] = (int64_t)v; }, py::arg("i"), py::arg("v"))
		.def("__len__", [](const PyAdsName&) { return 2; })
		.def("isEmpty", &PyAdsName::isEmpty, "是否为全 0 空名称")
		.def("clear", &PyAdsName::clear, "清零")
		.def("toObjectId", [](const PyAdsName& n) {
			return n.toObjectId();
		}, "转 AcDbObjectId（无效返回 0）")
		.def("isValid", &PyAdsName::isValid, "能否成功转回非空 ObjectId")
		.def("__str__", [](const PyAdsName& n) { return n.toString(); })
		.def("__repr__", [](const PyAdsName& n) { return n.toString(); })
		.def("__eq__", &PyAdsName::operator==, py::is_operator())
		.def("__ne__", &PyAdsName::operator!=, py::is_operator())
		.def("__lt__", &PyAdsName::operator<, py::is_operator())
		.def("__hash__", [](const PyAdsName& n) {
			return (size_t)(((uint64_t)n[0] * 31) ^ (uint64_t)n[1]);
		});
}

void bind_PySelectionSet(py::module& m) {
	py::class_<PyEdSelectionSets>(m, "SelectionSet",
		"AutoCAD 选择集封装（拥有选择集：析构时自动 acedSSFree 释放）")
		.def(py::init<>(), "空选择集")
		.def(py::init([](uint64_t a, uint64_t b) {
			ads_name ss = { (int64_t)a, (int64_t)b };
			return PyEdSelectionSets(ss);
		}), py::arg("a"), py::arg("b"), "从现有选择集 (a, b) 标识构造（注意：析构会释放该集）")
		.def("__len__", &PyEdSelectionSets::count, "选择集元素个数")
		.def("count", &PyEdSelectionSets::count, "元素个数")
		.def("isEmpty", &PyEdSelectionSets::isEmpty)
		.def("clear", &PyEdSelectionSets::clear, "清空并释放")
		.def("getEntNameAt", &PyEdSelectionSets::getEntNameAt, py::arg("index"),
			"第 index 个元素的 AdsName")
		.def("getResbufAt", [](const PyEdSelectionSets& ss, size_t i) {
			return TVListToPyList(ss.getResbufAt(i).toList());
		}, py::arg("index"), "第 index 个元素的 DXF 数据列表 [(code, value), ...]")
		.def("getObjectIdAt", [](const PyEdSelectionSets& ss, size_t i) {
			return ss.getObjectIdAt(i);
		}, py::arg("index"), "第 index 个元素的 ObjectId")
		.def("add", &PyEdSelectionSets::add, py::arg("object_id"), "添加实体到选择集")
		.def("remove", &PyEdSelectionSets::remove, py::arg("object_id"), "从选择集删除实体")
		.def("hasMember", &PyEdSelectionSets::hasMember, py::arg("object_id"), "是否包含实体")
		.def("subentName", [](const PyEdSelectionSets& ss, size_t ei, size_t si) {
			AcDbFullSubentPath p = ss.subentName(ei, si);
			AcDbObjectIdArray ids; p.objectIds(ids);
			py::list idl;
			for (auto& id : ids) idl.append(id);
			auto sid = p.subentId();
			return py::make_tuple(idl, (int)sid.type(), (uint64_t)sid.index());
		}, py::arg("ent_index"), py::arg("subent_index"),
			"子实体路径：(object_id_list, subent_type, subent_index)")
		.def("subentNameX", [](const PyEdSelectionSets& ss, size_t ei, size_t si, int flags) {
			return TVListToPyList(ss.subentNameX(ei, si, flags));
		}, py::arg("ent_index"), py::arg("subent_index"), py::arg("flags"),
			"子实体扩展查询：[(code, value), ...]")
		.def("setFirst", &PyEdSelectionSets::ssSetFirst, "设为首选集（同时清除上次首选集）")
		.def("xform", &PyEdSelectionSets::ssXform, py::arg("matrix"), "对选择集应用变换矩阵")
		.def("nameX", [](const PyEdSelectionSets& ss, int idx) {
			return TVListToPyList(ss.ssNameX(idx));
		}, py::arg("idx") = 0, "SSNameX 结果：[(code, value), ...]")
		.def("objectIds", [](const PyEdSelectionSets& ss) {
			py::list out;
			for (auto& id : ss.objectIds())
				out.append(id);
			return out;
		}, "全部元素的 ObjectId 列表")
		.def("__str__", [](const PyEdSelectionSets& ss) { return ss.toString(); })
		.def("__repr__", [](const PyEdSelectionSets& ss) { return ss.toString(); })
		.def("__iter__", [](const PyEdSelectionSets& ss) {
			py::list out;
			for (auto& id : ss.objectIds())
				out.append(id);
			return py::iter(out);
		}, "迭代 ObjectId 列表");
}

//-----------------------------------------------------------------------------
// pybind11 embedded module
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// 统一交互结果承载：Python 侧单一 PyEdResult 类（value 类型擦除，status 为
// PromptStatus）。C++ 侧仍为强类型模板 PyEdResult<T>（value + status +
// operator T()），此处做类型擦除以便一个 Python 类容纳不同 value。
//-----------------------------------------------------------------------------
struct PyPromptResult {
    py::object value = py::none();
    PromptStatus status = PromptStatus::kError;
};

template<typename T>
py::object PyValueToObject(T&& v) { return py::cast(std::forward<T>(v)); }

template<typename T>
PyPromptResult wrapPromptResult(PyEdResult<T>&& r) {
    PyPromptResult out;
    out.status = r.status;
    out.value = (r.status == PromptStatus::kOK) ? PyValueToObject(std::move(r.value)) : py::none();
    return out;
}

void bind_PyPromptResult(pym& m) {
    py::class_<PyPromptResult>(m, "PyEdResult",
        "交互结果：value 为具体值（失败/取消为 None），status 为 PromptStatus；"
        "支持 int()/float()/bool() 取用（对应 C++ operator T() 隐式转换）")
        .def_readonly("value", &PyPromptResult::value)
        .def_readonly("status", &PyPromptResult::status)
        .def_property_readonly("isOk", [](const PyPromptResult& r) { return r.status == PromptStatus::kOK; })
        .def_property_readonly("isCanceled", [](const PyPromptResult& r) { return r.status == PromptStatus::kCancel; })
        .def_property_readonly("isModeless", [](const PyPromptResult& r) { return r.status == PromptStatus::kModeless; })
        .def_property_readonly("isError", [](const PyPromptResult& r) { return r.status == PromptStatus::kError; })
        .def_property_readonly("isNone", [](const PyPromptResult& r) { return r.status == PromptStatus::kNone; })
        .def_property_readonly("isOther", [](const PyPromptResult& r) { return r.status == PromptStatus::kOther; })
        .def_property_readonly("isKeyWord", [](const PyPromptResult& r) { return r.status == PromptStatus::kKeyword; })
        .def("__int__", [](const PyPromptResult& r) {
            if (py::isinstance<py::int_>(r.value)) return r.value.cast<int>();
            if (py::isinstance<py::float_>(r.value)) return (int)r.value.cast<double>();
            return 0;
        })
        .def("__float__", [](const PyPromptResult& r) {
            if (py::isinstance<py::int_>(r.value)) return (double)r.value.cast<int>();
            if (py::isinstance<py::float_>(r.value)) return r.value.cast<double>();
            return 0.0;
        })
        .def("__bool__", [](const PyPromptResult& r) { return r.status == PromptStatus::kOK; })
        .def("__str__", [](const PyPromptResult& r) { return py::str(r.value); })
        .def("__repr__", [](const PyPromptResult& r) {
            return py::str("PyEdResult(status={}, value={})").format(py::cast(r.status), r.value);
        });
}

void bind_PyEd(pym& m) {
    m.doc() = "AutoCAD aced* editor global functions";
    py::module_::import("PyArxCore.Ge"); // AcGePoint3d / AcGeMatrix3d type_info 须在 PyArxCore.Ge 子模块中已注册

    // PyAdsName / SelectionSet 对象封装必须先注册：其后 acedSS*/acdbEnt* 的
    // 默认参数与返回值都引用这两个类型；顺序错误会抛
    // "could not convert default argument 'ename: PyAdsName' ... type not registered yet"
    bind_PyAdsName(m);
    bind_PySelectionSet(m);

    // non-interactive
    m.def("acedPrompt", &PyEdPrompt, py::arg("str"));
    m.def("acedAlert", &PyEdAlert, py::arg("str"));
    m.def("acedGetVar", &PyEdGetVar, py::arg("sym"));
    m.def("acedSetVar", &PyEdSetVar, py::arg("sym"), py::arg("value"));
    m.def("acedGetEnv", &PyEdGetEnv, py::arg("sym"));
    m.def("acedSetEnv", &PyEdSetEnv, py::arg("sym"), py::arg("val"));
    m.def("acedFindFile", &PyEdFindFile, py::arg("fname"));
    m.def("acedFindTrustedFile", &PyEdFindTrustedFile, py::arg("fname"),
        "在受信任路径搜索文件，未找到返回空串，等价 acedFindTrustedFile。");
    m.def("acedInvoke", &PyEdInvoke, py::arg("args"),
        "调用 LISP 函数，args 为 TvList（首项 TypedValue(RTSTR, 函数名)，其余参数）；失败 None，等价 acedInvoke。");
#ifdef PyEdModule  // 交互：文件选择对话框（GUI 弹模态框，无头无显示通道）
    m.def("acedGetFileD", &PyEdGetFileD, py::arg("title"), py::arg("def_name") = L"",
        py::arg("ext") = L"", py::arg("flags") = 0,
        "文件选择对话框；取消返回空串，等价 acedGetFileD。");
#endif
    m.def("acedGetSym", &PyEdGetSym, py::arg("name"), "读 LISP 符号值，失败 None，等价 acedGetSym。");
    m.def("acedPutSym", &PyEdPutSym, py::arg("name"), py::arg("value"),
        "写 LISP 符号，value 为 TypedValue(RTSHORT/RTREAL/RTSTR, …)，等价 acedPutSym。");
    m.def("acedHelp", &PyEdHelp, py::arg("file"), py::arg("topic") = L"", py::arg("cmd") = 0,
        "调起帮助，等价 acedHelp。");
    m.def("acedHelpForExternal", &PyEdHelpForExternal, py::arg("function_name"),
        "按函数名调起外部帮助，等价 acedHelpForExternal。");
    m.def("acedFNSplit", &PyEdFNSplit, py::arg("path"),
        "拆分路径为 (目录, 文件名, 扩展名)，等价 acedFNSplit。");
    m.def("acedUpdate", &PyEdUpdate, py::arg("p1") = py::none(), py::arg("p2") = py::none(),
        "重生成区域；两点均 None 刷新当前视口，等价 acedUpdate。");
    m.def("acedInitGet", &PyEdInitGet, py::arg("val"), py::arg("kwl"));
    m.def("acedUsrBrk", &PyEdUsrBrk);
    m.def("acedIsDragging", &PyEdIsDragging);
    m.def("acedIsUpdateDisplayPaused", &PyEdIsUpdateDisplayPaused);
    m.def("acedUpdateDisplayPause", &PyEdUpdateDisplayPause, py::arg("enable"));
    m.def("acedFiberWorld", &PyEdFiberWorld);
    m.def("acedLoadPartialMenu", &PyEdLoadPartialMenu, py::arg("file"));
    m.def("acedGetCommandVersion", &PyEdGetCommandVersion);
    m.def("acedMspace", &PyEdMspace);
    m.def("acedPspace", &PyEdPspace);
    m.def("acedSetCurrentVPort", &PyEdSetCurrentVPort, py::arg("vpnumber"));

    // [NEW] non-interactive session / command utilities (acedads.h / aced.h)
    m.def("acedGetInput", &PyEdGetInput, "Fetch keyword text right after RTKWORD");
    m.def("acedGetAppName", &PyEdGetAppName);
    m.def("acedGetCName", &PyEdGetCName, py::arg("cmd"), "Translate command name local<->global");
    m.def("acedGetCfg", &PyEdGetCfg, py::arg("sym"));
    m.def("acedSetCfg", &PyEdSetCfg, py::arg("sym"), py::arg("val"));
    m.def("acedUndef", &PyEdUndef, py::arg("name"), py::arg("fcode"));
    m.def("acedGetFunCode", &PyEdGetFunCode);
    m.def("acedArxLoaded", &PyEdArxLoaded, "List currently loaded ARX application names");
    m.def("acedIsMenuGroupLoaded", &PyEdIsMenuGroupLoaded, py::arg("group"));
    m.def("acedUnloadPartialMenu", &PyEdUnloadPartialMenu, py::arg("file"));
    m.def("acedReloadMenus", &PyEdReloadMenus, py::arg("incremental") = false);
    m.def("acedPostCommandPrompt", &PyEdPostCommandPrompt);
    m.def("acedUpdateDisplay", &PyEdUpdateDisplay, "Force an immediate display refresh");
    m.def("acedIsInBackgroundMode", &PyEdIsInBackgroundMode);
    m.def("acedEnableUsrbrk", &PyEdEnableUsrbrk);
    m.def("acedDisableUsrbrk", &PyEdDisableUsrbrk);
    m.def("acedIsUsrbrkDisabled", &PyEdIsUsrbrkDisabled);
    m.def("acedGraphScr", &PyEdGraphScr);
    m.def("acedTextScr", &PyEdTextScr);
    m.def("acedTextPage", &PyEdTextPage);
    m.def("acedRedraw", &PyEdRedraw, py::arg("ename") = py::none(), py::arg("mode") = 0,
        "Redraw entity (mode 0=full,1=undraw,2=highlight,3=unhighlight); None=whole screen");
    m.def("acedRetPoint", &PyEdRetPoint, py::arg("pt"));
    m.def("acedRetName", &PyEdRetName, py::arg("ename"), py::arg("type"));
    m.def("acedRetList", &PyEdRetList, py::arg("rbuf"));
    m.def("acedRetVal", &PyEdRetVal, py::arg("rbuf"));
    m.def("acutWcMatch", &PyEdWcMatch, py::arg("text"), py::arg("pattern"), "Wildcard match (case-insensitive)");

    // Editor-level free services: UCS, viewport scaling, context-menu alias.
    m.def("acedGetCurrentUCS", &PyEdGetCurrentUCS);
    m.def("acedSetCurrentUCS", &PyEdSetCurrentUCS, py::arg("mat"));
    m.def("acedRestorePreviousUCS", &PyEdRestorePreviousUCS);
    m.def("acedGetCurVportCircleSides", &PyEdGetCurVportCircleSides);
    m.def("acedCurVportPixelToDisplay", &PyEdCurVportPixelToDisplay);
    m.def("acedCurVportScreenToDisplay", &PyEdCurVportScreenToDisplay);
    m.def("acedGetCMBaseAlias", &PyEdGetCMBaseAlias, py::arg("mode"));
    m.def("acedSetCMBaseAlias", &PyEdSetCMBaseAlias, py::arg("alias"), py::arg("mode"));

    // [NEW] viewport queries (aced.h); acedSetCurVportCircleSides and
    // acedNumberOfViewports are not exported (LNK2001), omitted.
    m.def("acedActiveViewportId", &PyEdActiveViewportId, "Active viewport old-style ObjectId (0 when null)");
    m.def("acedGetCurViewportObjectId", &PyEdGetCurViewportObjectId, "Layout viewport old-style ObjectId (0 in tile mode)");
    m.def("acedViewportIdFromNumber", &PyEdViewportIdFromNumber, py::arg("n_vp_num"));
    m.def("acedGetDpiScalingValue", &PyEdGetDpiScalingValue);

    // Skeleton (deferred, need document-scoped accessors / trampolines):
    //  - AcEdInputPointManager: registerPointFilter/revokePointFilter,
    //    addPointMonitor/removePointMonitor, addInputContextReactor, forced-pick
    //    and system-cursor counters require an AcApDocument and C++ callback
    //    subclasses; not bound here.
    //  - AcEdSelectionSetService: a callback-provided interface (no standalone
    //    accessor); add/remove/highlight only make sense inside an
    //    AcEdSSGetFilter reactor; not bound here.
    //  - AcEdInplaceTextEditor: static current()/invoke() and the instance
    //    cut/copy/paste/undo family depend on registering the AcDbTextEditor
    //    base class; deferred.
    m.def("acedRegCmdsPtr", &PyEdRegCmdsPtr, "Opaque pointer to the AcEdCommandStack (acedRegCmds)");
    m.def("acedArxLoad", &PyEdArxLoad, py::arg("app"));
    m.def("acedArxUnload", &PyEdArxUnload, py::arg("app"));
    m.def("acedRetStr", &PyEdRetStr, py::arg("str"));
    m.def("acedRetInt", &PyEdRetInt, py::arg("v"));
    m.def("acedRetReal", &PyEdRetReal, py::arg("v"));
    m.def("acedRetT", &PyEdRetT);
    m.def("acedRetNil", &PyEdRetNil);
    m.def("acedRetVoid", &PyEdRetVoid);
    m.def("acedDefun", &PyEdDefun, py::arg("name"), py::arg("fcode"));
    m.def("acedMenucmd", &PyEdMenucmd, py::arg("str"));

    // interactive (block for user input; do not call at import/self-test time)
    // 交互输入统一走第 3 代仿 LISP (getpoint/getint/getreal/getstring/getangle/
    //  getorient/getdist/getcorner/getkword/entsel/osnap)；需要带 PromptStatus
    // 状态的高级交互用第 2 代 pyedGetXxx / pyedEntSel（返回 PyEdResult）。
#ifdef PyEdModule  // 交互：acedSSGet 默认点选/窗口选实体（无头无图形交互）
    m.def("acedSSGet", &PyEdSSGetPy, py::arg("mode") = py::none(), py::arg("pt1") = py::none(),
        py::arg("pt2") = py::none(), py::arg("filter") = TVList{},
        "filter: TvList（TypedValue 组码链）；返回拥有 SelectionSet（析构自动 SSFree）。");
#endif
    // 选择集统一走第 3 代 (ssget/sslength/ssname/ssmemb/ssadd/ssdel/ssgetfirst/
    //  sssetfirst) + SelectionSet 对象方法 (count/get_entname_at/has_member/add/remove)。

    // 实体统一走第 3 代 (entnext/entlast/entdel/handent/entmake/entupd/entget/entmod)。
    m.def("acedVports", &PyEdVports, "Current viewport descriptors as an owned PyResbuf chain");

    m.def("acedCmd", &PyEdCmd, py::arg("args"), "Run a resbuf command chain (wraps acedCmdS)");

    // [NEW] pure editor math / format utilities
    m.def("acedTrans", &PyEdTrans, py::arg("pt"),
        py::arg("from") = TypedValue{ (short)0, (int16_t)0 },
        py::arg("to") = TypedValue{ (short)0, (int16_t)0 }, py::arg("disp") = 0,
        "Transform pt between coordinate systems; from/to 为 TypedValue：RTSHORT=坐标系码(0 WCS/1 UCS/2 DCS/3 PSDCS)，RTENAME=实体空间。");
    m.def("acdbInters", &PyEdInters, py::arg("p1"), py::arg("p2"), py::arg("p3"), py::arg("p4"),
        py::arg("extend") = false, "Segment intersection; None when none");
    m.def("acdbRToS", &PyEdRToS, py::arg("val"), py::arg("unit") = -1, py::arg("prec") = -1,
        "Real -> localized string (unit/prec -1 = current settings)");
    m.def("acdbAngToS", &PyEdAngToS, py::arg("angle"), py::arg("unit") = -1, py::arg("prec") = -1,
        "Radians -> localized angle string");
    m.def("acdbDisToF", &PyEdDisToF, py::arg("s"), py::arg("unit"), "Localized distance string -> double");
    m.def("acdbAngToF", &PyEdAngToF, py::arg("s"), py::arg("unit"), "Localized angle string -> radians");
    m.def("acedTextBox", &PyEdTextBox, py::arg("dxf"),
        "文字 DXF TvList（10 位置/40 高度/50 旋转/1 内容）-> 包围盒 ((x,y,z),(x,y,z))，失败 None，等价 acedTextBox。");

    // ---- acut* 实用函数 ----
    m.def("acutPrintf", &PyAcutPrintf, py::arg("fmt"), "打印到 CAD 命令行");
    m.def("acutCvUnit", &PyAcutCvUnit, py::arg("value"), py::arg("oldunit"), py::arg("newunit"),
        "单位换算（oldunit/newunit 为单位名如 \"inch\"/\"foot\"/\"meter\"）");
    m.def("acutAngle", &PyAcutAngle, py::arg("p1"), py::arg("p2"), "两点夹角（弧度）");
    m.def("acutDistance", &PyAcutDistance, py::arg("p1"), py::arg("p2"), "两点距离");
    m.def("acutPolar", &PyAcutPolar, py::arg("pt"), py::arg("angle"), py::arg("dist"),
        "极坐标：从 pt 按 angle(弧度)/dist 求新点");
    m.def("acdbRawAngToS", &PyAcdbRawAngToS, py::arg("angle"), py::arg("unit") = -1, py::arg("prec") = -1,
        "原始角度 -> 字符串（不跟随当前角度制式）");
    m.def("acdbRawAngToF", &PyAcdbRawAngToF, py::arg("s"), py::arg("unit"),
        "原始角度字符串 -> 弧度");
    m.def("acdbSNValid", &PyAcdbSNValid, py::arg("s"), py::arg("pipetest") = false,
        "符号名合法性验证（pipetest=true 时拒绝含 | 的名）");

    // ---- acdb 符号表 / 字典 / 扩展 ----
    m.def("acdbEntGetX", &PyEdEntGetX, py::arg("ename"), py::arg("args") = TVList{},
        "带 Xdata 参数取实体 DXF 列表；失败 None");
    m.def("acdbRegApp", &PyEdRegApp, py::arg("appname"), "注册 APP 名（Xdata 用）");
    m.def("acdbTblNext", &PyEdTblNext, py::arg("tblname"), py::arg("rewind") = false,
        "符号表遍历（tblname: LAYER/TYPE/LTSTYLE/...）；rewind=true 从表头开始；末尾 None");
    m.def("acdbTblSearch", &PyEdTblSearch, py::arg("tblname"), py::arg("sym"), py::arg("setnext") = false,
        "符号表搜索；setnext=true 后下次 TblNext 从该项继续；未找到 None");
    m.def("acdbTblObjName", &PyEdTblObjName, py::arg("tblname"), py::arg("sym"),
        "符号表项 -> AdsName；失败空名称");
    m.def("acdbNamedObjDict", &PyEdNamedObjDict, "数据库命名对象字典 AdsName");
    m.def("acdbDictSearch", &PyEdDictSearch, py::arg("dict"), py::arg("sym"), py::arg("setnext") = false,
        "字典搜索；未找到 None");
    m.def("acdbDictNext", &PyEdDictNext, py::arg("dict"), py::arg("rewind") = false,
        "字典遍历；rewind=true 从头开始；末尾 None");
    m.def("acdbDictRename", &PyEdDictRename, py::arg("dict"), py::arg("oldsym"), py::arg("newsym"),
        "字典项重命名");
    m.def("acdbDictRemove", &PyEdDictRemove, py::arg("dict"), py::arg("sym"), "删除字典项");
    m.def("acdbDictAdd", &PyEdDictAdd, py::arg("dict"), py::arg("sym"), py::arg("obj"),
        "向字典添加对象（obj 为实体 AdsName），返回新项名");

    // ---- interactive-style infrastructure ----
    // PromptStatus / OptionFlags 枚举已迁移至 PyArx.Enum 模块

    py::class_<PromptOptions>(m, "PromptOptions")
        .def(py::init<>())
        .def_readwrite("flags", &PromptOptions::Flags)
        .def_readwrite("message", &PromptOptions::Message)
        .def_readwrite("keyWords", &PromptOptions::KeyWords)
        .def("setMessage", &PromptOptions::setMessage, py::arg("message"))
        .def("setFlag", &PromptOptions::setFlag, py::arg("flag"));

    py::class_<EntityValue>(m, "EntityValue")
        .def_property_readonly("id", [](const EntityValue& v) {
            return v.id;
        })
        .def_readwrite("pickedPoint", &EntityValue::pickedPoint);

    py::class_<SubEntityValue>(m, "SubEntityValue")
        .def_property_readonly("id", [](const SubEntityValue& v) {
            return v.id;
        })
        .def_readwrite("pickedPoint", &SubEntityValue::pickedPoint)
        .def_readwrite("xform", &SubEntityValue::xform)
        .def_property_readonly("container", [](const SubEntityValue& v) {
            return v.container;
        });

    bind_PyPromptResult(m);

#ifdef PyEdModule  // ===== 交互输入命令（阻塞等待用户键盘/点选/拖动，无头排除）=====
    // 统一 PyEdResult<T>：返回 value + status（失败/取消 value=None），
    // 支持 int()/float()/bool() 取用（对应 C++ operator T() 隐式转换）。
    m.def("pyedGetInt", [](const PromptOptions& o) { return wrapPromptResult(pyedGetInt(o)); }, py::arg("opts") = PromptOptions());
    m.def("pyedGetInt", [](const AcString& msg) { return wrapPromptResult(pyedGetInt(msg)); }, py::arg("message") = L"请输入整数:");
    m.def("pyedGetReal", [](const PromptOptions& o) { return wrapPromptResult(pyedGetReal(o)); }, py::arg("opts") = PromptOptions());
    m.def("pyedGetReal", [](const AcString& msg) { return wrapPromptResult(pyedGetReal(msg)); }, py::arg("message") = L"输入实数:");
    m.def("pyedGetString", [](int cronly, const PromptOptions& o) { return wrapPromptResult(pyedGetString(cronly, o)); }, py::arg("cronly") = 0, py::arg("opts") = PromptOptions());
    m.def("pyedGetString", [](const AcString& msg, int cronly) { return wrapPromptResult(pyedGetString(msg, cronly)); }, py::arg("message") = L"请输入字符:", py::arg("cronly") = 0);
    m.def("pyedGetKeyword", [](const PromptOptions& o) { return wrapPromptResult(pyedGetKeyword(o)); }, py::arg("opts") = PromptOptions());
    m.def("pyedGetKeyword", [](const AcString& msg) { return wrapPromptResult(pyedGetKeyword(msg)); }, py::arg("message") = L"请输入关键字:");
    m.def("pyedGetPoint", [](const AcGePoint3d& basept, const PromptOptions& o) { return wrapPromptResult(pyedGetPoint(basept, o)); }, py::arg("basept") = AcGePoint3d(), py::arg("opts") = PromptOptions());
    m.def("pyedGetPoint", [](const AcString& msg) { return wrapPromptResult(pyedGetPoint(msg)); }, py::arg("message") = L"请选择一个点:");
    m.def("pyedGetCorner", [](const AcGePoint3d& basept, const PromptOptions& o) { return wrapPromptResult(pyedGetCorner(basept, o)); }, py::arg("basept"), py::arg("opts") = PromptOptions());
    m.def("pyedGetCorner", [](const AcGePoint3d& basept, const AcString& msg) { return wrapPromptResult(pyedGetCorner(basept, msg)); }, py::arg("basept"), py::arg("message") = L"选择对角:");
    m.def("pyedGetDist", [](const AcGePoint3d& basept, const PromptOptions& o) { return wrapPromptResult(pyedGetDist(basept, o)); }, py::arg("basept"), py::arg("opts") = PromptOptions());
    m.def("pyedGetDist", [](const AcGePoint3d& basept, const AcString& msg) { return wrapPromptResult(pyedGetDist(basept, msg)); }, py::arg("basept"), py::arg("message") = L"输入目标距离:");
    m.def("pyedGetAngle", [](const AcGePoint3d& basept, const PromptOptions& o) { return wrapPromptResult(pyedGetAngle(basept, o)); }, py::arg("basept"), py::arg("opts") = PromptOptions());
    m.def("pyedGetAngle", [](const AcGePoint3d& basept, const AcString& msg) { return wrapPromptResult(pyedGetAngle(basept, msg)); }, py::arg("basept"), py::arg("message") = L"输入角度:");
    m.def("pyedOsnap", [](const AcGePoint3d& pt, const AcString& mode) { return wrapPromptResult(pyedOsnap(pt, mode)); },
        py::arg("pt"), py::arg("mode"), "对象捕捉：对给定点按模式（如 'end,mid,cen'）求捕捉点，返回 PyEdResult");
    m.def("pyedEntSel", [](const PromptOptions& o) { return wrapPromptResult(pyedEntSel(o)); }, py::arg("opts") = PromptOptions());
    m.def("pyedEntSel", [](const AcString& msg) { return wrapPromptResult(pyedEntSel(msg)); }, py::arg("message") = L"请选择图元:");
    m.def("pyedNEntSel", [](const PromptOptions& o) { return wrapPromptResult(pyedNEntSel(o)); }, py::arg("opts") = PromptOptions());
    m.def("pyedNEntSel", [](const AcString& msg) { return wrapPromptResult(pyedNEntSel(msg)); }, py::arg("message") = L"请选择子图元:");
    m.def("pyedSelect", [](const PromptOptions& o, const TVList& filter, const AcString& mode) {
            return wrapPromptResult(pyedSelectPy(o, filter, mode));
        },
        py::arg("opts") = PromptOptions(), py::arg("filter") = TVList{}, py::arg("mode") = L":A",
        "多模式选择集（mode: :A/X/:W/:C/...）；filter: TvList（TypedValue 组码链）");
    m.def("pyedSetFirst", &pyedSetFirst, py::arg("object_ids"));
#endif  // ===== 交互输入命令结束 =====

    // ============================================================
    // 命令发送：command(*args) 仿 (command ...)；参数混合
    // str/int/float/point(2 或 3 元组)/AdsName，自动构造 resbuf 链，
    // 末尾自动补 RTNONE 终止符。底层走 acedCmdS。
    // 交互输入统一走第 2 代 pyedGetXxx / pyedEntSel / pyedSelect（返回 PyEdResult）。
    // ============================================================
    m.def("command", [](py::args args) {
        TVList tvs;
        for (auto a : args) {
            if (py::isinstance<py::str>(a))
                tvs.emplace_back(RTSTR, a.cast<AcString>());
            else if (py::isinstance<py::int_>(a))
                tvs.emplace_back(RTSHORT, a.cast<int32_t>());
            else if (py::isinstance<py::float_>(a))
                tvs.emplace_back(RTREAL, a.cast<double>());
            else if (py::isinstance<PyAdsName>(a))
                tvs.emplace_back(RTENAME, a.cast<PyAdsName>());
            else if (py::isinstance<py::tuple>(a) || py::isinstance<py::list>(a)) {
                auto s = a.cast<py::sequence>();
                if (s.size() == 2)
                    tvs.emplace_back(RTPOINT, std::array<double,3>{s[0].cast<double>(), s[1].cast<double>(), 0.0});
                else if (s.size() >= 3)
                    tvs.emplace_back(RTPOINT, std::array<double,3>{s[0].cast<double>(), s[1].cast<double>(), s[2].cast<double>()});
                else return false;
            } else {
                return false;
            }
        }
        tvs.emplace_back(RTNONE, DxfValue::kNull);
        return ::acedCmdS(PyResbuf(tvs).data()) == RTNORM;
    }, "command(*args): 仿 (command ...) 发送命令；参数可为 str/int/float/point(tuple)/AdsName，末尾自动 RTNONE");

    // ============================================================
    // 选择集 ssget 系列 + 实体 entget 系列（第 3 代仿 LISP）
    // 约定：失败/空返回 None；dxf 用 {组码: 值} dict（点组码多值用 list）。
    // ============================================================
    auto tvFromValue = [](short code, const py::handle& val, TVList& tv) -> bool {
        if (py::isinstance<py::str>(val))
            tv.emplace_back(code, val.cast<AcString>());
        else if (py::isinstance<py::int_>(val))
            tv.emplace_back(code, val.cast<int32_t>());
        else if (py::isinstance<py::float_>(val))
            tv.emplace_back(code, val.cast<double>());
        else if (py::isinstance<PyAdsName>(val))
            tv.emplace_back(code, val.cast<PyAdsName>());
        else if (py::isinstance<py::tuple>(val) || py::isinstance<py::list>(val)) {
            auto s = val.cast<py::sequence>();
            if (s.size() == 2)
                tv.emplace_back(code, std::array<double,3>{s[0].cast<double>(), s[1].cast<double>(), 0.0});
            else if (s.size() == 3)
                tv.emplace_back(code, std::array<double,3>{s[0].cast<double>(), s[1].cast<double>(), s[2].cast<double>()});
            else {
                // list of points with the same group code -> multiple TypedValue
                for (auto item : s) {
                    auto it = item.cast<py::sequence>();
                    if (it.size() == 2)
                        tv.emplace_back(code, std::array<double,3>{it[0].cast<double>(), it[1].cast<double>(), 0.0});
                    else if (it.size() >= 3)
                        tv.emplace_back(code, std::array<double,3>{it[0].cast<double>(), it[1].cast<double>(), it[2].cast<double>()});
                    else return false;
                }
            }
        } else {
            return false;
        }
        return true;
    };
    auto dxfToTvList = [&tvFromValue](const py::object& dxf) -> TVList {
        TVList tv;
        if (dxf.is_none()) return tv;
        if (py::isinstance<py::dict>(dxf)) {
            for (auto item : dxf.cast<py::dict>()) {
                const short code = item.first.cast<short>();
                tvFromValue(code, item.second, tv);
            }
        } else if (py::isinstance<py::list>(dxf) || py::isinstance<py::tuple>(dxf)) {
            for (auto kv : dxf.cast<py::sequence>()) {
                auto p = kv.cast<py::sequence>();
                const short code = p[0].cast<short>();
                tvFromValue(code, p[1], tv);
            }
        }
        return tv;
    };

#ifdef PyEdModule  // 交互：仿 (ssget [mode]...)，默认交互点选/窗口选实体
    m.def("ssget", [](const AcString& mode, const std::optional<AcGePoint3d>& pt1,
                      const std::optional<AcGePoint3d>& pt2, const py::object& filter) {
        TVList tv = tvListFromPy(filter);
        PyEdSelectionSets ss = PyEdSSGetPy(mode, pt1, pt2, tv);
        if (ss.count() == 0) return py::object(py::none());
        return py::cast(std::move(ss));
    }, py::arg("mode") = AcString(), py::arg("pt1") = std::optional<AcGePoint3d>(),
        py::arg("pt2") = std::optional<AcGePoint3d>(), py::arg("filter") = py::none(),
        "仿 (ssget [mode] [pt1 [pt2]] [filter])：mode=''/X/W/C/L/P/F 等，filter 为 [(code,value)...]；空集返回 None");
#endif

    m.def("sslength", [](const PyEdSelectionSets& ss) { return (int)ss.count(); },
        py::arg("ss"), "仿 (sslength ss)：选择集元素个数");
    m.def("ssname", &PyEdSSName, py::arg("ss"), py::arg("i"), "仿 (ssname ss i)：第 i 个元素 AdsName");
    m.def("ssmemb", &PyEdSSMemb, py::arg("ename"), py::arg("ss"), "仿 (ssmemb e ss)：e 是否在 ss 中");
    m.def("ssadd", [](const py::object& ename, const py::object& ss) {
        if (!ss.is_none()) {
            auto& s = ss.cast<PyEdSelectionSets&>();
            if (ename.is_none()) return py::object(py::none());
            return py::cast(PyEdSSAdd(ename.cast<PyAdsName>(), &s));
        }
        if (ename.is_none()) return py::cast(PyEdSSAdd(PyAdsName{}, nullptr));
        return py::cast(PyEdSSAdd(ename.cast<PyAdsName>(), nullptr));
    }, py::arg("ename") = py::none(), py::arg("ss") = py::none(),
        "仿 (ssadd [e [ss]])：无参建新集 / 有 e 建含 e 集 / 有 ss 则加入并返回新集");
    m.def("ssdel", &PyEdSSDel, py::arg("ename"), py::arg("ss"), "仿 (ssdel e ss)：从集删除 e");
    m.def("ssgetfirst", &PyEdSSGetFirst, "仿 (ssgetfirst)：返回 (组选择集 list, 首选集 list)");
    m.def("sssetfirst", &pyedSetFirst, py::arg("object_ids"),
        "仿 (sssetfirst ...)：设首选集；object_ids 为 ObjectId 列表或 None");

    m.def("entget", [](const py::object& ename) -> py::object {
        // None 入参（根因修复）：旧签名强转 PyAdsName，pybind 在类型转换阶段直接抛
        // TypeError "incompatible function arguments"；现在 None 归一为失败返回 None。
        if (ename.is_none()) return py::object(py::none());
        PyAdsName n = ename.cast<PyAdsName>();
        PyResbuf rb = PyEdEntGet(n);
        if (rb.isNull()) return py::object(py::none());
        py::dict d;
        for (const auto& tv : rb.toList()) {
            py::object v = DxfValueToPy(tv.value);
            const int key = tv.code;
            py::object keyo = py::int_(key);
            if (d.contains(keyo)) {
                py::object cur = d[keyo];
                if (py::isinstance<py::list>(cur))
                    cur.cast<py::list>().append(v);
                else
                    d[keyo] = py::make_tuple(cur, v);
            } else {
                d[keyo] = v;
            }
        }
        return py::object(d);
    }, py::arg("ename"), "仿 (entget e)：返回 DXF 组码字典 {code: value}，失败返回 None");

    m.def("entmod", [&dxfToTvList](const py::object& dxf) {
        TVList tv = dxfToTvList(dxf);
        if (tv.empty()) return false;
        return PyEdEntMod(tv);
    }, py::arg("dxf"), "仿 (entmod dxf)：按 {code:value} 修改实体");

    m.def("entmake", [&dxfToTvList](const py::object& dxf) {
        TVList tv = dxfToTvList(dxf);
        if (tv.empty()) return PyAdsName{};
        return PyEdEntMakeX(tv);
    }, py::arg("dxf"), "仿 (entmake dxf)：按 {code:value} 建实体，返回新 AdsName");

    m.def("entdel", [](const py::object& ename) -> bool {
        if (ename.is_none()) return false;
        return PyEdEntDel(ename.cast<PyAdsName>());
    }, py::arg("ename"), "仿 (entdel e)：删除/恢复实体；None 返回 False");
    m.def("entupd", [](const py::object& ename) -> bool {
        if (ename.is_none()) return false;
        return PyEdEntUpd(ename.cast<PyAdsName>());
    }, py::arg("ename"), "仿 (entupd e)：重生成实体图形；None 返回 False");
    m.def("handent", [](const py::object& h) -> py::object {
        if (h.is_none()) return py::object(py::none());
        PyAdsName n = PyEdHandEnt(h.cast<AcString>());
        if (n.isEmpty()) return py::object(py::none());
        return py::cast(n);
    }, py::arg("handle"), "仿 (handent handle)：句柄转 AdsName，失败/None 返回 None");
    m.def("entnext", [](const py::object& ename) {
        PyAdsName n = ename.is_none() ? PyEdEntNext(PyAdsName{}) : PyEdEntNext(ename.cast<PyAdsName>());
        if (n.isEmpty()) return py::object(py::none());
        return py::cast(n);
    }, py::arg("ename") = py::none(), "仿 (entnext [e])：下一实体 AdsName，末尾返回 None");
    m.def("entlast", []() {
        PyAdsName n = PyEdEntLast();
        if (n.isEmpty()) return py::object(py::none());
        return py::cast(n);
    }, "仿 (entlast)：数据库最后实体 AdsName，失败返回 None");

    // 批量文字搜索：对一批 ObjectId 建一次文字索引（Text/MText，大小写无关），
    // 按 mode 返回命中 id 列表；mode: prefix(默认) 前缀 / contains 任意位置 / full 整串全等。
    m.def("textFind", &PyEdTextFind, py::arg("ids"), py::arg("keyword"),
        py::arg("mode") = "prefix",
        "在 ids 中按文字搜索（AcDbText/AcDbMText，大小写无关）；返回 list[oldId]");

    // AcEdJig trampoline + AcEdDragStyle (registered in this same module).
    // 交互拖动环（drag/acquire* 阻塞等待用户点取与拖动），无头无显示通道，仅 GUI。
#ifdef PyEdModule
    registerPyEdJig(m);
#endif
}
