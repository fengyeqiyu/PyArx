<#
.SYNOPSIS
  AutoCAD 崩溃进程守卫：崩溃报告进程一出现就杀，并连带清掉它指向的已崩 CAD 进程。

.DESCRIPTION
  主检测（进程级，默认 0.5s 一次）：扫 senddmp.exe / senddmp64.exe / WerFault.exe 等崩溃报告宿主，
  出现即结束其进程树；同时从其命令行解析 /PID，若指向 acad/acadlt/accoreconsole 等 CAD 进程且仍存活，一并结束。
  兜底检测（窗口级，每 10 个 tick 一次）：扫崩溃/错误弹窗（标题含 错误报告/已停止工作/未响应/Application Error 等），
  关窗并清理宿主；CAD 自身弹的错误框只在进程卡死时才杀。

  用法：
    pwsh -File G:\PyArx\tools\CadCrashGuard.ps1 -Loop        # 常驻（默认 0.5s）
    pwsh -File G:\PyArx\tools\CadCrashGuard.ps1 -Once        # 扫一次
    pwsh -File G:\PyArx\tools\CadCrashGuard.ps1 -Status      # 只看，不动手
    pwsh -File G:\PyArx\tools\CadCrashGuard.ps1 -Stop        # 停常驻实例
    -Interval 0.5 / -DryRun / -WatchPid 29088 / -KillHungCad -HungSeconds 30

  退出码：0 正常（无命中或已处理完）/ 4 有清理失败
#>
[CmdletBinding()]
param(
  [switch]$Status,
  [switch]$Once,
  [switch]$Loop,
  [switch]$Stop,
  [switch]$DryRun,
  [switch]$KillHungCad,
  [double]$Interval = 0.5,
  [int]$HungSeconds = 30,
  [int]$WatchPid = 0,
  [switch]$Quiet,
  [string]$LogPath = ''
)

$ErrorActionPreference = 'Continue'
$script:Root = Split-Path -Parent $PSScriptRoot
if (-not $LogPath) { $LogPath = Join-Path $script:Root 'log\cad_crash_guard.log' }
$script:LogPath = $LogPath
$script:PidFile = Join-Path $script:Root 'log\cad_crash_guard.pid'

# 崩溃报告宿主进程（出现即视为崩溃）
$script:CerProcs = @('senddmp', 'senddmp64', 'adskcer', 'adskcerreport', 'adskysoft')
# 受管 CAD 进程
$script:CadProcs = @('acad', 'acadlt', 'accoreconsole', 'acadcore', 'acadltcore')
$script:TitleRx = '错误报告|意外关闭|停止工作|已停止|未响应|不响应|应用程序错误|致命错误|崩溃|Error Report|Application Error|stopped working|not responding|FATAL|Unhandled|Exception'
$script:ClassRx = '32770|Adsk.*Cer|SendDmp|CERWnd'

# ---------------------------------------------------------------- 窗口枚举
if (-not ('CadGuardWin' -as [type])) {
  Add-Type -TypeDefinition @'
using System;
using System.Text;
using System.Collections.Generic;
using System.Runtime.InteropServices;

public class CadGuardWin {
    public long Hwnd; public uint Pid; public string Title; public string Class; public bool Hung;
}

public static class CadGuardApi {
    delegate bool EnumProc(IntPtr h, IntPtr l);
    [DllImport("user32.dll")] static extern bool EnumWindows(EnumProc cb, IntPtr p);
    [DllImport("user32.dll", CharSet = CharSet.Unicode)] static extern int GetWindowTextW(IntPtr h, StringBuilder s, int n);
    [DllImport("user32.dll", CharSet = CharSet.Unicode)] static extern int GetClassNameW(IntPtr h, StringBuilder s, int n);
    [DllImport("user32.dll")] static extern uint GetWindowThreadProcessId(IntPtr h, out uint pid);
    [DllImport("user32.dll")] static extern bool IsWindowVisible(IntPtr h);
    [DllImport("user32.dll")] static extern bool IsWindow(IntPtr h);
    [DllImport("user32.dll")] static extern bool IsHungAppWindow(IntPtr h);
    [DllImport("user32.dll", CharSet = CharSet.Unicode)]
    static extern IntPtr SendMessageTimeout(IntPtr h, uint msg, IntPtr w, IntPtr l, uint flags, uint timeout, out IntPtr res);

    public static CadGuardWin[] VisibleWindows() {
        var list = new List<CadGuardWin>();
        EnumWindows(delegate(IntPtr h, IntPtr l) {
            if (!IsWindowVisible(h)) return true;
            var sb = new StringBuilder(1024);
            GetWindowTextW(h, sb, 1024);
            if (sb.Length == 0) return true;
            var cb = new StringBuilder(256);
            GetClassNameW(h, cb, 256);
            uint pid = 0;
            GetWindowThreadProcessId(h, out pid);
            var w = new CadGuardWin();
            w.Hwnd = (long)h; w.Pid = pid; w.Title = sb.ToString();
            w.Class = cb.ToString(); w.Hung = IsHungAppWindow(h);
            list.Add(w);
            return true;
        }, IntPtr.Zero);
        return list.ToArray();
    }

    public static bool Close(long hwnd) {
        IntPtr r;
        SendMessageTimeout(new IntPtr(hwnd), 0x0010 /*WM_CLOSE*/, IntPtr.Zero, IntPtr.Zero,
                           0x0002 /*SMTO_ABORTIFHUNG*/, 800, out r);
        return !IsWindow(new IntPtr(hwnd));
    }

    public static bool Alive(long hwnd) { return IsWindow(new IntPtr(hwnd)) && IsWindowVisible(new IntPtr(hwnd)); }
}
'@
}

