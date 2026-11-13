# -*- coding: utf-8 -*-
"""demo_studio.py — 新版 UIDesign 示例：PyArx 车间监控台（纯 Python 预览）。

这一版是"改造后控件"的整合设计稿，重点展示三样新能力：
  1) DrawCanvas 图元选中 / 框选 / Delete 删除（设备布置图页，底板用 `Rect!` 锁定不参与点选）；
  2) 页容器 + 每页独立动画开关（离开那页就停，回来还开着就续跑）；
  3) 全套自绘皮肤（按钮 / 开关 / 进度 / 滑块 / 表盘 / 步进框 / 表格 / 列表）。

导航：左侧四个按钮切页；右上"演示动画"总开关，页内还有各自的开关。
"""
import os
import tempfile
import time

import PyMfcXml as p

# ---- 设备布置图：底板与通道用 `!` 锁定，只有设备块和文字可选中 ----
PLAN = (
    'Clear,#1C2027;'
    'Rect!,0,0,548,296,#1C2027;'
    'RoundRect!,12,12,520,120,10,#232A33;'
    'RoundRect!,12,160,520,120,10,#232A33;'
    'Line!,270,12,270,280,#39414D,2;'
    'Text!,26,20,车间 A · 冲压区,12,#8A94A3;'
    'Text!,26,168,车间 B · 装配区,12,#8A94A3;'
    'RoundRect,40,56,96,44,6,#3574C8;'
    'Text,44,70,冲压机 01,12,#E9EEF5;'
    'RoundRect,156,56,96,44,6,#3574C8;'
    'Text,160,70,冲压机 02,12,#E9EEF5;'
    'RoundRect,40,100,96,40,6,#2E5F9E;'
    'Text,44,112,送料台 01,12,#E9EEF5;'
    'RoundRect,156,100,96,40,6,#2E5F9E;'
    'Text,160,112,送料台 02,12,#E9EEF5;'
    'RoundRect,40,198,96,44,6,#2FA36B;'
    'Text,44,212,装配台 01,12,#E9EEF5;'
    'RoundRect,156,198,96,44,6,#2FA36B;'
    'Text,160,212,装配台 02,12,#E9EEF5;'
    'RoundRect,320,198,96,44,6,#C08A3C;'
    'Text,324,212,检测台 01,12,#E9EEF5;'
    'RoundRect,320,56,96,44,6,#8A5FC8;'
    'Text,324,70,缓存仓 01,12,#E9EEF5'
)

