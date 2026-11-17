"""把 Gi 里 14 处枚举形参改成 int 形参（用 pyarx::enumSetter 模板，C++ 签名不动）。"""
import io

COLOR = r"G:\PyArx\PyArxCore\PyGiColor.cpp"
MAT = r"G:\PyArx\PyArxCore\PyGiMaterialLight.cpp"

repl_color = [
    ('#include "PyErrorStatusException.h"',
     '#include "PyErrorStatusException.h"\n#include "PyEnumArg.h"'),
    ('.def("setMethod", &AcCmTransparency::setMethod, py::arg("method"))',
     '.def("setMethod", pyarx::enumSetter(&AcCmTransparency::setMethod), py::arg("method"))'),
    ('.def("setProjection", &AcGiMapper::setProjection, py::arg("projection"))',
     '.def("setProjection", pyarx::enumSetter(&AcGiMapper::setProjection), py::arg("projection"))'),
    ('.def("setAutoTransform", &AcGiMapper::setAutoTransform, py::arg("auto_transform"))',
     '.def("setAutoTransform", pyarx::enumSetter(&AcGiMapper::setAutoTransform), py::arg("auto_transform"))'),
    ('.def("setUTiling", &AcGiMapper::setUTiling, py::arg("tiling"))',
     '.def("setUTiling", pyarx::enumSetter(&AcGiMapper::setUTiling), py::arg("tiling"))'),
    ('.def("setVTiling", &AcGiMapper::setVTiling, py::arg("tiling"))',
     '.def("setVTiling", pyarx::enumSetter(&AcGiMapper::setVTiling), py::arg("tiling"))'),
]

repl_mat = [
    ('#include "AcGiEnvironment.h"',
     '#include "AcGiEnvironment.h"\n#include "PyEnumArg.h"'),
    ('.def("setMethod", &AcGiMaterialColor::setMethod)',
     '.def("setMethod", pyarx::enumSetter(&AcGiMaterialColor::setMethod), py::arg("method"))'),
    ('.def("setAttenuationType", &AcGiLightAttenuation::setAttenuationType)',
     '.def("setAttenuationType", pyarx::enumSetter(&AcGiLightAttenuation::setAttenuationType), py::arg("typ"))'),
    ('.def("setFilter", &AcGiMaterialMap::setFilter)',
     '.def("setFilter", pyarx::enumSetter(&AcGiMaterialMap::setFilter), py::arg("filter"))'),
    ('.def("setSource", &AcGiMaterialMap::setSource)',
     '.def("setSource", pyarx::enumSetter(&AcGiMaterialMap::setSource), py::arg("source"))'),
    ('.def("setChannelFlags", &AcGiMaterialTraits::setChannelFlags)',
     '.def("setChannelFlags", pyarx::enumSetter(&AcGiMaterialTraits::setChannelFlags), py::arg("flags"))'),
    ('.def("setIlluminationModel", &AcGiMaterialTraits::setIlluminationModel)',
     '.def("setIlluminationModel", pyarx::enumSetter(&AcGiMaterialTraits::setIlluminationModel), py::arg("model"))'),
    ('.def("setMode", &AcGiMaterialTraits::setMode)',
     '.def("setMode", pyarx::enumSetter(&AcGiMaterialTraits::setMode), py::arg("value"))'),
    ('.def("setExtendedLightShape", &AcGiShadowParameters::setExtendedLightShape)',
     '.def("setExtendedLightShape", pyarx::enumSetter(&AcGiShadowParameters::setExtendedLightShape), py::arg("shape"))'),
    ('.def("setShadowType", &AcGiShadowParameters::setShadowType)',
     '.def("setShadowType", pyarx::enumSetter(&AcGiShadowParameters::setShadowType), py::arg("typ"))'),
]


def apply(path, pairs):
    text = io.open(path, encoding="utf-8").read()
    done, miss = 0, []
    for old, new in pairs:
        if old not in text:
            miss.append(old[:60])
            continue
        text = text.replace(old, new, 1)
        done += 1
    io.open(path, "w", encoding="utf-8", newline="").write(text)
    return done, miss


for path, pairs in ((COLOR, repl_color), (MAT, repl_mat)):
    done, miss = apply(path, pairs)
    print("%s: replaced=%d miss=%s" % (path.split("\\")[-1], done, miss))
