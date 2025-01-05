
add_library(tinyktx2 STATIC tinyktx2.h tinyktx2.cpp)

target_include_directories(tinyktx2 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set_property(TARGET tinyktx2 PROPERTY FOLDER "thirdparty")