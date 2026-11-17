# -*- coding: utf-8 -*-
"""LoRA 训练进度可视化监控（纯标准库，Tk + Canvas，不需 torch/matplotlib）。

用法：
    pythonw train_progress.py [日志路径]
默认监控 G:\\PyArx\\tools\\train\\logs\\train15b_api_dml.log
"""
import math
import os
import re
import sys
import time
import tkinter as tk
import tkinter.font as tkfont

LOG = sys.argv[1] if len(sys.argv) > 1 else r"G:\PyArx\tools\train\logs\train15b_api_dml.log"

BG      = "#0e1217"
CARD    = "#161d26"
CARD_ED = "#232d3a"
FG      = "#e7edf4"
MUTED   = "#7b8da3"
ACCENT  = "#35d0ba"
ACCENT2 = "#2aa79a"
WARN    = "#ffa14a"
GOOD    = "#57d98a"
BAD     = "#ff6b6b"

RE_STEP  = re.compile(r"ep(\d+)\s+step(\d+)\s+loss=([0-9eE.+-]+)\s+([0-9]+)s")
RE_TRAIN = re.compile(r"train:\s+rows=(\d+)\s+epochs=(\d+)")
RE_MAXLEN = re.compile(r"maxlen=(\d+)")
RE_DONE  = re.compile(r"\[4/4\]|\bdone\b")


