
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

elseif(WIN32) #Windows平台：Vulkan + DX12
    file(GLOB RENDERCORE_SOURCES CONFIGURE_DEPENDS  ${rendercore_dir}/*.cpp 
                                                    ${rendercore_dir}/*.c 
                                                    ${rendercore_dir}/*.h
                                                    ${rendercore_dir}/vulkan/*.cpp
                                                    ${rendercore_dir}/vulkan/*.c
                                                    ${rendercore_dir}/vulkan/*.h
                                                    ${rendercore_dir}/dx12/*.cpp
                                                    ${rendercore_dir}/dx12/*.h)

elseif(LINUX) #Linux平台：仅 Vulkan
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

# vulkan-headers 路径：从 RenderCore 目录向上到仓库根（Engine/Runtime/RenderCore → 仓库根 = 3 层）
# 不用 PROJECT_SOURCE_DIR（在 Android 的 jni 子工程里会指向错误位置）
get_filename_component(GNX_REPO_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../.." ABSOLUTE)
target_include_directories(RenderCore PRIVATE ${rendercore_include_dir}
                                               ${GNX_REPO_ROOT}/ThirdParty/vulkan-headers
                                               ${GNX_REPO_ROOT}/ThirdParty/DirectX-Headers/include/directx
                                               )
target_compile_definitions(RenderCore PRIVATE RENDERCORE_EXPORTS)

# 平台宏必须以编译定义的形式下发到整个 RenderCore 目标（包括 volk.c）
# volk.c 只 include <vulkan/vulkan.h>，不会包含 VKRenderDefine.h；
# 若此处不定义，vulkan.h 不会 include vulkan_xlib.h / vulkan_win32.h 等，
# 导致 volk.c 里的 vkCreateXlibSurfaceKHR / vkCreateWin32SurfaceKHR 等符号不会被编译出来，
# 进而在链接阶段出现 undefined reference。
if(WIN32)
    target_compile_definitions(RenderCore PRIVATE VK_USE_PLATFORM_WIN32_KHR)
elseif(ANDROID)
    target_compile_definitions(RenderCore PRIVATE VK_USE_PLATFORM_ANDROID_KHR)
elseif(APPLE)
    target_compile_definitions(RenderCore PRIVATE VK_USE_PLATFORM_METAL_EXT)
elseif(UNIX AND NOT APPLE)
    # Linux 桌面：X11
    target_compile_definitions(RenderCore PRIVATE VK_USE_PLATFORM_XLIB_KHR)
endif()

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

# ---------------------------------------------------------------------------
# Direct3D 12 后端（仅 Windows）
#   d3d12        : D3D12 运行时（含 D3D12SerializeRootSignature / D3D12GetDebugInterface）
#   dxgi         : DXGI 工厂、适配器枚举、交换链
# ---------------------------------------------------------------------------
if(WIN32)
    # 打开 HRESULT 检查：失败时打印格式化错误并中断（见 DX12RenderDefine.h）
    target_compile_definitions(RenderCore PRIVATE GNX_DX12_RESULT_CHECK)

    target_link_libraries(RenderCore PUBLIC d3d12 dxgi)
endif()

if(APPLE)
    target_compile_options(RenderCore PRIVATE "-fobjc-arc")
    target_link_libraries(RenderCore PUBLIC "-framework QuartzCore" "-framework Metal" "-framework Foundation")
endif()

set_property(TARGET RenderCore PROPERTY FOLDER "Runtime")
