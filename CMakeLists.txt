cmake_minimum_required( VERSION 3.7)

#declare project
project( rlottie VERSION 0.2 LANGUAGES C CXX ASM)

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE MinSizeRel)
endif()

if (NOT DEFINED BUILD_SHARED_LIBS)
    # Keep the previous behavior of the build system, consistent with Meson.
    set(BUILD_SHARED_LIBS ON)
endif()

#declare target
add_library( rlottie )
set_target_properties( rlottie PROPERTIES DEFINE_SYMBOL RLOTTIE_BUILD )

#declare version of the target
set(player_version_major 0)
set(player_version_minor 2)
set(player_version ${player_version_major}.${player_version_minor})
set_target_properties(rlottie PROPERTIES
                        VERSION    ${player_version}
                        SOVERSION  ${player_version_major}
                      )

#declare alias so that library can be used inside the build tree, e.g. when testing
add_library(rlottie::rlottie ALIAS rlottie)

option(LOTTIE_MODULE "Enable LOTTIE MODULE SUPPORT" ON)
option(LOTTIE_THREAD "Enable LOTTIE THREAD SUPPORT" OFF)
option(LOTTIE_CACHE "Enable LOTTIE CACHE SUPPORT" ON)
option(LOTTIE_TEST "Build LOTTIE AUTOTESTS" OFF)
option(LOTTIE_CCACHE "Enable LOTTIE ccache SUPPORT" OFF)
option(LOTTIE_ASAN "Compile with asan" OFF)
option(LOTTIE_QT "Enable Qt Rendering Backend" ON)

set(LOTTIE_MODULE_PATH "${CMAKE_SHARED_LIBRARY_PREFIX}rlottie-image-loader${CMAKE_SHARED_LIBRARY_SUFFIX}"
    CACHE STRING "Absolute or relative path to dynamic loader plugin.")

configure_file(${CMAKE_CURRENT_LIST_DIR}/cmake/config.h.in ${CMAKE_CURRENT_BINARY_DIR}/config.h)

# 将配置目录添加到所有目标的包含路径中
# include_directories(${CMAKE_CURRENT_BINARY_DIR})

target_include_directories(rlottie
    PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/inc>
    PRIVATE
        "${CMAKE_CURRENT_BINARY_DIR}"
        "${CMAKE_CURRENT_SOURCE_DIR}/src"
    )

#declare common target compilation options
if(MSVC)
    target_compile_options(rlottie
                        PUBLIC
                        PRIVATE
                            /std:c++14
                            /EHs-c- # disable exceptions
                            /GR- # disable RTTI
                            /W3
                        )
else()
    target_compile_options(rlottie
                        PUBLIC
                        PRIVATE
                            -std=c++14
                            -fno-exceptions
                            -fno-unwind-tables
                            -fno-asynchronous-unwind-tables
                            -fno-rtti
                            -Wall
                            -fvisibility=hidden
                            -Wnon-virtual-dtor
                            -Woverloaded-virtual
                            -Wno-unused-parameter
                        )
endif()

if (WIN32 AND NOT BUILD_SHARED_LIBS)
    target_compile_definitions(rlottie PUBLIC -DRLOTTIE_BUILD=0)
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MTd")
endif()

#declare dependancy
set( CMAKE_THREAD_PREFER_PTHREAD TRUE )
find_package( Threads )

target_link_libraries(rlottie
                    PUBLIC
                        "${CMAKE_THREAD_LIBS_INIT}"
                     )

if (NOT APPLE AND NOT WIN32)
    target_link_libraries(rlottie
                        PRIVATE
                            "-Wl,--version-script=${CMAKE_CURRENT_SOURCE_DIR}/rlottie.expmap"
                          )
endif()

if (LOTTIE_MODULE)
    # for dlopen, dlsym and dlclose dependancy
    target_link_libraries(rlottie PRIVATE ${CMAKE_DL_LIBS})
endif()

if (NOT LOTTIE_ASAN)
    if(APPLE)
        target_link_libraries(rlottie
                            PUBLIC
                                 "-Wl, -undefined error"
                              )
    elseif(NOT MSVC)
        target_link_libraries(rlottie
                            PUBLIC
                                 "-Wl,--no-undefined"
                              )
    endif()
endif()

if (LOTTIE_CCACHE)
    find_program(CCACHE_FOUND ccache)
    if (CCACHE_FOUND)
        message(STATUS "Found ccache")
        set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ccache)
        set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK ccache)
    else()
        message(STATUS "Could NOT find ccache (this is NOT an error)")
    endif()
endif()

if (LOTTIE_ASAN)
    target_compile_options(rlottie PUBLIC -fsanitize=address)
    target_link_options(rlottie PUBLIC  -fsanitize=address)
