// PyMfcHost.Anim.cpp —— 拆分自 PyMfcHost.cpp（按职责分组；接口声明仍在 PyMfcHost.h）。
#include "PyMfcHostInternal.h"

void PyMfcHost::stepSpin(int id, int delta)
{
        setSpinValue(id, getSpinValue(id) + delta);
    }

void PyMfcHost::bumpBusyPhase(int id)
{
        auto it = m_busyPhase.find(id);
        m_busyPhase[id] = (it != m_busyPhase.end() ? it->second : 0) + 1;
    }

int PyMfcHost::flickTick(int id)
{
        if (m_flBase.count(id))
            return flickTickFlickable(id);
        auto it = m_flickVel.find(id);
        int v = (it != m_flickVel.end()) ? it->second : 0;
        int over = m_lvOver.count(id) ? m_lvOver[id] : 0;
        if (v == 0 && over == 0)
            return 0;

        const ULONGLONG now = ::GetTickCount64();
        auto st = m_flickStamp.find(id);
        const int dt = (st != m_flickStamp.end() && now > st->second)
                           ? static_cast<int>(now - st->second) : 16;
        m_flickStamp[id] = now;
        const int adv = v * (dt > 0 ? dt : 16) / 1000;

        const int maxSc = lvMaxScroll(id);
        const int raw = getListScroll(id) + over + adv;
        int sc = raw;
        if (raw < 0)
        {
            sc = 0;
            over = raw;                     // 顶端越界：值越小，内容向下多滑出越多
        }
        else if (raw > maxSc)
        {
            sc = maxSc;
            over = raw - maxSc;
        }
        else
        {
            over = 0;
        }
        if (over < -96) over = -96;
        if (over > 96) over = 96;

        if (over != 0)
        {
            // 触边：速度被吃掉，只留弹性收敛（每帧 0.86 收，收到 2px 内归零）
            v = 0;
            over = static_cast<int>(over * 0.86);
            if (over > -2 && over < 2)
                over = 0;
        }
        else
        {
            v = static_cast<int>(v * 0.92);
            if (v > -40 && v < 40)
                v = 0;
        }

        setListScroll(id, sc);
        m_lvOver[id] = over;
        m_flickVel[id] = v;
        return (v != 0 || over != 0) ? 1 : 0;
    }

void PyMfcHost::flickMetrics(int id, int& viewW, int& viewH, int& cw, int& ch,
                      int& minX, int& minY) const
{
        viewW = viewH = 0;
        HWND h = findByIdDeep(m_hWnd, id);
        if (h != nullptr)
        {
            RECT rc = {};
            ::GetClientRect(h, &rc);
            viewW = rc.right;
            viewH = rc.bottom;
        }
        auto itCW = m_flCW.find(id);
        auto itCH = m_flCH.find(id);
        cw = itCW != m_flCW.end() ? itCW->second : 0;
        ch = itCH != m_flCH.end() ? itCH->second : 0;
        auto itBase = m_flBase.find(id);
        if (itBase != m_flBase.end())
            for (const auto& kv : itBase->second)
            {
                if (kv.second.right > cw) cw = kv.second.right;
                if (kv.second.bottom > ch) ch = kv.second.bottom;
            }
        if (cw < viewW) cw = viewW;
        if (ch < viewH) ch = viewH;
        minX = viewW - cw;          // 负值：可向左推的距离
        minY = viewH - ch;
    }

void PyMfcHost::flickApply(int id)
{
        auto itBase = m_flBase.find(id);
        if (itBase == m_flBase.end())
            return;
        const POINT off = m_flOff.count(id) ? m_flOff[id] : POINT{0, 0};
        HWND hfl = findByIdDeep(m_hWnd, id);
        if (hfl == nullptr)
            return;
        for (const auto& kv : itBase->second)
        {
            HWND hk = ::GetDlgItem(hfl, kv.first);
            if (hk == nullptr)
                continue;
            const RECT& b = kv.second;
            ::SetWindowPos(hk, nullptr, b.left + off.x, b.top + off.y,
                           b.right - b.left, b.bottom - b.top,
                           SWP_NOZORDER | SWP_NOACTIVATE);
        }
    }

