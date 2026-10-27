#Requires -Version 5.1
<#
.SYNOPSIS
    命名空间重构期间的快速语法自检（不链接、不依赖 Keil 工程）。

.DESCRIPTION
    用 Keil 自带的 armclang 对每个头文件 / 源文件单独做 -fsyntax-only，
    比 UV4 全量编译快得多，用于重构过程中每改一批就验证一次。

    退出码：全部通过 0；有文件失败 1。

.PARAMETER Root
    仓库根目录，默认是本脚本所在目录的上一级。

.PARAMETER Only
    只检查路径匹配该通配符的文件，例如 -Only '*device*'。

.PARAMETER Verbose
    打印每个文件的结果（默认只打印失败项）。

.EXAMPLE
    .\check-ns.ps1
    检查全部头文件与源文件。
#>
[CmdletBinding()]
param(
    [string]$Root,
    [string]$Only,
    [switch]$HeadersOnly,
    [switch]$SrcOnly
)

$ErrorActionPreference = 'Stop'

if (-not $Root) {
    $Root = Split-Path -Parent (Split-Path -Parent $MyInvocation.MyCommand.Path)
}
$Root = (Resolve-Path $Root).Path

$armclang = 'C:\Keil_v5\ARM\ARMCLANG\bin\armclang.exe'
if (-not (Test-Path $armclang)) {
    Write-Error "找不到 armclang: $armclang"
    exit 2
}

# 与 Keil 工程里的 IncludePath / Define 保持一致；
# STM32F10X_MD 平时由 Keil 按器件型号自动定义，命令行下要显式补上。
$commonArgs = @(
    '--target=arm-arm-none-eabi'
    '-mcpu=cortex-m3'
    '-fsyntax-only'
    '-Wno-pragma-once-outside-header'
    "-I$Root\include"
    "-I$Root\platform\Start"
    "-I$Root\platform\User"
    "-I$Root\platform\Library"
    "-I$Root\platform\System"
    "-I$Root\TPL\ETL\include"
    "-I$Root\TPL"
    '-DUSE_STDPERIPH_DRIVER'
    '-DSTM32F10X_MD'
    '-DSTM32_DEVICE_HEADER=stm32f10x.h'
    '-DETL_PROFILE=my_etl_profile.h'
)

# C++ 专用参数（.c 文件不能带 -std=c++17）
$cppArgs = @('-std=c++17', '-xc++')
$cArgs = @('-std=c11', '-xc')

$targets = @()
if (-not $SrcOnly) {
    $targets += Get-ChildItem -Recurse -Path "$Root\include\EMBMartin" -Filter *.h |
        Sort-Object FullName
}
if (-not $HeadersOnly) {
    $targets += Get-ChildItem -Recurse -Path "$Root\src" -Include *.cpp, *.c |
        Sort-Object FullName
}
if ($Only) {
    $targets = $targets | Where-Object { $_.FullName -like $Only }
}

if (-not $targets) {
    Write-Error "没有匹配到任何文件"
    exit 2
}

$failed = @()
$passed = 0
$sw = [System.Diagnostics.Stopwatch]::StartNew()

foreach ($f in $targets) {
    $rel = $f.FullName.Replace("$Root\", '')
    $extra = if ($f.Extension -eq '.c') { $cArgs } else { $cppArgs }
    $out = & $armclang @commonArgs @extra $f.FullName 2>&1
    $code = $LASTEXITCODE

    if ($code -ne 0) {
        $failed += [pscustomobject]@{ File = $rel; Output = $out }
        Write-Host "[FAIL] $rel" -ForegroundColor Red
    }
    else {
        $passed++
        if ($VerbosePreference -eq 'Continue') {
            Write-Host "[ OK ] $rel" -ForegroundColor DarkGray
        }
    }
}

$sw.Stop()

Write-Host ''
Write-Host ('=' * 70)
Write-Host ("合计 {0} 个文件：通过 {1}，失败 {2}，耗时 {3:N1} 秒" -f `
        $targets.Count, $passed, $failed.Count, $sw.Elapsed.TotalSeconds)

if ($failed.Count -gt 0) {
    Write-Host ''
    foreach ($x in $failed) {
        Write-Host ('-' * 70) -ForegroundColor Red
        Write-Host $x.File -ForegroundColor Red
        $x.Output | Where-Object { $_ -match 'error|warning' } |
            Select-Object -First 12 | ForEach-Object { Write-Host "    $_" }
    }
    exit 1
}

exit 0
