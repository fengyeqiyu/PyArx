// XmlMatrix4.h —— 统一的 4×4 变换矩阵（列向量约定：p' = M · p）。
//
// 为什么是 4×4 而不是 2×3：2×3 只能表达平面仿射（平移/缩放/旋转/斜切）。
// 但 XML 通道迟早要接 CSS 的 perspective / rotateX / rotateY / matrix3d，
// 以及画布自身的视图变换（cad2d 的缩放平移 + 未来的俯视图）。这些一旦出现，
// 2×3 就得推倒重写；而 4×4 从一开始就能涵盖全部线性变换，只是当前绘制端
// 取它的 2D 投影去喂 D2D（D2D 只认 2×3，投影这一步是纯机械的降维）。
//
// 存储：行主序 m[row][col]，等价于 m[row*4+col]。
//   m = | m0 m1 m2 m3 |   第三列 (m3,m7,m11,m15) 是平移分量（列向量约定）
//       | m4 m5 m6 m7 |
//       | m8 m9 mA mB |
//       | mC mD mE mF |
#pragma once

#include <cmath>
#include <string>

namespace xmlgeo
{
    struct XmlMatrix4
    {
        float m[16];

        XmlMatrix4()
        {
            for (int i = 0; i < 16; ++i)
                m[i] = (i % 5 == 0) ? 1.0f : 0.0f;   // 单位阵
        }

        static XmlMatrix4 Identity() { return XmlMatrix4(); }

        static XmlMatrix4 Translate(float tx, float ty, float tz = 0.0f)
        {
            XmlMatrix4 r;
            r.m[3] = tx;
            r.m[7] = ty;
            r.m[11] = tz;
            return r;
        }

        static XmlMatrix4 Scale(float sx, float sy, float sz = 1.0f)
        {
            XmlMatrix4 r;
            r.m[0] = sx;
            r.m[5] = sy;
            r.m[10] = sz;
            return r;
        }

        // 绕 Z 轴旋转（CSS transform:rotate(deg)，正角为屏幕顺时针，与 CSS 一致）
        static XmlMatrix4 RotateZ(float deg)
        {
            const float a = deg * 3.14159265358979323846f / 180.0f;
            const float c = std::cos(a);
            const float s = std::sin(a);
            XmlMatrix4 r;
            r.m[0] = c;  r.m[1] = -s;
            r.m[4] = s;  r.m[5] = c;
            return r;
        }

        // 绕任意轴（1=X, 2=Y, 3=Z），接 CSS rotateX / rotateY 与未来的 3D 视图
        static XmlMatrix4 RotateAxis(int axis, float deg)
        {
            if (axis == 3)
                return RotateZ(deg);
            const float a = deg * 3.14159265358979323846f / 180.0f;
            const float c = std::cos(a);
            const float s = std::sin(a);
            XmlMatrix4 r;
            if (axis == 1)          // 绕 X：y/z 平面转
            {
                r.m[5] = c;  r.m[6] = -s;
                r.m[9] = s;  r.m[10] = c;
            }
            else                    // 绕 Y：x/z 平面转
            {
                r.m[0] = c;  r.m[2] = s;
                r.m[8] = -s; r.m[10] = c;
            }
            return r;
        }

        static XmlMatrix4 SkewX(float deg)
        {
            XmlMatrix4 r;
            r.m[1] = std::tan(deg * 3.14159265358979323846f / 180.0f);
            return r;
        }

        static XmlMatrix4 SkewY(float deg)
        {
            XmlMatrix4 r;
            r.m[4] = std::tan(deg * 3.14159265358979323846f / 180.0f);
            return r;
        }

        // 复合：a * b 表示"先应用 b，再应用 a"（与 CSS transform 列表的直觉一致：
        // `transform: A B` 等价于先 B 后 A，即 M = A · B）
        static XmlMatrix4 Multiply(const XmlMatrix4& a, const XmlMatrix4& b)
        {
            XmlMatrix4 r;
            for (int i = 0; i < 4; ++i)
                for (int j = 0; j < 4; ++j)
                {
                    float s = 0.0f;
                    for (int k = 0; k < 4; ++k)
                        s += a.m[i * 4 + k] * b.m[k * 4 + j];
                    r.m[i * 4 + j] = s;
                }
            return r;
        }