# ---------------------------------------------------------------- 日志
function Write-Log {
  param([string]$Msg, [string]$Level = 'INFO')
  $line = ('{0} [{1}] {2}' -f (Get-Date -Format 'HH:mm:ss.fff'), $Level, $Msg)
  if (-not $Quiet -and $Level -ne 'INFO') { Write-Host $line }
  try {
    if ((Test-Path -LiteralPath $script:LogPath) -and (Get-Item -LiteralPath $script:LogPath).Length -gt 2MB) {
      Move-Item -LiteralPath $script:LogPath -Destination ($script:LogPath + '.1') -Force
    }
    Add-Content -LiteralPath $script:LogPath -Value $line -Encoding utf8
  } catch { }
}

# ---------------------------------------------------------------- 进程工具
function Get-ProcMeta {
  param([int]$ProcId)
  try {
    $p = Get-CimInstance Win32_Process -Filter "ProcessId=$ProcId" -ErrorAction Stop
    if ($p) {
      return [pscustomobject]@{
        Id = [int]$p.ProcessId; Parent = [int]$p.ParentProcessId; Name = $p.Name
        NameLower = $p.Name.ToLowerInvariant().Replace('.exe', ''); CmdLine = [string]$p.CommandLine
      }
    }
  } catch { }
  return $null
}

function Get-CerTargets {
  param([string]$CmdLine)
  $o = [ordered]@{ CrashPid = 0; Dmp = ''; Dwg = '' }
  if (-not $CmdLine) { return $o }
  if ($CmdLine -match '/PID\s+(\d+)') { $o.CrashPid = [int]$Matches[1] }
  if ($CmdLine -match '/DMP\s+"?([^"]+?\.dmp)' ) { $o.Dmp = $Matches[1] }
  if ($CmdLine -match '/DWG\s+"?([^"]+?)"?(?:\s|$)') { $o.Dwg = $Matches[1] }
  return $o
}

function Kill-Now {
  param([int]$ProcId, [string]$Why)
  $p = Get-Process -Id $ProcId -ErrorAction SilentlyContinue
  if (-not $p) { Write-Log ("pid={0} 已不在（{1}）" -f $ProcId, $Why); return $true }
  $t0 = Get-Date
  try { $p.Kill() } catch { }
  for ($i = 0; $i -lt 6; $i++) {
    Start-Sleep -Milliseconds 100
    if (-not (Get-Process -Id $ProcId -ErrorAction SilentlyContinue)) {
      Write-Log ("已杀 pid={0} {1} ({2}) 耗时{3}ms" -f $ProcId, $p.ProcessName, $Why, [int]((Get-Date) - $t0).TotalMilliseconds) 'KILL'
      return $true
    }
  }
  & taskkill.exe /PID $ProcId /T /F 2>$null | Out-Null
  Start-Sleep -Milliseconds 200
  $gone = -not (Get-Process -Id $ProcId -ErrorAction SilentlyContinue)
  Write-Log ("pid={0} {1} ({2}) -> {3}" -f $ProcId, $p.ProcessName, $Why, $(if ($gone) { 'taskkill 成功' } else { 'taskkill 失败' })) $(if ($gone) { 'KILL' } else { 'FAIL' })
  return $gone
}

