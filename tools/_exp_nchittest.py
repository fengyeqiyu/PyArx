import io
P = r'C:\Users\huich\AppData\Local\Doubao\User Data\Default\.doubao\agent_mode\workspace\experience\mfc-owner-draw-click-lost-flicker-and-printwindow.md'
t = io.open(P, encoding='utf-8-sig').read()
add = """

## 坑 4：自绘控件收不到"真实"鼠标，但 SendMessage 模拟一切正常

**症状**：向自绘控件 `SendMessage` 发 `WM_LBUTTONDOWN/UP` 能正常选中、滚轮也正常；
真人用鼠标点同一位置毫无反应，滚轮也不缩放。

**真因**：这类自绘控件常挂在 `STATIC`（`SS_OWNERDRAW`）类上。Static 的类过程对
`WM_NCHITTEST` 默认返回 `HTTRANSPARENT`（-1），含义是"这个点在命中测试上透明，
请把点击交给下面的窗口"。绘制被接管了，但"命中声明"没跟着改，于是系统把真实
鼠标全发给父窗。`SendMessage` 绕过系统命中链、直接进窗口过程，所以模拟永远是对的 ——
**"模拟通、真实不通"这个反差本身就是最有力的线索**。

**定位路径**（每条都能独立证伪一个假设，不要跳步）：
1. `WindowFromPoint(目标点)` —— 返回的不是目标控件句柄，就坐实了命中链问题；
2. 枚举该点的所有子窗口（rect / 可见性 / Z 序）—— 排除"被别的控件压盖"；
3. `GetWindowRgn` —— 返回 0 说明没有窗口区域干扰；
4. 直接 `SendMessage(WM_NCHITTEST)` 看返回值 —— 拿到 -1 即确诊。

**修法**：在自绘控件的窗口过程最前面一行 `if (msg == WM_NCHITTEST) return HTCLIENT;`。

**顺带坑**：`SendInput` 产生的真实输入同样要靠宿主消息循环分发。脚本发完不 `pump()`
就立刻读状态，读到的还是旧值，看起来像"修复无效"。真实输入验收脚本里每次
`SendInput` 之后都要 pump 一次再断言。

**通用教训**：自绘（owner-draw）只接管了"画"，没有接管"我在这里"。凡是 `STATIC`
承载的自绘控件，`WM_NCHITTEST` 必须显式回 `HTCLIENT`，否则点击与滚轮全部穿透。
"""
t = t.rstrip() + "\n" + add
io.open(P, 'w', encoding='utf-8-sig', newline='').write(t)
print('经验已追加坑 4')
