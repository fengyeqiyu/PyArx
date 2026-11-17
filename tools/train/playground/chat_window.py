# -*- coding: utf-8 -*-
"""PyArx 本地模型对话窗（tkinter 桌面版）。

复用 playground 的 HTTP 服务（127.0.0.1:8765），不重复加载模型、不额外占 2GB 内存。
启动前服务必须已在跑；窗口检测不到会给出提示。

用法：pythonw chat_window.py
"""

import json
import queue
import threading
import tkinter as tk
import urllib.request

BASE = "http://127.0.0.1:8765"

BG = "#0d1016"
PANEL = "#141821"
BAR = "#0a0d13"
INK = "#dfe6f2"
DIM = "#8492a8"
ACC = "#4d8cff"
OK = "#3ddc97"
BAD = "#ff6b6b"
LINE = "#232a38"

CHIPS = [
    "在原点画一条到 100,0 的线",
    "画一个圆心 50,50 半径 30 的圆",
    "统计一下当前图里有多少实体",
    "查一下 LIVE_TEST 层上有什么",
    "把句柄 698 的线删掉",
    "cad_save 保存有什么坑？",
    "为什么不让模型自己写脚本？",
    "帮我设计一个配电系统图",
]


class ChatWindow:
    def __init__(self, root):
        self.root = root
        self.q = queue.Queue()
        self.busy = False

        root.title("PyArx · 本地模型对话测试")
        root.geometry("980x740")
        root.configure(bg=BG)
        root.minsize(720, 520)

        self._build_top()
        self._build_chat()
        self._build_input()
        self._build_chips()

        self.append_sys("窗口就绪。输入指令或点下方快捷句，模型输出会连同 tool_call 一起显示。")
        self.root.after(80, self._pump)
        self._poll_status()
        root.bind("<Control-Return>", self.send)
        root.bind("<Return>", self.send)

    # ---------------------------------------------------------------- 布局
    def _build_top(self):
        bar = tk.Frame(self.root, bg=BAR, height=52)
        bar.pack(side="top", fill="x")
        bar.pack_propagate(False)

        tk.Label(bar, text="PyArx", bg=BAR, fg=INK,
                 font=("Microsoft YaHei", 13, "bold")).pack(side="left", padx=(16, 6))
        tk.Label(bar, text="本地模型对话测试", bg=BAR, fg=DIM,
                 font=("Microsoft YaHei", 9)).pack(side="left")

        self.badge = tk.Label(bar, text="base", bg=BAR, fg=DIM,
                              font=("Consolas", 9), padx=8, pady=2,
                              highlightbackground=LINE, highlightthickness=1)
        self.badge.pack(side="left", padx=14)

        self.dot = tk.Label(bar, text="●", bg=BAR, fg=ACC, font=("Segoe UI", 10))
        self.dot.pack(side="left")

        self.btn_reload = tk.Button(bar, text="重载 adapter", command=self.reload,
                                    bg=PANEL, fg=INK, relief="flat",
                                    activebackground=ACC, activeforeground=BAR,
                                    font=("Microsoft YaHei", 9), padx=10, cursor="hand2")
        self.btn_reload.pack(side="right", padx=14, pady=12)

        self.prog = tk.Label(bar, text="", bg=BAR, fg=DIM, font=("Consolas", 9))
        self.prog.pack(side="right", padx=10)

    def _build_chat(self):
        wrap = tk.Frame(self.root, bg=BG)
        wrap.pack(side="top", fill="both", expand=True, padx=14, pady=(12, 0))

        self.txt = tk.Text(wrap, bg=PANEL, fg=INK, insertbackground=INK,
                           relief="flat", wrap="word", padx=16, pady=14,
                           font=("Microsoft YaHei", 10), spacing1=2, spacing3=2,
                           highlightthickness=0)
        sb = tk.Scrollbar(wrap, command=self.txt.yview, bg=PANEL,
                          troughcolor=BG, relief="flat", width=10)
        self.txt.configure(yscrollcommand=sb.set)
        sb.pack(side="right", fill="y")
        self.txt.pack(side="left", fill="both", expand=True)

        self.txt.tag_config("who", foreground=DIM, font=("Microsoft YaHei", 8),
                            spacing1=8)
        self.txt.tag_config("who_me", foreground=DIM, justify="right",
                            font=("Microsoft YaHei", 8), spacing1=10)
        self.txt.tag_config("me", foreground="#9dc0ff", justify="right",
                            spacing1=2, spacing3=6)
        self.txt.tag_config("ai", foreground=INK, spacing1=2, spacing3=6)
        self.txt.tag_config("sys", foreground=DIM, font=("Microsoft YaHei", 9),
                            justify="center", spacing1=10, spacing3=10)
        self.txt.tag_config("tc", foreground="#c9f0d8", background="#0e1727",
                            font=("Consolas", 9), lmargin1=26, lmargin2=34,
                            rmargin=26, spacing1=6, spacing3=6)
        self.txt.tag_config("pass", foreground=OK, font=("Microsoft YaHei", 9),
                            lmargin1=26, lmargin2=26, spacing3=8)
        self.txt.tag_config("fail", foreground=BAD, font=("Microsoft YaHei", 9),
                            lmargin1=26, lmargin2=26, spacing3=8)
        self.txt.tag_config("meta", foreground=DIM, font=("Consolas", 9),
                            lmargin1=26, lmargin2=26)
        self.txt.config(state="disabled")

    def _build_input(self):
        bar = tk.Frame(self.root, bg=BAR, height=64)
        bar.pack(side="bottom", fill="x")
        bar.pack_propagate(False)

        self.entry = tk.Entry(bar, bg=PANEL, fg=INK, insertbackground=INK,
                              relief="flat", font=("Microsoft YaHei", 10),
                              highlightthickness=1, highlightbackground=LINE,
                              highlightcolor=ACC)
        self.entry.pack(side="left", fill="x", expand=True, padx=(16, 10), pady=14, ipady=6)
        self.entry.focus_set()

        self.btn = tk.Button(bar, text="发送", command=self.send,
                             bg=ACC, fg="#08111f", relief="flat",
                             font=("Microsoft YaHei", 10, "bold"),
                             padx=22, pady=5, cursor="hand2",
                             activebackground="#7aa8ff", activeforeground="#08111f")
        self.btn.pack(side="right", padx=(0, 16), pady=14)

    def _build_chips(self):
        wrap = tk.Frame(self.root, bg=BAR)
        wrap.pack(side="bottom", fill="x", padx=12, pady=(0, 8))
        for i, t in enumerate(CHIPS):
            b = tk.Label(wrap, text=t, bg=BAR, fg=DIM, font=("Microsoft YaHei", 8),
                         cursor="hand2", padx=8, pady=2,
                         highlightbackground=LINE, highlightthickness=1)
            b.pack(side="left", padx=3)
            b.bind("<Button-1>", lambda e, s=t: self.quick(s))
            b.bind("<Enter>", lambda e, w=b: w.config(fg=ACC, highlightbackground=ACC))
            b.bind("<Leave>", lambda e, w=b: w.config(fg=DIM, highlightbackground=LINE))

    # ---------------------------------------------------------------- 输出
    def _ins(self, text, tag):
        self.txt.config(state="normal")
        self.txt.insert("end", text, tag)
        self.txt.config(state="disabled")
        self.txt.see("end")

    def append_sys(self, text):
        self._ins(text + "\n", "sys")

    def append_me(self, text):
        self._ins("你\n", "who_me")
        self._ins(text + "\n", "me")

    def append_ai(self, d):
        meta = "%s · %ss" % (d.get("model"), d.get("seconds"))
        self._ins("本地模型\n", "who")
        self._ins((d.get("raw") or "(空)") + "\n", "ai")
        tc = d.get("tool_call")
        if tc is not None:
            self._ins(json.dumps(tc, ensure_ascii=False, indent=2) + "\n", "tc")
            if d.get("valid"):
                self._ins("✓ 校验通过，可以执行\n", "pass")
            else:
                self._ins("✗ 已拦截：" + str(d.get("reason")) + "\n", "fail")
        else:
            self._ins("（未解析出 tool_call）\n", "meta")
        self._ins(meta + "\n", "meta")

    def append_err(self, text):
        self._ins("本地模型\n", "who")
        self._ins(text + "\n", "fail")

    # ---------------------------------------------------------------- 交互
    def quick(self, text):
        self.entry.delete(0, "end")
        self.entry.insert(0, text)
        self.send()

    def send(self, event=None):
        if self.busy:
            return "break"
        text = self.entry.get().strip()
        if not text:
            return "break"
        self.busy = True
        self.btn.config(state="disabled", text="思考中…")
        self.append_me(text)
        self.entry.delete(0, "end")
        threading.Thread(target=self._call, args=(text,), daemon=True).start()
        return "break"

    def _call(self, text):
        try:
            body = json.dumps({"text": text, "max_new": 160}).encode("utf-8")
            req = urllib.request.Request(BASE + "/api/chat", data=body,
                                         headers={"Content-Type": "application/json"})
            with urllib.request.urlopen(req, timeout=240) as r:
                self.q.put(("chat", json.load(r)))
        except Exception as exc:
            self.q.put(("err", "%s\n（服务在 %s，检查 playground 是否在跑）" % (exc, BASE)))

    def reload(self):
        try:
            urllib.request.urlopen(urllib.request.Request(BASE + "/api/reload",
                                                          data=b"{}",
                                                          headers={"Content-Type": "application/json"}),
                                   timeout=10)
            self.append_sys("已请求重载 adapter，加载约 16 秒。")
        except Exception as exc:
            self.append_sys("重载失败：%s" % exc)

    def _pump(self):
        try:
            while True:
                kind, payload = self.q.get_nowait()
                if kind == "chat":
                    if payload.get("ok"):
                        self.append_ai(payload)
                    else:
                        self.append_err(str(payload.get("err")))
                    self.busy = False
                    self.btn.config(state="normal", text="发送")
                elif kind == "err":
                    self.append_err(payload)
                    self.busy = False
                    self.btn.config(state="normal", text="发送")
                elif kind == "status":
                    self._apply_status(payload)
        except queue.Empty:
            pass
        self.root.after(80, self._pump)

    def _poll_status(self):
        def work():
            try:
                with urllib.request.urlopen(BASE + "/api/status", timeout=8) as r:
                    self.q.put(("status", json.load(r)))
            except Exception:
                self.q.put(("status", None))
        threading.Thread(target=work, daemon=True).start()
        self.root.after(5000, self._poll_status)

    def _apply_status(self, st):
        if st is None:
            self.dot.config(fg=BAD)
            self.badge.config(text="offline", fg=BAD)
            self.prog.config(text="服务未连接")
            return
        m = st.get("model") or {}
        if m.get("loading"):
            self.dot.config(fg=ACC)
            self.badge.config(text="loading", fg=ACC)
        elif m.get("ready"):
            self.dot.config(fg=OK)
            self.badge.config(text="base+lora" if m.get("lora") else "base",
                              fg=OK)
        else:
            self.dot.config(fg=BAD)
            self.badge.config(text="down", fg=BAD)

        total, cur = st.get("total_steps") or 0, st.get("cur_step") or 0
        phase = {"training": "训练中", "done": "已训完"}.get(st.get("phase"), st.get("phase") or "")
        loss = st.get("last_loss")
        txt = "%s ep%d %d/%d" % (phase, st.get("cur_ep") or 0, cur, total)
        if loss is not None:
            txt += " · loss %.4f" % loss
        if st.get("adapter"):
            txt += " · adapter✓"
        self.prog.config(text=txt)


def main():
    root = tk.Tk()
    ChatWindow(root)
    root.mainloop()


if __name__ == "__main__":
    main()
