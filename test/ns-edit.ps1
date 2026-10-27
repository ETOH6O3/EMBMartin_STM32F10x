#Requires -Version 5.1
<#
.SYNOPSIS
    命名空间重构用的批量文本替换工具（保证 UTF-8 无 BOM + CRLF）。

.DESCRIPTION
    仓库约定所有库文件为 UTF-8 无 BOM + CRLF。PowerShell 的
    Set-Content -Encoding utf8 会写 BOM 并可能改换行符，因此统一走这里。

    支持三种用法：
      1. 单条替换：  -Path a.h -From 'X' -To 'Y'
      2. 正则替换：  -Path a.h -Pattern 'X(\d+)' -Replacement 'Y$1'
      3. 批量清单：  -Map @{ 'X'='Y'; 'A'='B' } -Path a.h,b.h

.PARAMETER Path
    目标文件，支持通配符与数组。

.PARAMETER From
    字面量查找内容。

.PARAMETER To
    字面量替换内容。

.PARAMETER Pattern
    正则表达式。

.PARAMETER Replacement
    正则替换内容。

.PARAMETER Map
    有序替换表（键为字面量）。

.PARAMETER DryRun
    只报告会改哪些文件，不写盘。

.EXAMPLE
    .\ns-edit.ps1 -Path ..\include\EMBMartin\device\*.h -From 'EMBMARTIN_STM32F10X_NAMESPACE_BEGIN' -To 'EMBMARTIN_DEVICE_NAMESPACE_BEGIN'
#>
[CmdletBinding(DefaultParameterSetName = 'Literal')]
param(
    [Parameter(Mandatory = $true, Position = 0)]
    [string[]]$Path,

    [Parameter(Mandatory = $true, ParameterSetName = 'Literal', Position = 1)]
    [string]$From,

    [Parameter(Mandatory = $true, ParameterSetName = 'Literal', Position = 2)]
    [string]$To,

    [Parameter(Mandatory = $true, ParameterSetName = 'Regex')]
    [string]$Pattern,

    [Parameter(Mandatory = $true, ParameterSetName = 'Regex')]
    [string]$Replacement,

    [Parameter(Mandatory = $true, ParameterSetName = 'Map')]
    [hashtable]$Map,

    [switch]$DryRun
)

$ErrorActionPreference = 'Stop'

$enc = New-Object System.Text.UTF8Encoding($false)

$files = foreach ($p in $Path) {
    Get-ChildItem -Path $p -File -ErrorAction SilentlyContinue
}
$files = $files | Sort-Object FullName -Unique

if (-not $files) {
    Write-Error "没有匹配到文件: $($Path -join ', ')"
    exit 2
}

$changedFiles = 0
$changedHits = 0

foreach ($f in $files) {
    $bytes = [System.IO.File]::ReadAllBytes($f.FullName)

    # 去掉可能存在的 BOM，统一按无 BOM 读
    if ($bytes.Length -ge 3 -and $bytes[0] -eq 0xEF -and $bytes[1] -eq 0xBB -and $bytes[2] -eq 0xBF) {
        $bytes = $bytes[3..($bytes.Length - 1)]
    }
    $text = $enc.GetString($bytes)

    $orig = $text
    $hits = 0

    switch ($PSCmdlet.ParameterSetName) {
        'Literal' {
            $n = ([regex]::Matches($text, [regex]::Escape($From))).Count
            if ($n -gt 0) {
                $text = $text.Replace($From, $To)
                $hits = $n
            }
        }
        'Regex' {
            $n = ([regex]::Matches($text, $Pattern)).Count
            if ($n -gt 0) {
                $text = [regex]::Replace($text, $Pattern, $Replacement)
                $hits = $n
            }
        }
        'Map' {
            foreach ($k in $Map.Keys) {
                $n = ([regex]::Matches($text, [regex]::Escape($k))).Count
                if ($n -gt 0) {
                    $text = $text.Replace($k, $Map[$k])
                    $hits += $n
                }
            }
        }
    }

    if ($text -ne $orig) {
        # 统一换行为 CRLF
        $text = $text -replace "`r`n", "`n"
        $text = $text -replace "`n", "`r`n"

        $changedFiles++
        $changedHits += $hits
        Write-Host ("[{0}] {1}  ({2} 处)" -f $(if ($DryRun) { 'DRY' } else { 'EDIT' }),
            $f.FullName, $hits) -ForegroundColor $(if ($DryRun) { 'DarkGray' } else { 'Green' })

        if (-not $DryRun) {
            [System.IO.File]::WriteAllText($f.FullName, $text, $enc)
        }
    }
}

Write-Host ''
Write-Host ("文件 {0} 个，替换 {1} 处{2}" -f $changedFiles, $changedHits,
    $(if ($DryRun) { '（DRY RUN，未写盘）' } else { '' }))
exit 0
