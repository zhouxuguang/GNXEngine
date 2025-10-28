
set(allocator_process_dir ${CMAKE_CURRENT_SOURCE_DIR}/source)
set(allocator_process_include_dir ${CMAKE_CURRENT_SOURCE_DIR}/include)
file(GLOB_RECURSE ALLOCATOR_PROCESS_SOURCES CONFIGURE_DEPENDS ${allocator_process_dir}/*.cpp 
    ${allocator_process_dir}/*.c ${allocator_process_dir}/*.h)
file(GLOB_RECURSE ALLOCATOR_PROCESS_INCLUDES CONFIGURE_DEPENDS ${allocator_process_include_dir}/*.h)

set(SOURCE_FILES ${ALLOCATOR_PROCESS_SOURCES} ${ALLOCATOR_PROCESS_INCLUDES})

add_library(Allocator STATIC ${SOURCE_FILES})

source_group(TREE ${allocator_process_dir} PREFIX source FILES ${ALLOCATOR_PROCESS_SOURCES})
source_group(TREE ${allocator_process_include_dir} PREFIX include FILES ${ALLOCATOR_PROCESS_INCLUDES})

target_include_directories(Allocator PUBLIC ${allocator_process_include_dir})

target_link_libraries(Allocator PRIVATE BaseLib tbb tbbmalloc)

set_property(TARGET Allocator PROPERTY FOLDER "Runtime")
