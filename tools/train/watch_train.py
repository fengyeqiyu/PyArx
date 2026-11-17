# -*- coding: utf-8 -*-
"""LoRA 训练旁路监视器（纯标准库，不占 GPU、不读日志以外的资源）。

每 INTERVAL 秒往 status 文件追加一行快照：进程存活、epoch/step、loss、速度、预计剩余。
进程消失或日志停更超过 STALL 秒时写 DEAD/STALL 并退出，方便外层 cron 直接读结论。

用法：
    python watch_train.py [--log xxx.log] [--pid 1234] [--out xxx.txt] [--interval 60]
"""
import argparse
import json
import os
import re
import sys
import time

RE_STEP = re.compile(r"ep(\d+)\s+step(\d+)\s+loss=([0-9eE.+-]+)\s+([0-9]+)s")
RE_HEAD = re.compile(r"\[3/4\]\s+train:\s+rows=(\d+)\s+epochs=(\d+).*?resume=(\d+)")
RE_DONE = re.compile(r"^done\s|\[4/4\]")
RE_CKPT = re.compile(r">>\s+ckpt|ckpt\(epoch\)")


def snapshot(log_path, pid):
    try:
        size = os.path.getsize(log_path)
        mtime = os.path.getmtime(log_path)
    except OSError:
        return None
    rows = epochs = resume = 0
    step = 0
    loss = None
    elapsed = 0.0
    tag = ""
    done = False
    with open(log_path, "rb") as f:
        # 只读尾部 256KB，日志再长也不会变慢
        f.seek(max(0, size - 262144))
        data = f.read().decode("utf-8", "replace")
    for line in data.splitlines():
        m = RE_HEAD.search(line)
        if m:
            rows, epochs, resume = int(m.group(1)), int(m.group(2)), int(m.group(3))
        m = RE_STEP.search(line)
        if m:
            step = int(m.group(2))
            loss = float(m.group(3))
            elapsed = float(m.group(4))
            body = line.split("|")[-1].strip()
            tag = body[:40]
        if RE_DONE.search(line):
            done = True
    n_ckpt = data.count(">> ckpt")
    total = rows * epochs if rows else 0
    alive = False
    rss = 0
    try:
        import subprocess
        out = subprocess.run(["tasklist", "/FI", "PID eq %d" % pid, "/FO", "CSV", "/NH"],
                             capture_output=True, text=True, timeout=15).stdout.strip()
        if out and "python" in out.lower():
            alive = True
            parts = out.split('","')
            try:
                rss = int(parts[-1].strip('"').replace(",", "").replace(" K", "")) // 1024
            except Exception:
                pass
    except Exception:
        pass
    return dict(rows=rows, epochs=epochs, resume=resume, step=step, loss=loss,
                elapsed=elapsed, total=total, alive=alive, rss_mb=rss,
                ckpt=n_ckpt, done=done, tag=tag, mtime=mtime)


def main():
    ap = argparse.ArgumentParser()
    here = os.path.dirname(os.path.abspath(__file__))
    ap.add_argument("--log", default=os.path.join(here, "logs", "train100k_run2.log"))
    ap.add_argument("--pid", type=int, default=0)
    ap.add_argument("--out", default=os.path.join(here, "logs", "train100k_monitor.txt"))
    ap.add_argument("--interval", type=int, default=60)
    ap.add_argument("--stall", type=int, default=900)
    ap.add_argument("--once", action="store_true", help="只打一条快照就退出（自检用）")
    a = ap.parse_args()

    last_step = -1
    last_change = time.time()
    while True:
        s = snapshot(a.log, a.pid)
        now = time.time()
        if s is None:
            line = "[%s] NO_LOG %s" % (time.strftime("%Y-%m-%d %H:%M:%S"), a.log)
            print(line, flush=True)
            return 1
        if s["step"] != last_step:
            last_step = s["step"]
            last_change = now
        pct = (s["step"] / s["total"] * 100.0) if s["total"] else 0.0
        sps = (s["step"] / s["elapsed"]) if s["elapsed"] > 0 and s["step"] else 0.0
        eta_h = ((s["total"] - s["step"]) / sps / 3600.0) if sps > 0 else -1
        loss = "-" if s["loss"] is None else "%.3f" % s["loss"]
        if s["done"]:
            state = "DONE"
        elif not s["alive"]:
            state = "DEAD"
        elif now - s["mtime"] > a.stall:
            state = "STALL"
        else:
            state = "RUN"
        line = ("[%s] %s step=%d/%d %.1f%% loss=%s spd=%.2f/s eta=%.1fh rss=%dM ckpt=%d %s"
                % (time.strftime("%Y-%m-%d %H:%M:%S"), state, s["step"], s["total"], pct,
                   loss, sps, eta_h, s["rss_mb"], s["ckpt"], s["tag"]))
        print(line, flush=True)
        if a.once:
            return 0
        try:
            with open(a.out, "a", encoding="utf-8") as f:
                f.write(line + "\n")
            # 只保留最后 3000 行，长期跑不会无限膨胀
            if os.path.getsize(a.out) > 400_000:
                with open(a.out, encoding="utf-8") as f:
                    keep = f.readlines()[-2000:]
                with open(a.out, "w", encoding="utf-8") as f:
                    f.writelines(keep)
        except Exception:
            pass
        if state in ("DONE", "DEAD"):
            return 0 if state == "DONE" else 2
        time.sleep(a.interval)


if __name__ == "__main__":
    sys.exit(main())
