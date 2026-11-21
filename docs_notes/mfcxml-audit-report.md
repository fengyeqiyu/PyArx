# MFCXml（XML 通道）审计报告

- 审计时间：2026-11-18
- 仓库：`G:\PyArx`（Windows，pwsh 7.6.4）
- 产物：`X64\Release\MfcXmlCore.dll` / `PyMfcXml.pyd`（本轮全部走旁路 `log\flexbuild\` 规避占用锁）
- 判定基准：**HTML/CSS**（用户已纠正：不对齐 QML；QML 私语法只保留兼容、不扩展）
- 验收总纲：同一界面用 HTML 写与用 XML 写，观感看不出差别、书写成本看不出差别；语言仍是 XML，禁止回退 HTML/QML/WebView

---

## 一、结论摘要（对照任务 8 条验收要求）

| # | 验收要求 | 状态 | 证据 |
|---|---|---|---|
| 1 | 控件层 flex 容器（不依赖 XmlCore 算坐标） | **部分达成** | `MfcXmlCore\controls\XmlFlexPanel.{h,cpp}` 已存在，自身持 items、自算矩形、`reflowItems()` 自己 `moveControl`；但**真实父子层级（SetParent）未做**，仍是宿主扁平兄弟 |
| 2 | flex 全语义（direction/wrap/justify/align/align-self/grow/shrink/basis/gap） | **大部分达成** | 源码实况：`wrap / wrap-reverse`（行分断 line 190、反向 line 348）、`baseline`（line 454）、`rowgap`（line 476）等均已实现；`align-self` 仍靠 XmlCore 传整型、容器无 item 级 CSS 解析；`baseline` 与 Start 同分支（占位） |
| 3 | 容器自身响应尺寸变化重排（WM_SIZE/SetBounds） | **部分达成** | 有 `reflowItems()` 与 WM_SIZE 入口，但缺"容器放大/缩小子项跟排"的实测截图证据 |
| 4 | 父子关系要真（SetParent） | **未达成**（已列为下轮待办） | `XmlFlexPanel.cpp:389` 仍是 `m_host->moveControl(...)`，扁平结构；裁剪/层叠未验证 |
| 5 | XmlCore 的 LayoutFlex 降级为适配层 | **达成** | flex 容器分支改交"整块外框 + offset"给 `panelImpl.computeLayout`，XmlCore 不再算子项矩形；非 flex 老界面走原路径未变（cad2d 基线一致） |
| 6 | 标签/属性 HTML 对齐且大小写不敏感 | **达成** | `<div style="display:flex">` / `<XFlexPanel>` / `<row>` / `<column>` 均可命中；`flex-direction`≡`flexDirection`、`justify-content`≡`justifyContent`、`align-items`≡`alignItems` 等价 |
| 7 | 同界面 HTML/XML 双写截图对照 + 缩放重排实测 | **落位达成** | `examples\flex_demo\flex_demo_html.xml` + `compare.py`：13 控件矩形**最大偏差 = 0**；`tests\xml_ui\t_html_parity.py` 14 项全绿。缩放重排实测证据仍缺 |
| 8 | 回归红线 | **达成** | `examples\cad2d\xml_view.py --batch` 统计与基线一致；`examples\flex_demo` 正确；已绿 xml_ui 套件全绿（明细见第四节） |

**一句话结论**：外观与书写成本已从"XML 明显更啰嗦/明显不同"推进到"落位逐控件零偏差"，但**容器背景绘制**与**真实父子层级**两处仍是硬缺口，双版截图像素差停在 30.25%。其中容器背景已按用户指定的 `registerPaintNode` 路线打通全部环节，剩余问题收敛到绘制实参（详见第九节专项）。

---

## 二、审计方法与判定口径

1. **只读审计先行**：先定位"HTML 能做而 XML 做不到 / XML 明显更啰嗦"的具体点，再动代码；每轮至少落地一处可验证实现。
2. **区分"代码里有 X"与"XML 真走了 X"**：排查三条常见失效路径——① 控件级 `setControlAppearance` 从未被调用（写了等于没写）；② `XmlCore.cpp` 存在两份几何解析（主解析 ≈1043 与 `RecomputeGeometry()` ≈5228），只改一处会静默失效；③ 同一语义被两处各实现一次（`XmlCore` 与 `XmlFlexPanel` 各扣一次 padding）。
3. **验证只用真机闭环**：构建 `EXIT=0` + 受影响测试断言 + `--batch` 统计比对 + 像素级 `ImageChops` 比对；不靠"看起来对了"。
4. **回归红线只跑受影响部分**，不做全量回归（用户明确要求压缩流程）。

---

## 三、已落地项（本轮 6 轮，全部 EXIT=0 并验证）

| 轮次 | 缺口（HTML 能做 / XML 做不到 的具体点） | 实现 | 证据 |
|---|---|---|---|
| 1 | `<button style="background/color;border-radius">` 写了等于没写（控件级外观从未推送，`foreground` 还被错映射到 `FrontColor`） | `XmlQmlNames.h` 把 `foreground` 改指 `Foreground` 并补 `foregroundcolor`；控件级 `setControlAppearance` 生效 | 调试日志 `[ap] id=1000 bg='#2563eb' fg='#ffffff'`、`[drawitem] styled=1`；`t_style_appearance` 6 项绿；`build_audit_1507` |
| 2 | `<label>运行状态</label>` 不写宽度被拉满整行（HTML 里 label 是行内、宽按内容） | 新增 `IsInlineTextTag()` + `ApproxTextWidth()`，行内标签未写 Width 时按内容宽；宽度必须 `Put` 进属性（否则被 `RecomputeGeometry` 抹回） | `<label>`→60×24、`<span>OK</span>`→19×24、`<XLabel>` 仍 384（老界面零影响）；`t_inline_text` 7 项绿 |
| 3 | `Width="100%"` 被 `wcstol` 读成 100px（百分比写不进属性） | 主解析与 `RecomputeGeometry` **双处同步**加 `wPct/hPct` | `Width="100%"` ≡ `style="width:100%"`；显式 120px 不被吃；`t_pct_attr` 4 项绿 |
| 4 | cad2d 现场只能用 QML `anchors.fill="parent"` 表达"铺满并随窗口缩放" | `xml_view.py:366` 改 `Width="100%" Height="100%"`（py 模板 `%` 转义为 `%%`） | `--batch` 统计与基线逐项一致，迁移前后截图 `bbox=None`（逐像素相同） |
| 5 | 同一界面 HTML 写法与 XML 写法不敢说等价（无对照机制） | 新建 `flex_demo_html.xml`（div/button/input/label + 纯 CSS）、`compare.py`、`t_html_parity.py`；修 3 个真 bug：容器 padding 双算（子项多偏 16px）、`autoH` 被当显式高度致 `align-items:stretch` 失效（卡片 118 而非 224）、嵌套 flex 容器自然宽走 `NaturalWidth` 兜底 120 | 13 控件矩形**最大偏差 0**；`t_html_parity` 14 项绿；`build_audit_1635/1645/1655` |
| 6 | `<div>` 默认铺主题底色（HTML div 默认透明）、文字换行/垂直对齐无 CSS 写法、flex 列容器里 button 被默认尺寸表压住 | `<div>` 未声明背景时 `Background=transparent`；补 `justify-content/align-items/align-self/flex-wrap/row-gap/column-gap/white-space/vertical-align/line-height` 映射；列容器内 button/input 无显式宽时交 stretch | 像素差 88.83% → **30.25%**；`t_cssflex` 11、`t_flexpanel` 23、`t_div_css` 6、t_html_defaults 9 全绿；`build_audit_1710/1725/1740` |
| 7 | **容器背景路线拍板**（用户指定 `registerPaintNode`）：`<div background>` 在 XML 侧无任何绘制路径——容器是空 STATIC，宿主不做 owner-draw，控件基类无 WM_PAINT | 打通三层：`IXmlMfcHost::applyContainerBackdrop` 虚接口 → `XmlMfcControlBase::pushBackdrop`（`pushAppearance` 尾部）→ `XmlFlexPanel::pushBackdrop/setBounds`；宿主侧 `registerPaintNode(kind=6)` + `drawNodeContainer`（复用 `xmlDrawBackplate`）+ `case 6` 分派 | 链路编译 EXIT=0（`build_audit_1820~1855`），但运行期 `[backdrop*]` 零日志 → 继续定位 |
| 8 | **定位到路线失效的两条硬事实**（避免继续盲改）：① `PyMfcHost::reflowXml()` 在该运行路径**从未执行**（`PYARX_XML_LAYOUT_DBG=1` 下不生成 `xml_layout.log`，`PyMfcLoad.cpp:947` 的 `reflowXml()` 属 ItemView 重建路径）；② 布局主路径走 `moveMany` 而非 `moveControl` | `reflowXml` 尾部、`moveControl` 尾部、`moveMany` 循环内、`OnPaint` 出帧前（`m_backdropSyncDirty` 惰性）四处挂同步点；`setControlAppearance` 置脏 | `build_audit_1900/1905` EXIT=0 |
| 9 | **修掉一个真回归**：`applyContainerBackdrop` 里的 `SW_HIDE` 命中父容器（`page` div）→ 连同子控件整页消失（HTML 版白屏，像素差 37.45%） | 改为「有子窗口的容器不隐藏」（`::GetWindow(h, GW_CHILD)` 判空）；容器改 `SS_OWNERDRAW`（与按钮/标签共用 `OnDrawItem` 外观管线）；CSS `background` 双写 `Background`+`BackColor` | 白屏消除、回到 30.25%；cad2d `--batch` 基线逐项未变；`build_audit_1910/1915` |

---

## 四、回归红线状态（本次触发末次实测）

```
tests\xml_ui:  t_div_css 6/0 · t_cssflex 11/0 · t_flexpanel 23/0 · t_pct_attr 4/0
               t_style_appearance 6/0 · t_inline_text 7/0 · t_html_defaults 9/0 · t_html_parity 14/0