int PyMfcHost::flickTickFlickable(int id)
{
        if (!m_flOff.count(id))
            return 0;
        const ULONGLONG now = ::GetTickCount64();
        auto st = m_flStamp.find(id);
        const int dt = (st != m_flStamp.end() && now > st->second)
                           ? static_cast<int>(now - st->second) : 16;
        m_flStamp[id] = now;
        auto vt = m_flVel.find(id);
        int vx = vt != m_flVel.end() ? vt->second.x : 0;
        int vy = vt != m_flVel.end() ? vt->second.y : 0;
        int vw, vh, cw, ch, minX, minY;
        flickMetrics(id, vw, vh, cw, ch, minX, minY);
        POINT& off = m_flOff[id];
        off.x += vx * (dt > 0 ? dt : 16) / 1000;
        off.y += vy * (dt > 0 ? dt : 16) / 1000;
        if (off.x > 0)
        {
            off.x = static_cast<int>(off.x * 0.86);
            if (off.x < 3) off.x = 0;
            vx = 0;
        }
        else if (off.x < minX)
        {
            off.x = minX + static_cast<int>((off.x - minX) * 0.86);
            if (off.x - minX > -3) off.x = minX;
            vx = 0;
        }
        else
        {
            vx = static_cast<int>(vx * 0.92);
            if (vx > -40 && vx < 40) vx = 0;
        }
        if (off.y > 0)
        {
            off.y = static_cast<int>(off.y * 0.86);
            if (off.y < 3) off.y = 0;
            vy = 0;
        }
        else if (off.y < minY)
        {
            off.y = minY + static_cast<int>((off.y - minY) * 0.86);
            if (off.y - minY > -3) off.y = minY;
            vy = 0;
        }
        else
        {
            vy = static_cast<int>(vy * 0.92);
            if (vy > -40 && vy < 40) vy = 0;
        }
        if (vt != m_flVel.end())
            vt->second = POINT{vx, vy};
        flickApply(id);
        ::InvalidateRect(findByIdDeep(m_hWnd, id), nullptr, TRUE);
        // 还在动，或者还停在越界位置（要继续收敛）才继续跑
        const bool busy = vx != 0 || vy != 0 || off.x > 0 || off.x < minX ||
                          off.y > 0 || off.y < minY;
        return busy ? 1 : 0;
    }

void PyMfcHost::pageSlide(int id, int which, int offset)
{
        auto it = m_pgPages.find(id);
        if (it == m_pgPages.end() || which < 0 || which >= static_cast<int>(it->second.size()))
            return;
        const RECT hr = pageHostRect(id);
        const auto& kids = it->second[which];
        if (kids.empty())
            return;
        HDWP dwp = ::BeginDeferWindowPos(static_cast<int>(kids.size()));
        for (int kid : kids)
        {
            HWND hk = ::GetDlgItem(m_hWnd, kid);
            if (hk == nullptr)
                continue;
            auto b = m_pgBase[id].find(kid);
            if (b == m_pgBase[id].end())
                continue;
            const RECT& base = b->second;
            const int bx = base.left - hr.left;
            const int by = base.top - hr.top;
            const int w = base.right - base.left;
            const int h = base.bottom - base.top;
            if (dwp != nullptr)
            {
                // 不能用 SWP_NOREDRAW：它连"把窗口像素搬到新位置"都不做，
                // 子窗会在原位留一块残影、新位置空着，父窗一重画就闪成一片。
                dwp = ::DeferWindowPos(dwp, hk, nullptr, hr.left + bx + offset, hr.top + by, w, h,
                                       SWP_NOZORDER | SWP_NOACTIVATE);
                if (dwp != nullptr)
                    continue;
            }
            // 批量句柄拿不到时退回逐个移动（功能优先，慢一点也要动）
            ::SetWindowPos(hk, nullptr, hr.left + bx + offset, hr.top + by, w, h,
                           SWP_NOZORDER | SWP_NOACTIVATE);
        }
        if (dwp != nullptr)
            ::EndDeferWindowPos(dwp);
    }

void PyMfcHost::viewAnimate(int id, int index, int durMs)
{
        const int cnt = viewCount(id);
        if (cnt <= 0)
        {
            m_pgIdx[id] = index;
            return;
        }
        if (index < 0) index = 0;
        if (index >= cnt) index = cnt - 1;
        const int old = viewIndex(id);
        if (old == index)
            return;
        if (durMs <= 0)
        {
            viewSetIndex(id, index);
            return;
        }
        m_pgAnimFrom[id] = old;
        m_pgAnimTo[id] = index;
        m_pgAnimStart[id] = ::GetTickCount64();
        m_pgAnimDur[id] = durMs;
        m_pgIdx[id] = index;
        HWND hh = findByIdDeep(m_hWnd, id);
        if (hh != nullptr)
            ::SetTimer(hh, kPageTimerId, 16, nullptr);
    }

