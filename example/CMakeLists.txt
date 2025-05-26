cmake_minimum_required(VERSION 3.7)

include_directories(${CMAKE_SOURCE_DIR}/inc/ 
                    ${CMAKE_SOURCE_DIR}/src/vector/
                    ${CMAKE_SOURCE_DIR}
                    ${CMAKE_BINARY_DIR}/
                )


add_executable(lottie2gif lottie2gif.cpp)
target_link_libraries(lottie2gif PRIVATE rlottie)

#add_executable(lottieview lottieview.cpp lottieviewtest.cpp)
#add_executable(lottieviewer lottieviewer.cpp)

#target_link_libraries(lottieview PRIVATE rlottie)
#target_link_libraries(lottieviewer PRIVATE rlottie)

add_executable(test_log test_log.cpp)
target_link_libraries(test_log PRIVATE rlottie)

if(LOTTIE_QT)
    message(STATUS "Qt5 found, building qtplayer example")
    add_executable(qtplayer qtplayer.cpp)
    
    set_target_properties(qtplayer PROPERTIES
        AUTOMOC ON
    )
    
    target_link_libraries(qtplayer PRIVATE 
            rlottie 
            Qt5::Core 
            Qt5::Widgets 
            Qt5::Gui 
        )

    add_executable(qtrender test_qt_rendering.cpp)
    target_link_libraries(qtrender PRIVATE 
            rlottie 
            Qt5::Core 
            Qt5::Widgets 
            Qt5::Gui 
        )
else()
    message(STATUS "Qt5 not found, skipping qtplayer example")
endif()

