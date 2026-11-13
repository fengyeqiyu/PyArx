#pragma once
// SVG 场景：把一份 SVG 解析成"折线轮廓 + 填充/描边样式"，再用 XmlCanvas 的
// 图元画出来。解析用 nanosvg（单头文件、MIT），渲染仍走本栈自己的 D2D/DWrite，
// 不引入第二套渲染引擎。
//
// 为什么不是"整张 SVG 位图化"：面板要能跟着缩放/平移重画，位图一放大就糊，
// 所以只取几何（轮廓折线）和样式，绘制由 XmlCanvas 每帧重做，任意倍率都清晰。
#include <string>
#include <vector>
#include "XmlCore.h"
#include "XmlDraw.h"

namespace xmlsvg
{
    // 模型坐标（SVG 用户单位，左上原点、y 向下，与屏幕一致）
    struct SvgPoint
    {
        float x = 0.0f;
        float y = 0.0f;
    };

    struct SvgContour
    {
        std::vector<SvgPoint> pts;   // 已把贝塞尔拍平成折线
        bool closed = false;
    };

    struct SvgShape
    {
        std::vector<SvgContour> contours;
        bool hasFill = false;
        COLORREF fill = 0;
        bool hasStroke = false;
        COLORREF stroke = 0;
        float strokeWidth = 1.0f;    // SVG 用户单位
        float opacity = 1.0f;
        std::wstring id;             // 原 SVG 的 id（用于命中测试/按元素高亮）
    };

    struct SvgText
    {
        std::wstring text;           // 已解码 XML 实体
        float x = 0.0f;              // 锚点坐标（已叠加 group 的 translate）
        float y = 0.0f;              // 基线 y
        float sizePx = 12.0f;        // font-size（像素）
        COLORREF color = 0;
        int weight = 400;            // font-weight
        int anchor = 0;              // 0=start 1=middle 2=end
        std::wstring family;         // 首个可用字族名
        float opacity = 1.0f;
    };

    struct SvgScene
    {
        std::vector<SvgShape> shapes;
        std::vector<SvgText> texts;  // nanosvg 不处理 <text>，这里自己扫出来用 DWrite 画
        float width = 0.0f;          // viewBox / width-height 决定的画布尺寸
        float height = 0.0f;
        int pathCount = 0;           // 原始 <path> 条数，便于判断解析是否丢图元
        std::vector<std::wstring> warnings;
    };

    // text 按 UTF-8 传入；失败时 err 说明原因（空文档、没有可用图元等）
    XML_CORE_API bool parseSvgText(const std::string& utf8, SvgScene& out, std::wstring& err);
    XML_CORE_API bool loadSvgFile(const std::wstring& path, SvgScene& out, std::wstring& err);

    // 把场景画到画布：模型坐标 -> 屏幕 = p * scale + (offsetX, offsetY)
    XML_CORE_API void drawSvgScene(XmlCanvas& canvas, const SvgScene& scene,
                                    float scale, int offsetX, int offsetY);
}
