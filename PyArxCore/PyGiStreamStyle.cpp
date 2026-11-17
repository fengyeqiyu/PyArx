// ===========================================================================
// PyGiStreamStyle.cpp —— AcGiDrawStream / AcGiVisualStyle 绑定
// ===========================================================================
#include "stdafx.h"
#include "PyGiStreamStyle.h"
#include "acgidrawstream.h"
#include "acgivisualstyle.h"

// ---------------------------------------------------------------------------
// AcGiDrawStream
// ---------------------------------------------------------------------------
GiDrawStreamHolder::GiDrawStreamHolder(bool withOwnerPlaceholder)
{
	// withOwnerPlaceholder 保留形参：SDK 另有一个 AcGiDrawStream(const AcGiDrawable*) 构造，
	// 但 owner 只能是真实的 AcGiDrawable 实例，Python 侧暂无对应包装，故只走无参构造。
	(void)withOwnerPlaceholder;
	m_p = new AcGiDrawStream();
}

GiDrawStreamHolder::~GiDrawStreamHolder()
{
	if (m_p) {
		delete m_p;
		m_p = nullptr;
	}
}

bool GiDrawStreamHolder::isValid() const { return m_p && m_p->isValid(); }
bool GiDrawStreamHolder::isPersistent() const { return m_p && m_p->isPersistent() == Adesk::kTrue; }
int GiDrawStreamHolder::drawableType() const { return m_p ? static_cast<int>(m_p->drawableType()) : 0; }
unsigned long long GiDrawStreamHolder::objectId() const
{
	return m_p ? static_cast<unsigned long long>(m_p->id().asOldId()) : 0ull;
}
bool GiDrawStreamHolder::hasOwner() const { return m_p && m_p->getOwner() != nullptr; }

// GraphicsUpdateProc 是裸函数指针，不能直接捕获 Python 可调用对象；
// 用文件级静态保存，保证回调期间 GIL 与引用都安全。
static py::object g_drawStreamCallback;

static bool drawStreamUpdateProc(const AcArray<AcGiDrawable*>& drawableArray)
{
	py::gil_scoped_acquire gil;
	if (!g_drawStreamCallback || g_drawStreamCallback.is_none()) return true;
	try {
		py::list ids;
		for (int i = 0; i < drawableArray.length(); ++i) {
			AcGiDrawable* p = drawableArray.at(i);
			ids.append(py::int_(static_cast<long long>(reinterpret_cast<intptr_t>(p))));
		}
		py::object r = g_drawStreamCallback(ids);
		if (r.is_none()) return true;
		return py::cast<bool>(r);
	} catch (py::error_already_set& e) {
		e.discard_as_unraisable("AcGiDrawStream.build.callback");
		return false;
	}
}

bool GiDrawStreamHolder::build(const std::vector<GiDrawStreamHolder*>& streams, py::object callback)
{
	AcArray<AcGiDrawStream*> arr;
	for (auto* s : streams) {
		if (s && s->stream()) arr.append(s->stream());
	}
	if (callback.is_none()) {
		return AcGiDrawStream::build(arr, nullptr);
	}
	g_drawStreamCallback = callback;
	bool ok = AcGiDrawStream::build(arr, &drawStreamUpdateProc);
	g_drawStreamCallback = py::object();
	return ok;
}

// ---------------------------------------------------------------------------
// 绑定
// ---------------------------------------------------------------------------
void bind_PyGiStreamStyle(pym& m)
{
	py::class_<GiDrawStreamHolder>(m, "AcGiDrawStream",
		"图形流 drawable（AcGiDrawStream::build 驱动批量更新）")
		.def(py::init<bool>(), py::arg("withOwnerPlaceholder") = false)
		.def("isValid", &GiDrawStreamHolder::isValid)
		.def("isPersistent", &GiDrawStreamHolder::isPersistent)
		.def("drawableType", &GiDrawStreamHolder::drawableType)
		.def("objectId", &GiDrawStreamHolder::objectId)
		.def("hasOwner", &GiDrawStreamHolder::hasOwner)
		.def_static("build", &GiDrawStreamHolder::build,
			py::arg("streams"), py::arg("callback") = py::none(),
			"build(streams, callback=None) -> bool");

	// ---- AcGiVisualStyle ----
	py::enum_<AcGiVisualStyle::Type>(m, "AcGiVisualStyleType")
		.value("kFlat", AcGiVisualStyle::kFlat)
		.value("kFlatWithEdges", AcGiVisualStyle::kFlatWithEdges)
		.value("kGouraud", AcGiVisualStyle::kGouraud)
		.value("kGouraudWithEdges", AcGiVisualStyle::kGouraudWithEdges)
		.value("k2DWireframe", AcGiVisualStyle::k2DWireframe)
		.value("k3DWireframe", AcGiVisualStyle::k3DWireframe)
		.value("kHidden", AcGiVisualStyle::kHidden)
		.value("kBasic", AcGiVisualStyle::kBasic)
		.value("kRealistic", AcGiVisualStyle::kRealistic)
		.value("kConceptual", AcGiVisualStyle::kConceptual)
		.value("kCustom", AcGiVisualStyle::kCustom)
		.value("kDim", AcGiVisualStyle::kDim)
		.value("kBrighten", AcGiVisualStyle::kBrighten)
		.value("kThicken", AcGiVisualStyle::kThicken)
		.value("kLinePattern", AcGiVisualStyle::kLinePattern)
		.value("kFacePattern", AcGiVisualStyle::kFacePattern)
		.value("kColorChange", AcGiVisualStyle::kColorChange)
		.value("kFaceOnly", AcGiVisualStyle::kFaceOnly)
		.value("kEdgeOnly", AcGiVisualStyle::kEdgeOnly)
		.value("kDisplayOnly", AcGiVisualStyle::kDisplayOnly)
		.value("kJitterOff", AcGiVisualStyle::kJitterOff)
		.value("kOverhangOff", AcGiVisualStyle::kOverhangOff)
		.value("kEdgeColorOff", AcGiVisualStyle::kEdgeColorOff)
		.value("kShadesOfGray", AcGiVisualStyle::kShadesOfGray)
		.value("kSketchy", AcGiVisualStyle::kSketchy)
		.value("kXRay", AcGiVisualStyle::kXRay)
		.value("kShadedWithEdges", AcGiVisualStyle::kShadedWithEdges)
		.value("kShaded", AcGiVisualStyle::kShaded)
		.value("kByViewport", AcGiVisualStyle::kByViewport)
		.value("kByLayer", AcGiVisualStyle::kByLayer)
		.value("kByBlock", AcGiVisualStyle::kByBlock)
		.value("kEmptyStyle", AcGiVisualStyle::kEmptyStyle);

	py::class_<AcGiVisualStyle>(m, "AcGiVisualStyle",
		"视觉样式对象（实例由 CAD 提供，Python 侧只做读取/传递）")
		.def("type", [](const AcGiVisualStyle& s) { return s.type(); })
		.def("__eq__", [](const AcGiVisualStyle& a, const AcGiVisualStyle& b) { return a == b; },
			py::is_operator());

	py::class_<AcGiVisualStyleTraits>(m, "AcGiVisualStyleTraits",
		"视觉样式 traits 接口（抽象，实例由 CAD 提供）")
		.def("setAcGiVisualStyle", &AcGiVisualStyleTraits::setAcGiVisualStyle)
		.def("acgiVisualStyle", &AcGiVisualStyleTraits::acgiVisualStyle);
}