endif()

if (NOT LIB_INSTALL_DIR)
    set (LIB_INSTALL_DIR "/usr/lib")
endif (NOT LIB_INSTALL_DIR)

#declare source and include files
add_subdirectory(inc)
add_subdirectory(src)

# 静态链接图像加载模块
# if (TARGET rlottie-image-loader)
#     target_link_libraries(rlottie PRIVATE rlottie-image-loader)
# endif()

# 如果指定了模块加载方式，需要静态链接image-loader
# if (NOT LOTTIE_MODULE)
#     target_sources(rlottie PRIVATE src/vector/stb/stb_image.cpp)
# endif()

# 如果启用了Qt渲染后端，添加Qt依赖
if (LOTTIE_QT)
    message(STATUS "Configuring Qt rendering backend")
    
    # 设置Qt的路径
    set(CMAKE_PREFIX_PATH "/home/lecheng/Qt5.14.2/5.14.2/gcc_64/lib/cmake" ${CMAKE_PREFIX_PATH})
    
    # 查找Qt5，添加Widgets组件
    find_package(Qt5 COMPONENTS Core Gui Widgets QUIET)
    
    if (Qt5_FOUND)
        message(STATUS "Found Qt5: ${Qt5_DIR}")
        
        # 添加Qt头文件
        target_include_directories(rlottie PRIVATE 
            ${Qt5Core_INCLUDE_DIRS}
            ${Qt5Gui_INCLUDE_DIRS}
            ${Qt5Widgets_INCLUDE_DIRS}
        )
        
        # 显式添加Qt渲染器源文件
        target_sources(rlottie PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/src/vector/vpainter_qt.cpp"
        )
        
        # 链接Qt库，添加Widgets
        target_link_libraries(rlottie PRIVATE Qt5::Core Qt5::Gui Qt5::Widgets)
    else()
        message(WARNING "Qt5 not found, disabling Qt rendering backend")
        set(LOTTIE_QT OFF CACHE BOOL "Enable Qt Rendering Backend" FORCE)
        
        # 重新配置config.h确保禁用Qt后端
        configure_file(${CMAKE_CURRENT_LIST_DIR}/cmake/config.h.in ${CMAKE_CURRENT_BINARY_DIR}/config.h)
    endif()
endif()

add_subdirectory(example)

if (LOTTIE_TEST)
    enable_testing()
    add_subdirectory(test)
endif()

SET(PREFIX ${CMAKE_INSTALL_PREFIX})
SET(EXEC_DIR ${PREFIX})
SET(LIBDIR ${LIB_INSTALL_DIR})
SET(INCDIR ${PREFIX}/include)

CONFIGURE_FILE(${PROJECT_NAME}.pc.in ${PROJECT_NAME}.pc)
INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.pc DESTINATION ${LIB_INSTALL_DIR}/pkgconfig)


#install header
install(FILES
        inc/rlottie.h
        inc/rlottie_capi.h
        inc/rlottiecommon.h
        DESTINATION include)

#install lib
install( TARGETS rlottie EXPORT rlottie-targets
         LIBRARY     DESTINATION    ${LIB_INSTALL_DIR}
         ARCHIVE     DESTINATION    ${LIB_INSTALL_DIR}
         INCLUDES    DESTINATION    include
       )

# 将rlottie-image-loader也添加到导出集合中
#if(TARGET rlottie-image-loader)
#    install(TARGETS rlottie-image-loader EXPORT rlottie-targets
#            LIBRARY     DESTINATION    ${LIB_INSTALL_DIR}
#            ARCHIVE     DESTINATION    ${LIB_INSTALL_DIR}
#           )
#endif()

#install config file.

install( EXPORT rlottie-targets
         FILE          rlottieTargets.cmake
         NAMESPACE     rlottie::
         DESTINATION   ${LIB_INSTALL_DIR}/cmake/rlottie
       )


#Create a ConfigVersion.cmake file
include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/rlottieConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY AnyNewerVersion
)

configure_package_config_file(${CMAKE_CURRENT_LIST_DIR}/cmake/rlottieConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/rlottieConfig.cmake
    INSTALL_DESTINATION ${LIB_INSTALL_DIR}/cmake/rlottie
)

#Install the config, configversion and custom find modules
install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/rlottieConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/rlottieConfigVersion.cmake
    DESTINATION ${LIB_INSTALL_DIR}/cmake/rlottie
)


export(EXPORT rlottie-targets FILE ${CMAKE_CURRENT_BINARY_DIR}/rlottieTargets.cmake NAMESPACE rlottie::)

#Register package in user's package registry
export(PACKAGE rlottie)
