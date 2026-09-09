cmake_minimum_required(VERSION 3.17)
project(GNXEditor VERSION 0.1 LANGUAGES C CXX)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

option(ENABLE_EDITOR_SANDBOX "Build editor rendering sandbox" OFF)
option(ENABLE_EDITOR_TESTS "Build editor unit tests" ${ENABLE_TESTING})

if(MSVC)
  add_compile_options(/wd4819 /wd4828 "$<$<COMPILE_LANGUAGE:CXX>:/Zc:__cplusplus>")
  add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif()

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)

add_library(GNXEditorCore STATIC
  EditorContext.h
  EditorProjectService.cpp EditorProjectService.h
  EditorRenderHost.cpp EditorRenderHost.h
  EditorSettings.cpp EditorSettings.h
  AssetImportService.cpp AssetImportService.h)
set_target_properties(GNXEditorCore PROPERTIES AUTOMOC ON)
target_link_libraries(GNXEditorCore PUBLIC Qt${QT_VERSION_MAJOR}::Widgets GNXEngine nanopb)
target_include_directories(GNXEditorCore PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

set(EDITOR_WIDGET_SOURCES
  EditorApplication.cpp EditorApplication.h
  mainwindow.cpp mainwindow.h mainwindow.ui
  ContentWidget.cpp ContentWidget.h
  TextureItemDelegate.cpp TextureItemDelegate.h
  TextureEditorDialog.cpp TextureEditorDialog.h
  EditorViewportWidget.cpp EditorViewportWidget.h
  EditorInputAdapter.cpp EditorInputAdapter.h NativeSurface.cpp NativeSurface.h
  NewProjectDialog.cpp NewProjectDialog.h OpenProjectDialog.cpp OpenProjectDialog.h)
if(APPLE)
  list(APPEND EDITOR_WIDGET_SOURCES macUtils.h macUtils.mm)
endif()
add_library(GNXEditorWidgets STATIC ${EDITOR_WIDGET_SOURCES})
set_target_properties(GNXEditorWidgets PROPERTIES AUTOUIC ON AUTOMOC ON AUTORCC ON)
target_link_libraries(GNXEditorWidgets PUBLIC GNXEditorCore Qt${QT_VERSION_MAJOR}::Widgets)
target_include_directories(GNXEditorWidgets PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(GNXEditor main.cpp)
set_target_properties(GNXEditor PROPERTIES AUTOMOC ON AUTORCC ON
  MACOSX_BUNDLE_GUI_IDENTIFIER zhouxuguang.gnxengine.com
  MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
  MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
  MACOSX_BUNDLE TRUE WIN32_EXECUTABLE TRUE)
target_link_libraries(GNXEditor PRIVATE GNXEditorWidgets)

if(APPLE)
  foreach(target GNXEditorCore GNXEditorWidgets GNXEditor)
    target_link_libraries(${target} PRIVATE "-framework CoreGraphics" "-framework CoreVideo" "-framework QuartzCore"
      "-framework GameController" "-framework Metal" "-framework ImageIO" "-framework OpenGL" "-framework AppKit")
  endforeach()
  target_compile_options(GNXEditorWidgets PRIVATE "-fobjc-arc")
endif()

if(ENABLE_EDITOR_SANDBOX)
  set(SANDBOX_SOURCES View.h TestPost/TestPost.cpp TestPost/TestPost.hpp
    TestImageDecode.cpp TestImageDecode.h TestComputeShader.cpp TestComputeShader.hpp
    TestTransform.cpp TestTransform.hpp testSkybox.cpp testSkybox.h TestBlitEncoder.cpp)
  if(WIN32)
    list(APPEND SANDBOX_SOURCES View.cpp WindowsVulkanView.cpp WindowsVulkanView.h)
  elseif(APPLE)
    list(APPEND SANDBOX_SOURCES view.mm MetalView.mm MetalView.h OpenGLView.mm OpenGLView.h
      RenderDelegate.mm RenderDelegate.h ResourceUtil.mm ResourceUtil.h testShader.mm testShader.h)
  endif()
  add_library(GNXEditorRenderSandbox STATIC ${SANDBOX_SOURCES})
  target_link_libraries(GNXEditorRenderSandbox PRIVATE GNXEditorCore Qt${QT_VERSION_MAJOR}::Widgets)
  set_target_properties(GNXEditorRenderSandbox PROPERTIES AUTOMOC ON)
endif()

if(ENABLE_EDITOR_TESTS)
  enable_testing()
  add_executable(GNXEditorTests EditorTests.cpp)
  target_link_libraries(GNXEditorTests PRIVATE GNXEditorCore Qt${QT_VERSION_MAJOR}::Widgets)
  add_test(NAME GNXEditor.CoreSmoke COMMAND GNXEditorTests)
  set_property(TARGET GNXEditorTests PROPERTY FOLDER "Editor")
endif()

foreach(target GNXEditorCore GNXEditorWidgets GNXEditor)
  set_property(TARGET ${target} PROPERTY FOLDER "Editor")
endforeach()
install(TARGETS GNXEditor BUNDLE DESTINATION . LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
