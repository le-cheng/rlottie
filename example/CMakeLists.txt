cmake_minimum_required(VERSION 3.7)

include_directories(${CMAKE_SOURCE_DIR}/inc/
                    ${CMAKE_SOURCE_DIR}/src/vector/
                    ${CMAKE_SOURCE_DIR}
                    ${CMAKE_BINARY_DIR}/
                )

# 设置示例文件夹路径
set(DEMO_DIR "${CMAKE_CURRENT_SOURCE_DIR}/resource/")
add_definitions(-DDEMO_DIR="${DEMO_DIR}")

# 传统lottie2gif工具 (不依赖Qt)
add_executable(lottie2gif lottie2gif.cpp)
target_link_libraries(lottie2gif PRIVATE rlottie)

# 性能测试工具 (不依赖Qt)
add_executable(lottieperf lottieperf.cpp)
target_link_libraries(lottieperf PRIVATE rlottie)

# 渲染框架演示程序
add_executable(render_framework_demo render_framework_demo.cpp)
target_link_libraries(render_framework_demo rlottie)
target_compile_definitions(render_framework_demo PRIVATE -DDEMO_DIR="${DEMO_DIR}")

# 如果启用了渲染管理器，添加相应的链接
if(LOTTIE_RENDER_MANAGER)
    target_compile_definitions(render_framework_demo PRIVATE -DLOTTIE_RENDER_MANAGER)
    target_include_directories(render_framework_demo PRIVATE ${CMAKE_SOURCE_DIR}/src/render)
endif()

if(LOTTIE_QT)
    message(STATUS "Qt5 found, building Qt-based examples")
    
    # Qt渲染器播放器
    add_executable(qtplayer qtplayer.cpp)
    set_target_properties(qtplayer PROPERTIES AUTOMOC ON)
    target_link_libraries(qtplayer PRIVATE rlottie Qt5::Core Qt5::Widgets Qt5::Gui)

    # Qt基础演示 (转换自demo.cpp)
    add_executable(qtdemo qtdemo.cpp)
    set_target_properties(qtdemo PROPERTIES AUTOMOC ON)
    target_link_libraries(qtdemo PRIVATE rlottie Qt5::Core Qt5::Widgets Qt5::Gui)

    # Qt标记演示 (转换自demo_marker.cpp)
    add_executable(qtdemo_marker qtdemo_marker.cpp)
    set_target_properties(qtdemo_marker PROPERTIES AUTOMOC ON)
    target_link_libraries(qtdemo_marker PRIVATE rlottie Qt5::Core Qt5::Widgets Qt5::Gui)

    # Qt性能测试视图 (转换自lottieviewtest.cpp)
    add_executable(qtperformance qtperformance.cpp)
    set_target_properties(qtperformance PROPERTIES AUTOMOC ON)
    target_link_libraries(qtperformance PRIVATE rlottie Qt5::Core Qt5::Widgets Qt5::Gui)

    # Qt多文件查看器 (转换自lottieviewer.cpp)
    add_executable(qtviewer qtviewer.cpp)
    set_target_properties(qtviewer PROPERTIES AUTOMOC ON)
    target_link_libraries(qtviewer PRIVATE rlottie Qt5::Core Qt5::Widgets Qt5::Gui)

    # 更多示例可以在这里添加
    # add_executable(qtux qtux.cpp)
    # add_executable(qtpath qtpath.cpp)

else()
    message(STATUS "Qt5 not found, skipping Qt-based examples")
endif() 