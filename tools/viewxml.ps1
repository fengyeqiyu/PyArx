# viewxml.ps1 —— 把一份 XML 界面离屏渲染成 PNG（不需要开窗口、不需要桌面会话）
#
#   pwsh tools/viewxml.ps1 -Xml examples\ui\power_plan_shell.xml
#   pwsh tools/viewxml.ps1 -Xml x.xml -Out docs\x.png -Size 1320x820
#
# 画法与设计器画布同一份（XmlLeafPaint / XmlPowerPlanDraw / XmlTableDraw），
# 所以产物就是"这份 XML 渲染出来长什么样"的标准图，适合给 AI / 文档 / CI 用。

param(
    [Parameter(Mandatory = $true)][string]$Xml,
    [string]$Out = '',
    [string]$Size = '',
    [string]$Exe = 'G:\PyArx\X64\Release\Viewer.exe'
)

$ErrorActionPreference = 'Stop'

if (-not (Test-Path -LiteralPath $Xml)) { throw "XML 不存在: $Xml" }
if (-not (Test-Path -LiteralPath $Exe)) { throw "Viewer 不存在: $Exe（先编译 Release）" }

if ([string]::IsNullOrWhiteSpace($Out))
{
    $name = [IO.Path]::GetFileNameWithoutExtension($Xml)
    $Out = Join-Path (Split-Path -Parent $Xml) ($name + '.png')
}

$cli = @('--render', $Out)
if (-not [string]::IsNullOrWhiteSpace($Size)) { $cli += @('--size', $Size) }
$cli += $Xml

# D2D 走 WARP：离屏渲染不碰显卡，远程/无桌面机器也能出图
$env:PYARX_D2D_RT = 'software'
$env:XML_D2D_AA = '1'

# Viewer 是 Windows 子系统程序，直接调用不会让 PowerShell 等它结束：
# 用 Start-Process 同步等待，拿真实退出码（否则 $LASTEXITCODE 是空的）
$p = Start-Process -FilePath $Exe -ArgumentList $cli -Wait -PassThru -NoNewWindow
if ($p.ExitCode -ne 0) { throw "渲染失败（exit=$($p.ExitCode)）" }
if (-not (Test-Path -LiteralPath $Out)) { throw "没有产出文件: $Out" }

Write-Output $Out
