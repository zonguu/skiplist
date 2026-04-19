#!/usr/bin/env bash
set -euo pipefail

# --------------------------------------
# 默认配置
# --------------------------------------
readonly BUILD_MODE_DEFAULT="Debug"
readonly PROJECT_DIR="$(pwd)"
readonly BUILD_DIR="${PROJECT_DIR}/build"
readonly JOBS_DEFAULT="$(nproc 2>/dev/null || echo 4)"

BUILD_MODE="${BUILD_MODE_DEFAULT}"
WITH_TEST="OFF"
WITH_ASAN="OFF"
JOBS="${JOBS_DEFAULT}"
CLEAN=false

# --------------------------------------
# 帮助文档
# --------------------------------------
usage() {
    echo "用法: $0 [选项]"
    echo ""
    echo "选项:"
    echo "  --mode=<mode>       编译模式 (Debug/Release), 默认: ${BUILD_MODE_DEFAULT}"
    echo "  --test=<0|1|on|off> 是否编译测试代码, 默认: 0"
    echo "  --asan=<0|1|on|off> 启用 AddressSanitizer, 默认: 0"
    echo "  -j <jobs>          编译任务数, 默认: ${JOBS_DEFAULT}"
    echo "  --clean            清理构建文件"
    echo "  -h, --help         显示帮助信息"
    exit ${1:-1}
}

# --------------------------------------
# 错误处理
# --------------------------------------
die() {
    echo "错误: $*" >&2
    exit 1
}

normalize_bool() {
    local value="${1:-}"
    case "${value,,}" in
        1|on|yes|true)
            echo "ON"
            ;;
        0|off|no|false|"")
            echo "OFF"
            ;;
        *)
            die "无效布尔值: ${value}. 预期 1/0/on/off/yes/no/true/false"
            ;;
    esac
}

validate_jobs() {
    local value="${1:-}"
    if ! [[ "${value}" =~ ^[1-9][0-9]*$ ]]; then
        die "无效并行任务数: ${value}"
    fi
}

# --------------------------------------
# 解析命令行参数
# --------------------------------------
while [[ $# -gt 0 ]]; do
    case "$1" in
        --mode=*)
            BUILD_MODE="${1#*=}"
            shift
            ;;
        --test=*)
            WITH_TEST="$(normalize_bool "${1#*=}")"
            shift
            ;;
        --asan=*)
            WITH_ASAN="$(normalize_bool "${1#*=}")"
            shift
            ;;
        -j)
            JOBS="${2:-}"
            shift 2
            ;;
        --clean)
            CLEAN=true
            shift
            ;;
        -h|--help)
            usage 0
            ;;
        *)
            die "未知选项: $1"
            ;;
    esac
done

# --------------------------------------
# 清理构建文件
# --------------------------------------
if [[ "${CLEAN}" == true ]]; then
    echo ">> 清理构建文件..."
    rm -rf "${BUILD_DIR}"
    exit 0
fi

validate_jobs "${JOBS}"

# --------------------------------------
# 创建构建目录并进入
# --------------------------------------
mkdir -p "${BUILD_DIR}"
cd "${BUILD_DIR}"

# --------------------------------------
# 调用 CMake 生成构建系统
# --------------------------------------
echo ">> 生成构建配置..."
echo "   MODE:       ${BUILD_MODE}"
echo "   WITH_TEST:  ${WITH_TEST}"
echo "   WITH_ASAN:  ${WITH_ASAN}"
echo "   JOBS:       ${JOBS}"

cmake \
    -S "${PROJECT_DIR}" \
    -B "${BUILD_DIR}" \
    -DCMAKE_BUILD_TYPE="${BUILD_MODE}" \
    -DWITH_TEST="${WITH_TEST}" \
    -DWITH_ASAN="${WITH_ASAN}"

# --------------------------------------
# 编译
# --------------------------------------
echo ">> 开始编译..."
cmake --build . -- -j "${JOBS}"

# --------------------------------------
# 返回项目根目录
# --------------------------------------
cd "${PROJECT_DIR}"
echo ">> 编译完成，输出目录: ${PROJECT_DIR}/output"
