

set(rendercore_dir ${CMAKE_CURRENT_SOURCE_DIR}/../../../source/RenderCore)
set(rendercore_include_dir ${CMAKE_CURRENT_SOURCE_DIR}/../../../include/RenderCore)

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
    file(GLOB_RECURSE RENDERCORE_SOURCES CONFIGURE_DEPENDS ${rendercore_dir}/*.cpp ${rendercore_dir}/*.c ${rendercore_dir}/*.h) 
endif()

file(GLOB_RECURSE RENDERCORE_INCLUDES CONFIGURE_DEPENDS ${rendercore_include_dir}/*.h)

add_library(rendercore OBJECT ${RENDERCORE_SOURCES} ${RENDERCORE_INCLUDES})
target_include_directories(rendercore PUBLIC ${rendercore_include_dir} ${CMAKE_CURRENT_SOURCE_DIR}/../../../include)
target_link_libraries(rendercore PRIVATE mathutil spirv_reflect)
target_compile_options(rendercore PRIVATE "-fobjc-arc")
#target_link_libraries(rendercore PRIVATE "-framework OpenGLES")