int PyMfcHost::pageTick(int id)
{
        if (!m_pgAnimDur.count(id))
            return 0;
        const int dur = m_pgAnimDur[id];
        const ULONGLONG now = ::GetTickCount64();
        const int t = static_cast<int>(now - m_pgAnimStart[id]);
        const RECT hr = pageHostRect(id);
        const int w = hr.right - hr.left;
        if (t >= dur)
        {
            pageLayout(id, m_pgAnimFrom[id], 0, false);
            pageLayout(id, m_pgAnimTo[id], 0, true);
            m_pgAnimDur.erase(id);
            ::InvalidateRect(findByIdDeep(m_hWnd, id), nullptr, TRUE);
            return 0;
        }
        // 缓动 outCubic：1-(1-x)^3
        const double x = static_cast<double>(t) / dur;
        const double e = 1.0 - (1.0 - x) * (1.0 - x) * (1.0 - x);
        const int off = static_cast<int>(w * e);
        pageSlide(id, m_pgAnimFrom[id], -off);
        pageSlide(id, m_pgAnimTo[id], w - off);
        // 只补容器空出来的条纹，不擦底、不逐个子窗重画
        ::InvalidateRect(findByIdDeep(m_hWnd, id), nullptr, FALSE);
        return 1;
    }

double PyMfcHost::easeAt(const std::string& ease, double t)
{
        if (t <= 0.0) return 0.0;
        if (t >= 1.0) return 1.0;
        auto pow2 = [](double v) { return v * v; };
        auto pow3 = [](double v) { return v * v * v; };
        if (ease == "linear") return t;
        if (ease == "inQuad") return pow2(t);
        if (ease == "outQuad") return 1.0 - pow2(1.0 - t);
        if (ease == "inOutQuad") return t < 0.5 ? 2.0 * pow2(t) : 1.0 - 2.0 * pow2(1.0 - t);
        if (ease == "inCubic") return pow3(t);
        if (ease == "outCubic") return 1.0 - pow3(1.0 - t);
        if (ease == "inOutCubic")
            return t < 0.5 ? 4.0 * pow3(t) : 1.0 - 4.0 * pow3(1.0 - t);
        if (ease == "outBack")
        {
            const double c1 = 1.70158, c3 = c1 + 1.0;
            const double u = t - 1.0;
            return 1.0 + c3 * u * u * u + c1 * u * u;
        }
        if (ease == "outElastic")
        {
            const double c4 = (2.0 * 3.14159265358979) / 3.0;
            return std::pow(2.0, -10.0 * t) * std::sin((t * 10.0 - 0.75) * c4) + 1.0;
        }
        if (ease == "spring")
        {
            // 临界阻尼附近的解析解：先冲过 1 再回落，收尾为 1
            const double w = 12.0;
            return 1.0 - std::exp(-6.0 * t) * (std::cos(w * t) + 0.5 * std::sin(w * t));
        }
        if (ease.compare(0, 7, "bezier(") == 0)
        {
            double x1 = 0.25, y1 = 0.1, x2 = 0.25, y2 = 1.0;
            double v[4] = {0, 0, 0, 0};
            std::sscanf(ease.c_str(), "bezier(%lf,%lf,%lf,%lf)", &v[0], &v[1], &v[2], &v[3]);
            x1 = v[0]; y1 = v[1]; x2 = v[2]; y2 = v[3];
            auto bez = [](double a, double b, double u) {
                const double m = 1.0 - u;
                return 3.0 * m * m * u * a + 3.0 * m * u * u * b + u * u * u;
            };
            double lo = 0.0, hi = 1.0, u = t;
            for (int i = 0; i < 24; ++i)   // 二分求参数，稳过牛顿
            {
                u = (lo + hi) / 2.0;
                const double x = bez(x1, x2, u);
                if (x < t) lo = u; else hi = u;
            }
            return bez(y1, y2, (lo + hi) / 2.0);
        }
        return t;
    }

