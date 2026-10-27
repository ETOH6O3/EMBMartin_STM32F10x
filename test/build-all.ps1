#Requires -Version 5.1
<#
.SYNOPSIS
    一键编译 / 回归测试工作区里所有 Keil 工程。

.DESCRIPTION
    扫描工作区下所有含 project.uvprojx 的目录（默认排除备份与临时目录），
    依次用 Keil UV4 命令行编译，最后汇总成一张表。

    退出码：全部通过 0；有工程失败 1；找不到任何工程 2。

.PARAMETER Root
    工作区根目录，默认是本脚本所在目录的上一级（即仓库根，脚本位于 test/ 下）。

.PARAMETER Filter
    只编译名字匹配该通配符的工程，例如 -Filter '6-*' 或 -Filter '*OLED*'。

.PARAMETER Exclude
    额外排除的目录名通配符（默认已排除 martin_work / link_check / ref / TPL 等）。

.PARAMETER Rebuild
    全量重编（UV4 -r），默认是增量编译（UV4 -b）。

.PARAMETER List
    只列出会被编译的工程，不实际编译。

.PARAMETER LogDir
    编译日志目录，默认 <Root>\.build-logs。

.PARAMETER KeepLogs
    保留所有工程的日志（默认只保留失败工程的日志）。

.EXAMPLE
    .\build-all.ps1
    编译全部工程。

.EXAMPLE
    .\build-all.ps1 -Filter '6-*' -Rebuild
    只全量重编 6-x 那几个工程。

.EXAMPLE
    .\build-all.ps1 -List
    看看会编哪些，不真编。
#>
[CmdletBinding()]
param(
    [string]$Root,
    [string]$Filter = '*',
    [string[]]$Exclude = @('martin_work', 'link_check', 'ref', 'TPL', 'Objects', 'Listings', 'RTE', 'DebugConfig'),
    [switch]$Rebuild,
    [switch]$List,
    [string]$LogDir,
    [switch]$KeepLogs
)

$ErrorActionPreference = 'Stop'

# ---------------------------------------------------------------- 环境检查
$UV4 = 'C:\Keil_v5\UV4\UV4.exe'
if (-not (Test-Path $UV4)) {
    $UV4 = (Get-Command UV4.exe -ErrorAction SilentlyContinue)?.Source
}
if (-not $UV4 -or -not (Test-Path $UV4)) {
    Write-Host '找不到 Keil UV4.exe，请确认已安装 MDK 并加入 PATH，或修改脚本里的 $UV4。' -ForegroundColor Red
    exit 2
}

if (-not $Root) { $Root = Split-Path -Parent $PSScriptRoot }
if (-not (Test-Path $Root)) { Write-Host "Root 不存在：$Root" -ForegroundColor Red; exit 2 }
if (-not $LogDir) { $LogDir = Join-Path $Root '.build-logs' }

