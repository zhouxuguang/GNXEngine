
set(appframework_dir ${CMAKE_CURRENT_SOURCE_DIR}/source)
set(appframework_include_dir ${CMAKE_CURRENT_SOURCE_DIR}/include)
file(GLOB_RECURSE APPFRAMEWORK_SOURCES CONFIGURE_DEPENDS ${appframework_dir}/*.cpp ${appframework_dir}/*.c)
file(GLOB_RECURSE APPFRAMEWORK_INCLUDES CONFIGURE_DEPENDS ${appframework_include_dir}/*.h)

if(APPLE)
   list(APPEND APPFRAMEWORK_SOURCES ${appframework_dir}/MTLGLFWFramework.mm ${appframework_dir}/*.h)
else(WIN32)
    list(APPEND APPFRAMEWORK_SOURCES ${appframework_dir}/Win32GLFWFrameWork.h)
endif(APPLE)


set(SOURCE_FILES ${APPFRAMEWORK_SOURCES} ${APPFRAMEWORK_INCLUDES})
message("SOURCE_FILES: ${SOURCE_FILES}")
add_executable(APPFrameWork ${SOURCE_FILES})

source_group(TREE ${appframework_dir} PREFIX source FILES ${APPFRAMEWORK_SOURCES})
source_group(TREE ${appframework_include_dir} PREFIX include FILES ${APPFRAMEWORK_INCLUDES})

target_include_directories(APPFrameWork PRIVATE ${appframework_include_dir})

target_link_libraries(APPFrameWork PRIVATE RenderSystem glfw)

if(APPLE)
    target_link_libraries(APPFrameWork PRIVATE "-framework CoreGraphics" "-framework CoreVideo" "-framework QuartzCore" 
        "-framework GameController" "-framework Metal" "-framework ImageIO" "-framework OpenGL" "-framework AppKit")
    target_compile_options(APPFrameWork PRIVATE "-fobjc-arc")
    set_target_properties(APPFrameWork PROPERTIES LINK_FLAGS "-fobjc-arc")
endif()

set_property(TARGET APPFrameWork PROPERTY FOLDER "Runtime")
