# 创建静态库以避免未定义符号问题
# add_library(rlottie-image-loade-static STATIC
#             stb_image.cpp
#            )
#            
# # 直接导出符号，避免被隐藏
# if(NOT MSVC)
#     target_compile_options(rlottie-image-loade-static PUBLIC
#                           -fvisibility=default
#                           )
# endif()

# 如果需要安装动态库
if(LOTTIE_MODULE)
    add_library(rlottie-image-loader SHARED
                stb_image.cpp
               )
    if(NOT MSVC)
        target_compile_options(rlottie-image-loader PRIVATE
                               -fvisibility=hidden
                              )
    endif()

    get_filename_component(LOTTIE_MODULE_FILENAME ${LOTTIE_MODULE_PATH} NAME)
    get_filename_component(LOTTIE_MODULE_DIR ${LOTTIE_MODULE_PATH} DIRECTORY)
    if (NOT LOTTIE_MODULE_DIR)
        set(LOTTIE_MODULE_DIR ${LIB_INSTALL_DIR})
    endif()

    set_target_properties(rlottie-image-loader PROPERTIES
                          DEFINE_SYMBOL RLOTTIE_BUILD
                          PREFIX ""
                          SUFFIX ""
                          OUTPUT_NAME ${LOTTIE_MODULE_FILENAME}
                         )
    install(TARGETS rlottie-image-loader
            LIBRARY DESTINATION ${LOTTIE_MODULE_DIR}
           )
else()
    target_sources(rlottie
        PRIVATE
            "${CMAKE_CURRENT_LIST_DIR}/stb_image.cpp"
           )
endif()
