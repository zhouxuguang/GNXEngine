

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
                                                        ${rendercore_dir}/metal/*.h) 
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

add_library(RenderCore OBJECT ${SOURCE_FILES})

source_group(TREE ${rendercore_dir} PREFIX source FILES ${RENDERCORE_SOURCES})
source_group(TREE ${rendercore_include_dir} PREFIX include FILES ${RENDERCORE_INCLUDES})

target_include_directories(RenderCore PRIVATE ${rendercore_include_dir}
                                               ${PROJECT_SOURCE_DIR}/ThirdParty/vulkan-headers)
target_compile_definitions(RenderCore PRIVATE RENDERCORE_EXPORTS)

# NVIDIA Nsight Aftermath GPU 崩溃转储集成
if(ENABLE_NSIGHT_AFTERMATH)
    target_compile_definitions(RenderCore PRIVATE ENABLE_NSIGHT_AFTERMATH)
    target_include_directories(RenderCore PRIVATE ${PROJECT_SOURCE_DIR}/ThirdParty/AftermathSDK/include)
    # 编译完成后将 Aftermath DLL 复制到可执行文件同一目录（Debug/Release）
    add_custom_target(CopyAftermathDLL ALL
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${PROJECT_SOURCE_DIR}/ThirdParty/AftermathSDK/lib/GFSDK_Aftermath_Lib.x64.dll"
            "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$<CONFIG>"
        COMMENT "Copying GFSDK_Aftermath_Lib.x64.dll to output directory"
    )
    add_dependencies(RenderCore CopyAftermathDLL)
endif()

target_link_libraries(RenderCore PRIVATE MathUtil spirv_reflect TracyClient)

if(APPLE)
    target_compile_options(RenderCore PRIVATE "-fobjc-arc")
    target_link_libraries(RenderCore PUBLIC "-framework QuartzCore" "-framework Metal")
endif()

set_property(TARGET RenderCore PROPERTY FOLDER "Runtime")
