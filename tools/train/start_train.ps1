# start_train.ps1 —— 启动一轮训练：先把增量池并入，再用合并池训练。
#
# 用法（参数都可改，默认是本机 1.5B + RX580 DirectML 的实测可行值）：
#   .\start_train.ps1                        # 用默认值
#   .\start_train.ps1 -MaxSteps 20000        # 指定终止步数
#   .\start_train.ps1 -OutDir qwen1.5b-cad-lora-pool
#
# 注意：脚本只负责“合并 + 起一个新训练进程”，不会杀已有训练进程。

param(
    [string]$BaseModel = "D:\models\qwen2.5-1.5b",
    [string]$OutDir    = "G:\PyArx\tools\train\out\qwen1.5b-cad-lora-pool",
    [int]$MaxSteps     = 4000,
    [int]$Epochs       = 1,
    [int]$MaxLen       = 384,
    [int]$Batch        = 1,
    [int]$Accum        = 4,
    [int]$Sample       = 200,
    [int]$Threads      = 6,
    [double]$Lr        = 2e-4
)

$ErrorActionPreference = "Stop"
$TrainDir = "G:\PyArx\tools\train"
$Py       = "C:\temp\py312\python.exe"
$env:PYTHONIOENCODING = "utf-8"

# 1) 合并增量池 -> sft_pool.jsonl
Write-Host "[1/2] 合并增量池 ..."
& $Py "$TrainDir\merge_stream.py"
if ($LASTEXITCODE -ne 0) { throw "merge_stream.py failed: $LASTEXITCODE" }

$Pool = Join-Path $TrainDir "sft_pool.jsonl"
if (-not (Test-Path -LiteralPath $Pool)) { throw "未生成 $Pool" }
$PoolRows = (Get-Content -LiteralPath $Pool -ReadCount 5000 | Measure-Object -Property Count -Sum).Sum
Write-Host ("[2/2] 训练池 {0} 条 -> 起训练 (MaxSteps={1}, Epochs={2}) ..." -f $PoolRows, $MaxSteps, $Epochs)

# 2) 起训练（独立后台进程，隐藏窗口）
$env:CAD_BASE_MODEL   = $BaseModel
$env:CAD_TRAIN_DATA   = $Pool
$env:CAD_ADAPTER_OUT  = $OutDir
$env:CAD_TRAIN_MAXLEN = "$MaxLen"
$env:CAD_TRAIN_EPOCHS = "$Epochs"
$env:CAD_TRAIN_BATCH  = "$Batch"
$env:CAD_TRAIN_ACCUM  = "$Accum"
$env:CAD_TRAIN_LR     = "$Lr"
$env:CAD_TRAIN_MAX_STEPS = "$MaxSteps"
$env:CAD_TRAIN_SAMPLE = "$Sample"
$env:CAD_TRAIN_THREADS = "$Threads"
$env:CAD_TRAIN_CKPT   = "1"

$LogDir = Join-Path $TrainDir "logs"
New-Item -ItemType Directory -Force -Path $LogDir | Out-Null
$Stamp = Get-Date -Format "MMdd_HHmm"
$Log   = Join-Path $LogDir ("train15b_pool_" + $Stamp + ".log")
$Err   = Join-Path $LogDir ("train15b_pool_" + $Stamp + ".err")

$Proc = Start-Process -FilePath $Py `
    -ArgumentList @("$TrainDir\train_lora_dml.py") `
    -WorkingDirectory $TrainDir `
    -RedirectStandardOutput $Log -RedirectStandardError $Err `
    -WindowStyle Hidden -PassThru

Write-Host ("训练已启动 pid={0}" -f $Proc.Id)
Write-Host ("日志 {0}" -f $Log)
Write-Host ("监控：`$env:CAD_MONITOR_LOG='{0}'; `$env:CAD_MONITOR_MAX_STEPS='{1}'; Start-Process '{2}\progress_mfc\TrainMonitor.exe'" -f $Log, $MaxSteps, $TrainDir)
$Proc.Id
