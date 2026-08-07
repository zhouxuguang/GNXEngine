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
# 代理说明:
#   macOS 上 curl 默认不读取系统代理，脚本会自动检测系统代理（scutil --proxy）
#   并走代理下载，保证与 Windows 的 Invoke-WebRequest 一致的速度。
#   如需手动指定代理: HTTPS_PROXY=http://127.0.0.1:7897 ./fetch-deps.sh
#
set -euo pipefail

# ---------------- 默认参数 ----------------
DEFAULT_URL="https://github.com/zhouxuguang/engine-dep/releases/download/1.0/ispc.zip"
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
DEFAULT_DEST_DIR="${SCRIPT_DIR}/buildtools"

URL="${1:-$DEFAULT_URL}"
DEST_DIR="${2:-$DEFAULT_DEST_DIR}"

# ---------------- 检测 7z 命令（p7zip: 7zz / 7z / 7za） ----------------
SZIP_CMD=""
for c in 7zz 7z 7za; do
    if command -v "${c}" >/dev/null 2>&1; then
        SZIP_CMD="${c}"
        break
    fi
done

# ---------------- macOS 系统代理自动检测 ----------------
# 关键：curl 默认不读取 macOS 系统设置里的代理（GUI 配置），
# 必须通过 scutil --proxy 读取后显式传给 curl。
# 这样 Mac 上已开启代理（Clash/Surge 等）时，下载走代理，速度与 Windows 一致。
CURL_PROXY_ARGS=()
detect_system_proxy() {
    # 若用户已显式设置环境变量，优先使用（不覆盖用户选择）
    if [[ -n "${HTTPS_PROXY:-}" || -n "${https_proxy:-}" ]]; then
        return
    fi
    # 仅 macOS 需要；Linux/其他平台没有 scutil
    if [[ "$(uname)" != "Darwin" ]]; then
        return
    fi
    if ! command -v scutil >/dev/null 2>&1; then
        return
    fi

    # 读取系统代理配置（HTTPEnable/HTTPSEnable/HTTPProxy/HTTPPort）
    local proxy_info proxy_enable proxy_host proxy_port
    proxy_info="$(scutil --proxy 2>/dev/null || true)"
    proxy_enable="$(echo "${proxy_info}" | grep -i 'HTTPEnable' | awk -F: '{print $2}' | tr -d ' ')"
    proxy_host="$(echo "${proxy_info}" | grep -i 'HTTPProxy' | awk -F: '{print $2}' | tr -d ' ')"
    proxy_port="$(echo "${proxy_info}" | grep -i 'HTTPPort' | awk -F: '{print $2}' | tr -d ' ')"

    if [[ "${proxy_enable}" == "1" && -n "${proxy_host}" && -n "${proxy_port}" ]]; then
        CURL_PROXY_ARGS=(-x "http://${proxy_host}:${proxy_port}")
    fi
}
detect_system_proxy

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

# ---------------- 下载（直连 + 系统代理） ----------------
echo_step "1" "Downloading ${ARCHIVE_NAME} ..."
if [[ ${#CURL_PROXY_ARGS[@]} -gt 0 ]]; then
    echo_info "使用系统代理: ${CURL_PROXY_ARGS[*]}"
fi
echo "    -> ${URL}"

if curl -fSL --retry 3 --connect-timeout 10 \
         --speed-time 10 --speed-limit 4096 \
         "${CURL_PROXY_ARGS[@]}" \
         -o "${ARCHIVE_PATH}" "${URL}"; then
    if ! verify_archive "${ARCHIVE_PATH}"; then
        echo_error "下载内容不是有效的归档文件: ${ARCHIVE_NAME}"
        rm -f "${ARCHIVE_PATH}"
        exit 1
    fi
    echo "    Done: ${ARCHIVE_PATH}"
else
    echo_error "下载失败: ${URL}"
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
