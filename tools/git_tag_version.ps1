# git_tag_version.ps1 —— 给当前提交自动打版本号标签
#
# 约定（与 README「版本与分支约定」一致）：
#   - master 永远是最新代码，历史版本靠 tag 固定
#   - 每个提交对应一个 v主.次.补丁 标签
#   - 提交主题以 feat / refactor 开头（或含「控件封装深化」）→ 次版本 +1、补丁归零
#   - 其余（fix / chore / build / docs / test / 网页端 Update README 等）→ 补丁 +1
#
# 用法：
#   .\tools\git_tag_version.ps1                 # 只在本机打标签
#   .\tools\git_tag_version.ps1 -Push           # 打标签并推到 Gitee（需要 $env:GITEE_TOKEN）
#   .\tools\git_tag_version.ps1 -DryRun         # 只看会打成什么版本
#
# 编码：UTF-8。

param(
    [switch]$Push,
    [switch]$DryRun
)

$ErrorActionPreference = 'Stop'

# git 不在 PATH，优先用固定路径
$gitCandidates = @(
    'C:\Program Files\Git\cmd\git.exe',
    'C:\Program Files (x86)\Git\cmd\git.exe'
)
$git = $null
foreach ($c in $gitCandidates) {
    if (Test-Path -LiteralPath $c) { $git = $c; break }
}
if ($null -eq $git) {
    $cmd = Get-Command git -ErrorAction SilentlyContinue
    if ($null -ne $cmd) { $git = $cmd.Source }
}
if ($null -eq $git) { throw '找不到 git.exe' }

function Invoke-Git {
    param([Parameter(ValueFromRemainingArguments = $true)][string[]]$Args)
    $out = & $git @Args
    if ($LASTEXITCODE -ne 0) { throw ("git 失败：git " + ($Args -join ' ')) }
    return $out
}

# 仓库根（脚本可能从任意目录调用）
$root = (Invoke-Git rev-parse --show-toplevel)
Set-Location -LiteralPath $root

# 当前 HEAD 的提交与主题
$headSha = (Invoke-Git rev-parse HEAD)
$headSubj = (Invoke-Git log -1 --format=%s HEAD)

# HEAD 是否已经有 tag
$existing = & $git tag --points-at HEAD
if ($LASTEXITCODE -ne 0) { throw 'git tag --points-at 失败' }
if ($null -ne $existing -and $existing.Count -gt 0) {
    $first = @($existing)[0]
    Write-Host ("HEAD 已有标签：{0}，无需重复打标" -f $first)
    exit 0
}

# 取最新的 vX.Y.Z 标签（按版本号排序，不按字典序）
$allTags = @(& $git tag -l 'v*')
if ($LASTEXITCODE -ne 0) { throw 'git tag -l 失败' }
$versionTags = @()
foreach ($t in $allTags) {
    if ($t -match '^v(\d+)\.(\d+)\.(\d+)$') {
        $versionTags += [pscustomobject]@{
            Tag = $t
            Major = [int]$matches[1]
            Minor = [int]$matches[2]
            Patch = [int]$matches[3]
        }
    }
}
if ($versionTags.Count -eq 0) {
    $major = 0; $minor = 1; $patch = 0
    $prevTag = '(无)'
}
else {
    $latest = $versionTags | Sort-Object Major, Minor, Patch | Select-Object -Last 1
    $prevTag = $latest.Tag
    $major = $latest.Major
    $minor = $latest.Minor
    $patch = $latest.Patch

    # 功能类提交升次版本，其余升补丁
    $isFeature = $false
    if ($headSubj -match '^(feat|refactor)') { $isFeature = $true }
    if ($headSubj -match '控件封装深化|属性表') { $isFeature = $true }
    if ($headSubj -match 'BREAKING') {
        $major = $major + 1
        $minor = 0
        $patch = 0
        $isFeature = $false
    }
    elseif ($isFeature) {
        $minor = $minor + 1
        $patch = 0
    }
    else {
        $patch = $patch + 1
    }
}

$newTag = ("v{0}.{1}.{2}" -f $major, $minor, $patch)
Write-Host ("上一个版本：{0}" -f $prevTag)
Write-Host ("本次提交：{0}" -f $headSubj)
Write-Host ("即将打标签：{0} -> {1}" -f $newTag, $headSha.Substring(0, 8))

if ($DryRun) { exit 0 }

& $git -c user.name='fengyeqiyu' -c user.email='fengyeqiyu@users.noreply.gitee.com' tag -a $newTag -m $headSubj
if ($LASTEXITCODE -ne 0) { throw ("打标签失败：" + $newTag) }
Write-Host ("已打标签：" + $newTag)

if ($Push) {
    if ($null -eq $env:GITEE_TOKEN -or $env:GITEE_TOKEN -eq '') {
        throw '需要先设置 $env:GITEE_TOKEN 才能推送标签'
    }
    $env:GIT_TERMINAL_PROMPT = '0'
    $url = "https://fengyeqiyu:$($env:GITEE_TOKEN)@gitee.com/fengyeqiyu/PyArx.git"
    $out = & $git push $url $newTag 2>&1
    if ($LASTEXITCODE -ne 0) {
        $masked = @($out) | ForEach-Object { $_ -replace ([regex]::Escape($env:GITEE_TOKEN)), '***' }
        throw ("推送标签失败：" + ($masked -join "`n"))
    }
    Write-Host ("已推送标签：" + $newTag)
}
