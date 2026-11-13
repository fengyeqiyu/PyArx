import io
P = r'G:\PyArx\docs\mfcxaml-powerplan-app.md'
t = io.open(P, encoding='utf-8-sig').read()
add = """
## 11. 「真实鼠标选不中 / 滚轮不缩放」的真因：WM_NCHITTEST 回了 HTTRANSPARENT

现象：SendMessage 模拟点击 7/7 全过，但真人用鼠标点卡片没反应、滚轮也不缩放。

排查顺序（每一步都是实测，不是猜）：

1. `WindowFromPoint(卡片中心)` 返回的不是画布句柄，而是一个 id=0 的容器；
2. 枚举该点下的子窗口，覆盖该点的只有画布自己（rect、`IsWindowVisible`、Z 序都正常）；
3. `GetWindowRgn` 返回 0（没有裁剪区域）；
4. 直接给画布发 `WM_NCHITTEST` → **返回 -1**。

`-1` 是 `HTTRANSPARENT`：**"我这个点在命中测试上透明，把点击交给下面的窗口"**。
画布是 `SS_OWNERDRAW` 的 Static，Static 的类过程默认就这么回 —— 绘制被我们接管了，
但命中声明没跟着改。于是系统把真实鼠标全部发给父窗：卡片点不中、滚轮也滚不到。
而 `SendMessage` 是直接投递到窗口过程、**绕过系统命中链**，所以模拟点击一直是好的。
这个"模拟通、真实不通"的反差就是根因线索。

修法（`PyMfcOwnerDrawProc` 最前面，和皮肤过程里已有的约定对齐）：

```cpp
if (msg == WM_NCHITTEST)
    return HTCLIENT;
```

另外记录一个测试方法上的坑：真实输入（`SendInput`）投递的消息同样要靠宿主
消息循环分发，脚本里 `SendInput` 之后不 `M.pump()` 就立刻读状态，读到的是旧值 ——
这一点曾让我误判成"仍然点不中"。现在 `pp_real_test.py` 每次真实输入后都 pump。

`pp_real_test.py`（真实鼠标，不是 SendMessage）**6/6 PASS**：
系统命中落在画布、真点击选中 `AT1-1` 且详情浮层打开（探针 `up=(140,389) wasDrag=1 hit=AT1-1`）、
真滚轮 `0.2000→0.2240` 并缩回、真左键拖动平移。
"""
t = t.rstrip() + "\n" + add
io.open(P, 'w', encoding='utf-8-sig', newline='').write(t)
print('文档第 11 节已写')
