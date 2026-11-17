#pragma once
// PyEnumArg.h —— 把「枚举形参」的绑定入口改成「int 形参」，同时不改 C++ 侧签名。
// 用法：.def("setMode", pyarx::enumSetter(&AcGiMaterialTraits::setMode), py::arg("value"))
// 这样 Python 侧既可传枚举成员（有 __index__），也可传裸整数（内部 static_cast 回枚举）。
#include <cstdint>

namespace pyarx {

// 返回 void 的 setter
template <typename C, typename E>
auto enumSetter(void (C::*fn)(E))
{
	return [fn](C& self, int value) { (self.*fn)(static_cast<E>(value)); };
}

// 返回非 void 的 setter / 方法
template <typename C, typename R, typename E>
auto enumSetter(R (C::*fn)(E))
{
	return [fn](C& self, int value) -> R { return (self.*fn)(static_cast<E>(value)); };
}

} // namespace pyarx
