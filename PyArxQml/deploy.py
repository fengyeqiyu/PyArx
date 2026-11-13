# -*- coding: utf-8 -*-
"""把 Qt 6.2.6 运行期文件部署到目标目录（默认 G:\\PyArx\\X64\\Release）。

规则（都是硬规则，写死在这里，不做"顺手清目录"）：
  1. 只处理白名单里的 Qt DLL，绝不碰 PyArxCore / PyArxEngine / MFCWebView 等自有二进制；
  2. 目标已存在且版本号一致的 DLL 直接跳过，不覆盖（避免把正在用的文件锁住）；
  3. 目标已存在但版本号不一致 → 默认拒绝覆盖并报错（防止 CAD 升级后同名 DLL 混用）；
     确实要覆盖时加 --force；
  4. qml 树按白名单子目录复制，缺什么补什么，不镜像删除。

用法：
    python deploy.py                 # 部署到默认目标
    python deploy.py --dry-run       # 只打印将要做什么
    python deploy.py --dst <目录>
"""
from __future__ import print_function

import argparse
import fnmatch
import os
import shutil
import subprocess
import sys

QT_PREFIX = r'G:\Qt\6.2.6\msvc2019_64'
DEFAULT_DST = r'G:\PyArx\X64\Release'
EXPECTED_VERSION = '6.2.6.0'

# 白名单：Qt 运行期 DLL（不含 Qt6Widgets、Qt6WebEngine 等本项目不用的）
DLLS = [
    'Qt6Core.dll', 'Qt6Gui.dll', 'Qt6Qml.dll', 'Qt6QmlModels.dll', 'Qt6Network.dll',
    'Qt6Quick.dll', 'Qt6OpenGL.dll', 'Qt6QmlWorkerScript.dll', 'Qt6QmlLocalStorage.dll',
    'Qt6QmlXmlListModel.dll', 'Qt6QmlCore.dll',
    'Qt6QuickControls2.dll', 'Qt6QuickControls2Impl.dll', 'Qt6QuickTemplates2.dll',
    'Qt6QuickLayouts.dll', 'Qt6QuickShapes.dll', 'Qt6QuickParticles.dll',
    'Qt6OpenGLWidgets.dll',
]

# 平台插件单独放 platforms\
PLATFORMS = ['qwindows.dll']

# qml 树只搬这两棵（QtQml/ 与 QtQuick/ 递归包含 Controls / Layouts / Templates / Shapes 等全部子目录）
QML_DIRS = ['QtQml', 'QtQuick']

# 绝不能覆盖的自有/第三方二进制名字前缀
FORBID_PREFIX = ('PyArxCore', 'PyArxEngine', 'PyArxQml', 'PyMfcWebView', 'PyMfcXaml',
                 'MFCWebView', 'acad', 'acdb', 'acui')


def file_version(path):
    """读 PE 文件版本号；读不到返回空串。"""
    if not os.path.exists(path):
        return ''
    try:
        ps = ('(Get-Item -LiteralPath %r).VersionInfo.FileVersion' % path)
        out = subprocess.check_output(['powershell', '-NoProfile', '-Command', ps],
                                      stderr=subprocess.STDOUT)
        return out.decode('utf-8', 'ignore').strip()
    except Exception:
        return ''


def plan_copy(src_dir, dst_dir, items, dry, force):
    """把 src_dir 下白名单 items 复制到 dst_dir，返回 (copied, skipped, blocked)"""
    copied, skipped, blocked = [], [], []
    for name in items:
        src = os.path.join(src_dir, name)
        dst = os.path.join(dst_dir, name)
        if not os.path.exists(src):
            blocked.append((name, '源缺失'))
            continue

        if any(name.lower().startswith(p.lower()) for p in FORBID_PREFIX):
            blocked.append((name, '在禁止名单里'))
            continue

        if os.path.exists(dst):
            sv, dv = file_version(src), file_version(dst)
            if sv and dv and sv != dv:
                if not force:
                    blocked.append((name, '版本不一致 dst=%s src=%s' % (dv, sv)))
                    continue
            elif sv and dv and sv == dv:
                skipped.append((name, '版本一致 %s' % sv))
                continue

        copied.append(name)
        if not dry:
            d = os.path.dirname(dst)
            if d and not os.path.isdir(d):
                os.makedirs(d)
            shutil.copy2(src, dst)
    return copied, skipped, blocked


