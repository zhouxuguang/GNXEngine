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
  Application/EditorContext.h
  Application/EditorSettings.cpp Application/EditorSettings.h
  Project/EditorProjectService.cpp Project/EditorProjectService.h
  Assets/AssetImportService.cpp Assets/AssetImportService.h
  Viewport/EditorRenderHost.cpp Viewport/EditorRenderHost.h)
set_target_properties(GNXEditorCore PROPERTIES AUTOMOC ON)
target_link_libraries(GNXEditorCore PUBLIC Qt${QT_VERSION_MAJOR}::Core GNXEngine nanopb)
target_include_directories(GNXEditorCore PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/Application
  ${CMAKE_CURRENT_SOURCE_DIR}/Project
  ${CMAKE_CURRENT_SOURCE_DIR}/Assets
  ${CMAKE_CURRENT_SOURCE_DIR}/Viewport)

set(EDITOR_WIDGET_SOURCES
  Application/EditorApplication.cpp Application/EditorApplication.h
  MainWindow/mainwindow.cpp MainWindow/mainwindow.h MainWindow/mainwindow.ui
  Assets/ContentWidget.cpp Assets/ContentWidget.h
  Assets/TextureItemDelegate.cpp Assets/TextureItemDelegate.h
  Assets/TextureEditorDialog.cpp Assets/TextureEditorDialog.h
  Viewport/EditorViewportWidget.cpp Viewport/EditorViewportWidget.h
  Viewport/EditorInputAdapter.cpp Viewport/EditorInputAdapter.h
  Viewport/NativeSurface.cpp Viewport/NativeSurface.h
  Project/NewProjectDialog.cpp Project/NewProjectDialog.h
  Project/OpenProjectDialog.cpp Project/OpenProjectDialog.h)
if(APPLE)
  list(APPEND EDITOR_WIDGET_SOURCES Viewport/macUtils.h Viewport/macUtils.mm)
endif()
add_library(GNXEditorWidgets STATIC ${EDITOR_WIDGET_SOURCES})
set_target_properties(GNXEditorWidgets PROPERTIES AUTOUIC ON AUTOMOC ON AUTORCC ON)
target_link_libraries(GNXEditorWidgets PUBLIC GNXEditorCore Qt${QT_VERSION_MAJOR}::Widgets)
target_include_directories(GNXEditorWidgets PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/MainWindow)

add_executable(GNXEditor Application/main.cpp)
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
  add_executable(GNXEditorRenderSandbox Sandbox/SandboxMain.cpp)
  target_link_libraries(GNXEditorRenderSandbox PRIVATE GNXEditorCore)
  set_property(TARGET GNXEditorRenderSandbox PROPERTY FOLDER "Editor/Sandbox")
endif()

if(ENABLE_EDITOR_TESTS)
  enable_testing()
  add_executable(GNXEditorTests Tests/EditorTests.cpp)
  target_link_libraries(GNXEditorTests PRIVATE GNXEditorCore Qt${QT_VERSION_MAJOR}::Widgets)
  add_test(NAME GNXEditor.CoreSmoke COMMAND GNXEditorTests)
  set_property(TARGET GNXEditorTests PROPERTY FOLDER "Editor")
endif()

foreach(target GNXEditorCore GNXEditorWidgets GNXEditor)
  set_property(TARGET ${target} PROPERTY FOLDER "Editor")
endforeach()
install(TARGETS GNXEditor BUNDLE DESTINATION . LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