cad2d --batch: canvas 1032x761 · scene xml 570.4 KB · k=0.00101
               line=1292 pline=3478 circle=450 arc=56 text=3 block=1606 skip=16406   （与基线一致）
flex_demo:     13 控件落位 Δ=0；截图 diff 30.25%（未下降，背景未落图）
```

> 注：`tests\xml_ui` 各项为上一次全绿时的结果，本轮改动集中在宿主绘制/容器创建，未重跑全部套件（用户要求压缩流程）；cad2d 与 flex_demo 两条红线为**本轮实测**。

---

## 五、能力现状实况（源码级清点，非推测）

**`style` 内联已识别的 CSS 键**（`XmlCore.cpp` 源码实况）：
`width`(含 %/px)、`height`、`flex`(简写)、`flex-grow`、`flex-shrink`、`flex-basis`、`display`、`flex-direction`、`padding`、`gap`、`margin`、`justify-content`、`align-items`、`align-self`、`flex-wrap`、`row-gap`、`column-gap`、`border-radius`、`color`/`foreground`、`background`/`background-color`、`font-size`、`font-weight`、`text-align`、`white-space`、`vertical-align`、`line-height`、`top/left/right/bottom`。

**控件层 flex 容器实况**（`XmlFlexPanel.cpp`）：wrap 多行分断、wrap-reverse、baseline 分支、row-gap 均已有代码路径；`reflowItems()` 自算自移；**未 SetParent**。

**HTML 默认值表**（`XmlCore` 内置）：button 88×30、input 160×30 等，仅在"作者未写尺寸且不在 flex 拉伸语境"时生效。注：`control_catalog.xml` 全盘不存在，`defaultSizeFor()` 恒落 120×28 兜底。

---

## 六、剩余缺口（按优先级）

| 优先级 | 缺口 | 影响 | 下一步 |
|---|---|---|---|
| P0 | **容器（`<div>`）的 `background` 仍未落图**（路线已定 `registerPaintNode`，链路已通到 `kind=6` + `drawNodeContainer`，但运行期没生效） | 卡片底色整块缺失，是 30.25% 像素差的唯一主因；HTML 观感基线做不到 | **已排除的假设**：外观没推进（`[ap] id=1007 bg='#262B31'` 证伪）、容器无 HWND（`getRect` 返回 200×201 证伪）、容器非 owner-draw（`[drawitem] style=0x5400000D` 即 `SS_OWNERDRAW` 证伪）。**下一步**：在 `OnDrawItem` 的 `paintBack` 分支打印 `opacity / itemRc / 最终 back 色`，确认是否 `itemRc` 空矩形或 `Blend` 降成父色后与父表面同色 |
| P0 | **真实父子层级未做**（仍 `moveControl` 扁平兄弟） | 裁剪、层叠、容器内相对坐标不正确；DOM/XML 树语义不真 | 子控件 `SetParent` 到容器 + 容器内相对坐标；先在 flex_demo 单点验证再铺开 |
| P1 | 缩放重排无实测证据（WM_SIZE/SetBounds） | 验收第 3、7 条缺"容器缩小/放大子项跟排"截图 | 扩 `compare.py`：同界面 704×404 / 560×380 / 900×520 三档出图并断言子项随容器变化 |
| P1 | `align-self` 无 item 级 CSS 解析；`baseline` 仍是 Start 占位 | 单子项对齐做不到 HTML 水平 | 容器内解析 `align-self`，`baseline` 用首行文字基线 |
| P2 | `inset`/`min-width`/`max-width`/`overflow`/`box-shadow`/`opacity` 未识别 | 复杂卡片外观受限 | 按 `html-xml-map.md` 逐项补 |
| P2 | 代码注释仍有"QML 对齐"措辞 | 与设计基准不一致 | 搜改措辞 |
| P2 | cad2d 线宽开关未验收（`lineweight` 开关用户已问，未见反馈） | 用户观感未闭环 | 用带 lineweight 的真实 DWG 重扫验证 |

---

## 七、风险与待用户决策

1. **`<XLabel>` 是否也改成"不写宽度按内容"**：实测 `examples` 下 83 处 `<XLabel>` 有 40 处未写 Width，直接改会打散约一半老界面排版；当前只对 HTML 标签写法生效（保守）。需用户决定是否统一。
2. **容器背景绘制路线**：✅ 已决策 —— 用户指定走宿主 `registerPaintNode`（不再考虑容器自绘 WM_PAINT）。该路线已落地到代码，剩余是"为什么画不出来"的实现问题，见第六节 P0。
3. **严格父子层级**：`SetParent` 后 `moveControl` 坐标系、z-order、裁剪都要跟着改，属一次结构性改动，建议单独一轮并只跑 flex_demo + cad2d 两条红线。

---

## 八、构建与测试日志索引

| 日志 | 内容 |
|---|---|
| `log\build_audit_1507.txt` | 外观进 owner-draw + foreground 映射 |
| `log\build_audit_1525.txt` | 行内文本自然宽 |
| `log\build_audit_1550.txt` | 百分比尺寸属性 |
| `log\build_audit_1620.txt` | cad2d 迁移 HTML 写法 |
| `log\build_audit_1635/1645/1655.txt` | padding 双算 / stretch / 自然宽 三 bug |
| `log\build_audit_1710.txt` | div 透明背板 + CSS 对齐映射 |
| `log\build_audit_1725.txt` | white-space / vertical-align 映射 |
| `log\build_audit_1740.txt` | 容器内 button 拉伸语义 |
| `log\build_audit_1820/1825_core.txt` | 核心库 Rebuild（发现增量构建不重建 `MfcXmlCore.dll`） |
| `log\build_audit_1830/1835/1840/1845/1850/1855/1900.txt` | 容器背板挂点迭代（含 2 次 `C2082`：形参 `h` 与局部 `HWND h` 重名） |
| `log\build_audit_1905.txt` | 白屏回归修复（有子窗口的容器不 `SW_HIDE`） |
| `log\build_audit_1910.txt` | 容器改 `SS_OWNERDRAW` |
| `log\build_audit_1915.txt` | 核心库 Rebuild + CSS `background` 双写 |

配套：`docs_notes\html-xml-map.md`（HTML↔XML 对齐清单，增量）、`docs_notes\mfcxml-audit-log.md`（每轮一行）。

> 目录说明：任务描述里写的是 `docs\...`，仓库内实际审计产物一直在 `docs_notes\`（`docs\` 目录不存在）。

---

## 九、专项：容器背景链路（`registerPaintNode` 路线）审计

**待解问题**：同一界面双写，13 个控件落位 Δ=0，但截图仍有 30.25% 像素差，差异区集中在 y≈280–320（三栏卡片与底部按钮区）——HTML 版整片是页面色 `(30,34,38)`，XML 版是卡片色 `(38,43,49)` / 按钮色 `(47,53,59)`。

**证据链（按时间顺序，含被证伪的假设）**

| # | 假设 | 验证手段 | 结论 |
|---|---|---|---|
| 1 | 容器没有绘制路径 | 源码：容器 = 空 `STATIC`，`XmlMfcControlBase`/`XmlPanel` 无 `WM_PAINT`，宿主不做 owner-draw | ✅ 成立（这是 30.25% 差的唯一主因） |
| 2 | 背板挂在 `setControlAppearance` 就够了 | `[backdrop]` 日志计数 | ❌ 证伪：外观推送发生在控件创建之前，`GetDlgItem` 恒空 |
| 3 | 布局走 `moveControl`，挂那里就够 | 源码 `PyMfcWrap.cpp:562/1570` 转发 | ❌ 证伪：主路径是 `moveMany`（批量 `DeferWindowPos`） |
| 4 | 布局会走 `reflowXml()`，挂其尾部统一同步 | `PYARX_XML_LAYOUT_DBG=1` → 是否生成 `xml_layout.log` | ❌ 证伪：**不生成**，`reflowXml()` 在本路径从未执行（`PyMfcLoad.cpp:947` 属 ItemView 重建路径） |
| 5 | `pyd` 没重编（改动没进产物） | 二进制宽字符串检索 `[System.Text.Encoding]::Unicode` | ❌ 证伪：`backdrop-sync` 确实编入 `PyMfcXml.pyd`（ASCII 检索找不到是正常的，宽字符串不是 ASCII） |
| 6 | `style="background:#262B31"` 没进外观 | `[ap]` 日志 | ❌ 证伪：`[ap] id=1007 bg='#262B31'` 存在（此前用 `#FF262B31` 检索落空，因 CSS 六位色不带 alpha 前缀） |
| 7 | 容器没 HWND / 不是 owner-draw | `win.getRect(c1)`、`[drawitem] style=` | ❌ 证伪：`getRect=(16,72,216,201)`；`style=0x5400000D` 低字节 `0x0D` 即 `SS_OWNERDRAW` |

**过程中修掉的真缺陷**

1. `SW_HIDE` 命中父容器 → 整页白屏（HTML 版像素差一度 37.45%）。修复：`::GetWindow(h, GW_CHILD) == nullptr` 才隐藏。
2. 子项落位与背板登记顺序错位：`moveMany` 先 `updatePaintNodeRect` 再 `DeferWindowPos`，直接用 `GetWindowRect` 会拿到**旧位置**；改为把布局算出的 `x,y,w,h` 显式传进 `syncOneContainerBackdrop(id,x,y,w,h)`。
3. 形参/局部同名 `h` 导致 `C2082`（两次），已统一改 `hw` / 保持形参名。

**当前判定**：链路（属性→外观→登记→出帧）每一环都已被独立证明"在"，但端到端仍不出像素。剩下的可疑点收敛到**绘制那一刻的实参**：`OnDrawItem` 里 `paintBack` 为真时调用 `xmlDrawBackplate(*cv, itemRc, *ap, surface, m_border)`，若 `itemRc` 为空或 `ap.opacity` 非 255 被 `Blend` 降回 `surface`，画面与父表面完全同色 —— 日志上看"都做了"，像素上看"什么都没有"。这正是下一轮要打的点。

