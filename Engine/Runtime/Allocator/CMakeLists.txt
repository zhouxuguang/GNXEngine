
set(allocator_process_dir ${CMAKE_CURRENT_SOURCE_DIR}/source)
set(allocator_process_include_dir ${CMAKE_CURRENT_SOURCE_DIR}/include)
file(GLOB_RECURSE ALLOCATOR_PROCESS_SOURCES CONFIGURE_DEPENDS ${allocator_process_dir}/*.cpp 
    ${allocator_process_dir}/*.c ${allocator_process_dir}/*.h)
file(GLOB_RECURSE ALLOCATOR_PROCESS_INCLUDES CONFIGURE_DEPENDS ${allocator_process_include_dir}/*.h)

list(FILTER ALLOCATOR_PROCESS_SOURCES EXCLUDE REGEX "AllocatorNewDelete\\.cpp$")

set(SOURCE_FILES ${ALLOCATOR_PROCESS_SOURCES} ${ALLOCATOR_PROCESS_INCLUDES})

add_library(Allocator OBJECT ${SOURCE_FILES})

target_compile_definitions(Allocator PRIVATE ALLOCATOR_EXPORTS)

source_group(TREE ${allocator_process_dir} PREFIX source FILES ${ALLOCATOR_PROCESS_SOURCES})
source_group(TREE ${allocator_process_include_dir} PREFIX include FILES ${ALLOCATOR_PROCESS_INCLUDES})

target_include_directories(Allocator PUBLIC ${allocator_process_include_dir})

target_link_libraries(Allocator PRIVATE BaseLib tbb tbbmalloc mimalloc)

set_property(TARGET Allocator PROPERTY FOLDER "Runtime")

# 全局 operator new/delete 替换（静态库，由 GNXEngine 以 PUBLIC 传播给各可执行模块）
if(GNX_ALLOCATOR_OVERRIDE_NEW)
    add_library(AllocatorOverride STATIC ${allocator_process_dir}/AllocatorNewDelete.cpp)
    target_include_directories(AllocatorOverride PRIVATE ${allocator_process_include_dir})
    target_link_libraries(AllocatorOverride PRIVATE BaseLib)
    set_property(TARGET AllocatorOverride PROPERTY FOLDER "Runtime")
endif()