COLORREF PyMfcHost::lerpColor(COLORREF a, COLORREF b, double p)
{
        auto ch = [](COLORREF c, int shift) { return (int)((c >> shift) & 0xFF); };
        auto mix = [&](int shift) {
            const double v = ch(a, shift) + (ch(b, shift) - ch(a, shift)) * p;
            return (int)(v < 0 ? 0 : (v > 255 ? 255 : v));
        };
        return RGB(mix(0), mix(8), mix(16));
    }

void PyMfcHost::ensureAnimWnd()
{
        if (m_animWnd != nullptr)
            return;
        WNDCLASSEXW wc = {sizeof(wc)};
        wc.style = 0;
        wc.lpfnWndProc = &PyMfcHost::animHostProc;
        wc.hInstance = AfxGetInstanceHandle();
        wc.lpszClassName = kAnimHostClass;
        ::RegisterClassExW(&wc);   // 已注册会失败，忽略即可
        m_animWnd = ::CreateWindowExW(0, kAnimHostClass, L"", 0, 0, 0, 0, 0,
                                      HWND_MESSAGE, nullptr, wc.hInstance, this);
    }

void PyMfcHost::animAdd(const PyMfcHost::AnimTrack& tr)
{
        if (m_anims.empty())
            m_animStartMs = static_cast<DWORD>(::GetTickCount64());
        // 每条轨道自带绝对起点：全局时间线只在新轨道加入时才重置，若期间还有别的
        // 未完成轨道，新轨道会被旧起点拖到"一上来就快结束"。
        AnimTrack seat = tr;
        seat.startMs = static_cast<DWORD>(::GetTickCount64());
        m_anims.push_back(seat);
        ensureAnimWnd();
        if (!m_animTickOn && m_animWnd != nullptr)
        {
            ::SetTimer(m_animWnd, kAnimTimerId, 16, nullptr);   // ~60fps
            m_animTickOn = true;
        }
    }

void PyMfcHost::applyAnimValue(const PyMfcHost::AnimTrack& tr, double v, COLORREF c)
{
        HWND h = ::GetDlgItem(m_hWnd, tr.id);
        if (tr.color)
        {
            if (m_ctrlBack.count(tr.id) && m_ctrlBack[tr.id] == c)
                return;                       // 同色不重画
            m_ctrlBack[tr.id] = c;
            if (h != nullptr)
                ::InvalidateRect(h, nullptr, TRUE);
            return;
        }
        m_animApplying = true;
        if (tr.prop == L"X" || tr.prop == L"Y" || tr.prop == L"Width" || tr.prop == L"Height")
        {
            if (h != nullptr)
            {
                RECT rc = {};
                ::GetWindowRect(h, &rc);
                POINT tl = {rc.left, rc.top};
                POINT br = {rc.right, rc.bottom};
                ::MapWindowPoints(HWND_DESKTOP, m_hWnd, &tl, 1);
                ::MapWindowPoints(HWND_DESKTOP, m_hWnd, &br, 1);
                int x = tl.x, y = tl.y;
                int w = br.x - tl.x, hgt = br.y - tl.y;
                if (tr.prop == L"X") x = static_cast<int>(v);
                else if (tr.prop == L"Y") y = static_cast<int>(v);
                else if (tr.prop == L"Width") w = static_cast<int>(v);
                else hgt = static_cast<int>(v);
                if (w < 1) w = 1;
                if (hgt < 1) hgt = 1;
                ::SetWindowPos(h, nullptr, x, y, w, hgt,
                               SWP_NOZORDER | SWP_NOACTIVATE);
                // 老矩形与新矩形的并集都可能"变回背景/露出兄弟"，交给兄弟重画
                RECT oldRc = { tl.x, tl.y, tl.x + (br.x - tl.x), tl.y + (br.y - tl.y) };
                RECT newRc = { x, y, x + w, y + hgt };
                repairSiblingsOf(h, oldRc, newRc);
                ::InvalidateRect(m_hWnd, &oldRc, TRUE);
                ::InvalidateRect(m_hWnd, &newRc, TRUE);
            }
        }
        else if (tr.prop == L"Value")
        {
            int kind = 0;
            auto it = m_skinKinds.find(tr.id);
            if (it != m_skinKinds.end())
                kind = it->second;
            if (kind == kSkinBoxSpin) setSpinValue(tr.id, static_cast<int>(v));
            else if (kind == kSkinBoxDial) setDial(tr.id, static_cast<int>(v));
            else if (kind == kSkinBoxScrollBar) setScrollBarValue(tr.id, static_cast<int>(v));
            else if (kind == kSkinBoxRange) setRangeValues(tr.id, rangeMinOf(tr.id),
                                                           static_cast<int>(v));
            else if (h != nullptr)
            {
                wchar_t cls[64] = {0};
                ::GetClassNameW(h, cls, 64);
                if (_wcsicmp(cls, L"msctls_trackbar32") == 0)
                    ::SendMessageW(h, TBM_SETPOS, TRUE, static_cast<LPARAM>(static_cast<int>(v)));
                else if (_wcsicmp(cls, L"msctls_progress32") == 0)
                    ::SendMessageW(h, PBM_SETPOS, static_cast<WPARAM>(static_cast<int>(v)), 0);
            }
        }
        m_animApplying = false;
    }