# ---------------------------------------------------------------- 收集工程
function Test-Excluded {
    param([string]$Path, [string]$RootPath, [string[]]$Patterns)
    $rel = $Path.Substring($RootPath.Length).TrimStart('\', '/')
    foreach ($seg in ($rel -split '[\\/]')) {
        foreach ($p in $Patterns) { if ($seg -like $p) { return $true } }
    }
    return $false
}

$projects = Get-ChildItem -Path $Root -Recurse -Filter 'project.uvprojx' -File -ErrorAction SilentlyContinue |
    Where-Object { -not (Test-Excluded $_.FullName $Root $Exclude) } |
    Where-Object { $_.Directory.Name -like $Filter -or ($_.Directory.FullName.Substring($Root.Length) -like $Filter) } |
    Sort-Object { $_.Directory.FullName }

if ($projects.Count -eq 0) {
    Write-Host "在 $Root 下没有找到匹配的工程。" -ForegroundColor Yellow
    exit 2
}

if ($List) {
    Write-Host "将编译 $($projects.Count) 个工程：" -ForegroundColor Cyan
    $projects | ForEach-Object { '  ' + $_.Directory.FullName.Substring($Root.Length).TrimStart('\') }
    exit 0
}

# ---------------------------------------------------------------- 准备日志目录
if (-not (Test-Path $LogDir)) { New-Item -ItemType Directory -Path $LogDir -Force | Out-Null }
Get-ChildItem $LogDir -Filter '*.log' -ErrorAction SilentlyContinue | Remove-Item -Force -ErrorAction SilentlyContinue

# ---------------------------------------------------------------- 逐个编译
$action = if ($Rebuild) { '-r' } else { '-b' }
$results = @()
$sw = [System.Diagnostics.Stopwatch]::StartNew()
$idx = 0

Write-Host ''
Write-Host "开始编译 $($projects.Count) 个工程（$(if($Rebuild){'全量重编'}else{'增量编译'})）" -ForegroundColor Cyan
Write-Host ('-' * 78)

foreach ($proj in $projects) {
    $idx++
    $name = $proj.Directory.Name
    $rel = $proj.Directory.FullName.Substring($Root.Length).TrimStart('\')
    $log = Join-Path $LogDir ("{0:D2}_{1}.log" -f $idx, ($name -replace '[^\w\u4e00-\u9fa5-]', '_'))

    Write-Host -NoNewline ("[{0}/{1}] {2} ... " -f $idx, $projects.Count, $rel)

    $proc = Start-Process -FilePath $UV4 `
        -ArgumentList $action, 'project.uvprojx', '-j0', '-o', $log `
        -WorkingDirectory $proj.Directory.FullName `
        -Wait -PassThru -WindowStyle Hidden

    $text = ''
    if (Test-Path $log) { $text = Get-Content -Raw -Encoding utf8 $log -ErrorAction SilentlyContinue }

    # 从日志里解析错误/警告数
    $errCount = 0
    $warnCount = 0
    if ($text -match '(\d+)\s+Error\(s\),\s*(\d+)\s+Warning\(s\)') {
        $errCount = [int]$Matches[1]
        $warnCount = [int]$Matches[2]
    } elseif ($text -match '(\d+)\s+Error') {
        $errCount = [int]$Matches[1]
    }

    $ok = ($errCount -eq 0) -and ($proc.ExitCode -eq 0 -or $proc.ExitCode -eq 1)

    if ($ok) {
        Write-Host 'OK' -ForegroundColor Green
    } else {
        Write-Host ("FAIL（{0} error）" -f $errCount) -ForegroundColor Red
        # 打印前几条错误，方便直接定位
        if ($text) {
            $errs = ($text -split "`n") | Where-Object { $_ -match 'error:|Error: ' } | Select-Object -First 3
            foreach ($e in $errs) { Write-Host ('        ' + $e.Trim()) -ForegroundColor DarkRed }
        }
    }

    $results += [pscustomobject]@{
        Project = $rel
        Status  = if ($ok) { 'OK' } else { 'FAIL' }
        Errors  = $errCount
        Warnings= $warnCount
        ExitCode= $proc.ExitCode
        Log     = $log
    }

    # 通过的工程默认删日志，省得目录里堆一堆
    if ($ok -and -not $KeepLogs -and (Test-Path $log)) { Remove-Item $log -Force }
}

$sw.Stop()

# ---------------------------------------------------------------- 汇总
$passed = @($results | Where-Object Status -eq 'OK').Count
$failed = @($results | Where-Object Status -eq 'FAIL').Count

Write-Host ('-' * 78)

# 手工排版，避免 Format-Table 在管道 / 重定向下抛 FormatEntryData 错误
# 中文占两列，按显示宽度补空格才能对齐
function Get-DisplayWidth {
    param([string]$Text)
    $w = 0
    foreach ($ch in $Text.ToCharArray()) {
        $code = [int]$ch
        # CJK 及全角字符大致范围
        if (($code -ge 0x1100 -and $code -le 0x115F) -or
            ($code -ge 0x2E80 -and $code -le 0xA4CF) -or
            ($code -ge 0xAC00 -and $code -le 0xD7A3) -or
            ($code -ge 0xF900 -and $code -le 0xFAFF) -or
            ($code -ge 0xFE30 -and $code -le 0xFE6F) -or
            ($code -ge 0xFF00 -and $code -le 0xFF60) -or
            ($code -ge 0xFFE0 -and $code -le 0xFFE6)) { $w += 2 } else { $w += 1 }
    }
    return $w
}

function Format-Row {
    param([string[]]$Cells, [int[]]$Widths)
    $sb = ''
    for ($i = 0; $i -lt $Cells.Count; $i++) {
        $cell = [string]$Cells[$i]
        $pad = $Widths[$i] - (Get-DisplayWidth $cell)
        if ($pad -lt 0) { $pad = 0 }
        $sb += $cell + (' ' * $pad)
        if ($i -lt $Cells.Count - 1) { $sb += '  ' }
    }
    return $sb.TrimEnd()
}

# 按显示宽度算各列宽
$wProj = 7
foreach ($r in $results) {
    $dw = Get-DisplayWidth $r.Project
    if ($dw -gt $wProj) { $wProj = $dw }
}
if ($wProj -gt 44) { $wProj = 44 }
$widths = @($wProj, 6, 6, 8)

$header = Format-Row @('Project', 'Status', 'Errors', 'Warnings') $widths
Write-Host $header -ForegroundColor Cyan
Write-Host ('-' * (Get-DisplayWidth $header)) -ForegroundColor DarkGray
foreach ($r in $results) {
    $line = Format-Row @($r.Project, $r.Status, [string]$r.Errors, [string]$r.Warnings) $widths
    $color = if ($r.Status -eq 'OK') { 'Gray' } else { 'Red' }
    Write-Host $line -ForegroundColor $color
}
Write-Host ('-' * (Get-DisplayWidth $header)) -ForegroundColor DarkGray

Write-Host ("合计 {0} 个工程：通过 {1}，失败 {2}，耗时 {3:N1} 秒" -f $results.Count, $passed, $failed, $sw.Elapsed.TotalSeconds) `
    -ForegroundColor $(if ($failed -eq 0) { 'Green' } else { 'Red' })

if ($failed -gt 0) {
    Write-Host ''
    Write-Host '失败工程的日志：' -ForegroundColor Yellow
    $results | Where-Object Status -eq 'FAIL' | ForEach-Object { '  ' + $_.Log }
    exit 1
}

if (-not $KeepLogs) {
    # 全通过时把空的日志目录也收掉
    $left = Get-ChildItem $LogDir -ErrorAction SilentlyContinue
    if (-not $left) { Remove-Item $LogDir -Force -ErrorAction SilentlyContinue }
}

exit 0
