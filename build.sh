#!/bin/bash

# --------------------------------------
# 默认配置
# --------------------------------------
BUILD_MODE="Debug"                 # 默认编译模式 (Debug/Release)
DEFAULT_TARGET="skip_list"          # 默认目标名称
SOURCE_DIR="."                       # CMakeLists.txt 所在目录
PROJECT_DIR=$(pwd)
BUILD_DIR="${PROJECT_DIR}/build"                    # 构建目录
JOBS=1
WITH_TEST=OFF
WITH_ASAN=OFF


echo "项目目录: ${PROJECT_DIR}"
# --------------------------------------
# 帮助文档
# --------------------------------------
usage() {
    echo "用法: $0 [选项]"
    echo "选项:"
    echo "  -m <BUILD_MODE>      编译模式 (Debug/Release), 默认: Debug"
    echo "  -t <test>       是否编译测试代码, 默认: OFF"
    echo "  -j <jobs>       编译任务数, 默认: 1"
    echo "  -asan           启用 AddressSanitizer"
    echo "  -h              显示帮助信息"
    echo "  --clean         清理构建文件"
    exit 1
}

# --------------------------------------
# 解析命令行参数
# --------------------------------------
while [[ $# -gt 0 ]]; do
    case "$1" in
        -m)
            BUILD_MODE="$2"
            shift 2
            ;;
        -t)
            WITH_TEST="$2"
            shift 2
            ;;
        -a)
            WITH_ASAN="$2"
            shift 2
            ;;
        -j)
            JOBS="$2"
            shift 2
            ;;
        --clean)
            CLEAN=true
            shift
            ;;
        -h|--help)
            usage
            ;;
        *)
            echo "错误: 未知选项 $1"
            usage
            ;;
    esac
done


# --------------------------------------
# 清理构建文件
# --------------------------------------
if [[ "$CLEAN" == true ]]; then
    echo ">> 清理构建文件..."
    rm -rf ${BUILD_DIR}
    exit 0
fi

# --------------------------------------
# 创建构建目录并进入
# --------------------------------------
mkdir -p ${BUILD_DIR}
cd ${BUILD_DIR}

# 如果WITH_ASAN为1，则设置环境变量
if [[ "${WITH_ASAN}" -eq 1 ]]; then
    WITH_ASAN=ON
fi

# --------------------------------------
# 调用 CMake 生成构建系统
# --------------------------------------
echo ">> 生成构建配置..."
cmake \
    -S ${PROJECT_DIR} \
    -B ${BUILD_DIR} \
    -DCMAKE_BUILD_TYPE=${BUILD_MODE} \
    -DWITH_TEST=${WITH_TEST} \
    -DWITH_ASAN=${WITH_ASAN}
    echo ">> 开始编译..."
    cmake --build . -- -j"${JOBS}"

if [[ $? -ne 0 ]]; then
    echo ">> CMake 配置失败！"
fi

# --------------------------------------
# 返回项目根目录
# --------------------------------------
cd ..