XML = '''<?xml version="1.0" encoding="utf-8"?>
<XWindow xmlns:x="http://schemas.microsoft.com/winfx/2006/xml"
        Title="PyArx 车间监控台 · UIDesign" Width="1200" Height="772" Background="#15181D">
  <XLayout x:Name="root">

    <!-- ===== 顶部标题区 ===== -->
    <XLabel x:Name="appTitle" Text="PyArx 车间监控台" Width="460" Height="26"
               Canvas.Left="20" Canvas.Top="34" FontSize="18" Foreground="#E9EEF5" />
    <XLabel x:Name="appSub" Text="P1 车间 · 3 条产线 · 每 5 秒采样" Width="560" Height="18"
               Canvas.Left="20" Canvas.Top="64" FontSize="12" Foreground="#8A94A3" />
    <XLabel x:Name="swDemoLabel" Text="演示动画" Width="110" Height="20"
               Canvas.Left="668" Canvas.Top="44" FontSize="12" Foreground="#9AA4B2" TextAlign="Right" />
    <XSwitch x:Name="swDemo" Width="96" Height="32" Value="0" Canvas.Left="786" Canvas.Top="40" />
    <XButton x:Name="btnRefresh" Text="刷新" Width="80" Height="32" Canvas.Left="900" Canvas.Top="40" />
    <XButton x:Name="btnExport" Text="导出" Width="80" Height="32" Canvas.Left="990" Canvas.Top="40" />

    <XQCanvas x:Name="rule" Width="1140" Height="3" Canvas.Left="20" Canvas.Top="94"
             Commands="Line,0,1,1140,1,#2A3038,1" />

    <!-- ===== 左栏：导航 ===== -->
    <XLabel x:Name="navTitle" Text="视图" Width="160" Height="18"
               Canvas.Left="20" Canvas.Top="110" FontSize="12" Foreground="#8A94A3" />
    <XButton x:Name="nav0" Text="总览" Width="190" Height="36" Canvas.Left="20" Canvas.Top="134" />
    <XButton x:Name="nav1" Text="设备布置" Width="190" Height="36" Canvas.Left="20" Canvas.Top="174" />
    <XButton x:Name="nav2" Text="维护记录" Width="190" Height="36" Canvas.Left="20" Canvas.Top="214" />
    <XButton x:Name="nav3" Text="趋势与动画" Width="190" Height="36" Canvas.Left="20" Canvas.Top="254" />

    <XLabel x:Name="navTip" Width="190" Height="72"
               Text="布置图页：点图元选中，空白处拖拽框选，Delete 删除"
               Canvas.Left="20" Canvas.Top="302" FontSize="11" Foreground="#76808F" />

    <XLabel x:Name="outTitle" Text="今日产量" Width="190" Height="18"
               Canvas.Left="20" Canvas.Top="392" FontSize="12" Foreground="#8A94A3" />
    <XProgressBar x:Name="outBar" Width="180" Height="12" Value="64" Minimum="0" Maximum="100"
              Canvas.Left="20" Canvas.Top="414" />
    <XLabel x:Name="outText" Text="1 280 / 2 000 件" Width="190" Height="18"
               Canvas.Left="20" Canvas.Top="434" FontSize="11" Foreground="#9AA4B2" />

    <!-- ===== 中间：页容器 ===== -->
    <XStackView x:Name="sv" Width="592" Height="486" Canvas.Left="228" Canvas.Top="112" />

    <!-- 第 1 页 · 总览 -->
    <XLabel x:Name="pg1" Text="产线运行状态" Width="300" Height="20"
               Canvas.Left="244" Canvas.Top="136" FontSize="14" Foreground="#E9EEF5" />
    <XLabel x:Name="lA" Text="产线 A · 冲压        78%" Width="300" Height="18"
               Canvas.Left="244" Canvas.Top="170" FontSize="12" Foreground="#B8C0CC" />
    <XProgressBar x:Name="p1a" Width="540" Height="14" Value="78" Minimum="0" Maximum="100"
              Canvas.Left="244" Canvas.Top="192" />
    <XLabel x:Name="lB" Text="产线 B · 装配        62%" Width="300" Height="18"
               Canvas.Left="244" Canvas.Top="220" FontSize="12" Foreground="#B8C0CC" />
    <XProgressBar x:Name="p1b" Width="540" Height="14" Value="62" Minimum="0" Maximum="100"
              Canvas.Left="244" Canvas.Top="242" />
    <XLabel x:Name="lC" Text="产线 C · 检测        45%" Width="300" Height="18"
               Canvas.Left="244" Canvas.Top="270" FontSize="12" Foreground="#B8C0CC" />
    <XProgressBar x:Name="p1c" Width="540" Height="14" Value="45" Minimum="0" Maximum="100"
              Canvas.Left="244" Canvas.Top="292" />

    <XLabel x:Name="slLabel" Text="目标节拍 60 件/时" Width="300" Height="18"
               Canvas.Left="244" Canvas.Top="332" FontSize="12" Foreground="#8A94A3" />
    <XSlider x:Name="sl" Width="420" Height="30" Value="60" Minimum="0" Maximum="100"
            Canvas.Left="244" Canvas.Top="354" />

    <XLabel x:Name="sw1Label" Text="本页动画" Width="120" Height="20"
               Canvas.Left="244" Canvas.Top="404" FontSize="12" Foreground="#9AA4B2" />
    <XSwitch x:Name="sw1" Width="96" Height="32" Value="0" Canvas.Left="244" Canvas.Top="428" />
    <XLabel x:Name="p1note" Text="开启后本页进度条持续涨落" Width="360" Height="18"
               Canvas.Left="368" Canvas.Top="436" FontSize="11" Foreground="#76808F" />

    <!-- 第 2 页 · 设备布置（可编辑画布） -->
    <XLabel x:Name="pg2" Text="设备布置图 · 可选中 / 可删除" Width="420" Height="20"
               Canvas.Left="244" Canvas.Top="136" FontSize="14" Foreground="#E9EEF5" />
    <XQCanvas x:Name="cv" Width="548" Height="296" Canvas.Left="244" Canvas.Top="162" />
    <XLabel x:Name="cvhint" Width="548" Height="18"
               Text="点设备块选中，空白处拖拽框选；Delete 删除，可反复点“恢复布置”重来"
               Canvas.Left="244" Canvas.Top="470" FontSize="11" Foreground="#76808F" />
    <XLabel x:Name="cvstat" Width="548" Height="18" Text=""
               Canvas.Left="244" Canvas.Top="492" FontSize="11" Foreground="#7FD3A8" />

    <!-- 第 3 页 · 维护记录 -->
    <XLabel x:Name="pg3" Text="维护记录" Width="300" Height="20"
               Canvas.Left="244" Canvas.Top="136" FontSize="14" Foreground="#E9EEF5" />
    <XTableView x:Name="tv" Width="548" Height="300" Canvas.Left="244" Canvas.Top="162" />
    <XLabel x:Name="p3note" Width="548" Height="18"
               Text="表头随皮肤走；点行可选中，双击进入明细"
               Canvas.Left="244" Canvas.Top="470" FontSize="11" Foreground="#76808F" />

    <!-- 第 4 页 · 趋势与动画 -->
    <XLabel x:Name="pg4" Text="趋势与动画 · 每页独立开关" Width="460" Height="20"
               Canvas.Left="244" Canvas.Top="136" FontSize="14" Foreground="#E9EEF5" />
    <XLabel x:Name="sw4Label" Text="本页动画" Width="120" Height="20"
               Canvas.Left="244" Canvas.Top="166" FontSize="12" Foreground="#9AA4B2" />
    <XSwitch x:Name="sw4" Width="96" Height="32" Value="0" Canvas.Left="244" Canvas.Top="190" />
    <XButton x:Name="ab" Text="宽度动画" Width="160" Height="40" Canvas.Left="244" Canvas.Top="236" />
    <XDial x:Name="ad" Width="120" Height="120" Value="0" Minimum="0" Maximum="100"
          Canvas.Left="244" Canvas.Top="292" />
    <XProgressBar x:Name="ap" Width="300" Height="16" Value="0" Minimum="0" Maximum="100"
              Canvas.Left="384" Canvas.Top="310" />
    <XLabel x:Name="p4note" Width="520" Height="18"
               Text="切到别的页动画立刻停，回来且开关还开着就自动续跑"
               Canvas.Left="244" Canvas.Top="428" FontSize="11" Foreground="#76808F" />

    <!-- ===== 右栏：实时指标 ===== -->
    <XLabel x:Name="rtTitle" Text="实时指标" Width="200" Height="18"
               Canvas.Left="836" Canvas.Top="110" FontSize="12" Foreground="#8A94A3" />
    <XDial x:Name="rtDial" Width="124" Height="124" Value="78" Minimum="0" Maximum="100"
          Canvas.Left="836" Canvas.Top="134" />
    <XLabel x:Name="lRa" Text="产线 A 利用率" Width="190" Height="16"
               Canvas.Left="972" Canvas.Top="140" FontSize="11" Foreground="#9AA4B2" />
    <XProgressBar x:Name="ra" Width="180" Height="12" Value="78" Minimum="0" Maximum="100"
              Canvas.Left="972" Canvas.Top="160" />
    <XLabel x:Name="lRb" Text="产线 B 利用率" Width="190" Height="16"
               Canvas.Left="972" Canvas.Top="188" FontSize="11" Foreground="#9AA4B2" />
    <XProgressBar x:Name="rb" Width="180" Height="12" Value="62" Minimum="0" Maximum="100"
              Canvas.Left="972" Canvas.Top="208" />

    <XLabel x:Name="spinLabel" Text="目标节拍（件/时）" Width="220" Height="16"
               Canvas.Left="836" Canvas.Top="272" FontSize="11" Foreground="#9AA4B2" />
    <XSpinBox x:Name="spin" Width="200" Height="30" Value="60" Minimum="0" Maximum="120"
             Canvas.Left="836" Canvas.Top="292" />

    <XLabel x:Name="logLabel" Text="事件日志" Width="200" Height="16"
               Canvas.Left="836" Canvas.Top="336" FontSize="11" Foreground="#9AA4B2" />
    <XListCtrl x:Name="logs" Width="320" Height="222" Canvas.Left="836" Canvas.Top="356" />

    <!-- ===== 状态栏 ===== -->
    <XLabel x:Name="status" Text="就绪" Width="700" Height="20"
               Canvas.Left="20" Canvas.Top="700" FontSize="11" Foreground="#76808F" />
    <XLabel x:Name="statusRight" Text="DrawCanvas 图元编辑已启用" Width="320" Height="20"
               Canvas.Left="836" Canvas.Top="700" FontSize="11" Foreground="#76808F" TextAlign="Right" />

  </XLayout>
</XWindow>'''

