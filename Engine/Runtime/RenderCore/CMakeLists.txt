

set(rendercore_dir ${CMAKE_CURRENT_SOURCE_DIR}/source)
set(rendercore_include_dir ${CMAKE_CURRENT_SOURCE_DIR}/include)

if(APPLE)
    if(APPLE AND  NOT IOS)  #mac平台
        file(GLOB RENDERCORE_SOURCES CONFIGURE_DEPENDS ${rendercore_dir}/*.cpp 
                                                        ${rendercore_dir}/*.c 
                                                        ${rendercore_dir}/*.h
                                                        ${rendercore_dir}/metal/*.mm
                                                        ${rendercore_dir}/metal/*.h
                                                        ${rendercore_dir}/vulkan/*.cpp
                                                        ${rendercore_dir}/vulkan/*.c
                                                        ${rendercore_dir}/vulkan/*.h) 
    else()     #ios平台
        file(GLOB RENDERCORE_SOURCES CONFIGURE_DEPENDS ${rendercore_dir}/*.cpp 
                                                        ${rendercore_dir}/*.c 
                                                        ${rendercore_dir}/*.h
                                                        ${rendercore_dir}/metal/*.mm
                                                        ${rendercore_dir}/metal/*.h
                                                        ${rendercore_dir}/gles/*.cpp
                                                        ${rendercore_dir}/gles/*.h
                                                        ${rendercore_dir}/gles/*.c) 
    endif()
    
elseif(ANDROID) #android平台
    file(GLOB RENDERCORE_SOURCES CONFIGURE_DEPENDS  ${rendercore_dir}/*.cpp 
                                                    ${rendercore_dir}/*.c 
                                                    ${rendercore_dir}/*.h
                                                    ${rendercore_dir}/vulkan/*.cpp
                                                    ${rendercore_dir}/vulkan/*.c
                                                    ${rendercore_dir}/vulkan/*.h) 

elseif(WIN32 OR LINUX) #WINDOWS平台和Linux
    file(GLOB RENDERCORE_SOURCES CONFIGURE_DEPENDS  ${rendercore_dir}/*.cpp 
                                                    ${rendercore_dir}/*.c 
                                                    ${rendercore_dir}/*.h
                                                    ${rendercore_dir}/vulkan/*.cpp
                                                    ${rendercore_dir}/vulkan/*.c
                                                    ${rendercore_dir}/vulkan/*.h) 
endif()

file(GLOB_RECURSE RENDERCORE_INCLUDES CONFIGURE_DEPENDS ${rendercore_include_dir}/*.h)

set(SOURCE_FILES ${RENDERCORE_SOURCES} ${RENDERCORE_INCLUDES})

add_library(RenderCore STATIC ${SOURCE_FILES})

source_group(TREE ${rendercore_dir} PREFIX source FILES ${RENDERCORE_SOURCES})
source_group(TREE ${rendercore_include_dir} PREFIX include FILES ${RENDERCORE_INCLUDES})

target_include_directories(RenderCore PRIVATE ${rendercore_include_dir})
target_link_libraries(RenderCore PRIVATE MathUtil spirv_reflect)

if(APPLE)
    target_compile_options(RenderCore PRIVATE "-fobjc-arc")
    #target_link_libraries(RenderCore PRIVATE "-framework OpenGLES")
endif()

set_property(TARGET RenderCore PROPERTY FOLDER "Runtime")