double PyMfcHost::currentPropValue(int id, const std::wstring& prop) const
{
        if (prop == L"Value")
        {
            int kind = 0;
            auto it = m_skinKinds.find(id);
            if (it != m_skinKinds.end())
                kind = it->second;
            if (kind == kSkinBoxSpin) return getSpinValue(id);
            if (kind == kSkinBoxDial) return getDial(id);
            if (kind == kSkinBoxScrollBar) return getScrollBarValue(id);
            if (kind == kSkinBoxRange) return getRangeHigh(id);
            if (HWND h = findByIdDeep(m_hWnd, id))
            {
                wchar_t cls[64] = {0};
                ::GetClassNameW(h, cls, 64);
                if (_wcsicmp(cls, L"msctls_trackbar32") == 0)
                    return static_cast<double>(::SendMessageW(h, TBM_GETPOS, 0, 0));
                if (_wcsicmp(cls, L"msctls_progress32") == 0)
                    return static_cast<double>(::SendMessageW(h, PBM_GETPOS, 0, 0));
            }
            return 0.0;
        }
        HWND h = findByIdDeep(m_hWnd, id);
        if (h == nullptr)
            return 0.0;
        RECT rc = {};
        ::GetWindowRect(h, &rc);
        POINT tl = {rc.left, rc.top};
        POINT br = {rc.right, rc.bottom};
        ::MapWindowPoints(HWND_DESKTOP, m_hWnd, &tl, 1);
        ::MapWindowPoints(HWND_DESKTOP, m_hWnd, &br, 1);
        if (prop == L"X") return tl.x;
        if (prop == L"Y") return tl.y;
        if (prop == L"Width") return br.x - tl.x;
        if (prop == L"Height") return br.y - tl.y;
        return 0.0;
    }

void PyMfcHost::animate(int id, const std::wstring& prop, double to, int dur,
                 const std::string& ease, int delay, bool loop)
{
        animateFrom(id, prop, currentPropValue(id, prop), to, dur, ease, delay, loop);
    }

void PyMfcHost::animateFrom(int id, const std::wstring& prop, double from, double to, int dur,
                     const std::string& ease, int delay, bool loop)
{
        AnimTrack tr;
        tr.id = id;
        tr.prop = prop;
        tr.from = from;
        tr.to = to;
        tr.t0 = delay < 0 ? 0 : delay;
        tr.dur = dur < 1 ? 1 : dur;
        tr.ease = ease;
        tr.loop = loop;
        animAdd(tr);
    }

void PyMfcHost::animateColor(int id, COLORREF to, int dur, const std::string& ease, int delay)
{
        AnimTrack tr;
        tr.id = id;
        tr.prop = L"Color";
        tr.color = true;
        // 起色优先取"屏幕上的当前色"：动画进行中就是 m_ctrlBack 的中间值；
        // 还没动画过就取控件声明的背景色（否则会从硬编码深灰起步，画面先跳一下）。
        if (m_ctrlBack.count(id))
        {
            tr.c0 = m_ctrlBack[id];
        }
        else
        {
            COLORREF c0 = RGB(49, 56, 66);
            const xmlapp::XmlAppearance* ap = findAppearance(id);
            if (ap != nullptr && !ap->backColor.empty())
                xmlapp::ParseColor(ap->backColor, c0);
            tr.c0 = c0;
        }
        tr.c1 = to;
        tr.t0 = delay < 0 ? 0 : delay;
        tr.dur = dur < 1 ? 1 : dur;
        tr.ease = ease;
        animAdd(tr);
    }