PATH = os.path.join(tempfile.mkdtemp(prefix='pymfc_studio_'), 'studio.xml')
SEG = 700
CYC = 1400


def build():
    with open(PATH, 'w', encoding='utf-8') as f:
        f.write(XML)
    return PATH


class Studio:
    def __init__(self, w, names):
        self.w = w
        self.n = names
        self.page = 0
        self.want = [False, False, False, False]      # 各页开关的意愿
        self.running = [False, False, False, False]
        self.t0 = time.time()
        self.logs = []

    # ---- 日志 ----
    def log(self, text):
        self.logs.insert(0, text)
        del self.logs[8:]
        w = self.w
        w.clearListItems(self.n['logs'])
        for line in self.logs:
            parts = line.split('|', 1)
            w.addListItem(self.n['logs'], (parts[0] + '|' + parts[1]) if len(parts) > 1 else (line + '|'))

    def status(self, text):
        self.w.setText(self.n['status'], text)

    # ---- 页切换 ----
    def goto(self, i):
        w = self.w
        w.setViewIndex(self.n['sv'], i)
        w.animatePage(self.n['sv'], i)
        self.page = i
        self.on_page_changed()

    def on_page_changed(self):
        for i in range(4):
            if i != self.page:
                self.stop(i)
        if self.want[self.page]:
            self.start(self.page)
        self.status('已切到第 %d 页 · %s' % (self.page + 1, ['总览', '设备布置', '维护记录', '趋势与动画'][self.page]))

    # ---- 动画 ----
    def schedule(self, i):
        w, n = self.w, self.n
        if i == 0:
            w.animateFrom(n['p1a'], 'Value', 30, 92, SEG, 'outCubic', 0, False)
            w.animateFrom(n['p1a'], 'Value', 92, 30, SEG, 'outCubic', SEG, False)
            w.animateFrom(n['p1b'], 'Value', 44, 70, SEG, 'outCubic', 0, False)
            w.animateFrom(n['p1b'], 'Value', 70, 44, SEG, 'outCubic', SEG, False)
            w.animateFrom(n['p1c'], 'Value', 20, 66, SEG, 'inOutQuad', 0, False)
            w.animateFrom(n['p1c'], 'Value', 66, 20, SEG, 'inOutQuad', SEG, False)
        elif i == 3:
            w.animateFrom(n['ab'], 'Width', 160, 300, SEG, 'linear', 0, False)
            w.animateFrom(n['ab'], 'Width', 300, 160, SEG, 'linear', SEG, False)
            w.animateFrom(n['ad'], 'Value', 0, 100, SEG, 'outCubic', 0, False)
            w.animateFrom(n['ad'], 'Value', 100, 0, SEG, 'outCubic', SEG, False)
            w.animateFrom(n['ap'], 'Value', 0, 100, SEG, 'inOutQuad', 0, False)
            w.animateFrom(n['ap'], 'Value', 100, 0, SEG, 'inOutQuad', SEG, False)

    def start(self, i):
        if self.running[i] or i not in (0, 3):
            return
        self.running[i] = True
        self.schedule(i)
        if i == 0:
            self.w.setText(self.n['p1note'], '本页动画运行中')
        else:
            self.w.setText(self.n['p4note'], '本页动画运行中 · 切页即停')

    def stop(self, i):
        if not self.running[i]:
            return
        self.running[i] = False
        w, n = self.w, self.n
        if i == 0:
            for k in ('p1a', 'p1b', 'p1c'):
                w.stopAnimations(n[k])
            w.setText(n['p1note'], '本页动画已停止')
        else:
            for k in ('ab', 'ad', 'ap'):
                w.stopAnimations(n[k])
            w.setText(n['p4note'], '本页动画已停止 · 切页即停')

    def tick(self):
        w, n = self.w, self.n
        # 每页自己的开关（第 1 页与第 4 页）
        for i, sw in ((0, 'sw1'), (3, 'sw4')):
            on = bool(w.getSwitch(n[sw]))
            if on != self.want[i]:
                self.want[i] = on
                if not on:
                    self.stop(i)
                elif i == self.page:
                    self.start(i)
                self.log('第 %d 页动画开关 → %s' % (i + 1, '开' if on else '关'))
        # 总开关：关的时候把所有页的意愿清掉
        master = bool(w.getSwitch(n['swDemo']))
        if master != getattr(self, 'master', None):
            self.master = master
            if not master:
                for i in range(4):
                    self.stop(i)
                self.log('总开关 → 关，全部动画停')
            else:
                self.log('总开关 → 开')
        if time.time() - self.t0 > (CYC - 200) / 1000.0:
            for i in (0, 3):
                if self.running[i]:
                    self.schedule(i)
            self.t0 = time.time()


