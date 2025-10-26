# 查找 ISPC 编译器
find_program(CMAKE_ISPC_COMPILER ispc
    PATHS
        "$ENV{ISPC_HOME}/bin"
        "$ENV{HOME}/ispc/bin"
        "/usr/local/bin"
    DOC "Intel ISPC compiler"
)

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
    # CMAKE_SYSTEM_PROCESSOR在windows上输出AMD64
    # 检查目标处理器架构
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|AMD64")
        set(ISPC_ARCH "x86-64")
        message(STATUS "Building for x86-64 architecture")
    elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "arm|ARM")
        set(ISPC_ARCH "aarch64")
        message(STATUS "Building for ARM architecture")
    endif()

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