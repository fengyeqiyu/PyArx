#pragma once
#include "stdafx.h"
#include "singleInstance.h"



class ArxOutPut:public SingleInstance<ArxOutPut> {
public:
	ArxOutPut() {};
	//重定向输出类必须要有write方法
	void write(AcString str) {
		if (str.isEmpty()) return;
		try {
			// 确保 GIL 持有（Python 回调时 GIL 已持有）
			// acutPrintf 在命令线程内始终安全
			const wchar_t* p = str.kwszPtr();
			if (p && *p) {
				acutPrintf(p);
			}
		}
		catch (...) {
			// 吞掉所有异常，不穿透到 Python 解释器
		}
	}
	void flush() {
		// 空 flush，不做任何操作（避免访问失效状态）
	}
};

// 模块绑定入口
void bind_ArxOutPut(py::module& m); //输出重定向模块绑定

// PyArxEngine.Redir 子模块入口，由 PyEngine.cpp redirectOut() 挂载（归属引擎，不挂在 PyArxCore）
void bind_PyRedir(pym& m);

// PyArxEngine 顶层 pip 系列入口（归属引擎，与 Redir/Cmd 平级），由 redirectOut 对 PyArxEngine 模块调用
void bind_PyPip(pym& m);