def fmt_sec(s):
    if s <= 0:
        return "-"
    if s < 60:
        return "%.0f 秒" % s
    if s < 3600:
        return "%d 分 %02d 秒" % (int(s // 60), int(s % 60))
    return "%.1f 小时" % (s / 3600.0)


class App(object):
    def __init__(self, root, log):
        self.root = root
        self.log = log
        self.rows = 0
        self.epochs = 0
        self.maxlen = 0
        self.steps = []          # [(ep, step_no, loss, elapsed_sec)]
        self.done = False
        self.offset = 0
        self.last_feed = 0.0     # 最近一条新数据的墙钟时间

        root.title("训练监控 · Qwen2.5-1.5B · PyArxCore LoRA")
        root.geometry("1000x620")
        root.minsize(760, 480)
        root.configure(bg=BG)

        self.cv = tk.Canvas(root, bg=BG, highlightthickness=0)
        self.cv.pack(fill="both", expand=True)
        self.cv.bind("<Configure>", lambda e: self.draw())

        fam_cn = "Microsoft YaHei UI"
        self.f_title = tkfont.Font(family=fam_cn, size=14, weight="bold")
        self.f_num   = tkfont.Font(family=fam_cn, size=23, weight="bold")
        self.f_num2  = tkfont.Font(family=fam_cn, size=17, weight="bold")
        self.f_lbl   = tkfont.Font(family=fam_cn, size=9)
        self.f_note  = tkfont.Font(family="Consolas", size=9)
        self.f_small = tkfont.Font(family=fam_cn, size=9, weight="bold")

        self.read_all()
        self.draw()
        self.tick()

    # ---------------------------------------------------------------- 读日志
    def feed(self, line):
        line = line.strip()
        if not line:
            return
        m = RE_TRAIN.search(line)
        if m:
            self.rows = int(m.group(1))
            self.epochs = int(m.group(2))
            return
        m = RE_MAXLEN.search(line)
        if m and not self.maxlen:
            self.maxlen = int(m.group(1))
            return
        m = RE_STEP.search(line)
        if m:
            self.steps.append((int(m.group(1)), int(m.group(2)),
                               float(m.group(3)), int(m.group(4))))
            self.last_feed = time.time()
            return
        if RE_DONE.search(line):
            self.done = True

    def read_all(self):
        if not os.path.exists(self.log):
            return
        try:
            with open(self.log, "r", encoding="utf-8", errors="ignore") as f:
                for line in f:
                    self.feed(line)
            self.offset = os.path.getsize(self.log)
        except Exception:
            pass

    def poll(self):
        try:
            if not os.path.exists(self.log):
                return
            size = os.path.getsize(self.log)
            if size < self.offset:          # 文件被截断/重建
                self.offset = 0
            if size == self.offset:
                return
            with open(self.log, "r", encoding="utf-8", errors="ignore") as f:
                f.seek(self.offset)
                chunk = f.read()
                self.offset = f.tell()
            for line in chunk.splitlines():
                self.feed(line)
        except Exception:
            pass

    def tick(self):
        self.poll()
        self.draw()
        self.root.after(1000, self.tick)

    # ---------------------------------------------------------------- 指标
    def metrics(self):
        if not self.steps:
            return None
        total = (self.rows * self.epochs) if (self.rows and self.epochs) else 0
        cur = self.steps[-1][1]
        pct = (cur / float(total)) if total else 0.0
        # 最近 20 步的平均速度
        tail = self.steps[-20:]
        if len(tail) >= 2:
            d_step = tail[-1][1] - tail[0][1]
            d_sec = tail[-1][3] - tail[0][3]
            speed = (d_sec / float(d_step)) if d_step > 0 else 0.0
        else:
            speed = self.steps[-1][3] / float(max(1, self.steps[-1][1]))
        eta = (total - cur) * speed if (total and speed) else 0.0
        loss_now = self.steps[-1][2]
        loss_avg = sum(s[2] for s in tail) / float(len(tail))
        loss_best = min(s[2] for s in self.steps)
        ema = self.steps[0][2]
        for s in self.steps:
            ema = ema * 0.92 + s[2] * 0.08
        return {
            "total": total, "cur": cur, "pct": pct, "speed": speed,
            "eta": eta, "loss_now": loss_now, "loss_avg": loss_avg,
            "loss_best": loss_best, "ema": ema,
            "elapsed": self.steps[-1][3], "ep": self.steps[-1][0],
            "n": len(self.steps),
        }

    def status(self):
        if self.done:
            return GOOD, "已完成"
        age = time.time() - self.last_feed if self.last_feed else 1e9
        if not self.steps:
            return WARN, "等待训练数据"
        if age < 8:
            return ACCENT, "训练中"
        return WARN, "无新数据 %s" % fmt_sec(age)

    # ---------------------------------------------------------------- 绘制
    def draw(self):
        cv = self.cv
        cv.delete("all")
        W = cv.winfo_width()
        H = cv.winfo_height()
        if W < 200 or H < 200:
            return
        P = 20
        m = self.metrics()

        # 顶部
        cv.create_text(P, 22, text="LoRA 训练监控", anchor="w", fill=FG, font=self.f_title)
        cv.create_text(P + 150, 24, text="Qwen2.5-1.5B  ·  PyArxCore", anchor="w",
                       fill=MUTED, font=self.f_small)
        col, txt = self.status()
        cv.create_oval(W - P - 118, 16, W - P - 106, 28, fill=col, outline=col)
        cv.create_text(W - P - 98, 22, text=txt, anchor="w", fill=col, font=self.f_small)

        # 指标卡
        cy0, cy1 = 46, 150
        gap = 12
        cw = (W - 2 * P - 3 * gap) / 4.0
        titles = ["训练进度", "当前 loss", "速度", "预计剩余"]
        if m:
            sub = [
                "第 %d/%d 轮 · %d/%d 步" % (m["ep"], self.epochs or 0, m["cur"], m["total"]),
                "均 %0.3f · 最低 %0.3f" % (m["loss_avg"], m["loss_best"]),
                "%0.2f 秒/步" % m["speed"],
                fmt_sec(m["eta"]),
            ]
            main = [
                "%d%%" % int(round(m["pct"] * 100)),
                "%0.3f" % m["loss_now"],
                "%0.1f" % m["speed"],
                fmt_sec(m["eta"]),
            ]
        else:
            sub = ["尚无数据"] * 4
            main = ["-"] * 4

        for i in range(4):
            x0 = P + i * (cw + gap)
            x1 = x0 + cw
            cv.create_rectangle(x0, cy0, x1, cy1, fill=CARD, outline=CARD_ED, width=1)
            cv.create_text(x0 + 12, cy0 + 14, text=titles[i], anchor="w",
                           fill=MUTED, font=self.f_lbl)
            cv.create_text(x0 + 12, cy0 + 46, text=main[i], anchor="w",
                           fill=FG, font=self.f_num)
            cv.create_text(x0 + 12, cy1 - 16, text=sub[i], anchor="w",
                           fill=MUTED, font=self.f_note)

        # 进度条
        by0, by1 = cy1 + 18, cy1 + 26
        cv.create_rectangle(P, by0, W - P, by1, fill="#1b2431", outline="")
        if m and m["pct"] > 0:
            w = (W - 2 * P) * min(1.0, m["pct"])
            cv.create_rectangle(P, by0, P + max(2, w), by1, fill=ACCENT2, outline="")

        # 曲线区
        ch0 = by1 + 22
        ch1 = H - 46
        if ch1 - ch0 < 80:
            ch0 = by1 + 12
        cv.create_text(P, ch0 - 8, text="loss 曲线", anchor="sw", fill=MUTED, font=self.f_lbl)
        if m:
            info = "rows=%d  epochs=%d  maxlen=%s  已用 %s" % (
                self.rows, self.epochs, self.maxlen or "-", fmt_sec(m["elapsed"]))
        else:
            info = "rows=%d  epochs=%d  maxlen=%s" % (self.rows, self.epochs, self.maxlen or "-")
        cv.create_text(W - P, ch0 - 8, text=info, anchor="se", fill=MUTED, font=self.f_note)

        cv.create_rectangle(P, ch0, W - P, ch1, fill="#10161d", outline=CARD_ED, width=1)

        if not self.steps:
            cv.create_text((P + W - P) / 2.0, (ch0 + ch1) / 2.0,
                           text="等待训练数据…（每 1 秒刷新）", fill=MUTED, font=self.f_lbl)
        else:
            self.draw_chart(P + 8, ch0 + 8, W - P - 8, ch1 - 8, m)

        # 底部
        cv.create_text(P, H - 22, text=self.log, anchor="w", fill="#5b6a7d", font=self.f_note)
        cv.create_text(W - P, H - 22, text=time.strftime("%H:%M:%S"), anchor="e",
                       fill="#5b6a7d", font=self.f_note)

    def draw_chart(self, x0, y0, x1, y1, m):
        cv = self.cv
        losses = [s[2] for s in self.steps]
        lo = min(losses)
        hi = max(losses)
        pad = (hi - lo) * 0.15 or max(0.5, abs(hi) * 0.05)
        lo -= pad
        hi += pad
        span = max(1e-6, hi - lo)
        n_ref = float(m["total"] or len(losses))
        w = max(1.0, x1 - x0)
        h = max(1.0, y1 - y0)

        # 网格 + y 轴刻度
        for i in range(5):
            gy = y0 + h * i / 4.0
            cv.create_line(x0, gy, x1, gy, fill="#1a222c")
            v = hi - span * i / 4.0
            cv.create_text(x0 - 2, gy, text="%0.2f" % v, anchor="e", fill="#4d5b6d", font=self.f_note)

        # 理论初始值参考（log(151936)）
        ref = math.log(151936.0)
        if ref > lo and ref < hi:
            ry = y1 - (ref - lo) / span * h
            cv.create_line(x0, ry, x1, ry, fill="#3a2c1e", dash=(6, 6))
            cv.create_text(x0 + 6, ry - 8, text="ln(151936)≈11.9", anchor="w",
                           fill="#8a6a44", font=self.f_note)

        # loss 折线
        pts = []
        for i, s in enumerate(self.steps):
            px = x0 + (s[1] / n_ref) * w
            py = y1 - (s[2] - lo) / span * h
            pts.extend([px, py])
        if len(pts) >= 4:
            cv.create_line(*pts, fill=WARN, width=1)

        # EMA 平滑
        ema_pts = []
        e = self.steps[0][2]
        for s in self.steps:
            e = e * 0.92 + s[2] * 0.08
            px = x0 + (s[1] / n_ref) * w
            py = y1 - (e - lo) / span * h
            ema_pts.extend([px, py])
        if len(ema_pts) >= 4:
            cv.create_line(*ema_pts, fill=ACCENT, width=2)

        # 当前位置
        cx = x0 + (m["cur"] / n_ref) * w
        cv.create_line(cx, y0, cx, y1, fill="#2f4a5a")
        if self.epochs and self.rows:
            for k in range(1, self.epochs):
                ex = x0 + (k * self.rows / n_ref) * w
                cv.create_line(ex, y0, ex, y1, fill="#243444", dash=(4, 4))


def main():
    root = tk.Tk()
    App(root, LOG)
    root.mainloop()


if __name__ == "__main__":
    main()
