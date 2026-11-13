# publish.ps1 —— 一键自动发布：提交 -> 推送 master -> 给历史补打版本号 -> 推送标签
#
# 约定（README「版本与分支约定」）：
#   master 永远最新；每个提交一个 v主.次.补丁 标签；
#   主题以 feat / refactor 开头（或含「控件封装深化」「属性表」）-> 次版本+1；
#   BREAKING -> 主版本+1；其余 -> 补丁+1。
#
# 用法（在仓库根或任意目录都行）：
#   .\tools\publish.ps1 -Build -Message "本次改动说明"     # 先编译 Release，再提交发布
#   .\tools\publish.ps1 -Message "..."                     # 不编译，直接提交发布
#   .\tools\publish.ps1 -DryRun                            # 只演练，不写库、不推送
#   推送需要环境变量 $env:GITEE_TOKEN（令牌只出现在内存里，不落地到脚本）。

param(
    [string]$Message,
    [switch]$Build,
    [switch]$DryRun
)

$ErrorActionPreference = 'Stop'

# ---------- git 路径 ----------
$gitCandidates = @(
    'C:\Program Files\Git\cmd\git.exe',
    'C:\Program Files (x86)\Git\cmd\git.exe'
)
$git = $null
foreach ($c in $gitCandidates) { if (Test-Path -LiteralPath $c) { $git = $c; break } }
if ($null -eq $git) { $git = (Get-Command git -ErrorAction SilentlyContinue).Source }
if ($null -eq $git) { throw '找不到 git.exe' }

function Invoke-Git {
    param([Parameter(ValueFromRemainingArguments = $true)][string[]]$Args)
    $out = & $git @Args
    if ($LASTEXITCODE -ne 0) { throw ("git 失败：git " + ($Args -join ' ')) }
    return $out
}

$root = (Invoke-Git rev-parse --show-toplevel)
Set-Location -LiteralPath $root

# ---------- 1) 可选：编译 Release ----------
if ($Build) {
    $msb = 'C:\Program Files\Microsoft Visual Studio\18\Community\MSBuild\Current\Bin\MSBuild.exe'
    if (-not (Test-Path -LiteralPath $msb)) { throw "找不到 MSBuild：$msb" }
    Get-Process UIDesign, Viewer -ErrorAction SilentlyContinue | Stop-Process -Force
    $env:ComSpec = 'C:\Windows\System32\cmd.exe'
    Write-Host '==> 编译 Release x64'
    & $msb (Join-Path $root 'PyArx.sln') '/t:UIDesign;Viewer' `
        /p:Configuration=Release /p:Platform=x64 /p:VcpkgApplocalDeps=false /m /nologo /v:m
    if ($LASTEXITCODE -ne 0) { throw '编译失败，已中止发布' }
}

# ---------- 2) 提交未跟踪之外的改动 ----------
# 只看已跟踪文件的改动（?? 未跟踪文件不纳入自动提交）
$porcelain = @(Invoke-Git status --porcelain | Where-Object { $_ -notmatch '^\?\?' })
if ($porcelain.Count -gt 0) {
    if ([string]::IsNullOrWhiteSpace($Message)) {
        throw '存在未提交改动，请用 -Message "提交说明" 指定提交主题'
    }
    Write-Host '==> 提交改动'
    Invoke-Git add -u
    Invoke-Git add tools/publish.ps1
    if ($DryRun) {
        Write-Host ("[DryRun] 待提交：{0}" -f $Message)
    }
    else {
        Invoke-Git -c user.name='fengyeqiyu' -c user.email='fengyeqiyu@users.noreply.gitee.com' `
            commit -m $Message
    }
}
else {
    Write-Host '工作区干净，无需新提交'
}

