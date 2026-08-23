#!/bin/bash
# ============================================================
# compile_shaders.sh — 批量离线编译 GNXEngine 内置 shader
#
# 遍历 Engine/Shader/built-in/ 下所有 .shader（含子目录），
# 对每个 shader 检测其入口函数阶段（VS/PS/CS/TS/MS），
# 为每个存在的阶段 × 目标平台格式生成 .gnxasset 到 data_asset/Shader/。
#
# 用法:
#   ./tools/compile_shaders.sh                 # 编译全部 3 种格式
#   ./tools/compile_shaders.sh macos           # 只编译 msl_macos
#   ./tools/compile_shaders.sh ios             # 只编译 msl_ios
#   ./tools/compile_shaders.sh vulkan          # 只编译 spirv
# ============================================================

set -e

# -------------------- 路径配置 --------------------
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
BUILTIN_SHADER_DIR="$ROOT_DIR/Engine/Shader/built-in"
OUTPUT_ROOT="$ROOT_DIR/data_asset/Shader"

# shader_compile 可执行文件（自动探测 build/Debug 或 build/Release）
BUILD_TYPE="${BUILD_TYPE:-Debug}"
if [ -z "$SHADER_COMPILE" ]; then
    if [ -x "$ROOT_DIR/build/$BUILD_TYPE/shader_compile" ]; then
        SHADER_COMPILE="$ROOT_DIR/build/$BUILD_TYPE/shader_compile"
    else
        SHADER_COMPILE="$ROOT_DIR/build/Debug/shader_compile"
    fi
fi
# 动态库路径（GNXEngine 是 SHARED 库）
if [ -z "$DYLIB_DIR" ]; then
    if ls "$ROOT_DIR/build/$BUILD_TYPE"/*.dylib >/dev/null 2>&1; then
        DYLIB_DIR="$ROOT_DIR/build/$BUILD_TYPE"
    else
        DYLIB_DIR="$ROOT_DIR/build/Debug"
    fi
fi

# -------------------- 参数解析 --------------------
FORMATS=""
for arg in "$@"; do
    case "$arg" in
        macos)   FORMATS="$FORMATS msl_macos" ;;
        ios)     FORMATS="$FORMATS msl_ios" ;;
        vulkan)  FORMATS="$FORMATS spirv" ;;
        *)       echo "未知目标: $arg (可选: macos / ios / vulkan)" >&2; exit 1 ;;
    esac
done
# 默认编译全部 3 种格式
if [ -z "$FORMATS" ]; then
    FORMATS="spirv msl_ios msl_macos"
fi

# -------------------- 前置检查 --------------------
if [ ! -x "$SHADER_COMPILE" ]; then
    echo "错误: 找不到 shader_compile: $SHADER_COMPILE" >&2
    echo "请先构建: cmake --build build --target shader_compile" >&2
    exit 1
fi

if [ ! -d "$DYLIB_DIR" ] || ! ls "$DYLIB_DIR"/*.dylib >/dev/null 2>&1; then
    echo "错误: 找不到动态库目录: $DYLIB_DIR" >&2
    exit 1
fi

# -------------------- 阶段检测 --------------------
# 检测 .shader 文件中存在的入口函数阶段（返回: VS PS CS TS MS 的子集）
detect_stages() {
    local file="$1"
    grep -oE "(^|[[:space:]])(VS|PS|CS|TS|MS)[[:space:]]*\(" "$file" \
        | grep -oE "VS|PS|CS|TS|MS" | sort -u
}

# -------------------- 批量编译 --------------------
total=0
failed=0
shader_count=0

echo "=============================================="
echo " GNXEngine Shader 批量编译"
echo " 源目录: $BUILTIN_SHADER_DIR"
echo " 输出目录: $OUTPUT_ROOT"
echo " 格式: $FORMATS"
echo "=============================================="

mkdir -p "$OUTPUT_ROOT"

# 递归遍历所有 .shader
while IFS= read -r shaderFile; do
    relPath="${shaderFile#$BUILTIN_SHADER_DIR/}"
    relDir="$(dirname "$relPath")"
    shaderName="${relPath%.shader}"          # 含相对子目录前缀，如 "vt/GBufferVTPBR"

    # 检测该 shader 的阶段
    stages=$(detect_stages "$shaderFile")
    if [ -z "$stages" ]; then
        echo "  [跳过] $relPath: 未检测到 VS/PS/CS/TS/MS 入口"
        continue
    fi

    shader_count=$((shader_count + 1))

    for stage in $stages; do
        # 转小写作为文件后缀（运行时 LoadCompiledShaderFromAsset 用 vs/ps/cs/ts/ms）
        stageLower=$(echo "$stage" | tr '[:upper:]' '[:lower:]')
        for format in $FORMATS; do
            outFile="$OUTPUT_ROOT/$shaderName.$stageLower.$format.gnxasset"
            outDir="$(dirname "$outFile")"
            mkdir -p "$outDir"

            # 阶段简写映射到工具参数
            case "$stage" in
                VS) stageArg="vs" ;;
                PS) stageArg="ps" ;;
                CS) stageArg="cs" ;;
                TS) stageArg="ts" ;;
                MS) stageArg="ms" ;;
            esac

            echo "  [编译] $shaderName [$stage/$format]"
            if ! DYLD_LIBRARY_PATH="$DYLIB_DIR" "$SHADER_COMPILE" \
                    "$shaderFile" -s "$stageArg" -f "$format" -o "$outFile" \
                    >/dev/null 2>&1; then
                echo "    !! 失败: $relPath ($stage/$format)" >&2
                failed=$((failed + 1))
            else
                total=$((total + 1))
            fi
        done
    done
done < <(find "$BUILTIN_SHADER_DIR" -name "*.shader" | sort)

echo "=============================================="
echo " 完成: 编译 $shader_count 个 shader, 生成 $total 个产物, 失败 $failed"
if [ "$failed" -gt 0 ]; then
    echo " !! 有 $failed 个编译失败，请检查日志" >&2
    exit 1
fi
echo " 输出目录: $OUTPUT_ROOT"
echo "=============================================="
