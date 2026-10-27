# ---------------------------------------------------------------------------
# EMBMartin — 一键构建脚本（Windows / PowerShell）
# ---------------------------------------------------------------------------
# 用法：
#   .\build-cmake.ps1              # 默认 Release
#   .\build-cmake.ps1 -Clean       # 清掉 build 目录重来
#   .\build-cmake.ps1 -Install     # 构建后装到 ./install
#
# 为什么需要这个脚本：
#   1. 自动挑一个可用的 cmake —— 机器上若装了多个版本，避免用到坏的那个。
#   2. 自动挑一个可用的 ninja。
#   3. 路径含中文时 CMake 本身没问题，但脚本要保证以 UTF-8 传参。
# ---------------------------------------------------------------------------

[CmdletBinding()]
param(
    [string]$BuildDir  = "build",
    [string]$Prefix    = "",
    # 注意：这里不能叫 $Config——它是 PowerShell 的自动变量（默认空），
    # 会覆盖同名参数，导致 -DCMAKE_BUILD_TYPE= 传空值、ninja 规则名被污染。
    [string]$BuildType = "Release",
    [switch]$Clean,
    [switch]$Install
)

$ErrorActionPreference = 'Stop'
$Root = Split-Path -Parent $MyInvocation.MyCommand.Path

# --------------------------------------------------------------- 找 cmake
# 注意：不能用 `cmake --version` 判断可用性——能打印版本号不代表能 configure。
# 实测：本机 C:\Program Files\CMake 的 4.1.1 在检测 armclang 时必崩
# （退出码 0xC0000409），而 vcpkg 自带的 4.4.0 正常。
# 所以探测必须「启用 C/CXX 语言」并「指定真实工具链」，否则测不出来。
function Test-CMakeWorks {
    param([string]$Exe, [string]$Toolchain)
    $probe = Join-Path ([System.IO.Path]::GetTempPath()) ("cmprobe_" + [guid]::NewGuid().ToString('N'))
    try {
        New-Item -ItemType Directory $probe -Force | Out-Null
        $txt = "cmake_minimum_required(VERSION 3.24)`nproject(probe LANGUAGES C CXX)`n"
        [System.IO.File]::WriteAllText("$probe\CMakeLists.txt", $txt, (New-Object System.Text.UTF8Encoding($false)))
        'int probe(void){return 0;}' | Set-Content "$probe\probe.c" -Encoding ascii
        & $Exe -S $probe -B "$probe\b" -G Ninja "-DCMAKE_TOOLCHAIN_FILE=$Toolchain" *> $null
        return ($LASTEXITCODE -eq 0)
    } catch {
        return $false
    } finally {
        Remove-Item $probe -Recurse -Force -ErrorAction SilentlyContinue
    }
}

function Find-CMake {
    param([string]$Toolchain)
    $candidates = @()
    $onPath = Get-Command cmake -ErrorAction SilentlyContinue
    if ($onPath) { $candidates += $onPath.Source }
    $candidates += 'C:\Program Files\CMake\bin\cmake.exe'
    # vcpkg 自带的 cmake 通常版本更新，放进来作为回退
    if ($env:VCPKG_ROOT) {
        $candidates += Get-ChildItem "$env:VCPKG_ROOT\downloads\tools" -Recurse -Filter 'cmake.exe' -ErrorAction SilentlyContinue |
                       Where-Object { $_.FullName -match '\\bin\\cmake\.exe$' } |
                       Select-Object -ExpandProperty FullName
    }
    $seen = @{}
    foreach ($c in $candidates) {
        if (-not $c -or -not (Test-Path $c)) { continue }
        if ($seen.ContainsKey($c)) { continue }
        $seen[$c] = $true
        Write-Host "探测 cmake: $c" -NoNewline
        if (Test-CMakeWorks -Exe $c -Toolchain $Toolchain) {
            Write-Host "  OK"
            return $c
        }
        Write-Host "  不可用，跳过"
    }
    throw "找不到可用的 cmake。请安装 CMake，或用 -CMakePath 指定。"
}

# --------------------------------------------------------------- 找 ninja
function Find-Ninja {
    $n = Get-Command ninja -ErrorAction SilentlyContinue
    if ($n) { return $n.Source }
    if ($env:VCPKG_ROOT) {
        $v = Get-ChildItem "$env:VCPKG_ROOT\downloads\tools" -Recurse -Filter 'ninja.exe' -ErrorAction SilentlyContinue |
             Select-Object -First 1 -ExpandProperty FullName
        if ($v) { return $v }
    }
    return $null
}

$toolchain = Join-Path $Root 'cmake\toolchain-armclang.cmake'
$cmake = Find-CMake -Toolchain $toolchain
$ninja = Find-Ninja
$bd    = Join-Path $Root $BuildDir

Write-Host "cmake : $cmake"
Write-Host "ninja : $(if($ninja){$ninja}else{'(未找到，将由 CMake 自选生成器)'})"
Write-Host ""

if ($Clean -and (Test-Path $bd)) {
    Write-Host "清理 $bd ..."
    Remove-Item $bd -Recurse -Force
}

$genArgs = @()
if ($ninja) { $genArgs = @('-G', 'Ninja', "-DCMAKE_MAKE_PROGRAM=$ninja") }

& $cmake -S $Root -B $bd @genArgs "-DCMAKE_TOOLCHAIN_FILE=$toolchain" "-DCMAKE_BUILD_TYPE=$BuildType"
if ($LASTEXITCODE -ne 0) { throw "CMake 配置失败" }

& $cmake --build $bd
if ($LASTEXITCODE -ne 0) { throw "构建失败" }

if ($Install) {
    $pfx = if ($Prefix) { Join-Path $Root $Prefix } else { Join-Path $Root 'install' }
    & $cmake --install $bd --prefix $pfx
    if ($LASTEXITCODE -ne 0) { throw "安装失败" }
    Write-Host ""
    Write-Host "已安装到: $pfx"
}

Write-Host ""
Write-Host "完成。静态库位于: $bd\libEMBMartin.a"