# ---------------------------------------------------------------- 主检测：进程级
function Invoke-ProcessSweep {
  param([switch]$ReportOnly)
  $hits = 0; $fails = 0
  $names = @($script:CerProcs + 'werfault')
  $victims = @(Get-Process -Name $names -ErrorAction SilentlyContinue | Where-Object { $_.Id -ne $PID })
  if ($WatchPid) { $victims = @($victims | Where-Object { $_.Id -eq $WatchPid }) }

  foreach ($v in $victims) {
    $m = Get-ProcMeta $v.Id
    if (-not $m) { continue }
    # 无条件：崩溃报告宿主（senddmp / WerFault）出现即杀，不做命令行关联判断
    $cer = Get-CerTargets $m.CmdLine
    $hits++
    if ($ReportOnly) { Write-Log ("存在崩溃进程 pid={0} {1} crashpid={2}" -f $v.Id, $m.Name, $cer.CrashPid); continue }
    if ($DryRun) { Write-Log ("命中(仅报告) pid={0} {1} crashpid={2} dmp={3} dwg={4}" -f $v.Id, $m.Name, $cer.CrashPid, $cer.Dmp, $cer.Dwg) 'DRY'; continue }
    Write-Log ("命中崩溃进程 pid={0} {1} crashpid={2} dmp={3} dwg={4}" -f $v.Id, $m.Name, $cer.CrashPid, $cer.Dmp, $cer.Dwg) 'HIT'
    if (-not (Kill-Now -ProcId $v.Id -Why '崩溃报告宿主')) { $fails++ }
    if ($cer.CrashPid -gt 0 -and $cer.CrashPid -ne $v.Id) {
      $cp = Get-ProcMeta $cer.CrashPid
      if ($cp -and ($script:CadProcs -contains $cp.NameLower)) {
        if (-not (Kill-Now -ProcId $cer.CrashPid -Why '崩溃 CAD 进程(/PID)')) { $fails++ }
      } elseif ($cp) { Write-Log ("/PID {0} 是 {1}，不在 CAD 名单，跳过" -f $cer.CrashPid, $cp.Name) }
    }
  }
  return [pscustomobject]@{ Hits = $hits; Failed = $fails }
}

# ---------------------------------------------------------------- 兜底：窗口级
function Invoke-WindowSweep {
  $hits = @()
  $wins = [CadGuardApi]::VisibleWindows()
  if ($wins.Count -eq 0) { return @() }
  $meta = @{}
  foreach ($w in $wins) {
    if ($w.Pid -eq $PID) { continue }
    if (-not $meta.ContainsKey($w.Pid)) { $meta[$w.Pid] = Get-ProcMeta $w.Pid }
    $m = $meta[$w.Pid]
    if (-not $m) { continue }
    if ($WatchPid -and $w.Pid -ne $WatchPid) { continue }
    $t = $w.Title
    $kind = $null
    if ($script:CerProcs -contains $m.NameLower) { $kind = 'CER' }
    elseif ($script:CadProcs -contains $m.NameLower -and $t -match $script:TitleRx -and $w.Class -match $script:ClassRx) { $kind = 'CAD-ERROR-DIALOG' }
    if (-not $kind) { continue }
    $cer = Get-CerTargets $m.CmdLine
    $hits += [pscustomobject]@{ Kind = $kind; Pid = [int]$w.Pid; Hwnd = $w.Hwnd; Proc = $m.Name; Title = $t; Hung = $w.Hung; CrashPid = $cer.CrashPid; Dmp = $cer.Dmp; Dwg = $cer.Dwg }
  }
  return $hits
}

function Invoke-WindowFix {
  param($Hits, [switch]$ReportOnly)
  $fails = 0
  foreach ($h in $Hits) {
    if (-not $ReportOnly) {
      if ($h.Kind -eq 'CER') {
        Write-Log ("关闭崩溃窗口 pid={0} '{1}'" -f $h.Pid, $h.Title) 'HIT'
      }
    }
    if (-not $ReportOnly -and -not $DryRun) {
      $hw = [long]$h.Hwnd
      if ($hw -ne 0 -and [CadGuardApi]::Alive($hw)) { [void][CadGuardApi]::Close($hw) }
      $m = Get-ProcMeta ([int]$h.Pid)
      if ($m -and ($script:CerProcs -contains $m.NameLower)) {
        if (-not (Kill-Now -ProcId $h.Pid -Why '崩溃报告宿主')) { $fails++ }
      } elseif ($m -and $h.Kind -eq 'CAD-ERROR-DIALOG') {
        $proc = Get-Process -Id $h.Pid -ErrorAction SilentlyContinue
        if ($proc -and -not $proc.Responding) {
          if (-not (Kill-Now -ProcId $h.Pid -Why 'CAD 卡死')) { $fails++ }
        } else { Write-Log ("pid={0} {1} 仍响应，仅关窗" -f $h.Pid, $m.Name) }
      }
      if ($h.CrashPid -gt 0) {
        $cp = Get-ProcMeta ([int]$h.CrashPid)
        if ($cp -and ($script:CadProcs -contains $cp.NameLower)) {
          if (-not (Kill-Now -ProcId $h.CrashPid -Why '崩溃 CAD 进程(/PID)')) { $fails++ }
        }
      }
      if ($hw -ne 0 -and [CadGuardApi]::Alive($hw)) { Write-Log ("hwnd=0x{0:X} 仍在" -f $hw) 'FAIL'; $fails++ }
    }
  }
  return $fails
}

