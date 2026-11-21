#include "PyEdEditor.h"
#include <afxwin.h>
#include "core_rxmfcapi.h"

namespace
{
	struct PyEdEditor
	{
	};
	struct PyEdEditorReactor
	{
	};
}

void bind_PyEdEditor(pym& ed)
{
	// ---------------- Ed.Editor ----------------
	// PyRx 的 Editor 是编辑器单例类；PyArx 的等价能力已经在 Ed 模块的 aced*/pyed* 函数里，
	// 这里用 staticmethod 转发（既可 Ed.Editor.getPoint()，也可 Ed.Editor().getPoint()）。
	if (!py::hasattr(ed, "Editor"))
	{
		py::class_<PyEdEditor> cls(ed, "Editor", "编辑器接口（转发到 Ed 模块的 aced*/pyed* 实现）");
		cls.def(py::init<>());
		cls.def_static("className", []() { return std::string("Editor"); });
		cls.attr("__repr__") = py::staticmethod(py::cpp_function([](py::object) { return std::string("Editor"); }));

		static const char* kFwd[][2] = {
			{"activeViewportId",      "acedActiveViewportId"},
			{"curViewportObjectId",   "acedGetCurViewportObjectId"},
			{"entSel",                "pyedEntSel"},
			{"nEntSelP",              "pyedNEntSel"},
			{"nEntSelPEx",            "pyedNEntSel"},
			{"getAngle",              "pyedGetAngle"},
			{"getCorner",             "pyedGetCorner"},
			{"getDist",               "pyedGetDist"},
			{"getDouble",             "pyedGetReal"},
			{"getInput",              "acedGetInput"},
			{"getInteger",            "pyedGetInt"},
			{"getKword",              "pyedGetKeyword"},
			{"getPoint",              "pyedGetPoint"},
			{"getReal",               "pyedGetReal"},
			{"getString",             "pyedGetString"},
			{"initGet",               "acedInitGet"},
			{"getCurrentUCS",         "acedGetCurrentUCS"},
			{"setCurrentUCS",         "acedSetCurrentUCS"},
			{"getCurrentSelectionSet", "ssgetfirst"},
			{"regen",                 "acedUpdate"},
			{"writeMessage",          "acutPrintf"},
			{"select",                "acedSSGet"},
			{"selectAll",             "acedSSGet"},
			{"selectCrossingPolygon", "acedSSGet"},
			{"selectCrossingWindow",  "acedSSGet"},
			{"selectFence",           "acedSSGet"},
			{"selectImplied",         "acedSSGet"},
			{"selectKeyword",         "acedSSGet"},
			{"selectLast",            "acedSSGet"},
			{"selectPrevious",        "acedSSGet"},
			{"selectPrompt",          "acedSSGet"},
			{"selectWindow",          "acedSSGet"},
			{"selectWindowPolygon",   "acedSSGet"},
			{"ssget",                 "acedSSGet"},
			{"ssgetkw",               "acedSSGet"},
		};
		for (auto& f : kFwd)
		{
			if (py::hasattr(ed, f[1]))
				cls.attr(f[0]) = py::staticmethod(ed.attr(f[1]));
		}
	}

	// ---------------- acedCoordFromPixelToWorld（PyRx Ed.Core 同名函数的底层实现）----------------
	// 支持 4 种入参：(tuple) / (x, y) / (windnum, tuple) / (windnum, x, y)
	if (!py::hasattr(ed, "acedCoordFromPixelToWorld"))
	{
		ed.def("acedCoordFromPixelToWorld", [](py::args a) -> py::object {
			int windnum = 1;
			bool hasWind = false;
			int x = 0, y = 0;
			if (a.size() == 1 && py::isinstance<py::tuple>(a[0]) && py::len(a[0]) == 2)
			{
				x = py::cast<int>(a[0][py::int_(0)]);
				y = py::cast<int>(a[0][py::int_(1)]);
			}
			else if (a.size() == 2 && py::isinstance<py::tuple>(a[1]) && py::len(a[1]) == 2)
			{
				windnum = py::cast<int>(a[0]);
				hasWind = true;
				x = py::cast<int>(a[1][py::int_(0)]);
				y = py::cast<int>(a[1][py::int_(1)]);
			}
			else if (a.size() == 2)
			{
				x = py::cast<int>(a[0]);
				y = py::cast<int>(a[1]);
			}
			else if (a.size() == 3)
			{
				windnum = py::cast<int>(a[0]);
				hasWind = true;
				x = py::cast<int>(a[1]);
				y = py::cast<int>(a[2]);
			}
			else
			{
				throw py::value_error("acedCoordFromPixelToWorld: 期望 (tuple) / (x,y) / (windnum,tuple) / (windnum,x,y)");
			}

			CPoint cpnt(x, y);
			acedDwgPoint out{ 0.0, 0.0, 0.0 };
			if (hasWind)
			{
				if (!acedCoordFromPixelToWorld(windnum, cpnt, out))
					return py::none();
			}
			else
			{
				acedCoordFromPixelToWorld(cpnt, out);  // 两参重载返回 void，无失败信号
			}
			return py::cast(AcGePoint3d(out[0], out[1], out[2]));
		});
	}
	// PyArx 尚无 CDB 反射器事件源；这里只提供可被 Python 继承的类骨架，
	// 不做事件派发（子类覆写方法不会被回调）。
	if (!py::hasattr(ed, "EditorReactor"))
	{
		py::class_<PyEdEditorReactor>(ed, "EditorReactor",
			"编辑器反射器占位类：PyArx 未接线 CDB 事件源，覆写的方法不会被回调")
			.def(py::init<>())
			.def("addReactor", [](PyEdEditorReactor&) { return true; })
			.def("removeReactor", [](PyEdEditorReactor&, py::object) { return true; }, py::arg("reactor"))
			.def_static("className", []() { return std::string("EditorReactor"); });
	}
}
