
set(BASELIB_SOURCES )
set(baselib_dir ${CMAKE_CURRENT_SOURCE_DIR}/source)
set(baselib_include_dir ${CMAKE_CURRENT_SOURCE_DIR}/include)
file(GLOB_RECURSE BASELIB_SOURCES CONFIGURE_DEPENDS ${baselib_dir}/*.cpp ${baselib_dir}/*.c ${baselib_dir}/*.h)
file(GLOB_RECURSE BASELIB_INCLUDES CONFIGURE_DEPENDS ${baselib_include_dir}/*.h)

set(SOURCE_FILES ${BASELIB_SOURCES} ${BASELIB_INCLUDES})

if(LINUX)
    find_package(PkgConfig)
    pkg_check_modules(UUID REQUIRED uuid)
endif(LINUX)

add_library(BaseLib OBJECT ${SOURCE_FILES})

source_group(TREE ${baselib_dir} PREFIX source FILES ${BASELIB_SOURCES})
source_group(TREE ${baselib_include_dir} PREFIX include FILES ${BASELIB_INCLUDES})

target_include_directories(BaseLib PRIVATE ${baselib_include_dir})
if(WIN32)
    target_link_libraries(BaseLib PUBLIC zlib)
elseif(LINUX)
    target_link_libraries(BaseLib PRIVATE ${UUID_LIBRARIES})
endif()

set_property(TARGET BaseLib PROPERTY FOLDER "Runtime")
