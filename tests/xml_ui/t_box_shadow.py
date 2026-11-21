# 第23轮：CSS box-shadow 落地（此前只能写私有皮肤脚本 Shadow op，style 里完全无法表达）
import os, sys
BIN = os.environ["PYARX_BIN"]; sys.path.insert(0, BIN); os.add_dll_directory(BIN)
os.environ.setdefault("PYARX_D2D_RT", "software")
import PyMfcXml as M
from PIL import Image

fails = []
def chk(name, cond, got=""):
    print(("PASS  " if cond else "FAIL  ") + name + ("  " + str(got) if got else ""))
    if not cond: fails.append(name)

def build(inner, out):
    n = {}
    w = M.guiload_text('<XWindow Width="300" Height="200" Background="#202020">' + inner + '</XWindow>', names=n)
    M.pump(200)
    w.save_png(out)
    r = w.get_rect(n["box"])
    w.close()
    return r

PAGE = 0x20 * 3            # 页面底色 #202020 的三通道和

def scan_dark(im, rect):
    """在盒子下缘之外的一段带里找"明显暗于页面底色"的像素。

    不用精确采样点：savePng 是整窗截图（含标题栏），客户区坐标会有固定偏移，
    固定点采样会把阴影扫到盒子内部去。这里只判"盒子下方出现了压暗的像素带"，
    对偏移免疫。
    """
    x0 = rect[0] + 20
    x1 = rect[0] + rect[2] - 20
    y0 = rect[1] + rect[3]
    y1 = min(rect[1] + rect[3] + 60, im.size[1] - 8)    # 留出窗口底部黑边
    for y in range(max(0, y0), y1):
        for x in range(max(0, x0), x1):
            px = im.getpixel((x, y))
            if sum(px[:3]) < PAGE - 6:
                return (x, y, px)
    return None

# ---- 第87轮：inset 盒内阴影区域扫描（口径同第86轮 t_gradient，偏移免疫） ----
def _lum(px):
    return (px[0] * 30 + px[1] * 59 + px[2] * 11) // 100

