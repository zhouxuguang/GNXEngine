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
function(add_ispc_target)
    set(options)
    set(oneValueArgs TARGET OUTPUT_DIR)
    set(multiValueArgs SOURCES HEADER_DEPENDENCIES)
    cmake_parse_arguments(ISPC "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
    
    # 设置输出目录
    if(NOT ISPC_OUTPUT_DIR)
        set(ISPC_OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR})
    endif()
    
    # 创建目标文件列表
    set(OBJECT_FILES)
    
    foreach(ispc_src ${ISPC_SOURCES})
        get_filename_component(ispc_name ${ispc_src} NAME_WE)
        
        # 生成目标文件名
        set(obj_file ${ISPC_OUTPUT_DIR}/${ispc_name}.o)
        set(h_file ${ISPC_OUTPUT_DIR}/${ispc_name}_ispc.h)
        
        # 添加编译命令
        add_custom_command(
            OUTPUT ${obj_file} ${h_file}
            COMMAND ${CMAKE_ISPC_COMPILER}
                ${ispc_src}
                -o ${obj_file}
                -h ${h_file}
                --arch=${CMAKE_SYSTEM_PROCESSOR}
                --target=${ISPC_TARGET}
                ${ISPC_FLAGS}
            DEPENDS ${ispc_src} ${ISPC_HEADER_DEPENDENCIES}
            COMMENT "Compiling ISPC file: ${ispc_src}"
        )
        
        list(APPEND OBJECT_FILES ${obj_file})
    endforeach()
    
    # 返回目标文件列表
    set(OBJECT_FILES ${OBJECT_FILES} PARENT_SCOPE)
endfunction()