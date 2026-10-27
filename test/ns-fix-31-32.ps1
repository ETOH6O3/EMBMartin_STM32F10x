#Requires -Version 5.1
<#
.SYNOPSIS
    把 3-1 / 3-2 两个自包含教学工程的本地 platform 副本改为引用仓库根 platform。

.DESCRIPTION
    这两个工程原本自带 Library / Start / System / User 全套副本，与仓库根的
    platform/ 内容一致（User 只多一个 main.cpp）。改为统一引用 ..\..\platform\
    后：
      - 仓库里只剩一份 platform 实现
      - 不会与 platform\System\cdelay.c 出现重复符号
      - 与 3-3 及之后所有工程结构一致

    User 组只保留 main.cpp（其余 3 个文件改用 platform\User）。

.PARAMETER Root
    仓库根目录。

.PARAMETER DryRun
    只报告不写盘。
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

$targets = @('3-1 LED闪烁', '3-2 LED流水灯')

foreach ($name in $targets) {
    $proj = "$Root\example\$name\project.uvprojx"
    if (-not (Test-Path $proj)) {
        Write-Host "[SKIP] $name  （找不到工程文件）" -ForegroundColor DarkGray
        continue
    }

    $bytes = [System.IO.File]::ReadAllBytes($proj)
    $text = $enc.GetString($bytes)
    $orig = $text

    # Library / Start / System / User 下的本地路径 -> 仓库根 platform
    $text = $text.Replace('.\Library\', '..\..\platform\Library\')
    $text = $text.Replace('.\Start\', '..\..\platform\Start\')
    $text = $text.Replace('.\System\Delay.c', '..\..\platform\System\cdelay.c')
    $text = $text.Replace('.\System\Delay.h', '..\..\platform\System\cdelay.h')

    # User 组：保留 main.cpp，其余改到 platform\User
    foreach ($f in @('stm32f10x_conf.h', 'stm32f10x_it.c', 'stm32f10x_it.h')) {
        $text = $text.Replace(".\User\$f", "..\..\platform\User\$f")
    }

    # FileName 也要跟着改（Delay.c -> cdelay.c）
    $text = $text.Replace('<FileName>Delay.c</FileName>', '<FileName>cdelay.c</FileName>')
    $text = $text.Replace('<FileName>Delay.h</FileName>', '<FileName>cdelay.h</FileName>')

    if ($text -ne $orig) {
        $left = ([regex]::Matches($text, '<FilePath>\.\\')).Count
        Write-Host ("[{0}] {1,-16} 剩余本地路径 {2} 个" -f `
            $(if ($DryRun) { 'DRY' } else { 'EDIT' }), $name, $left)

        if (-not $DryRun) {
            [System.IO.File]::WriteAllText($proj, $text, $enc)
        }
    }
    else {
        Write-Host "[SKIP] $name  （无改动）" -ForegroundColor DarkGray
    }
}

Write-Host ''
Write-Host ("完成{0}" -f $(if ($DryRun) { '（DRY RUN）' } else { '' }))
exit 0
