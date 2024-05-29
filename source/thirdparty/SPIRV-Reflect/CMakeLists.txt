add_library(spirv_reflect OBJECT spirv_reflection.c spirv_reflection.h)
target_include_directories(spirv_reflect PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
