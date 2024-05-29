cmake_minimum_required(VERSION 3.17)

project(GNXEditor VERSION 0.1 LANGUAGES C CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)

set(PROJECT_SOURCES
        main.cpp
        mainwindow.cpp
        mainwindow.h
        mainwindow.ui
        view.mm
        view.h
        MetalView.mm
        MetalView.h
        OpenGLView.mm
        OpenGLView.h
        RenderDelegate.mm
        RenderDelegate.h
        testShader.mm
        testShader.h
        TestPost/TestPost.cpp
        TestPost/TestPost.hpp
        ResourceUtil.mm
        ResourceUtil.h
        TestImageDecode.cpp
        TestImageDecode.h
        TestComputeShader.cpp
        TestComputeShader.hpp
        testShader.h
        testShader.mm
        testSkybox.h
        testSkybox.mm
        TestTransform.cpp
        TestTransform.hpp
)

add_executable(GNXEditor
    ${PROJECT_SOURCES}
)

set_property(TARGET GNXEditor PROPERTY AUTOUIC ON)
set_property(TARGET GNXEditor PROPERTY AUTOMOC ON)
set_property(TARGET GNXEditor PROPERTY AUTORCC ON)

if(APPLE)
    target_link_libraries(GNXEditor PRIVATE "-framework CoreGraphics" "-framework CoreVideo" "-framework QuartzCore" 
        "-framework GameController" "-framework Metal" "-framework ImageIO" "-framework OpenGL" "-framework AppKit")
    target_compile_options(GNXEditor PRIVATE "-fobjc-arc")
    set_target_properties(GNXEditor PROPERTIES LINK_FLAGS "-fobjc-arc")
endif()

target_link_libraries(GNXEditor PRIVATE Qt${QT_VERSION_MAJOR}::Widgets 
        baselib imagecodec mathutil rendercore rendersystem shadercompiler spirv_reflect)

set_target_properties(GNXEditor PROPERTIES
    MACOSX_BUNDLE_GUI_IDENTIFIER zhouxuguang.gnxengine.com
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    XCODE_ATTRIBUTE_CLANG_ENABLE_OBJC_ARC YES 
    WIN32_EXECUTABLE TRUE
)

#将引擎产生的中间文件放在构建目录的engine目录下
add_subdirectory(../project/cmake engine)

install(TARGETS GNXEditor
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
