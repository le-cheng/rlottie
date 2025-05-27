#!/bin/bash

# rlottie Qt示例程序启动脚本
# 用于快速编译和运行Qt渲染后端示例程序

set -e

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Qt库路径
QT_LIB_PATH="/home/lecheng/Qt5.14.2/5.14.2/gcc_64/lib"

echo -e "${BLUE}=== rlottie Qt渲染后端示例程序 ===${NC}"
echo ""

# 检查build目录
if [ ! -d "build" ]; then
    echo -e "${YELLOW}创建build目录...${NC}"
    mkdir build
fi

cd build

# 配置项目
echo -e "${YELLOW}配置项目 (Qt渲染后端)...${NC}"
cmake .. -DLOTTIE_QT=ON -DLOTTIE_THREAD=OFF

# 编译项目
echo -e "${YELLOW}编译rlottie库...${NC}"
make rlottie

echo -e "${YELLOW}编译Qt示例程序...${NC}"
make qtplayer qtperformance qtviewer qtdemo qtdemo_marker lottieperf

echo -e "${GREEN}编译完成！${NC}"
echo ""

# 进入example目录
cd example

echo -e "${BLUE}可用的示例程序：${NC}"
echo -e "  ${GREEN}1. qtplayer${NC}        - 文件夹浏览播放器"
echo -e "  ${GREEN}2. qtperformance${NC}   - 性能测试工具"
echo -e "  ${GREEN}3. qtviewer${NC}        - 文件查看器"
echo -e "  ${GREEN}4. qtdemo${NC}          - 动态属性演示"
echo -e "  ${GREEN}5. qtdemo_marker${NC}   - 标记功能演示"
echo -e "  ${GREEN}6. lottieperf${NC}      - 原始性能测试"
echo ""

# 询问用户要运行的程序
echo -e "${YELLOW}请选择要运行的程序 (1-6) 或按 Enter 退出:${NC}"
read -p "> " choice

case $choice in
    1)
        echo -e "${GREEN}启动 qtplayer...${NC}"
        LD_LIBRARY_PATH="$QT_LIB_PATH:$LD_LIBRARY_PATH" ./qtplayer
        ;;
    2)
        echo -e "${GREEN}启动 qtperformance...${NC}"
        LD_LIBRARY_PATH="$QT_LIB_PATH:$LD_LIBRARY_PATH" ./qtperformance
        ;;
    3)
        echo -e "${GREEN}启动 qtviewer...${NC}"
        LD_LIBRARY_PATH="$QT_LIB_PATH:$LD_LIBRARY_PATH" ./qtviewer
        ;;
    4)
        echo -e "${GREEN}启动 qtdemo...${NC}"
        LD_LIBRARY_PATH="$QT_LIB_PATH:$LD_LIBRARY_PATH" ./qtdemo
        ;;
    5)
        echo -e "${GREEN}启动 qtdemo_marker...${NC}"
        LD_LIBRARY_PATH="$QT_LIB_PATH:$LD_LIBRARY_PATH" ./qtdemo_marker
        ;;
    6)
        echo -e "${GREEN}启动 lottieperf...${NC}"
        ./lottieperf -c 10 -i 50
        ;;
    "")
        echo -e "${BLUE}退出。${NC}"
        ;;
    *)
        echo -e "${RED}无效选择。${NC}"
        ;;
esac

echo ""
echo -e "${BLUE}示例程序位置: $(pwd)${NC}"
echo -e "${BLUE}要手动运行程序，请使用:${NC}"
echo -e "  ${YELLOW}LD_LIBRARY_PATH=\"$QT_LIB_PATH:\$LD_LIBRARY_PATH\" ./程序名${NC}" 