cmake_minimum_required(VERSION 3.17)

project(GNXEditor VERSION 0.1 LANGUAGES C CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(MSVC)
  add_compile_options(/wd4819)
  add_compile_options(/wd4828)
  add_compile_options("$<$<COMPILE_LANGUAGE:CXX>:/Zc:__cplusplus>")
  add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif()

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)

set(PROJECT_SOURCES
        main.cpp
        mainwindow.cpp
        mainwindow.h
        mainwindow.ui
        ContentWidget.h
        ContentWidget.cpp
        RenderWindowWidget.h
        RenderWindowWidget.cpp
        NewProjectDialog.h
        NewProjectDialog.cpp
        OpenProjectDialog.h
        OpenProjectDialog.cpp
        EditorConfig.h
        EditorConfig.cpp
        TestPost/TestPost.cpp
        TestPost/TestPost.hpp
        TestImageDecode.cpp
        TestImageDecode.h
        TestComputeShader.cpp
        TestComputeShader.hpp
        TestTransform.cpp
        TestTransform.hpp
        testSkybox.h
        testSkybox.cpp
        TestBlitEncoder.cpp
)

set(PLATFORM_SRC )
if(APPLE)
set(PLATFORM_SRC 
                macUtils.h
                macUtils.mm
                view.mm
                view.h
                MetalView.mm
                MetalView.h
                OpenGLView.mm
                OpenGLView.h
                RenderDelegate.mm
                RenderDelegate.h
                ResourceUtil.mm
                ResourceUtil.h
                testShader.h
                testShader.mm)
endif()

if(WIN32)
set(PLATFORM_SRC 
                view.cpp
                view.h
                WindowsVulkanView.cpp
                WindowsVulkanView.h)
endif()


add_executable(GNXEditor
    ${PROJECT_SOURCES} ${PLATFORM_SRC}
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

target_link_libraries(GNXEditor PRIVATE Qt${QT_VERSION_MAJOR}::Widgets GNXEngine nanopb)

set_target_properties(GNXEditor PROPERTIES
    MACOSX_BUNDLE_GUI_IDENTIFIER zhouxuguang.gnxengine.com
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    XCODE_ATTRIBUTE_CLANG_ENABLE_OBJC_ARC YES 
    WIN32_EXECUTABLE TRUE
)

set_property(TARGET GNXEditor PROPERTY FOLDER "Editor")
#set_property(TARGET GNXEditor_autogen PROPERTY FOLDER "Editor")

install(TARGETS GNXEditor
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