        // 绕点 (cx,cy) 变换：T(c) · M · T(-c)
        XmlMatrix4 AboutPoint(float cx, float cy, float cz = 0.0f) const
        {
            return Multiply(Translate(cx, cy, cz), Multiply(*this, Translate(-cx, -cy, -cz)));
        }

        void TransformPoint(float x, float y, float z, float& ox, float& oy) const
        {
            ox = m[0] * x + m[1] * y + m[2] * z + m[3];
            oy = m[4] * x + m[5] * y + m[6] * z + m[7];
        }

        bool IsIdentity2D() const
        {
            return m[0] == 1.0f && m[1] == 0.0f && m[2] == 0.0f && m[3] == 0.0f &&
                   m[4] == 0.0f && m[5] == 1.0f && m[6] == 0.0f && m[7] == 0.0f &&
                   m[10] == 1.0f && m[11] == 0.0f;
        }
    };

    // 取一个浮点数值 + 可选单位（px / % / deg / 无单位）。百分比按 ref 折算。
    inline float ParseCssNumber(const std::wstring& raw, float ref, bool& ok)
    {
        ok = false;
        std::wstring s = raw;
        while (!s.empty() && (s.front() == L' ' || s.front() == L'\t'))
            s.erase(s.begin());
        while (!s.empty() && (s.back() == L' ' || s.back() == L'\t'))
            s.pop_back();
        if (s.empty())
            return 0.0f;
        wchar_t* end = nullptr;
        const double v = wcstod(s.c_str(), &end);
        std::wstring unit = (end != nullptr) ? std::wstring(end) : std::wstring();
        for (auto& c : unit)
            c = static_cast<wchar_t>(towlower(c));
        ok = true;
        if (unit == L"%")  return static_cast<float>(v * ref / 100.0);
        if (unit == L"em") return static_cast<float>(v * 16.0);   // 近似：1em ≈ 16px
        return static_cast<float>(v);                              // px / deg / 无单位
    }

    // 把一段长度值（"20px" / "50%" / "30"）折成像素
    inline bool ParseCssLength(const std::wstring& raw, float ref, float& out)
    {
        bool ok = false;
        out = ParseCssNumber(raw, ref, ok);
        return ok;
    }

