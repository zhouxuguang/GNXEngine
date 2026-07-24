set(GKLIB_PATH "GKlib")
set(METIS_PATH ${CMAKE_CURRENT_SOURCE_DIR})

include(${GKLIB_PATH}/GKlibSystem.cmake)
# Add include directories.
include_directories(${GKLIB_PATH})
add_subdirectory("libmetis")
