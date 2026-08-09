# 查找 ISPC 编译器
# 优先用 fetch-deps.ps1 下载到 buildtools/ 的本地版本
# 路径: buildtools/ispc/{win|mac|linux}/ispc(.exe)
#
# 注意：不要用 CMAKE_SOURCE_DIR 定位，因为 Android/iOS 等子构建
# （如 demo/*/android/app/jni）会让 CMAKE_SOURCE_DIR 指向子目录，导致路径错位。
# 这里用 CMAKE_CURRENT_LIST_DIR（本文件所在 cmake/ 目录）推导项目根，
# 无论从哪个构建入口 include 都能正确定位到 ${项目根}/buildtools/ispc/...
get_filename_component(_gnx_project_root "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)

if(CMAKE_HOST_SYSTEM_NAME STREQUAL "Windows")
    set(_ispc_host_exe "${_gnx_project_root}/buildtools/ispc/win/ispc.exe")
elseif(CMAKE_HOST_SYSTEM_NAME STREQUAL "Darwin")
    set(_ispc_host_exe "${_gnx_project_root}/buildtools/ispc/mac/ispc")
else()
    set(_ispc_host_exe "${_gnx_project_root}/buildtools/ispc/linux/ispc")
endif()

# find_program(CMAKE_ISPC_COMPILER ${_ispc_exe}
#     PATHS
#         "${_ispc_host_exe}"
#     DOC "Intel ISPC compiler"
# )

set(CMAKE_ISPC_COMPILER ${_ispc_host_exe})

#message(STATUS "ISPC host exe: ${_ispc_host_exe}")
message(STATUS "ISPC found:    ${CMAKE_ISPC_COMPILER}")

if(NOT CMAKE_ISPC_COMPILER)
    message(FATAL_ERROR "ISPC compiler not found. Please install ISPC and set ISPC_HOME.")
endif()

# 定义 ISPC 编译函数
function(add_ispc_target ISPC_OUTPUT_FILES ISPC_HEADER_DIR)
    set(options)
    set(oneValueArgs TARGET OUTPUT_DIR)
    set(multiValueArgs SOURCES HEADER_DEPENDENCIES)
    cmake_parse_arguments(ISPC "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
    
    # 设置输出目录
    if(NOT ISPC_OUTPUT_DIR)
        set(ISPC_OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR})
    endif()

    # error : Unsupported value for --arch, supported values are: x86, x86-64, arm, aarch64, xe64 ispc
    # Apple 平台用 CMAKE_OSX_ARCHITECTURES，其他平台用 CMAKE_SYSTEM_PROCESSOR
    message(STATUS "ISPC arch detect: CMAKE_OSX_ARCHITECTURES=${CMAKE_OSX_ARCHITECTURES}, CMAKE_SYSTEM_PROCESSOR=${CMAKE_SYSTEM_PROCESSOR}")
    if(CMAKE_OSX_ARCHITECTURES MATCHES "arm64" OR CMAKE_SYSTEM_PROCESSOR MATCHES "^(aarch64|arm64|armv8)")
        set(ISPC_ARCH "aarch64")
        message(STATUS "ISPC: Building for ARM64 (aarch64) architecture")
    elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^(arm|ARM|armv7)")
        set(ISPC_ARCH "arm")
        message(STATUS "ISPC: Building for ARM32 (arm) architecture")
    elseif(CMAKE_OSX_ARCHITECTURES MATCHES "x86_64" OR CMAKE_SYSTEM_PROCESSOR MATCHES "^(x86_64|AMD64|amd64)")
        set(ISPC_ARCH "x86-64")
        message(STATUS "ISPC: Building for x86-64 architecture")
    elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^(i.86|x86)")
        set(ISPC_ARCH "x86")
        message(STATUS "ISPC: Building for x86 architecture")
    else()
        message(FATAL_ERROR "ISPC: Unsupported architecture. CMAKE_OSX_ARCHITECTURES=${CMAKE_OSX_ARCHITECTURES}, CMAKE_SYSTEM_PROCESSOR=${CMAKE_SYSTEM_PROCESSOR}")
    endif()

    # ISPC target OS: must match cross-compilation target, not host
    if(ANDROID)
        set(ISPC_TARGET_OS "android")
    elseif(IOS)
        set(ISPC_TARGET_OS "ios")
    elseif(APPLE)
        set(ISPC_TARGET_OS "macos")
    elseif(WIN32)
        set(ISPC_TARGET_OS "windows")
    elseif(UNIX)
        set(ISPC_TARGET_OS "linux")
    else()
        set(ISPC_TARGET_OS "linux")
    endif()
    message(STATUS "ISPC: target-os=${ISPC_TARGET_OS}")

    set(ISPC_KNOWN_TARGETS "sse2" "sse4" "avx1-" "avx2" "avx512skx" "avx512knl" "neon")
    
    # 创建目标文件列表，最后的文件列表
    set(ALL_ISPC_BUILD_OUTPUT_FILES)
    
    foreach(ispc_src ${ISPC_SOURCES})
        get_filename_component(ispc_name ${ispc_src} NAME_WE)

        # 生成目标文件名
        set(ISPC_HEADER_NAME "${CMAKE_CURRENT_BINARY_DIR}/${ispc_name}_ispc.h")
        set(ISPC_OBJ_NAME "${CMAKE_CURRENT_BINARY_DIR}/${ispc_name}.ispc${CMAKE_CXX_OUTPUT_EXTENSION}")

        set(ISPC_BUILD_OUTPUT)
        list(APPEND ISPC_BUILD_OUTPUT ${ISPC_HEADER_NAME} ${ISPC_OBJ_NAME})

        string(FIND "${ISPC_TARGET}" "," MULTI_TARGET)
        if (${MULTI_TARGET} GREATER -1)
            foreach (ispc_target ${ISPC_KNOWN_TARGETS})
                string(FIND ${ISPC_TARGET} ${ispc_target} FOUND_TARGET)
                if (${FOUND_TARGET} GREATER -1)
                    set(OUTPUT_TARGET ${ispc_target})
                    if (${ispc_target} STREQUAL "avx1-")
                        set(OUTPUT_TARGET "avx")
                    endif()
                    list(APPEND ISPC_BUILD_OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${ispc_name}_ispc_${OUTPUT_TARGET}.h"
                                "${CMAKE_CURRENT_BINARY_DIR}/${ispc_name}.ispc_${OUTPUT_TARGET}${CMAKE_CXX_OUTPUT_EXTENSION}")
                endif()
            endforeach()
        endif()
        
        # 添加编译命令
        add_custom_command(
            OUTPUT ${ISPC_BUILD_OUTPUT}
            COMMAND ${CMAKE_ISPC_COMPILER}
                ${ispc_src}
                -o ${ISPC_OBJ_NAME}
                -h ${ISPC_HEADER_NAME}
                --arch=${ISPC_ARCH}
                --target=${ISPC_TARGET}
                --target-os=${ISPC_TARGET_OS}
                --pic
                ${ISPC_FLAGS}
            DEPENDS ${ispc_src} ${ISPC_HEADER_DEPENDENCIES}
            COMMENT "Compiling ISPC file: ${ispc_src}"
            VERBATIM
        )
    
        # 7. 收集到全局列表（用于最终目标）
        list(APPEND ALL_ISPC_BUILD_OUTPUT_FILES ${ISPC_BUILD_OUTPUT})
    endforeach()
    
    # 返回目标文件列表
    set(${ISPC_OUTPUT_FILES} ${ALL_ISPC_BUILD_OUTPUT_FILES} PARENT_SCOPE)
    set(${ISPC_HEADER_DIR} ${ISPC_OUTPUT_DIR} PARENT_SCOPE)
endfunction()