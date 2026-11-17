"""cad_guard.py —— AutoCAD 崩溃/弹窗守卫（纯 Win32 ctypes，不依赖 CAD 是否存活）

能力：
  scan()   列出当前所有可疑窗口（CER 错误报告、Assertion failed、Fatal Error、模态另存为等）
  close()  对这些窗口发 WM_CLOSE（优雅关闭，不杀进程）
  status() 汇总：acad 进程是否存活 + 出现过的可疑窗口

设计原则：
  * 只关"崩溃/报错"类窗口，绝不主动 kill 有未保存图纸的 acad.exe。
  * 可以被 MCP server 在每次 pyarx_run 之前调用，清掉挡住 SendCommand 的模态框。
"""

import ctypes
import ctypes.wintypes as wt
import json
import subprocess
import sys

user32 = ctypes.WinDLL("user32", use_last_error=True)
kernel32 = ctypes.WinDLL("kernel32", use_last_error=True)

WM_CLOSE = 0x0010

# 标题关键字（小写匹配）：崩溃上报、断言、致命错误、脚本错误、格式确认等
PATTERNS = {
    "cer": ["autocad 错误报告", "autocad error report", "错误报告", "error report"],
    "assert": ["assertion failed", "assert failed"],
    "fatal": ["fatal error", "致命错误", "unhandled exception"],
    "wer": ["werfault"],
}

EnumWindowsProc = ctypes.WINFUNCTYPE(wt.BOOL, wt.HWND, wt.LPARAM)


def _window_text(hwnd):
    buf = ctypes.create_unicode_buffer(512)
    user32.GetWindowTextW(hwnd, buf, 512)
    return buf.value


def _class_name(hwnd):
    buf = ctypes.create_unicode_buffer(256)
    user32.GetClassNameW(hwnd, buf, 256)
    return buf.value


def _pid_of(hwnd):
    pid = wt.DWORD()
    user32.GetWindowThreadProcessId(hwnd, ctypes.byref(pid))
    return pid.value


def _proc_name(pid):
    try:
        out = subprocess.run(
            ["tasklist", "/FI", "PID eq %d" % pid, "/FO", "CSV", "/NH"],
            capture_output=True, text=True, timeout=10,
        ).stdout.strip()
        if out.startswith('"'):
            return out.split('","')[0].strip('"')
    except Exception:
        pass
    return ""


def scan():
    found = []

    def cb(hwnd, _):
        if not user32.IsWindowVisible(hwnd):
            return True
        title = _window_text(hwnd)
        if not title:
            return True
        low = title.lower()
        kind = None
        for k, pats in PATTERNS.items():
            if any(p in low for p in pats):
                kind = k
                break
        if kind:
            found.append({
                "hwnd": hwnd,
                "title": title,
                "class": _class_name(hwnd),
                "pid": _pid_of(hwnd),
                "proc": _proc_name(_pid_of(hwnd)),
                "kind": kind,
            })
        return True

    user32.EnumWindows(EnumWindowsProc(cb), 0)
    return found


def acad_pids():
    try:
        out = subprocess.run(
            ["tasklist", "/FI", "IMAGENAME eq acad.exe", "/FO", "CSV", "/NH"],
            capture_output=True, text=True, timeout=10,
        ).stdout.strip()
        pids = []
        for line in out.splitlines():
            if line.startswith('"acad.exe'):
                pids.append(int(line.split('","')[1]))
        return pids
    except Exception:
        return []


def close(dry_run=False):
    closed = []
    for w in scan():
        ok = False
        if not dry_run:
            ok = bool(user32.PostMessageW(w["hwnd"], WM_CLOSE, 0, 0))
        closed.append({"title": w["title"], "kind": w["kind"], "pid": w["pid"],
                       "proc": w["proc"], "post_wm_close": ok})
    return closed


def status():
    return {"acad_pids": acad_pids(), "dialogs": scan()}


if __name__ == "__main__":
    cmd = sys.argv[1] if len(sys.argv) > 1 else "status"
    if cmd == "scan":
        print(json.dumps({"dialogs": scan()}, ensure_ascii=False, indent=1))
    elif cmd == "close":
        dry = "--dry" in sys.argv
        print(json.dumps({"closed": close(dry_run=dry)}, ensure_ascii=False, indent=1))
    else:
        print(json.dumps(status(), ensure_ascii=False, indent=1))
