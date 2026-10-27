#Requires -Version 5.1
<#
.SYNOPSIS
    命名空间分层重构：按文件所在目录，把旧的命名空间宏换成对应的层宏。

.DESCRIPTION
    规则：文件在 include/EMBMartin/<layer>/ 或 src/<layer>/ 下，
    其代码就写在 EMBMartin::<layer> 里。

    旧宏 -> 新宏：
      EMBMARTIN_NAMESPACE_BEGIN/END          -> <layer>
      EMBMARTIN_STM32F10X_NAMESPACE_BEGIN/END -> <layer>
      EMBMARTIN_SYS_NAMESPACE_BEGIN/END       -> hal
      EMBMARTIN_OLED_*                        -> 保持（已重定义为 device 的别名）
      EMBMARTIN_BALANCED_CAR_*                -> 保持（已重定义为 app 的别名）
      EMBMARTIN_DETAIL_*                      -> 保持（嵌套在层内的 detail）

    跳过 macro.h 本身（它是宏定义处）。

.PARAMETER Root
    仓库根目录，默认是本脚本所在目录的上一级。

.PARAMETER DryRun
    只报告，不写盘。

.EXAMPLE
    .\ns-migrate.ps1 -DryRun
#>
[CmdletBinding()]
param(
    [string]$Root,
    [switch]$DryRun
)

$ErrorActionPreference = 'Stop'

if (-not $Root) {
    $Root = Split-Path -Parent (Split-Path -Parent $MyInvocation.MyCommand.Path)
}
$Root = (Resolve-Path $Root).Path

$enc = New-Object System.Text.UTF8Encoding($false)

$layerMacro = @{
    'lang'      = 'EMBMARTIN_LANG_NAMESPACE'
    'hal'       = 'EMBMARTIN_HAL_NAMESPACE'
    'device'    = 'EMBMARTIN_DEVICE_NAMESPACE'
    'algorithm' = 'EMBMARTIN_ALGORITHM_NAMESPACE'
    'app'       = 'EMBMARTIN_APP_NAMESPACE'
}

$files = @()
$files += Get-ChildItem -Recurse -Path "$Root\include\EMBMartin" -Filter *.h
$files += Get-ChildItem -Recurse -Path "$Root\src" -Include *.cpp, *.c
$files = $files | Sort-Object FullName

$totalFiles = 0
$totalHits = 0
$skipped = @()

foreach ($f in $files) {
    $rel = $f.FullName.Replace("$Root\", '')

    # 确定所在层
    if ($rel -notmatch '^(?:include\\EMBMartin|src)\\(algorithm|app|device|hal|lang)\\') {
        continue
    }
    $layer = $Matches[1]

    # macro.h 是宏定义处，不参与
    if ($rel -eq 'include\EMBMartin\macro.h') {
        $skipped += $rel
        continue
    }

    $bytes = [System.IO.File]::ReadAllBytes($f.FullName)
    if ($bytes.Length -ge 3 -and $bytes[0] -eq 0xEF -and $bytes[1] -eq 0xBB -and $bytes[2] -eq 0xBF) {
        $bytes = $bytes[3..($bytes.Length - 1)]
    }
    $text = $enc.GetString($bytes)
    $orig = $text
    $hits = 0

    $mac = $layerMacro[$layer]

    # 旧的顶层宏 / STM32 宏 -> 本层宏
    foreach ($old in @('EMBMARTIN_NAMESPACE', 'EMBMARTIN_STM32F10X_NAMESPACE')) {
        foreach ($suffix in @('BEGIN', 'END')) {
            $from = "${old}_${suffix}"
            $to = "${mac}_${suffix}"
            $n = ([regex]::Matches($text, [regex]::Escape($from))).Count
            if ($n -gt 0) {
                $text = $text.Replace($from, $to)
                $hits += $n
            }
        }
    }

    # 旧的 sys 宏 -> hal（Delayer / InnerTimerManager / IRQGuard / delay_* 都在 hal）
    foreach ($suffix in @('BEGIN', 'END')) {
        $from = "EMBMARTIN_SYS_NAMESPACE_${suffix}"
        $to = "EMBMARTIN_HAL_NAMESPACE_${suffix}"
        $n = ([regex]::Matches($text, [regex]::Escape($from))).Count
        if ($n -gt 0) {
            $text = $text.Replace($from, $to)
            $hits += $n
        }
    }

    if ($text -ne $orig) {
        $totalFiles++
        $totalHits += $hits
        Write-Host ("[{0}] {1,-52} -> {2}  ({3})" -f `
            $(if ($DryRun) { 'DRY' } else { 'EDIT' }), $rel, $layer, $hits)

        if (-not $DryRun) {
            $text = $text -replace "`r`n", "`n"
            $text = $text -replace "`n", "`r`n"
            [System.IO.File]::WriteAllText($f.FullName, $text, $enc)
        }
    }
}

Write-Host ''
Write-Host ("改动 {0} 个文件，{1} 处宏{2}" -f $totalFiles, $totalHits,
    $(if ($DryRun) { '（DRY RUN）' } else { '' }))
if ($skipped) { Write-Host ("跳过: {0}" -f ($skipped -join ', ')) }
exit 0