def wire(w, names, st):
    # 导航按钮：零参回调，所以要闭包绑死页码
    for i in range(4):
        w.onClick(names['nav%d' % i], (lambda k: (lambda: st.goto(k)))(i))

    # 画布：可编辑 + 图元状态 + 选择回调
    cv = names['cv']
    w.setCanvasCommands(cv, PLAN)
    w.setCanvasEditable(cv, True)

    def refresh_cv(*a):
        sel = w.getCanvasSelection(cv)
        n = w.getCanvasItemCount(cv)
        cap = sum(1 for i in range(n) if w.getCanvasItemOp(cv, i) not in ('Clear',))
        if sel:
            ops = [w.getCanvasItemOp(cv, i) for i in sel]
            w.setText(names['cvstat'], '共 %d 个图元 · 选中 %d 个（%s）'
                     % (n, len(sel), '、'.join(sorted(set(ops)))))
            st.log('布置图选中 %d 个图元' % len(sel))
        else:
            w.setText(names['cvstat'], '共 %d 个图元 · 未选中' % n)
        (void)(cap)

    w.onCanvasSelect(cv, refresh_cv) if False else w.onCanvasSelect(cv, refresh_cv)
    refresh_cv()

    # 刷新 / 导出 两个动作按钮
    w.onClick(names['btnRefresh'], lambda: st.log('手动刷新一次采样') or st.status('已刷新采样数据'))
    w.onClick(names['btnExport'], lambda: st.log('导出当前看板为 PNG') or st.status('已导出看板快照'))

    # 滑块 / 步进框联动节拍
    def on_tick_value(*a):
        v = w.getPos(names['sl'])
        w.setText(names['slLabel'], '目标节拍 %d 件/时' % v)

    w.onValue(names['sl'], on_tick_value)