void PyMfcHost::animateState(int id, const std::wstring& prop, double to, int dur,
                      const std::string& ease, int delay)
{
        animate(id, prop, to, dur, ease, delay, false);
    }

void PyMfcHost::startCssKeyframes(int id, const std::wstring& spec, int durMs, bool loop)
{
        if (spec.empty() || durMs <= 0)
            return;
        CssKeyframeAnim a;
        a.id = id;
        a.durMs = durMs;
        a.loop = loop;
        // 帧表：`0:#ff0000;50:#00ff00;100:#ff0000`（分号分隔，百分比 : 颜色）
        size_t pos = 0;
        while (pos <= spec.size())
        {
            const size_t end = spec.find(L';', pos);
            std::wstring one = spec.substr(pos, end == std::wstring::npos ? std::wstring::npos : end - pos);
            pos = end == std::wstring::npos ? spec.size() + 1 : end + 1;
            const size_t colon = one.find(L':');
            if (colon == std::wstring::npos)
                continue;
            double pct = _wtof(one.substr(0, colon).c_str());
            std::wstring colStr = one.substr(colon + 1);
            while (!colStr.empty() && (colStr.back() == L' ' || colStr.back() == L'\t')) colStr.pop_back();
            COLORREF c = 0;
            if (!xmlapp::ParseColor(colStr, c))
                continue;
            if (pct < 0.0) pct = 0.0;
            if (pct > 100.0) pct = 100.0;
            a.frames.emplace_back(pct / 100.0, c);
        }
        if (a.frames.size() < 2)
            return;
        std::sort(a.frames.begin(), a.frames.end(),
                  [](const std::pair<double, COLORREF>& x, const std::pair<double, COLORREF>& y) {
                      return x.first < y.first;
                  });
        a.startMs = static_cast<DWORD>(::GetTickCount64());
        for (size_t i = 0; i < m_cssAnims.size(); ++i)
        {
            if (m_cssAnims[i].id == id)
            {
                m_cssAnims[i] = a;
                return;
            }
        }
        m_cssAnims.push_back(a);
        ensureAnimWnd();
        if (!m_animTickOn && m_animWnd != nullptr)
        {
            ::SetTimer(m_animWnd, kAnimTimerId, 16, nullptr);
            m_animTickOn = true;
        }
}

void PyMfcHost::startHoverTransition(int id, bool entering)
{
        auto tt = m_cssTransitionMs.find(id);
        if (tt == m_cssTransitionMs.end() || tt->second <= 0)
            return;
        const xmlapp::XmlAppearance* ap = findAppearance(id);
        if (ap == nullptr)
            return;
        const std::wstring& target = entering ? ap->hoverBackColor : ap->backColor;
        COLORREF to = 0;
        if (target.empty() || !xmlapp::ParseColor(target, to))
            return;
        COLORREF from = to;
        if (m_ctrlBack.count(id))
            from = m_ctrlBack[id];
        else
        {
            const std::wstring& other = entering ? ap->backColor : ap->hoverBackColor;
            COLORREF tmp = 0;
            if (!other.empty() && xmlapp::ParseColor(other, tmp))
                from = tmp;
        }
        if (from == to)
            return;
        // 同控件的旧颜色轨道先撤掉，避免连续 hover 叠出多份插值互相拉扯
        m_anims.erase(std::remove_if(m_anims.begin(), m_anims.end(),
                                     [id](const AnimTrack& t) { return t.id == id && t.color; }),
                      m_anims.end());
        AnimTrack tr;
        tr.id = id;
        tr.prop = L"Color";
        tr.color = true;
        tr.c0 = from;
        tr.c1 = to;
        tr.t0 = 0;
        tr.dur = tt->second;
        auto te = m_cssTransitionEase.find(id);
        if (te != m_cssTransitionEase.end() && !te->second.empty())
        {
            std::string ez;   // ease 名是 ASCII（linear / ease-in-out / cubic-bezier(..)）
            for (wchar_t c : te->second)
                ez.push_back(static_cast<char>(c));
            tr.ease = ez;
        }
        else
            tr.ease = "easeInOut";
        animAdd(tr);
}

void PyMfcHost::stopAnimations(int id)
{
        if (id == 0)
            m_anims.clear();
        else
            m_anims.erase(std::remove_if(m_anims.begin(), m_anims.end(),
                                         [id](const AnimTrack& t) { return t.id == id; }),
                          m_anims.end());
        if (m_anims.empty() && m_animTickOn && m_animWnd != nullptr)
        {
            ::KillTimer(m_animWnd, kAnimTimerId);
            m_animTickOn = false;
        }
    }