# ---------- 3) 解析当前最新版本 ----------
$latestTags = @(& $git tag -l 'v*')
$major = 0; $minor = 1; $patch = 0
foreach ($t in $latestTags) {
    if ($t -match '^v(\d+)\.(\d+)\.(\d+)$') {
        $m, $mi, $p = [int]$matches[1], [int]$matches[2], [int]$matches[3]
        if ($m -gt $major -or ($m -eq $major -and $mi -gt $minor) -or
            ($m -eq $major -and $mi -eq $minor -and $p -gt $patch)) {
            $major, $minor, $patch = $m, $mi, $p
        }
    }
}

function Get-NextVersion([string]$subject) {
    if ($subject -match 'BREAKING')      { return @(($major + 1), 0, 0) }
    if ($subject -match '^(feat|refactor)') { return @($major, ($minor + 1), 0) }
    if ($subject -match '控件封装深化|属性表') { return @($major, ($minor + 1), 0) }
    return @($major, $minor, ($patch + 1))
}

# ---------- 4) 给还没标签的提交按顺序补标 ----------
$lastTag = & $git describe --tags --abbrev=0 2>$null
if ($LASTEXITCODE -ne 0) { $lastTag = $null }
$shas = if ($lastTag) { @(Invoke-Git rev-list --reverse "$lastTag..HEAD") }
        else { @(Invoke-Git rev-list --reverse HEAD) }

$baseLabel = if ($lastTag) { $lastTag } else { '无' }
Write-Host ("==> 待补标提交 {0} 个（基线 {1}）" -f $shas.Count, $baseLabel)
$tagged = @()
foreach ($sha in $shas) {
    $subject = (Invoke-Git log -1 --format=%s $sha)
    $already = @(& $git tag --points-at $sha)
    if ($already.Count -gt 0) { continue }

    ($vMajor, $vMinor, $vPatch) = Get-NextVersion $subject
    $newTag = ("v{0}.{1}.{2}" -f $vMajor, $vMinor, $vPatch)
    $major, $minor, $patch = $vMajor, $vMinor, $vPatch

    Write-Host ("  {0}  {1}  {2}" -f $sha.Substring(0, 8), $newTag, $subject)
    if (-not $DryRun) {
        & $git -c user.name='fengyeqiyu' -c user.email='fengyeqiyu@users.noreply.gitee.com' `
            tag -a $newTag $sha -m $subject
        if ($LASTEXITCODE -ne 0) { throw ("打标签失败：" + $newTag) }
    }
    $tagged += $newTag
}

# ---------- 5) 推送 master 与标签 ----------
if ($DryRun) {
    Write-Host '[DryRun] 不推送 master / 标签'
    exit 0
}
if ([string]::IsNullOrWhiteSpace($env:GITEE_TOKEN)) {
    # 没设环境变量时，从 origin 远程地址里取已存的令牌
    $remote = & $git remote get-url origin 2>$null
    if ($remote -match '://[^:]+:([^@]+)@') { $env:GITEE_TOKEN = $matches[1] }
}
if ([string]::IsNullOrWhiteSpace($env:GITEE_TOKEN)) {
    throw '需要先设置 $env:GITEE_TOKEN 才能推送到 Gitee'
}
$env:GIT_TERMINAL_PROMPT = '0'
$url = "https://fengyeqiyu:$($env:GITEE_TOKEN)@gitee.com/fengyeqiyu/PyArx.git"
$masked = [regex]::Escape($env:GITEE_TOKEN)

Write-Host '==> 推送 master'
$out = & $git push $url master 2>&1
if ($LASTEXITCODE -ne 0) { throw (($out | ForEach-Object { $_ -replace $masked, '***' }) -join "`n") }
$out | ForEach-Object { $_ -replace $masked, '***' }

Write-Host '==> 推送标签'
$out = & $git push $url --tags 2>&1
if ($LASTEXITCODE -ne 0) { throw (($out | ForEach-Object { $_ -replace $masked, '***' }) -join "`n") }
$out | ForEach-Object { $_ -replace $masked, '***' }

Write-Host ("发布完成：HEAD={0} 最新标签={1}" -f ((Invoke-Git rev-parse --short HEAD)), $tagged[-1])
