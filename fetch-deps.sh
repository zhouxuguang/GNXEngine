#!/usr/bin/env bash
#
# fetch-deps.sh - Download and extract build tools for GNXEngine
# 仿照 fetch-deps.ps1 的功能实现的 Shell 版本
#
# 用法:
#   ./fetch-deps.sh [URL] [DEST_DIR]
#
# 示例:
#   ./fetch-deps.sh
#   ./fetch-deps.sh "https://github.com/zhouxuguang/engine-dep/releases/download/1.0/ispc.zip"
#   ./fetch-deps.sh "https://example.com/tools.tar.gz" "$PWD/tools"
#
# 镜像加速（国内直连 GitHub 很慢/不稳时使用）:
#   FETCH_MIRROR=ghfast   ./fetch-deps.sh          # 指定镜像名
#   FETCH_MIRROR="https://gh-proxy.com/" ./fetch-deps.sh
#   FETCH_MIRROR=""       ./fetch-deps.sh          # 禁用镜像，强制直连
#   （默认 auto：对 github.com 链接自动依次尝试内置镜像，失败后回退直连）
#
set -euo pipefail

# ---------------- 默认参数 ----------------
DEFAULT_URL="https://github.com/zhouxuguang/engine-dep/releases/download/1.0/ispc.zip"
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
DEFAULT_DEST_DIR="${SCRIPT_DIR}/buildtools"

URL="${1:-$DEFAULT_URL}"
DEST_DIR="${2:-$DEFAULT_DEST_DIR}"
MIRROR="${FETCH_MIRROR:-auto}"

# ---------------- 检测 7z 命令（p7zip: 7zz / 7z / 7za） ----------------
SZIP_CMD=""
for c in 7zz 7z 7za; do
    if command -v "${c}" >/dev/null 2>&1; then
        SZIP_CMD="${c}"
        break
    fi
done

# ---------------- 内置 GitHub 镜像表（兼容 bash 3.2，不用关联数组） ----------------
# 将镜像名解析为完整前缀；若是完整 URL 则原样返回
resolve_mirror() {
    local name="$1"
    case "${name}" in
        ghfast)   echo "https://ghfast.top/" ;;
        gh-proxy) echo "https://gh-proxy.com/" ;;
        ghproxy)  echo "https://ghproxy.net/" ;;
        ghps)     echo "https://ghps.cc/" ;;
        *)        echo "${name}" ;;
    esac
}

# 生成候选下载地址列表（按优先级，最后直连兜底）
build_candidates() {
    local candidates=()
    local m prefix
    if [[ "${MIRROR}" == "auto" ]]; then
        if [[ "${URL}" == *github.com* ]]; then
            for m in ghfast gh-proxy ghproxy; do
                candidates+=("$(resolve_mirror "$m")${URL}")
            done
        fi
        candidates+=("${URL}")          # 直连兜底
    elif [[ -z "${MIRROR}" ]]; then
        candidates+=("${URL}")          # 禁用镜像
    else
        prefix="$(resolve_mirror "${MIRROR}")"
        if [[ -n "${prefix}" && "${URL}" == *github.com* ]]; then
            candidates+=("${prefix}${URL}")
        fi
        candidates+=("${URL}")
    fi
    printf '%s\n' "${candidates[@]}"
}

# ---------------- 辅助输出函数 ----------------
echo_info()  { printf "\033[32m[*]\033[0m %s\n" "$1"; }
echo_step()  { printf "\033[36m[%s]\033[0m %s\n" "$1" "$2"; }
echo_error() { printf "\033[31m    ERROR: %s\033[0m\n" "$1" >&2; }

# 校验归档文件完整性（防止镜像返回 HTML 错误页）
verify_archive() {
    local file="$1"
    case "${ARCHIVE_NAME}" in
        *.zip)            unzip -tq "${file}" >/dev/null 2>&1 ;;
        *.7z)             "${SZIP_CMD}" t "${file}" >/dev/null 2>&1 ;;
        *.tar.gz|*.tgz)   tar -tzf "${file}" >/dev/null 2>&1 ;;
        *.tar.bz2|*.tbz2) tar -tjf "${file}" >/dev/null 2>&1 ;;
        *)                return 0 ;;
    esac
}

# ---------------- 解析归档文件名 ----------------
ARCHIVE_NAME="$(basename "${URL}")"
if [[ -z "${ARCHIVE_NAME}" ]]; then
    echo_error "无法从 URL 中解析文件名: ${URL}"
    exit 1