void PyMfcHost::setBehavior(int id, const std::wstring& prop, int dur, const std::string& ease)
{
        m_behaviors[std::to_wstring(id) + L"|" + prop] = {dur, ease};
    }

bool PyMfcHost::behaviorFor(int id, const std::wstring& prop, int& dur, std::string& ease) const
{
        auto it = m_behaviors.find(std::to_wstring(id) + L"|" + prop);
        if (it == m_behaviors.end())
            return false;
        dur = it->second.first;
        ease = it->second.second;
        return true;
    }

void PyMfcHost::animTick()
{
        const DWORD now = static_cast<DWORD>(::GetTickCount64());
        bool alive = false;
        for (size_t i = 0; i < m_anims.size();)
        {
            AnimTrack& tr = m_anims[i];
            long long el = static_cast<long long>(now) -
                           static_cast<long long>(tr.startMs != 0 ? tr.startMs : m_animStartMs) -
                           tr.t0;
            if (el < 0)
            {
                ++i;
                alive = true;
                continue;
            }
            double p = static_cast<double>(el) / static_cast<double>(tr.dur);
            if (p > 1.0)
                p = 1.0;
            if (tr.color)
                applyAnimValue(tr, 0.0, lerpColor(tr.c0, tr.c1, easeAt(tr.ease, p)));
            else
            {
                const double v = tr.from + (tr.to - tr.from) * easeAt(tr.ease, p);
                // 末帧必须无条件落到 tr.to：整数型属性（SpinBox 的 Value）在最后一步
                // 往往只差 0.4 个像素级余量，被下面的去抖门限吃掉后，终点会停在 98 而不是 99。
                const bool final = p >= 1.0;
                const bool changed = final || !tr.hasLast || std::fabs(v - tr.lastV) >= 0.5;
                tr.hasLast = true;
                tr.lastV = v;
                if (changed)   // 数值没动就不必 SetWindowPos / 重画，避免空转闪烁
                    applyAnimValue(tr, final ? tr.to : v, 0);
            }
            if (p >= 1.0)
            {
                if (tr.loop)
                {
                    tr.t0 += tr.dur;   // 下一轮
                    ++i;
                    alive = true;
                    continue;
                }
                m_anims.erase(m_anims.begin() + static_cast<std::ptrdiff_t>(i));
                continue;
            }
            ++i;
            alive = true;
        }
        // CSS animation / keyframes：整条时间线按 fmod 走，循环时回到第一帧
        for (size_t i = 0; i < m_cssAnims.size();)
        {
            CssKeyframeAnim& a = m_cssAnims[i];
            long long el = static_cast<long long>(now) - static_cast<long long>(a.startMs);
            if (el < 0) el = 0;
            double p = static_cast<double>(el) / static_cast<double>(a.durMs);
            const bool finished = (!a.loop && p >= 1.0);
            if (a.loop)
                p = p - std::floor(p);
            else if (p > 1.0)
                p = 1.0;

            COLORREF c = a.frames.back().second;
            for (size_t k = 1; k < a.frames.size(); ++k)
            {
                const double p0 = a.frames[k - 1].first;
                const double p1 = a.frames[k].first;
                if (p <= p1 || k == a.frames.size() - 1)
                {
                    const double span = p1 - p0;
                    const double t = span > 1e-9 ? (p - p0) / span : 1.0;
                    c = lerpColor(a.frames[k - 1].second, a.frames[k].second,
                                  (std::max)(0.0, (std::min)(1.0, t)));
                    break;
                }
            }
            if (m_ctrlBack.count(a.id) == 0 || m_ctrlBack[a.id] != c)
            {
                m_ctrlBack[a.id] = c;
                if (HWND h = findByIdDeep(m_hWnd, a.id))
                    ::InvalidateRect(h, nullptr, TRUE);
            }
            if (finished)
            {
                m_cssAnims.erase(m_cssAnims.begin() + static_cast<std::ptrdiff_t>(i));
                continue;
            }
            ++i;
            alive = true;
        }
        if (!alive && m_animTickOn && m_animWnd != nullptr)
        {
            ::KillTimer(m_animWnd, kAnimTimerId);
            m_animTickOn = false;
        }
    }
