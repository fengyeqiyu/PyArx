#include "stdafx.h"
#include "PyDbImage.h"

PyWrapper_Cpp(DbImage);

void bind_PyDbImage(pym& m) {
	py::class_<PyDbImage, PyDbEntity>(m, "PyDbImage")
		.def(py::init<AcDbImage*>(), py::arg("p"))
		PyWrapper_BIND(DbImage)
		;
	// dbimage.h 仅两个纯虚方法，均依赖未封装的 AcGi 类型，暂不绑：
	//   virtual AcGiSentScanLines* getScanLines(const AcGiRequestScanLines&) const = 0;
	//   virtual Adesk::Boolean     freeScanLines(AcGiSentScanLines*) const = 0;
	// 原因：工程无 AcGiRequestScanLines / AcGiSentScanLines 的 Python 包装，Python 侧无法构造入参，
	//       也无法持有返回的扫描线对象；属 P2 依赖（待 PyGi 层包装）。
	// 注意：任务简述提到的 getImageSize/setImageSize/getImageTransparency 不在 dbimage.h（属派生类
	//       AcDbRasterImage），不在本基类封装范围。pack：CHM 无页面，疑误提取，跳过。
}