    // CSS transform 解析：支持 translate / translateX / translateY / translate3d /
    // rotate / rotateZ / scale / scaleX / scaleY / skewX / skewY / matrix(a,b,c,d,e,f)。
    // 百分比按元素盒子尺寸折算（HTML 里 translate 的百分比就是相对自身盒子）。
    // 解析失败返回 false，调用方保持不变换（宁可不变形，也不要画歪）。
    inline bool ParseCssTransform(const std::wstring& spec, float boxW, float boxH, XmlMatrix4& out)
    {
        out = XmlMatrix4::Identity();
        if (spec.empty())
            return false;

        bool any = false;
        size_t i = 0;
        const size_t n = spec.size();
        while (i < n)
        {
            while (i < n && (spec[i] == L' ' || spec[i] == L'\t' || spec[i] == L','))
                ++i;
            if (i >= n)
                break;

            // 函数名
            size_t nameStart = i;
            while (i < n && (iswalnum(spec[i]) || spec[i] == L'_'))
                ++i;
            const std::wstring fn = spec.substr(nameStart, i - nameStart);
            if (i >= n || spec[i] != L'(')
                return false;    // 语法不认识：整条放弃，不动控件
            size_t close = spec.find(L')', i);
            if (close == std::wstring::npos)
                return false;
            const std::wstring argsRaw = spec.substr(i + 1, close - i - 1);
            i = close + 1;

            // 拆参数（逗号或空白分隔）
            std::wstring args[6];
            {
                int cnt = 0;
                std::wstring cur;
                for (size_t k = 0; k <= argsRaw.size(); ++k)
                {
                    const wchar_t c = (k < argsRaw.size()) ? argsRaw[k] : L' ';
                    if (c == L',' || c == L' ' || c == L'\t')
                    {
                        if (!cur.empty() && cnt < 6)
                        {
                            args[cnt++] = cur;
                            cur.clear();
                        }
                    }
                    else
                        cur.push_back(c);
                }
            }

            std::wstring f = fn;
            for (auto& c : f)
                c = static_cast<wchar_t>(towlower(c));

            XmlMatrix4 step = XmlMatrix4::Identity();
            if (f == L"translate" || f == L"translate3d" || f == L"translatez")
            {
                bool ok = false;
                float x = 0.0f, y = 0.0f;
                if (!args[0].empty())
                    x = ParseCssNumber(args[0], boxW, ok);
                if (!args[1].empty())
                    y = ParseCssNumber(args[1], boxH, ok);
                step = XmlMatrix4::Translate(x, y);
            }
            else if (f == L"translatex")
            {
                bool ok = false;
                step = XmlMatrix4::Translate(ParseCssNumber(args[0], boxW, ok), 0.0f);
            }
            else if (f == L"translatey")
            {
                bool ok = false;
                step = XmlMatrix4::Translate(0.0f, ParseCssNumber(args[0], boxH, ok));
            }
            else if (f == L"rotate" || f == L"rotatez")
            {
                bool ok = false;
                step = XmlMatrix4::RotateZ(ParseCssNumber(args[0], 1.0f, ok));
            }
            else if (f == L"rotatex" || f == L"rotatey")
            {
                bool ok = false;
                step = XmlMatrix4::RotateAxis((f == L"rotatex") ? 1 : 2, ParseCssNumber(args[0], 1.0f, ok));
            }
            else if (f == L"scale")
            {
                bool ok = false;
                const float sx = ParseCssNumber(args[0], 1.0f, ok);
                float sy = sx;
                if (!args[1].empty())
                    sy = ParseCssNumber(args[1], 1.0f, ok);
                step = XmlMatrix4::Scale(sx, sy);
            }
            else if (f == L"scalex" || f == L"scaley")
            {
                bool ok = false;
                const float v = ParseCssNumber(args[0], 1.0f, ok);
                step = (f == L"scalex") ? XmlMatrix4::Scale(v, 1.0f) : XmlMatrix4::Scale(1.0f, v);
            }
            else if (f == L"skewx")
            {
                bool ok = false;
                step = XmlMatrix4::SkewX(ParseCssNumber(args[0], 1.0f, ok));
            }
            else if (f == L"skewy")
            {
                bool ok = false;
                step = XmlMatrix4::SkewY(ParseCssNumber(args[0], 1.0f, ok));
            }
            else if (f == L"matrix")
            {
                if (args[0].empty() || args[3].empty())
                    return false;
                bool ok = false;
                const float a = ParseCssNumber(args[0], 1.0f, ok);
                const float b = ParseCssNumber(args[1], 1.0f, ok);
                const float c = ParseCssNumber(args[2], 1.0f, ok);
                const float d = ParseCssNumber(args[3], 1.0f, ok);
                const float e = args[4].empty() ? 0.0f : ParseCssNumber(args[4], 1.0f, ok);
                const float g = args[5].empty() ? 0.0f : ParseCssNumber(args[5], 1.0f, ok);
                step.m[0] = a; step.m[1] = c; step.m[3] = e;
                step.m[4] = b; step.m[5] = d; step.m[7] = g;
            }
            else
            {
                return false;    // 未知函数：整条不生效，交回作者检查
            }

            // 列表里靠前的在外层：M = 前项 · 后项
            out = XmlMatrix4::Multiply(out, step);
            any = true;
        }
        return any;
    }
}
