#!/bin/bash

rm -rf build

set -e

BUILD_MODE="normal"
if [ "$1" = "asan" ]; then
    BUILD_MODE="asan"
    echo "正在使用 ASAN 模式构建项目..."
else
    echo "正在构建 MCP 项目..."
fi

PROJECT_ROOT=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
VCPKG_INSTALLED="$PROJECT_ROOT/cmake-build-debug/vcpkg_installed/x64-linux"

if [ ! -d "build" ]; then
    mkdir build
    echo "已创建构建目录"
fi

cd build

echo "正在使用 CMake 配置项目..."


if [ "$BUILD_MODE" = "asan" ]; then

    cmake .. -DCMAKE_BUILD_TYPE=Debug -DENABLE_ASAN=ON -DCMAKE_MAKE_PROGRAM=/usr/bin/make -DCMAKE_CXX_COMPILER=/usr/bin/g++ -DVCPKG_MANIFEST_MODE=OFF -DCMAKE_PREFIX_PATH="$VCPKG_INSTALLED/share" -DSQLiteCpp_DIR="$VCPKG_INSTALLED/share/sqlitecpp"
else
    cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_MAKE_PROGRAM=/usr/bin/make -DCMAKE_CXX_COMPILER=/usr/bin/g++ -DVCPKG_MANIFEST_MODE=OFF -DCMAKE_PREFIX_PATH="$VCPKG_INSTALLED/share" -DSQLiteCpp_DIR="$VCPKG_INSTALLED/share/sqlitecpp"
fi


echo "正在构建项目..."


if command -v nproc &> /dev/null; then
    JOBS=$(nproc)
elif command -v sysctl &> /dev/null; then
    JOBS=$(sysctl -n hw.ncpu)
else
    JOBS=4
fi


make -j$JOBS

echo ""
echo "构建完成！"
echo ""
echo "运行服务器: cd build/src && ./mcp_server"