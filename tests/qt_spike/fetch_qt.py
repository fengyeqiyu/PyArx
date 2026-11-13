# -*- coding: utf-8 -*-
"""绕过 aqtinstall 的校验和：按镜像的 Updates.xml 拉取 7z 并解压到 G:\Qt。

镜像上的归档与 Qt 官方 checksum 不一致，aqt 会拒收，所以这里自己下载 + py7zr 解压。
用法： python fetch_qt.py --list   只列清单
       python fetch_qt.py          下载并解压
"""
import os
import sys
import urllib.request
import xml.etree.ElementTree as ET

DESKTOP = "https://mirrors.aliyun.com/qt/online/qtsdkrepository/windows_x86/desktop/qt6_624"
DEST = r"G:\Qt"
WANT = [
    "qt.qt6.624.win64_msvc2019_64",                       # base：qtbase / qtdeclarative / icu / opengl32sw ...
    "qt.qt6.624.addons.qtwebengine.win64_msvc2019_64",
    "qt.qt6.624.addons.qtwebchannel.win64_msvc2019_64",
    "qt.qt6.624.addons.qtpositioning.win64_msvc2019_64",
]


def collect():
    req = urllib.request.Request(DESKTOP + "/Updates.xml", headers={"User-Agent": "Mozilla/5.0"})
    root = ET.fromstring(urllib.request.urlopen(req, timeout=60).read())
    out = []
    for pkg in root.iter("PackageUpdate"):
        name = (pkg.findtext("Name") or "").strip()
        if name not in WANT:
            continue
        ver = (pkg.findtext("Version") or "").strip()
        archives = (pkg.findtext("DownloadableArchives") or "").strip()
        if not archives:
            continue
        for a in archives.split(","):
            a = a.strip()
            if not a:
                continue
            out.append({"pkg": name, "version": ver, "file": ver + a,
                        "url": "%s/%s/%s" % (DESKTOP, name, ver + a)})
    return out


def size_of(url):
    req = urllib.request.Request(url, method="HEAD", headers={"User-Agent": "Mozilla/5.0"})
    with urllib.request.urlopen(req, timeout=60) as r:
        return int(r.headers.get("Content-Length", 0))


def main():
    items = collect()
    total = 0
    for i in items:
        i["size"] = size_of(i["url"])
        total += i["size"]
        print("%7.1f MB  %s  (%s)" % (i["size"] / 1048576, i["file"], i["pkg"].split(".")[2]))
    print("合计 %.0f MB，%d 个归档" % (total / 1048576, len(items)))
    if "--list" in sys.argv:
        return

    dl = os.path.join(DEST, "_dl")
    os.makedirs(dl, exist_ok=True)
    for i in items:
        path = os.path.join(dl, i["file"])
        if os.path.exists(path) and os.path.getsize(path) >= i["size"] * 0.99:
            print("跳过 " + i["file"])
            continue
        print("下载 " + i["file"])
        req = urllib.request.Request(i["url"], headers={"User-Agent": "Mozilla/5.0"})
        with urllib.request.urlopen(req, timeout=900) as r, open(path, "wb") as f:
            while True:
                chunk = r.read(1 << 20)
                if not chunk:
                    break
                f.write(chunk)
        print("  完成 %.1f MB" % (os.path.getsize(path) / 1048576))

    import py7zr
    for i in items:
        path = os.path.join(dl, i["file"])
        print("解压 " + i["file"])
        with py7zr.SevenZipFile(path, "r") as z:
            z.extractall(DEST)
    print("ALL DONE -> " + DEST)


if __name__ == "__main__":
    main()