fi

ARCHIVE_PATH="${TMPDIR:-/tmp}/${ARCHIVE_NAME}"

echo "============================================"
echo " GNXEngine Build Tools Fetcher"
echo "============================================"
echo ""

# 创建目标目录
if [[ ! -d "${DEST_DIR}" ]]; then
    mkdir -p "${DEST_DIR}"
    echo_info "Created: ${DEST_DIR}"
fi

# ---------------- 下载（多镜像回退 + 断点续传） ----------------
CANDIDATES=()
while IFS= read -r cand; do
    CANDIDATES+=("${cand}")
done < <(build_candidates)
echo_step "1" "Downloading ${ARCHIVE_NAME} (共 ${#CANDIDATES[@]} 个候选地址) ..."

downloaded=0
for cand in "${CANDIDATES[@]}"; do
    echo "    -> ${cand}"
    # -C - 断点续传；--speed-time/--speed-limit 超时中止死连接；--retry 失败重试
    if curl -fSL --retry 3 --connect-timeout 15 \
             --speed-time 30 --speed-limit 1024 \
             -C - -o "${ARCHIVE_PATH}" "${cand}"; then
        if verify_archive "${ARCHIVE_PATH}"; then
            downloaded=1
            echo "    Done: ${ARCHIVE_PATH}"
            break
        else
            echo_error "下载内容不是有效的归档文件，尝试下一个镜像..."
            rm -f "${ARCHIVE_PATH}"
        fi
    else
        echo_error "下载失败，尝试下一个镜像..."
    fi
done

if [[ "${downloaded}" -ne 1 ]]; then
    echo_error "所有下载地址均失败: ${URL}"
    rm -f "${ARCHIVE_PATH}"
    exit 1
fi

# ---------------- 解压 ----------------
echo_step "2" "Extracting to ${DEST_DIR} ..."
case "${ARCHIVE_NAME}" in
    *.zip)
        if ! command -v unzip >/dev/null 2>&1; then
            echo_error "未找到 unzip 命令，请先安装: brew install unzip"
            rm -f "${ARCHIVE_PATH}"
            exit 1
        fi
        if ! unzip -q -o "${ARCHIVE_PATH}" -d "${DEST_DIR}"; then
            echo_error "Extraction failed: ${ARCHIVE_NAME}"
            rm -f "${ARCHIVE_PATH}"
            exit 1
        fi
        ;;
    *.7z)
        if [[ -z "${SZIP_CMD}" ]]; then
            echo_error "未找到 7z 命令，请先安装: brew install p7zip (Linux: sudo apt install p7zip-full)"
            rm -f "${ARCHIVE_PATH}"
            exit 1
        fi
        if ! "${SZIP_CMD}" x -y -o"${DEST_DIR}" "${ARCHIVE_PATH}" >/dev/null; then
            echo_error "Extraction failed: ${ARCHIVE_NAME}"
            rm -f "${ARCHIVE_PATH}"
            exit 1
        fi
        ;;
    *.tar.gz|*.tgz)
        if ! tar -xzf "${ARCHIVE_PATH}" -C "${DEST_DIR}"; then
            echo_error "Extraction failed: ${ARCHIVE_NAME}"
            rm -f "${ARCHIVE_PATH}"
            exit 1
        fi
        ;;
    *.tar.bz2|*.tbz2)
        if ! tar -xjf "${ARCHIVE_PATH}" -C "${DEST_DIR}"; then
            echo_error "Extraction failed: ${ARCHIVE_NAME}"
            rm -f "${ARCHIVE_PATH}"
            exit 1
        fi
        ;;
    *)
        echo_error "不支持的压缩格式: ${ARCHIVE_NAME} (仅支持 zip / tar.gz / tar.bz2)"
        rm -f "${ARCHIVE_PATH}"
        exit 1
        ;;
esac
echo "    Done."

# ---------------- 恢复可执行权限 ----------------
# zip 在 Windows 上打包时通常不保存 Unix 可执行位，导致解压后的工具
# （如 ispc / clang 等）出现 "Permission denied"。这里统一补上可执行权限。
echo_step "3" "Restoring execute permissions ..."
find "${DEST_DIR}" -type f -exec chmod +x {} +
echo "    Done."

# ---------------- 清理归档 ----------------
rm -f "${ARCHIVE_PATH}"
echo "    Done."

echo ""
echo "============================================"
echo " Build tools installed to: ${DEST_DIR}"
echo "============================================"
