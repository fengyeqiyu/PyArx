#pragma once
// ===========================================================================
// PyGiMaterialLight.h —— 材质 / 灯光 / 渲染环境三块 AcGi 接口
//   acgimaterial.h : AcGiMaterialTraits / AcGiMaterialColor / AcGiMaterialMap /
//                    AcGiMaterialTexture 家族
//   AcGiLightTraits.h : AcGiLightTraits 家族 / AcGiShadowParameters /
//                    AcGiLightAttenuation / AcGiSkyParameters
//   AcGiEnvironment.h : 背景 traits 家族 / 渲染设置 traits / 色调与曝光参数
//
// 说明：Traits 一族在 SDK 里全部是纯虚接口，实例只能由 CAD 渲染侧给出，
// 因此绑定不带构造函数，用于类型判断（isinstance）与未来拿到实例后调用。
// 值类/参数类（MaterialColor/Map/Texture、ShadowParameters、LightAttenuation、
// SkyParameters、ToneOperatorParameters、PhotographicExposureParameters）
// 可直接在 Python 侧 new。
// ===========================================================================
#include "stdafx.h"

void bind_PyGiMaterial(pym& m);
void bind_PyGiLight(pym& m);
void bind_PyGiEnvironment(pym& m);
