# HTML ↔ XML 对齐清单（MfcXml）

基准：HTML/CSS。目标是「同一份界面，HTML 写和 XML 写，观感与书写成本都看不出差别」。语言仍是 XML。

| HTML / CSS | XML 等价写法 | 状态 |
|---|---|---|
| `<style>` 块 + `class="a b"` | `<style>` 里的 `.card{}` / `#id{}` / `tag.class{}` / 逗号并列 → 合并进各节点 style，行内 style 最后覆盖；`:hover`/`:focus`/`:active` 落到 HoverBackground/HoverColor 等状态属性；权重按 id > class > tag、同权重按源码顺序；支持后代（空格）、子代（`>`）、紧邻兄弟（`+`）、后继兄弟（`~`）组合器，如 `.card .title`、`.card > .body`、`#lab + input`、`#lab ~ .ctl`；属性选择器 `[type="text"]`（存在/`=`/`~=`/`^=`/`$=`/`*=`/`|=`）与结构伪类 `:first-child`/`:last-child`/`:nth-child(odd|even|N|an+b)`、`:not(复合选择器)`、`:disabled`/`:checked`/`:selected`（读 `disabled`/`enabled`/`checked` 属性）、`!important`、`@media (max-width/min-width/max-height/min-height)`（按根元素宽高静态判定）已补；`::before/::after`（纯文本 `content`，拼进元素文字）已补；`content:attr(x)`（可与文本片段混排）已补；`@keyframes` 颜色帧块已补（尺寸位移帧未做）与 `@media` | 已补（99–100 轮，t_css_class 10/10）`n| `<ul>` / `<ol>` + `<li>` | `<ul>/<ol>` 落 `XListBox`，条目自动带 `• ` / `1. ` 前缀（与浏览器 marker 一致） | 已补（97 轮，t_html_vocab 30/30）`n| `<div>` 块级容器 | `<div>`（htmlsugar）/ `<XStackPanel>` | 已有 |
| `<row>` 横排 | `<row>`（控件层 flex，天生横排） | 已补 |
| `<column>` 竖排 | `<column>`（控件层 flex，天生竖排） | 已补 |
| `display:flex` | `<FlexBox>` / `<XFlexPanel>` / `Display="flex"` | 已有 |
| `<button>` 默认圆角（浏览器 6px） | 原生小写 `<button>` 不写圆角时自动注入 `Radius=6`（写 0 / 写值均不覆盖；`<XButton>` 不受影响） | 已补（35 轮） |
| `flex-direction` / `flexDirection` | 同名属性（大小写、中划线/驼峰等价） | 已有 |
| `flex-wrap` | `flex-wrap="wrap"` / `Wrap="1"` | 已有 |
| `flex-flow`（direction + wrap 简写） | 同名写法 `flex-flow: row wrap` / `column nowrap` | 已补（34 轮） |
| `display:none` 不占位 | 布局与容器重排都跳过隐藏项 | 已补（33 轮） |
| `visibility:hidden` 保留占位 | `style="visibility:hidden"` 或 `Visibility="hidden"` | 已补（44 轮）：只藏窗口、不动几何（此前被悄悄转成 `Visible=False`，等于整块拿掉；`display:none` 的不占位仍走 Visible 分支） |
| `transition:background-color .2s ease` | `style="transition:200ms linear"` / `transition-duration` / `transition-timing-function` | 已补（45/46 轮）：hover/active/focus 底色切换按 ms 插值，linear/ease/in-out/cubic-bezier 可用；动画帧真正落到背板（此前 `animateColor` 只算不画）；`animation`/`@keyframes` 仍未做 |
| `justify-content` | `justify-content` / `Justify` | 已有 |
| `align-items` | `align-items` / `Align` | 已有 |
| `align-self` | `align-self` / `Self` / `AlignSelf` | 已补 |
| `margin:0 auto`（块级居中） | 同名 `style="margin:0 auto"`，非 flex 父容器内也生效 | 已补（36 轮） |
| `inset`（1~4 值简写） | 同名 `style="inset:0"` → 撑满父内框；非全 0 值按上右下左落 `Top/Right/Bottom/Left` | 已补（37 轮） |
| `font`（简写） | 同名 `style="font:bold 16px/1.4 微软雅黑"` → `FontWeight/FontSize/LineHeight/FontFamily` 一次落定 | 已补（38 轮） |
| `text-decoration:underline` | 同名 `style="text-decoration:underline"` → `TextDecoration`；线按文字基线落点（`XmlCanvas::textBaseline`），盒高 40/70 都紧贴文字 | 已补（39 轮，5 轮修基线） |
| `text-decoration:line-through`（`<del>` 默认） | 同名 `style="text-decoration:line-through"` → `TextDecoration`，删除线画在基线上方约 0.32 字高、穿过字面 | 已补（7 轮） |
| `#RGB` / `#RGBA`（颜色短写） | 同名 `background:#fff` / `#0008` → 展开成 `#ffffff` / `#000000` + `BgAlpha` | 已补（40 轮） |
| `letter-spacing`（小数/负值） | 同名 `style="letter-spacing:-.5px"` → `LetterSpacing`，支持小数与负值并作用到 DWrite 字符间距 | 已补（41 轮） |
| `text-transform` | 同名 `style="text-transform:uppercase"` → `TextTransform`（uppercase/lowercase/capitalize，只改渲染不改数据） | 已补（42 轮） |
| `font-family`（回退列表/引号） | 同名 `style="font-family:\"Segoe UI\", Arial, sans-serif"` → `FontFamily` 取首个具体字体，通用族留空 | 已补（43 轮） |
| `:hover`（背景/文字色） | `hover-background` / `hover-color`（或属性 `HoverBackground` / `HoverColor`）→ 鼠标进入换色、离开复原 | 已补（44/45 轮，像素已验证） |
| `display:grid` + `grid-template-columns: 1fr 2fr` | 同名 style 写法 → 容器落 `XGrid`，`1fr`→`*`、`2fr`→`2*`、`32px`/`48` 原样、`auto` 原样；`repeat(3,1fr)` 展开 | 已补（89 轮） |
| `grid-template-rows` / `gap` | 同名 style 写法（轨道可用空间按 content box 扣 padding） | 已补（89 轮） |
| `grid-column: 1 / span 2`、`grid-row: 2` | 同名 style 写法 → `Grid.Column` / `Grid.Row` / `*Span`（1-based 线号转 0-based） | 已补（89 轮） |
| grid 的 auto-placement（不写落格） | 不写 `grid-column`/`grid-row` 的子项按源顺序行优先填格（`grid-auto-flow:row`） | 已补（89 轮） |
| grid 项默认拉伸到单元格 | 按内容测出的伪宽度（`autoTextW`）不再算作者宽度，单元格拉满 | 已补（89/90 轮）：HTML `<button>`/`<input>` 的伪宽高改打 `sugarW`/`sugarH`（跨几何重算保留），格子里两轴都拉满 || `:focus`（背景/文字色） | `focus-background` / `focus-color`（或属性 `FocusBackground` / `FocusColor`）→ 获焦换色；`<input>` 走 Edit 的 Palette | 已补（54 轮） |
| `:active`（背景/文字色） | `active-background` / `active-color`（或属性 `ActiveBackground` / `ActiveColor`）→ 按下换色，优先级 focus > active > hover | 已补（55 轮） |
| `<input>` 默认态 | 默认 4px 圆角 + 1px 边框（浏览器天生带的），显式声明不覆盖 | 已补（46 轮） |
| `:disabled` | `disabled="true"`（或属性 `Enabled="False"`）→ 文字灰 + 背板压淡 | 已补（47 轮） |
| `border-radius:50%` | 同名 style，回读 `Radius="50%"` → 按短边算成圆 | 已补（48 轮） |
| `<input>` 文字对齐/内衬 | 默认左对齐 + 6px 内衬（浏览器同款），`text-align` 可覆盖 | 已补（49 轮） |
| `line-height` | 同名 style / 属性 `LineHeight`；`1.5` 倍数、`24px`、`150%` 三种写法，走 D2D `SetLineSpacing` | 已补（51 轮） |
| `text-indent` | 同名 style / 属性 `TextIndent`；`2em`、`24px`，首行前置全角空格 | 已补（52 轮） |
| `gap` / `row-gap` / `column-gap` | `gap="10"`、`gap="10 20"`、`row-gap`、`column-gap` | 已有（58 轮补齐 style 里的 `gap:10 30` 两值简写：此前第二个值被丢弃） |
| `flex-grow` | `Grow` / `FlexGrow` | 已有 |
| `flex-basis` | `flex-basis` / `FlexBasis`（`auto` / px / `50%` 都认，控件层容器按百分比算主轴） | 已有 |
| `width:100%` / `height:100%` | `style="width:100%"` | 已补（所有元素，不只 HTML 标签） |
| `width:50%` 等中间百分比 | `style="width:50%"`（布局阶段按父内框换算，不再被 `wcstol` 读成 50px） | 已补 |
| `padding`（1/2/4 值） | `padding` / `Padding` | 已有 |
| `margin`（1/2/3/4 值） | `style="margin:8"` | 已补（所有元素都生效，含 `<XLabel>`/`<row>`/`<div>`） |
| `border-radius`/`color`/`background`/`font-size`/`font-weight`/`text-align` | style 内同名键 → `Radius`/`Foreground`/`Background`/`FontSize`/`FontWeight`/`TextAlign` | 已补（纯别名映射） |
| `text-align: justify/start/end` | `text-align:justify`→两端对齐（`DWRITE_TEXT_ALIGNMENT_JUSTIFIED`，自动开 Wrap）；`start/end`→Left/Right | 已补（56 轮） |
| `text-align` 继承 | 写在容器（`<div>`/`<XStackPanel>` 等）的 style 上，子元素未声明时自动跟随（注入 styledAttrs，不影响 XML 写回） | 已补（57 轮） |
| `inset` / `top` / `left` / `right` / `bottom` | style 内 `top`/`left`/`right`/`bottom` → `Top`/`Left`/`Right`/`Bottom` | 已补（`Right`/`Bottom` 由宿主换算） |
| `<button>`（居中、圆角、可点、默认高 32） | `<XButton>`（默认高 32、文字居中） | 已有 |
| `<label>` 透明无背板 | `<XLabel>`（无背板） | 已有 |
| `<input>` 单行输入 | `<XTextField>` / `<input>` | 已有 |
| 真实父子容器（`SetParent` 到容器、容器内相对坐标） | 目前是扁平兄弟 + 绝对坐标 | 缺失（列为下一轮） |