def copy_qml_tree(src_qml, dst_qml, dry):
    """按白名单子目录复制 qml 树，返回 (新拷文件数, 跳过文件数, 缺失目录列表)"""
    copied = skipped = 0
    missing = []
    for rel in QML_DIRS:
        s = os.path.join(src_qml, rel)
        if not os.path.isdir(s):
            missing.append(rel)
            continue
        for root, dirs, files in os.walk(s):
            # 不搬 .qmlproject / 私有调试产物
            dirs[:] = [d for d in dirs if d not in ('.git',)]
            for fn in files:
                if fnmatch.fnmatch(fn, '*.qmlproject') or fn.endswith('.qmlproject.user'):
                    continue
                sp = os.path.join(root, fn)
                rp = os.path.relpath(sp, src_qml)
                dp = os.path.join(dst_qml, rp)
                if os.path.exists(dp) and os.path.getsize(dp) == os.path.getsize(sp):
                    skipped += 1
                    continue
                copied += 1
                if not dry:
                    d = os.path.dirname(dp)
                    if d and not os.path.isdir(d):
                        os.makedirs(d)
                    shutil.copy2(sp, dp)
    return copied, skipped, missing


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--src', default=QT_PREFIX, help='Qt 6.2.6 前缀（默认 %s）' % QT_PREFIX)
    ap.add_argument('--dst', default=DEFAULT_DST, help='目标目录（默认 %s）' % DEFAULT_DST)
    ap.add_argument('--dry-run', action='store_true')
    ap.add_argument('--force', action='store_true', help='版本不一致也覆盖（默认拒绝）')
    ap.add_argument('--no-qml', action='store_true', help='只补 DLL，不搬 qml 树')
    args = ap.parse_args()

    src_bin = os.path.join(args.src, 'bin')
    src_qml = os.path.join(args.src, 'qml')
    src_plat = os.path.join(args.src, 'plugins', 'platforms')
    if not os.path.isdir(src_bin):
        print('[错误] 源目录不存在：%s' % src_bin)
        return 2
    if not os.path.isdir(args.dst):
        os.makedirs(args.dst)

    print('源 = %s' % args.src)
    print('目标 = %s' % args.dst)
    print('模式 = %s%s' % ('dry-run' if args.dry_run else '实际复制', '，force' if args.force else ''))
    print('')

    print('== Qt DLL ==')
    copied, skipped, blocked = plan_copy(src_bin, args.dst, DLLS, args.dry_run, args.force)
    for n in copied:
        print('  拷 %s' % n)
    for n, why in skipped:
        print('  跳过 %s（%s）' % (n, why))
    for n, why in blocked:
        print('  [阻止] %s（%s）' % (n, why))

    print('== 平台插件 platforms\\ ==')
    c2, s2, b2 = plan_copy(src_plat, os.path.join(args.dst, 'platforms'), PLATFORMS, args.dry_run, args.force)
    for n in c2:
        print('  拷 platforms\\%s' % n)
    for n, why in s2:
        print('  跳过 platforms\\%s（%s）' % (n, why))
    for n, why in b2:
        print('  [阻止] platforms\\%s（%s）' % (n, why))

    if not args.no_qml:
        print('== qml 树 ==')
        if not os.path.isdir(src_qml):
            print('  [跳过] 源没有 qml 目录')
        else:
            qc, qs, qm = copy_qml_tree(src_qml, os.path.join(args.dst, 'qml'), args.dry_run)
            print('  新拷 %d 个文件，跳过 %d 个文件' % (qc, qs))
            if qm:
                print('  源里缺这些子目录（不影响本项目）：%s' % ', '.join(qm))

    errs = [b for b in blocked + b2 if '版本不一致' in b[1]]
    print('')
    if errs:
        print('[失败] 有 %d 个 DLL 版本与预期不一致，已拒绝覆盖。' % len(errs))
        print('        确认目标机器 Qt 版本后再决定：加 --force 强制覆盖，或换 --dst。')
        return 3
    print('[完成] 版本基线 = %s' % EXPECTED_VERSION)
    return 0


if __name__ == '__main__':
    sys.exit(main())
