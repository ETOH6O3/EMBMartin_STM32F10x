#Requires -Version 5.1
<#
.SYNOPSIS
    把例程 / 测试里的 using namespace 块改成按层显式引入。

.DESCRIPTION
    旧写法：
        using namespace std;
        using namespace EMBMartin;
        using namespace STM32;
        （martin_work 还多一行 using namespace sys;）

    新写法（只写该文件真正用到的层）：
        using namespace std;
        using namespace EMBMartin::lang;
        using namespace EMBMartin::hal;
        using namespace EMBMartin::device;
        ...

    层的判定依据是该文件里是否出现该层特有的标识符。

.PARAMETER Root
    仓库根目录。

.PARAMETER DryRun
    只报告。

.EXAMPLE
    .\ns-examples.ps1 -DryRun
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

# 各层的代表性标识符
$layerNames = [ordered]@{
    'lang'      = @('String', 'StringViewArray', 'OutStream', 'InStream', 'IOStream', 'fixed',
        'pipe', 'Slice', 'Range', 'TupleView', 'format_to', 'formatted_size', 'LogLevel')
    'hal'       = @('GPIOPin', 'OutPin', 'Inpin', 'USARTPins', 'USART', 'I2C', 'PWM',
        'OuterTimer', 'TIMCounter', 'InnerTimerManager', 'Delayer', 'IRQGuard',
        'delay_ms', 'delay_us', 'delay_s', 'EXTIManager', 'null_pin',
        'PA0', 'PA1', 'PA2', 'PA3', 'PA4', 'PA5', 'PA6', 'PA7', 'PA8', 'PA9', 'PA10', 'PA11', 'PA12', 'PA13', 'PA14', 'PA15',
        'PB0', 'PB1', 'PB2', 'PB3', 'PB4', 'PB5', 'PB6', 'PB7', 'PB8', 'PB9', 'PB10', 'PB11', 'PB12', 'PB13', 'PB14', 'PB15',
        'PC13', 'PC14', 'PC15')
    'device'    = @('LED', 'Buzzer', 'Key', 'Sensor', 'RotaryEncoder', 'EXTIRotaryEncoder',
        'CounterSensor', 'SteeringEngine', 'DCMotorDriver', 'USBConsole', 'BluetoothConsole',
        'BreathingLED', 'OLED', 'OLEDConsole', 'MPU6050', 'UltrasonicSensor',
        'MartinDebugger', 'JiangXieDebugger', 'Debugger')
    'algorithm' = @('Coordinate', 'SingleLoopPID', 'BalancedCarPID', 'PIDParams',
        'AngleComplementaryFilter', 'IterableUInt', 'add_result', 'sub_result',
        'mul_result', 'div_result', 'split_float', 'split_double', 'ceil_div')
    'app'       = @('BalancedCar')
}

$files = @()
$files += Get-ChildItem -Recurse -Path "$Root\example" -Filter main.cpp
$files += Get-ChildItem -Recurse -Path "$Root\test" -Filter main.cpp
$files = $files | Sort-Object FullName -Unique

$totalFiles = 0

foreach ($f in $files) {
    $bytes = [System.IO.File]::ReadAllBytes($f.FullName)
    if ($bytes.Length -ge 3 -and $bytes[0] -eq 0xEF -and $bytes[1] -eq 0xBB -and $bytes[2] -eq 0xBF) {
        $bytes = $bytes[3..($bytes.Length - 1)]
    }
    $text = $enc.GetString($bytes)
    $rel = $f.FullName.Replace("$Root\", '')

    # 判断用到哪些层
    $used = @()
    foreach ($layer in $layerNames.Keys) {
        foreach ($n in $layerNames[$layer]) {
            if ([regex]::IsMatch($text, "(?<![\w:])\b$([regex]::Escape($n))\b")) {
                $used += $layer
                break
            }
        }
    }

    # 构造新 using 块
    $newLines = @('using namespace std;')
    foreach ($layer in @('lang', 'hal', 'device', 'algorithm', 'app')) {
        if ($used -contains $layer) {
            $newLines += "using namespace EMBMartin::$layer;"
        }
    }
    $newBlock = $newLines -join "`r`n"

    # 保留原文件的换行风格：例程里 LF / CRLF 都有，不能统一改
    $nl = if ($text.Contains("`r`n")) { "`r`n" } else { "`n" }

    # 替换旧的 using 块（连续若干行 using namespace ...）
    $lines = $text -split "`r`n|`n", 0
    $first = -1
    $last = -1
    for ($i = 0; $i -lt $lines.Count; $i++) {
        if ($lines[$i] -match '^\s*using namespace\s+(std|EMBMartin|STM32|sys)\s*;') {
            if ($first -lt 0) { $first = $i }
            $last = $i
        }
    }

    if ($first -lt 0) {
        Write-Host "[SKIP] $rel  （没有 using 块）" -ForegroundColor DarkGray
        continue
    }

    # 保留旧块最后一行之后的空行结构：用新块替换 [first..last]
    $before = if ($first -gt 0) { $lines[0..($first - 1)] } else { @() }
    $after = if ($last -lt $lines.Count - 1) { $lines[($last + 1)..($lines.Count - 1)] } else { @() }

    $newAll = @()
    $newAll += $before
    $newAll += $newLines
    $newAll += $after

    $out = $newAll -join $nl

    if ($out -ne $text) {
        $totalFiles++
        Write-Host ("[{0}] {1,-46} -> {2}" -f $(if ($DryRun) { 'DRY' } else { 'EDIT' }), $rel, ($used -join ','))

        if (-not $DryRun) {
            [System.IO.File]::WriteAllText($f.FullName, $out, $enc)
        }
    }
}

Write-Host ''
Write-Host ("改动 {0} 个文件{1}" -f $totalFiles, $(if ($DryRun) { '（DRY RUN）' } else { '' }))
exit 0
