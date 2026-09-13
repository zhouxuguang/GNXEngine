#!/bin/bash

# ============================================================
#  GNXEngine iOS 编译脚本（使用 CMake 原生 iOS 支持 + Ninja）
#
#  用法:
#    ./build_ios.sh                           # 默认：真机 arm64 Debug
#    ./build_ios.sh simulator                 # 模拟器 arm64
#    ./build_ios.sh -t Release                # Release 编译
#    ./build_ios.sh -t Release build          # 配置 + 编译
#    ./build_ios.sh clean                     # 清理构建目录
# ============================================================

set -e

# -------------------- 默认配置 --------------------
SYSROOT="iphoneos"                 # iphoneos（真机）/ iphonesimulator（模拟器）
ARCH="arm64"                       # arm64 / x86_64
BUILD_TYPE="Debug"                 # Debug / Release / RelWithDebInfo / MinSizeRel
DEPLOYMENT_TARGET="15.0"           # 最低 iOS 版本
BUILD_DIR="build/ios"              # 构建输出目录
ENABLE_TESTING="OFF"               # 单元测试
ENABLE_EDITOR="OFF"                # 编辑器
ENABLE_TRACY="OFF"                 # Tracy profiler
DO_BUILD="OFF"                     # configure 后是否立即编译
CLEAN_BUILD="OFF"                  # 清理后重新配置

# -------------------- 解析参数 --------------------
while [[ $# -gt 0 ]]; do
    case $1 in
        simulator|sim)
            SYSROOT="iphonesimulator"
            BUILD_DIR="build/ios-simulator"
            shift
            ;;
        device|dev)
            SYSROOT="iphoneos"
            BUILD_DIR="build/ios"
            shift
            ;;
        clean)
            CLEAN_BUILD="ON"
            shift
            ;;
        build)
            DO_BUILD="ON"
            shift
            ;;
        -s|--sdk)
            SYSROOT="$2"
            shift 2
            ;;
        -a|--arch)
            ARCH="$2"
            shift 2
            ;;
        -t|--type)
            BUILD_TYPE="$2"
            shift 2
            ;;
        -d|--deployment-target)
            DEPLOYMENT_TARGET="$2"
            shift 2
            ;;
        -o|--output)
            BUILD_DIR="$2"
            shift 2
            ;;
        --test)
            ENABLE_TESTING="ON"
            shift
            ;;
        --editor)
            ENABLE_EDITOR="ON"
            shift
            ;;
        --tracy)
            ENABLE_TRACY="ON"
            shift
            ;;
        -j|--jobs)
            JOBS="$2"
            shift 2
            ;;
        -h|--help)
            echo "用法: $0 [选项] [动作]"
            echo ""
            echo "动作:"
            echo "  (无参数)              配置 CMake（不编译）"
            echo "  simulator|sim         模拟器 (arm64)"
            echo "  device|dev            真机 (arm64) [默认]"
            echo "  build                 配置后立即编译"
            echo "  clean                 清理构建目录后重新配置"
            echo ""
            echo "选项:"
            echo "  -s, --sdk             指定 SDK (iphoneos/iphonesimulator) [默认: iphoneos]"
            echo "  -a, --arch            指定架构 (arm64/x86_64) [默认: arm64]"
            echo "  -t, --type            编译类型 (Debug/Release/RelWithDebInfo/MinSizeRel) [默认: Debug]"
            echo "  -d, --deployment-target 最低 iOS 版本 [默认: 15.0]"
            echo "  -o, --output          构建目录 [默认: build/ios]"
            echo "  -j, --jobs            并行编译任务数 [默认: CPU 核心数]"
            echo "  --test                启用单元测试"
            echo "  --editor              启用编辑器"
            echo "  --tracy               启用 Tracy profiler"
            echo ""
            echo "示例:"
            echo "  $0                              # 配置真机 Debug"
            echo "  $0 sim                          # 配置模拟器 Debug"
            echo "  $0 -t Release build             # 配置 + Release 编译真机"
            echo "  $0 sim -t Release build         # 配置 + Release 编译模拟器"
            echo "  $0 clean                        # 清理后重新配置"
            exit 0
            ;;
        *)
            echo "未知参数: $1"
            echo "使用 -h 或 --help 查看帮助"
            exit 1
            ;;
    esac
done

# -------------------- 清理 --------------------
if [[ "$CLEAN_BUILD" == "ON" ]]; then
    echo "==> 清理构建目录: $BUILD_DIR"
    rm -rf "$BUILD_DIR"
    echo "==> 清理完成"
fi

# -------------------- 检测 Ninja --------------------
if ! command -v ninja &> /dev/null; then
    echo "错误: 未找到 ninja, 请先安装: brew install ninja"
    exit 1
fi

# -------------------- 构建 CMake 参数 --------------------
CMAKE_ARGS=(
    -G Ninja
    -DCMAKE_SYSTEM_NAME=iOS
    -DCMAKE_OSX_SYSROOT="$SYSROOT"
    -DCMAKE_OSX_ARCHITECTURES="$ARCH"
    -DCMAKE_OSX_DEPLOYMENT_TARGET="$DEPLOYMENT_TARGET"
    -DCMAKE_BUILD_TYPE="$BUILD_TYPE"
    -DENABLE_TESTING="$ENABLE_TESTING"
    -DENABLE_EDITOR="$ENABLE_EDITOR"
    -DENABLE_TRACY="$ENABLE_TRACY"
    -DENABLE_NSIGHT_AFTERMATH=OFF
)

# -------------------- 打印配置信息 --------------------
echo "============================================"
echo "  GNXEngine iOS 编译 (CMake + Ninja)"
echo "============================================"
echo "  SDK:            $SYSROOT"
echo "  架构 (ARCH):     $ARCH"
echo "  iOS 最低版本:    $DEPLOYMENT_TARGET"
echo "  编译类型:        $BUILD_TYPE"
echo "  构建目录:        $BUILD_DIR"
echo "  单元测试:        $ENABLE_TESTING"
echo "  编辑器:          $ENABLE_EDITOR"
echo "  Tracy:           $ENABLE_TRACY"
echo "============================================"

# -------------------- 执行 CMake 配置 --------------------
echo ""
echo "==> CMake 配置..."

cmake "${CMAKE_ARGS[@]}" -B "$BUILD_DIR" -S .

echo ""
echo "============================================"
echo "  CMake 配置完成!"
echo "============================================"

# -------------------- 编译 --------------------
if [[ "$DO_BUILD" == "ON" ]]; then
    echo ""
    echo "==> 开始编译..."

    BUILD_CMD=(cmake --build "$BUILD_DIR")
    if [[ -n "$JOBS" ]]; then
        BUILD_CMD+=(-j "$JOBS")
    fi

    "${BUILD_CMD[@]}"

    echo ""
    echo "============================================"
    echo "  编译完成!"
    echo "============================================"
else
    echo ""
    echo "  手动编译: cmake --build $BUILD_DIR"
    echo "  或重新运行: $0 build"
fi
