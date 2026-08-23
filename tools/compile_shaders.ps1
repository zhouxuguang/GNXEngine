# ============================================================
# compile_shaders.ps1 — 批量离线编译 GNXEngine 内置 shader（Windows）
#
# 对应 macOS/Linux 的 tools/compile_shaders.sh，功能完全一致：
#   遍历 Engine/Shader/built-in/ 下所有 .shader（含子目录），
#   对每个 shader 检测其入口函数阶段（VS/PS/CS/TS/MS），
#   为每个存在的阶段 × 目标平台格式生成 .gnxasset 到 data_asset/Shader/。
#
# 用法:
#   .\tools\compile_shaders.ps1                  # 编译全部格式（默认 spirv）
#   .\tools\compile_shaders.ps1 -Format spirv    # 只编译 SPIR-V（Windows/Vulkan）
#   .\tools\compile_shaders.ps1 -Format dxil     # 只编译 DXIL
#   .\tools\compile_shaders.ps1 -Config Release  # 使用 Release 构建的工具
#
# 前置条件:
#   1. 已构建 shader_compile 工具:
#        cmake --build build --config Debug --target shader_compile
#   2. GNXEngine 是 SHARED 库（GNXEngine.dll / GNXEngined.dll），
#      且 ShaderCompiler 链接了 dxcompiler.dll，脚本会自动把这些目录加入 PATH。
# ============================================================

[CmdletBinding()]
param(
    # 目标格式（可多个，用逗号分隔）: spirv | msl_ios | msl_macos | dxil | glsl
    [string[]]$Format = @("spirv"),

    # 构建配置: Debug | Release
    [string]$Config = "Debug",

    # 可选覆盖 shader_compile 可执行文件路径
    [string]$ShaderCompile = "",

    # 可选覆盖构建输出目录（默认 build/$Config）
    [string]$BuildDir = ""
)

$ErrorActionPreference = "Stop"

# -------------------- 路径配置 --------------------
$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$RootDir   = Split-Path -Parent $ScriptDir
$BuiltinShaderDir = Join-Path $RootDir "Engine/Shader/built-in"
$OutputRoot       = Join-Path $RootDir "data_asset/Shader"

# -------------------- 工具定位 --------------------
if (-not $BuildDir) { $BuildDir = Join-Path $RootDir "build/$Config" }

if (-not $ShaderCompile) {
    $ShaderCompile = Join-Path $BuildDir "shader_compile.exe"
    # 与 .sh 一致：优先指定配置，找不到回退 build/Debug
    if (-not (Test-Path $ShaderCompile)) {
        $ShaderCompile = Join-Path $RootDir "build/Debug/shader_compile.exe"
    }
}

# -------------------- 前置检查 --------------------
if (-not (Test-Path $ShaderCompile)) {
    Write-Host "错误: 找不到 shader_compile: $ShaderCompile" -ForegroundColor Red
    Write-Host "请先构建: cmake --build build --config Debug --target shader_compile" -ForegroundColor Red
    exit 1
}

if (-not (Test-Path $BuiltinShaderDir)) {
    Write-Host "错误: 找不到内置 shader 目录: $BuiltinShaderDir" -ForegroundColor Red
    exit 1
}

# -------------------- DLL 环境（Windows 特有） --------------------
# GNXEngine 是 SHARED 库：shader_compile.exe 需要 GNXEngine.dll / GNXEngined.dll；
# ShaderCompiler 链接 dxcompiler.dll（位于 ThirdParty/DirectXShaderCompiler/lib/win/）。
# 把这些目录加入 PATH，等价于 macOS .sh 的 DYLD_LIBRARY_PATH。
$ToolDir = Split-Path -Parent $ShaderCompile
$DllDirs = @()
if ($ToolDir -and (Test-Path $ToolDir)) { $DllDirs += $ToolDir }

$DxcWinDir = Join-Path $RootDir "ThirdParty/DirectXShaderCompiler/lib/win"
if (Test-Path $DxcWinDir) { $DllDirs += $DxcWinDir }

foreach ($dir in $DllDirs) {
    if ($env:PATH -notlike "*$dir*") {
        $env:PATH = "$dir;$env:PATH"
    }
}