## 已确认的坑

- `style="width:100%"` 以前被 `wcstol` 读成 `100px`，已改为百分比语义（置 `Layout.fillWidth`）。
- `<row>`/`<column>` 以前在原生 XML 通道直接报「未知控件」，只有 Python `html()` 编译器认；现已注册为控件层 flex 容器。
- `html()` 把 `<row>` 编译成 `<XStackPanel Orientation="Horizontal">`（流式），原生 XML 走 flex 容器（更强）；两者都横排，观感一致。

## 本轮增量（12:4x CSS flex 子项声明）

| HTML/CSS | XML | 状态 |
|---|---|---|
| `flex-basis: 120px` | `flex-basis="120"` / `basis="120"` / `style="flex-basis:120px"` | 已补（basis 与 Width 分离，主轴尺寸独立声明） |
| `flex-shrink: 0` | `flex-shrink="0"` / `shrink="0"` / `style="flex-shrink:0"` | 已补（此前硬编码 1.0，写了也不生效） |
| `flex: 1` | `style="flex:1"` | 已补（= grow 1 / shrink 1 / basis 0） |
| `flex: 2 0 auto` | `style="flex:2 0 auto"` | 已补（三分量简写） |
| `flex-grow: 2` | `flex-grow="2"` / `Grow="2"` / `style="flex-grow:2"` | 已有 |

## 本轮增量（13:0x style 任意百分比 + stretch 语义纠正）

| HTML/CSS | XML | 状态 |
|---|---|---|
| `width: 50%` | `style="width:50%"` | 已补（按容器内容盒换算成主轴尺寸；此前只认 100%，其余被静默吞掉） |
| `height: 33%` | `style="height:33%"` | 已补（交叉轴百分比） |
| `align-items:stretch` 只作用于 auto 尺寸 | 未写 Height 的子项拉满、写了的不动 | 已纠正（旧实现会覆盖显式 Height/Width） |

## 本轮增量（14:35 display 语义 + div 落成控件层容器）

| HTML/CSS | XML | 状态 |
|---|---|---|
| `<div style="display:flex">`（默认主轴 row） | 同名写法 | 已补（此前 style 里的 display 完全被忽略，只能靠私有约定 class="row" 横排） |
| `display:flex` / `inline-flex` | `style="display:flex"` | 已补（元素即 flex 容器，flex-direction 默认 row，与 CSS 一致） |
| `display:block` / 裸 `<div>` | `<div>` | 已补（块级竖排，主轴 column） |
| `flex-direction: column` | `style="flex-direction:column"` | 已补（覆盖 display:flex 的默认 row） |
| `<div>` 作为布局容器 | 落成控件层 `XFlexPanel` | 已补（子项由容器自己排；显式 Width 不再被交叉轴 stretch 吃掉，旧实现会被拉满成整行宽） |

## 本轮增量（15:0x style 外观真正进 owner-draw）

| HTML/CSS | XML | 状态 |
|---|---|---|
| ackground:#2563eb | style="background:#2563eb" / Background="#2563eb" | 已补（此前控件级 setControlAppearance 从未被调用，findAppearance 恒空，外观声明全被丢） |
| color:#ffffff | style="color:#ffffff" / Foreground=... | 已补并修 bug（QML 映射把 oreground 指到 FrontColor，CSS 的文字色到绘制端读不到） |
| border-radius:8px | style="border-radius:8px" / Radius="8" | 已补（61 轮起容器真圆角：控件窗口 SetWindowRgn 裁形，圆角外露父底色） |
| 状态：hover / pressed / disabled | ODS_SELECTED / ODS_DISABLED → 绘制分支 | 已有（宿主四态机） |

## 本轮增量（15:2x 行内文本标签自然宽）

| HTML/CSS | XML | 状态 |
|---|---|---|
| `<label>运行状态</label>` 宽度=文字宽 | `<label>…</label>` / `<span>…</span>` / `<strong>` 等行内标签 | 已补（此前不写 Width 一律铺满整行，等于每个标签都得手写 Width） |
| `<XLabel>` 保持 WPF TextBlock 铺满 | `<XLabel>` | 不变（老界面大量依赖，改了会打散既有排版） |
| `label { width: auto }` | 不写 Width | 已补（CJK 按 1×字号、其余 0.55×字号 + padding 估算） |

## 本轮增量（15:5x 百分比尺寸属性写法）

| HTML/CSS | XML | 状态 |
|---|---|---|
| `width:100%` | `Width="100%"` / `style="width:100%"` | 已补（百分比语义；早先 _wtoi(100%) 读成 100px 的真 bug 已修） |
| `height:100%` | `Height="100%"` / `style="height:100%"` | 已补 |
| `width:50%` | `style="width:50%"` | 已有（flex 容器按容器宽换算） |
| `width:120px` | `Width="120"` | 已有（显式像素优先） |

## 本轮增量（16:0x cad2d 现场迁移 HTML 写法）

| HTML/CSS | XML | 状态 |
|---|---|---|
| `width:100%;height:100%` 作页面根 | `Width="100%%" Height="100%%"`（py 格式化模板里要写 `%%`） | 已补并现场验证（cad2d `page` 由 `anchors.fill="parent"` 迁到百分比写法，`--batch` 截图与迁移前**逐像素相同**） |
| QML `anchors.fill="parent"` | 仅保留兼容 | 已有（不再作为设计目标） |

## 本轮增量（17:1x XML/HTML 双写同一界面对照）

| HTML/CSS | XML | 状态 |
|---|---|---|
| `div` 默认透明（不刷背板） | `<div>`（XFlexPanel 容器） | 已补（此前容器拿主题底色 m_bg 铺一整块，页面比窗口底色差一层灰） |
| `flex-direction:column` + `gap` + `padding` 同一容器 | `style="display:flex;flex-direction:column;gap:12px;padding:16px"` | 已补 bug（容器 padding 被 XmlCore 与控件各算一次，子项整体多偏 16px） |
| 块容器无高度 → `align-items:stretch` 拉满 | `<div style="flex:1">` 子项 | 已补 bug（autoH 被当成显式高度，stretch 不生效，卡片只有内容高 118 而非 224） |
| 嵌套 flex 容器自然宽 = 内容宽 | `<div style="display:flex">` 作子项 | 已补 bug（走 NaturalWidth 兜底 120，两个按钮被压成 56，应为 160） |
| `flex:1` / `flex:2` 简写 | `style="flex:1"` | 已有 |
| `justify-content` / `align-items` / `flex-wrap` / `row-gap` / `column-gap` | 同名 CSS 键 | 已补映射（此前 style 只认 display/flex-direction/gap） |
| `text-wrap` / 垂直对齐 | `white-space` / `text-wrap` → `TextWrapping`；`vertical-align` → `VerticalAlignment` | 已补（`text-wrap:wrap\|nowrap`、`vertical-align:top\|middle\|bottom` 均归一） |

