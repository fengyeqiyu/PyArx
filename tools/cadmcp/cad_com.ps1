param(
  [string]$Action = "save",
  [string]$Path = ""
)

$ErrorActionPreference = "Stop"

try {
  $app = [Runtime.InteropServices.Marshal]::GetActiveObject("AutoCAD.Application")
} catch {
  [ordered]@{ ok = $false; error = "AutoCAD not running" } | ConvertTo-Json -Compress -Depth 6 | Write-Output
  exit 0
}

switch ($Action) {
  "docs" {
    $list = @()
    foreach ($d in $app.Documents) {
      $list += [ordered]@{ name = $d.Name; full = $d.FullName; saved = [bool]$d.Saved; active = ($d.Name -eq $app.ActiveDocument.Name) }
    }
    [ordered]@{ ok = $true; count = @($list).Count; docs = $list } | ConvertTo-Json -Compress -Depth 6 | Write-Output
  }
  "stats" {
    try {
      $doc = $app.ActiveDocument
      $ms = $doc.ModelSpace
      $layers = @{}; $typeMap = @{}; $layerNames = @(); $handles = @(); $handlesTruncated = $false
      $n = 0
      foreach ($e in $ms) {
        $n++
        try { $l = [string]$e.Layer } catch { $l = "?" }
        if ($layers.ContainsKey($l)) { $layers[$l] = [int]$layers[$l] + 1 } else { $layers[$l] = 1 }
        try { $t = [string]$e.ObjectName } catch { $t = "?" }
        if ($typeMap.ContainsKey($t)) { $typeMap[$t] = [int]$typeMap[$t] + 1 } else { $typeMap[$t] = 1 }
        try {
          $h = [string]$e.Handle
          if ($handles.Count -lt 50) { $handles += $h } else { $handlesTruncated = $true }
        } catch { }
      }
      foreach ($ly in $doc.Layers) { try { $layerNames += [string]$ly.Name } catch {} }
      [ordered]@{ ok = $true; name = $doc.Name; count = $n; layers = $layers; types = $typeMap; layerNames = $layerNames; handles = $handles; handlesTruncated = $handlesTruncated } | ConvertTo-Json -Compress -Depth 6 | Write-Output
    } catch {
      [ordered]@{ ok = $false; error = $_.Exception.Message } | ConvertTo-Json -Compress | Write-Output
    }
  }
  "save" {
    $list = @()
    foreach ($d in $app.Documents) {
      $item = [ordered]@{ name = $d.Name }
      try {
        $d.Save()
        $item.ok = $true
        $item.saved = [bool]$d.Saved
      } catch {
        $item.ok = $false
        $item.error = $_.Exception.Message
      }
      $list += $item
    }
    [ordered]@{ ok = $true; count = @($list).Count; saved = $list } | ConvertTo-Json -Compress -Depth 6 | Write-Output
  }
  "new" {
    try {
      $doc = $app.Documents.Add()
      [ordered]@{ ok = $true; name = $doc.Name; count = $app.Documents.Count } | ConvertTo-Json -Compress | Write-Output
    } catch {
      [ordered]@{ ok = $false; error = $_.Exception.Message } | ConvertTo-Json -Compress | Write-Output
    }
  }
  "open" {
    if (-not $Path) { [ordered]@{ ok = $false; error = "Path required" } | ConvertTo-Json -Compress | Write-Output; exit 0 }
    if (-not (Test-Path -LiteralPath $Path)) { [ordered]@{ ok = $false; error = "file not found: $Path" } | ConvertTo-Json -Compress | Write-Output; exit 0 }
    try {
      $doc = $app.Documents.Open($Path)
      [ordered]@{ ok = $true; name = $doc.Name; full = $doc.FullName; count = $app.Documents.Count } | ConvertTo-Json -Compress | Write-Output
    } catch {
      [ordered]@{ ok = $false; error = $_.Exception.Message } | ConvertTo-Json -Compress | Write-Output
    }
  }
  "close" {
    $target = $null
    foreach ($d in $app.Documents) { if ($Path -and ($d.FullName -eq $Path)) { $target = $d } }
    if (-not $target -and -not $Path) { $target = $app.ActiveDocument }
    if (-not $target) { [ordered]@{ ok = $false; error = "document not open: $Path" } | ConvertTo-Json -Compress | Write-Output; exit 0 }
    try {
      $target.Close($false)
      [ordered]@{ ok = $true; closed = $target.Name; count = $app.Documents.Count } | ConvertTo-Json -Compress | Write-Output
    } catch {
      [ordered]@{ ok = $false; error = $_.Exception.Message } | ConvertTo-Json -Compress | Write-Output
    }
  }
  "quit" {
    $saveFirst = $true
    $list = @()
    try {
      foreach ($d in $app.Documents) {
        if (-not $d.Saved) {
          $item = [ordered]@{ name = $d.Name }
          try { $d.Save(); $item.ok = $true } catch { $item.ok = $false; $item.error = $_.Exception.Message }
          $list += $item
        }
      }
    } catch {
      $saveFirst = $false
    }
    $res = [ordered]@{ ok = $true; saved = $list }
    try {
      $app.Quit()
      $res.quit = "sent"
    } catch {
      $res.quit = "error: " + $_.Exception.Message
    }
    $res | ConvertTo-Json -Compress -Depth 6 | Write-Output
  }
  default {
    [ordered]@{ ok = $false; error = "unknown action: $Action" } | ConvertTo-Json -Compress | Write-Output
  }
}