def locate_band(img, rect):
    """沿中列在整窗内定位浅色盒子带。
    savePng 抓的是整窗矩形（含标题栏非客户区），getRect 是客户区坐标，
    两者有固定纵向偏移（~31px），所以不写死绝对坐标：跳过顶部 1/6（标题栏），
    把明显浅于页面底色 #202020 的连续行段判为盒子带。返回 (cx, yTop, yBottom)，
    找不到浅色带时 yTop=None。"""
    x, y, w, h = rect
    cx = x + w // 2
    start = max(1, img.size[1] // 6)
    rows = []
    for yy in range(start, img.size[1] - 4):
        if sum(img.getpixel((cx, yy))[:3]) > PAGE + 30:
            rows.append(yy)
    if not rows:
        return cx, None, None
    return cx, rows[0], rows[-1]

# A) 有 box-shadow：盒子下方必须出现压暗的像素带
rA = build('<div Name="box" Left="40" Top="40" Width="160" Height="90" '
           'style="background:#3A4048;box-shadow:0 4 10 rgba(0,0,0,0.55)"/>', "G:/PyArx/log/shadow_on.png")
imA = Image.open("G:/PyArx/log/shadow_on.png").convert("RGB")
darkA = scan_dark(imA, rA)
chk("阴影把下方像素压暗", darkA is not None, darkA)

# B) 无 box-shadow：同样带着范围应干净（对照组，证明差异来自阴影）
rB = build('<div Name="box" Left="40" Top="40" Width="160" Height="90" '
           'style="background:#3A4048"/>', "G:/PyArx/log/shadow_off.png")
imB = Image.open("G:/PyArx/log/shadow_off.png").convert("RGB")
darkB = scan_dark(imB, rB)
chk("无阴影时同带范围是干净背景", darkB is None, darkB)
chk("两组确有差异", darkA != darkB, (darkA, darkB))

# C) 原文可回读（属性面板 / 脚本能拿到声明值）
n = {}
w = M.guiload_text('<XWindow Width="300" Height="200"><div Name="box" Top="10" Left="10" '
                   'Width="100" Height="60" style="box-shadow:0 2 6 #000000"/></XWindow>', names=n)
M.pump(100)
v = w.get_xml_value(n["box"], "BoxShadow")
w.close()
chk("BoxShadow 可回读", v == "0 2 6 #000000", v)

# D) inset / none 当没写，不能画出错误的投影
rD = build('<div Name="box" Left="40" Top="40" Width="160" Height="90" '
           'style="background:#3A4048;box-shadow:inset 0 2 6 #000000"/>', "G:/PyArx/log/shadow_inset.png")
imD = Image.open("G:/PyArx/log/shadow_inset.png").convert("RGB")
chk("inset 不画外投影", scan_dark(imD, rD) is None, scan_dark(imD, rD))

# E) inset 盒内阴影正向断言：只压暗盒内上沿一条渐隐带，不漏到盒外。
#    浅底 #FFFFFF 保证压暗可判；采样沿中列定位真实盒子带（不写死绝对坐标）。
#    阈值依据：inset 0 2px 6px rgba(0,0,0,.55) 叠白，理论最强混合 ≈ 115/255，
#    中心为 255，落差约 140；12/255 是保守下限，容忍刷子软边近似与抗锯齿；
#    单调容差 8/255 同 t_gradient；盒外与纯色页底 #202020 差值 ≤12（暗带口径沿用
#    scan_dark 的 PAGE-6）。依赖主线刷子通道落地，本轮静态自查，待跑通验证。
rE = build('<div Name="box" Left="40" Top="40" Width="160" Height="90" '
           'style="background:#FFFFFF;box-shadow:inset 0 2px 6px rgba(0,0,0,.55)"/>',
           "G:/PyArx/log/shadow_inset_in.png")
imE = Image.open("G:/PyArx/log/shadow_inset_in.png").convert("RGB")
cxE, tE, bE = locate_band(imE, rE)
if tE is None:
    chk("inset 盒内阴影：中列定位到浅色盒子带", False, "未找到浅色带")
else:
    # E1 盒内侧近上沿（上沿内 1~3px）平均亮度明显低于盒中心
    top_rows = range(tE + 1, min(tE + 4, bE))
    top_lum = [_lum(imE.getpixel((cxE, yy))[:3]) for yy in top_rows]
    cy = (tE + bE) // 2
    center_lum = _lum(imE.getpixel((cxE, cy))[:3])
    top_avg = sum(top_lum) / len(top_lum)
    drop = center_lum - top_avg
    chk("inset 近上沿压暗 ≥12/255（上沿内→中心）", drop >= 12,
        {"top": top_lum, "center": center_lum, "drop": round(drop, 1), "band": (tE, bE)})

    # E2 从近上沿向中心方向趋势单调（逐步变亮，容差 8/255）
    n = 5
    ys = [tE + 2 + (cy - tE - 2) * i // (n - 1) for i in range(n)]
    prof = [_lum(imE.getpixel((cxE, yy))[:3]) for yy in ys]
    mono = all(prof[i + 1] >= prof[i] - 8 for i in range(n - 1))
    chk("inset 上沿→中心亮度单调趋亮", mono,
        {"prof": prof, "ys": ys, "band": (tE, bE)})

    # E3 盒外（上沿之外、下沿之外）不出现暗带：与页底 #202020 差值 ≤12
    start = max(1, imE.size[1] // 6)
    above = [sum(imE.getpixel((cxE, yy))[:3])
             for yy in range(max(start, tE - 8), tE)]
    below = [sum(imE.getpixel((cxE, yy))[:3])
             for yy in range(bE + 1, min(imE.size[1] - 4, bE + 9))]
    dev = max(abs(v - PAGE) for v in above + below) if (above or below) else 0
    chk("inset 盒外上下无暗带（与底色差≤12）", dev <= 12,
        {"above": above, "below": below, "PAGE": PAGE, "dev": dev, "band": (tE, bE)})

# F) inset spread 对照型断言：同一窗口内左右并排，各自沿自己盒子中列定位，
#    只做自比较（不依赖绝对色值 / 绝对像素坐标）。口径复用 locate_band / _lum /
#    getRect 内缩；阈值 12/255 与 E1 一致。
#    预期：inset 合成此前忽略 spread（PyMfcHost.cpp:7470-7602），d'=d-spread 尚未
#    接入，F1/F2 本轮应为红；接入后转绿。
def _profile(img, rect, n=7):
    """沿盒子中列取上沿→中心 n 点亮度剖面。返回 (col, yTop, yBottom, prof, ys)；
    定位失败时 yTop=None。列取 getRect 中列，行段由 locate_band 在截图内反推，
    与 E 组同一套偏移免疫口径。"""
    col, yt, yb = locate_band(img, rect)
    if yt is None:
        return col, None, None, [], []
    cy = (yt + yb) // 2
    ys = [yt + 2 + (cy - yt - 2) * i // (n - 1) for i in range(n)]
    prof = [_lum(img.getpixel((col, yy))[:3]) for yy in ys]
    return col, yt, yb, prof, ys

def _darkest(prof, ys):
    """剖面内最暗行索引（相对上沿 yt 的像素偏移，ys 首点为 yt+2）与最暗亮度；
    空剖面返回 None。"""
    if not prof:
        return None, None
    k = min(range(len(prof)), key=lambda i: prof[i])
    return ys[k] - ys[0], prof[k]

def _band_lum(img, col, yt, offs):
    """上沿内指定偏移（1~3px 等）处逐行亮度列表。"""
    return [_lum(img.getpixel((col, yt + o))[:3]) for o in offs]

# F1 正 spread：inset 0 2 6 10（左）vs inset 0 2 6 0（右）。
#    正 spread 让暗带向内扩展：离上沿更深处（上沿内约 12px）左条应明显更暗
#    （右条该处已渐隐回白）；两条近上沿 1~3px 都仍压暗（差值不判，只记录）。
nF = {}
wF = M.guiload_text(
    '<XWindow Width="420" Height="160" Background="#202020">'
    '<div Name="f1" Left="30" Top="30" Width="160" Height="90" '
    'style="background:#FFFFFF;box-shadow:inset 0 2px 6px 10px rgba(0,0,0,.55)"/>'
    '<div Name="f0" Left="230" Top="30" Width="160" Height="90" '
    'style="background:#FFFFFF;box-shadow:inset 0 2px 6px 0 rgba(0,0,0,.55)"/>'
    '</XWindow>', names=nF)
M.pump(200)
wF.save_png("G:/PyArx/log/shadow_spread.png")
rF1 = wF.get_rect(nF["f1"]); rF0 = wF.get_rect(nF["f0"])
imF = Image.open("G:/PyArx/log/shadow_spread.png").convert("RGB")
wF.close()

c1, t1, b1, p1, ys1 = _profile(imF, rF1)
c0, t0, b0, p0, ys0 = _profile(imF, rF0)
if t1 is None or t0 is None:
    chk("F1 正 spread：两条盒子带均定位到", False, {"f1": (c1, t1, b1), "f0": (c0, t0, b0)})
else:
    # 深处采样：上沿内 12px（在 90px 高盒子内、中心之上，落在暗带渐隐区）
    deep = 12
    deep1 = _lum(imF.getpixel((c1, t1 + deep))[:3])
    deep0 = _lum(imF.getpixel((c0, t0 + deep))[:3])
    near1 = _band_lum(imF, c1, t1, (1, 2, 3))
    near0 = _band_lum(imF, c0, t0, (1, 2, 3))
    dk1, dm1 = _darkest(p1, ys1); dk0, dm0 = _darkest(p0, ys0)
    chk("F1 正 spread：深处(上沿内12px)正spread条更暗 ≥12/255",
        deep0 - deep1 >= 12,
        {"spread+10": {"deep": deep1, "near": near1, "prof": p1, "darkest_off": dk1,
                       "darkest_lum": dm1, "band": (t1, b1)},
         "spread=0": {"deep": deep0, "near": near0, "prof": p0, "darkest_off": dk0,
                      "darkest_lum": dm0, "band": (t0, b0)},
         "deep_diff(0-10)": deep0 - deep1})

# F2 负 spread：inset 0 2 6 -4（左）vs spread=0（右）。
#    负 spread 让暗带更贴边：近上沿 1~3px 左条应更亮（暗带被向内收，边沿压暗减弱）；
#    辅证：最暗行索引更靠上（暗带峰值贴近边），容差按实测留 3px。
nG = {}
wG = M.guiload_text(
    '<XWindow Width="420" Height="160" Background="#202020">'
    '<div Name="g1" Left="30" Top="30" Width="160" Height="90" '
    'style="background:#FFFFFF;box-shadow:inset 0 2px 6px -4px rgba(0,0,0,.55)"/>'
    '<div Name="g0" Left="230" Top="30" Width="160" Height="90" '
    'style="background:#FFFFFF;box-shadow:inset 0 2px 6px 0 rgba(0,0,0,.55)"/>'
    '</XWindow>', names=nG)
M.pump(200)
wG.save_png("G:/PyArx/log/shadow_spread_neg.png")
rG1 = wG.get_rect(nG["g1"]); rG0 = wG.get_rect(nG["g0"])
imG = Image.open("G:/PyArx/log/shadow_spread_neg.png").convert("RGB")
wG.close()

h1, u1, v1, q1, yq1 = _profile(imG, rG1)
h0, u0, v0, q0, yq0 = _profile(imG, rG0)
if u1 is None or u0 is None:
    chk("F2 负 spread：两条盒子带均定位到", False, {"g1": (h1, u1, v1), "g0": (h0, u0, v0)})
else:
    near1 = _band_lum(imG, h1, u1, (1, 2, 3))
    near0 = _band_lum(imG, h0, u0, (1, 2, 3))
    near_avg1 = sum(near1) / len(near1)
    near_avg0 = sum(near0) / len(near0)
    dk1, dm1 = _darkest(q1, yq1); dk0, dm0 = _darkest(q0, yq0)
    # 辅证：负 spread 最暗行更靠上（偏移更小）；峰值亮度差与抗锯齿会平移 1~2 行，
    # 容差 3px。若剖面采样步长内未分辨出峰值，该辅证仅作记录、不单独判红。
    peak_closer = dk1 is not None and dk0 is not None and dk1 <= dk0 + 3
    chk("F2 负 spread：近上沿(1~3px)负spread条更亮 ≥12/255",
        near_avg1 - near_avg0 >= 12,
        {"spread-4": {"near": near1, "near_avg": round(near_avg1, 1), "prof": q1,
                      "darkest_off": dk1, "darkest_lum": dm1, "band": (u1, v1)},
         "spread=0": {"near": near0, "near_avg": round(near_avg0, 1), "prof": q0,
                      "darkest_off": dk0, "darkest_lum": dm0, "band": (u0, v0)},
         "near_diff(-4-0)": round(near_avg1 - near_avg0, 1),
         "peak_closer(容差3px)": peak_closer})

print("t_box_shadow: PASS=%d FAIL=%d" % (10 - len(fails), len(fails)))