def main():
    names = {}
    w = p.guiload(build(), names=names)
    w.show()
    p.pump(300)

    sv = names['sv']
    w.addPage(sv, [names['pg1'], names['lA'], names['p1a'], names['lB'], names['p1b'],
                   names['lC'], names['p1c'], names['slLabel'], names['sl'],
                   names['sw1Label'], names['sw1'], names['p1note']])
    w.addPage(sv, [names['pg2'], names['cv'], names['cvhint'], names['cvstat']])
    w.addPage(sv, [names['pg3'], names['tv'], names['p3note']])
    w.addPage(sv, [names['pg4'], names['sw4Label'], names['sw4'], names['ab'], names['ad'],
                   names['ap'], names['p4note']])
    w.setViewIndex(sv, 0)

    tv = names['tv']
    w.setTableColumns(tv, '设备\t类型\t上次维护\t状态')
    for row in ('冲压机 01\t液压\t08-12\t正常',
                '冲压机 02\t液压\t08-19\t待检',
                '送料台 01\t输送\t07-30\t正常',
                '装配台 01\t装配\t08-02\t正常',
                '检测台 01\t视觉\t08-21\t告警',
                '缓存仓 01\t仓储\t07-11\t正常'):
        w.addTableRow(tv, row)

    st = Studio(w, names)
    wire(w, names, st)
    st.log('看板已启动')
    for i, name in enumerate(('总览就绪', '设备布置图 22 个图元', '维护记录 6 条', '趋势页待开启')):
        st.log(name)
    st.status('就绪 · 左侧按钮切页')

    while True:
        p.pump(40)
        st.tick()


if __name__ == '__main__':
    main()