## 本轮增量（17:4x 文字映射 + 容器内拉伸）

| HTML/CSS | XML | 状态 |
|---|---|---|
| `white-space:wrap` / `nowrap` | `style="white-space:wrap"` → `TextWrapping` | 已补映射 |
| `vertical-align:top/center/bottom` | `style="vertical-align:top"` → `VerticalAlignment` | 已补映射（值归一化成 Top/Center/Bottom） |
| `line-height` | `style="line-height:18px"` → `LineHeight` | 已补映射 |
| column 容器里 `button`/`input` 无 width → 拉满 | `<button>` / `<input>` 不写 Width | 已补（默认尺寸表 88/160 不再压住 align-items:stretch） |
| 容器 `background` | `<div style="background:#262B31">` | 已补（82 轮）：drawContainerBackdrops 在 GDI 路径启用，容器 div 的背板由宿主 DC 画在子窗之下 |

## 本轮增量（2026-11-18 18:2x–19:0x 控件层 flex 落地 + 容器 z 序）

| HTML 能做 | XML 写法 | 状态 |
|---|---|---|
| flex 容器自己排子项（不靠外层算坐标） | `XmlFlexPanel`（`computeLayout` 纯计算 + `reflowItems` 自己 `moveControl`） | 已补（容器持 `m_items`，宿主 `attachFlexContainer` 灌子项） |
| 容器随窗口缩放重排 | 容器挂 `WM_SIZE` 子类化，`SetBounds`/`WM_SIZE` 都自己重排 | 已补（HTML 版 704→544 实测卡片 216→162、卡内 label 192→138） |
| `<div style="display:flex">` 与 `<XStackPanel Display="flex">` 同一行为 | `XStackPanel` + `Display="flex"` 归一成 `XFlexPanel` | 已补（默认开；`PYARX_FLEX_NORM=0` 回老路径） |
| 行内 `<label>` 在 flex 容器里被 stretch 拉满 | 父容器递归时剥离行内文本的"内容宽"伪 Width 声明 | 已补（`XmlElement::autoTextW` 标记，父非 flex 时行为不变） |
| 容器背景色（`background:#262B31`） | 容器 `Background` → 宿主背板刷子 | 已补（像素采样 (38,43,49)=#262B31，与 HTML 版一致） |
| 容器层级（父在下、子在上、叶子最上） | 容器按 id **正序** `HWND_BOTTOM` | 已补（逆序会把父容器糊在子容器之上） |
| `style="z-index:N"` / `ZIndex="N"` / `Level="N"` 决定谁压谁 | 同一份属性，链路：style→`ZIndex`→`host->setControlLevel`→HWND 重排 + 节点 `zLevel` | 已补（第 31 轮）：修四处（style 无分支、属性不下发、节点无 zLevel、applyZOrder 的 same 正/逆序比错）；`t_zindex` 7/7，默认=后写在上、±值都可翻层、几何不变 |
| `style="cursor:pointer"` 等光标声明 | `Cursor="pointer"`（属性）/ `style` 内 `cursor` | 已补（第 32 轮）：关键字表 pointer/hand/grab/default/text/ibeam/wait/progress/help/crosshair/move/all-scroll/not-allowed/no-drop/*-resize，显式声明优先于按控件类型猜；`t_cursor` 7/7 |
| 隐藏：`hidden` / `Visible="False"` / `style="display:none"` / `style="visibility:hidden"` | 同名属性或 style 键，全部落到 `host->setVisible` + 宿主 `m_hidden` 集合 | 已补（第 33 轮）：此前这些键只"认名字"没有消费者，写了照旧显示；且重排会把隐藏放回来（moveMany 无条件 SHOW）——已用 `m_hidden` 压住；`t_visible` 7/7。**待补**：`display:none` 的"不占位"（目前只藏不塌陷） |
| 容器身份识别 | `addPanel` 登记 `m_containerIds` | 已补（XML 面板 STATIC 带文本，按"空 STATIC"判会整批漏登记 → 整页空白） |
| `width:100%;height:100%` 跟随窗口 | `Width="100%" Height="100%"`（flex_demo.xml page 已改） | 已补（缩放后 page→cards→c1 逐级收窄） |
| `flex-wrap` / `align-self` / `flex-basis` / `row-gap` / `column-gap` | 同名 XML 属性 | 部分（`ViewCode`已落地 wrap/self/basis，`setOwnProperty` 有映射，完整链路待逐项验证） |
| `<button>`/`<label>`/`<input>` 默认值表 | 不写尺寸也符合 HTML 直觉 | 部分（`button` 88x32 居中圆角、`label` 透明无背板已补；统一默认值表待做） |

### 第 9 轮补充

| HTML 能做 | XML 写法 | 状态 |
|---|---|---|
| 属性对缩放重排持续生效（不只是首帧） | 容器属性灌到真实 XmlFlexPanel 实例 | 已补（此前只进栈上临时对象，重排会跳回默认值） |
| flex-wrap:wrap 多行 | FlexWrap="wrap" / flex-wrap:wrap | 已补（实测 560 宽 2×2、400 宽容器自排成 4 行） |
| row-gap / column-gap 分开设 | RowGap / ColumnGap | 已补（容器 setOwnProperty 已接收并参与重排） |

### 第 11 轮补充

| HTML 能做 | XML 写法 | 状态 |
|---|---|---|
| button 默认宽度随内容（不是固定 88） | `<button>确定</button>` | 已补（52/178 按内容，`style="width:120px"` 可覆盖；`<XButton>` 保持 88） |

### 第 12 轮补充

| HTML 能做 | XML 写法 | 状态 |
|---|---|---|
| min-width / max-width 约束 shrink 与 grow | `style="min-width:150px;max-width:120px"` / `MinWidth="150"` | 已补（初始尺寸/shrink/stretch 三处 clamp，grow 走多轮分配） |
| min-height / max-height | `style="max-height:60px"` | 已补（同批字段，交叉轴投影） |

### 第 13 轮补充

| HTML 能做 | XML 写法 | 状态 |
|---|---|---|
| opacity:0.5 半透明 | `style="opacity:0.5"` / `Opacity="128"` | 已补（0~1 比例 ×255 与 0~255 整数并存，此前 0.5 被读成 0） |
| box-sizing:border-box | `style="box-sizing:border-box"` | 已补（声明化；XML 缺省 padding 内挤，本就等价 border-box） |

### 第 14 轮补充

| HTML 能做 | XML 写法 | 状态 |
|---|---|---|
| align-items:baseline 文字基线齐平 | `style="align-items:baseline"` / `Align="baseline"` | 已补（行内最大基线 + 逐项下移，换行多行各自算） |

### 第 15–19 轮补充

| HTML 能做 | XML 写法 | 状态 |
|---|---|---|
| 子元素真的属于父元素（裁剪/层叠/焦点链） | 无写法（自动：flex 容器内子控件 `SetParent` 到容器） | 机制已实现，**默认关**（`PYARX_FLEX_PARENT=1` 开启）。打开后层级/坐标/可见性全部正确，但卡内文字消失：子控件走 `SS_OWNERDRAW`，文字画在宿主 DC，嵌套后宿主 DC 不含容器区；整类 STATIC 改自绘已被反证失败（会画成色块 + 文本错读）。待走"按控件类型注册自绘 + `drawSkinControl` 补 Static 分支" |
| 父容器刷新不覆盖子控件 | 无写法（自动：容器 `WS_CLIPCHILDREN`） | 已补（`addPanel` 增加该样式；这是真父窗口的必要条件） |
| 父容器换父后子项自动归位 | 无写法（自动） | 已补（`SetParent` 后向容器补发 `WM_SIZE` 触发 `reflowItems`） |
| `padding:4 16`（上下 4 / 左右 16） | `style="padding:4 16"` | 已补（1/2/3/4 值 + 早先只吃单值） |
| `padding-left:30` 单边 | `style="padding-left:30"` | 已补（左右上下四个单边键） |
| `Padding` 属性与 CSS 简写顺序一致 | `Padding="4 16 4 16"`（上右下左） | 已补（此前按"左上下右"解析，`padding:4 16` 被读成左 4 / 上 16） |
| `inset:12 20` 四边定位简写 | `style="inset:12 20"` | 已补（6 轮：同时写两条边时由边反推宽高，不再退回内容宽；百分比边按父内框换算） |
| `right:20;bottom:12` 只写右下两条边 | `style="right:20px;bottom:12px"` | 已补（6 轮：贴父内框右下，宽高仍按声明值/内容） |
| `<input>` 不写尺寸按浏览器默认（size=20 字符、1.3 倍行高） | `<input type="text"/>` | 已补（14 号字 = 162x20，字号变化跟着变；老写法 `<XTextField>` 保 160x26） |

### 第 22–23 轮补充

| HTML 能做 | XML 写法 | 状态 |
|---|---|---|
| `font-weight:600` 数字字重（浏览器最常见的写法） | `style="font-weight:600"` / `FontWeight="600"` | 已补（绘制端只认 Normal/Bold/SemiBold/Light，早先数字被静默落成 Normal；现在 100~350→Light、500~600→SemiBold、≥700→Bold，关键字大小写不敏感） |
| `font-family` | `style="font-family:Consolas"` | 已补（→ FontFamily，绘制端原有能力） |
| `font-style:italic` | `style="font-style:italic"` | 已补（→ FontStyle，大小写归一） |
| `letter-spacing:4` 字距 | `style="letter-spacing:4"` / `LetterSpacing="3"` | 已补（TextFormat 无字符间距 → 走 IDWriteTextLayout1::SetCharacterSpacing；实测 6 字符 +24px；0 时不换路径，无回归成本） |
| `box-shadow:0 4 10 rgba(0,0,0,.5)` 外投影 | 同名 style → `XmlAppearance.boxShadow`，宿主画布逐圈画半透明环 | 已补（94 轮，t_box_shadow 10/10、t_shadow_list 6/6）；`inset` 内投影仍由容器位图刷合成，未在 XML 通路上验证 | 旧记录：**缺失（架构性）**。每个控件在自己的窗口 DC 上自绘，窗口矩形=元素盒子，阴影画在盒外会被窗口直接裁掉，只剩下"把背板压暗一层"。要真落地必须先有"绘制表面"层（父表面统一绘制，与真父子同一批改造）。代码注释已留结论，宁可不支持也不要"写了静默画错" |

### 第 25–27 轮补充

| HTML 能做 | XML 写法 | 状态 |
|---|---|---|
| `flex:0 0 50%` / `flex-basis:30%` 百分比 | `style="flex:0 0 50%"` | 已补（早先 `wcstol("50%")`=50px 真 bug；现在按容器主轴尺寸换算，600 宽里 50%→300；basis 盖过 width，符合 CSS；column 容器按高度换算） |
| `margin:0 auto` 水平居中 | `style="margin:0 auto"` | 已补（auto 早先被 `wcstol` 读成 0，整条声明静默失效；现在 auto 边距先均分剩余主轴空间，剩下的才给 justify-content） |
| `margin-left:auto` 把元素顶到右端 | `style="margin-left:auto"` | 已补（实测 600 宽容器里贴右 480；与 `justify-content:center` 共存时 auto 仍优先） |
| `margin-left/top/right/bottom` 单边写法 | `style="margin-left:8"` | 已补（早先只认 `margin` 简写，单边写法整条被丢弃，作者只能写 `margin:0 0 0 8`） |
| 交叉轴 auto（column 容器里 `margin-left/right:auto` 水平居中） | 同上 | 已补（交叉轴 auto 优先于 align-items；row 容器里 `margin-top:auto` 压底） |
| `margin:auto` 在非 flex 父元素里居中 | 同名简写 → 剩余空间按 auto 均分 | 已补：普通块父里实测居中（探针验证）；仍无块级流式布局 | 说明：宿主是绝对定位坐标系（Left/Top 直接落窗口），非 flex 父没有"剩余空间"概念，要落地得先给普通容器加块级流式布局（reference：与 `overflow`/真父子同属"绘制/布局表面"改造） |

## 第 29–32 轮增量（HTML ↔ XML）

| HTML | XML | 状态 |
|---|---|---|
| `style="border:1px solid #5F6976"` | 同写法（四键 `border` / `border-width` / `border-style` / `border-color`，顺序无关） | 已补（29 轮） |
| `border-top/right/bottom/left` 及 `-width/-color/-style` | 同名 style（含简写 token：`2px solid #333`）→ `Border<Top|Right|Bottom|Left>[Width|Color|Style]` | 已补（84 轮，t_border_side 10/10；边序 上/右/下/左，`0`=显式无边，未声明边沿用整圈） |
| `outline` / `outline-width` / `outline-color` / `outline-style` / `outline-offset` | 同名 style → `Outline[Width|Color|Style|Offset]`；不占盒模型，画在盒外（offset 可负） | 已补（85 轮，t_outline 4/4；需宿主层落笔，控件自绘会被子窗口裁掉；容器 div 上的 outline 待补） |
| `overflow:hidden/auto/scroll` | 同名 style → `Overflow`；容器窗口收 region + 绘制层按祖先容器矩形求交裁剪 | 已补（86 轮，t_overflow 3/3；`visible`/未声明 = 不裁，与 HTML 默认一致；`scroll` 目前只裁不滚，滚动条待补） |
| `border-style:solid/dashed/none` | `BorderStyle` 自动归一为 `Single` / `None` | 已补（29 轮，修"写了却画不出"） |
| `background:linear-gradient(...)` | `BgImage` 属性（可回读原串） | 契约已有；div 容器视觉已补（第86轮 CTLCOLOR 位图刷通道）；button 叶子已补（第88轮 OnDrawItem 自绘 D2D 渐变铺底）；input 叶子已补（第90轮皮肤层铺底，drawSkinControl 调色板 + drawEdit 渐变优先，非 CTLCOLOR） |
| `background:rgba(r,g,b,a)` | 拆成 `BackColor=#RRGGBB` + `BgAlpha`（只作用背景，文字/边框不透明） | 已补（31 轮） |
| `background:#RRGGBBAA` | 同上（CSS alpha 在末尾） | 已补（31 轮） |
| `opacity:0.5` | `Opacity`（0~255），整块混合 | 已有 |
| `style="display:none"` | `Visible=False`（控件仍存在、不显示） | 已补（32 轮） |
| `hidden` 属性 | `Visible=False`（仅 HTML 小写标签） | 已有 |
| `display:none` 的元素不占位 | 布局跳过隐藏项（flex 主轴与 justify 都不算它） | 已补（33 轮） |
| `box-shadow` | 同名 style（含多阴影列表） | 已补（94 轮）：外投影改由宿主整窗画布落笔（盒外环带），不再被控件窗口裁掉 |
| `overflow:hidden` 裁剪 | 同名 style → `Overflow`，容器窗口收 region + 绘制层求交裁剪 | 已补（86 轮，t_overflow_scroll 7/7；scroll 目前只裁不滚） |
| `text-decoration:underline` | 同名 style → `TextDecoration`，按文字基线自绘（39 轮）+ 5 轮修基线 | 已补 |

## 第60轮补充：style 里的 flex 声明

落地方式：`ApplyStyleFlexProps`（`MfcXmlCore\XmlCore.cpp`，`namespace htmlsugar`）在 `xmlApplyHtmlSugar` 之前遍历整棵树，把 style 里的 flex 声明写成本地属性，并同步 `flexSelf` 字段——因为属性写法本来生效、style 写法失效，根因是糖化会把 `<div>` 重建成 `<XFlexPanel>`，布局读到的是重建后的节点。

| HTML/CSS 写法 | XML 等价 | 状态 |
|---|---|---|
| `style="display:flex"` | `<div>` / `Display="flex"` | 已补 |
| `style="flex-direction:column"` | `FlexDirection="column"` | 已补 |
| `style="flex-wrap:wrap"` | `Wrap="wrap"` | 已补 |
| `style="justify-content:center"` | `Justify="center"` | 已补 |
| `style="align-items:flex-end"` | `Align="flex-end"` | 已补 |
| `style="align-self:flex-end"` | `align-self="flex-end"` / `Self="..."` | 已补（字段同步） |
| `style="gap:10"` | `gap="10"` / `Gap="10"` | 已补 |
| `style="flex:1"` | `Flex="1"` | 已补 |
| `style="flex-grow/shrink/basis"` | `Grow/Shrink/Basis` | 已补 |

验证：`tests\xml_ui\t_style_flex.py` 6/6（align-self 起止、justify、align-items、column、gap）。

## 第61-64轮补充：容器圆角 / 自适应重排 / 块级百分比 / order

| HTML/CSS | XML 等价 | 状态 |
|---|---|---|
| `style="border-radius:12"` 在 `<div>` 上出圆角 | 控件窗口 SetWindowRgn 裁形，圆角外露父底色 | 已补（61 轮，t_div_radius） |
| 窗口 resize 时布局自动重排 | 宿主 `OnSize` 里 `reflowXml()`，无需 `win.onResize` 重建 | 已补（62 轮，t_resize_reflow） |
| `width:50%` / `height:50%`（块级、非 flex） | `style="width:50%"` 或 `Width="50%"` | 已补（63 轮，t_pct_block） |
| `order:-1` / `order:2` | `style="order:-1"` 或 `Order="-1"` | 已补（64 轮，t_flex_order） |
| `align-content`（多行交叉轴分布，六值全） | `style="align-content:center"` / `AlignContent` | 已补（65 轮，t_align_content） |
| 嵌套容器背板：`<div style=background>` 在 `<div style="display:flex">` 内 | 同左（无需额外写法） | 已补（71 轮，t_nested_div）：无背板带子容器退化成空 region，子项不再被容器窗口盖住 |
| 三层嵌套：卡片里的盒子（父容器有背板） | 同左 | 缺失（71 轮定位 XFAIL）：父容器保留窗口会遮住兄弟子窗，待真父子层级（SetParent 到容器） |
- `box-shadow`（`0 4 10 rgba(0,0,0,.55)`）→ `BoxShadow` / `Shadow`：**已补**（第79轮解析+回读；第80轮 DrawBackplate 内 DrawBoxShadow 逐圈近似 + 宿主背板 clip 放宽 80px，但 div 容器当前靠自身 CTLCOLOR 刷子刷底，不经 DrawBackplate → 视觉待容器窗口自绘接管）
- `:disabled`（禁用态降对比）→ `disabled="true"`：**已补**（背板轻微淡出 + 文字转 RGB(140)，自绘与 owner-draw 两条路径一致）
- 匿名块级容器（`<div>`/`<section>`/`<form>`/`<fieldset>`/`<row>`/`<column>`）→ 不登记进 names：**已补**（自动名仍写 child.name，仅不对外暴露，与 HTML"布局容器不是控件"一致）

## 第 85 轮增量：HTML 自然尺寸口径（t_xml_ergonomics 期望对齐，只改测试期望，未动布局代码）

| HTML/CSS | XML 等价 / 尺寸口径 | 状态 |
|---|---|---|
| `<button>` inline-block 自然宽 / 自然高 | `<XButton>` 不写 Width/Height → 宽 88（内容宽估算只对原生小写 `<button>` 生效，老写法走兜底 88）、高 30；旧 catalog 兜底为 100×32，已废弃 | 已校正期望（XmlCore.cpp 默认值表：宽 1144、高 1183；旧兜底 NaturalWidth 1757 / DefaultHeight 1738） |
| 块容器内容高 = 子项自然高求和 + gap + 上下 padding | 竖排容器：`padT + Σ子项自然高 + spacing×(n-1) + padB`；横排行高取 max(子项)。本样例：label 24、行 max(输入26, 按钮30)=30、checkbox 24 → 12+24+8+30+8+24+12=118 | 已校正期望（XmlCore.cpp `xmlNaturalHeight` 3181-3223；label 24 / checkbox 24 兜底 1737/1740） |
| 单控件作片段根：根高 = 控件自然高 | 根即控件、无额外 padding；窗口建完按真实 style/exstyle 再校准非客户区，客户区精确等于内容高 → 按钮根客户区 (320, 30)（旧期望 32） | 已校正期望（PyMfcLoad.cpp `FragmentRootHeight` 1821-1825 调 `xmlNaturalHeight`；非客户区校准 1948-1966） |
| 附：老写法 `<XTextField>` 不写尺寸 | 160×26（原生 `<input>` 才按 size=20 字符 / 1.3 倍行高；`HtmlInput` 标记区分两条路径）；本测试旧期望 120 同步为 160 | 已有（XmlCore.cpp 1144/1183；对照 t_html_input.py:36） |

## 第 86 轮增量：容器渐变 / 盒内阴影的像素验收口径（副线，只记断言与采样方式）

| HTML/CSS | XML 等价 / 采样验收口径 | 状态 |
|---|---|---|
| `background:linear-gradient(...)` 容器盒内真出过渡色 | `<div style="background:linear-gradient(180deg,#000,#FFF)"/>`；`t_gradient.py` 区域扫描断言：沿中轴在整窗内定位渐变带（黑端=亮度最低行、白端=亮度最高行，跳过顶部 1/6 避标题栏图标），带内取 7 点，亮度单调（容差 8/255）、相邻平均步长 > 12/255、首尾贴近 #000000 / #FFFFFF；内框 6px 粗扫唯一色数 > 1 判非纯色。不写死绝对坐标，规避 savePng 整窗（含标题栏）偏移 | **已补**：主线 CTLCOLOR 位图刷通道落地，t_gradient 12/12 通过（flexbuild 与 Release 复验一致） |
| `box-shadow` 盒内（inset / 容器盒内阴影） | 同属性 `BoxShadow` / `Shadow`；验收口径同上：盒内区域扫描（`getRect` 内框粗扫），不依赖固定客户区坐标 | **仍缺**：div 容器走 WM_CTLCOLORSTATIC 纯色刷子，不经 DrawBackplate，盒内阴影无绘制通道；区域扫描采样函数已在 t_gradient 渐变用例留好同款，可直接复用 |
| 主线落地补记（第86轮）：div 容器 `linear-gradient` | OnCtlColor 容器自身分支返回渐变 DIB 位图刷；`ParseLinearGradient` 角度起点与色标后缀两处缺陷已修；纯色 SolidBrush 快路径不变 | 已补（容器 CTLCOLOR 位图刷；flexbuild 与 Release 复验 t_gradient 12/12 通过） |
| 主线落地补记（第86轮）：inset 盒内阴影 | 本轮未接入同一刷子通道 | **仍缺**：下轮在同一位图刷内合成盒内暗带，先划清盒内暗带与盒下扫描口径的边界（t_box_shadow D 断言盒下无暗带） |

## 第 87 轮增量：inset 盒内阴影断言基线（副线，只补像素断言，未动解析/刷子）

| HTML/CSS | XML 等价 / 采样验收口径 | 状态 |
|---|---|---|
| `box-shadow: inset 0 2px 6px rgba(0,0,0,.55)` 盒内上沿压暗渐隐带 | `<div Name="box" Left="40" Top="40" Width="160" Height="90" style="background:#FFFFFF;box-shadow:inset 0 2px 6px rgba(0,0,0,.55)"/>`；t_box_shadow E 组：沿中列在整窗内定位浅色盒子带（跳过标题栏 1/6，偏移免疫，同 t_gradient 口径）；E1 近上沿内 1~3px 平均亮度较盒中心低 ≥12/255（理论最强混合 ≈115/255 vs 中心 255，12 为保守下限）；E2 上沿→中心 5 点亮度单调趋亮（容差 8/255）；E3 盒外上沿外/下沿外各 8px 带与页底 #202020 差值 ≤12、无暗带；失败信息带实测采样值（top/center/drop/prof/above/below/dev/band） | **已补（容器 CTLCOLOR 位图刷合成）**：inset 与底色/渐变合成进同一张容器 DIB 位图刷，按 id+样式+几何签名缓存；无 inset 时缓存键与像素保持第 86 轮快路径不变；t_box_shadow 8/8 通过（flexbuild 与 Release 复验一致） |

## 第 96 轮增量：inset spread 接入内投合成

| HTML/CSS | XML 等价 / 采样验收口径 | 状态 |
|---|---|---|
| `box-shadow: inset 0 2px 6px 10px rgba(0,0,0,.55)` / `-4px` spread 内阴影 | 同 E 组双 div 对照（420×160 窗，两 div 160×90、#FFFFFF，中列定位盒子带）；F1 正 spread：上沿内 12px 深处正 spread 条较 spread=0 暗 ≥12/255；F2 负 spread：近上沿 1~3px 负 spread 条较 spread=0 亮 ≥12/255，最暗行更靠上（容差 3px，辅证不单独判红） | **已补（内投距离基准）**：spread 作用在距离基准 `d' = max(0, d - spread)`——正 spread 暗带向内扩展，负 spread 暗带更贴边；不改位图尺寸（位图即客户区几何）；缓存键仅当 `spread != 0` 才在 inset 签名段追加 spread 值，无 inset 与 spread=0 单 inset 的键串逐字节不变；外投 `DrawBoxShadow` 既有 `InflateRect` 路径未动，未碰渐变通道与多阴影解析。t_box_shadow 10/10 通过（flexbuild 与 Release 复验一致） |

## 第 88 轮增量：XButton 渐变落地（OnDrawItem 自绘路径）

| HTML/CSS | XML 等价 / 采样验收口径 | 状态 |
|---|---|---|
| `background:linear-gradient(180deg,#4A5568,#2D3748)` 叶子按钮真出过渡色（OnDrawItem 自绘路径，非 WM_CTLCOLOR） | `<XButton Name="g" Left="40" Top="60" Width="140" Height="36" style="background:linear-gradient(180deg,#4A5568,#2D3748)">渐变</XButton>`；t_gradient B1–B4：沿按钮内偏左 25% 列（非纯色粗扫另取 20%/30% 两列；文字水平居中，取样列在字形左侧之外）在整窗内定位按钮带（跳过标题栏 1/6，行通道和 > #202020+30，偏移免疫，不写死绝对坐标）；带内缩 2px 取 7 点顶→底：B1 唯一色数 > 1 判非纯色；B2 亮度单调递减（容差 8/255）；B3 相邻平均亮度步长 > 4/255（两端亮度差 30/255，7 点均分每段 ≈5，纯色台阶步长≈0）；B4 顶端逐通道贴近 #4A5568、底端贴近 #2D3748（容差 18/255，含首末行栅格化与边框抗锯齿余量）；失败信息带 top/bottom/prof/lum/ys/uniques/steps/band/col | **已补（OnDrawItem 自绘 D2D 渐变铺底）**：`paintBack` 判据补入 `bgImage` 非空，渐变按钮不再跳过背板；背板复用既有 `xmlDrawBackplate` 的 D2D 线性渐变分支（D2DDcCanvas 真渐变笔刷，按 c1/c2/angle 缓存），无渐变按钮仍走原纯色填充，三态取色逻辑未动；t_gradient 16/16 通过（Release 复验一致） |
| `<input>/<XTextField>` 渐变背景 | 真 EDIT 类；底色由皮肤层铺底决定（非 CTLCOLOR） | **已补（皮肤层铺底：drawSkinControl 调色板 + drawEdit 渐变优先，不是 CTLCOLOR）**：`drawSkinControl` 配色段解析 `ap.bgImage` 的 CSS linear-gradient 进 Palette 渐变字段（`hasGradient/gFrom/gTo/gAngle`，不覆盖 `pal.back` 纯色语义；角度换算同 xmlDrawBackplate，Blend 对主题底）；`drawEdit` 首行渐变优先——有渐变走既有 `cv.fillRectGradient`（D2D 真线性渐变笔刷），无渐变回落原纯色 `fillRect(rc, pal.back)`；Edit 与 RICHEDIT50W 同一路径。未动圆角/边框/cue/选区/光标/IME/只读态/字号字色。t_gradient E1–E4：E1 唯一色数 6；E2 亮度 81→56 单调递减；E3 步长 3~5；E4 顶 (72,83,101)、底 (47,58,75) 在 #4A5568/#2D3748 容差 18 内，20/20 通过（Release 复验一致）。第 88 轮挂在 `WM_CTLCOLOREDIT` 的工厂被子类化旁路，仅留作历史路径，不作为 EDIT 渐变落点 |

## 第 93 轮增量：无样式 `<input>` 独立字段底色（HTML 语义）

| 缺口 | 规格 | 落点与验收 |
| --- | --- | --- |
| 无样式 `<XTextField>` 内部与页面/主题底同色，只剩边框；HTML `<input>` 应有独立字段底色 | 有 Background 窗 + 无 style 输入框 interior ≠ 页面底；无 Background 深色窗 interior ≠ 页面底且 ≠ 主题 m_bg；采样沿中列扫描、跳过标题栏 1/6、内缩 2px 取中段 5 点均值，不写死绝对坐标 | **已补（落点 A：皮肤层配色段，非 CTLCOLOR）**：`drawSkinControl` 配色段对 Edit/RICHEDIT50W 且未显式声明 backColor 时，`back` 不取页面底 `m_bg`，改取主题字段色 `m_btn`（深 (62,70,80) / 浅 (240,242,245)）；显式 backColor（如 #336699）仍走原声明优先级，`pal.back` 纯色语义、三态取色与 disabled 混色逻辑未动。像素实测字段带 interior = (62,70,80)；t_input_defaults 8/8（含 #336699 精确护栏），Release 复验一致。注：`WM_CTLCOLOREDIT` 在本机被子类化旁路、未被调用，EDIT 可见底色落点仍是皮肤层铺底，不改原生 CTLCOLOR 通道 |


## 第 8 轮增量：HTML 语义标签默认样式（不写 CSS 也对）
| HTML | XML 等价 | 状态 |
|---|---|---|
| `<strong>` / `<b>` | 落 `XLabel` 并自动 `FontWeight=Bold` | 已补（8 轮，显式 font-weight 优先） |
| `<em>` / `<i>` / `<cite>` / `<var>` | 自动 `FontStyle=Italic` | 已补 |
| `<u>` / `<ins>` | 自动 `TextDecoration=underline` | 已补（配 5/7 轮的基线自绘） |
| `<del>` / `<s>` / `<strike>` | 自动 `TextDecoration=line-through` | 已补（配 7 轮删除线绘制） |
| `<h1>`–`<h6>` | 自动字号 32/24/19/16/13/11 + 加粗（浏览器默认字表） | 已补 |
| `<small>` | 自动 `FontSize=13` | 已补 |
| `<mark>` | 自动 `Background=#ffff00` | 已补（底色走控件背板通道） |
| `<code>` / `<pre>` / `<kbd>` / `<samp>` / `<tt>` | 自动 `FontFamily=Consolas`；`<pre>` 额外 `TextWrapping=Wrap` | 已补（**修真 bug**：此前落未注册的 `XCodeEditor`，加载直接报"未知控件"，宽还是铺满） |

| `<a href="…">文字</a>` | 落 `XLabel`：默认蓝色 `#0066cc` + 下划线，href 映射 `Url`，内嵌 `<span>/<b>` 正常转换 | 已补（10 轮，修真 bug：文字丢失、嵌套标签报"未知控件"） |
| `min-width` / `max-width`（非 flex 流） | `style="min-width:200;max-width:150"` 或属性 `MinWidth`/`MaxWidth`，支持 px 与百分比 | 已补（11 轮，块级流/绝对定位生效；flex 子项早已支持） |
| `min-height` / `max-height`（非 flex 流） | `style="min-height:200;max-height:100"` 或属性 `MinHeight`/`MaxHeight`，px 与百分比 | 已补（12 轮） |
| CSS 继承（color / font-* / text-align / line-height / letter-spacing / white-space / text-transform / text-indent） | 父元素声明即对子控件生效；子元素自己写了则覆盖 | 已补（13 轮，`xmlApplyHtmlSugar` 之后整树下发；`PYARX_CSS_INHERIT=0` 可关） |
| `vertical-align`（元素级） | `style="vertical-align:middle"` → `VerticalAlignment` | 已有（此前 map 误标缺失，代码 4456 行早已映射，本轮复核更正） |
| `text-decoration` 传播 | 父元素 `style="text-decoration:underline"` → 子树文字带下划线；子元素写 `none` 取消 | 已补（14 轮，CSS 规定的"传播"而非继承） |
| `<button>` UA 默认背板 | `background:#efefef` + `border:2px #767676` + `color:#111`（未显式声明时才补） | 已补（15 轮，像素验收） |
| CSS `border` 简写的**绘制** | `style="border:2px solid #f00"` 解析出的 `BorderStyle=solid` 现在会被绘制端认（原先只认 Single/Sunken/Raised，简写"解析成功、画不出来"） | 已补（15 轮，`xmlapp::HasVisibleBorder`） |
| 容器（`<div>`/面板）自身边框绘制 | `style="border:2px solid #f00"` 写在 `<div>` 上 | 已补（82 轮，同一背板通道；t_container_border 13/13） |
| 容器 `<div>`/面板自身 CSS 边框 | `style="border:2px solid #f00"` 写在 div 上现在真出框（空 STATIC 走 CTLCOLOR 位图刷，边框合成进刷子） | 已补（16 轮） |
| 容器 `linear-gradient` 通道序 | 容器位图刷写 DIB 时 R/B 互换（`linear-gradient(#ff0000)` 画成纯蓝） | 已补（16 轮真 bug） |
| 画布后端 | 缺省 D3D11 设备 + DXGI 交换链 + D2D 目标（硬件优先→WARP→GDI 三级回退），`PYARX_SURFACE=0` 强制 GDI | 已补（18 轮，默认开启） |
| `border-style` 线型 | dashed/dotted/double/groove/ridge 不再被归一成 Single，容器按 CSS 语义画虚/点/双/明暗半道 | 已补（17 轮） |
| 容器圆角外角 | 圆角之外的像素露出页面底色（之前涂成盒子底色） | 已补（17 轮） |
| 嵌套容器内的子项落到宿主绝对坐标（`getHostWindow`） | 无写法（自动） | 已补（容器 reflowItems 换算到宿主客户区，不再只换到父窗口） |
| `transform`（2D/CSS 全套） | `style="transform:translate(40px,10px) scale(2) rotate(15deg)"` | 已补（23 轮）：style→外观→4×4 矩阵→D2D 投影，绕盒中心；声明后控件只走节点单通路（隐藏 HWND，避免 WM_DRAWITEM 再画一份未变换正文），文字不再按未变换盒裁剪 |
| `transform` 后的溢出绘制 | CSS overflow 语义（变换不裁剪自己） | 已补（23 轮）：带 transform 的节点裁剪框放开到宿主客户区 |
| 容器内非 flex 子项的绝对定位 | `div` 里 `<label style="left:10;top:10;width:160;height:32">`（div 不声明 display:flex） | 未支持（25 轮定位）：子窗口建窗尺寸恒 0（节点 rc=容器原点、宽高 0），必须靠 flex 才能落位；应按容器 padding box 解析 left/top/width/height |
| 容器 `<div>` 自身 `transform` | `div style="transform:rotate(-15deg)"` | 已补（25 轮）：祖先链 × 自身累计 4×4 矩阵，统一在节点帧推入；容器窗口隐藏、子树（含 label/button）随宿主一起按同一矩阵绘制；命中按变换后四边形 `boost::geometry::contains`。非 flex 容器内的绝对定位子项（`left/top/width/height`）几何仍不落地，见下一行 |

| `text-overflow: ellipsis` | `style="text-overflow:ellipsis"` / `TextOverflow="ellipsis"`（单行放不下补 "…"，默认 clip） | 已补（24 轮）：统一到 TextFlag，WM_DRAWITEM 与表面节点帧两条通路结果一致 |
| `text-shadow:2px 2px 0 #ff0000` | `style="text-shadow:2px 2px 0 #ff0000"` / `TextShadow="..."`（`0 0 4 #000`、`none` 关闭） | 已补（39 轮）：`XmlAppearance::textShadow` + `ParseTextShadow`（颜色/rgba 可选，其余按序 ox/oy/blur）；`D2DCanvasBase::setTextShadow` 在 `text()` 内先画偏移阴影层再画正文；实际落点是 owner-draw 的 `PyMfcProcs.cpp::OnDrawItem`（label/button 文字通路），不是 `drawOwnerNode`。同时修掉 `setControlAppearance` 分批覆盖导致后加字段丢失的 bug |
| `animation` / `@keyframes` | `style="animation:pulse 900ms linear infinite"` + 属性 `Keyframes="0:#f00;50:#0f0;100:#f00"`（HTML 的 @keyframes 块在 XML 里用一条属性替代） | 已补（51 轮，t_animation 4/4）：颜色关键帧 + infinite 循环；尺寸/位移关键帧、animation-delay 未做 |
| `position:fixed`（在可滚容器里不随内容滚动） | `style="position:fixed"` → `Position` 属性 | 已补（50 轮，t_overflow_scroll 7/7）：只落地"不跟滚"；`sticky` 已补（59 轮，t_sticky 6/6）：`position:sticky` + `top:8px` 吸顶，基准 y 首次滚动时惰性捕获；"脱离容器裁剪"仍未做 |
| `border-top-left-radius` 等分角圆角 | `style="border-top-left-radius:40px"` / 属性 `BorderTopLeftRadius`（未声明角回落统一 `border-radius`） | 已补（48 轮，t_corner_radius 5/5）：D2D 走路径几何、容器背板逐角切角 |
| `overflow:scroll` 内容滚动 | 同名 `style="overflow:scroll"` / `overflow:auto`；滚轮在容器（或其子控件）上时内容整体上下滚，每格 40px，clamp 在 `0..contentH-viewH` | 已补（47 轮，t_overflow_scroll 5/5）：容器 id 登记 + 子窗口 SetWindowPos 平移；**滚动条外观仍缺**，`overflow:auto` 尚未区分"无溢出不滚" |
| `pointer-events:none` | `style="pointer-events:none"` / `PointerEvents="none"`；脚本可 `win.setPointerEvents(id,"none"/"auto")` | 已补（40 轮）：控件窗口挂 `WS_EX_TRANSPARENT`，鼠标从命中链消失、点击落到下面兄弟；外观完全不变。`getPointerEvents` 读表、`isMouseTransparent` 读真实扩展样式 |
| `position:fixed` / `sticky` | — | **仍缺**：XML 侧没有任何 `position` 关键字（绝对定位靠 `left/top` 隐式触发），固定到视口不随容器移动无法表达 |
| `aspect-ratio:16/9` | `style="width:160;aspect-ratio:16/9"` → `AspectRatio`；`2/1`、`1.5` 同样认 | 已补（41 轮普通流/绝对定位、43 轮 flex 子项）：只补未声明的那一边，且 43 轮起改为**对称补全**（只写 height 也能按比例推出宽，不被 align-items:stretch 拉满）。flex 容器内实测通过：`<div>` 里 `width:160`+16/9 → 160×90，`height:90`+16/9 → 160×90（t_aspect_ratio 7/7） |

| DWG 真正渲染（不依赖 SVG） | `dwg_render.exe in.dxf out.bmp`；DWG 侧 `dwg2dxf.ps1`（AccoreConsole: FILEDIA=0 + _DXFOUT R2004） | 已补（27 轮）：新库 `MfcXmlCore\XmlVector.h` 自研扫描线光栅（填充/描边/AA/线性渐变/4×4 变换），DXF 解析器支持 LINE/LWPOLYLINE/POLYLINE+CIRCLE+ARC+INSERT；无 SVG、无 D2D 几何管线 |




| `object-fit`（图片在盒子里的缩放模式） | `style="object-fit:contain"` 或属性 `ObjectFit="cover"`，值 fill/contain/cover/none/scale-down | 已补（52 轮，t_object_fit 5/5）：图片盒子改自绘、按 CSS 语义缩放与裁边；`object-position` 已补（53 轮，t_object_fit 6/6，支持 left/center/right/top/bottom 与百分比；px 长度值已补（57 轮，`object-position:30px 20px` 走绝对偏移，t_object_fit 7/7）） |




| `<img>` 不写 width/height → 按原图自然尺寸占位（intrinsic size） | 直接 `<img src="……"/>`，无需写 Width/Height | 已补（60 轮，t_img_natural 3/3）：加载前用 WIC 只读文件头取像素尺寸写进 Width/Height，再进布局；此前一律按布局默认盒拉伸 |
| `<img>` + `max-width:100%`（响应式图片，超宽按容器等比缩放） | 直接 `<img src="…" style="max-width:100%"/>` | 已补（61 轮，t_img_natural 5/5）：布局夹窄宽度后按 NaturalWidth/NaturalHeight 等比重算高，344 宽容器里 400×200 → 344×172 |
| 多文档文件页签条（文件名 + 活跃高亮 + 脏标记 + 关闭 × + 末尾 + 新建） | <XFileTab style="left:0;top:0;width:100%;height:34"><XFileTabItem Text="平面图.dwg" Active="true"/><XFileTabItem Text="总装.dwg" Dirty="true" Closable="true"/></XFileTab>，另支持 ShowAdd / ActiveIndex，事件串 activate:N / close:N / add | 已补（63 轮，t_filetab.py 19/19 全通过）：HTML 侧无原生文件页签，只有 role=tablist 语义 + CSS/脚本拼装，故 XML 侧自建 XFileTab；Rebuild EXIT=0（log\build_audit_181230.txt，dll 18:12:19 / pyd 18:12:46），交互串 activate:N / close:N / add 已实跑验证 |
| `input{font-size:16px;font-family:…}` | `<input style="font-size:16;font-family:'Microsoft YaHei'"/>` / `<XTextField FontSize="16" FontFamily="Microsoft YaHei"/>` | 已补（62 轮，t_input_font 8/8）：字号/字体名/字重真正下发到原生 EDIT（WM_SETFONT），聚焦前后同一把 HFONT，不再"点进去换字形" |
| `align-content: stretch`（wrap 后各行等高，剩余交叉空间均分） | `style="align-content:stretch"` | 已补（65 轮，t_flexpanel 41/41）：computeLayout 按行均分剩余交叉空间，400×120 两行各 60 高 |
| `flex-direction:column` + `flex-wrap:wrap`（列方向换列） | 同名 `flex-direction:column` + `flex-wrap:wrap` | 已有（65 轮实测确认）：200×120 容器 + gap="8 12" 同列项距 38、列距 92；容器 Height 缺省时不按行撑高仍待做 |
| `inset` 四值简写（left+right 撑宽、top+bottom 撑高） | `style="inset:10 20 5 40"` | 已有（65 轮勘误）：实测 220×120 容器得 160×105、左偏 40、上偏 10；原「尚未实现」注释已改正 |
| `display:flex;flex-wrap:wrap` 容器不写 Height（高度按行累加） | 同名 `display:flex;flex-wrap:wrap`，不给 Height | 已补（66 轮，t_flexpanel 47/47）：控件层 measureDesiredCross 回报「各行高 + row-gap」，400 宽 3×150 → 容器 120 高，gap=10 → 130 |
| `flex-wrap` 容器显式写死 Height（不应被内容撑大） | `Height="24"` + `flex-wrap="wrap"` | 已有（66 轮防回归）：显式 24 仍 400×24、子项 24 高 |
| `flex-direction:column;flex-wrap:wrap` + 显式 Height（自动裂多列、宽度撑开） | 同名 `flex-direction:column` + `flex-wrap="wrap"` + `Height="120"`，不写 Width | 已补（67 轮，t_flexpanel 52/52）：3 个 150×60 子项 → 2 列，无 gap 宽 300、`gap="0 12"` 宽 312（第二列 x=162）；无显式高不拆列 |
| column+wrap 容器显式写 Width（作者优先，不被撑开） | `Width="150"` + column + `flex-wrap="wrap"` | 已有（67 轮防回归）：保持 150，第 3 项溢出 x=150 |
| flex 容器的子项是真子节点（跟随移动 / 容器内层叠 / 焦点链正确） | 无需写法：`<FlexBox>`/`<row>`/`<column>` 子项默认 SetParent 到容器；`PYARX_FLEX_PARENT=0` 可退回扁平兄弟 | 已有（68 轮实测钉死）：子项 GetParent 即容器，容器平移 +40 时子项 dx=40 不经 reflow；SetParent 只换父不改 hwnd，事件/皮肤 prop 不丢 |
| 超出容器的子项被容器裁剪 | 容器自带 `WS_CLIPCHILDREN`，无需额外声明 | 已有（68 轮）：第二行 C (108,161,100,30) 溢出容器底 131 即被裁；像素级采样留待下一轮 |
| `padding` / `margin` 简写顺序 | CSS `padding:纵 横`（`12px 0` = 上下 12、左右 0）；XML 属性 `Padding` 是「左 上 右 下」，两值形式 = 横 纵 —— 直接照抄会整体反 12px（第 111 轮 cad2d 改写时踩到并验证） | 已有（顺序差异已核实，见 111 轮） |
| `:hover{border-color:…}` / `:focus{border-color:…}` / `:active{…}` | 同名状态规则 → `HoverBorderColor` / `FocusBorderColor` / `ActiveBorderColor`（不改几何） | 已补（115 轮，t_css_class 52/52 + 像素实证 #333→#00FF00） |
| `:hover{opacity:.5}` / `:focus{opacity:.8}` | 同名状态规则 → `HoverOpacity` / `FocusOpacity` / `ActiveOpacity`（0~1 与 0~255 双写法） | 已补（116 轮，t_css_class 55/55 + 像素实证 白底 217→124） |
| `@keyframes name{0%{…}100%{…}}` + `animation:name 1s` | `<style>` 里的 `@keyframes` 块（含 `from/to`、`0%,100%` 并列、`@-webkit-keyframes`）→ 内部帧表，`animation` 只写名字 | 已补（113 轮，t_css_class 45/45；只插值颜色，尺寸位移帧未做） |
| `content:"[" attr(data-tag) "]"`（伪元素里引用属性值） | 同名 `content`，可与字符串片段混排、可多个 `attr()` | 已补（114 轮，t_css_class 48/48：`[A] 甲` / `[B] 乙` 随 `data-tag` 变） || `:hover{transform:scale(1.2)}`（悬停轻微放大） | 同名状态规则 → `HoverTransform` / `FocusTransform` / `ActiveTransform`，只改绘制不改布局 | 已补（118 轮，t_css_class 58/58 + t_css_transform 11/11，像素实证 120×50→132×55） |
| `:hover{box-shadow:0 8px 20px rgba(0,0,0,.55)}`（悬停抬起投影） | 同名状态规则 → `HoverShadow` / `FocusShadow` / `ActiveShadow`，绘制端按 focus→hover→常态取值 | 已补（119 轮，t_css_class 61/61 + t_box_shadow 10/10，像素实证 盒外暗像素 0→364） |
| `:disabled{background-color:#ccc;color:#999;border-color:#bbb}`（禁用态换皮） | `DisabledBackground` / `DisabledColor` / `DisabledBorderColor` / `DisabledOpacity`；几何声明仍静态合、外观声明走状态 | 已补（120 轮，t_css_class 64/64 + t_html_elements 147/147，实测真禁用后底色 (211,211,211)） |
| `<button disabled>` 真禁用 | `disabled`（HTML 标签）→ `Enabled="False"` → `setEnabled(false)`，绘制走 Disabled* 分支 | 已补（121 轮，IsWindowEnabled=0 实测，t_css_class 66/66） |
| `<XButton disabled>` / `<XTextField readonly>`（XML 标签也用 HTML 拼写） | 统一在 PyMfcLoad 侧收敛成 Enabled/IsReadOnly，再下发宿主；PropAlias 改大小写不敏感 | 已补（122 轮，WS_DISABLED 与 ES_READONLY 实测，t_css_class 69/69） |
| 属性别名（`readonly` / `IsReadOnly` / `ReadOnly`） | 属性表 `Aliases` 字段（`ReadOnly|readonly`），主名保持 `IsReadOnly`；Name 字段不得含 `|` | 已补（123 轮，ES_READONLY 四拼写实测，t_css_class 70/70） |
| `borderRadius` / `backgroundColor` / `fontSize` / `textAlign`（驼峰声明名） | 与 kebab 写法等价：`CssKeyNormalize` 驼峰展开成 kebab 后进同一分支 | 已补（124 轮，5 组对照实测一致，t_style_box 9/9） |
| `color:#cc0000`（HTML 写法） | 内部存 Foreground，`Color` / `Foreground` / `forecolor` 三名等价，白名单+Set+Get 同步 | 已补（125 轮，回读与像素双向实测，t_style_box 10/10） |

| `<div bg="#123456">` / `<XButton bg=...>` | 与 `Background` 属性、`style="background:"` 等价：HTML 标签侧在 CommonAliases 统一改写为 BackColor，外观层再认 bg/bgcolor 别名 | 已补（126 轮，三路像素一致 (18,52,86)，t_style_box 10/10） |
| `Border="2px solid #ff0000"`（属性式简写） | 与 `BorderThickness/BorderStyle/BorderColor` 三段等价，简写按 token 拆 | 已补（126 轮，得 2/#ff0000/Single） |
| `Padding="8 16"`（属性式简写） | 与 `style="padding:8px 16px"` 等价：CSS 1/2/3/4 值口径，统一写成规范四值「左 上 右 下」 | 已补（127 轮，div/XPane 四写法子项矩形一致，t_style_box 10/10） |
| `div:hover{background:...}`（容器命中态） | 容器底色真实出图路径待定（插入桩点未被调用），暂不生效 | 未补（127 轮登记，实测试探后回退） |
- 128 轮：`div:hover{background}` 已补（宿主 OnMouseMove 命中 kind=6 容器节点 → containerHovered → CTLCOLOR/背板）。

- 130 轮审计结论（未改动）：`width:100%`/`width:50%` 在 div 内已按父内框换算（实测 200→200 / 100 像素），`XmlCore.Html.cpp:1505` 早有 % 分支、`XmlCore.Layout.cpp:710` 按父内框换算；缺口清单里"width:100% 被 wcstol 读成 100px"已过期，勿重复挖。

- 130 轮：`<button>` UA 默认外观（圆角6/底#efefef/框#767676/字#111）修正为真正生效（HtmlButton 标记），此前因标签改写永远不命中；`<label>` 同法加 HtmlLabel。

- 131 轮：`<input>` UA 默认补 Background=#ffffff、Padding=2 8（HtmlInput 标记同样生效），显式 style 不覆盖。

- 132 轮：圆角盒子的弧外像素改为用父底色填掉（原来漏窗口擦除刷的底色，四角是方角）；圆角分支开始支持 border-width > 1px（以前固定 1px 描边）。

- 133 轮：`<select>`/`<textarea>`（及 input 字色）UA 默认补齐：白底 + 1px #767676 + 4px 圆角 + padding + 字色 #111111。
| `<img style="border-radius:50%">`（圆形头像） | 同名 style → `Radius`，D2D 后端 `drawImageClippedRound` 逐像素行轴对齐裁剪拼圆弧（先铺盒底色擦掉旧方角像素）；PushLayer 几何遮罩在 DC 目标上不裁，已弃用 | 已补（138 轮） |
| `border-radius:50%` 非正方形（椭圆） | `ResolveRadiusXY` 按宽高分别换算 rx/ry，画布 `drawImageClippedRound(mask,rx,ry,dst,path)` 按椭圆公式逐行裁剪 | 已补（139 轮） |
