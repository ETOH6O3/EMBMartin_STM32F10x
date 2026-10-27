#Requires -Version 5.1
<#
.SYNOPSIS
    命名空间分层重构第二步：给跨层引用的标识符补上限定前缀。

.DESCRIPTION
    映射表直接写在脚本里（避免命令行传长列表时的绑定问题）。

    处理规则：
      - 跳过 #include 行
      - 跳过以 /* 或 * 开头的注释行；行尾 // 注释原样保留
      - 只匹配完整标识符，且前面不是 \w 或 :（已经是 hal::PWM 的不再重复加）

.PARAMETER Root
    仓库根目录。

.PARAMETER Stage
    执行哪个阶段（见 $Stages 表）。

.PARAMETER DryRun
    只报告。

.EXAMPLE
    .\ns-qualify.ps1 -Stage device-hal -DryRun
#>
[CmdletBinding()]
param(
    [string]$Root,
    [Parameter(Mandatory = $true)][string]$Stage,
    [switch]$DryRun
)

$ErrorActionPreference = 'Stop'

if (-not $Root) {
    $Root = Split-Path -Parent (Split-Path -Parent $MyInvocation.MyCommand.Path)
}
$Root = (Resolve-Path $Root).Path

# 阶段定义：Layer = 被处理的层；Prefix = 补的前缀；Names = 需要加前缀的标识符
$Stages = @{
    # device 层引用 hal 的类型
    'device-hal'     = @{
        Layer  = 'device'
        Prefix = 'hal'
        Names  = @(
            'GPIOPin', 'USARTPins', 'OutPin', 'Inpin', 'IQRHandlerFunc',
            'Get_EXTI_Line', 'Get_GPIO_PinSource', 'Get_GPIO_PortSource',
            'Get_RCC_APB1Periph', 'Get_RCC_APB2Periph', 'Get_TIM_Index',
            'Get_USART_Index', 'Get_USART_IRQChannel', 'get_TIM_RCC_APB1Periph',
            'EXTIManager', 'I2C', 'PWM', 'OuterTimer', 'TIMCounter', 'USART',
            'InnerTimerManager', 'Delayer', 'IRQGuard', 'delay_ms', 'delay_us',
            'null_pin'
        )
    }
    # device 层引用 lang 的类型
    'device-lang'    = @{
        Layer  = 'device'
        Prefix = 'lang'
        Names  = @(
            'OutStream', 'InStream', 'IOStream', 'String', 'StringViewArray',
            'has_iterator_v', 'remove_cvref_t', 'remove_cvref', 'fixed'
        )
    }
    # device 层引用 algorithm 的类型
    'device-algorithm' = @{
        Layer  = 'device'
        Prefix = 'algorithm'
        Names  = @('Coordinate', 'ceil_div', 'add_result', 'sub_result', 'mul_result', 'div_result', 'split_float', 'split_double', 'IterableUInt')
    }
    # algorithm 层引用 device 的类型
    'algorithm-device' = @{
        Layer  = 'algorithm'
        Prefix = 'device'
        Names  = @('MPU6050')
    }
    # algorithm 层引用 hal 的类型
    'algorithm-hal'  = @{
        Layer  = 'algorithm'
        Prefix = 'hal'
        Names  = @('GPIOPin', 'I2C', 'PWM', 'OutPin', 'Inpin', 'USART', 'InnerTimerManager', 'Delayer', 'IRQGuard', 'delay_ms', 'delay_us')
    }
    # algorithm 层引用 lang 的类型
    'algorithm-lang' = @{
        Layer  = 'algorithm'
        Prefix = 'lang'
        Names  = @('OutStream', 'InStream', 'IOStream', 'String', 'has_iterator_v', 'remove_cvref_t', 'remove_cvref', 'fixed')
    }
    # app 层引用各层的类型
    'app-all'        = @{
        Layer  = 'app'
        Prefix = ''      # 特殊：见下方 AppMap
        Names  = @()
    }
    # hal 层引用 lang 的类型
    'hal-lang'       = @{
        Layer  = 'hal'
        Prefix = 'lang'
        Names  = @('OutStream', 'InStream', 'IOStream', 'String', 'has_iterator_v', 'remove_cvref_t', 'remove_cvref', 'fixed', 'enum_name', 'type_name')
    }
}

if (-not $Stages.ContainsKey($Stage)) {
    Write-Error "未知阶段: $Stage（可用: $($Stages.Keys -join ', ')）"
    exit 2
}

$cfg = $Stages[$Stage]
$Layer = $cfg.Layer
$Prefix = $cfg.Prefix
$Names = $cfg.Names

$enc = New-Object System.Text.UTF8Encoding($false)

$files = @()
$files += Get-ChildItem -Recurse -Path "$Root\include\EMBMartin\$Layer" -Include *.h -ErrorAction SilentlyContinue
$files += Get-ChildItem -Recurse -Path "$Root\src\$Layer" -Include *.cpp, *.c -ErrorAction SilentlyContinue
$files = $files | Sort-Object FullName -Unique

$totalFiles = 0
$totalHits = 0

foreach ($f in $files) {
    $bytes = [System.IO.File]::ReadAllBytes($f.FullName)
    if ($bytes.Length -ge 3 -and $bytes[0] -eq 0xEF -and $bytes[1] -eq 0xBB -and $bytes[2] -eq 0xBF) {
        $bytes = $bytes[3..($bytes.Length - 1)]
    }
    $text = $enc.GetString($bytes)

    $lines = $text -split "`r`n", 0
    $changed = $false
    $fileHits = 0

    for ($i = 0; $i -lt $lines.Count; $i++) {
        $line = $lines[$i]

        if ($line -match '^\s*#\s*include') { continue }

        $code = $line
        $comment = ''
        $idx = $line.IndexOf('//')
        if ($idx -ge 0) {
            $code = $line.Substring(0, $idx)
            $comment = $line.Substring($idx)
        }

        if ($code -match '^\s*(/\*|\*)') { continue }
        if ($code.Trim().Length -eq 0) { continue }

        $newCode = $code
        foreach ($n in $Names) {
            $pat = '(?<![\w:])' + [regex]::Escape($n) + '\b'
            $c = ([regex]::Matches($newCode, $pat)).Count
            if ($c -gt 0) {
                $newCode = [regex]::Replace($newCode, $pat, "$Prefix::$n")
                $fileHits += $c
            }
        }

        if ($newCode -ne $code) {
            $lines[$i] = $newCode + $comment
            $changed = $true
        }
    }

    if ($changed) {
        $totalFiles++
        $totalHits += $fileHits
        Write-Host ("[{0}] {1,-52} ({2})" -f `
            $(if ($DryRun) { 'DRY' } else { 'EDIT' }), $f.FullName.Replace("$Root\", ''), $fileHits)

        if (-not $DryRun) {
            $out = ($lines -join "`r`n")
            [System.IO.File]::WriteAllText($f.FullName, $out, $enc)
        }
    }
}

Write-Host ''
Write-Host ("[{0}] {1} 层 -> {2}::*   改动 {3} 个文件，{4} 处{5}" -f $Stage, $Layer, $Prefix,
    $totalFiles, $totalHits, $(if ($DryRun) { '（DRY RUN）' } else { '' }))
exit 0