# ---------------------------------------------------------------- 入口
if ($Stop) {
  if (Test-Path -LiteralPath $script:PidFile) {
    $old = 0
    try { $old = [int]((Get-Content -LiteralPath $script:PidFile -Raw).Trim()) } catch { }
    if ($old -gt 0) {
      $pr = Get-ProcMeta $old
      if ($pr -and $pr.CmdLine -match 'CadCrashGuard') { Kill-Now -ProcId $old -Why '停止守卫' | Out-Null }
      else { Write-Log ("pid={0} 不是守卫实例，忽略" -f $old) }
    }
    Remove-Item -LiteralPath $script:PidFile -Force -ErrorAction SilentlyContinue
  }
  'stopped'
  exit 0
}

if ($Status) {
  $p = Invoke-ProcessSweep -ReportOnly
  $w = Invoke-WindowSweep
  "崩溃进程: $($p.Hits)   崩溃窗口: $($w.Count)"
  if ($w.Count -gt 0) { $w | Select-Object Kind, Pid, Proc, Title, CrashPid, Dmp | Format-Table -AutoSize | Out-String -Width 200 }
  Get-Process -Name @($script:CerProcs) -ErrorAction SilentlyContinue | Select-Object Id, ProcessName, StartTime | Format-Table -AutoSize | Out-String
  exit 0
}

if ($Loop) {
  Set-Content -LiteralPath $script:PidFile -Value $PID -Encoding utf8
  Write-Log ("守卫启动 pid={0} interval={1}s" -f $PID, $Interval) 'START'
  $tick = 0
  while ($true) {
    try {
      $r = Invoke-ProcessSweep
      if ($r.Hits -gt 0 -and -not $Quiet) { Write-Host ("[{0}] 拦截崩溃进程 {1} 个" -f (Get-Date -Format 'HH:mm:ss'), $r.Hits) }
      # 兜底窗口检测每 10 tick（默认 5s）一次
      if ($tick % 10 -eq 0) {
        $w = Invoke-WindowSweep
        if ($w.Count -gt 0) { [void](Invoke-WindowFix -Hits $w) }
      }
      if ($KillHungCad) {
        foreach ($cp in $script:CadProcs) {
          foreach ($proc in (Get-Process -Name $cp -ErrorAction SilentlyContinue)) {
            if (-not $proc.Responding) {
              $k = "h$($proc.Id)"
              if (-not $script:HungSeen.ContainsKey($k)) { $script:HungSeen[$k] = Get-Date; Write-Log ("pid={0} {1} 无响应，计时" -f $proc.Id, $proc.ProcessName) }
              elseif (((Get-Date) - $script:HungSeen[$k]).TotalSeconds -ge $HungSeconds) { [void](Kill-Now -ProcId $proc.Id -Why 'CAD 长时间无响应') }
            } else { $script:HungSeen.Remove($k) }
          }
        }
      }
      $tick++
    } catch { Write-Log ("扫描异常: " + $_.Exception.Message) 'ERROR' }
    Start-Sleep -Milliseconds ([int]($Interval * 1000))
  }
}

# 单次
$sw = [Diagnostics.Stopwatch]::StartNew()
$p = Invoke-ProcessSweep
$w = Invoke-WindowSweep
$fails = $p.Failed
if ($w.Count -gt 0) { $fails += (Invoke-WindowFix -Hits $w) }
if ($p.Hits -eq 0 -and $w.Count -eq 0) { Write-Host '无崩溃进程/窗口，未做处理'; exit 0 }
Write-Log ("本轮完成 崩溃进程={0} 崩溃窗口={1} 失败={2} 耗时={3}ms" -f $p.Hits, $w.Count, $fails, [int]$sw.Elapsed.TotalMilliseconds) $(if ($fails) { 'FAIL' } else { 'OK' })
if ($fails -gt 0) { exit 4 }
exit 0