# 校验关键 dll（GNXEngine 与 dxcompiler 至少其一存在；不同配置命名不同）
$gnxDll = Get-ChildItem -Path $ToolDir -Filter "GNXEngine*.dll" -ErrorAction SilentlyContinue
$dxcDll = Join-Path $DxcWinDir "dxcompiler.dll"
if (-not $gnxDll -and -not (Test-Path $dxcDll)) {
    Write-Host "错误: 找不到 GNXEngine*.dll（$ToolDir）或 dxcompiler.dll（$DxcWinDir）" -ForegroundColor Red
    Write-Host "请先构建: cmake --build build --config Debug --target shader_compile" -ForegroundColor Red
    exit 1
}

# -------------------- 阶段检测 --------------------
# 检测 .shader 文件中存在的入口函数阶段。
# 与 bash 版 grep -oE "(^|[[:space:]])VS|PS|CS|TS|MS[[:space:]]*\(" 语义一致：
# 匹配"入口标记前是行首或空白"的 VS(/PS(/CS(/TS(/MS( 标记，覆盖两种风格：
#   void CS(...                    → CS
#   VertexOut VS(appdata_tan vin)  → VS
#   VS_OUTPUT VS(VS_INPUT vin)     → VS
$StageRegex = '(?m)(?:^|\s)(VS|PS|CS|TS|MS)\s*\('

function Detect-Stages {
    param([string]$Content)
    $stages = @()
    foreach ($m in [regex]::Matches($Content, $StageRegex)) {
        if ($stages -notcontains $m.Groups[1].Value) {
            $stages += $m.Groups[1].Value
        }
    }
    return $stages
}

# -------------------- 批量编译 --------------------
$total = 0
$failed = 0
$shaderCount = 0

Write-Host "=============================================="
Write-Host " GNXEngine Shader 批量编译"
Write-Host " 源目录: $BuiltinShaderDir"
Write-Host " 输出目录: $OutputRoot"
Write-Host " 格式: $($Format -join ', ')"
Write-Host " 工具: $ShaderCompile"
Write-Host "=============================================="

New-Item -ItemType Directory -Path $OutputRoot -Force | Out-Null

$shaderFiles = Get-ChildItem -Path $BuiltinShaderDir -Recurse -Filter "*.shader" | Sort-Object FullName

foreach ($shaderFile in $shaderFiles) {
    $relPath  = $shaderFile.FullName.Substring($BuiltinShaderDir.Length + 1)
    $relDir   = Split-Path -Parent $relPath
    $shaderName = $relPath -replace '\.shader$', ''   # 含相对子目录前缀，如 "vt/GBufferVTPBR"

    $content = Get-Content -LiteralPath $shaderFile.FullName -Raw
    $stages = Detect-Stages -Content $content

    if ($stages.Count -eq 0) {
        Write-Host "  [跳过] $relPath`: 未检测到 VS/PS/CS/TS/MS 入口"
        continue
    }

    $shaderCount++

    foreach ($stage in $stages) {
        $stageLower = $stage.ToLower()
        foreach ($fmt in $Format) {
            $outFile = Join-Path $OutputRoot "$shaderName.$stageLower.$fmt.gnxasset"
            $outDir  = Split-Path -Parent $outFile
            if ($outDir -and -not (Test-Path $outDir)) {
                New-Item -ItemType Directory -Path $outDir -Force | Out-Null
            }

            Write-Host "  [编译] $shaderName [$stage/$fmt]"
            & $ShaderCompile $shaderFile.FullName -s $stageLower -f $fmt -o $outFile 2>&1 | Out-Null
            if ($LASTEXITCODE -ne 0) {
                Write-Host "    !! 失败: $relPath ($stage/$fmt)" -ForegroundColor Red
                $failed++
            } else {
                $total++
            }
        }
    }
}

Write-Host "=============================================="
Write-Host " 完成: 编译 $shaderCount 个 shader, 生成 $total 个产物, 失败 $failed"
if ($failed -gt 0) {
    Write-Host " !! 有 $failed 个编译失败，请检查日志" -ForegroundColor Red
    exit 1
}
Write-Host " 输出目录: $OutputRoot"
Write-Host "=============================================="
